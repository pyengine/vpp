# GRPC based APIs for VPP
[![VPP](https://fd.io/sites/cpstandard/files/logo_fdio_top.png)](https://wiki.fd.io/view/VPP)

This plugin is attempting to provide VPP API service over [GRPC]. 
With gRPC we can define our service in a .proto file similar to .api files today and implement clients in any of gRPC’s supported languages, which in turn can be run in environments ranging from servers inside Google to your own tablet - all the complexity of communication between different languages and environments is handled by gRPC. We also get all the advantages of working with protocol buffers, including efficient serialization, a simple IDL, and easy interface updating. More at [why-grpc].

Today to support VPP clients for Java, Python, lua language bindings libraries and glue code needs to be built and packaged with VPP. The VPP clients communicate with VPP over shared memory transport and hence have to be co-located on the same system. Implementing GRPC server side in VPP will allow client written in any of the GRPC supported languages and transport to communicate with VPP. For e.g. this will make possible to have remote clients communicating with VPP over HTTP2 - either secure (SSL/TLS, Token based) or insecure channel.

## Design Considerations
* GRPC C++ Server APIs: GRPC while written in C, the C interface is low level and misses many of the conveniences of higher level languages particularly when it comes to method invocation and protobuf serialization [see here](https://groups.google.com/forum/#!topic/grpc-io/e74W0BbSHTo). It is easier to use C++ GRPC APIs to implement server into VPP. Hence the RPC server, thread pool manager for GRPC transport, mapping of messages to server function implementation is in C++. TODO: explore the C APIs.
* VLIB Process Node Vs Thread: I am not able to find GRPC C++ server APIs with suspend/resume control. MAking it a process node takes over the vlib main and never relinquish. So GRPC server is a thread. Due to this the VPP functions and APIs that assume being run as part of vlib process fail. Workaround is creating a dummy process node and setting up vlib main to use that as the current process before function calls into VPP. TODO: explore the possibility of C server with thread pool management built into and as a VLIB process node.
* Synchronization b/n vlib main thread and grpc server threads: Ideally GRPC server would be process nodes in VPP that will make it simple and no special synchronization needed. Now if we see stats thread in VPP it has explicit lock, that is acquired and released by all the APIs and the stats thread that operate on the FIB and interface related objects. Similar locking is added for process node dispatch in VPP and the API handlers in GRPC server implementation that assume process context. This will streamline VPP vlib main thread with the GRPC threads. If there are no GRPC messages being there will be no contention to acquire the lock for dispatch of process nodes in vlib main, hence should have minimal performance impact. 
* Autogeneration of .proto from .api: Implemented in src/vpp-api/java/jvpp/gen/jvppgen/proto_gen.py (is drafty, needed work to consider all cases). The routine is hooked on to the jvpp autogeneration code where to generate the proto file, “--gen_proto yes” needs to be added for jvppgen call. Invocation at ./src/vpp-api/java/jvpp/gen/jvpp_gen.py
* Autogeneration of handlers from api.c: [something to think about]

## Build and Run
The .proto in this plugin is manually written and currently supports only cli_inband, show version and interface dump APIs. There is also example clients in example-client directory with a remote shell implemented.

**Before building VPP**
```sh
    git clone https://github.com/grpc/grpc.git
    git submodule update --init
    sudo apt-get install pkg-config
    make
    sudo make install
    cd grpc/third_party/protobuf
    make
    sudo make install
```

Build and run VPP and the api-server plugin will listen on 50051 port (insecure channel).
```sh
vpp# sh plugins
 Plugin path is: <>vpp/build-root/install-vpp_debug-native/plugins/lib64/vpp_plugins
 Plugins loaded: 
  3.<>vpp/build-root/install-vpp_debug-native/plugins/lib64/vpp_plugins/libapis_plugin.so

vpp# show threads
ID     Name      Type     LWP     Sched Policy (Priority)  lcore  Core   Socket State     
0      vpp_main   31793   other (0)                0      0      0      wait
1                 api-server-t31853   other (0)    0      0      0      wait
2                 stats       31854   other (0)    0      0      0      wait
```
**Example clients**
Example clients – C++ and python are in : plugins/apis-plugin/apis/example-client

* For python client pre-install packages:
```sh
    sudo python -m pip install --upgrade pip
    sudo python -m pip install grpc grpcio grpcio-tools
```
* Run “make” to build c++ client and python client

* Run: python vppcore_client.py. Modify the server name and port in this to connect to a remote vpp instance. This will give a remote shell for vpp.

   [GRPC]: <http://www.grpc.io/>
   [why-grpc]: http://www.grpc.io/docs/tutorials/basic/c.html

