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
#ifndef __included_l2sess_h__
#define __included_l2sess_h__

#include <vnet/vnet.h>
#include <vnet/ip/ip.h>
#include <vnet/ethernet/ethernet.h>

#include <vppinfra/hash.h>
#include <vppinfra/error.h>
#include <vppinfra/elog.h>

#include <vnet/l2/l2_output.h>
#include <vnet/l2/l2_input.h>

#define _(node_name, node_var, is_out, is_ip6, is_track)
#undef _
#define foreach_l2sess_node \
  _("l2s-input-ip4-add", l2sess_in_ip4_add, 0, 0, 0)  \
  _("l2s-input-ip6-add", l2sess_in_ip6_add, 0, 1, 0)  \
  _("l2s-output-ip4-add", l2sess_out_ip4_add, 1, 0, 0) \
  _("l2s-output-ip6-add", l2sess_out_ip6_add, 1, 1, 0) \
  _("l2s-input-ip4-track", l2sess_in_ip4_track, 0, 0, 1) \
  _("l2s-input-ip6-track", l2sess_in_ip6_track, 0, 1, 1) \
  _("l2s-output-ip4-track",l2sess_out_ip4_track, 1, 0, 1) \
  _("l2s-output-ip6-track", l2sess_out_ip6_track, 1, 1, 1)

#define _(node_name, node_var, is_out, is_ip6, is_track)  \
  extern vlib_node_registration_t node_var;
foreach_l2sess_node
#undef _

typedef struct {
  ip46_address_t addr;
  u64 active_time;
  u64 n_packets;
  u64 n_bytes;
  u16 port;
} l2s_session_side_t;

enum {
  L2S_SESSION_SIDE_IN = 0,
  L2S_SESSION_SIDE_OUT,
  L2S_N_SESSION_SIDES
};

typedef struct {
  u64 create_time;
  l2s_session_side_t side[L2S_N_SESSION_SIDES];
  u8 l4_proto;
  u8 is_ip6;
} l2s_session_t;


#define UDP_SESSION_IDLE_TIMEOUT_SEC 600
#define TCP_SESSION_IDLE_TIMEOUT_SEC (3600*24)

typedef struct {
    /* API message ID base */
    u16 msg_id_base;
    /*
     * the next two fields are present for all nodes, but
     *  only one of them is used per node - depending
     * on whether the node is an input or output one.
     */
#define _(node_name, node_var, is_out, is_ip6, is_track) \
    u32 node_var ## _input_next_node_index[32]; \
    l2_output_next_nodes_st node_var ## _next_nodes;
foreach_l2sess_node
#undef _
    l2_output_next_nodes_st output_next_nodes;

    /* Next indices of the tracker nodes */
    u32 next_slot_track_node_by_is_ip6_is_out[2][2];

    /* 
     * Pairing of "forward" and "reverse" tables by table index.
     * Each relationship has two entries - for one and the other table,
     * so it is bidirectional.
     */
     
    u32 *fwd_to_rev_by_table_index;

    /*
     * The vector of per-interface session pools
     */

    l2s_session_t **sessions_by_sw_if_index;

    /* The session timeouts */
    u64 tcp_session_idle_timeout;
    u64 udp_session_idle_timeout;

    /* convenience */
    vlib_main_t * vlib_main;
    vnet_main_t * vnet_main;
    ethernet_main_t * ethernet_main;
} l2sess_main_t;

l2sess_main_t l2sess_main;


#endif /* __included_l2sess_h__ */
