/*
 * Test suite for class VppOM
 *
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
#define BOOST_TEST_MODULE "VPP OBJECT MODEL"
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_inserter.hpp>


#include <iostream>
#include <deque>

#include "vom/om.hpp"
#include "vom/interface.hpp"
#include "vom/l2_binding.hpp"
#include "vom/l3_binding.hpp"
#include "vom/bridge_domain.hpp"
#include "vom/bridge_domain_entry.hpp"
#include "vom/prefix.hpp"
#include "vom/route.hpp"
#include "vom/route_domain.hpp"
#include "vom/vxlan_tunnel.hpp"
#include "vom/sub_interface.hpp"
#include "vom/acl_list.hpp"
#include "vom/acl_binding.hpp"
#include "vom/acl_l3_rule.hpp"
#include "vom/acl_l2_rule.hpp"
#include "vom/arp_proxy_config.hpp"
#include "vom/arp_proxy_binding.hpp"
#include "vom/ip_unnumbered.hpp"
#include "vom/interface_ip6_nd.hpp"
#include "vom/interface_span.hpp"

using namespace boost;
using namespace VOM;

/**
 * An expectation exception
 */
class ExpException
{
public:
    ExpException(unsigned int number)
    {
        // a neat place to add a break point
        std::cout << "  ExpException here: " << number << std::endl;
    }
};

class MockCmdQ : public HW::cmd_q
{
public:
    MockCmdQ():
        m_strict_order(true)
    {
    }
    virtual ~MockCmdQ()
    {
    }
    void expect(cmd *f)
    {
        m_exp_queue.push_back(f);
    }
    void enqueue(cmd *f)
    {
        m_act_queue.push_back(f);
    }
    void enqueue(std::queue<cmd*> &cmds)
    {
        while (cmds.size())
        {
            m_act_queue.push_back(cmds.front());
            cmds.pop();
        }
    }

    void strict_order(bool on)
    {
        m_strict_order = on;
    }

    bool is_empty()
    {
        return ((0 == m_exp_queue.size()) &&
                (0 == m_act_queue.size()));
    }

