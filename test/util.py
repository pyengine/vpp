
from scapy.layers.l2 import Ether, ARP
from scapy.layers.inet6 import IPv6, ICMPv6ND_NS, ICMPv6NDOptSrcLLAddr


class Util(object):

    @classmethod
    def resolve_arp(cls, args):
        for i in args:
            ip = cls.VPP_IP4S[i]
            cls.log("Sending ARP request for %s on port %u" % (ip, i))
            arp_req = (Ether(dst="ff:ff:ff:ff:ff:ff", src=cls.MY_MACS[i]) /
                       ARP(op=ARP.who_has, pdst=ip,
                           psrc=cls.MY_IP4S[i], hwsrc=cls.MY_MACS[i]))
            cls.pg_add_stream(i, arp_req)
            cls.pg_enable_capture([i])

            cls.cli(2, "trace add pg-input 1")
            cls.pg_start()
            arp_reply = cls.pg_get_capture(i)[0]
            if arp_reply[ARP].op == ARP.is_at:
                cls.log("VPP pg%u MAC address is %s " % (i, arp_reply[ARP].hwsrc))
                cls.VPP_MACS[i] = arp_reply[ARP].hwsrc
            else:
                cls.log("No ARP received on port %u" % i)
            cls.cli(2, "show trace")

    @classmethod
    def resolve_icmpv6_nd(cls, args):
        for i in args:
            ip = cls.VPP_IP6S[i]
            cls.log("Sending ICMPv6ND_NS request for %s on port %u" % (ip, i))
            nd_req = (Ether(dst="ff:ff:ff:ff:ff:ff", src=cls.MY_MACS[i]) /
                      IPv6(src=cls.MY_IP6S[i], dst=ip) /
                      ICMPv6ND_NS(tgt=ip) /
                      ICMPv6NDOptSrcLLAddr(lladdr=cls.MY_MACS[i]))
            cls.pg_add_stream(i, nd_req)
            cls.pg_enable_capture([i])

            cls.cli(2, "trace add pg-input 1")
            cls.pg_start()
            nd_reply = cls.pg_get_capture(i)[0]
            icmpv6_na = nd_reply['ICMPv6 Neighbor Discovery - Neighbor Advertisement']
            dst_ll_addr = icmpv6_na['ICMPv6 Neighbor Discovery Option - Destination Link-Layer Address']
            cls.VPP_MACS[i] = dst_ll_addr.lladdr

    @classmethod
    def config_ip4(cls, args):
        for i in args:
            cls.MY_IP4S[i] = "172.16.%u.2" % i
            cls.VPP_IP4S[i] = "172.16.%u.1" % i
            cls.api("sw_interface_add_del_address pg%u %s/24" % (i, cls.VPP_IP4S[i]))
            cls.log("My IPv4 address is %s" % (cls.MY_IP4S[i]))

    @classmethod
    def config_ip6(cls, args):
        for i in args:
            cls.MY_IP6S[i] = "fd00:%u::2" % i
            cls.VPP_IP6S[i] = "fd00:%u::1" % i
            cls.api("sw_interface_add_del_address pg%u %s/32" % (i, cls.VPP_IP6S[i]))
            cls.log("My IPv6 address is %s" % (cls.MY_IP6S[i]))
