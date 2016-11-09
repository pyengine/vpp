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
#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/pg/pg.h>
#include <vppinfra/error.h>
#include <vnet/ip/ip.h>
#include <vnet/sr/sr.h>
#include <ioam/ip6/ioam_cache.h>
#include <vnet/ip/ip6_hop_by_hop.h>
#include <vnet/ip/ip6_hop_by_hop_packet.h>

typedef struct
{
  u32 next_index;
  u32 flow_label;
} cache_ts_trace_t;

/* packet trace format function */
static u8 *
format_cache_ts_trace (u8 * s, va_list * args)
{
  CLIB_UNUSED (vlib_main_t * vm) = va_arg (*args, vlib_main_t *);
  CLIB_UNUSED (vlib_node_t * node) = va_arg (*args, vlib_node_t *);
  cache_ts_trace_t *t = va_arg (*args, cache_ts_trace_t *);

  s = format (s, "CACHE: flow_label %d, next index %d",
	      t->flow_label, t->next_index);
  return s;
}

vlib_node_registration_t ioam_cache_ts_node;

#define foreach_cache_ts_error \
_(RECORDED, "ip6 iOAM headers cached")

typedef enum
{
#define _(sym,str) CACHE_TS_ERROR_##sym,
  foreach_cache_ts_error
#undef _
    CACHE_TS_N_ERROR,
} cache_ts_error_t;

static char *cache_ts_error_strings[] = {
#define _(sym,string) string,
  foreach_cache_ts_error
#undef _
};

typedef enum
{
  IOAM_CACHE_TS_NEXT_POP_HBYH,
  IOAM_CACHE_TS_ERROR_NEXT_DROP,
  IOAM_CACHE_TS_N_NEXT,
} cache_ts_next_t;