    rc_t write()
    {
        cmd *f_exp, *f_act;
        rc_t rc = rc_t::OK;

        while (m_act_queue.size())
        {
            bool matched = false;
            auto it_exp = m_exp_queue.begin();
            auto it_act = m_act_queue.begin();

            f_act = *it_act;

            std::cout << " Act: " << f_act->to_string() << std::endl;
            while (it_exp != m_exp_queue.end())
            {
                f_exp = *it_exp;
                try
                {
                    std::cout << "  Exp: " << f_exp->to_string() << std::endl;

                    if (typeid(*f_exp) != typeid(*f_act))
                    {
                        throw ExpException(1);
                    }

                    if (typeid(*f_exp) == typeid(interface::af_packet_create_cmd))
                    {
                        rc = handle_derived<interface::af_packet_create_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(interface::loopback_create_cmd))
                    {
                        rc = handle_derived<interface::loopback_create_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(interface::loopback_delete_cmd))
                    {
                        rc = handle_derived<interface::loopback_delete_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(interface::af_packet_delete_cmd))
                    {
                        rc = handle_derived<interface::af_packet_delete_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(interface::state_change_cmd))
                    {
                        rc = handle_derived<interface::state_change_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(interface::set_table_cmd))
                    {
                        rc = handle_derived<interface::set_table_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(interface::set_mac_cmd))
                    {
                        rc = handle_derived<interface::set_mac_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(interface::set_tag))
                    {
                        rc = handle_derived<interface::set_tag>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(route_domain::create_cmd))
                    {
			rc = handle_derived<route_domain::create_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(route_domain::delete_cmd))
                    {
                        rc = handle_derived<route_domain::delete_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(l3_binding::bind_cmd))
                    {
                        rc = handle_derived<l3_binding::bind_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(l3_binding::unbind_cmd))
                    {
                        rc = handle_derived<l3_binding::unbind_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(bridge_domain::create_cmd))
                    {
                        rc = handle_derived<bridge_domain::create_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(bridge_domain::delete_cmd))
                    {
                        rc = handle_derived<bridge_domain::delete_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(bridge_domain_entry::create_cmd))
                    {
                        rc = handle_derived<bridge_domain_entry::create_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(bridge_domain_entry::delete_cmd))
                    {
                        rc = handle_derived<bridge_domain_entry::delete_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(l2_binding::bind_cmd))
                    {
                        rc = handle_derived<l2_binding::bind_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(l2_binding::unbind_cmd))
                    {
                        rc = handle_derived<l2_binding::unbind_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(l2_binding::set_vtr_op_cmd))
                    {
                        rc = handle_derived<l2_binding::set_vtr_op_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(vxlan_tunnel::create_cmd))
                    {
                        rc = handle_derived<vxlan_tunnel::create_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(vxlan_tunnel::delete_cmd))
                    {
                        rc = handle_derived<vxlan_tunnel::delete_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(sub_interface::create_cmd))
                    {
                        rc = handle_derived<sub_interface::create_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(sub_interface::delete_cmd))
                    {
                        rc = handle_derived<sub_interface::delete_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ACL::l3_list::update_cmd))
                    {
                        rc = handle_derived<ACL::l3_list::update_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ACL::l3_list::delete_cmd))
                    {
                        rc = handle_derived<ACL::l3_list::delete_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ACL::l3_binding::bind_cmd))
                    {
                        rc = handle_derived<ACL::l3_binding::bind_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ACL::l3_binding::unbind_cmd))
                    {
                        rc = handle_derived<ACL::l3_binding::unbind_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ACL::l2_list::update_cmd))
                    {
                        rc = handle_derived<ACL::l2_list::update_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ACL::l2_list::delete_cmd))
                    {
                        rc = handle_derived<ACL::l2_list::delete_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ACL::l2_binding::bind_cmd))
                    {
                        rc = handle_derived<ACL::l2_binding::bind_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ACL::l2_binding::unbind_cmd))
                    {
                        rc = handle_derived<ACL::l2_binding::unbind_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(arp_proxy_binding::bind_cmd))
                    {
                        rc = handle_derived<arp_proxy_binding::bind_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(arp_proxy_binding::unbind_cmd))
                    {
                        rc = handle_derived<arp_proxy_binding::unbind_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(arp_proxy_config::config_cmd))
                    {
                        rc = handle_derived<arp_proxy_config::config_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(arp_proxy_config::unconfig_cmd))
                    {
                        rc = handle_derived<arp_proxy_config::unconfig_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ip_unnumbered::config_cmd))
                    {
                        rc = handle_derived<ip_unnumbered::config_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ip_unnumbered::unconfig_cmd))
                    {
                        rc = handle_derived<ip_unnumbered::unconfig_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ip6nd_ra_config::config_cmd))
                    {
                        rc = handle_derived<ip6nd_ra_config::config_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ip6nd_ra_config::unconfig_cmd))
                    {
                        rc = handle_derived<ip6nd_ra_config::unconfig_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ip6nd_ra_prefix::config_cmd))
                    {
                        rc = handle_derived<ip6nd_ra_prefix::config_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(ip6nd_ra_prefix::unconfig_cmd))
                    {
                        rc = handle_derived<ip6nd_ra_prefix::unconfig_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(interface_span::config_cmd))
                    {
                        rc = handle_derived<interface_span::config_cmd>(f_exp, f_act);
                    }
                    else if (typeid(*f_exp) == typeid(interface_span::unconfig_cmd))
                    {
                        rc = handle_derived<interface_span::unconfig_cmd>(f_exp, f_act);
                    }
                    else
                    {
                        throw ExpException(2);
                    }

                    // if we get here then we found the match.
                    m_exp_queue.erase(it_exp);
                    m_act_queue.erase(it_act);
                    delete f_exp;
                    delete f_act;

                    // return any injected failures to the agent
                    if (rc_t::OK != rc && rc_t::NOOP != rc)
                    {
                        return (rc);
                    }

                    matched = true;
                    break;
                }
                catch (ExpException &e)
                {
                    // The expected and actual do not match
                    if (m_strict_order)
                    {
                        // in strict ordering mode this is fatal, so rethrow
                        throw e;
                    }
                    else
                    {
                        // move the interator onto the next in the expected list and
                        // check for a match
                        ++it_exp;
                    }
                }
            }

            if (!matched)
                throw ExpException(3);
        }

        return (rc);
    }
private:

    template <typename T>
    rc_t handle_derived(const cmd *f_exp, cmd *f_act)
    {
        const T *i_exp;
        T *i_act;

        i_exp = dynamic_cast<const T*>(f_exp);
        i_act = dynamic_cast<T*>(f_act);
        if (!(*i_exp == *i_act))
        {
            throw ExpException(4);
        }
        // pass the data and return code to the agent
        i_act->item() = i_exp->item();

        return (i_act->item().rc());
    }

    // The Q to push the expectations on
    std::deque<cmd*> m_exp_queue;

    // the queue to push the actual events on
    std::deque<cmd*> m_act_queue;

    // control whether the expected queue is strictly ordered.
    bool m_strict_order;
};

class VppInit {
public:
    std::string name;
    MockCmdQ *f;

    VppInit()
        : name("vpp-ut"),
          f(new MockCmdQ())
    {
        HW::init(f);
        OM::init();
    }

    ~VppInit() {
        delete f;
    }
};

BOOST_AUTO_TEST_SUITE(VppOM_test)

#define TRY_CHECK_RC(stmt)                    \
{                                             \
    try {                                     \
        BOOST_CHECK(rc_t::OK == stmt);        \
    }                                         \
    catch (ExpException &e)                   \
    {                                         \
        BOOST_CHECK(false);                   \
    }                                         \
    BOOST_CHECK(vi.f->is_empty());            \
}

