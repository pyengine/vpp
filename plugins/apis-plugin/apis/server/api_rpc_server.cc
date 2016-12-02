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
using vppcore::vppClientContext;
using vppcore::ShowVersionReply;
using vppcore::vppCommand;
using vppcore::vppCommandOutput;
using vppcore::vpp;

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
};

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

