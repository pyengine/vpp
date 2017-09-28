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
    m_nh(),
    m_rd(nullptr),
    m_interface(nullptr),
    m_weight(1),
    m_preference(0)
{
}

path::path(const boost::asio::ip::address &nh,
                  std::shared_ptr<interface> interface,
                  uint8_t weight,
                  uint8_t preference):
    m_type(special_t::STANDARD),
    m_nh(nh),
    m_rd(nullptr),
    m_interface(interface),
    m_weight(weight),
    m_preference(preference)
{
}

path::path(const boost::asio::ip::address &nh,
                  std::shared_ptr<route_domain> rd,
                  uint8_t weight,
                  uint8_t preference):
    m_type(special_t::STANDARD),
    m_nh(nh),
    m_rd(rd),
    m_interface(nullptr),
    m_weight(weight),
    m_preference(preference)
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

void path::to_vpp(vapi_type_fib_path &path) const
{
    if (special_t::STANDARD == m_type)
    {
        to_bytes(m_nh, &path.afi, path.next_hop);

        if (m_rd)
        {
            // FIXME - VPP needs next-hop table in prog API
        }
        if (m_interface)
        {
            path.sw_if_index = m_interface->handle().value();
        }
    }
    else if (special_t::DROP == m_type)
    {
        path.is_drop = 1;
    }
    else if (special_t::UNREACH == m_type)
    {
        path.is_unreach = 1;
    }
    else if (special_t::PROHIBIT == m_type)
    {
        path.is_prohibit = 1;
    }
    else if (special_t::LOCAL == m_type)
    {
        path.is_local = 1;
    }
}

std::string path::to_string() const
{
    std::ostringstream s;

    s << "path:["
      << "type:" << m_type.to_string()
      << "neighbour:" << m_nh.to_string()
      << "route_domain:" << m_rd->to_string()
      << "interface:" << m_interface->to_string()
      << "weight:" << m_weight
      << "preference:" << m_preference
      << "]";

    return (s.str());
}

ip_route::ip_route(const prefix_t &prefix):
    m_hw(false),
    m_prefix(prefix),
    m_rd(nullptr)
{
    /*
     * the route goes in the default table
     */
    route_domain rd(prefix.l3_proto(),
                    DEFAULT_TABLE);

    m_rd = rd.singular();
}

ip_route::ip_route(const ip_route &r):
    m_hw(r.m_hw),
    m_prefix(r.m_prefix),
    m_rd(r.m_rd)
{
}

ip_route::ip_route(const prefix_t &prefix,
                   std::shared_ptr<route_domain> rd):
    m_hw(false),
    m_prefix(prefix),
    m_rd(rd)
{
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
        //   HW::enqueue(new delete_cmd(m_hw, m_proto, m_table_id));
    }
}

void ip_route::replay()
{
    if (m_hw)
    {
        //  HW::enqueue(new create_cmd(m_hw, m_proto, m_table_id));
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
        // HW::enqueue(new create_cmd(m_hw, m_proto, m_table_id));
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
