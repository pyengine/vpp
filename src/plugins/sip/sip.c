/*
 * Copyright (c) 2017 Choonho Son and/or its affiliates.
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

#include <sip/sip.h>
#include <vnet/plugin/plugin.h>
#include <vpp/app/version.h>

static const char
  *sip_start_line = "SIP/2.0 %s %s\r\n";

static const char
  *sip_header_template = "Via: SIP/2.0/TCP\r\n";

static const char
  *sip_error_template = "SIP/2.0 %s\r\n";


static void
free_sip_process (sip_proxy_args * args)
{
  vlib_node_runtime_t *rt;
  vlib_main_t *vm = &vlib_global_main;
  sip_proxy_main_t *spm = &sip_proxy_main;
  vlib_node_t *n;
  u32 node_index;
  sip_proxy_args **save_args;

  node_index = args->node_index;
  ASSERT (node_index != 0);

  n = vlib_get_node (vm, node_index);
  rt = vlib_node_get_runtime (vm, n->index);
  save_args = vlib_node_get_runtime_data (vm, n->index);

  /* Reset process session pointer */
  clib_mem_free (*save_args);
  *save_args = 0;

  /* Turn off the process node */
  vlib_node_set_state (vm, rt->node_index, VLIB_NODE_STATE_DISABLED);

  /* add node index to the freelist */
  vec_add1 (spm->free_sip_cli_process_node_indices, node_index);
}


static void
sip_cli_output (uword args, u8 * buffer, uword buffer_bytes)
{
  u8 **output_vecp = (u8 **)args;
  u8 *output_vec;
  u32 offset;

  output_vec = *output_vecp;

  offset = vec_len (output_vec);
  vec_validate (output_vec, offset + buffer_bytes - 1);
  clib_memcpy (output_vec + offset, buffer, buffer_bytes);

  *output_vecp = output_vec;
}

void
send_sip_data (stream_session_t * s, u8 * data)
{
  session_fifo_event_t evt;
  u32 offset, bytes_to_send;
  f64 delay = 10e-3;
  sip_proxy_main_t *spm = &sip_proxy_main;
  vlib_main_t *vm = spm->vlib_main;
  f64 last_sent_timer = vlib_time_now (vm);

  bytes_to_send = vec_len (data);
  offset = 0;

  while (bytes_to_send > 0)
    {
      int actual_transfer;

      actual_transfer = svm_fifo_enqueue_nowait
    (s->server_tx_fifo, bytes_to_send, data + offset);

      /* Made any progress? */
      if (actual_transfer <= 0)
    {
      vlib_process_suspend (vm, delay);
      /* 10s deadman timer */
      if (vlib_time_now (vm) > last_sent_timer + 10.0)
        {
          /* $$$$ FC: reset transport session here? */
          break;
        }
      /* Exponential backoff, within reason */
      if (delay < 1.0)
        delay = delay * 2.0;
    }
      else
    {
      last_sent_timer = vlib_time_now (vm);
      offset += actual_transfer;
      bytes_to_send -= actual_transfer;

      if (svm_fifo_set_event (s->server_tx_fifo))
        {
          /* Fabricate TX event, send to vpp */
          evt.fifo = s->server_tx_fifo;
          evt.event_type = FIFO_EVENT_APP_TX;

          unix_shared_memory_queue_add (spm->vpp_queue[s->thread_index],
                        (u8 *) & evt,
                        0 /* do wait for mutex */ );
        }
      delay = 10e-3;
    }
  }
}

static void
send_sip_error (stream_session_t * s, char *str)
{
  u8 *data;
  data = format (0, sip_error_template, str);
  send_sip_data (s, data);
  vec_free (data);
}

