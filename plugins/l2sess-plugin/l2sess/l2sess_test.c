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
/*
 *------------------------------------------------------------------
 * l2sess_test.c - test harness plugin
 *------------------------------------------------------------------
 */

#include <vat/vat.h>
#include <vlibapi/api.h>
#include <vlibmemory/api.h>
#include <vlibsocket/api.h>
#include <vppinfra/error.h>

uword unformat_sw_if_index (unformat_input_t * input, va_list * args);

/* Declare message IDs */
#include <l2sess/l2sess_msg_enum.h>

/* define message structures */
#define vl_typedefs
#include <l2sess/l2sess_all_api_h.h>
#undef vl_typedefs

/* declare message handlers for each api */

#define vl_endianfun             /* define message structures */
#include <l2sess/l2sess_all_api_h.h>
#undef vl_endianfun

/* instantiate all the print functions we know about */
#define vl_print(handle, ...)
#define vl_printfun
#include <l2sess/l2sess_all_api_h.h>
#undef vl_printfun

/* Get the API version number. */
#define vl_api_version(n,v) static u32 api_version=(v);
#include <l2sess/l2sess_all_api_h.h>
#undef vl_api_version


typedef struct {
    /* API message ID base */
    u16 msg_id_base;
    vat_main_t *vat_main;
} l2sess_test_main_t;

l2sess_test_main_t l2sess_test_main;

#define foreach_standard_reply_retval_handler   \
_(l2sess_bind_tables_reply)

#define _(n)                                            \
    static void vl_api_##n##_t_handler                  \
    (vl_api_##n##_t * mp)                               \
    {                                                   \
        vat_main_t * vam = l2sess_test_main.vat_main;   \
        i32 retval = ntohl(mp->retval);                 \
        if (vam->async_mode) {                          \
            vam->async_errors += (retval < 0);          \
        } else {                                        \
            vam->retval = retval;                       \
            vam->result_ready = 1;                      \
        }                                               \
    }
foreach_standard_reply_retval_handler;
#undef _

/*
 * Table of message reply handlers, must include boilerplate handlers
 * we just generated
 */
#define foreach_vpe_api_reply_msg                                       \
_(L2SESS_BIND_TABLES_REPLY, l2sess_bind_tables_reply)


/* M: construct, but don't yet send a message */

#define M(T,t)                                                  \
do {                                                            \
    vam->result_ready = 0;                                      \
    mp = vl_msg_api_alloc(sizeof(*mp));                         \
    memset (mp, 0, sizeof (*mp));                               \
    mp->_vl_msg_id = ntohs (VL_API_##T + sm->msg_id_base);      \
    mp->client_index = vam->my_client_index;                    \
} while(0);

#define M2(T,t,n)                                               \
do {                                                            \
    vam->result_ready = 0;                                      \
    mp = vl_msg_api_alloc(sizeof(*mp)+(n));                     \
    memset (mp, 0, sizeof (*mp));                               \
    mp->_vl_msg_id = ntohs (VL_API_##T + sm->msg_id_base);      \
    mp->client_index = vam->my_client_index;                    \
} while(0);

/* S: send a message */
#define S (vl_msg_api_send_shmem (vam->vl_input_queue, (u8 *)&mp))

/* W: wait for results, with timeout */
#define W                                       \
do {                                            \
    timeout = vat_time_now (vam) + 1.0;         \
                                                \
    while (vat_time_now (vam) < timeout) {      \
        if (vam->result_ready == 1) {           \
            return (vam->retval);               \
        }                                       \
    }                                           \
    return -99;                                 \
} while(0);

static int api_l2sess_bind_tables (vat_main_t * vam)
{
    l2sess_test_main_t * sm = &l2sess_test_main;
    unformat_input_t * i = vam->input;
    f64 timeout;
    vl_api_l2sess_bind_tables_t * mp;
    u32 fwd_table_index = ~0;
    u32 rev_table_index = ~0;
    u8 bind_type = 0;

    if (!unformat (i, "%d", &fwd_table_index)) {
      errmsg ("missing forward table index\n");
      return -99;
    }
    if (!unformat (i, "%d", &rev_table_index)) {
      errmsg ("missing reverse table index\n");
      return -99;
    }

    /* Construct the API message */
    M(L2SESS_BIND_TABLES, l2sess_bind_tables);
    mp->fwd_table_index = ntohl(fwd_table_index);
    mp->rev_table_index = ntohl(rev_table_index);
    mp->bind_type = bind_type;

    /* send it... */
    S;

    /* Wait for a reply... */
    W;
}

/*
 * List of messages that the api test plugin sends,
 * and that the data plane plugin processes
 */
#define foreach_vpe_api_msg \
_(l2sess_bind_tables, "<fwd-table-idx> <rev-table-idx>")

void vat_api_hookup (vat_main_t *vam)
{
    l2sess_test_main_t * sm = &l2sess_test_main;
    /* Hook up handlers for replies from the data plane plug-in */
#define _(N,n)                                                  \
    vl_msg_api_set_handlers((VL_API_##N + sm->msg_id_base),     \
                           #n,                                  \
                           vl_api_##n##_t_handler,              \
                           vl_noop_handler,                     \
                           vl_api_##n##_t_endian,               \
                           vl_api_##n##_t_print,                \
                           sizeof(vl_api_##n##_t), 1);
    foreach_vpe_api_reply_msg;
#undef _

    /* API messages we can send */
#define _(n,h) hash_set_mem (vam->function_by_name, #n, api_##n);
    foreach_vpe_api_msg;
#undef _ 
 
    /* Help strings */
#define _(n,h) hash_set_mem (vam->help_by_name, #n, h);
    foreach_vpe_api_msg;
#undef _
}

clib_error_t * vat_plugin_register (vat_main_t *vam)
{
  l2sess_test_main_t * sm = &l2sess_test_main;
  u8 * name;

  sm->vat_main = vam;

  name = format (0, "l2sess_%08x%c", api_version, 0);
  sm->msg_id_base = vl_client_get_first_plugin_msg_id ((char *) name);

  if (sm->msg_id_base != (u16) ~0)
    vat_api_hookup (vam);

  vec_free(name);

  return 0;
}
