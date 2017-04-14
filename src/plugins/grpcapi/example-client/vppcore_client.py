"""The Python implementation of the GRPC vppcore client."""

from __future__ import print_function
import cmd, sys
import grpc
import ipaddr
import vppcore_pb2

global stub
channel = grpc.insecure_channel('localhost:50051')
stub = vppcore_pb2.vppStub(channel)

class vppShell(cmd.Cmd):
    intro = 'Welcome to the VPP shell.\n'
    prompt = 'vpp# '
    file = None

    def default(self, arg):
        global stub
        response = stub.ExecCli(vppcore_pb2.vppCommand(command=arg))
        print(response.cliout)    
    def do_exec(self, arg):
        'execute vpp command'
        global stub
        response = stub.ExecCli(vppcore_pb2.vppCommand(command=arg))
        print(response.cliout)
    def do_quit(self, arg):
        'Close the shell and exit:  BYE'
        return True

def run():
  channel = grpc.insecure_channel('localhost:50051')
  stub = vppcore_pb2.vppStub(channel)
  response = stub.ShowVersion(vppcore_pb2.vppClientContext(name='example_vpp_client'))
  print("VPP client received: ", response)

  print("Testing ip6 fib dump..")

  ip6_fib = stub.ip6_fib_dump(vppcore_pb2.vppClientContext(name='example_vpp_client'))

  for entry in ip6_fib:
     print(ipaddr.IPv6Address(ipaddr.Bytes((entry.address))), '/', entry.address_length, "fib table:", entry.table_id)
     for path in entry.path:
       print("sw if index:", path.sw_if_index, "weight", path.weight, "is_local[%s]" % path.is_local)
       print(ipaddr.IPv6Address(ipaddr.Bytes((path.next_hop))))
  vppShell().cmdloop()

if __name__ == '__main__':
  run()
