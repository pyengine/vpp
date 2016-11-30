"""The Python implementation of the GRPC vppcore client."""

from __future__ import print_function
import cmd, sys
import grpc

import vppcore_pb2

global stub
channel = grpc.insecure_channel('localhost:50051')
stub = vppcore_pb2.vppStub(channel)

class vppShell(cmd.Cmd):
    intro = 'Welcome to the VPP shell.\n'
    prompt = 'vpp# '
    file = None
    
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
  vppShell().cmdloop()

if __name__ == '__main__':
  run()
