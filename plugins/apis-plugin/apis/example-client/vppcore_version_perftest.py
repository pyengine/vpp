"" "The Python implementation of the GRPC vppcore client." ""
from __future__ import print_function
import sys, getopt
import grpc
import vppcore_pb2
import timeit
def test_setup (host, port):
   global stub
   target = '{0}:{1}'.format (host, port)
   print ("connecting to ", target)
   channel = grpc.insecure_channel (target)
   stub = vppcore_pb2.vppStub (channel)

def test_showversion ():
   global stub
   t = stub.ShowVersion (vppcore_pb2.vppClientContext (name = 'perfclient'))

def run (argv):
   host = 'localhost'
   port = 50051
   number_of_requests = 15000
   try:
     opts, args = getopt.getopt (argv[1:], "ht:p:n:",["host=", "port=", "number_of_calls="])
   except getopt.GetoptError:
     print (argv[0], ' -t <target vpp host> -p <port> -n <number of calls>')
     sys.exit (2)
   for opt,arg in opts:
        if opt == '-h':
           print (argv[0], ' -t <target vpp host> -p <port> -n <number of calls>')
           sys.exit ()
        elif opt in ("-t", "--host"):
           host = arg
        elif opt in ("-p", "--port"):
           port = int (arg)
        elif opt in ("-n", "--number_of_call"):
           number_of_requests = int (arg)
   test_setup(host, port)
   print ("Calling show verstion on {0}:{1} ({2} times)".format(host, port, number_of_requests))
   time_taken = timeit.timeit ('test_showversion()', number =
			      number_of_requests, setup =
			      "from __main__ import test_showversion")
   print ("Time taken {0} seconds".format (time_taken))
   print ("{0} calls per second".format (number_of_requests / time_taken))

if __name__ == '__main__':
   run (sys.argv)
