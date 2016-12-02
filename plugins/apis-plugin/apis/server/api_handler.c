/*
 *------------------------------------------------------------------
 * api_handler.c - message handler for grpc server
 *
 * Copyright (c) 2010-2016 Cisco and/or its affiliates.
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
 *------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#include <vppinfra/clib.h>
#include <vppinfra/vec.h>
#include <vppinfra/hash.h>
#include <vppinfra/bitmap.h>
#include <vppinfra/fifo.h>
#include <vppinfra/time.h>
#include <vppinfra/mheap.h>
#include <vppinfra/heap.h>
#include <vppinfra/pool.h>
#include <vppinfra/format.h>
#include <vppinfra/error.h>

#include <vnet/api_errno.h>
#include <vnet/vnet.h>

extern vlib_main_t *api_server_get_vlib_main(void);
extern u32 api_server_process_node_index(void);

static void
inband_cli_output (uword arg, u8 * buffer, uword buffer_bytes)
{
  u8 **mem_vecp = (u8 **) arg;
  u8 *mem_vec = *mem_vecp;
  u32 offset = vec_len (mem_vec);

  vec_validate (mem_vec, offset + buffer_bytes - 1);
  clib_memcpy (mem_vec + offset, buffer, buffer_bytes);
  *mem_vecp = mem_vec;
}

int
cli_handler (const char *cli, u32 cli_length,
		u8 **reply, u32 *reply_length)
{
  vlib_main_t *vm = api_server_get_vlib_main ();
  unformat_input_t input;

  /*
   * CLI input/output functions are looked up from
   * current process node. This is run in thread context.
   * So setting the current node index here
   */

  while (__sync_lock_test_and_set (vm->main_lockp, 1))
    ;

  vm->node_main.current_process_index = api_server_process_node_index();

  *reply = 0;
  unformat_init_string (&input, (char *)cli, cli_length);
  vlib_cli_input (vm, &input, inband_cli_output, (uword) reply);
  vm->node_main.current_process_index = ~0;
  *reply_length = vec_len (*reply);
  *vm->main_lockp = 0;
  return(0);
}

void vector_free (u8 *vec)
{
  vec_free (vec);
}
