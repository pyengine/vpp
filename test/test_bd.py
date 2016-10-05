#!/usr/bin/env python

import unittest
from framework import VppTestCase, VppTestRunner
from template_bd import BridgeDomain


## Simple Bridge domain tests which forwards frames
class TestBridgeDomain(BridgeDomain, VppTestCase):
    """ BD Forwarding Test Case """

    ## Run __init__ for all parent class
    #
    #  Initialize BridgeDomain objects, set documentation string for inherited
    #  tests and initialize VppTestCase object which must be called after
    #  doc strings are set.
    def __init__(self, *args):
        BridgeDomain.__init__(self)
        self.test_decap.__func__.__doc__ = ' Forward ethernet frames '
        self.test_encap.__func__.__doc__ = (' Forward ethernet frames '
                                            'opposite direction ')
        VppTestCase.__init__(self, *args)

    ## Implementation for BridgeDomain encapsulate function.
    #
    #  In this case do nothing with packet.
    def encapsulate(self, pkt):
        return pkt

    ## Implementation for BridgeDomain function.
    #
    #  In this case do nothing with packet.
    def decapsulate(self, pkt):
        return pkt

    ## Implementation for BridgeDomain function.
    #
    #  In this case do nothing with packet.
    def check_encapsulation(self, pkt):
        pass

    ## Setup for BD test cases.
    @classmethod
    def setUpClass(cls):
        super(TestBridgeDomain, cls).setUpClass()
        try:
            ## Create 2 pg interfaces.
            cls.create_interfaces(range(2))

            ## Put pg0 and pg1 into BD.
            cls.api("sw_interface_set_l2_bridge pg0 bd_id 1")
            cls.api("sw_interface_set_l2_bridge pg1 bd_id 1")
        except:
            ## In case setUpClass fails run tear down (it's not called automaticly)
            cls.tearDownClass()
            raise

    ## Tear down which is called after each test_* method in this object.
    def tearDown(self):
        super(TestBridgeDomain, self).tearDown()
        self.cli(2, "show bridge-domain 1 detail")

if __name__ == '__main__':
    unittest.main(testRunner=VppTestRunner)
