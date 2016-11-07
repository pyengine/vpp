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

#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <acl/acl.h>

#include <vnet/l2/l2_classify.h>

#include <vlibapi/api.h>
#include <vlibmemory/api.h>
#include <vlibsocket/api.h>

/* define message IDs */
#include <acl/acl_msg_enum.h>

/* define message structures */
#define vl_typedefs
#include <acl/acl_all_api_h.h>
#undef vl_typedefs

/* define generated endian-swappers */
#define vl_endianfun
#include <acl/acl_all_api_h.h>
#undef vl_endianfun

/* instantiate all the print functions we know about */
#define vl_print(handle, ...) vlib_cli_output (handle, __VA_ARGS__)
#define vl_printfun
#include <acl/acl_all_api_h.h>
#undef vl_printfun

/* Get the API version number */
#define vl_api_version(n,v) static u32 api_version=(v);
#include <acl/acl_all_api_h.h>
#undef vl_api_version


acl_main_t acl_main;

static void
noop_handler (void *notused)
{
}

#define vl_api_acl_add_t_endian noop_handler
#define vl_api_acl_add_t_print noop_handler

/*
 * A handy macro to set up a message reply.
 * Assumes that the following variables are available:
 * mp - pointer to request message
 * rmp - pointer to reply message type
 * rv - return value
 */

#define REPLY_MACRO(t)                                          \
do {                                                            \
    unix_shared_memory_queue_t * q =                            \
    vl_api_client_index_to_input_queue (mp->client_index);      \
    if (!q)                                                     \
        return;                                                 \
                                                                \
    rmp = vl_msg_api_alloc (sizeof (*rmp));                     \
    rmp->_vl_msg_id = ntohs((t)+sm->msg_id_base);               \
    rmp->context = mp->context;                                 \
    rmp->retval = ntohl(rv);                                    \
                                                                \
    vl_msg_api_send_shmem (q, (u8 *)&rmp);                      \
} while(0);

#define REPLY_MACRO2(t, body)                                   \
do {                                                            \
    unix_shared_memory_queue_t * q;                             \
    rv = vl_msg_api_pd_handler (mp, rv);                        \
    q = vl_api_client_index_to_input_queue (mp->client_index);  \
    if (!q)                                                     \
        return;                                                 \
                                                                \
    rmp = vl_msg_api_alloc (sizeof (*rmp));                     \
    rmp->_vl_msg_id = ntohs((t)+am->msg_id_base);                               \
    rmp->context = mp->context;                                 \
    rmp->retval = ntohl(rv);                                    \
    do {body;} while (0);                                       \
    vl_msg_api_send_shmem (q, (u8 *)&rmp);                      \
} while(0);

#define REPLY_MACRO3(t, n, body)                                \
do {                                                            \
    unix_shared_memory_queue_t * q;                             \
    rv = vl_msg_api_pd_handler (mp, rv);                        \
    q = vl_api_client_index_to_input_queue (mp->client_index);  \
    if (!q)                                                     \
        return;                                                 \
                                                                \
    rmp = vl_msg_api_alloc (sizeof (*rmp) + n);                 \
    rmp->_vl_msg_id = ntohs((t)+am->msg_id_base);                               \
    rmp->context = mp->context;                                 \
    rmp->retval = ntohl(rv);                                    \
    do {body;} while (0);                                       \
    vl_msg_api_send_shmem (q, (u8 *)&rmp);                      \
} while(0);

#define VALIDATE_SW_IF_INDEX(mp)                                \
 do { u32 __sw_if_index = ntohl(mp->sw_if_index);               \
    vnet_main_t *__vnm = vnet_get_main();                       \
    if (pool_is_free_index(__vnm->interface_main.sw_interfaces, \
                           __sw_if_index)) {                    \
        rv = VNET_API_ERROR_INVALID_SW_IF_INDEX;                \
        goto bad_sw_if_index;                                   \
    }                                                           \
} while(0);

#define BAD_SW_IF_INDEX_LABEL                   \
do {                                            \
bad_sw_if_index:                                \
    ;                                           \
} while (0);



