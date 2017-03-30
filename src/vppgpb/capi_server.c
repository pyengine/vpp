
#include <arpa/inet.h>
#include <vnet/vnet.h>
#include <vlibapi/api.h>
#include <vlibmemory/api.h>
#include <vpp/api/vpe.pb-c.h>

#include <vppgpb/capi_server.h>
#include <vpp/api/vpe_msg_enum.h>
#include <vnet/ethernet/ethernet.h>

static void *
vppgpb_alloc (void *ctx, size_t size)
{
    return (clib_mem_alloc(size));
}
static void
vppgpb_free (void *ctx, void *ptr)
{
    return (clib_mem_free(ptr));
}

static ProtobufCAllocator vppgpb_allocator = {
    .alloc = vppgpb_alloc,
    .free = vppgpb_free,
    .allocator_data = NULL,
};

ProtobufCAllocator *
capi_allocator (void)
{
    return (&vppgpb_allocator);
}

void
capi_init (void)
{
}
