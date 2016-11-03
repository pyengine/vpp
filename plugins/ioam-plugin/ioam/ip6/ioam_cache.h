/*
 * Copyright (c) 2016 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __included_ioam_cache_h__
#define __included_ioam_cache_h__

#include <vnet/vnet.h>
#include <vnet/ip/ip.h>
#include <vnet/ip/ip_packet.h>
#include <vnet/ip/ip4_packet.h>
#include <vnet/ip/ip6_packet.h>
#include <vnet/ip/udp.h>

#include <vppinfra/pool.h>
#include <vppinfra/hash.h>
#include <vppinfra/error.h>
#include <vppinfra/elog.h>
#include <vppinfra/bihash_8_8.h>


#define MAX_CACHE_ENTRIES 1024
#define IOAM_CACHE_TABLE_DEFAULT_HASH_NUM_BUCKETS (4 * 1024)
#define IOAM_CACHE_TABLE_DEFAULT_HASH_MEMORY_SIZE (2<<20)


typedef struct {
  ip6_address_t src_address;
  ip6_address_t dst_address;
  u16 src_port;
  u16 dst_port;
  u8 protocol;
  u32 seq_no;
  u8 *ioam_rewrite_string;
} ioam_cache_entry_t;

typedef struct {
  ip6_address_t src_address;
  ip6_address_t dst_address;
  u16 src_port;
  u16 dst_port;
  u8 protocol;
  u32 seq_no;
  u32 buffer_index;
} ioam_server_select_entry_t;

typedef struct {
  /* API message ID base */
  u16 msg_id_base;

  /* Pool of ioam_cache_buffer_t */
  ioam_cache_entry_t *ioam_rewrite_pool;

  u64 lookup_table_nbuckets;
  u64 lookup_table_size;
  clib_bihash_8_8_t ioam_rewrite_cache_table;


  /* Lock per thread to swap buffers between worker and timer process*/
  volatile u32 **lockp;

  /* time scale transform*/
  u32 unix_time_0;
  f64 vlib_time_0;

  /* convenience */
  vlib_main_t * vlib_main;
  vnet_main_t * vnet_main;

  uword my_hbh_slot;
  u32 export_process_node_index;
} ioam_cache_main_t;

ioam_cache_main_t ioam_cache_main;

vlib_node_registration_t ioam_cache_node;


/* Compute flow hash.  We'll use it to select which Sponge to use for this
   flow.  And other things. */
always_inline u32
ip6_compute_flow_hash_ext (const ip6_header_t * ip,
			   u8 protocol,
			   u16 src_port,
			   u16 dst_port,
		       flow_hash_config_t flow_hash_config)
{
    u64 a, b, c;
    u64 t1, t2;
    
    t1 = (ip->src_address.as_u64[0] ^ ip->src_address.as_u64[1]);
    t1 = (flow_hash_config & IP_FLOW_HASH_SRC_ADDR) ? t1 : 0;

    t2 = (ip->dst_address.as_u64[0] ^ ip->dst_address.as_u64[1]);
    t2 = (flow_hash_config & IP_FLOW_HASH_DST_ADDR) ? t2 : 0;

    a = (flow_hash_config & IP_FLOW_HASH_REVERSE_SRC_DST) ? t2 : t1;
    b = (flow_hash_config & IP_FLOW_HASH_REVERSE_SRC_DST) ? t1 : t2;
    b ^= (flow_hash_config & IP_FLOW_HASH_PROTO) ? protocol : 0;

    t1 = src_port; 
    t2 = dst_port;

    t1 = (flow_hash_config & IP_FLOW_HASH_SRC_PORT) ? t1 : 0;
    t2 = (flow_hash_config & IP_FLOW_HASH_DST_PORT) ? t2 : 0;

    c = (flow_hash_config & IP_FLOW_HASH_REVERSE_SRC_DST) ?
        ((t1<<16) | t2) : ((t2<<16) | t1);

    hash_mix64 (a, b, c);
    return (u32) c;
}

inline static void ioam_cache_entry_free (ioam_cache_entry_t *entry)
{
  ioam_cache_main_t *cm = &ioam_cache_main;
  if (entry)
    {
      vec_free(entry->ioam_rewrite_string);
      memset(entry, 0, sizeof(*entry)); 
      pool_put(cm->ioam_rewrite_pool, entry);
    }  
}

inline static ioam_cache_entry_t * ioam_cache_entry_cleanup (u32 pool_index)
{
  ioam_cache_main_t *cm = &ioam_cache_main;
  ioam_cache_entry_t *entry = 0;

  entry = pool_elt_at_index(cm->ioam_rewrite_pool, pool_index);
  ioam_cache_entry_free(entry);
  return(0);
}

