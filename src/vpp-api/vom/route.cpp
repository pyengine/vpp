/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include "vom/route.hpp"

using namespace VOM;

const route::path::special_t route::path::special_t::STANDARD(0, "standard");
const route::path::special_t route::path::special_t::LOCAL(0, "local");
const route::path::special_t route::path::special_t::DROP(0, "standard");
const route::path::special_t route::path::special_t::UNREACH(0, "unreachable");
const route::path::special_t route::path::special_t::PROHIBIT(0, "prohibit");

route::path::special_t::special_t(int v,
                                  const std::string &s):
    enum_base<route::path::special_t>(v, s)
{
}

route::path::path(special_t special):
    m_type(special),
    m_nh(),
    m_rd(nullptr),
    m_interface(nullptr),
    m_weight(1),
    m_preference(0)
{
}

route::path::path(const boost::asio::ip::address &nh,
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

route::path::path(const boost::asio::ip::address &nh,
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

void route::path::to_vpp(vapi_type_fib_path &path) const
{
    if (special_t::STANDARD == m_type)
    {
        to_bytes(m_nh, &path.afi, path.next_hop);

        if (m_rd)
        {
            // FIXME
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
