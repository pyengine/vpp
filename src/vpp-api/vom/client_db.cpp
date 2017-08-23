/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include "vom/client_db.hpp"

using namespace VPP;

object_ref_list& client_db::find(const client_db::key_t &k)
{
    return (m_objs[k]);
}

void client_db::flush(const client_db::key_t &k)
{
    m_objs.erase(m_objs.find(k));
}
