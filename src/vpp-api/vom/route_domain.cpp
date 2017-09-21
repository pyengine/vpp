/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <cassert>
#include <iostream>

#include "vom/route_domain.hpp"
#include "vom/cmd.hpp"

using namespace VOM;

/**
 * A DB of al the interfaces, key on the name
 */
singular_db<route::table_id_t, route_domain> route_domain::m_db;

/**
 * Construct a new object matching the desried state
 */
route_domain::route_domain(l3_proto_t proto,
                           route::table_id_t id):
    m_hw(false),
    m_proto(proto),
    m_table_id(id)
{
}

route_domain::route_domain(const route_domain& o):
    m_hw(o.m_hw),
    m_proto(o.m_proto),
    m_table_id(o.m_table_id)
{
}

route::table_id_t route_domain::table_id() const
{
    return (m_table_id);
}

void route_domain::sweep()
{
    if (m_hw)
    {
        HW::enqueue(new delete_cmd(m_hw, m_proto, m_table_id));
    }
}

void route_domain::replay()
{
    if (m_hw)
    {
        HW::enqueue(new create_cmd(m_hw, m_proto, m_table_id));
    }
}

route_domain::~route_domain()
{
    sweep();

    // not in the DB anymore.
    m_db.release(m_table_id, this);
}

std::string route_domain::to_string() const
{
    std::ostringstream s;
    s << "route-domain:["
      << m_table_id
      << "]";

    return (s.str());
}

void route_domain::update(const route_domain &desired)
{
    /*
     * create the table if it is not yet created
     */
    if (rc_t::OK != m_hw.rc())
    {
        HW::enqueue(new create_cmd(m_hw, m_proto, m_table_id));
    }
}

std::shared_ptr<route_domain> route_domain::find_or_add(const route_domain &temp)
{
    return (m_db.find_or_add(temp.m_table_id, temp));
}

std::shared_ptr<route_domain> route_domain::singular() const
{
    return find_or_add(*this);
}

void route_domain::dump(std::ostream &os)
{
    m_db.dump(os);
}
