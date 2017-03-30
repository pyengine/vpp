
syntax = "proto3";

option go_package = "fdio";
package fdio;

import "vpp/api/vpe.proto";
import "vnet/interface.proto";
import "vnet/ip/ip.proto";

// fd.io service definition.
service Interface {
  // Create a loopback Interface
  rpc CreateLoopback (cvl_api_create_loopback_t)
    returns (cvl_api_create_loopback_reply_t)
  {}

  rpc SetState (cvl_api_sw_interface_set_flags_t)
    returns (cvl_api_sw_interface_set_flags_reply_t)
  {}

  rpc SetAddress (cvl_api_sw_interface_add_del_address_t)
    returns (cvl_api_sw_interface_add_del_address_reply_t)
  {}
}

service Route {
  rpc Create (cvl_api_ip_add_del_route_t)
    returns (cvl_api_ip_add_del_route_reply_t)
    {}
}