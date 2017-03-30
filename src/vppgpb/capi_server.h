#ifndef __CAPI_SERVER_H__
#define __CAPI_SERVER_H__

#include <vppgpb/capi_types.h>
#include <protobuf-c/protobuf-c.h>

extern ProtobufCAllocator *capi_allocator (void);

#define GPB_SET(_data, _name, _value) {            \
    (_data)->has_##_name = 1;                      \
    (_data)->_name = _value;                       \
}

#define REPLY(req, rep)                                         \
do {                                                            \
    unix_shared_memory_queue_t * q =                            \
        vl_api_client_index_to_input_queue (ntohl(req->client_index));  \
    if (!q)                                                     \
        return;                                                 \
    rep->type = ntohs (rep->type);                              \
    rep->size = ntohs (rep->size);                              \
    vl_msg_api_send_shmem (q, (u8 *)&rep);                      \
} while(0);

#endif
