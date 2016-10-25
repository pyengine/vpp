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
    rmp->_vl_msg_id = ntohs((t));                               \
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
    rmp->_vl_msg_id = ntohs((t));                               \
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
  *acl_list_index = a - am->acls;

  return 0;
}

static int
acl_del_list(u32 acl_list_index)
{
  acl_main_t *am = &acl_main;
  acl_list_t  * a;
  int i, ii;

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
  clib_mem_free(a->rules);
  pool_put(am->acls, a);
  return -1;
}

static int
acl_interface_add_inout_acl (u32 sw_if_index, u8 is_input, u32 acl_list_index)
{
  acl_main_t *am = &acl_main;
  if (is_input) {
    vec_validate(am->input_acl_vec_by_sw_if_index, sw_if_index);
    vec_add(am->input_acl_vec_by_sw_if_index[sw_if_index], &acl_list_index, 1);
  } else {
    vec_validate(am->output_acl_vec_by_sw_if_index, sw_if_index);
    vec_add(am->output_acl_vec_by_sw_if_index[sw_if_index], &acl_list_index, 1);
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
  } else {
    vec_validate(am->output_acl_vec_by_sw_if_index, sw_if_index);
    for (i=0; i<vec_len(am->output_acl_vec_by_sw_if_index[sw_if_index]); i++) {
      if (acl_list_index == am->output_acl_vec_by_sw_if_index[sw_if_index][i]) {
	vec_del1(am->output_acl_vec_by_sw_if_index[sw_if_index], i);
	rv = 0;
	break;
      }
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


/* API message handler */
static void
vl_api_acl_add_t_handler (vl_api_acl_add_t * mp)
{
  vl_api_acl_add_reply_t * rmp;
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
send_acl_details(unix_shared_memory_queue_t * q,
                         u32 sw_if_index, acl_list_t *acl, u32 context)
{
  vl_api_acl_details_t *mp;
  int msg_size = sizeof (*mp);

  mp = vl_msg_api_alloc (msg_size);
  memset (mp, 0, msg_size);
  mp->_vl_msg_id = ntohs (VL_API_ACL_DETAILS);

  /* fill in the message */
  mp->context = context;
  mp->sw_if_index = htonl(sw_if_index);
  mp->count = htonl(acl->count);
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
    send_acl_details(q, sw_if_index, &am->acls[*p_acl_index], context);
  }
  vec_foreach (p_acl_index, am->output_acl_vec_by_sw_if_index[sw_if_index]) {
    send_acl_details(q, sw_if_index, &am->acls[*p_acl_index], context);
  }
}


static void
vl_api_acl_dump_t_handler (vl_api_acl_dump_t * mp)
{
  acl_main_t * am = &acl_main;
  vnet_sw_interface_t *swif;
  vnet_interface_main_t *im = &am->vnet_main->interface_main;
  int rv = -1;
  
  u32 sw_if_index;
  unix_shared_memory_queue_t *q;
  VALIDATE_SW_IF_INDEX (mp);

  sw_if_index = ntohl (mp->sw_if_index);

  q = vl_api_client_index_to_input_queue (mp->client_index);
  if (q == 0)
    {
      return;
    }
  
  if (~0 == sw_if_index) {
    /* *INDENT-OFF* */
    pool_foreach (swif, im->sw_interfaces,
    ({
      send_all_acl_details(am, q, swif->sw_if_index, mp->context);
    }));
    /* *INDENT-ON* */
  } else {
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


static clib_error_t *
acl_init (vlib_main_t * vm)
{
  acl_main_t * am = &acl_main;
  clib_error_t * error = 0;

  u8 * name = format (0, "acl_%08x%c", api_version, 0);

  /* Ask for a correctly-sized block of API message decode slots */
  am->msg_id_base = vl_msg_api_get_msg_ids ((char *) name,
					    VL_MSG_FIRST_AVAILABLE);

  error = acl_plugin_api_hookup (vm);

  vec_free(name);

  return error;
}

VLIB_INIT_FUNCTION (acl_init);
