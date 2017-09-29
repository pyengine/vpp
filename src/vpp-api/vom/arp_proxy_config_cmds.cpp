/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <iostream>
#include <algorithm>

#include "vom/arp_proxy_config.hpp"

#include <vapi/vpe.api.vapi.hpp>

using namespace VOM;

arp_proxy_config::config_cmd::config_cmd(HW::item<bool> &item,
                                         const boost::asio::ip::address_v4 &low,
                                         const boost::asio::ip::address_v4 &high):
    rpc_cmd(item),
    m_low(low),
    m_high(high)
{
}

bool arp_proxy_config::config_cmd::operator==(const config_cmd& o) const
{
    return ((m_low == o.m_low) &&
            (m_high == o.m_high));
}

rc_t arp_proxy_config::config_cmd::issue(connection &con)
{
    msg_t req(con.ctx(), std::ref(*this));

    auto &payload = req.get_request().get_payload();
    payload.is_add = 1;

    std::copy_n(std::begin(m_low.to_bytes()),
                m_low.to_bytes().size(),
                payload.low_address);
    std::copy_n(std::begin(m_high.to_bytes()),
                m_high.to_bytes().size(),
                payload.hi_address);

    VAPI_CALL(req.execute());

    m_hw_item.set(wait());

    return (rc_t::OK);
}

std::string arp_proxy_config::config_cmd::to_string() const
{
    std::ostringstream s;
    s << "ARP-proxy-config: " << m_hw_item.to_string()
      << " low:" << m_low.to_string()
      << " high:" << m_high.to_string();

    return (s.str());
}

arp_proxy_config::unconfig_cmd::unconfig_cmd(HW::item<bool> &item,
                                             const boost::asio::ip::address_v4 &low,
                                             const boost::asio::ip::address_v4 &high):
    rpc_cmd(item),
    m_low(low),
    m_high(high)
{
}

bool arp_proxy_config::unconfig_cmd::operator==(const unconfig_cmd& o) const
{
    return ((m_low == o.m_low) &&
            (m_high == o.m_high));
}

rc_t arp_proxy_config::unconfig_cmd::issue(connection &con)
{
    msg_t req(con.ctx(), std::ref(*this));

    auto &payload = req.get_request().get_payload();
    payload.is_add = 0;

    std::copy_n(std::begin(m_low.to_bytes()),
                m_low.to_bytes().size(),
                payload.low_address);
    std::copy_n(std::begin(m_high.to_bytes()),
                m_high.to_bytes().size(),
                payload.hi_address);

    VAPI_CALL(req.execute());

    wait();
    m_hw_item.set(rc_t::NOOP);

    return (rc_t::OK);
}

std::string arp_proxy_config::unconfig_cmd::to_string() const
{
    std::ostringstream s;
    s << "ARP-proxy-unconfig: " << m_hw_item.to_string()
      << " low:" << m_low.to_string()
      << " high:" << m_high.to_string();

    return (s.str());
}