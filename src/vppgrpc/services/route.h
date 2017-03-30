
#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <vppgpb/connection.h>
#include <vppgrpc/server.grpc.pb.h>
#include <vnet/ip/ip.pb.h>

extern "C" {
    #include <vpp/api/vpe_msg_enum.h>
}

using grpc::ServerContext;
using grpc::Status;

class fdioRouteServiceImpl final : public fdio::Route::Service
{
public:
    fdioRouteServiceImpl(const Connection &conn);

    Status Create(ServerContext* context,
                  const fdio::cvl_api_ip_add_del_route_t* request,
                  fdio::cvl_api_ip_add_del_route_reply_t* reply)
        override;

private:
    const Connection &m_conn;
};