static uword
sip_cli_process (vlib_main_t *vm,
          vlib_node_runtime_t * rt, vlib_frame_t * f)
{
  sip_proxy_main_t *spm = &sip_proxy_main;
  u8 *request = 0, *reply = 0;
  sip_proxy_args **save_args;
  sip_proxy_args *args;
  stream_session_t *s;
  unformat_input_t input;
  int rv;
  /* TODO u32 vs. u64 */
  u32 req_len;
  //int i;
  msg_entry_t _me, *me = &_me;
  msg_start_t _ms, *ms = &_ms;
  u8 *header = 0, *msg = 0;

  save_args = vlib_node_get_runtime_data (spm->vlib_main, rt->node_index);
  args = *save_args;
  s = session_get_from_handle (args->session_handle);
  ASSERT (s);

  request = (u8 *) (void *) (args->data);
  req_len = vec_len(request);
  // RFC 3261, 16.3 Validate the request
  if (req_len < 7)
    {
      goto bad_request;
    }
  
  // Parse Request URI (role as server or client)
  rv = parse_msg(request, req_len, me);
  if (rv < 0)
    goto bad_request;
  else if (rv == 0)
    goto response_msg;
  else
    goto request_msg;

bad_request:
  send_sip_error (s, "400 Bad Request");
  goto out;

response_msg:
  send_sip_error (s, "RESPONSE MSG");
  goto out;

request_msg:
  /* Parse All request */
  ms = (msg_start_t *)me;

  /* Generate the SIP header */

  /* Run the command */
  unformat_init_vector (&input, request);
  vlib_cli_input (vm, &input, sip_cli_output, (uword) &reply);
  unformat_free (&input);
  request = 0;
  if (ms->u.request.method == INVITE_METHOD) {
    /* Generate the start-line */
    msg = format (0, sip_start_line, "200", "OK");
  } else {
    msg = format (0, sip_start_line, "NOT", "INVITE");
  }
  /* Generate the message-header */
  header = format (0, sip_header_template);

  /* Generate the message-body */
    
  //msg = format (msg, header);

  /* Send it */
  send_sip_data (s, msg);

out:
  /* Cleanup */
  vec_free (request);
  vec_free (reply);
  vec_free (msg);
  vec_free (header);
  //vec_free (body);

  free_sip_process (args);
  return 0;
}
 
static void
alloc_sip_process (sip_proxy_args * args)
{
  char *name;
  vlib_node_t *n;
  sip_proxy_main_t *spm = &sip_proxy_main;
  vlib_main_t *vm = spm->vlib_main;
  uword l = vec_len (spm->free_sip_cli_process_node_indices);
  sip_proxy_args **save_args;

  if (vec_len (spm->free_sip_cli_process_node_indices) > 0)
    {
      n = vlib_get_node (vm, spm->free_sip_cli_process_node_indices[l - 1]);
      vlib_node_set_state (vm, n->index, VLIB_NODE_STATE_POLLING);
      _vec_len (spm->free_sip_cli_process_node_indices) = l - 1;
    }
  else
    {
      static vlib_node_registration_t r = {
    .function = sip_cli_process,
    .type = VLIB_NODE_TYPE_PROCESS,
    .process_log2_n_stack_bytes = 16,
    .runtime_data_bytes = sizeof (void *),
      };

      name = (char *) format (0, "sip-cli-%d", l);
      r.name = name;
      vlib_register_node (vm, &r);
      vec_free (name);

      n = vlib_get_node (vm, r.index);
    }

  /* Save the node index in the args. It won't be zero. */
  args->node_index = n->index;

  /* Save the args (pointer) in the node runtime */
  save_args = vlib_node_get_runtime_data (vm, n->index);
  *save_args = args;

  vlib_start_process (vm, n->runtime_index);
}

static void
alloc_sip_process_callback (void *cb_args)
{
  alloc_sip_process ((sip_proxy_args *) cb_args);
}

static int
sip_session_accept_callback(stream_session_t * s)
{
  sip_proxy_main_t *spm = &sip_proxy_main;

  spm->vpp_queue[s->thread_index] = 
    session_manager_get_vpp_event_queue (s->thread_index);
  s->session_state = SESSION_STATE_READY;
  spm->byte_index = 0;
  return 0;
}


static void
sip_session_disconnect_callback(stream_session_t * s)
{
  sip_proxy_main_t *spm = &sip_proxy_main;
  vnet_disconnect_args_t _a, *a = &_a;

  a->handle = session_handle (s);
  a->app_index = spm->app_index;
  vnet_disconnect_session (a);
}

static int
sip_session_connected_callback(u32 app_index, u32 api_contenxt,
              stream_session_t * s, u8 is_fail)
{
  clib_warning ("called...");
  return -1;
}

static int
sip_add_segment_callback (u32 client_index,
              const u8 * seg_name, u32 seg_size)
{
  clib_warning ("called...");
  return -1;
}

