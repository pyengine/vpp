
#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <vppgrpc/services/interface.h>

extern "C" {
    #include <vpp/api/vpe_msg_enum.h>
}

using grpc::Status;

fdioInterfaceServiceImpl::fdioInterfaceServiceImpl(const Connection &conn):
    m_conn(conn)
{}

Status fdioInterfaceServiceImpl::CreateLoopback(
    ServerContext* context,
    const fdio::cvl_api_create_loopback_t* request,
    fdio::cvl_api_create_loopback_reply_t* reply)
{
    std::cout << "Server: create-loopback:"
              << request->DebugString();
    if (m_conn.transmit(VL_API_CREATE_LOOPBACK, *request, *reply))
    {
        std::cout << "Server: create-loopback-reply:"
                  << reply->DebugString();
    }
    else
    {
        std::cout << "Server: create-loopback-reply: failed"
                  << std::endl;
    }
    return Status::OK;
}

Status fdioInterfaceServiceImpl::SetState(
    ServerContext* context,
    const fdio::cvl_api_sw_interface_set_flags_t* request,
    fdio::cvl_api_sw_interface_set_flags_reply_t* reply)
{
    std::cout << "Server: set-state:"
              << request->DebugString();
    if (m_conn.transmit(VL_API_SW_INTERFACE_SET_FLAGS, *request, *reply))
    {
        std::cout << "Server: set-state-reply:"
                  << reply->DebugString();
    }
    else
    {
        std::cout << "Server: set-state-reply: failed"
                  << std::endl;
    }
    return Status::OK;
}

Status fdioInterfaceServiceImpl::SetAddress(
    ServerContext* context,
    const fdio::cvl_api_sw_interface_add_del_address_t* request,
    fdio::cvl_api_sw_interface_add_del_address_reply_t* reply)
{
    std::cout << "Server: add-del-address:"
              << request->DebugString();
    if (m_conn.transmit(VL_API_SW_INTERFACE_ADD_DEL_ADDRESS,
                        *request, *reply))
    {
        std::cout << "Server: add-del-address-reply:"
                  << reply->DebugString();
    }
    else
    {
        std::cout << "Server: add-del-address-reply: failed"
                  << std::endl;
    }
    return Status::OK;
}
