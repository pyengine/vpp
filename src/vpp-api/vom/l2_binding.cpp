/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <cassert>
#include <iostream>

#include "vom/l2_binding.hpp"
#include "vom/cmd.hpp"

using namespace VPP;

/**
 * A DB of all the L2 Configs
 */
singular_db<const handle_t, l2_binding> l2_binding::m_db;

l2_binding::event_handler l2_binding::m_evh;

/**
 * Construct a new object matching the desried state
 */
l2_binding::l2_binding(const interface &itf,
                       const bridge_domain &bd):
    m_itf(itf.singular()),
    m_bd(bd.singular()),
    m_binding(0)
{
}

l2_binding::l2_binding(const l2_binding& o):
    m_itf(o.m_itf),
    m_bd(o.m_bd),
    m_binding(0)
{
}

void l2_binding::sweep()
{
    if (m_binding && handle_t::INVALID != m_itf->handle())
    {
        HW::enqueue(new unbind_cmd(m_binding,
                                   m_itf->handle(),
                                   m_bd->id(),
                                   interface::type_t::BVI == m_itf->type()));
    }
    HW::write();
}

void l2_binding::replay()
{
    if (m_binding && handle_t::INVALID != m_itf->handle())
    {
        HW::enqueue(new bind_cmd(m_binding,
                                 m_itf->handle(),
                                 m_bd->id(),
                                 interface::type_t::BVI == m_itf->type()));
    }
}

l2_binding::~l2_binding()
{
    sweep();

    // not in the DB anymore.
    m_db.release(m_itf->handle(), this);
}

std::string l2_binding::to_string() const
{
    std::ostringstream s;
    s << "L2-config:[" << m_itf->to_string()
      << " " << m_bd->to_string()
      << " " << m_binding.to_string()
      << "]";

    return (s.str());
}

void l2_binding::update(const l2_binding &desired)
{
    /*
     * the desired state is always that the interface should be created
     */
    if (rc_t::OK != m_binding.rc())
    {
        HW::enqueue(new bind_cmd(m_binding,
                                 m_itf->handle(),
                                 m_bd->id(),
                                 interface::type_t::BVI == m_itf->type()));
    }
}

std::shared_ptr<l2_binding> l2_binding::find_or_add(const l2_binding &temp)
{
    return (m_db.find_or_add(temp.m_itf->handle(), temp));
}

std::shared_ptr<l2_binding> l2_binding::singular() const
{
    return find_or_add(*this);
}

void l2_binding::dump(std::ostream &os)
{
    m_db.dump(os);
}

l2_binding::event_handler::event_handler()
{
    OM::register_listener(this);
    inspect::register_handler({"l2"}, "L2 bindings", this);
}

void l2_binding::event_handler::handle_replay()
{
    m_db.replay();
}

void l2_binding::event_handler::handle_populate(const client_db::key_t &key)
{
    /**
     * This is done while populating the bridge-domain
     */
}

dependency_t l2_binding::event_handler::order() const
{
    return (dependency_t::BINDING);
}

void l2_binding::event_handler::show(std::ostream &os)
{
    m_db.dump(os);
}