/* List of message types that this plugin understands */

#define foreach_acl_plugin_api_msg		\
_(ACL_ADD, acl_add)				\
_(ACL_DEL, acl_del)				\
_(ACL_INTERFACE_ADD_DEL, acl_interface_add_del)	\
_(ACL_DUMP, acl_dump)

/*
 * This routine exists to convince the vlib plugin framework that
 * we haven't accidentally copied a random .dll into the plugin directory.
 *
 * Also collects global variable pointers passed from the vpp engine
 */

clib_error_t *
vlib_plugin_register (vlib_main_t * vm, vnet_plugin_handoff_t * h,
                      int from_early_init)
{
  acl_main_t * am = &acl_main;
  clib_error_t * error = 0;

  am->vlib_main = vm;
  am->vnet_main = h->vnet_main;
  am->ethernet_main = h->ethernet_main;

  return error;
}

static int
acl_add_list (u32 count, vl_api_acl_rule_t rules[],
	      u32 * acl_list_index)
{
  acl_main_t *am = &acl_main;
  acl_list_t  * a;
  acl_rule_t  * r;
  int i;

  /* Get ACL index */
  pool_get_aligned (am->acls, a, CLIB_CACHE_LINE_BYTES);
  memset (a, 0, sizeof (*a));

  /* Init ACL struct */
  a->count = count;
  a->rules = clib_mem_alloc_aligned (sizeof(acl_rule_t) * count,
				     CLIB_CACHE_LINE_BYTES);
  if (!a->rules) {
    pool_put(am->acls, a);
    return -1;
  }
  clib_memcpy (a->rules, rules, sizeof(acl_rule_t) * count);
  for(i=0; i<count; i++) {
    r = &a->rules[i];
    r->is_permit = rules[i].is_permit;
    r->is_ipv6 = rules[i].is_ipv6;
    memcpy(&r->src, rules[i].src_ip_addr, sizeof(r->src));
    r->src_prefixlen = rules[i].src_ip_prefix_len;
    memcpy(&r->dst, rules[i].dst_ip_addr, sizeof(r->dst));
    r->dst_prefixlen = rules[i].dst_ip_prefix_len;
    r->proto = rules[i].proto;
    r->src_port = rules[i].src_port;
    r->dst_port = rules[i].dst_port;
  }
  *acl_list_index = a - am->acls;

  return 0;
}

static int
acl_del_list(u32 acl_list_index)
{
  acl_main_t *am = &acl_main;
  acl_list_t  * a;
  int i, ii;
  if (pool_is_free_index(am->acls, acl_list_index)) {
    return -1;
  }

  /* delete any references to the ACL */
  for(i = 0; i<vec_len(am->output_acl_vec_by_sw_if_index); i++) {
    for(ii = 0; ii < vec_len(am->output_acl_vec_by_sw_if_index[i]); /* see body */) {
      if (acl_list_index == am->output_acl_vec_by_sw_if_index[i][ii]) {
        vec_del1(am->output_acl_vec_by_sw_if_index[i], ii);
      } else {
        ii++;
      }
    }
  }
  for(i = 0; i<vec_len(am->input_acl_vec_by_sw_if_index); i++) {
    for(ii = 0; ii < vec_len(am->input_acl_vec_by_sw_if_index[i]); /* see body */) {
      if (acl_list_index == am->input_acl_vec_by_sw_if_index[i][ii]) {
        vec_del1(am->input_acl_vec_by_sw_if_index[i], ii);
      } else {
        ii++;
      }
    }
  }

  /* now we can delete the ACL itself */
  a = &am->acls[acl_list_index];
  if (a->rules) {
    clib_mem_free(a->rules);
  }
  pool_put(am->acls, a);
  return 0;
}

