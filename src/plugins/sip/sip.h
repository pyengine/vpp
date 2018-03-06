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

#ifndef SIP_H
#define SIP_H

#include <sip/sip_proto.h>

#include <vnet/vnet.h>
#include <vlibmemory/api.h>
#include <vnet/session/application.h>
#include <vnet/session/application_interface.h>

#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define MSG_LOG2_HASHSIZE (18)

typedef enum
{
  EVENT_WAKEUP = 1,
} sip_process_event_t;

typedef enum
{
  SIP_INVALID = 1 << 1,
  SIP_REQUEST = 1 << 2,
  SIP_RESPONSE = 1 << 3
} msg_type_t;

typedef enum
{
  SUBSCRIBE_METHOD = 1 << 1,
  REGISTER_METHOD  = 1 << 2,
  MESSAGE_METHOD   = 1 << 3,
  OPTIONS_METHOD   = 1 << 4,
  PUBLISH_METHOD   = 1 << 5,
  INVITE_METHOD    = 1 << 6,
  CANCEL_METHOD    = 1 << 7,
  UPDATE_METHOD    = 1 << 8,
  NOTIFY_METHOD    = 1 << 9,
  PRACK_METHOD     = 1 << 10,
  REFER_METHOD     = 1 << 11,
  INFO_METHOD      = 1 << 12,
  ACK_METHOD       = 1 << 13,
  BYE_METHOD       = 1 << 14,
  UNKNOWN_METHOD   = 1 << 31
} method_type_t;
  
typedef struct
{
  msg_type_t type;              /**< Type of SIP Message - Request/Response */
  union {
    struct {
      method_type_t method;         /**< SIP Method type if Request */
      char * uri;                     /**< Request URI      */
      char * version;                 /**< SIP Version      */
      u8 method_len;                /**< Length of method */
      u8 uri_len;                   /**< Length of uri    */
    } request;
    struct {
      char * version;                 /**< SIP Version      */
      char * status;                  /**< Status code      */
      char * reason;                  /**< Reason Phrase    */
      u32 status_code;                /**< Numeric number of Status code */
      u8 reason_len;                  /**< Length of Reason Phrase */
    } response;
  } u;
} msg_start_t;

typedef struct
{
  sip_header_t from;
  sip_header_t to;
  sip_header_t via;
  sip_header_t max_forwards;
} hdr_fields_t;

typedef struct
{
  u32 sec;
  u32 nsec;
} timestamp_nsec_t;

typedef struct
{
  msg_start_t  start_line;        /**< Message start line */

  hdr_fields_t fields;

  timestamp_nsec_t msg_start;
  timestamp_nsec_t msg_end; 
} msg_entry_t;

typedef struct
{
  u64 session_handle;
  u64 node_index;
  u8 * data;
} sip_proxy_args;

typedef struct {
  /*
   * Server app parameters
   */
  u8 **rx_buf;
  unix_shared_memory_queue_t **vpp_queue;
  u64 byte_index;

  uword **handler_by_sip_request;

  u32 *free_sip_cli_process_node_indices;

  unix_shared_memory_queue_t *vl_input_queue;   /**< Server's event queue */

  u32 app_index;        /**< Server app index */
  u32 my_client_index;  /**< API client handle */
  u32 node_index;       /**< process node index for event scheduling */

  /* Per CPU flow state */
  u8 ht_log2len;        /**< Hash table size is 2^log2len */
  u32 **hash_per_worker;
  msg_entry_t **pool_per_worker;

  /*
   * Config params
   */
  char *server_uri;             /**< Server URI */
  u32 prealloc_fifos;           /**< Preallocate fifos */
  u32 private_segment_size;     /**< Size of private segments */
  u32 private_segment_count;    /**< Number of private segments */

  vlib_main_t *vlib_main;
} sip_proxy_main_t;

sip_proxy_main_t sip_proxy_main;

extern int parse_msg(u8 *request, u32 len, msg_entry_t* me);

#endif //SIP_H
