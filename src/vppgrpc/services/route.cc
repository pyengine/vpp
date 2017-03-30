
#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <vppgrpc/services/route.h>

extern "C" {
    #include <vpp/api/vpe_msg_enum.h>
}

using grpc::Status;

fdioRouteServiceImpl::fdioRouteServiceImpl(const Connection &conn):
    m_conn(conn)
{
}

Status fdioRouteServiceImpl::Create(
    ServerContext* context,
    const fdio::cvl_api_ip_add_del_route_t* request,
    fdio::cvl_api_ip_add_del_route_reply_t* reply)
{
    std::cout << "Server: route-add-del:"
              << request->DebugString();
    if (m_conn.transmit(VL_API_IP_ADD_DEL_ROUTE, *request, *reply))
    {
        std::cout << "Server: route-add-del-reply:"
                  << reply->DebugString();
    }
    else
    {
        std::cout << "Server: routeadd-del-reply: failed"
                  << std::endl;
    }
    return Status::OK;
}
