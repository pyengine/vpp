/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <sstream>

#include "vom/acl_l3_rule.hpp"

using namespace VOM::ACL;

l3_rule::l3_rule(uint32_t priority,
               const action_t &action,
               const route::prefix_t &src,
               const route::prefix_t &dst):
    m_priority(priority),
    m_action(action),
    m_src(src),
    m_dst(dst),
    m_proto(0),
    m_srcport_or_icmptype_first(0),
    m_srcport_or_icmptype_last(0),
    m_dstport_or_icmpcode_first(0),
    m_dstport_or_icmpcode_last(0),
    m_tcp_flags_mask(0),
    m_tcp_flags_value(0)
{
}

bool l3_rule::operator<(const l3_rule &other) const
{
    return (other.m_priority < m_priority);
}

void l3_rule::to_vpp(vapi_type_acl_rule &rule) const
{
    rule.is_permit = m_action.value();
    m_src.to_vpp(&rule.is_ipv6, rule.src_ip_addr, &rule.src_ip_prefix_len);
    m_dst.to_vpp(&rule.is_ipv6, rule.dst_ip_addr, &rule.dst_ip_prefix_len);

    rule.proto = m_proto;
    rule.srcport_or_icmptype_first = m_srcport_or_icmptype_first;
    rule.srcport_or_icmptype_last  = m_srcport_or_icmptype_last;
    rule.dstport_or_icmpcode_first = m_dstport_or_icmpcode_first;
    rule.dstport_or_icmpcode_last  = m_dstport_or_icmpcode_last;

    rule.tcp_flags_mask  = m_tcp_flags_mask;
    rule.tcp_flags_value = m_tcp_flags_value;
}

bool l3_rule::operator==(const l3_rule &rule) const
{
    return ((m_action == rule.m_action) &&
            (m_src == rule.m_src) &&
            (m_dst == rule.m_dst) &&
	    (m_proto == rule.m_proto) &&
	    (m_srcport_or_icmptype_first == rule.m_srcport_or_icmptype_first) &&
            (m_srcport_or_icmptype_last == rule.m_srcport_or_icmptype_last) &&
            (m_dstport_or_icmpcode_first == rule.m_dstport_or_icmpcode_first) &&
            (m_dstport_or_icmpcode_last == rule.m_dstport_or_icmpcode_last) &&
            (m_tcp_flags_mask == rule.m_tcp_flags_mask) &&
	    (m_tcp_flags_value == rule.m_tcp_flags_value));
}

std::string l3_rule::to_string() const
{
    std::ostringstream s;

    s << "L3-rule:["
      << "priority:" << m_priority
      << " action:"  << m_action.to_string()
      << " src:"     << m_src.to_string()
      << " dst:"     << m_dst.to_string()
      << " proto:"   << std::to_string(m_proto)
      << " srcportfrom:" << m_srcport_or_icmptype_first
      << " srcportto: " << m_srcport_or_icmptype_last
      << " dstportfrom:" << m_dstport_or_icmpcode_first
      << " dstportto:" << m_dstport_or_icmpcode_last
      << " tcpflagmask:" << m_tcp_flags_mask
      << " tcpflagvalue:" << m_tcp_flags_value
      << "]";

    return (s.str());  
}

void l3_rule::setSrcIp(route::prefix_t src)
{
    m_src = src;
}

void l3_rule::setDstIp(route::prefix_t dst)
{
    m_dst = dst;
}

void l3_rule::setProto(uint8_t proto)
{
    m_proto = proto;
}

void l3_rule::setSrcFromPort(uint16_t srcport_or_icmptype_first)
{
    m_srcport_or_icmptype_first = srcport_or_icmptype_first;
}

void l3_rule::setSrcToPort(uint16_t srcport_or_icmptype_last)
{
    m_srcport_or_icmptype_last = srcport_or_icmptype_last;
}

void l3_rule::setDstFromPort(uint16_t dstport_or_icmpcode_first)
{
    m_dstport_or_icmpcode_first = dstport_or_icmpcode_first;
}

void l3_rule::setDstToPort(uint16_t dstport_or_icmpcode_last)
{
    m_dstport_or_icmpcode_last = dstport_or_icmpcode_last;
}

void l3_rule::setTCPFlagsMask(uint8_t tcp_flags_mask)
{
    m_tcp_flags_mask = tcp_flags_mask;
}

void l3_rule::setTCPFlagsValue(uint8_t tcp_flags_value)
{
    m_tcp_flags_value = tcp_flags_value;
}
