#include <arpa/inet.h>

#include <iostream>
#include <vppgpb/decoder.h>
#include <protos/vpe.api.pb.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

extern "C"
{
    #include <vppgpb/cdecode.h>
    #include <vpp/api/vpe_msg_enum.h>
    #include <vpp/api/vpe_msg_enum.h>
    #define vl_typedefs		/* define message structures */
    #include <vppgpb/vppgpb.api.h>
    #undef vl_typedefs
    #include <vppgpb/cvnet.h>
}


void* cdecode (uint32_t type,
               const uint8_t *data,
               uint32_t size)
{
    return (Decoder::decode(type, data, size));
}

static void*
decode_create_loopback (const cvl_api_create_loopback_t &cl)
{
    printf("loopbak create\n");
    cvl_api_create_loopback_reply_t clr;
    vl_api_gpb_reply_t *rmp;
    u32 sw_if_index;
    i32 cmac[6];
    size_t bytes;
    int rv;
    const std::string *const *mac = cl.mac_address().data();

    std::cout << "mac size: " <<cl.mac_address().size()  << std::endl;
    std::cout << "context: " <<cl.context() << std::endl;
    std::cout << "client-index: " <<cl.client_index()  << std::endl;
    rv = cvnet_create_loopback_interface (&sw_if_index,
                                          (uint8_t*)((*mac)->c_str()),
                                          0, 0);

    clr.set_retval(rv);
    clr.set_sw_if_index(sw_if_index);
    bytes = clr.ByteSize();
    
    rmp = static_cast<vl_api_gpb_reply_t *>(vppgpb_get_reply_buffer(bytes));
    rmp->type = ntohl(VL_API_CREATE_LOOPBACK_REPLY);
    rmp->count = ntohl(bytes);

    clr.SerializeToArray(rmp->data, bytes);

    return (rmp);
}

static DecodeTemplate<VL_API_CREATE_LOOPBACK,
                      cvl_api_create_loopback_t>
decode_create_loopback_functor(decode_create_loopback);

static void*
decode_create_loopback_reply (const cvl_api_create_loopback_reply_t &clr)
{
    printf("loopbak create reply: %d\n", clr.sw_if_index());
    return (NULL);
}

static DecodeTemplate<VL_API_CREATE_LOOPBACK_REPLY,
                      cvl_api_create_loopback_reply_t>
decode_create_loopback_reply_functor(decode_create_loopback_reply);