int acl_interface_early_in_enable_disable (acl_main_t * am, u32 sw_if_index,
                                   int enable_disable)
{
 /*
  * This function hooks the inbound ACL into the ethernet processing,
  * effectively making it the very first feature which sees all packets.
  * This is just for the testing - we will want to hook it from the classifier.
  */

  vnet_sw_interface_t * sw;
  int rv;
  u32 node_index = enable_disable ? acl_in_node.index : ~0;

  /* Utterly wrong? */
  if (pool_is_free_index (am->vnet_main->interface_main.sw_interfaces,
                          sw_if_index))
    return VNET_API_ERROR_INVALID_SW_IF_INDEX;

  /* Not a physical port? */
  sw = vnet_get_sw_interface (am->vnet_main, sw_if_index);
  if (sw->type != VNET_SW_INTERFACE_TYPE_HARDWARE)
    return VNET_API_ERROR_INVALID_SW_IF_INDEX;

  /*
   * Redirect pkts from the driver to the ACL node.
   * Returns VNET_API_ERROR_UNIMPLEMENTED if the h/w driver
   * doesn't implement the API.
   *
   * Node_index = ~0 => shut off redirection
   */
  rv = vnet_hw_interface_rx_redirect_to_node (am->vnet_main, sw_if_index,
                                              node_index);
  return rv;
}

int
acl_unhook_l2_input_classify(acl_main_t * am, u32 sw_if_index)
{
  return 0;
}

int
acl_unhook_l2_output_classify(acl_main_t * am, u32 sw_if_index)
{
  return 0;
}

/* Some aids in ASCII graphing the content */
#define XX "\377"
#define __ "\000"
#define _(x)
#define v

u8 ip4_5tuple_mask[] =
_("             dmac               smac            etype ")
_( ether)  __ __ __ __ __ __ v __ __ __ __ __ __ v __ __ v
_("        v ihl totlen ")
_(0x0000)  __ __ __ __
_("        ident fl+fo ")
_(0x0004)  __ __ __ __
_("       ttl pr checksum")
_(0x0008)  __ XX __ __
_("        src address    ")
_(0x000C)  XX XX XX XX
_("        dst address    ")
_(0x0010)  XX XX XX XX
_("L4 T/U  sport dport   ")
_(tcpudp)  XX XX XX XX
_(padpad)  __ __ __ __
_(padpad)  __ __ __ __
_(padeth)  __ __
;

u8 ip6_5tuple_mask[] =
_("             dmac               smac            etype ")
_( ether)  __ __ __ __ __ __ v __ __ __ __ __ __ v __ __ v
_("        v  tc + flow ")
_(0x0000)  __ __ __ __
_("        plen  nh hl  ")
_(0x0004)  __ __ XX __
_("        src address  ")
_(0x0008)  XX XX XX XX
_(0x000C)  XX XX XX XX
_(0x0010)  XX XX XX XX
_(0x0014)  XX XX XX XX
_("        dst address    ")
_(0x0018)  XX XX XX XX
_(0x001C)  XX XX XX XX
_(0x0020)  XX XX XX XX
_(0x0024)  XX XX XX XX
_("L4T/U  sport dport   ")
_(tcpudp)  XX XX XX XX
_(padpad)  __ __ __ __
_(padeth)  __ __
;

#undef XX
#undef __
#undef _
#undef v

static int
count_skip(u8 *p, u32 size)
{
  u64 *p64 = (u64 *)p;
  while ( (0ULL == *p64) && ((u8 *)p64 - p) < size ) {
    p64++;
  }
  return (p64-(u64 *)p)/2;
}

static int
acl_classify_add_del_table(vnet_classify_main_t * cm, u8 * mask, u32 mask_len, u32 next_table_index, u32 miss_next_index, u32 *table_index, int is_add)
{
  u32 nbuckets = 64;
  u32 memory_size = 32768;
  u32 skip = count_skip(mask, mask_len);
  u32 match = (mask_len/16) - skip;
  u8 *skip_mask_ptr = mask + 16*skip;
  return vnet_classify_add_del_table(cm, skip_mask_ptr, nbuckets, memory_size, skip, match, next_table_index, miss_next_index, table_index, is_add);
}