#define TRY_CHECK(stmt)                       \
{                                             \
    try {                                     \
        stmt;                                 \
    }                                         \
    catch (ExpException &e)                   \
    {                                         \
        BOOST_CHECK(false);                   \
    }                                         \
    BOOST_CHECK(vi.f->is_empty());            \
}

#define ADD_EXPECT(stmt)                      \
    vi.f->expect(new stmt)

#define STRICT_ORDER_OFF()                        \
    vi.f->strict_order(false)

BOOST_AUTO_TEST_CASE(test_interface) {
    VppInit vi;
    const std::string go = "GeorgeOrwell";
    const std::string js = "JohnSteinbeck";
    rc_t rc = rc_t::OK;

    /*
     * George creates and deletes the interface
     */
    std::string itf1_name = "afpacket1";
    interface itf1(itf1_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);

    /*
     * set the expectation for a afpacket interface create.
     *  2 is the interface handle VPP [mock] assigns
     */
    HW::item<handle_t> hw_ifh(2, rc_t::OK);
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf1_name));

    HW::item<interface::admin_state_t> hw_as_up(interface::admin_state_t::UP, rc_t::OK);
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));

    TRY_CHECK_RC(OM::write(go, itf1));

    HW::item<interface::admin_state_t> hw_as_down(interface::admin_state_t::DOWN, rc_t::OK);
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf1_name));

    TRY_CHECK(OM::remove(go));

    /*
     * George creates the interface, then John brings it down.
     * George's remove is a no-op, sice John also owns the interface
     */
    interface itf1b(itf1_name,
                    interface::type_t::AFPACKET,
                    interface::admin_state_t::DOWN);

    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf1_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));
    TRY_CHECK_RC(OM::write(go, itf1));

    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    TRY_CHECK_RC(OM::write(js, itf1b));

    TRY_CHECK(OM::remove(go));

    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf1_name));
    TRY_CHECK(OM::remove(js));

    /*
     * George adds an interface, then we flush all of Geroge's state
     */
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf1_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));
    TRY_CHECK_RC(OM::write(go, itf1));

    TRY_CHECK(OM::mark(go));

    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf1_name));
    TRY_CHECK(OM::sweep(go));

    /*
     * George adds an interface. mark stale. update the same interface. sweep
     * and expect no delete
     */
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf1_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    TRY_CHECK_RC(OM::write(go, itf1b));

    TRY_CHECK(OM::mark(go));

    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));
    TRY_CHECK_RC(OM::write(go, itf1));

    TRY_CHECK(OM::sweep(go));

    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf1_name));
    TRY_CHECK(OM::remove(go));

    /*
     * George adds an insterface, then we mark that state. Add a second interface
     * an flush the first that is now stale.
     */
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf1_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));
    TRY_CHECK_RC(OM::write(go, itf1));

    TRY_CHECK(OM::mark(go));

    std::string itf2_name = "afpacket2";
    interface itf2(itf2_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);
    HW::item<handle_t> hw_ifh2(3, rc_t::OK);

    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh2, itf2_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh2));
    TRY_CHECK_RC(OM::write(go, itf2));

    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf1_name));
    TRY_CHECK(OM::sweep(go));

    TRY_CHECK(OM::mark(go));

    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh2));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh2, itf2_name));
    TRY_CHECK(OM::sweep(go));
}