static int
sip_redirect_connect_callback( u32 client_index, void *mp)
{
  clib_warning ("called...");
  return -1;
}

static int
session_rx_request (stream_session_t * s)
{
  sip_proxy_main_t *spm = &sip_proxy_main;
  svm_fifo_t *rx_fifo;
  u32 max_dequeue;
  int actual_transfer;

  rx_fifo = s->server_rx_fifo;
  max_dequeue = svm_fifo_max_dequeue (rx_fifo);
  svm_fifo_unset_event (rx_fifo);
  if (PREDICT_FALSE(max_dequeue == 0))
    return -1;

  vec_validate (spm->rx_buf[s->thread_index], max_dequeue - 1);
  _vec_len (spm->rx_buf[s->thread_index]) = max_dequeue;

  actual_transfer = svm_fifo_dequeue_nowait (rx_fifo, max_dequeue,
                          spm->rx_buf[s->thread_index]);
  ASSERT (actual_transfer > 0);
  _vec_len (spm->rx_buf[s->thread_index]) = actual_transfer;
  return 0;
}

static int
sip_proxy_rx_callback (stream_session_t * s)
{
  sip_proxy_main_t *spm = &sip_proxy_main;
  sip_proxy_args *args;
  int rv;

  rv = session_rx_request (s);
  if (rv)
    return rv;

  /* send the command to a new//recycled vlib process */
  args = clib_mem_alloc (sizeof (*args));
  args->data = vec_dup (spm->rx_buf[s->thread_index]);
  args->session_handle = session_handle (s);

  /* Send an RPC request via the thread-0 input node */
  if (vlib_get_thread_index () != 0)
    {
      session_fifo_event_t evt;
      evt.rpc_args.fp = alloc_sip_process_callback;
      evt.rpc_args.arg = args;
      evt.event_type = FIFO_EVENT_RPC;
      unix_shared_memory_queue_add
    (session_manager_get_vpp_event_queue (0 /* main thread */ ),
     (u8 *) & evt, 0 /* do wait for mutex */ );
    }
  else
    alloc_sip_process (args);
  return 0;
}
 

static void
sip_session_reset_callback (stream_session_t * s)
{
  clib_warning ("called..");

  stream_session_cleanup (s);
}


static session_cb_vft_t sip_session_cb_vft = {
  .session_accept_callback = sip_session_accept_callback,
  .session_disconnect_callback = sip_session_disconnect_callback,
  .session_connected_callback = sip_session_connected_callback,
  .add_segment_callback = sip_add_segment_callback,
  .redirect_connect_callback = sip_redirect_connect_callback,
  .builtin_server_rx_callback = sip_proxy_rx_callback,
  .session_reset_callback = sip_session_reset_callback
};


/* Abuse VPP's input queue */
static int
sip_create_api_lookback (vlib_main_t * vm)
{
  sip_proxy_main_t *spm = &sip_proxy_main;
  api_main_t *am = &api_main;
  vl_shmem_hdr_t *shmem_hdr;

  shmem_hdr = am->shmem_hdr;
  spm->vl_input_queue = shmem_hdr->vl_input_queue;
  spm->my_client_index = 
    vl_api_memclnt_create_internal ("sip_proxy_server", spm->vl_input_queue);
  return 0;
}

static int
sip_server_attach ()
{
  sip_proxy_main_t *spm = &sip_proxy_main;
  u8 segment_name[128];
  u64 options[SESSION_OPTIONS_N_OPTIONS];
  vnet_app_attach_args_t _a, *a = &_a;

  memset (a, 0, sizeof (*a));
  memset (options, 0, sizeof (options));

  a->api_client_index = spm->my_client_index;
  a->session_cb_vft = &sip_session_cb_vft;
  a->options = options;
  //a->options[SESSION_OPTIONS_SEGMENT_SIZE] = 128 << 20;
  //a->options[SESSION_OPTIONS_SEGMENT_SIZE] = 8 << 24;
  a->options[SESSION_OPTIONS_RX_FIFO_SIZE] = 32 << 10;
  a->options[SESSION_OPTIONS_TX_FIFO_SIZE] = 32 << 10;
  a->options[APP_OPTIONS_FLAGS] = APP_OPTIONS_FLAGS_IS_BUILTIN;
  a->options[APP_OPTIONS_PRIVATE_SEGMENT_COUNT] = spm->private_segment_count;
  a->options[APP_OPTIONS_PRIVATE_SEGMENT_SIZE] = spm->private_segment_size;
  a->options[APP_OPTIONS_PREALLOC_FIFO_PAIRS] = 10240;
  a->segment_name = segment_name;
  a->segment_name_length = ARRAY_LEN (segment_name);

  if (vnet_application_attach (a))
    {
      clib_warning ("failed to attach server");
      return -1;
    }
  spm->app_index = a->app_index;
  return 0;
}

