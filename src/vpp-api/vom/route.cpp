/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include "vom/route.hpp"
#include "vom/singular_db.hpp"

using namespace VOM::route;

VOM::singular_db<ip_route::key_t, ip_route> ip_route::m_db;

const path::special_t path::special_t::STANDARD(0, "standard");
const path::special_t path::special_t::LOCAL(0, "local");
const path::special_t path::special_t::DROP(0, "standard");
const path::special_t path::special_t::UNREACH(0, "unreachable");
const path::special_t path::special_t::PROHIBIT(0, "prohibit");

path::special_t::special_t(int v,
                           const std::string &s):
    enum_base<path::special_t>(v, s)
{
}

path::path(special_t special):
    m_type(special),
    m_nh_proto(nh_proto_t::IPV4),
    m_nh(),
    m_rd(nullptr),
    m_interface(nullptr),
    m_weight(1),
    m_preference(0)
{
}

path::path(const boost::asio::ip::address &nh,
           const interface &interface,
           uint8_t weight,
           uint8_t preference):
    m_type(special_t::STANDARD),
    m_nh_proto(nh_proto_t::from_address(nh)),
    m_nh(nh),
    m_rd(nullptr),
    m_interface(interface.singular()),
    m_weight(weight),
    m_preference(preference)
{
}

path::path(const route_domain &rd,
           const boost::asio::ip::address &nh,
           uint8_t weight,
           uint8_t preference):
    m_type(special_t::STANDARD),
    m_nh_proto(nh_proto_t::from_address(nh)),
    m_nh(nh),
    m_rd(rd.singular()),
    m_interface(nullptr),
    m_weight(weight),
    m_preference(preference)
{
}

path::path(const interface &interface,
           const nh_proto_t &proto,
           uint8_t weight,
           uint8_t preference):
    m_type(special_t::STANDARD),
    m_nh_proto(proto),
    m_nh(),
    m_rd(nullptr),
    m_interface(interface.singular()),
    m_weight(weight),
    m_preference(preference)
{
}

path::path(const path &p):
    m_type(p.m_type),
    m_nh_proto(p.m_nh_proto),
    m_nh(p.m_nh),
    m_rd(p.m_rd),
    m_interface(p.m_interface),
    m_weight(p.m_weight),
    m_preference(p.m_preference)
{
}


bool path::operator<(const path &p) const
{
    if (m_type < p.m_type) return true;
    if (m_rd->table_id() < p.m_rd->table_id()) return true;
    if (m_nh < p.m_nh) return true;
    if (m_interface->handle() < p.m_interface->handle()) return true;

    return (false);
}

void path::to_vpp(vapi_payload_ip_add_del_route &payload) const
{
    payload.is_drop = 0;
    payload.is_unreach = 0;
    payload.is_prohibit = 0;
    payload.is_local = 0;
    payload.is_classify = 0;
    payload.is_multipath = 0;
    payload.is_resolve_host = 0;
    payload.is_resolve_attached = 0;

    if (nh_proto_t::ETHERNET == m_nh_proto)
    {
        payload.is_l2_bridged = 1;
    }

    if (special_t::STANDARD == m_type)
    {
        uint8_t path_v6;
        to_bytes(m_nh, &path_v6, payload.next_hop_address);

        if (m_rd)
        {
            payload.next_hop_table_id = m_rd->table_id();
        }
        if (m_interface)
        {
            payload.next_hop_sw_if_index = m_interface->handle().value();
        }
    }
    else if (special_t::DROP == m_type)
    {
        payload.is_drop = 1;
    }
    else if (special_t::UNREACH == m_type)
    {
        payload.is_unreach = 1;
    }
    else if (special_t::PROHIBIT == m_type)
    {
        payload.is_prohibit = 1;
    }
    else if (special_t::LOCAL == m_type)
    {
        payload.is_local = 1;
    }
    payload.next_hop_weight = m_weight;
    payload.next_hop_preference = m_preference;
    payload.next_hop_via_label = 0;
    payload.classify_table_index = 0;
}

std::string path::to_string() const
{
    std::ostringstream s;

    s << "path:["
      << "type:" << m_type.to_string()
      << " proto:" << m_nh_proto.to_string()
      << " neighbour:" << m_nh.to_string();
    if (m_rd)
    {
        s << " " << m_rd->to_string();
    }
    if (m_interface)
    {
        s << " " << m_interface->to_string();
    }
    s << " weight:" << static_cast<int>(m_weight)
      << " preference:" << static_cast<int>(m_preference)
      << "]";

    return (s.str());
}

ip_route::ip_route(const prefix_t &prefix):
    m_hw(false),
    m_rd(nullptr),
    m_prefix(prefix),
    m_paths()
{
    /*
     * the route goes in the default table
     */
    route_domain rd(DEFAULT_TABLE);

    m_rd = rd.singular();
}

ip_route::ip_route(const ip_route &r):
    m_hw(r.m_hw),
    m_rd(r.m_rd),
    m_prefix(r.m_prefix),
    m_paths(r.m_paths)
{
}

ip_route::ip_route(const route_domain &rd,
                   const prefix_t &prefix):
    m_hw(false),
    m_rd(rd.singular()),
    m_prefix(prefix),
    m_paths()
{
}

ip_route::~ip_route()
{
    sweep();

    // not in the DB anymore.
    m_db.release(std::make_pair(m_rd->table_id(), m_prefix), this);
}

void ip_route::add(const path &path)
{
    m_paths.insert(path);
}

void ip_route::remove(const path &path)
{
    m_paths.erase(path);
}

void ip_route::sweep()
{
    if (m_hw)
    {
        HW::enqueue(new delete_cmd(m_hw, m_rd->table_id(), m_prefix));
    }
    HW::write();
}

void ip_route::replay()
{
    if (m_hw)
    {
        HW::enqueue(new update_cmd(m_hw, m_rd->table_id(), m_prefix, m_paths));
    }
}
std::string ip_route::to_string() const
{
    std::ostringstream s;
    s << "route:["
      << m_rd->to_string()
      << ", "
      << m_prefix.to_string()
      << "]";

    return (s.str());
}

void ip_route::update(const ip_route &r)
{
    /*
     * create the table if it is not yet created
     */
    if (rc_t::OK != m_hw.rc())
    {
        HW::enqueue(new update_cmd(m_hw, m_rd->table_id(), m_prefix, m_paths));
    }
}

std::shared_ptr<ip_route> ip_route::find_or_add(const ip_route &temp)
{
    return (m_db.find_or_add(std::make_pair(temp.m_rd->table_id(),
                                            temp.m_prefix),
                             temp));
}

std::shared_ptr<ip_route> ip_route::singular() const
{
    return find_or_add(*this);
}

void ip_route::dump(std::ostream &os)
{
    m_db.dump(os);
}

std::ostream& VOM::route::operator<<(std::ostream &os,
                                     const ip_route::key_t &key)
{
    os << "["
       << key.first
       << ", "
       << key.second.to_string()
       << "]";

    return (os);
}

std::ostream& VOM::route::operator<<(std::ostream &os,
                                     const path_list_t &key)
{
    os << "[";
    for (auto k : key)
    {
        os << k.to_string()
           << " ";
    }
    os << "]";

    return (os);
}