BOOST_AUTO_TEST_CASE(test_bvi) {
    VppInit vi;
    const std::string ernest = "ErnestHemmingway";
    const std::string graham = "GrahamGreene";
    rc_t rc = rc_t::OK;
    l3_binding *l3;

    HW::item<interface::admin_state_t> hw_as_up(interface::admin_state_t::UP,
                                                rc_t::OK);
    HW::item<interface::admin_state_t> hw_as_down(interface::admin_state_t::DOWN,
                                                  rc_t::OK);

    /*
     * Enrest creates a BVI with address 10.10.10.10/24
     */
    route::prefix_t pfx_10("10.10.10.10", 24);

    const std::string bvi_name = "bvi1";
    interface itf(bvi_name,
                  interface::type_t::BVI,
                  interface::admin_state_t::UP);
    HW::item<handle_t> hw_ifh(4, rc_t::OK);
    HW::item<route::prefix_t> hw_pfx_10(pfx_10, rc_t::OK);

    ADD_EXPECT(interface::loopback_create_cmd(hw_ifh, bvi_name));
    ADD_EXPECT(interface::set_tag(hw_ifh, bvi_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));
    TRY_CHECK_RC(OM::write(ernest, itf));

    l3 = new l3_binding(itf, pfx_10);
    HW::item<bool> hw_l3_bind(true, rc_t::OK);
    HW::item<bool> hw_l3_unbind(false, rc_t::OK);
    ADD_EXPECT(l3_binding::bind_cmd(hw_l3_bind, hw_ifh.data(), pfx_10));
    TRY_CHECK_RC(OM::write(ernest, *l3));

    // change the MAC address on the BVI
    interface itf_new_mac(bvi_name,
                          interface::type_t::BVI,
                          interface::admin_state_t::UP);
    l2_address_t l2_addr({0,1,2,3,4,5});
    HW::item<l2_address_t> hw_mac(l2_addr, rc_t::OK);
    itf_new_mac.set(l2_addr);
    ADD_EXPECT(interface::set_mac_cmd(hw_mac, hw_ifh));
    TRY_CHECK_RC(OM::write(ernest, itf_new_mac));

    // create/write the interface to the OM again but with an unset MAC
    // this should not generate a MAC address update
    TRY_CHECK_RC(OM::write(ernest, itf));

    // change the MAC address on the BVI - again
    interface itf_new_mac2(bvi_name,
                           interface::type_t::BVI,
                           interface::admin_state_t::UP);
    l2_address_t l2_addr2({0,1,2,3,4,6});
    HW::item<l2_address_t> hw_mac2(l2_addr2, rc_t::OK);
    itf_new_mac2.set(l2_addr2);
    ADD_EXPECT(interface::set_mac_cmd(hw_mac2, hw_ifh));
    TRY_CHECK_RC(OM::write(ernest, itf_new_mac2));

    delete l3;
    ADD_EXPECT(l3_binding::unbind_cmd(hw_l3_unbind, hw_ifh.data(), pfx_10));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::loopback_delete_cmd(hw_ifh));
    TRY_CHECK(OM::remove(ernest));

    /*
     * Graham creates a BVI with address 10.10.10.10/24 in Routing Domain
     */


    route_domain *rd = new route_domain(l3_proto_t::IPV4, 1);
    HW::item<bool> hw_rd_create(true, rc_t::OK);
    HW::item<bool> hw_rd_delete(false, rc_t::OK);
    HW::item<route::table_id_t> hw_rd_bind(1, rc_t::OK);
    HW::item<route::table_id_t> hw_rd_unbind(route::DEFAULT_TABLE, rc_t::OK);
    ADD_EXPECT(route_domain::create_cmd(hw_rd_create, l3_proto_t::IPV4, 1));
    TRY_CHECK_RC(OM::write(graham, *rd));

    const std::string bvi2_name = "bvi2";
    interface *itf2 = new interface(bvi2_name,
                                    interface::type_t::BVI,
                                    interface::admin_state_t::UP,
                                    *rd);
    HW::item<handle_t> hw_ifh2(5, rc_t::OK);

    ADD_EXPECT(interface::loopback_create_cmd(hw_ifh2, bvi2_name));
    ADD_EXPECT(interface::set_tag(hw_ifh2, bvi2_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh2));
    ADD_EXPECT(interface::set_table_cmd(hw_rd_bind, hw_ifh2));

    TRY_CHECK_RC(OM::write(graham, *itf2));

    l3 = new l3_binding(*itf2, pfx_10);
    ADD_EXPECT(l3_binding::bind_cmd(hw_l3_bind, hw_ifh2.data(), pfx_10));
    TRY_CHECK_RC(OM::write(graham, *l3));

    delete l3;
    delete rd;
    delete itf2;

    ADD_EXPECT(l3_binding::unbind_cmd(hw_l3_unbind, hw_ifh2.data(), pfx_10));
    ADD_EXPECT(interface::set_table_cmd(hw_rd_unbind, hw_ifh2));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh2));
    ADD_EXPECT(interface::loopback_delete_cmd(hw_ifh2));
    ADD_EXPECT(route_domain::delete_cmd(hw_rd_delete, l3_proto_t::IPV4, 1));
    TRY_CHECK(OM::remove(graham));
}

