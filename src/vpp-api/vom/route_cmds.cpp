/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <iostream>
#include <algorithm>

#include "vom/route.hpp"

using namespace VOM;
using namespace VOM::route;

ip_route::update_cmd::update_cmd(HW::item<bool> &item,
                                 const prefix_t &prefix,
                                 table_id_t id,
                                 const path_list_t &paths):
    rpc_cmd(item),
    m_prefix(prefix),
    m_id(id),
    m_paths(paths)
{
}

bool ip_route::update_cmd::operator==(const update_cmd& other) const
{
    return ((m_prefix == other.m_prefix) &&
            (m_id == other.m_id));
}

rc_t ip_route::update_cmd::issue(connection &con)
{
    /* msg_t req(con.ctx(), std::ref(*this)); */

    /* auto &payload = req.get_request().get_payload(); */
    /* payload.table_id = m_id; */
    /* payload.is_add = 1; */
    /* payload.is_ipv6 = m_proto.is_ipv6(); */

    /* VAPI_CALL(req.execute()); */

    /* m_hw_item.set(wait()); */

    return rc_t::OK;
}

std::string ip_route::update_cmd::to_string() const
{
    std::ostringstream s;
    s << "ip-route-create: " << m_hw_item.to_string()
      << " id:" << m_id
      << " prefix:" << m_prefix.to_string()
      << " paths:" << m_paths;

    return (s.str());
}

ip_route::delete_cmd::delete_cmd(HW::item<bool> &item,
                                 const prefix_t &prefix,
                                 table_id_t id):
    rpc_cmd(item),
    m_prefix(prefix),
    m_id(id)
{
}

bool ip_route::delete_cmd::operator==(const delete_cmd& other) const
{
    return ((m_prefix == other.m_prefix) &&
            (m_id == other.m_id));
}

rc_t ip_route::delete_cmd::issue(connection &con)
{
    /* msg_t req(con.ctx(), std::ref(*this)); */

    /* auto &payload = req.get_request().get_payload(); */
    /* payload.table_id = m_id; */
    /* payload.is_add = 0; */
    /* payload.is_ipv6 = m_proto.is_ipv6(); */
    
    /* VAPI_CALL(req.execute()); */

    /* wait(); */
    /* m_hw_item.set(rc_t::NOOP); */

    return rc_t::OK;
}

std::string ip_route::delete_cmd::to_string() const
{
    std::ostringstream s;
    s << "ip-table-delete: " << m_hw_item.to_string()
      << " id:" << m_id
      << " prefix:" << m_prefix.to_string();

    return (s.str());
}
