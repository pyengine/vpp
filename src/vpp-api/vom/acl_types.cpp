/* -*- C++ -*-; c-basic-offset: 4; indent-tabs-mode: nil */
/*
 * Main implementation for OVS agent
 *
 * Copyright (c) 2014 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include "vom/acl_types.hpp"

using namespace VOM::ACL;

const action_t action_t::PERMITANDREFLEX(2, "permitandreflex");
const action_t action_t::PERMIT(1, "permit");
const action_t action_t::DENY(0, "deny");

action_t::action_t(int v, const std::string s):
    enum_base(v, s)
{
}

const action_t &action_t::from_int(uint8_t i)
{
    if (i == 2)
        return action_t::PERMITANDREFLEX;
    else if (i)
	return action_t::PERMIT;

    return action_t::DENY;
}

const action_t &action_t::from_bool(bool b, uint8_t c)
{
    if (b)
    {
	if (c)
	    return action_t::PERMITANDREFLEX;
        return action_t::PERMIT;
    }
    return action_t::DENY;
}
