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
route_domain::route_domain(route::table_id_t id):
    m_table_id(id)
{
}

route_domain::route_domain(const route_domain& o):
    m_table_id(o.m_table_id)
{
}

route::table_id_t route_domain::table_id() const
{
    return (m_table_id);
}

void route_domain::sweep()
{
}

void route_domain::replay()
{
}

route_domain::~route_domain()
{
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
     * No HW configuration associated with a route Domain
     */
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
