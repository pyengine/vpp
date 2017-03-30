
#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <vppgrpc/server.grpc.pb.h>
#include <vppgpb/connection.h>

using grpc::ServerContext;
using grpc::Status;

// Logic and data behind the server's behavior.
class fdioInterfaceServiceImpl final : public fdio::Interface::Service
{
public:
    fdioInterfaceServiceImpl(const Connection &conn);

    Status CreateLoopback(ServerContext* context,
                          const fdio::cvl_api_create_loopback_t* request,
                          fdio::cvl_api_create_loopback_reply_t* reply)
        override;

    Status SetState(ServerContext* context,
                    const fdio::cvl_api_sw_interface_set_flags_t* request,
                          fdio::cvl_api_sw_interface_set_flags_reply_t* reply)
        override;

    Status SetAddress(ServerContext* context,
                      const fdio::cvl_api_sw_interface_add_del_address_t* request,
                      fdio::cvl_api_sw_interface_add_del_address_reply_t* reply)
        override;

private:
    const Connection &m_conn;
};