BOOST_AUTO_TEST_CASE(test_bridge) {
    VppInit vi;
    const std::string franz = "FranzKafka";
    const std::string dante = "Dante";
    rc_t rc = rc_t::OK;

    /*
     * Franz creates an interface, Bridge-domain, then binds the two
     */

    // interface create
    std::string itf1_name = "afpacket1";
    interface itf1(itf1_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);

    HW::item<handle_t> hw_ifh(3, rc_t::OK);
    HW::item<interface::admin_state_t> hw_as_up(interface::admin_state_t::UP,
                                                rc_t::OK);
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf1_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));

    TRY_CHECK_RC(OM::write(franz, itf1));

    // bridge-domain create
    bridge_domain bd1(33);

    HW::item<uint32_t> hw_bd(33, rc_t::OK);
    ADD_EXPECT(bridge_domain::create_cmd(hw_bd));

    TRY_CHECK_RC(OM::write(franz, bd1));

    // L2-interface create and bind
    // this needs to be delete'd before the flush below, since it too maintains
    // references to the BD and Interface
    l2_binding *l2itf = new l2_binding(itf1, bd1);
    HW::item<bool> hw_l2_bind(true, rc_t::OK);

    ADD_EXPECT(l2_binding::bind_cmd(hw_l2_bind, hw_ifh.data(), hw_bd.data(), false));
    TRY_CHECK_RC(OM::write(franz, *l2itf));

    /*
     * Dante adds an interface to the same BD
     */
    std::string itf2_name = "afpacket2";
    interface itf2(itf2_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);

    HW::item<handle_t> hw_ifh2(4, rc_t::OK);
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh2, itf2_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh2));
    TRY_CHECK_RC(OM::write(dante, itf2));

    // BD add is a no-op since it exists
    TRY_CHECK_RC(OM::write(dante, bd1));

    l2_binding *l2itf2 = new l2_binding(itf2, bd1);
    HW::item<l2_binding::l2_vtr_op_t> hw_set_vtr(l2_binding::l2_vtr_op_t::L2_VTR_POP_1, rc_t::OK);
    l2itf2->set(l2_binding::l2_vtr_op_t::L2_VTR_POP_1, 68);

    ADD_EXPECT(l2_binding::bind_cmd(hw_l2_bind, hw_ifh2.data(), hw_bd.data(), false));
    ADD_EXPECT(l2_binding::set_vtr_op_cmd(hw_set_vtr, hw_ifh2.data(), 68));
    TRY_CHECK_RC(OM::write(dante, *l2itf2));

    // Add some sttic entries to the bridge-domain
    HW::item<bool> hw_be1(true, rc_t::OK);
    mac_address_t mac1({0,1,2,3,4,5});
    bridge_domain_entry *be1 = new bridge_domain_entry(bd1, mac1, itf2);
    ADD_EXPECT(bridge_domain_entry::create_cmd(hw_be1, mac1, bd1.id(), hw_ifh2.data()));
    TRY_CHECK_RC(OM::write(dante, *be1));

    // flush Franz's state
    delete l2itf;
    HW::item<interface::admin_state_t> hw_as_down(interface::admin_state_t::DOWN,
                                                  rc_t::OK);
    ADD_EXPECT(l2_binding::unbind_cmd(hw_l2_bind, hw_ifh.data(), hw_bd.data(), false));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf1_name));
    TRY_CHECK(OM::remove(franz));

    // flush Dante's state - the order the interface and BD are deleted
    // is an uncontrollable artifact of the C++ object destruction.
    delete l2itf2;
    delete be1;
    STRICT_ORDER_OFF();
    ADD_EXPECT(l2_binding::unbind_cmd(hw_l2_bind, hw_ifh2.data(), hw_bd.data(), false));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh2));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh2, itf2_name));
    ADD_EXPECT(bridge_domain_entry::delete_cmd(hw_be1, mac1, bd1.id()));
    ADD_EXPECT(bridge_domain::delete_cmd(hw_bd));
    TRY_CHECK(OM::remove(dante));
}

BOOST_AUTO_TEST_CASE(test_vxlan) {
    VppInit vi;
    const std::string franz = "FranzKafka";
    rc_t rc = rc_t::OK;

    /*
     * Franz creates an interface, Bridge-domain, then binds the two
     */

    // VXLAN create
    vxlan_tunnel::endpoint_t ep(boost::asio::ip::address::from_string("10.10.10.10"),
                               boost::asio::ip::address::from_string("10.10.10.11"),
                               322);

    vxlan_tunnel vxt(ep.src, ep.dst, ep.vni);

    HW::item<handle_t> hw_vxt(3, rc_t::OK);
    ADD_EXPECT(vxlan_tunnel::create_cmd(hw_vxt, "don't-care", ep));

    TRY_CHECK_RC(OM::write(franz, vxt));

    // bridge-domain create
    bridge_domain bd1(33);

    HW::item<uint32_t> hw_bd(33, rc_t::OK);
    ADD_EXPECT(bridge_domain::create_cmd(hw_bd));

    TRY_CHECK_RC(OM::write(franz, bd1));

    // L2-interface create and bind
    // this needs to be delete'd before the flush below, since it too maintains
    // references to the BD and Interface
    l2_binding *l2itf = new l2_binding(vxt, bd1);
    HW::item<bool> hw_l2_bind(true, rc_t::OK);

    ADD_EXPECT(l2_binding::bind_cmd(hw_l2_bind, hw_vxt.data(), hw_bd.data(), false));
    TRY_CHECK_RC(OM::write(franz, *l2itf));

    // flush Franz's state
    delete l2itf;
    HW::item<handle_t> hw_vxtdel(3, rc_t::NOOP);
    STRICT_ORDER_OFF();
    ADD_EXPECT(l2_binding::unbind_cmd(hw_l2_bind, hw_vxt.data(), hw_bd.data(), false));
    ADD_EXPECT(bridge_domain::delete_cmd(hw_bd));
    ADD_EXPECT(vxlan_tunnel::delete_cmd(hw_vxtdel, ep));
    TRY_CHECK(OM::remove(franz));
}