static int
acl_hook_l2_input_classify(acl_main_t * am, u32 sw_if_index)
{
  vnet_classify_main_t *cm = &vnet_classify_main;
  u32 ip4_table_index;
  u32 ip6_table_index;
  int rv;

  /* in case there were previous tables attached */
  acl_unhook_l2_input_classify(am, sw_if_index);
  rv = acl_classify_add_del_table(cm, ip4_5tuple_mask, sizeof(ip4_5tuple_mask)-1, ~0, am->l2_input_classify_next_acl, &ip4_table_index, 1);
  if (rv)
    return rv;
  rv = acl_classify_add_del_table(cm, ip6_5tuple_mask, sizeof(ip6_5tuple_mask)-1, ~0, am->l2_input_classify_next_acl, &ip6_table_index, 1);
  if (rv) {
    acl_classify_add_del_table(cm, ip4_5tuple_mask, sizeof(ip4_5tuple_mask)-1, ~0, am->l2_input_classify_next_acl, &ip4_table_index, 0);
    return rv;
  }
  rv = vnet_l2_input_classify_set_tables (sw_if_index, ip4_table_index, ip6_table_index, ~0);
  clib_warning("ACL enabling on interface sw_if_index %d, setting tables to the following: ip4: %d ip6: %d\n", sw_if_index, ip4_table_index, ip6_table_index);
  if (rv) {
    acl_classify_add_del_table(cm, ip6_5tuple_mask, sizeof(ip6_5tuple_mask)-1, ~0, am->l2_input_classify_next_acl, &ip6_table_index, 0);
    acl_classify_add_del_table(cm, ip4_5tuple_mask, sizeof(ip4_5tuple_mask)-1, ~0, am->l2_input_classify_next_acl, &ip4_table_index, 0);
    return rv;
  }
  vnet_l2_input_classify_enable_disable(sw_if_index, 1);
  return rv;
}

static int
acl_hook_l2_output_classify(acl_main_t * am, u32 sw_if_index)
{
  vnet_classify_main_t *cm = &vnet_classify_main;
  u32 ip4_table_index;
  u32 ip6_table_index;
  int rv;

  /* in case there were previous tables attached */
  acl_unhook_l2_output_classify(am, sw_if_index);
  rv = acl_classify_add_del_table(cm, ip4_5tuple_mask, sizeof(ip4_5tuple_mask)-1, ~0, am->l2_output_classify_next_acl, &ip4_table_index, 1);
  if (rv)
    return rv;
  rv = acl_classify_add_del_table(cm, ip6_5tuple_mask, sizeof(ip6_5tuple_mask)-1, ~0, am->l2_output_classify_next_acl, &ip6_table_index, 1);
  if (rv) {
    acl_classify_add_del_table(cm, ip4_5tuple_mask, sizeof(ip4_5tuple_mask)-1, ~0, am->l2_output_classify_next_acl, &ip4_table_index, 0);
    return rv;
  }
  rv = vnet_l2_output_classify_set_tables (sw_if_index, ip4_table_index, ip6_table_index, ~0);
  clib_warning("ACL enabling on interface sw_if_index %d, setting tables to the following: ip4: %d ip6: %d\n", sw_if_index, ip4_table_index, ip6_table_index);
  if (rv) {
    acl_classify_add_del_table(cm, ip6_5tuple_mask, sizeof(ip6_5tuple_mask)-1, ~0, am->l2_output_classify_next_acl, &ip6_table_index, 0);
    acl_classify_add_del_table(cm, ip4_5tuple_mask, sizeof(ip4_5tuple_mask)-1, ~0, am->l2_output_classify_next_acl, &ip4_table_index, 0);
    return rv;
  }
  vnet_l2_output_classify_enable_disable(sw_if_index, 1);
  return rv;
}


int acl_interface_in_enable_disable (acl_main_t * am, u32 sw_if_index,
                                   int enable_disable)
{
  int rv;

  /* Utterly wrong? */
  if (pool_is_free_index (am->vnet_main->interface_main.sw_interfaces,
                          sw_if_index))
    return VNET_API_ERROR_INVALID_SW_IF_INDEX;

  if(enable_disable) {
    rv = acl_hook_l2_input_classify(am, sw_if_index);
  } else {
    rv = acl_unhook_l2_input_classify(am, sw_if_index);
  }

  return rv;
}

