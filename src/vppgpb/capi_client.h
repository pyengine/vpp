
#ifndef __CAPI_CLIENT_H__
#define __CAPI_CLIENT_H__

#include <vppgpb/capi_types.h>

typedef struct api_connect_t_
{
    struct _unix_shared_memory_queue *vl_input_queue;
    uint32_t my_client_index;
} api_connection_t;

extern int capi_connect (const char *name,
                         api_connection_t *conn);

extern void capi_disconnect (void);

api_buffer_t *capi_transmit(const api_connection_t *conn,
                            api_buffer_t *request);

#endif
