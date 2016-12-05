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
 * api_rpc_server.c - VPP core api GRPC server
 *------------------------------------------------------------------
 */

#include <iostream>
#include <memory>
#include <string>
#include <grpc++/grpc++.h>
#include <apis/protos/vppcore.grpc.pb.h>

#include "api_handler.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using vppcore::vppClientContext;
using vppcore::ShowVersionReply;
using vppcore::vppCommand;
using vppcore::vppCommandOutput;
using vppcore::vpp;
using vppcore::ip6_fib_details;

// Logic and data behind the server's behavior.
class vppServiceImpl final : public vpp::Service {
	Status ShowVersion(ServerContext* context,
			   const vppClientContext* request,
			ShowVersionReply* reply) override {
		reply->set_version(vpe_api_get_version());
		reply->set_build_date(vpe_api_get_build_date());
		reply->set_build_directory(vpe_api_get_build_directory());
		reply->set_program("vpe");
		return Status::OK;
	}
	Status ExecCli (ServerContext* context,
			const vppCommand* cli,
			vppCommandOutput *cli_out) override {

		std::string command = cli->command();
		const char *command_c = command.c_str();
		u8 *output = 0;
		u32 out_len = 0;

		if (0 == cli_handler(command_c, command.length(), &output, &out_len))
		{
			cli_out->set_cliout((char *)output, out_len);
                        vector_free (output);
		}

		return Status::OK;
	}
	Status ip6_fib_dump (ServerContext* context,
			   const vppClientContext* request,
			     ServerWriter<ip6_fib_details>* writer) override {
	  ip6_fib_dump_handler(writer);
	  return Status::OK;
	}
 
};
extern "C" void ip6_fib_writer (void *writer, vl_api_ip6_fib_details_t *mp,
                                vl_api_fib_path_t *path)
{
  ip6_fib_details fb;
  vppcore::fib_path *fp;
  
  fb.set_table_id(mp->table_id);
  fb.clear_address();
  fb.set_address(mp->address, 16);
  fb.set_address_length((u32) mp->address_length);

  for (int i = 0; i < mp->count; i++)
    {
      fp = fb.add_path();
      fp->set_is_drop(path[i].is_drop);
      fp->set_is_local(path[i].is_local);
      fp->set_is_prohibit(path[i].is_prohibit);
      fp->set_is_unreach(path[i].is_unreach);
      fp->set_next_hop(path[i].next_hop, 16);
      fp->set_sw_if_index(path[i].sw_if_index);
      fp->set_afi(path[i].afi);
      fp->set_weight(path[i].weight);

    }

  static_cast<ServerWriter<ip6_fib_details>*>(writer)->Write(fb);
}

extern "C" void RunServer() {
	std::string server_address("0.0.0.0:50051");
	vppServiceImpl service;

	ServerBuilder builder;
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
	builder.RegisterService(&service);
	// Finally assemble the server.
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << server_address << std::endl;

	// Wait for the server to shutdown. Note that some other thread must be
	// responsible for shutting down the server for this call to ever return.
	server->Wait();
}

