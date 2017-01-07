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
/*
 *------------------------------------------------------------------
 * api_server.c - VPP core api server
 *------------------------------------------------------------------
 */
#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <vlibapi/api.h>
#include <vlibmemory/api.h>
#include <vlibsocket/api.h>
#include <signal.h>
#include <vlib/threads.h>

typedef struct {
  /* convenience */
  vlib_main_t * vlib_main;
  vnet_main_t * vnet_main;
  u32 dummy_process_node_index;
} vpp_core_api_main_t;

vpp_core_api_main_t vpp_core_api_main;

vlib_main_t *api_server_get_vlib_main(void)
{
  return(vpp_core_api_main.vlib_main);
}

u32 api_server_process_node_index (void)
{
  return(vpp_core_api_main.dummy_process_node_index);
}

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
	vpp_core_api_main_t *am = &vpp_core_api_main;
	clib_error_t *error = 0;

	memset(am, 0, sizeof(vpp_core_api_main));
	am->vlib_main = vm;
	am->vnet_main = h->vnet_main;

	return error;
}

extern void RunServer();

static void
api_server_thread (void *arg)
{
  vlib_thread_main_t *tm = vlib_get_thread_main ();
  vlib_worker_thread_t *w = (vlib_worker_thread_t *) arg;

  /* stats thread wants no signals. */
  {
    sigset_t s;
    sigfillset (&s);
    pthread_sigmask (SIG_SETMASK, &s, 0);
  }
  if (vec_len (tm->thread_prefix))
    vlib_set_thread_name ((char *)
			  format (0, "%v_grpc_svr%c", tm->thread_prefix, '\0'));
  clib_mem_set_heap(w->thread_mheap);
  while (1)
    {
      RunServer();
    }
}
static vlib_node_registration_t api_server_dummy_process_node;

static uword
api_server_dummy_process (vlib_main_t * vm,
		     vlib_node_runtime_t * rt, vlib_frame_t * f)
{
  vpp_core_api_main_t *sm = &vpp_core_api_main;

  sm->dummy_process_node_index = api_server_dummy_process_node.index;
  /* Wait for Godot... */
  vlib_process_wait_for_event_or_clock (vm, 1e9);

  while (1)
    {
      vlib_process_wait_for_event_or_clock (vm, 1e9);
    }
  return 0;			/* not so much */
}
/* *INDENT-OFF* */
VLIB_REGISTER_NODE (api_server_dummy_process_node, static) =
{
 .function = api_server_dummy_process,
 .type = VLIB_NODE_TYPE_PROCESS,
 .name = "api-server-process",
};

VLIB_REGISTER_THREAD (api_server_thread_node, static) =
{
  .name = "grpc_api_server",
  .function = api_server_thread,
  .name = "api-server-thread",
  .fixed_count = 1,
  .count = 1,
  .no_data_structure_clone = 1,
  .use_pthreads = 1,
};

/* *INDENT-ON* */

static clib_error_t *
api_server_init (vlib_main_t * vm)
{
	clib_error_t *error = 0;
	return error;
}

VLIB_INIT_FUNCTION (api_server_init);
