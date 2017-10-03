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
#include "vom/singular_db.hpp"

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
            void to_vpp(vapi_payload_ip_add_del_route &payload) const;

            /**
             * Less than operator for set insertion
             */
            bool operator<(const path &p) const;

            /**
             * convert to string format for debug purposes
             */
            std::string to_string() const;

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
         * A path-list is a set of paths
         */
        typedef std::set<path> path_list_t;

        /**
         * ostream output for iterator
         */
        std::ostream & operator<<(std::ostream &os,
                                  const path_list_t &path_list);

        /**
         * A IP route
         */
        class ip_route: public object_base
        {
        public:
            /**
             * The key for a route
             */
            typedef std::pair<route::table_id_t, prefix_t> key_t;

            /**
             * Construct a route in the default table
             */
            ip_route(const prefix_t &prefix);

            /**
             * Copy Construct
             */
            ip_route(const ip_route &r);

            /**
             * Construct a route in the given route domain
             */
            ip_route(const prefix_t &prefix,
                     std::shared_ptr<route_domain> rd);

            /**
             * Destructor
             */
            ~ip_route();

            /**
             * Return the matching 'singular instance'
             */
            std::shared_ptr<ip_route> singular() const;

            /**
             * Add a path.
             */
            void add(const path &path);

            /**
             * remove a path.
             */
            void remove(const path &path);

            /**
             * Find the instnace of the route domain in the OM
             */
            static std::shared_ptr<ip_route> find(const ip_route &temp);

            /**
             * Dump all route-doamin into the stream provided
             */
            static void dump(std::ostream &os);

            /**
             * replay the object to create it in hardware
             */
            void replay(void);

            /**
             * Convert to string for debugging
             */
            std::string to_string() const;

            /**
             * A command class that creates or updates the route
             */
            class update_cmd: public rpc_cmd<HW::item<bool>, rc_t,
                                             vapi::Ip_add_del_route>
            {
            public:
                /**
                 * Constructor
                 */
                update_cmd(HW::item<bool> &item,
                           const prefix_t &prefix,
                           table_id_t id,
                           const path_list_t &paths);

                /**
                 * Issue the command to VPP/HW
                 */
                rc_t issue(connection &con);

                /**
                 * convert to string format for debug purposes
                 */
                std::string to_string() const;

                /**
                 * Comparison operator - only used for UT
                 */
                bool operator==(const update_cmd&i) const;

            private:
                prefix_t m_prefix;
                route::table_id_t m_id;
                const path_list_t &m_paths;
            };

            /**
             * A cmd class that deletes a route
             */
            class delete_cmd: public rpc_cmd<HW::item<bool>, rc_t,
                                             vapi::Ip_add_del_route>
            {
            public:
                /**
                 * Constructor
                 */
                delete_cmd(HW::item<bool> &item,
                           const prefix_t &prefix,
                           table_id_t id);

                /**
                 * Issue the command to VPP/HW
                 */
                rc_t issue(connection &con);

                /**
                 * convert to string format for debug purposes
                 */
                std::string to_string() const;

                /**
                 * Comparison operator - only used for UT
                 */
                bool operator==(const delete_cmd&i) const;

            private:
                prefix_t m_prefix;
                route::table_id_t m_id;
            };

        private:
            /**
             * Commit the acculmulated changes into VPP. i.e. to a 'HW" write.
             */
            void update(const ip_route &obj);

            /**
             * Find or add the instnace of the route domain in the OM
             */
            static std::shared_ptr<ip_route> find_or_add(const ip_route &temp);

            /*
             * It's the VPPHW class that updates the objects in HW
             */
            friend class VOM::OM;

            /**
             * It's the VOM::singular_db class that calls replay()
             */
            friend class VOM::singular_db<key_t, ip_route>;

            /**
             * Sweep/reap the object if still stale
             */
            void sweep(void);

            /**
             * HW configuration for the result of creating the route
             */
            HW::item<bool> m_hw;

            /**
             * The prefix to match
             */
            prefix_t m_prefix;

            /**
             * The route domain the route is in.
             */
            std::shared_ptr<route_domain> m_rd;

            /**
             * The set of paths
             */
            path_list_t m_paths;

            /**
             * A map of all routes
             */
            static singular_db<key_t, ip_route> m_db;
        };

        std::ostream & operator<<(std::ostream &os,
                                  const ip_route::key_t &key);

    };
};

#endif