BOOST_AUTO_TEST_CASE(test_vlan) {
    VppInit vi;
    const std::string noam = "NoamChomsky";
    rc_t rc = rc_t::OK;

    std::string itf1_name = "host1";
    interface itf1(itf1_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);

    HW::item<handle_t> hw_ifh(2, rc_t::OK);
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf1_name));

    HW::item<interface::admin_state_t> hw_as_up(interface::admin_state_t::UP, rc_t::OK);
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));

    TRY_CHECK_RC(OM::write(noam, itf1));

    sub_interface *vl33 = new sub_interface(itf1,
                                            interface::admin_state_t::UP,
                                            33);

    HW::item<handle_t> hw_vl33(3, rc_t::OK);
    ADD_EXPECT(sub_interface::create_cmd(hw_vl33, itf1_name+".33", hw_ifh.data(), 33));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_vl33));

    TRY_CHECK_RC(OM::write(noam, *vl33));

    delete vl33;
    HW::item<interface::admin_state_t> hw_as_down(interface::admin_state_t::DOWN, rc_t::OK);
    HW::item<handle_t> hw_vl33_down(3, rc_t::NOOP);
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_vl33));
    ADD_EXPECT(sub_interface::delete_cmd(hw_vl33_down));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf1_name));

    TRY_CHECK(OM::remove(noam));
}

BOOST_AUTO_TEST_CASE(test_acl) {
    VppInit vi;
    const std::string fyodor = "FyodorDostoyevsky";
    const std::string leo = "LeoTolstoy";
    rc_t rc = rc_t::OK;

    /*
     * Fyodor adds an ACL in the input direction
     */
    std::string itf1_name = "host1";
    interface itf1(itf1_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);
    HW::item<handle_t> hw_ifh(2, rc_t::OK);
    HW::item<interface::admin_state_t> hw_as_up(interface::admin_state_t::UP, rc_t::OK);
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf1_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));
    TRY_CHECK_RC(OM::write(fyodor, itf1));

    route::prefix_t src("10.10.10.10", 32);
    ACL::l3_rule r1(10, ACL::action_t::PERMIT, src, route::prefix_t::ZERO);
    ACL::l3_rule r2(20, ACL::action_t::DENY, route::prefix_t::ZERO, route::prefix_t::ZERO);

    std::string acl_name = "acl1";
    ACL::l3_list acl1(acl_name);
    acl1.insert(r2);
    acl1.insert(r1);
    ACL::l3_list::rules_t rules = {r1, r2};

    HW::item<handle_t> hw_acl(2, rc_t::OK);
    ADD_EXPECT(ACL::l3_list::update_cmd(hw_acl, acl_name, rules));
    TRY_CHECK_RC(OM::write(fyodor, acl1));

    ACL::l3_binding *l3b = new ACL::l3_binding(ACL::direction_t::INPUT, itf1, acl1);
    HW::item<bool> hw_binding(true, rc_t::OK);
    ADD_EXPECT(ACL::l3_binding::bind_cmd(hw_binding, ACL::direction_t::INPUT,
                                       hw_ifh.data(), hw_acl.data()));
    TRY_CHECK_RC(OM::write(fyodor, *l3b));

    /**
     * Leo adds an L2 ACL in the output direction
     */
    TRY_CHECK_RC(OM::write(leo, itf1));

    std::string l2_acl_name = "l2_acl1";
    mac_address_t mac({0x0, 0x0, 0x1, 0x2, 0x3, 0x4});
    mac_address_t mac_mask({0xff, 0xff, 0xff, 0x0, 0x0, 0x0});
    ACL::l2_rule l2_r1(10, ACL::action_t::PERMIT, src, mac, mac_mask);
    ACL::l2_rule l2_r2(20, ACL::action_t::DENY, src, {}, {});

    ACL::l2_list l2_acl(l2_acl_name);
    l2_acl.insert(l2_r2);
    l2_acl.insert(l2_r1);

    ACL::l2_list::rules_t l2_rules = {l2_r1, l2_r2};

    HW::item<handle_t> l2_hw_acl(3, rc_t::OK);
    ADD_EXPECT(ACL::l2_list::update_cmd(l2_hw_acl, l2_acl_name, l2_rules));
    TRY_CHECK_RC(OM::write(leo, l2_acl));

    ACL::l2_binding *l2b = new ACL::l2_binding(ACL::direction_t::OUTPUT, itf1, l2_acl);
    HW::item<bool> l2_hw_binding(true, rc_t::OK);
    ADD_EXPECT(ACL::l2_binding::bind_cmd(l2_hw_binding, ACL::direction_t::OUTPUT,
                                       hw_ifh.data(), l2_hw_acl.data()));
    TRY_CHECK_RC(OM::write(leo, *l2b));

    delete l2b;
    ADD_EXPECT(ACL::l2_binding::unbind_cmd(l2_hw_binding, ACL::direction_t::OUTPUT,
                                         hw_ifh.data(), l2_hw_acl.data()));
    ADD_EXPECT(ACL::l2_list::delete_cmd(l2_hw_acl));
    TRY_CHECK(OM::remove(leo));

    delete l3b;
    HW::item<interface::admin_state_t> hw_as_down(interface::admin_state_t::DOWN,
                                                  rc_t::OK);
    STRICT_ORDER_OFF();
    ADD_EXPECT(ACL::l3_binding::unbind_cmd(hw_binding, ACL::direction_t::INPUT,
                                         hw_ifh.data(), hw_acl.data()));
    ADD_EXPECT(ACL::l3_list::delete_cmd(hw_acl));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf1_name));

    TRY_CHECK(OM::remove(fyodor));
}