static int 
sip_server_listen ()
{
  sip_proxy_main_t *spm = &sip_proxy_main;
  vnet_bind_args_t _a,*a = &_a;
  memset (a, 0, sizeof (*a));
  a->app_index = spm->app_index;
  a->uri = spm->server_uri;
  return vnet_bind_uri (a);
}

static int
sip_server_create (vlib_main_t * vm)
{
  sip_proxy_main_t *spm = &sip_proxy_main;
  u32 num_threads;
  vlib_thread_main_t *vtm = vlib_get_thread_main ();

  ASSERT (spm->my_client_index == (u32) ~ 0);
  if (sip_create_api_lookback (vm))
    return -1;

  num_threads = 1 /* main thread */ + vtm->n_threads;
  vec_validate (spm->vpp_queue, num_threads - 1);

  if (sip_server_attach ())
    {
      clib_warning ("failed to attach server");
      return -1;
    }
  if (sip_server_listen ())
    {
      clib_warning ("failed to start listening");
      return -1;
    }
  // TODO: add client
  return 0;
}

/* *INDENT-OFF* */
VLIB_PLUGIN_REGISTER () = {
    .version = VPP_BUILD_VER,
    .description = "Session Initiate Procotol Proxy Server",
};
/* *INDENT-ON* */

clib_error_t *
sip_proxy_init (vlib_main_t * vm)
{
  sip_proxy_main_t *spm = &sip_proxy_main;
  vlib_thread_main_t *vtm = vlib_get_thread_main ();
  u32 num_threads;

  spm->my_client_index = ~0;
  spm->vlib_main = vm;
  spm->private_segment_size = 536870912;
  num_threads = 1 /* main thread */ + vtm->n_threads;
  vec_validate (spm->rx_buf, num_threads - 1);

  return 0;
}

VLIB_INIT_FUNCTION (sip_proxy_init);

static clib_error_t *
proxy_create_command_fn (vlib_main_t * vm, unformat_input_t * input,
            vlib_cli_command_t * cmd)
{
  sip_proxy_main_t *spm = &sip_proxy_main;
  vlib_thread_main_t *vtm = vlib_get_thread_main ();
  u8 server_uri_set = 0;
  int rv;
  u32 num_threads;
  int i;

  vec_free (spm->server_uri);

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (input, "uri %s", &spm->server_uri))
        server_uri_set = 1;
      else
        return clib_error_return (0, "unknown input `%U'",
                  format_unformat_error, input);
    }

  vnet_session_enable_disable (vm, 1 /* turn on TCP, etc. */ );

  if (!server_uri_set)
    spm->server_uri = (char *) format (0, "tcp://0.0.0.0/5060", 0);

  rv = sip_server_create (vm);

  switch (rv)
    {
    case 0:
      break;
    default:
      return clib_error_return (0, "sip_server_create returned %d", rv);
    }

  //sip_create_msg_tables ();
  spm->ht_log2len = MSG_LOG2_HASHSIZE;
  num_threads = 1 /* main thread */ + vtm->n_threads;

  vec_validate (spm->pool_per_worker, num_threads - 1);
  vec_validate (spm->hash_per_worker, num_threads - 1);

  for(i = 0; i < num_threads; i++)
  {
    pool_alloc (spm->pool_per_worker[i], 1 << spm->ht_log2len);
    vec_resize (spm->hash_per_worker[i], 1 << spm->ht_log2len);
  }

  return 0;
}

/* *INDENT-OFF* */
VLIB_CLI_COMMAND (proxy_create_command, static) =
{
  .path = "set sip proxy",
  .short_help = "set sip proxy [uri <tcp://ip/port>]",
  .function = proxy_create_command_fn,
};
/* *INDENT-ON* */
