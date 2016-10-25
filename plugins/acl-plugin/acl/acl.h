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
#ifndef included_acl_h
#define included_acl_h

#include <vnet/vnet.h>
#include <vnet/ip/ip.h>
#include <vnet/ethernet/ethernet.h>

#include <vppinfra/hash.h>
#include <vppinfra/error.h>
#include <vppinfra/elog.h>

vlib_node_registration_t acl_node;

enum address_e { IP4, IP6 };
typedef struct
{
  enum address_e type;
  union {
    ip6_address_t ip6;
    ip4_address_t ip4;
  } addr;
} address_t;

/*
 * ACL rules
 */
typedef struct
{
  u8 is_permit;
  u8 is_ipv6;
  ip46_address_t src;
  u8 src_prefixlen;
  ip46_address_t dst;
  u8 dst_prefixlen;
  u8 proto;
  u16 src_port;
  u16 dst_port;
} acl_rule_t;

typedef struct
{
  u8 is_permit;
  u16 type;
  u8 src[6];
  u8 dst[6];
} l2_acl_rule_t;

/*
 * ACL
 */
typedef struct
{
  u32 count;
  acl_rule_t *rules;
} acl_list_t;

typedef struct
{
  u32 count;
  l2_acl_rule_t *rules;
} l2_acl_list_t;

typedef struct {
  /* API message ID base */
  u16 msg_id_base;

  acl_list_t *acls;	/* Pool of ACLs */
  l2_acl_list_t *l2_acls;	/* Pool of L2 ACLs */

  /* convenience */
  vlib_main_t * vlib_main;
  vnet_main_t * vnet_main;
  ethernet_main_t * ethernet_main;
} acl_main_t;

extern acl_main_t acl_main;


#endif
