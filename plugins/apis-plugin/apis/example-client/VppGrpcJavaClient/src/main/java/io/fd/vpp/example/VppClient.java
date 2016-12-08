package io.fd.vpp.example;

import io.fd.vpp.core.ShowVersionReply;
import io.fd.vpp.core.vppClientContext;
import io.fd.vpp.core.vppGrpc;
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import java.util.concurrent.TimeUnit;

public class VppClient {

    private final ManagedChannel channel;
    private final vppGrpc.vppBlockingStub stub;

    private VppClient(ManagedChannelBuilder<?> channelBuilder){
        channel = channelBuilder.build();
        stub = vppGrpc.newBlockingStub(channel);
    }

    public VppClient(String host, int port){
        this(ManagedChannelBuilder.forAddress(host,port).usePlaintext(true));
    }

    public void shutdown() throws InterruptedException {
        channel.shutdown().awaitTermination(5, TimeUnit.SECONDS);
    }

    public void showVersion(){

        vppClientContext context = vppClientContext.newBuilder().
                setName("Java grpc client").build();
        ShowVersionReply response;

        response = stub.showVersion(context);

        System.out.println("+++++++++++++++++++++++++++++++++++++++++++++++");
        System.out.println("Version: "+response.getVersion());
        System.out.println("Build Date: "+response.getBuildDate());
        System.out.println("Buid Directory: "+response.getBuildDirectory());
        System.out.println("Program: "+response.getProgram());
        System.out.println("+++++++++++++++++++++++++++++++++++++++++++++++");
    }

    public void showVersionPerfTest(int numOfCalls){

        vppClientContext context = vppClientContext.newBuilder().
                setName("Java grpc client").build();
        ShowVersionReply response;
        System.out.println("------------Calling show version "+numOfCalls+" times------------");
        long before = System.currentTimeMillis();
        int numOfItr = numOfCalls;
        while(numOfItr>0){
            stub.showVersion(context);
            numOfItr--;
        }
        long after = System.currentTimeMillis();
        long timeTaken = (after-before);
        double timeTakenSec = timeTaken/1000.0;
        System.out.println("+++++++++++++++++++++++++++++++++++++++++++++++");
        System.out.println("time taken: "+timeTaken+" ms");
        System.out.println("calls per second : "+((double)numOfCalls)/timeTakenSec);
        System.out.println("+++++++++++++++++++++++++++++++++++++++++++++++");
    }

    public static void main(String[] args) throws Exception {
        VppClient client = new VppClient("localhost", 50051);
        System.out.println("client started");
        try {
            client.showVersion();
//            client.showVersionPerfTest(20000);
        } finally {
            client.shutdown();
        }
    }
}
