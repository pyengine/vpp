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

#ifndef API_HANDLER_H
#define API_HANDLER_H

#include <vppinfra/types.h>
#define vl_typedefs             /* define message structures */
#include <vpp/api/vpe_all_api_h.h>
#undef vl_typedefs

extern "C" int
cli_handler(const char *cli, u32 cli_length,
		u8 **reply, u32 *reply_length);
extern "C" void vector_free(u8 *vector);

extern "C" char *vpe_api_get_build_directory (void);
extern "C" char *vpe_api_get_version (void);
extern "C" char *vpe_api_get_build_date (void);
extern "C" void ip6_fib_dump_handler (void *writer);
#endif
