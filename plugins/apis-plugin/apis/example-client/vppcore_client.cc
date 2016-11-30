#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>

#include "../example-client/vppcore.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using vppcore::vppClientContext;
using vppcore::ShowVersionReply;
using vppcore::vpp;

class vppClient {
 public:
  vppClient(std::shared_ptr<Channel> channel)
      : stub_(vpp::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string ShowVersion(const std::string& user) {
    // Data we are sending to the server.
    vppcore::vppClientContext request;
    request.set_name(user);

    // Container for the data we expect from the server.
    ShowVersionReply reply;
    ClientContext context;

    // The actual RPC.
    Status status = stub_->ShowVersion(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
    	std::cout << status.error_code() << ": " << status.error_message()
    			<< std::endl;
    	std::cout << reply.program() << ": " << reply.version() << ": "
    			<< reply.build_date() << ": "<< reply.build_directory()
				<< std::endl;
      return reply.version();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  std::string runCommand(const std::string& command) {
    // Data we are sending to the server.
    vppcore::vppCommand request;
    request.set_command(command);

    // Container for the data we expect from the server.
    vppcore::vppCommandOutput reply;
    ClientContext context;

    // The actual RPC.
    Status status = stub_->ExecCli(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
    	std::cout << status.error_code() << ": " << status.error_message()
    			<< std::endl;
    	std::cout << reply.cliout()
				<< std::endl;
      return reply.cliout();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }


 private:
  std::unique_ptr<vpp::Stub> stub_;
};

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  vppClient vpp(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  std::string user("cppvpp");
  std::string reply = vpp.ShowVersion(user);
  std::cout << "Version received: " << reply << std::endl;
  std::string show_interface_out = vpp.runCommand("show interface");
  std::cout << "show interface: " << show_interface_out << std::endl;
  return 0;
}
