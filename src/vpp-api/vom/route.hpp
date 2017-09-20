/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef __VOM_ROUTE_H__
#define __VOM_ROUTE_H__

#include "vom/prefix.hpp"
#include "vom/route_domain.hpp"
#include "vom/interface.hpp"

#include <vapi/ip.api.vapi.hpp>

namespace VOM
{
    /**
     * Types belonging to Routing
     */
    namespace route
    {
        /**
         * A path for IP or MPLS routes
         */
        class path
        {
        public:
            /**
             * Special path types
             */
            class special_t: public enum_base<special_t>
            {
            public:
                /**
                 * A standard path type. this includes path types
                 * that use the next-hop and interface
                 */
                const static special_t STANDARD;

                /**
                 * A local/for-us/recieve
                 */
                const static special_t LOCAL;

                /**
                 * drop path
                 */
                const static special_t DROP;

                /**
                 * a path will return ICMP unreachables
                 */
                const static special_t UNREACH;

                /**
                 * a path will return ICMP prohibit
                 */
                const static special_t PROHIBIT;

            private:
                /**
                 * Private constructor taking the value and the string name
                 */
                special_t(int v, const std::string &s);
            };

            /**
             * constructor for special paths
             */
            path(special_t special);

            /**
             * Constructor for standard non-recursive paths
             */
            path(const boost::asio::ip::address &nh,
                 std::shared_ptr<interface> interface,
                 uint8_t weight = 1,
                 uint8_t preference = 0);

            /**
             * Constructor for standard recursive paths
             */
            path(const boost::asio::ip::address &nh,
                 std::shared_ptr<route_domain> rd,
                 uint8_t weight = 1,
                 uint8_t preference = 0);

            /**
             * Convert the path into the VPP API representation
             */
            void to_vpp(vapi_type_fib_path &path) const;
        private:
            /**
             * The special path tpye
             */
            special_t m_type;

            /**
             * The next-hop
             */
            boost::asio::ip::address m_nh;

            /**
             * For recursive routes, this is the table in which the
             * the next-hop exists.
             */
            std::shared_ptr<route_domain> m_rd;

            /**
             * The next-hop interface [if present].
             */
            std::shared_ptr<interface> m_interface;

            /**
             * UCMP weight
             */
            uint8_t m_weight;

            /**
             * Path preference
             */
            uint8_t m_preference;
        };

        /**
         * A IP route
         */
        class ip_route
        {
            /**
             * Construct a route in the default table
             */
            ip_route(const prefix_t &prefix);

            /**
             * Construct a route in the given route domain
             */
            ip_route(const prefix_t &prefix,
                     std::shared_ptr<route_domain> rd);

            /**
             * Add a path.
             */
            void add(const path &path);

            /**
             * remove a path.
             */
            void remove(const path &path);

        private:
            /**
             * The route domain the route is in.
             */
            std::shared_ptr<route_domain> m_rd;

            /**
             * The prefix to match
             */
            prefix_t m_prefix;

            /**
             * The set of paths
             */
            std::set<path> m_paths;
        };
    };
};

#endif
