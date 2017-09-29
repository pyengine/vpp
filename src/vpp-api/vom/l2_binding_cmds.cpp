/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <iostream>

#include "vom/l2_binding.hpp"

using namespace VOM;

l2_binding::bind_cmd::bind_cmd(HW::item<bool> &item,
                               const handle_t &itf,
                               uint32_t bd,
                               bool is_bvi):
    rpc_cmd(item),
    m_itf(itf),
    m_bd(bd),
    m_is_bvi(is_bvi)
{
}

bool l2_binding::bind_cmd::operator==(const bind_cmd& other) const
{
    return ((m_itf == other.m_itf) &&
            (m_bd == other.m_bd) &&
            (m_is_bvi == other.m_is_bvi));
}

rc_t l2_binding::bind_cmd::issue(connection &con)
{
    msg_t req(con.ctx(), std::ref(*this));

    auto &payload = req.get_request().get_payload();
    payload.rx_sw_if_index = m_itf.value();
    payload.bd_id = m_bd;
    payload.shg = 0;
    payload.bvi = m_is_bvi;
    payload.enable = 1;

    VAPI_CALL(req.execute());

    m_hw_item.set(wait());
                                            
    return (rc_t::OK);
}

std::string l2_binding::bind_cmd::to_string() const
{
    std::ostringstream s;
    s << "L2-config-BD-bind: " << m_hw_item.to_string()
      << " itf:" << m_itf.to_string()
      << " bd:" << m_bd;

    return (s.str());
}

l2_binding::unbind_cmd::unbind_cmd(HW::item<bool> &item,
                                   const handle_t &itf,
                                   uint32_t bd,
                                   bool is_bvi):
    rpc_cmd(item),
    m_itf(itf),
    m_bd(bd),
    m_is_bvi(is_bvi)
{
}

bool l2_binding::unbind_cmd::operator==(const unbind_cmd& other) const
{
    return ((m_itf == other.m_itf) &&
            (m_bd == other.m_bd) &&
            (m_is_bvi == other.m_is_bvi));
}

rc_t l2_binding::unbind_cmd::issue(connection &con)
{
    msg_t req(con.ctx(), std::ref(*this));

    auto &payload = req.get_request().get_payload();
    payload.rx_sw_if_index = m_itf.value();
    payload.bd_id = m_bd;
    payload.shg = 0;
    payload.bvi = m_is_bvi;
    payload.enable = 0;

    VAPI_CALL(req.execute());

    wait();
    m_hw_item.set(rc_t::NOOP);

    return (rc_t::OK);
}

std::string l2_binding::unbind_cmd::to_string() const
{
    std::ostringstream s;
    s << "L2-config-BD-unbind: " << m_hw_item.to_string()
      << " itf:" << m_itf.to_string()
      << " bd:" << m_bd;

    return (s.str());
}