BOOST_AUTO_TEST_CASE(test_arp_proxy) {
    VppInit vi;
    const std::string kurt = "KurtVonnegut";
    rc_t rc = rc_t::OK;

    asio::ip::address_v4 low  = asio::ip::address_v4::from_string("10.0.0.0");
    asio::ip::address_v4 high = asio::ip::address_v4::from_string("10.0.0.255");

    arp_proxy_config ap(low, high);
    HW::item<bool> hw_ap_cfg(true, rc_t::OK);
    ADD_EXPECT(arp_proxy_config::config_cmd(hw_ap_cfg, low, high));
    TRY_CHECK_RC(OM::write(kurt, ap));

    std::string itf3_name = "host3";
    interface itf3(itf3_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);
    HW::item<handle_t> hw_ifh(2, rc_t::OK);
    HW::item<interface::admin_state_t> hw_as_up(interface::admin_state_t::UP, rc_t::OK);
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf3_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));
    TRY_CHECK_RC(OM::write(kurt, itf3));

    arp_proxy_binding *apb = new arp_proxy_binding(itf3, ap);
    HW::item<bool> hw_binding(true, rc_t::OK);
    ADD_EXPECT(arp_proxy_binding::bind_cmd(hw_binding, hw_ifh.data()));
    TRY_CHECK_RC(OM::write(kurt, *apb));

    delete apb;

    HW::item<interface::admin_state_t> hw_as_down(interface::admin_state_t::DOWN,
                                                  rc_t::OK);
    STRICT_ORDER_OFF();
    ADD_EXPECT(arp_proxy_binding::unbind_cmd(hw_binding, hw_ifh.data()));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf3_name));
    ADD_EXPECT(arp_proxy_config::unconfig_cmd(hw_ap_cfg, low, high));

    TRY_CHECK(OM::remove(kurt));
}

BOOST_AUTO_TEST_CASE(test_ip_unnumbered) {
    VppInit vi;
    const std::string eric = "EricAmbler";
    rc_t rc = rc_t::OK;

    /*
     * Interface 1 has the L3 address
     */
    std::string itf1_name = "host1";
    interface itf1(itf1_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);
    HW::item<handle_t> hw_ifh(2, rc_t::OK);
    HW::item<interface::admin_state_t> hw_as_up(interface::admin_state_t::UP, rc_t::OK);
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf1_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));
    TRY_CHECK_RC(OM::write(eric, itf1));

    route::prefix_t pfx_10("10.10.10.10", 24);
    l3_binding *l3 = new l3_binding(itf1, pfx_10);
    HW::item<bool> hw_l3_bind(true, rc_t::OK);
    HW::item<bool> hw_l3_unbind(false, rc_t::OK);
    ADD_EXPECT(l3_binding::bind_cmd(hw_l3_bind, hw_ifh.data(), pfx_10));
    TRY_CHECK_RC(OM::write(eric, *l3));

    /*
     * Interface 2 is unnumbered
     */
    std::string itf2_name = "host2";
    interface itf2(itf2_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);

    HW::item<handle_t> hw_ifh2(4, rc_t::OK);
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh2, itf2_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh2));
    TRY_CHECK_RC(OM::write(eric, itf2));

    ip_unnumbered *ipun = new ip_unnumbered(itf2, itf1);
    HW::item<bool> hw_ip_cfg(true, rc_t::OK);
    HW::item<bool> hw_ip_uncfg(false, rc_t::OK);
    ADD_EXPECT(ip_unnumbered::config_cmd(hw_ip_cfg, hw_ifh2.data(), hw_ifh.data()));
    TRY_CHECK_RC(OM::write(eric, *ipun));

    delete l3;
    delete ipun;

    HW::item<interface::admin_state_t> hw_as_down(interface::admin_state_t::DOWN, rc_t::OK);
    STRICT_ORDER_OFF();
    ADD_EXPECT(ip_unnumbered::unconfig_cmd(hw_ip_uncfg, hw_ifh2.data(), hw_ifh.data()));
    ADD_EXPECT(l3_binding::unbind_cmd(hw_l3_unbind, hw_ifh.data(), pfx_10));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh2));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh2, itf2_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf1_name));

    TRY_CHECK(OM::remove(eric));
}

