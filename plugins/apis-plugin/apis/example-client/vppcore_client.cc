#include <iostream>
#include <memory>
#include <string>
#include <sys/time.h>
#include <grpc++/grpc++.h>
#include <math.h>
#include "../example-client/vppcore.grpc.pb.h"

using
  grpc::Channel;
using
  grpc::ClientContext;
using
  grpc::Status;
using
  vppcore::vppClientContext;
using
  vppcore::ShowVersionReply;
using
  vppcore::vpp;

class
  vppClient
{
public:
  vppClient (std::shared_ptr < Channel > channel):
  stub_ (vpp::NewStub (channel))
  {
  }

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string
  PerfShowVersion (const std::string & user, float number)
  {
    // Data we are sending to the server.
    vppcore::vppClientContext request;
    request.set_name (user);

    // Container for the data we expect from the server.
    ShowVersionReply
      reply;
    struct timeval
      start,
      end;
    Status
      status;

    gettimeofday (&start, NULL);
    for (int i = 0; i < number; i++)
      {
	ClientContext
	  context;
	status = stub_->ShowVersion (&context, request, &reply);
      }
    gettimeofday (&end, NULL);
    long long
      time = (end.tv_sec * (unsigned int) 1e6 + end.tv_usec) -
      (start.tv_sec * (unsigned int) 1e6 + start.tv_usec);
    if (status.ok ())
      {
	std::cout << "time taken = " << time << "µs" << std::endl;
	std::cout << "cps = " << pow (10, 6) * number / time << std::endl;
	std::cout << status.error_code () << ": " << status.error_message ()
	  << std::endl;
	std::cout << reply.program () << ": " << reply.version () << ": "
	  << reply.build_date () << ": " << reply.build_directory ()
	  << std::endl;
	return reply.version ();
      }
    else
      {
	std::cout << status.error_code () << ": " << status.error_message ()
	  << std::endl;
	return "RPC failed";
      }
  }


  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string ShowVersion (const std::string & user)
  {
    // Data we are sending to the server.
    vppcore::vppClientContext request;
    request.set_name (user);

    // Container for the data we expect from the server.
    ShowVersionReply
      reply;
    struct timeval
      start,
      end;
    Status
      status;
    ClientContext
      context;

    status = stub_->ShowVersion (&context, request, &reply);

    if (status.ok ())
      {
	std::cout << status.error_code () << ": " << status.error_message ()
	  << std::endl;
	std::cout << reply.program () << ": " << reply.version () << ": "
	  << reply.build_date () << ": " << reply.build_directory ()
	  << std::endl;
	return reply.version ();
      }
    else
      {
	std::cout << status.error_code () << ": " << status.error_message ()
	  << std::endl;
	return "RPC failed";
      }
  }

  std::string runCommand (const std::string & command)
  {
    // Data we are sending to the server.
    vppcore::vppCommand request;
    request.set_command (command);

    // Container for the data we expect from the server.
    vppcore::vppCommandOutput reply;
    ClientContext
      context;

    Status
      status = stub_->ExecCli (&context, request, &reply);

    if (status.ok ())
      {
	std::cout << status.error_code () << ": " << status.error_message ()
	  << std::endl;
	std::cout << reply.cliout () << std::endl;
	return reply.cliout ();
      }
    else
      {
	std::cout << status.error_code () << ": " << status.error_message ()
	  << std::endl;
	return "RPC failed";
      }
  }


private:
  std::unique_ptr < vpp::Stub > stub_;
};

int
main (int argc, char **argv)
{
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  std::string target = "localhost:50051";
  int
    number_of_requests = 0;

  if (argc > 1)
    number_of_requests = atoi (argv[1]);
  if (argc > 2)
    target = argv[2];

  vppClient
  vpp (grpc::CreateChannel (target, grpc::InsecureChannelCredentials ()));
  std::string user ("cppvpp");
  std::string reply = vpp.ShowVersion (user);
  std::cout << "Version received: " << reply << std::endl;

  if (number_of_requests > 0)
    vpp.PerfShowVersion (user, number_of_requests);
  //std::string show_interface_out = vpp.runCommand("show interface");
  //std::cout << "show interface: " << show_interface_out << std::endl;
  return 0;
}
