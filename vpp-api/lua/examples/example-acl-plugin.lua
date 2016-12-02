--[[
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
]]


vpp = require "vpp-lapi"

root_dir = "/home/ubuntu/vpp"
pneum_path = root_dir .. "/build-root/install-vpp_debug-native/vpp-api/lib64/libpneum.so"

vpp:init({ pneum_path = pneum_path })

vpp:consume_api(root_dir .. "/build-root/install-vpp_debug-native/vlib-api/vlibmemory/memclnt.api")
vpp:consume_api(root_dir .. "/build-root/install-vpp_debug-native/vpp/vpp-api/vpe.api")
vpp:connect("aytest")
vpp:consume_api(root_dir .. "/plugins/acl-plugin/acl/acl.api", "acl")

ffi = require "ffi"
ffi.cdef([[
int inet_pton(int af, const char *src, void *dst);

]])


function ip46(addr_text)
  local out = ffi.new("char [200]")
  local AF_INET6 = 10
  local AF_INET = 2
  local is_ip6 = ffi.C.inet_pton(AF_INET6, vpp.c_str(addr_text), out)
  if is_ip6 == 1 then
    return ffi.string(out, 16), true
  end
  local is_ip4 = ffi.C.inet_pton(AF_INET, vpp.c_str(addr_text), out)
  if is_ip4 then
    return (ffi.string(out, 4).. string.rep("4", 12)), false
  end
end




-- api calls
--[[
reply = vpp:api_call("show_version")
print("Version: ", reply[1].version)
print(vpp.hex_dump(reply[1].version))
print(vpp.dump(reply))
print("---")

reply = vpp:api_call("acl_del", { context = 42, acl_index = 230 })
print(vpp.dump(reply))
print("---")

reply = vpp:api_call("acl_del", { context = 42, acl_index = 8 })
print(vpp.dump(reply))
print("---")

reply = vpp:api_call("acl_del", { context = 42, acl_index = 15 })
print(vpp.dump(reply))
print("---")

reply = vpp:api_call("acl_add_replace", { context = 42, acl_index = -1, count = 2, r = { { is_permit = 1, is_ipv6 = 1 }, { is_permit = 0, is_ipv6 = 1 } } })
print(vpp.dump(reply))
print("---")
interface_acl_in = reply[1].acl_index

reply = vpp:api_call("acl_add_replace", { context = 42, acl_index = -1, count = 3, r = { { is_permit = 1, is_ipv6 = 1 }, { is_permit = 0, is_ipv6 = 1 }, { is_permit = 1, is_ipv6 = 0 } } })
print(vpp.dump(reply))
print("---")
interface_acl_out = reply[1].acl_index


reply = vpp:api_call("acl_interface_add_del", { context = 42, sw_if_index = 0, is_add = 1, is_input = 1, acl_index = interface_acl_in })
print(vpp.dump(reply))
print("---")
reply = vpp:api_call("acl_interface_add_del", { context = 42, sw_if_index = 0, is_add = 1, is_input = 1, acl_index = interface_acl_in })
print(vpp.dump(reply))
print("---")

reply = vpp:api_call("acl_interface_add_del", { context = 42, sw_if_index = 0, is_add = 1, is_input = 0, acl_index = interface_acl_out })
print(vpp.dump(reply))
print("---")
reply = vpp:api_call("acl_interface_add_del", { context = 42, sw_if_index = 0, is_add = 1, is_input = 0, acl_index = interface_acl_out })
print(vpp.dump(reply))
print("---")

reply = vpp:api_call("acl_add_replace", { context = 42, count = 0, acl_index = -1 })
print(vpp.dump(reply))
print("---")

acl_index_to_delete = reply[1].acl_index
print("Deleting " .. tostring(acl_index_to_delete))
reply = vpp:api_call("acl_del", { context = 42, acl_index = acl_index_to_delete })
print(vpp.dump(reply))
print("---")

reply = vpp:api_call("acl_dump", { context = 42, acl_index = 0})
for ri, rv in ipairs(reply) do 
  print("Reply message #" .. tostring(ri))
  print(vpp.dump(rv))
  for ai, av in ipairs(rv.r) do
    print("ACL rule #" .. tostring(ai) .. " : " .. vpp.dump(av))
  end
   
end
print("---")

reply = vpp:api_call("acl_del", { context = 42, acl_index = interface_acl_out })
print(vpp.dump(reply))
print("---")
reply = vpp:api_call("acl_del", { context = 42, acl_index = interface_acl_in })
print(vpp.dump(reply))
print("---")

]]

r1 = {
  { is_permit = 1, is_ipv6 = 1, src_ip_addr = ip46("2001:db8:1::1"), src_ip_prefix_len = 128, dst_ip_addr = ip46("2001:db8:1::2"), dst_ip_prefix_len = 128 },
  { is_permit = 0, is_ipv6 = 1, src_ip_addr = ip46("2001:db8:1::1"), src_ip_prefix_len = 128, dst_ip_addr = ip46("2001:db8:1::4"), dst_ip_prefix_len = 128 },
  { is_permit = 2, is_ipv6 = 0, dst_ip_addr = ip46("192.0.2.2"), proto = 6, dstport_or_icmpcode_first = 22, dstport_or_icmpcode_last = 22,
                                                                            srcport_or_icmptype_first = 0, srcport_or_icmptype_last = 65535 },
  { is_permit = 1, is_ipv6 = 0 }

}

reply = vpp:api_call("acl_add_replace", { context = 42, acl_index = -1, count = 4, r = r1 })
-- reply = vpp:api_call("acl_add_replace", { context = 42, acl_index = -1, count = 1, r = r1 })
print(vpp.dump(reply))
print("---")

reply = vpp:api_call("acl_dump", { context = 42, acl_index = -1})
print(vpp.dump(reply))
print("---")


vpp:disconnect()