inline static ioam_cache_entry_t * ioam_cache_lookup (ip6_header_t *ip0,
                                  u16 src_port,
				  u16 dst_port,
				  u32 seq_no)
{
  ioam_cache_main_t *cm = &ioam_cache_main;
  u32 flow_hash = ip6_compute_flow_hash_ext(ip0, ip0->protocol,
					    src_port, dst_port,
					    IP_FLOW_HASH_DEFAULT | IP_FLOW_HASH_REVERSE_SRC_DST);
  clib_bihash_kv_8_8_t kv, value;
  
  kv.key = (u64)flow_hash << 32 | seq_no;
  kv.value = 0;
  value.key = 0;
  value.value = 0;

  if (clib_bihash_search_8_8(&cm->ioam_rewrite_cache_table, &kv, &value) >= 0)
    {
      ioam_cache_entry_t *entry = 0;
      
      entry = pool_elt_at_index(cm->ioam_rewrite_pool, value.value);
      /* match */
      if (ip6_address_compare(&ip0->src_address, &entry->dst_address) == 0 &&
	  ip6_address_compare(&ip0->dst_address, &entry->src_address) == 0 &&
	  entry->src_port == dst_port &&
	  entry->dst_port == src_port &&
	  entry->seq_no == seq_no)
	{
	  /* If lookup is successful remove it from the hash */
	  clib_bihash_add_del_8_8(&cm->ioam_rewrite_cache_table, &kv, 0);
	  return(entry);
	}
      else
	return(0);
      
    }
  return(0);
}

inline static int ioam_cache_add (ip6_header_t *ip0,
                                  u16 src_port,
				  u16 dst_port,
				  ip6_hop_by_hop_header_t *hbh0,
				  u32 seq_no)
{
  ioam_cache_main_t *cm = &ioam_cache_main;
  ioam_cache_entry_t *entry = 0;
  u32 rewrite_len = 0;
  u32 pool_index = 0;
  
  pool_get_aligned(cm->ioam_rewrite_pool, entry, CLIB_CACHE_LINE_BYTES);
  memset (entry, 0, sizeof (*entry));
  pool_index = entry - cm->ioam_rewrite_pool;
  
  clib_memcpy(entry->dst_address.as_u64, ip0->dst_address.as_u64, sizeof(ip6_address_t));
  clib_memcpy(entry->src_address.as_u64, ip0->src_address.as_u64, sizeof(ip6_address_t));
  entry->src_port = src_port;
  entry->dst_port = dst_port;
  entry->seq_no = seq_no;
  rewrite_len = ((hbh0->length + 1) << 3);
  vec_validate(entry->ioam_rewrite_string, rewrite_len);
  clib_memcpy(entry->ioam_rewrite_string, hbh0, rewrite_len);

  /* add it to hash, replacing and freeing any collision for now */
  u32 flow_hash = ip6_compute_flow_hash_ext(ip0, hbh0->protocol, src_port, dst_port,
					    IP_FLOW_HASH_DEFAULT);
  clib_bihash_kv_8_8_t kv, value;
  kv.key = (u64)flow_hash << 32 | seq_no;
  kv.value = 0;
  if (clib_bihash_search_8_8(&cm->ioam_rewrite_cache_table, &kv, &value) >= 0)
    {
      /* replace */
      ioam_cache_entry_cleanup(value.value);
    }
  kv.value = pool_index;
  clib_bihash_add_del_8_8(&cm->ioam_rewrite_cache_table, &kv, 1);
  return(0);
}

inline static int ioam_cache_table_init (vlib_main_t *vm)
{
  ioam_cache_main_t *cm = &ioam_cache_main;

  pool_alloc_aligned(cm->ioam_rewrite_pool,
		      MAX_CACHE_ENTRIES,
		      CLIB_CACHE_LINE_BYTES);
  cm->lookup_table_nbuckets = IOAM_CACHE_TABLE_DEFAULT_HASH_NUM_BUCKETS;
  cm->lookup_table_nbuckets = 1 << max_log2 (cm->lookup_table_nbuckets);
  cm->lookup_table_size = IOAM_CACHE_TABLE_DEFAULT_HASH_MEMORY_SIZE;

  clib_bihash_init_8_8(&cm->ioam_rewrite_cache_table,
	               "ioam rewrite cache table",
		       cm->lookup_table_nbuckets, cm->lookup_table_size);
  return(1);
}

inline static int ioam_cache_table_destroy (vlib_main_t *vm)
{
  ioam_cache_main_t *cm = &ioam_cache_main;
  ioam_cache_entry_t *entry = 0;  
  /* free pool and hash table */
  clib_bihash_free_8_8(&cm->ioam_rewrite_cache_table);
  pool_foreach (entry, cm->ioam_rewrite_pool,
		({
		  ioam_cache_entry_free(entry);
		}));
  pool_free(cm->ioam_rewrite_pool);
  cm->ioam_rewrite_pool = 0;
  return(0);
}

inline static u8 * format_ioam_cache_entry (u8 * s, va_list * args)
{
  ioam_cache_entry_t *e = va_arg (*args, ioam_cache_entry_t *);
  ioam_cache_main_t *cm = &ioam_cache_main;
  
  s = format (s, "%d: %U:%d to  %U:%d seq_no %lu\n",
	      (e - cm->ioam_rewrite_pool),
	      format_ip6_address, &e->src_address,
	      e->src_port,
	      format_ip6_address, &e->dst_address,
	      e->dst_port,
	      e->seq_no);
  s = format (s, "  %U",
	      format_ip6_hop_by_hop_ext_hdr,
	      (ip6_hop_by_hop_header_t *)e->ioam_rewrite_string,
	      vec_len(e->ioam_rewrite_string)-1);
  return s;
}

#endif /* __included_ioam_cache_h__ */
