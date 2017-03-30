
#include <iostream>
#include <vpp/api/vpe.pb.h>
#include <vnet/interface.pb.h>
#include <vppgpb/connection.h>
#include <vppgpb/capi_client.h>

extern "C" {
    #include <vpp/api/vpe_msg_enum.h>
}

int main (void)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    capi_init();
    Connection cc("GPB");

    fdio::cvl_api_create_loopback_t lc;
    fdio::cvl_api_create_loopback_reply_t lcr;
    uint8_t mac[] = {
        6, 7, 8,
    };
    
    lc.add_mac_address(mac, 3);

    std::cout << "name: " << lc.descriptor()->full_name() << std::endl;
    std::cout << lc.DebugString();

    cc.transmit(VL_API_CREATE_LOOPBACK, lc, lcr);
    std::cout << lcr.DebugString();

    fdio::cvl_api_sw_interface_set_flags_t swsf;
    fdio::cvl_api_sw_interface_set_flags_reply_t swsfr;
    
    swsf.set_admin_up_down(1);
    swsf.set_sw_if_index(lcr.sw_if_index());

    std::cout << swsf.DebugString();

    cc.transmit(VL_API_SW_INTERFACE_SET_FLAGS, swsf, swsfr);
    std::cout << swsfr.DebugString();

    fdio::cvl_api_sw_interface_add_del_address_t swad;
    fdio::cvl_api_sw_interface_add_del_address_reply_t swadr;
    
    swad.set_sw_if_index(lcr.sw_if_index());
    swad.set_address_length(24);
    uint8_t addr[] = {
        0xab,0xae,0xdd,0xfe,
    };
    swad.add_address(addr, 4);
    swad.set_is_add(1);

    std::cout << swad.DebugString();

    cc.transmit(VL_API_SW_INTERFACE_ADD_DEL_ADDRESS, swad, swadr);
    std::cout << swadr.DebugString();

    return 1;
}
