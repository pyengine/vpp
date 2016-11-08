/*
 * Copyright (c) 2015 Cisco and/or its affiliates.
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
#ifndef __included_ip6_ioam_flow_report_h__
#define __included_ip6_ioam_flow_report_h__


#define foreach_ioam_ipfix_info_element           \
_(ioamPacketCount, 5237, u32)                     \
_(ioamByteCount, 5238, u32)                       \
_(ioamPathMap, 5262, u32)                         \
_(ioamNumberOfNodes, 5263, u16)                   \
_(ioamNumberOfPaths, 5264, u16)                   \
_(ioamSfcValidatedCount, 5278, u32)               \
_(ioamSfcInValidatedCount, 5279, u32)                                  

typedef enum {
#define _(n,v,t) n = v,
  foreach_ioam_ipfix_info_element
#undef _
} ioam_ipfix_info_element_id_t;

#define foreach_ioam_ipfix_field                                          \
_(ipfix->pkt_counter, 0xffffffff, ioamPacketCount, 4)                      \
_(ipfix->bytes_counter, 0xffffffff, ioamByteCount, 4)                      \
_(ipfix->pot_data.sfc_validated_count, 0xffffffff, ioamSfcValidatedCount, 4)        \
_(ipfix->pot_data.sfc_invalidated_count, 0xffffffff, ioamSfcInValidatedCount, 4)
/* Following are sent manually: Src address, dest address, src port, dest port
 * path map,  number of paths */

clib_error_t *ioam_flow_report_init(vlib_main_t *vm);

#if 0
typedef  struct {
  u8 num_nodes;
  u8 trace_type;
  u16 reserve;
  u32 pkt_counter;
  u32 bytes_counter;
  ioam_path_map_t path[IOAM_TRACE_MAX_NODES];
} ioam_path;
#endif

clib_error_t *
ioam_flow_create(ip4_address_t collector, ip4_address_t src, u8 del);

#endif /* __included_ip6_ioam_flow_report_h__ */