static uword
ip6_ioam_cache_ts_node_fn (vlib_main_t * vm,
		    vlib_node_runtime_t * node, vlib_frame_t * frame)
{
  ioam_cache_main_t *cm = &ioam_cache_main;
  u32 n_left_from, *from, *to_next;
  cache_ts_next_t next_index;
  u32 recorded = 0;

  from = vlib_frame_vector_args (frame);
  n_left_from = frame->n_vectors;
  next_index = node->cached_next_index;
  while (__sync_lock_test_and_set (cm->lockp_ts, 1)) 
    ;
  while (n_left_from > 0)
    {
      u32 n_left_to_next;

      vlib_get_next_frame (vm, node, next_index, to_next, n_left_to_next);
      while (n_left_from > 0 && n_left_to_next > 0)
	{
	  u32 bi0;
	  vlib_buffer_t *p0;
	  u32 next0 = IOAM_CACHE_TS_NEXT_POP_HBYH;
	  ip6_header_t *ip0;
	  ip6_hop_by_hop_header_t *hbh0, *hbh_cmp;
	  tcp_header_t *tcp0;
	  u32 tcp_offset0;
	  i32 cache_ts_index = -1;
	  int result = 0;
	  int skip = 0;

	  bi0 = from[0];
	  from += 1;
	  n_left_from -= 1;

	  p0 = vlib_get_buffer (vm, bi0);
	  ip0 = vlib_buffer_get_current (p0);
	  if (IP_PROTOCOL_TCP ==
	      ip6_find_hdr (p0, ip0, IP_PROTOCOL_TCP, &tcp_offset0))
	    {
	      tcp0 = (tcp_header_t *)((u8 *)ip0 + tcp_offset0);
       	      if ((tcp0->flags & TCP_FLAG_SYN) == TCP_FLAG_SYN &&
	          (tcp0->flags & TCP_FLAG_ACK) == TCP_FLAG_ACK)
		{
		  /* Look up and compare*/
		  hbh0 = (ip6_hop_by_hop_header_t *) (ip0 + 1);
                  
		  if (0 == ioam_cache_ts_lookup (ip0,
						 hbh0->protocol,
						 clib_net_to_host_u16(tcp0->ports.src),
						 clib_net_to_host_u16(tcp0->ports.dst),
						 clib_net_to_host_u32(tcp0->ack_number),
						 &hbh_cmp,
                                                 &cache_ts_index,
                                                 1))//response seen
		    {
		      result = -1;
		      if (hbh_cmp)
		        result = ip6_ioam_analyse_compare_path_delay(hbh0, hbh_cmp);
		      if (result >=0 )
			{
			  /* current syn/ack is worse than the earlier: Drop */
			  next0 = IOAM_CACHE_TS_ERROR_NEXT_DROP;
			  /* Check if all responses are received or time has exceeded 
			     send cached response if yes */
			  ioam_cache_ts_check_and_send (cache_ts_index);
			}
		      else {
			/* Update cache with this buffer */
			/* If successfully updated then skip sending it*/
			if (0 == (result = ioam_cache_ts_update (cache_ts_index,
						                 bi0,
								 hbh0)))
			  {
			    skip = 1;
			  } 
			else
			  next0 = IOAM_CACHE_TS_ERROR_NEXT_DROP;
		      }
		    }
		  else
		    {
		      next0 = IOAM_CACHE_TS_ERROR_NEXT_DROP;
		    }
		}
	      else if ((tcp0->flags & TCP_FLAG_RST) == TCP_FLAG_RST)
		{
		  /* Look up and compare*/
		  hbh0 = (ip6_hop_by_hop_header_t *) (ip0 + 1);
		  if (0 == ioam_cache_ts_lookup (ip0,
						 hbh0->protocol,
						 clib_net_to_host_u16(tcp0->ports.src),
						 clib_net_to_host_u16(tcp0->ports.dst),
						 clib_net_to_host_u32(tcp0->ack_number),
						 &hbh_cmp,
                                                 &cache_ts_index,
                                                 1))//response seen
		    {
		      next0 = IOAM_CACHE_TS_ERROR_NEXT_DROP;
		      if (hbh_cmp)
                        ioam_cache_ts_check_and_send (cache_ts_index);		  
		    }
		   
		}
	    }
	  if (PREDICT_FALSE ((node->flags & VLIB_NODE_FLAG_TRACE)))
	    {
	      if (p0->flags & VLIB_BUFFER_IS_TRACED)
		{
		  cache_ts_trace_t *t =
		    vlib_add_trace (vm, node, p0, sizeof (*t));
		  t->flow_label =
		    clib_net_to_host_u32 (ip0->
					  ip_version_traffic_class_and_flow_label);
		  t->next_index = next0;
		}
	    }
	  /* verify speculative enqueue, maybe switch current next frame */
	  if (!skip) 
	    {
              to_next[0] = bi0;
	      to_next += 1;
	      n_left_to_next -= 1;
     	      vlib_validate_buffer_enqueue_x1 (vm, node, next_index,
					     to_next, n_left_to_next,
	    				     bi0, next0);
	    }
	}

      vlib_put_next_frame (vm, node, next_index, n_left_to_next);
    }
  *cm->lockp_ts = 0;
  vlib_node_increment_counter (vm, ioam_cache_ts_node.index,
			       CACHE_TS_ERROR_RECORDED, recorded);
  return frame->n_vectors;
}

/*
 * Node for IP6 iOAM header cache
 */
VLIB_REGISTER_NODE (ioam_cache_ts_node) =
/* *INDENT-OFF* */
{
  .function = ip6_ioam_cache_ts_node_fn,
  .name = "ip6-ioam-tunnel-select",
  .vector_size = sizeof (u32),
  .format_trace = format_cache_ts_trace,
  .type = VLIB_NODE_TYPE_INTERNAL,
  .n_errors = ARRAY_LEN (cache_ts_error_strings),
  .error_strings = cache_ts_error_strings,
  .n_next_nodes = IOAM_CACHE_TS_N_NEXT,
  /* edit / add dispositions here */
  .next_nodes =
  {
    [IOAM_CACHE_TS_NEXT_POP_HBYH] = "ip6-pop-hop-by-hop",
    [IOAM_CACHE_TS_ERROR_NEXT_DROP] = "error-drop",
  },
};
/* *INDENT-ON* */

