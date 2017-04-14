/*
 *------------------------------------------------------------------
 * api_handler.c - message handler for grpc server
 *
 * Copyright (c) 2010-2016 Cisco and/or its affiliates.
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
 *------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#include <vppinfra/clib.h>
#include <vppinfra/vec.h>
#include <vppinfra/hash.h>
#include <vppinfra/bitmap.h>
#include <vppinfra/fifo.h>
#include <vppinfra/time.h>
#include <vppinfra/mheap.h>
#include <vppinfra/heap.h>
#include <vppinfra/pool.h>
#include <vppinfra/format.h>
#include <vppinfra/error.h>

#include <vnet/api_errno.h>
#include <vnet/vnet.h>

#include <vnet/fib/fib_table.h>
#include <vnet/fib/ip6_fib.h>
#include <vnet/fib/ip4_fib.h>
#include <vnet/dpo/drop_dpo.h>
#include <vnet/dpo/receive_dpo.h>
#include <vnet/dpo/lookup_dpo.h>
#include <vnet/dpo/classify_dpo.h>
#include <vnet/dpo/ip_null_dpo.h>
#define vl_typedefs             /* define message structures */
#include <vpp/api/vpe_all_api_h.h>
#undef vl_typedefs

extern vlib_main_t *api_server_get_vlib_main(void);
extern u32 api_server_process_node_index(void);

static void
inband_cli_output (uword arg, u8 * buffer, uword buffer_bytes)
{
  u8 **mem_vecp = (u8 **) arg;
  u8 *mem_vec = *mem_vecp;
  u32 offset = vec_len (mem_vec);

  vec_validate (mem_vec, offset + buffer_bytes - 1);
  clib_memcpy (mem_vec + offset, buffer, buffer_bytes);
  *mem_vecp = mem_vec;
}

int
cli_handler (const char *cli, u32 cli_length,
		u8 **reply, u32 *reply_length)
{
  vlib_main_t *vm = api_server_get_vlib_main ();
  unformat_input_t input;

  /*
   * CLI input/output functions are looked up from
   * current process node. This is run in thread context.
   * So setting the current node index here
   */

  while (__sync_lock_test_and_set (vm->main_lockp, 1))
    ;

  vm->node_main.current_process_index = api_server_process_node_index();

  *reply = 0;
  unformat_init_string (&input, (char *)cli, cli_length);
  vlib_cli_input (vm, &input, inband_cli_output, (uword) reply);
  vm->node_main.current_process_index = ~0;
  *reply_length = vec_len (*reply);
  *vm->main_lockp = 0;
  return(0);
}

void vector_free (u8 *vec)
{
  vec_free (vec);
}
static void
copy_fib_next_hop (fib_route_path_encode_t * api_rpath,
		   vl_api_fib_path_t * fp)
{
  int is_ip4;

  if (api_rpath->rpath.frp_proto == FIB_PROTOCOL_IP4)
    fp->afi = IP46_TYPE_IP4;
  else if (api_rpath->rpath.frp_proto == FIB_PROTOCOL_IP6)
    fp->afi = IP46_TYPE_IP6;
  else
    {
      is_ip4 = ip46_address_is_ip4 (&api_rpath->rpath.frp_addr);
      if (is_ip4)
	fp->afi = IP46_TYPE_IP4;
      else
	fp->afi = IP46_TYPE_IP6;
    }
  if (fp->afi == IP46_TYPE_IP4)
    memcpy (fp->next_hop, &api_rpath->rpath.frp_addr.ip4,
	    sizeof (api_rpath->rpath.frp_addr.ip4));
  else
    memcpy (fp->next_hop, &api_rpath->rpath.frp_addr.ip6,
	    sizeof (api_rpath->rpath.frp_addr.ip6));
}

extern void ip6_fib_writer(void *, vl_api_ip6_fib_details_t *, vl_api_fib_path_t *path);

