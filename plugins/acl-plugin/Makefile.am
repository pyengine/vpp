# Copyright (c) 2016 Cisco and/or its affiliates.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CFLAGS = -Wall
AM_LDFLAGS = -module -shared -avoid-version

vpppluginsdir = ${libdir}/vpp_plugins
vppplugins_LTLIBRARIES = acl_plugin.la

acl_plugin_la_SOURCES =				\
	acl/acl.c				\
	acl/node.c				\
	acl/acl_plugin.api.h

BUILT_SOURCES = acl/acl.api.h acl/acl.py

SUFFIXES = .api.h .api

#
# ACL API
#
%.api.h: %.api
	mkdir -p $(dir $@) ; \
	$(CC) $(CPPFLAGS) -E -P -C -x c $^ \
	| vppapigen --input - --output $@ --show-name $@

define python_packaging_fn
$(file >setup.py,from setuptools import setup)
$(file >>setup.py,setup(name='vpp_papi_plugins.$(1)',packages=['vpp_papi_plugins','vpp_papi_plugins.$(1)'],namespace_packages=['vpp_papi_plugins']))
$(file >vpp_papi_plugins/__init__.py,__import__('pkg_resources').declare_namespace(__name__))
$(file >vpp_papi_plugins/$(1)/__init__.py,from . $(1) import *)
endef

%.py: %.api
	$(info Creating Python binding for $@)
	mkdir -p $(dir vpp_papi_plugins/$@) ;
	$(CC) $(CPPFLAGS) -E -P -C -x c $<			\
	| vppapigen --input - --python -			\
	| pyvppapigen.py --input - > vpp_papi_plugins/$@
	$(call python_packaging_fn,acl)

noinst_HEADERS =				\
  acl/acl_all_api_h.h				\
  acl/acl_msg_enum.h				\
  acl/acl.api.h

# Remove *.la files
install-data-hook:
	@(cd $(vpppluginsdir) && $(RM) $(vppplugins_LTLIBRARIES))