BOOST_AUTO_TEST_CASE(test_ip6nd) {
    VppInit vi;
    const std::string paulo = "PauloCoelho";
    rc_t rc = rc_t::OK;

    /*
     * ra config
     */
    std::string itf_name = "host_ip6nd";
    interface itf(itf_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);
    HW::item<handle_t> hw_ifh(3, rc_t::OK);
    HW::item<interface::admin_state_t> hw_as_up(interface::admin_state_t::UP, rc_t::OK);
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));
    TRY_CHECK_RC(OM::write(paulo, itf));

    route::prefix_t pfx_10("fd8f:69d8:c12c:ca62::3", 128);
    l3_binding *l3 = new l3_binding(itf, pfx_10);
    HW::item<bool> hw_l3_bind(true, rc_t::OK);
    HW::item<bool> hw_l3_unbind(false, rc_t::OK);
    ADD_EXPECT(l3_binding::bind_cmd(hw_l3_bind, hw_ifh.data(), pfx_10));
    TRY_CHECK_RC(OM::write(paulo, *l3));

    ra_config ra(0, 1, 0, 4);
    ip6nd_ra_config *ip6ra = new ip6nd_ra_config(itf, ra);
    HW::item<bool> hw_ip6nd_ra_config_config(true, rc_t::OK);
    HW::item<bool> hw_ip6nd_ra_config_unconfig(false, rc_t::OK);
    ADD_EXPECT(ip6nd_ra_config::config_cmd(hw_ip6nd_ra_config_config, hw_ifh.data(), ra));
    TRY_CHECK_RC(OM::write(paulo, *ip6ra));

    /*
     * ra prefix
     */
    ra_prefix ra_pfx(pfx_10, 0, 0, 2592000, 604800);
    ip6nd_ra_prefix *ip6pfx = new ip6nd_ra_prefix(itf, ra_pfx);
    HW::item<bool> hw_ip6nd_ra_prefix_config(true, rc_t::OK);
    HW::item<bool> hw_ip6nd_ra_prefix_unconfig(false, rc_t::OK);
    ADD_EXPECT(ip6nd_ra_prefix::config_cmd(hw_ip6nd_ra_prefix_config, hw_ifh.data(), ra_pfx));
    TRY_CHECK_RC(OM::write(paulo, *ip6pfx));

    delete ip6pfx;

    ADD_EXPECT(ip6nd_ra_prefix::unconfig_cmd(hw_ip6nd_ra_prefix_unconfig, hw_ifh.data(), ra_pfx));

    delete ip6ra;
    delete l3;

    HW::item<interface::admin_state_t> hw_as_down(interface::admin_state_t::DOWN, rc_t::OK);

    STRICT_ORDER_OFF();
    ADD_EXPECT(ip6nd_ra_config::unconfig_cmd(hw_ip6nd_ra_config_unconfig, hw_ifh.data(), ra));
    ADD_EXPECT(l3_binding::unbind_cmd(hw_l3_unbind, hw_ifh.data(), pfx_10));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf_name));

    TRY_CHECK(OM::remove(paulo));
}

BOOST_AUTO_TEST_CASE(test_interface_span) {
    VppInit vi;
    const std::string elif = "ElifShafak";
    rc_t rc = rc_t::OK;

    /*
     * Interface 1 to be mirrored
     */
    std::string itf1_name = "port-from";
    interface itf1(itf1_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);
    HW::item<handle_t> hw_ifh(2, rc_t::OK);
    HW::item<interface::admin_state_t> hw_as_up(interface::admin_state_t::UP, rc_t::OK);
    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh, itf1_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up, hw_ifh));
    TRY_CHECK_RC(OM::write(elif, itf1));

    /*
     * Interface 2 where traffic is mirrored
     */
    std::string itf2_name = "port-to";
    interface itf2(itf2_name,
                   interface::type_t::AFPACKET,
                   interface::admin_state_t::UP);

    HW::item<handle_t> hw_ifh2(4, rc_t::OK);
    HW::item<interface::admin_state_t> hw_as_up2(interface::admin_state_t::UP, rc_t::OK);

    ADD_EXPECT(interface::af_packet_create_cmd(hw_ifh2, itf2_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_up2, hw_ifh2));
    TRY_CHECK_RC(OM::write(elif, itf2));

    interface_span *itf_span = new interface_span(itf1, itf2, interface_span::state_t::TX_RX_ENABLED);
    HW::item<bool> hw_is_cfg(true, rc_t::OK);
    HW::item<bool> hw_is_uncfg(true, rc_t::OK);
    ADD_EXPECT(interface_span::config_cmd(hw_is_cfg, hw_ifh.data(), hw_ifh2.data(), interface_span::state_t::TX_RX_ENABLED));
    TRY_CHECK_RC(OM::write(elif, *itf_span));

    HW::item<interface::admin_state_t> hw_as_down(interface::admin_state_t::DOWN, rc_t::OK);
    HW::item<interface::admin_state_t> hw_as_down2(interface::admin_state_t::DOWN, rc_t::OK);

    delete itf_span;
    STRICT_ORDER_OFF();
    ADD_EXPECT(interface_span::unconfig_cmd(hw_is_uncfg, hw_ifh.data(), hw_ifh2.data()));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down, hw_ifh));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh, itf1_name));
    ADD_EXPECT(interface::state_change_cmd(hw_as_down2, hw_ifh2));
    ADD_EXPECT(interface::af_packet_delete_cmd(hw_ifh2, itf2_name));

    TRY_CHECK(OM::remove(elif));
}

BOOST_AUTO_TEST_SUITE_END()
