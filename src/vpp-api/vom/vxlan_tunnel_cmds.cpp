/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <typeinfo>
#include <cassert>
#include <iostream>

#include "vom/vxlan_tunnel.hpp"

DEFINE_VAPI_MSG_IDS_VXLAN_API_JSON;

using namespace VOM;

vxlan_tunnel::create_cmd::create_cmd(HW::item<handle_t> &item,
                                     const std::string &name,
                                     const endpoint_t &ep):
    interface::create_cmd<vapi::Vxlan_add_del_tunnel>(item, name),
    m_ep(ep)
{
}

bool vxlan_tunnel::create_cmd::operator==(const create_cmd& other) const
{
    return (m_ep == other.m_ep);
}

rc_t vxlan_tunnel::create_cmd::issue(connection &con)
{
    msg_t req(con.ctx(), std::ref(*this));

    auto &payload = req.get_request().get_payload();
    payload.is_add = 1;
    payload.is_ipv6 = 0;
    to_bytes(m_ep.src, &payload.is_ipv6, payload.src_address);
    to_bytes(m_ep.dst, &payload.is_ipv6, payload.dst_address);
    payload.mcast_sw_if_index = ~0;
    payload.encap_vrf_id = 0;
    payload.decap_next_index = ~0;
    payload.vni = m_ep.vni;

    VAPI_CALL(req.execute());

    m_hw_item = wait();

    if (m_hw_item)
    {
        interface::add(m_name, m_hw_item);
    }

    return rc_t::OK;
}


std::string vxlan_tunnel::create_cmd::to_string() const
{
    std::ostringstream s;
    s << "vxlan-tunnel-create: " << m_hw_item.to_string()
      << m_ep.to_string();

    return (s.str());
}

vxlan_tunnel::delete_cmd::delete_cmd(HW::item<handle_t> &item,
                                     const endpoint_t &ep):
    interface::delete_cmd<vapi::Vxlan_add_del_tunnel>(item),
    m_ep(ep)
{
}

bool vxlan_tunnel::delete_cmd::operator==(const delete_cmd& other) const
{
    return (m_ep == other.m_ep);
}

rc_t vxlan_tunnel::delete_cmd::issue(connection &con)
{
    msg_t req(con.ctx(), std::ref(*this));

    auto payload = req.get_request().get_payload();
    payload.is_add = 0;
    payload.is_ipv6 = 0;
    to_bytes(m_ep.src, &payload.is_ipv6, payload.src_address);
    to_bytes(m_ep.dst, &payload.is_ipv6, payload.dst_address);
    payload.mcast_sw_if_index = ~0;
    payload.encap_vrf_id = 0;
    payload.decap_next_index = ~0;
    payload.vni = m_ep.vni;

    VAPI_CALL(req.execute());

    wait();
    m_hw_item.set(rc_t::NOOP);

    interface::remove(m_hw_item);
    return (rc_t::OK);
}

std::string vxlan_tunnel::delete_cmd::to_string() const
{
    std::ostringstream s;
    s << "vxlan-tunnel-delete: " << m_hw_item.to_string()
      << m_ep.to_string();

    return (s.str());
}

vxlan_tunnel::dump_cmd::dump_cmd()
{
}

bool vxlan_tunnel::dump_cmd::operator==(const dump_cmd& other) const
{
    return (true);
}

rc_t vxlan_tunnel::dump_cmd::issue(connection &con)
{
    m_dump.reset(new msg_t(con.ctx(), std::ref(*this)));

    auto &payload = m_dump->get_request().get_payload();
    payload.sw_if_index = ~0;

    VAPI_CALL(m_dump->execute());

    wait();

    return rc_t::OK;
}

std::string vxlan_tunnel::dump_cmd::to_string() const
{
    return ("Vpp-vxlan_tunnels-Dump");
}