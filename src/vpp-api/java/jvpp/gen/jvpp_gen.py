#!/usr/bin/env python
#
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
#

import argparse
import importlib
import sys
import os
import json

from jvppgen import types_gen
from jvppgen import callback_gen
from jvppgen import notification_gen
from jvppgen import dto_gen
from jvppgen import proto_gen
from jvppgen import jvpp_callback_facade_gen
from jvppgen import jvpp_future_facade_gen
from jvppgen import jvpp_impl_gen
from jvppgen import jvpp_c_gen
from jvppgen import util

blacklist = [ "memclnt.api", "flowperpkt.api" ]

# Invocation:
# ~/Projects/vpp/vpp-api/jvpp/gen$ mkdir -p java/io/fd/vpp/jvpp && cd java/io/fd/vpp/jvpp
# ~/Projects/vpp/vpp-api/jvpp/gen/java/io/fd/vpp/jvpp$ ../../../../jvpp_gen.py -idefs_api_vpp_papi.py
#
# Compilation:
# ~/Projects/vpp/vpp-api/jvpp/gen/java/io/fd/vpp/jvpp$ javac *.java dto/*.java callback/*.java
#
# where
# defs_api_vpp_papi.py - vpe.api in python format (generated by vppapigen)

parser = argparse.ArgumentParser(description='VPP Java API generator')
parser.add_argument('-i', action="store", dest="inputfiles", nargs='+')
parser.add_argument('--plugin_name', action="store", dest="plugin_name")
parser.add_argument('--gen_proto', action="store", dest="gen_proto")
parser.add_argument('--root_dir', action="store", dest="root_dir")
args = parser.parse_args()

sys.path.append(".")
cwd = os.getcwd()

print "Generating Java API for %s" % args.inputfiles
print "inputfiles %s" % args.inputfiles
plugin_name = args.plugin_name
print "plugin_name %s" % plugin_name
gen_proto = args.gen_proto
print "generate proto %s" % gen_proto

cfg = {}

base_package = 'io.fd.vpp.jvpp'
plugin_package = base_package + '.' + plugin_name
root_dir = os.path.abspath(args.root_dir)
print "root_dir %s" % root_dir
work_dir = root_dir + "/target/" + plugin_package.replace(".","/")

try:
    os.makedirs(work_dir)
except OSError:
    if not os.path.isdir(work_dir):
        raise

os.chdir(work_dir)

for inputfile in args.inputfiles:
    if any(substring in inputfile for substring in blacklist):
        print "WARNING: Imput file %s blacklisted" % inputfile
        continue
    _cfg = json.load(open(cwd + "/" + inputfile, 'r'))
    if 'types' in cfg:
        cfg['types'].extend(_cfg['types'])
    else:
        cfg['types'] = _cfg['types']
    if 'messages' in cfg:
        cfg['messages'].extend(_cfg['messages'])
    else:
        cfg['messages'] = _cfg['messages']

def is_request_field(field_name):
    return field_name not in {'_vl_msg_id', 'client_index', 'context'}


def is_response_field(field_name):
    return field_name not in {'_vl_msg_id'}


def get_args(t, filter):
    arg_list = []
    for i in t:
        if is_crc(i):
            continue
        if not filter(i[1]):
            continue
        arg_list.append(i[1])
    return arg_list


def get_types(t, filter):
    types_list = []
    lengths_list = []
    crc = None
    for i in t:
        if is_crc(i):
            crc = ('crc', i['crc'][2:])
            continue
        if not filter(i[1]):
            continue
        if len(i) is 3:  # array type
            types_list.append(i[0] + '[]')
            lengths_list.append((i[2], False))
        elif len(i) is 4:  # variable length array type
            types_list.append(i[0] + '[]')
            lengths_list.append((i[3], True))
        else:  # primitive type
            types_list.append(i[0])
            lengths_list.append((0, False))
    return types_list, lengths_list, crc


def is_crc(arg):
    """ Check whether the argument inside message definition is just crc """
    return 'crc' in arg


def get_definitions(defs):
    # Pass 1
    func_list = []
    func_name = {}
    for a in defs:
        java_name = util.underscore_to_camelcase(a[0])

        # For replies include all the arguments except message_id
        if util.is_reply(java_name):
            types, lengths, crc = get_types(a[1:], is_response_field)
            func_name[a[0]] = dict(
                [('name', a[0]), ('java_name', java_name),
                 ('args', get_args(a[1:], is_response_field)), ('full_args', get_args(a[1:], lambda x: True)),
                 ('types', types), ('lengths', lengths), crc])
        # For requests skip message_id, client_id and context
        else:
            types, lengths, crc = get_types(a[1:], is_request_field)
            func_name[a[0]] = dict(
                [('name', a[0]), ('java_name', java_name),
                 ('args', get_args(a[1:], is_request_field)), ('full_args', get_args(a[1:], lambda x: True)),
                 ('types', types), ('lengths', lengths), crc])

        # Indexed by name
        func_list.append(func_name[a[0]])
    return func_list, func_name


types_package = 'types'
dto_package = 'dto'
proto_package = 'proto'
callback_package = 'callback'
notification_package = 'notification'
future_package = 'future'
# TODO find better package name
callback_facade_package = 'callfacade'

types_list, types_name = get_definitions(cfg['types'])

types_gen.generate_types(types_list, plugin_package, types_package, args.inputfiles)

func_list, func_name = get_definitions(cfg['messages'])

dto_gen.generate_dtos(func_list, base_package, plugin_package, plugin_name.title(), dto_package, args.inputfiles)
jvpp_impl_gen.generate_jvpp(func_list, base_package, plugin_package, plugin_name, dto_package, args.inputfiles)
callback_gen.generate_callbacks(func_list, base_package, plugin_package, plugin_name.title(), callback_package, dto_package, args.inputfiles)
notification_gen.generate_notification_registry(func_list, base_package, plugin_package, plugin_name.title(), notification_package, callback_package, dto_package, args.inputfiles)
jvpp_c_gen.generate_jvpp(func_list, plugin_name, args.inputfiles, root_dir)
jvpp_future_facade_gen.generate_jvpp(func_list, base_package, plugin_package, plugin_name.title(), dto_package, callback_package, notification_package, future_package, args.inputfiles)
jvpp_callback_facade_gen.generate_jvpp(func_list, base_package, plugin_package, plugin_name.title(), dto_package, callback_package, notification_package, callback_facade_package, args.inputfiles)
if(gen_proto == 'yes'):
    proto_gen.generate_protos(func_list, base_package, plugin_package, plugin_name.title(), proto_package, args.inputfiles)

print "Java API for %s generated successfully" % args.inputfiles
