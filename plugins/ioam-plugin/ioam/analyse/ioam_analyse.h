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

#ifndef PLUGINS_IOAM_PLUGIN_IOAM_ANALYSE_IOAM_ANALYSE_H_
#define PLUGINS_IOAM_PLUGIN_IOAM_ANALYSE_IOAM_ANALYSE_H_

#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vppinfra/types.h>
#include <ioam/lib-e2e/ioam_seqno_lib.h>
#include <vnet/ip/ip6_hop_by_hop.h>
#include <ioam/lib-trace/trace_util.h>

#define IOAM_FLOW_TEMPLATE_ID    260
#define IOAM_TRACE_MAX_NODES      16
#define IOAM_MAX_PATHS_PER_FLOW   4

typedef  struct {
  u16 ingress_if;
  u16 egress_if;
  u32 node_id;
} ioam_path_map_t;

typedef struct {
  /* No of entries in path */
  u8 num_nodes;

  u8 trace_type;

  /* Flag to indicate whether node is allocated */
  u8 is_free;

  u8 pad[5];

  /* Actual PATH flow has taken */
  ioam_path_map_t path[IOAM_TRACE_MAX_NODES];

  /* Num of pkts in the flow going over path */
  u64 pkt_counter;

  /* Num of bytes in the flow going over path */
  u64 bytes_counter;

  f64 min_delay;

  f64 max_delay;

  f64 mean_delay;
} ioam_analyse_trace_record;

typedef struct {
  ioam_analyse_trace_record path_data[IOAM_MAX_PATHS_PER_FLOW];
} ioam_analyse_trace_data;

typedef struct {
  /* Number of packets validated (passes through the service chain)
   * within the timestamps */
  u32 sfc_validated_count;

  /* Number of packets invalidated (failed through the service chain)
   * within the timestamps */
  u32 sfc_invalidated_count;
} ioam_analyse_pot_data;

typedef  struct ioam_analyser_data_t_ {
  u8 is_free;
  u8 pad[7];

  /* Num of pkts matching this flow */
  u64 pkt_counter;

  /* Num of bytes matching this flow */
  u64 bytes_counter;

  /* Analysed iOAM trace data */
  ioam_analyse_trace_data trace_data;

  /* Analysed iOAM pot data */
  ioam_analyse_pot_data pot_data;

  /* Analysed iOAM seqno data */
  seqno_rx_info seqno_data;

  /* Cache of previously analysed data, useful for export */
  struct ioam_analyser_data_t_ *chached_data_list;

  /* Lock to since we use this to export the data in other thread */
  volatile u32 *writer_lock;
} ioam_analyser_data_t;

always_inline
void * ip6_ioam_find_hbh_option (ip6_hop_by_hop_header_t *hbh0, u8 option)
{
  ip6_hop_by_hop_option_t *opt0, *limit0;
  u8 type0;

  opt0 = (ip6_hop_by_hop_option_t *) (hbh0 + 1);
  limit0 = (ip6_hop_by_hop_option_t *) ((u8 *)hbh0 + ((hbh0->length + 1) << 3));

  while (opt0 < limit0)
    {
      type0 = opt0->type;
      if (type0 == option)
        return ((void *) opt0);

      if (0 == type0) {
          opt0 = (ip6_hop_by_hop_option_t *) ((u8 *)opt0) + 1;
          continue;
      }
      opt0 = (ip6_hop_by_hop_option_t *)
              (((u8 *)opt0) + opt0->length + sizeof (ip6_hop_by_hop_option_t));
    }

  return NULL;
}

always_inline i32
ip6_ioam_analyse_calc_delay (ioam_trace_option_t *trace,
                             bool oneway)
{
  u16 size_of_traceopt_per_node, size_of_all_traceopts;
  u8 num_nodes;
  u32 *start_elt, *end_elt, *uturn_elt;
  u32 start_time, end_time;
  u8 done = 0;
  size_of_traceopt_per_node = fetch_trace_data_size(trace->ioam_trace_type);
  size_of_all_traceopts = trace->hdr.length - 2; /*ioam_trace_type,data_list_elts_left*/

  num_nodes = (u8) (size_of_all_traceopts / size_of_traceopt_per_node);

  end_elt = trace->elts + (size_of_traceopt_per_node * trace->data_list_elts_left / sizeof(u32));
  start_elt = trace->elts + (size_of_traceopt_per_node * (num_nodes - 1) / sizeof(u32));

  if (oneway && (trace->ioam_trace_type & BIT_TTL_NODEID))
    {
      done = 0;
      do
	{
          uturn_elt = start_elt - size_of_traceopt_per_node/sizeof(u32);

	  if ((clib_net_to_host_u32 (*start_elt) >> 24) <=
	      (clib_net_to_host_u32 (*uturn_elt) >> 24))
	    done = 1;
	} while (!done && (start_elt = uturn_elt) != end_elt);
    }
    if (trace->ioam_trace_type & BIT_TTL_NODEID)
    {
      start_elt++;
      end_elt++;
    }
  if (trace->ioam_trace_type & BIT_ING_INTERFACE)
    {
      start_elt++;
      end_elt++;
    }

  start_time = clib_net_to_host_u32 (*start_elt);
  end_time = clib_net_to_host_u32 (*end_elt);

  return (end_time - start_time);
}