static void
send_ip6_fib_details ( u32 table_id, fib_prefix_t *pfx,
                      fib_route_path_encode_t *api_rpaths, void *writer)
{
  vl_api_ip6_fib_details_t fib_details, *mp;
  vl_api_fib_path_t path[20];
  fib_route_path_encode_t *api_rpath;
  vl_api_fib_path_t *fp;
  int path_count;

  mp = &fib_details;
  path_count = vec_len(api_rpaths);
  mp->table_id = table_id;
  mp->address_length = pfx->fp_len;
  memcpy(mp->address, &pfx->fp_addr.ip6, sizeof(pfx->fp_addr.ip6));

  mp->count = path_count;
  fp = &path[0];
  vec_foreach(api_rpath, api_rpaths)
  {
    memset (fp, 0, sizeof (*fp));
    switch (api_rpath->dpo.dpoi_type)
      {
      case DPO_RECEIVE:
	fp->is_local = true;
	break;
      case DPO_DROP:
	fp->is_drop = true;
	break;
      case DPO_IP_NULL:
	switch (api_rpath->dpo.dpoi_index)
	  {
	  case IP_NULL_DPO_ACTION_NUM+IP_NULL_ACTION_NONE:
	    fp->is_drop = true;
	    break;
	  case IP_NULL_DPO_ACTION_NUM+IP_NULL_ACTION_SEND_ICMP_UNREACH:
	    fp->is_unreach = true;
	    break;
	  case IP_NULL_DPO_ACTION_NUM+IP_NULL_ACTION_SEND_ICMP_PROHIBIT:
	    fp->is_prohibit = true;
	    break;
	  default:
	    break;
	  }
	break;
      default:
	break;
      }
    fp->weight = api_rpath->rpath.frp_weight;
    fp->sw_if_index = api_rpath->rpath.frp_sw_if_index;
    copy_fib_next_hop (api_rpath, fp);
    fp++;
  }
  ip6_fib_writer(writer,mp, &path[0]);
}

typedef struct apt_ip6_fib_show_ctx_t_ {
    u32 fib_index;
    fib_node_index_t *entries;
} api_ip6_fib_show_ctx_t;

static void
api_ip6_fib_table_put_entries (clib_bihash_kv_24_8_t * kvp,
                               void *arg)
{
  api_ip6_fib_show_ctx_t *ctx = arg;

  if ((kvp->key[2] >> 32) == ctx->fib_index)
    {
      vec_add1(ctx->entries, kvp->value);
    }
}

static void
api_ip6_fib_table_get_all ( fib_table_t * fib_table,
                            void *writer)
{
  ip6_main_t *im6 = &ip6_main;
  fib_node_index_t *fib_entry_index;
  api_ip6_fib_show_ctx_t ctx = {
    .fib_index = fib_table->ft_index,
    .entries = NULL,
  };
  fib_route_path_encode_t *api_rpaths;
  fib_prefix_t pfx;

  BV (clib_bihash_foreach_key_value_pair)
    ((BVT (clib_bihash) *) & im6->ip6_table[IP6_FIB_TABLE_NON_FWDING].
     ip6_hash, api_ip6_fib_table_put_entries, &ctx);

  vec_sort_with_function (ctx.entries, fib_entry_cmp_for_sort);

  vec_foreach (fib_entry_index, ctx.entries)
  {
    fib_entry_get_prefix (*fib_entry_index, &pfx);
    api_rpaths = NULL;
    fib_entry_encode (*fib_entry_index, &api_rpaths);
    send_ip6_fib_details ( fib_table->ft_table_id,
			  &pfx, api_rpaths, writer);
    vec_free (api_rpaths);
  }

  vec_free (ctx.entries);
}

void
ip6_fib_dump_handler (void *writer)
{
  ip6_main_t *im6 = &ip6_main;
  fib_table_t *fib_table;
  vlib_main_t *vm = api_server_get_vlib_main ();

  /*
   * CLI input/output functions are looked up from
   * current process node. This is run in thread context.
   * So setting the current node index here
   */

  while (__sync_lock_test_and_set (vm->main_lockp, 1))
    ;

  /* *INDENT-OFF* */
  pool_foreach (fib_table, im6->fibs,
  ({
    api_ip6_fib_table_get_all(fib_table, writer);
  }));
  *vm->main_lockp = 0;
}