typedef struct
{
  u32 next_index;
} ip6_reset_ts_hbh_trace_t;

/* packet trace format function */
static u8 *
format_ip6_reset_ts_hbh_trace (u8 * s, va_list * args)
{
  CLIB_UNUSED (vlib_main_t * vm) = va_arg (*args, vlib_main_t *);
  CLIB_UNUSED (vlib_node_t * node) = va_arg (*args, vlib_node_t *);
  ip6_reset_ts_hbh_trace_t *t = va_arg (*args,
					  ip6_reset_ts_hbh_trace_t *);

  s = format (s, "IP6_IOAM_RESET_TUNNEL_SELECT_HBH: next index %d", t->next_index);
  return s;
}

vlib_node_registration_t ip6_reset_ts_hbh_node;

#define foreach_ip6_reset_ts_hbh_error \
_(PROCESSED, "iOAM Syn/Ack Pkts processed") \
_(SAVED, "iOAM Syn Pkts state saved") \
_(REMOVED, "iOAM Syn/Ack Pkts state removed")

typedef enum
{
#define _(sym,str) IP6_RESET_TS_HBH_ERROR_##sym,
  foreach_ip6_reset_ts_hbh_error
#undef _
    IP6_RESET_TS_HBH_N_ERROR,
} ip6_reset_ts_hbh_error_t;

static char *ip6_reset_ts_hbh_error_strings[] = {
#define _(sym,string) string,
  foreach_ip6_reset_ts_hbh_error
#undef _
};

#define foreach_ip6_ioam_cache_ts_input_next    \
  _(IP6_HBH_ADD, "ip6-add-hop-by-hop")          \
  _(IP6_LOOKUP, "ip6-lookup")                   \
  _(DROP, "error-drop")

typedef enum
  {
#define _(s,n) IP6_IOAM_CACHE_TS_INPUT_NEXT_##s,
      foreach_ip6_ioam_cache_ts_input_next
      #undef _
      IP6_IOAM_CACHE_TS_INPUT_N_NEXT,
  } ip6_ioam_cache_ts_input_next_t;