always_inline int
ip6_ioam_analyse_compare_path_delay (ip6_hop_by_hop_header_t *hbh0,
				     ip6_hop_by_hop_header_t *hbh1,
				     bool oneway)
{
  ioam_trace_option_t *trace0 = NULL, *trace1 = NULL;
  f64 delay0, delay1;

  trace0 = ip6_ioam_find_hbh_option(hbh0, HBH_OPTION_TYPE_IOAM_TRACE_DATA_LIST);
  trace1 = ip6_ioam_find_hbh_option(hbh1, HBH_OPTION_TYPE_IOAM_TRACE_DATA_LIST);

  if (PREDICT_FALSE((trace0 == NULL) && (trace1 == NULL)))
    return 0;

  if (PREDICT_FALSE(trace1 == NULL))
    return 1;

  if (PREDICT_FALSE(trace0 == NULL))
    return -1;

  delay0 = ip6_ioam_analyse_calc_delay(trace0, oneway);
  delay1 = ip6_ioam_analyse_calc_delay(trace1, oneway);

  return (delay0 - delay1);
}

always_inline int
ip6_ioam_analyse_hbh_trace (ioam_analyser_data_t *data,
                            ip6_hop_by_hop_option_t *opt, u16 len)
{
  ioam_analyse_trace_data *trace_data;
  ioam_trace_option_t *trace = (ioam_trace_option_t *)(opt);
  u16 size_of_traceopt_per_node;
  u16 size_of_all_traceopts;
  u8 i, j, num_nodes;
  u8 *ptr;
  u32 nodeid;
  u16 ingress_if, egress_if;
  ioam_path_map_t *path = NULL;
  ioam_analyse_trace_record *trace_record;

  trace_data = &data->trace_data;

  size_of_traceopt_per_node = fetch_trace_data_size(trace->ioam_trace_type);
  size_of_all_traceopts = trace->hdr.length - 2; /*ioam_trace_type,data_list_elts_left*/

  ptr = (u8 *)trace->elts;
  num_nodes = (u8) (size_of_all_traceopts / size_of_traceopt_per_node);

  for(i = 0; i < IOAM_MAX_PATHS_PER_FLOW; i++)
    {
      trace_record = trace_data->path_data + i;

      if (trace_record->is_free ||
          (num_nodes != trace_record->num_nodes) ||
          (trace->ioam_trace_type != trace_record->trace_type))
        continue;

      path = trace_record->path;

      for (j = 0; j < num_nodes; j++)
        {
          ptr = (u8 *) ((u8 *)trace->elts + (size_of_traceopt_per_node * j));

          nodeid = clib_net_to_host_u32 (*((u32 *) ptr)) & 0x00ffffff;
          ptr += 4;

          if (nodeid != path[j].node_id)
            break;

          if ((trace->ioam_trace_type == TRACE_TYPE_IF_TS_APP) ||
              (trace->ioam_trace_type == TRACE_TYPE_IF))
            {
              ingress_if = clib_net_to_host_u16(*((u16 *) ptr));
              ptr += 2;
              egress_if  = clib_net_to_host_u16(*((u16 *) ptr));
              if ((ingress_if != path[j].ingress_if) ||
                  (egress_if != path[j].egress_if))
                {
                  break;
                }
            }
        }

      if (j >= num_nodes)
        {
          goto found_match;
        }
    }

  for(i = 0; i < IOAM_MAX_PATHS_PER_FLOW; i++)
    {
      trace_record = trace_data->path_data + i;
      if (trace_record->is_free)
        {
          trace_record->is_free = 0;
          trace_record->num_nodes = num_nodes;
          trace_record->trace_type = trace->ioam_trace_type;
          path = trace_data->path_data[i].path;
          trace_record->pkt_counter = 0;
          trace_record->bytes_counter = 0;
          trace_record->min_delay = 0xFFFFFFFF;
          trace_record->max_delay = 0;
          trace_record->mean_delay = 0;
          break;
        }
    }

  for (j = 0; j < num_nodes; j++)
    {
      ptr = (u8 *) ((u8*)trace->elts + (size_of_traceopt_per_node * j));

      path[j].node_id = clib_net_to_host_u32 (*((u32 *) ptr)) & 0x00ffffff;
      ptr += 4;

      if ((trace->ioam_trace_type == TRACE_TYPE_IF_TS_APP) ||
          (trace->ioam_trace_type == TRACE_TYPE_IF))
        {
          path[j].ingress_if = clib_net_to_host_u16(*((u16 *) ptr));
          ptr += 2;
          path[j].egress_if  = clib_net_to_host_u16(*((u16 *) ptr));
        }
    }

  found_match:
  trace_record->pkt_counter++;
  trace_record->bytes_counter += len;

  if (trace->ioam_trace_type & BIT_TIMESTAMP)
    {
      /* Calculate time delay */
      i32 delay = ip6_ioam_analyse_calc_delay(trace,0);
      if (delay < trace_record->min_delay)
        trace_record->min_delay = delay;
      else if (delay > trace_record->max_delay)
        trace_record->max_delay = delay;

      trace_record->mean_delay = (trace_record->mean_delay + delay) / 2;
    }
  return 0;
}