int acl_interface_out_enable_disable (acl_main_t * am, u32 sw_if_index,
                                   int enable_disable)
{
  int rv;

  /* Utterly wrong? */
  if (pool_is_free_index (am->vnet_main->interface_main.sw_interfaces,
                          sw_if_index))
    return VNET_API_ERROR_INVALID_SW_IF_INDEX;

  if(enable_disable) {
    rv = acl_hook_l2_output_classify(am, sw_if_index);
  } else {
    rv = acl_unhook_l2_output_classify(am, sw_if_index);
  }

  return rv;
}


static int
acl_interface_add_inout_acl (u32 sw_if_index, u8 is_input, u32 acl_list_index)
{
  acl_main_t *am = &acl_main;
  if (is_input) {
    vec_validate(am->input_acl_vec_by_sw_if_index, sw_if_index);
    vec_add(am->input_acl_vec_by_sw_if_index[sw_if_index], &acl_list_index, 1);
    acl_interface_in_enable_disable(am, sw_if_index, 1);
  } else {
    vec_validate(am->output_acl_vec_by_sw_if_index, sw_if_index);
    vec_add(am->output_acl_vec_by_sw_if_index[sw_if_index], &acl_list_index, 1);
    acl_interface_out_enable_disable(am, sw_if_index, 1);
  }
  return 0;
}

static int
acl_interface_del_inout_acl (u32 sw_if_index, u8 is_input, u32 acl_list_index)
{
  acl_main_t *am = &acl_main;
  int i;
  int rv = -1;
  if (is_input) {
    vec_validate(am->input_acl_vec_by_sw_if_index, sw_if_index);
    for (i=0; i<vec_len(am->input_acl_vec_by_sw_if_index[sw_if_index]); i++) {
      if (acl_list_index == am->input_acl_vec_by_sw_if_index[sw_if_index][i]) {
	vec_del1(am->input_acl_vec_by_sw_if_index[sw_if_index], i);
	rv = 0;
	break;
      }
    }
    if (0 == vec_len(am->input_acl_vec_by_sw_if_index[sw_if_index])) {
      acl_interface_in_enable_disable(am, sw_if_index, 0);
    }
  } else {
    vec_validate(am->output_acl_vec_by_sw_if_index, sw_if_index);
    for (i=0; i<vec_len(am->output_acl_vec_by_sw_if_index[sw_if_index]); i++) {
      if (acl_list_index == am->output_acl_vec_by_sw_if_index[sw_if_index][i]) {
	vec_del1(am->output_acl_vec_by_sw_if_index[sw_if_index], i);
	rv = 0;
	break;
      }
    }
    if (0 == vec_len(am->output_acl_vec_by_sw_if_index[sw_if_index])) {
      acl_interface_out_enable_disable(am, sw_if_index, 0);
    }
  }
  return rv;
}

static int
acl_interface_add_del_inout_acl(u32 sw_if_index, u8 is_add, u8 is_input, u32 acl_list_index)
{
  int rv = -1;
  if (is_add) {
    rv = acl_interface_add_inout_acl(sw_if_index, is_input, acl_list_index);
  } else {
    rv = acl_interface_del_inout_acl(sw_if_index, is_input, acl_list_index);
  }
  return rv;
}


static void *
get_ptr_to_offset(vlib_buffer_t * b0, int offset)
{
  u8 *p = vlib_buffer_get_current (b0) + offset;
  return p;
}

static u8
acl_get_l4_proto(vlib_buffer_t * b0, int node_is_ip6)
{
  u8 proto;
  int proto_offset;
  if (node_is_ip6) {
    proto_offset = 20;
  } else {
    proto_offset = 23;
  }
  proto = *( (u8 *)vlib_buffer_get_current (b0) + proto_offset);
  return proto;
}

