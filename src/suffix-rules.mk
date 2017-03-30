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

# Shared suffix rules
# Please do not set "SUFFIXES = .api.h .api" here

%.api.h: %.api @VPPAPIGEN@
	@echo "  APIGEN  " $@ ;					\
	mkdir -p `dirname $@` ;					\
	$(CC) $(CPPFLAGS) -E -P -C -x c $<			\
	| @VPPAPIGEN@ --input - --output $@ --show-name $@ > /dev/null

%.api.json: %.api @VPPAPIGEN@
	@echo "  JSON API" $@ ;					\
	mkdir -p `dirname $@` ;					\
	$(CC) $(CPPFLAGS) -E -P -C -x c $<			\
	| @VPPAPIGEN@ --input - --json $@ > /dev/null

%.proto: %.api @VPPAPIGEN@
	@echo "  GPB API" $@ ;					\
	mkdir -p `dirname $@` ;					\
	$(CC) $(CPPFLAGS) -E -P -C -x c $<			\
	| @VPPAPIGEN@ --input - --gpb $@ > /dev/null;

%.proto: %.pp
	@echo " GPB CP" $@ ;					\
	cp $< $@

%.pb-c.c %.pb-c.h: %.proto
	@echo "  GPB PROTOC " $@;				\
	mkdir -p `dirname $@` ;					\
	protoc-c  -I=. -I=/ --c_out=. $<

%.pb.cc %.pb.h: %.proto
	@echo "  GPB " $@;					\
	mkdir -p `dirname $@` ;					\
	protoc -I=. -I=/ --cpp_out=. $<

%.grpc.pb.cc %.grpc.pb.h: %.proto
	@echo "  GRPC " $@;					\
	mkdir -p `dirname $@` ;					\
	protoc -I=. -I/ --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $<

%.cobra.pb.go: %.proto
	@echo "  GPB COBRA " $@;					\
	protoc --cobra_out=. --plugin=protoc-gen-grpc=`which protoc_gen_cobra` $<