always_inline int
ip6_ioam_analyse_hbh_e2e (ioam_analyser_data_t *data,
                          ip6_hop_by_hop_option_t *opt, u16 len)
{
  ioam_e2e_option_t * e2e;

  e2e = (ioam_e2e_option_t *) opt;
  ioam_analyze_seqno(&data->seqno_data,
                     (u64) clib_net_to_host_u32(e2e->e2e_data));
  return 0;
}

always_inline u8 *
format_path_map (u8 *s, va_list *args)
{
  ioam_path_map_t *pm = va_arg (*args, ioam_path_map_t *);
  u32 num_of_elts = va_arg (*args, u32);
  u32 i;

  for (i = 0; i < num_of_elts; i++)
    {
      s = format (s, "node_id: 0x%x, ingress_if: 0x%x, egress_if:0x%x\n",
                  pm->node_id, pm->ingress_if, pm->egress_if);
      pm++;
    }

  return (s);
}

always_inline u8 *
print_analyse_flow (u8 *s, ioam_analyser_data_t *record)
{
  int j;
  ioam_analyse_trace_record *trace_record;

  s = format(s, "pkt_counter : %lu\n", record->pkt_counter);
  s = format(s, "bytes_counter : %lu\n", record->bytes_counter);

  s = format(s, "Trace data: \n");

  for(j = 0; j < IOAM_MAX_PATHS_PER_FLOW; j++)
    {
      trace_record = record->trace_data.path_data + j;
      if (trace_record->is_free)
        continue;

      s = format(s, "path_map:\n%U", format_path_map,
                 trace_record->path,
                 trace_record->num_nodes);
      s = format(s, "pkt_counter: %lu\n", trace_record->pkt_counter);
      s = format(s, "bytes_counter: %lu\n", trace_record->bytes_counter);

      s = format(s, "min_delay: %lu\n", trace_record->min_delay);
      s = format(s, "max_delay: %lu\n", trace_record->max_delay);
      s = format(s, "mean_delay: %lu\n", trace_record->mean_delay);
    }

  s = format(s, "\nPOT data: \n");
  s = format(s, "sfc_validated_count : %lu\n",
             record->pot_data.sfc_validated_count);
  s = format(s, "sfc_invalidated_count : %lu\n",
             record->pot_data.sfc_invalidated_count);

  s = format(s, "\nSeqno Data:\n");
  s = format(s,
             "RX Packets        : %lu\n"
             "Lost Packets      : %lu\n"
             "Duplicate Packets : %lu\n"
             "Reordered Packets : %lu\n",
             record->seqno_data.rx_packets,
             record->seqno_data.lost_packets,
             record->seqno_data.dup_packets,
             record->seqno_data.reordered_packets);

  s = format(s, "\n");
  return s;
}

always_inline void ioam_analyse_init_data(ioam_analyser_data_t *data)
{
  u16 j;
  ioam_analyse_trace_data *trace_data;

  data->is_free = 1;

  /* We maintain data corresponding to last IP-Fix export, this may
   * get extended in future to maintain history of data */
  vec_validate_aligned(data->chached_data_list,
                       0,
                       CLIB_CACHE_LINE_BYTES);

  data->writer_lock = clib_mem_alloc_aligned (CLIB_CACHE_LINE_BYTES,
                                              CLIB_CACHE_LINE_BYTES);
  *(data->writer_lock) = 0;

  trace_data = &(data->trace_data);
  for(j = 0; j < IOAM_MAX_PATHS_PER_FLOW; j++)
    trace_data->path_data[j].is_free = 1;
}

#endif /* PLUGINS_IOAM_PLUGIN_IOAM_ANALYSE_IOAM_ANALYSE_H_ */