static int
acl_match_addr(ip46_address_t *addr1, ip46_address_t *addr2, int prefixlen, int is_ip6)
{
  if (prefixlen == 0) {
    /* match any always succeeds */
    return 1;
  }
  if (is_ip6) {
    if (memcmp(addr1, addr2, prefixlen/8)) {
      /* If the starting full bytes do not match, no point in bittwidling the thumbs further */
      return 0;
    }
    if (prefixlen % 8) {
      u8 b1 = *((u8 *)addr1 + 1 + prefixlen/8);
      u8 b2 = *((u8 *)addr2 + 1 + prefixlen/8);
      u8 mask0 = (0xff - ((1<<(8-(prefixlen%8))) -1));
      return (b1 & mask0) == b2;
    } else {
      /* The prefix fits into integer number of bytes, so nothing left to do */
      return 1;
    }
  } else {
    uint32_t a1 = ntohl(addr1->ip4.as_u32);
    uint32_t a2 = ntohl(addr2->ip4.as_u32);
    uint32_t mask0 = 0xffffffff - ((1<< (32-prefixlen))-1);
    clib_warning("acl_match_addr a1 %08x : mask %08x : a2 %08x", a1, mask0, a2);
    return (a1 & mask0) == a2;
  }
}

static int
acl_match_port(u16 port1, u16 port2, int is_ip6)
{
  if(port2 == 0)
    return 1;
  return (port1 == port2);
}

static int
acl_packet_match(acl_main_t *am, u32 acl_index, vlib_buffer_t * b0, u32 *nextp, u32 *acl_match_p, u32 *rule_match_p)
{
  ethernet_header_t *h0;
  u16 type0;

  ip46_address_t src, dst;
  int is_ip6;
  int is_ip4;
  u8 proto;
  u16 src_port;
  u16 dst_port;
  int i;
  acl_list_t  * a;
  acl_rule_t * r;

  h0 = vlib_buffer_get_current (b0);
  type0 =  clib_net_to_host_u16 (h0->type);
  is_ip4 = (type0 == ETHERNET_TYPE_IP4);
  is_ip6 = (type0 == ETHERNET_TYPE_IP6);

  if ( ! (is_ip4 || is_ip6) ) {
    return 0;
  }
  /* The bunch of hardcoded offsets here is intentional to get rid of them
     ASAP, when getting to a faster matching code */
  if (is_ip4) {
    clib_memcpy(&src.ip4, get_ptr_to_offset(b0, 26), 4);
    clib_memcpy(&dst.ip4, get_ptr_to_offset(b0, 30), 4);
    proto = acl_get_l4_proto(b0, 0);
    src_port = ntohs(*(u16 *)get_ptr_to_offset(b0, 34));
    dst_port = ntohs(*(u16 *)get_ptr_to_offset(b0, 36));
  }
  if (is_ip6) {
    clib_memcpy(&src, get_ptr_to_offset(b0, 22), 16);
    clib_memcpy(&dst, get_ptr_to_offset(b0, 38), 16);
    proto = acl_get_l4_proto(b0, 1);
    src_port = ntohs(*(u16 *)get_ptr_to_offset(b0, 54));
    dst_port = ntohs(*(u16 *)get_ptr_to_offset(b0, 56));
  }
  a = am->acls + acl_index;
  for(i=0; i<a->count; i++) {
    r = a->rules + i;
    if (is_ip6 != r->is_ipv6) {
      continue;
    }
    if (!acl_match_addr(&dst, &r->dst, r->dst_prefixlen, is_ip6))
      continue;
    if (!acl_match_addr(&src, &r->src, r->src_prefixlen, is_ip6))
      continue;
    if (r->proto && (proto != r->proto))
      continue;
    if (!acl_match_port(dst_port, r->dst_port, is_ip6))
      continue;
    if (!acl_match_port(src_port, r->src_port, is_ip6))
      continue;
    /* everything matches! */
    *nextp = r->is_permit ? ~0 : 0;
    if (acl_match_p) *acl_match_p = acl_index;
    if (rule_match_p) *rule_match_p = i;
    return 1;
  }
  return 0;
}

