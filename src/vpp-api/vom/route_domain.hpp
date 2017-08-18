/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef __VOM_ROUTE_DOMAIN_H__
#define __VOM_ROUTE_DOMAIN_H__

#include <string>
#include <stdint.h>

#include "vom/object_base.hpp"
#include "vom/om.hpp"
#include "vom/singular_db.hpp"
#include "vom/route.hpp"

namespace VOM
{
    /**
     * A base class for all object_base in the VPP object_base-Model.
     *  provides the abstract interface.
     */
    class route_domain: public object_base
    {
    public:
        /**
         * Construct a new object matching the desried state
         */
        route_domain(route::table_id_t id);
        /**
         * Copy Constructor
         */
        route_domain(const route_domain& o);
        /**
         * Destructor
         */
        ~route_domain();

        /**
         * Return the matching 'singular instance'
         */
        std::shared_ptr<route_domain> singular() const;

        /**
         * Debug print function
         */
        std::string to_string() const;

        /**
         * Get the table ID
         */
        route::table_id_t table_id() const;

        /**
         * Find the instnace of the route domain in the OM
         */
        static std::shared_ptr<route_domain> find(const route_domain &temp);

        /**
         * Dump all route-doamin into the stream provided
         */
        static void dump(std::ostream &os);

        /**
         * replay the object to create it in hardware
         */
        void replay(void);

    private:
        /**
         * Commit the acculmulated changes into VPP. i.e. to a 'HW" write.
         */
        void update(const route_domain &obj);

        /**
         * Find or add the instnace of the route domain in the OM
         */
        static std::shared_ptr<route_domain> find_or_add(const route_domain &temp);

        /*
         * It's the VPPHW class that updates the objects in HW
         */
        friend class VOM::OM;

        /**
         * It's the VOM::singular_db class that calls replay()
         */
        friend class VOM::singular_db<route::table_id_t, route_domain>;

        /**
         * Sweep/reap the object if still stale
         */
        void sweep(void);

        /**
         * VPP understands Table-IDs not table names.
         *  The table IDs for V4 and V6 are the same.
         */
        route::table_id_t m_table_id;

        /**
         * A map of all interfaces key against the interface's name
         */
        static singular_db<route::table_id_t, route_domain> m_db;
    };
};

#endif