static uword
ip6_reset_ts_hbh_node_fn (vlib_main_t * vm,
			    vlib_node_runtime_t * node, vlib_frame_t * frame)
{
  ioam_cache_main_t *cm = &ioam_cache_main;
  u32 n_left_from, *from, *to_next;
  ip_lookup_next_t next_index;
  u32 processed = 0, cache_ts_added = 0, cache_ts_deleted = 0;

  from = vlib_frame_vector_args (frame);
  n_left_from = frame->n_vectors;
  next_index = node->cached_next_index;
  while (__sync_lock_test_and_set (cm->lockp_ts, 1)) 
    ;

  while (n_left_from > 0)
    {
      u32 n_left_to_next;

      vlib_get_next_frame (vm, node, next_index, to_next, n_left_to_next);

      while (n_left_from > 0 && n_left_to_next > 0)
	{
	  u32 bi0;
	  vlib_buffer_t *b0;
	  u32 next0;
	  ip6_header_t *ip0;
	  tcp_header_t *tcp0;
          u32 tcp_offset0;
	  u8 no_of_responses = 0;

	  next0 = IP6_IOAM_CACHE_TS_INPUT_NEXT_IP6_LOOKUP;
	  /* speculatively enqueue b0 to the current next frame */
	  bi0 = from[0];
	  to_next[0] = bi0;
	  from += 1;
	  to_next += 1;
	  n_left_from -= 1;
	  n_left_to_next -= 1;

	  b0 = vlib_get_buffer (vm, bi0);

	  ip0 = vlib_buffer_get_current (b0);
	  if (IP_PROTOCOL_TCP !=
	      ip6_find_hdr (b0, ip0, IP_PROTOCOL_TCP, &tcp_offset0))
	    {
	      goto TRACE0;
	    }
	  tcp0 = (tcp_header_t *)((u8 *)ip0 + tcp_offset0);
	  if ((tcp0->flags & TCP_FLAG_SYN) == TCP_FLAG_SYN &&
	      (tcp0->flags & TCP_FLAG_ACK) == 0)
	    {
	      next0 = IP6_IOAM_CACHE_TS_INPUT_NEXT_IP6_HBH_ADD;
	      no_of_responses = ip6_sr_multicastmap_reset(&ip0->dst_address);
              if (no_of_responses > 0)
		{
		  /* Create TS select entry */
                  if ( 0 == ioam_cache_ts_add (ip0,
					       clib_net_to_host_u16(tcp0->ports.src),
					       clib_net_to_host_u16(tcp0->ports.dst), 
					       clib_net_to_host_u32(tcp0->seq_number) + 1,
					       no_of_responses))
		    {
		      cache_ts_added++;
		    }
                }
              processed++;	  
	    }
	  /*	  else if ((tcp0->flags & TCP_FLAG_SYN) == 0 &&
		   (tcp0->flags & TCP_FLAG_ACK) == TCP_FLAG_ACK)
	    {
	      if ( 0 == ioam_cache_ts_delete (p0, ip0,
					   clib_net_to_host_u16(tcp0->ports.src),
					   clib_net_to_host_u16(tcp0->ports.dst), 
					   clib_net_to_host_u32(tcp0->seq_number)))
		{
		  cache_ts_deleted++;
		}
	      processed++;
	    }
	  */
          
	TRACE0:
	  if (PREDICT_FALSE ((node->flags & VLIB_NODE_FLAG_TRACE)
			     && (b0->flags & VLIB_BUFFER_IS_TRACED)))
	    {
	      ip6_reset_ts_hbh_trace_t *t =
		vlib_add_trace (vm, node, b0, sizeof (*t));
	      t->next_index = next0;
	    }

	  /* verify speculative enqueue, maybe switch current next frame */
	  vlib_validate_buffer_enqueue_x1 (vm, node, next_index,
					   to_next, n_left_to_next,
					   bi0, next0);
	}

      vlib_put_next_frame (vm, node, next_index, n_left_to_next);
    }
  *cm->lockp_ts = 0;
  vlib_node_increment_counter (vm, ip6_reset_ts_hbh_node.index,
			       IP6_RESET_TS_HBH_ERROR_PROCESSED, processed);
  vlib_node_increment_counter (vm, ip6_reset_ts_hbh_node.index,
			       IP6_RESET_TS_HBH_ERROR_SAVED, cache_ts_added);
  vlib_node_increment_counter (vm, ip6_reset_ts_hbh_node.index,
			       IP6_RESET_TS_HBH_ERROR_REMOVED, cache_ts_deleted);

  return frame->n_vectors;
}

VLIB_REGISTER_NODE (ip6_reset_ts_hbh_node) =
/* *INDENT-OFF* */
{
  .function = ip6_reset_ts_hbh_node_fn,
  .name = "ip6-add-syn-hop-by-hop",
  .vector_size = sizeof (u32),
  .format_trace = format_ip6_reset_ts_hbh_trace,
  .type = VLIB_NODE_TYPE_INTERNAL,
  .n_errors = ARRAY_LEN (ip6_reset_ts_hbh_error_strings),
  .error_strings =  ip6_reset_ts_hbh_error_strings,
  /* See ip/lookup.h */
  .n_next_nodes = IP6_IOAM_CACHE_TS_INPUT_N_NEXT,
  .next_nodes =
  {
#define _(s,n) [IP6_IOAM_CACHE_TS_INPUT_NEXT_##s] = n,
    foreach_ip6_ioam_cache_ts_input_next
#undef _
  },
};
/* *INDENT-ON* */

VLIB_NODE_FUNCTION_MULTIARCH (ip6_reset_cache_ts_hbh_node,
			      ip6_reset_cache_ts_hbh_node_fn)