void input_acl_packet_match(u32 sw_if_index, vlib_buffer_t * b0, u32 *nextp, u32 *acl_match_p, u32 *rule_match_p)
{
  acl_main_t *am = &acl_main;
  int i;
  vec_validate(am->input_acl_vec_by_sw_if_index, sw_if_index);
  for (i=0; i<vec_len(am->input_acl_vec_by_sw_if_index[sw_if_index]); i++) {
    if(acl_packet_match(am, am->input_acl_vec_by_sw_if_index[sw_if_index][i], b0, nextp, acl_match_p, rule_match_p)) {
      return;
    }
  }

}

void output_acl_packet_match(u32 sw_if_index, vlib_buffer_t * b0, u32 *nextp, u32 *acl_match_p, u32 *rule_match_p)
{
  acl_main_t *am = &acl_main;
  int i;
  vec_validate(am->output_acl_vec_by_sw_if_index, sw_if_index);
  for (i=0; i<vec_len(am->output_acl_vec_by_sw_if_index[sw_if_index]); i++) {
    if(acl_packet_match(am, am->output_acl_vec_by_sw_if_index[sw_if_index][i], b0, nextp, acl_match_p, rule_match_p)) {
      return;
    }
  }
}

/* API message handler */
static void
vl_api_acl_add_t_handler (vl_api_acl_add_t * mp)
{
  vl_api_acl_add_reply_t * rmp;
  acl_main_t *am = &acl_main;
  int rv;
  u32 acl_list_index = ~0;

  rv = acl_add_list(ntohl(mp->count), mp->r, &acl_list_index);

  /* *INDENT-OFF* */
  REPLY_MACRO2(VL_API_ACL_ADD_REPLY,
  ({
    rmp->acl_index = htonl(acl_list_index);
  }));
  /* *INDENT-ON* */
}

static void
vl_api_acl_del_t_handler (vl_api_acl_del_t * mp)
{
  acl_main_t * sm = &acl_main;
  vl_api_acl_del_reply_t * rmp;
  int rv;

  rv = acl_del_list(ntohl(mp->acl_index));

  REPLY_MACRO(VL_API_ACL_DEL_REPLY);
}

static void
vl_api_acl_interface_add_del_t_handler (vl_api_acl_interface_add_del_t * mp)
{
  acl_main_t * sm = &acl_main;
  vl_api_acl_interface_add_del_reply_t * rmp;
  int rv = -1;
  VALIDATE_SW_IF_INDEX (mp);

  rv = acl_interface_add_del_inout_acl(ntohl(mp->sw_if_index), mp->is_add, mp->is_input, ntohl(mp->acl_index));

  BAD_SW_IF_INDEX_LABEL;

  REPLY_MACRO(VL_API_ACL_INTERFACE_ADD_DEL_REPLY);
}

static void
send_acl_details(acl_main_t * am, unix_shared_memory_queue_t * q,
                         u32 sw_if_index, acl_list_t *acl, u32 context)
{
  vl_api_acl_details_t *mp;
  int msg_size = sizeof (*mp) + sizeof(mp->r[0])*acl->count;

  mp = vl_msg_api_alloc (msg_size);
  memset (mp, 0, msg_size);
  mp->_vl_msg_id = ntohs (VL_API_ACL_DETAILS+am->msg_id_base);

  /* fill in the message */
  mp->context = context;
  mp->sw_if_index = htonl(sw_if_index);
  mp->count = htonl(acl->count);
  mp->acl_index = htonl(acl - am->acls);
  clib_memcpy (mp->r, acl->rules, acl->count * sizeof(acl->rules[0]));

  vl_msg_api_send_shmem (q, (u8 *) & mp);
}

