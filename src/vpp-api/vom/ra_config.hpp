/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef __VOM_RA_CONFIG_H__
#define __VOM_RA_CONFIG_H__

#include <string>
#include <stdint.h>

#include <vapi/ip.api.vapi.hpp>

namespace VOM
{
    /**
     * A representation of Router Advertisement configuration
     */
    class ra_config
    {
    public:
        /**
         * Construct a new object matching the desried state
         */
        ra_config(uint8_t suppress, uint8_t send_unicast,
                  uint8_t default_router, uint32_t max_interval);

        /**
         * Copy Constructor
         */
        ra_config(const ra_config& o) = default;

        /**
         * Destructor
         */
        ~ra_config() = default;

        /**
         * convert to string format for debug purposes
         */
        std::string to_string() const;

        /**
         * Comparison operator - only used for UT
         */
        bool operator==(const ra_config &ra_config) const;

        /**
         * convert the ra config to VPP API
         */
        void to_vpp(vapi_payload_sw_interface_ip6nd_ra_config &ra_config) const;
    private:
        /**
         * Disables sending ICMPv6 router-advertisement messages.
         */
        uint8_t m_suppress;

        /**
         * Advertises in ICMPv6 router-advertisement messages to use
         * stateful address auto-configuration to obtain address information.
         */
        uint8_t m_managed;

        /**
         * Indicates in ICMPv6 router-advertisement messages that
         * hosts use stateful auto configuration to obtain nonaddress
         * related information.
         */
        uint8_t m_other;

        /**
         * Indicates not to include the optional source link-layer
         * address in the ICMPv6 router-advertisement messages.
         */
        uint8_t m_ll_option;

        /**
         * Use the source address of the router-solicitation message if availiable.
         */
        uint8_t m_send_unicast;

        /**
         * Cease sending ICMPv6 router-advertisement messages.
         */
        uint8_t m_cease;

        /**
         * .... ?
         */
        uint8_t m_default_router;

        /**
         * Configures the interval between sending ICMPv6 router-advertisement
         * messages. The range for max-interval is from 4 to 200 seconds.
         */
        uint32_t m_max_interval;

        /**
         * min-interval can not be more than 75% of max-interval.
         * If not set, min-interval will be set to 75% of max-interval.
         * The range for min-interval is from 3 to 150 seconds.
         */
        uint32_t m_min_interval;

        /**
         * Advertises the lifetime of a default router in ICMPv6
         * router-advertisement messages. The range is from 0 to 9000 seconds.
         * '<lifetime>' must be greater than '<max-interval>'.
         * The default value is 600 seconds
         */
        uint32_t m_lifetime;

        /**
         * Number of initial ICMPv6 router-advertisement messages sent.
         * Range for count is 1 - 3 and default is 3.
         */
        uint32_t m_initial_count;

        /**
         * The interval between each initial messages.
         * Range for interval is 1 to 16 seconds, and default is 16 seconds.
         */
        uint32_t m_initial_interval;

    };

};

#endif