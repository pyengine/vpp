# Copyright (c) 2015 Cisco and/or its affiliates.
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
HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CPPFLAGS += -g -I/usr/local/include -I$(prefix)/../vpp/include -pthread
CXXFLAGS = -std=c++11

LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl -lvppinfra

PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin

GRPC_CPP_PLUGIN_PATH ?= $(shell which $(GRPC_CPP_PLUGIN))

PROTOS_PATH = $(srcdir)/apis/protos

vpath %.proto $(PROTOS_PATH)

########################################
# GRPC + protobuf
########################################
libvpp_api_server_plugin_la_SOURCES = 			\
	apis/server/api_rpc_server.cc               \
	apis/protos/vppcore.pb.cc                \
	apis/protos/vppcore.grpc.pb.cc		\
	apis/server/api_handler.c

libvpp_api_server_plugin_la_LDFLAGS =	\
	-L$(shell pwd)/../../vpp \
 	-lvppversion
libvpp_api_server_plugin_la_LIBADD =	\
                   -lvlibapi -lvlibmemory  -lvlib -lvppinfra
%.grpc.pb.cc: %.proto
	$(info Creating grpc pb for $@)
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(shell pwd)/apis/protos/ --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	$(info Creating pb for $@)
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(shell pwd)/apis/protos/ $<

vpppluginsdir = ${libdir}/vpp_plugins
vppplugins_LTLIBRARIES = libvpp_api_server_plugin.la
########################################
# VPP APIs - GRPC
########################################

libapis_plugin_la_SOURCES =			\
	apis/server/api_server.c


libapis_plugin_la_LIBADD =	\
	libvpp_api_server_plugin.la

vppplugins_LTLIBRARIES += libapis_plugin.la