static void
send_all_acl_details(acl_main_t * am, unix_shared_memory_queue_t * q, u32 sw_if_index, u32 context)
{
  u32 *p_acl_index;
  vec_validate(am->input_acl_vec_by_sw_if_index, sw_if_index);
  vec_validate(am->output_acl_vec_by_sw_if_index, sw_if_index);
  /* FIXME API: no way to tell to the caller which is input ACL and which one is output ACL */
  vec_foreach (p_acl_index, am->input_acl_vec_by_sw_if_index[sw_if_index]) {
    send_acl_details(am, q, sw_if_index, &am->acls[*p_acl_index], context);
  }
  vec_foreach (p_acl_index, am->output_acl_vec_by_sw_if_index[sw_if_index]) {
    send_acl_details(am, q, sw_if_index, &am->acls[*p_acl_index], context);
  }
}


static void
vl_api_acl_dump_t_handler (vl_api_acl_dump_t * mp)
{
  acl_main_t * am = &acl_main;
  vnet_sw_interface_t *swif;
  vnet_interface_main_t *im = &am->vnet_main->interface_main;
  acl_list_t  * acl;

  int rv = -1;
  u32 sw_if_index;
  unix_shared_memory_queue_t *q;

  q = vl_api_client_index_to_input_queue (mp->client_index);
  if (q == 0) {
    return;
  }

  if (mp->sw_if_index == ~0) {
    /* *INDENT-OFF* */
    pool_foreach (swif, im->sw_interfaces,
    ({
      send_all_acl_details(am, q, swif->sw_if_index, mp->context);
    }));
    /* Just dump all ACLs for now, with sw_if_index = ~0 */
    pool_foreach (acl, am->acls,
    ({
      send_acl_details(am, q, ~0, acl, mp->context);
    }));
    /* *INDENT-ON* */
  } else {
    VALIDATE_SW_IF_INDEX (mp);
    sw_if_index = ntohl (mp->sw_if_index);
    send_all_acl_details(am, q, sw_if_index, mp->context);
  }
  return;

  BAD_SW_IF_INDEX_LABEL;
  if (rv == -1) {
    /* FIXME API: should we signal an error here at all ? */
    return;
  }
}

/* Set up the API message handling tables */
static clib_error_t *
acl_plugin_api_hookup (vlib_main_t *vm)
{
  acl_main_t * sm = &acl_main;
#define _(N,n)                                                  \
    vl_msg_api_set_handlers((VL_API_##N + sm->msg_id_base),     \
                           #n,					\
                           vl_api_##n##_t_handler,              \
                           vl_noop_handler,                     \
                           vl_api_##n##_t_endian,               \
                           vl_api_##n##_t_print,                \
                           sizeof(vl_api_##n##_t), 1);
    foreach_acl_plugin_api_msg;
#undef _

    return 0;
}

void
acl_setup_nodes (void)
{
  vlib_main_t *vm = vlib_get_main ();
  acl_main_t * am = &acl_main;
  vlib_node_t *n;

  n = vlib_get_node_by_name(vm, (u8 *) "l2-input-classify");
  am->l2_input_classify_next_acl = vlib_node_add_next_with_slot(vm, n->index, acl_in_node.index, ~0);
  n = vlib_get_node_by_name(vm, (u8 *) "l2-output-classify");
  am->l2_output_classify_next_acl = vlib_node_add_next_with_slot(vm, n->index, acl_out_node.index, ~0);

  feat_bitmap_init_next_nodes(vm, acl_in_node.index, L2INPUT_N_FEAT,  l2input_get_feat_names (), am->acl_in_node_input_next_node_index);
}



static clib_error_t *
acl_init (vlib_main_t * vm)
{
  acl_main_t * am = &acl_main;
  clib_error_t * error = 0;
  memset(am, 0, sizeof(*am));
  am->vlib_main = vm;
  am->vnet_main = vnet_get_main();

  u8 * name = format (0, "acl_%08x%c", api_version, 0);

  /* Ask for a correctly-sized block of API message decode slots */
  am->msg_id_base = vl_msg_api_get_msg_ids ((char *) name,
					    VL_MSG_FIRST_AVAILABLE);

  error = acl_plugin_api_hookup (vm);
  acl_setup_nodes();

  vec_free(name);

  return error;
}

VLIB_INIT_FUNCTION (acl_init);
