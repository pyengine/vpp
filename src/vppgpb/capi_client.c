
#include <vppgpb/capi_client.h>

#include <vnet/vnet.h>
#include <vlibmemory/api.h>

#include <vpp/api/vpe_msg_enum.h>

#define vl_typedefs		/* define message structures */
#include <vpp/api/vpe_all_api_h.h>
#undef vl_typedefs

#define vl_endianfun		/* define message structures */
#include <vpp/api/vpe_all_api_h.h>
#undef vl_endianfun

/* instantiate all the print functions we know about */
#define vl_print(handle, ...) vlib_cli_output (handle, __VA_ARGS__)
#define vl_printfun
#include <vpp/api/vpe_all_api_h.h>
#undef vl_printfun

#include <vlibapi/api_helper_macros.h>
vpe_api_main_t vpe_api_main;

/* Global 'result is ready' flag */
typedef struct vppgpb_result_t_
{
    int ready;
    int value;
    api_buffer_t *reply;
} vppgpb_result_t;

vppgpb_result_t result;

/**
 * time.
 */
static clib_time_t clib_time;

/* S: send a message */
#define SEND(mp)                                                    \
{                                                                   \
    result.ready = 0;                                               \
    result.reply = NULL;                                            \
    vl_msg_api_send_shmem (conn->vl_input_queue, (u8 *)&mp);        \
}

/* W: wait for results, with timeout */
#define WAIT()					\
do {                                            \
    f64 timeout = vppgpb_time_now() + 1.0;      \
                                                \
    while (vppgpb_time_now () < timeout) {      \
        if (result.ready == 1) {                \
            break;                              \
        }                                       \
        vppgpb_suspend (1e3);                   \
    }                                           \
} while(0);

static f64
vppgpb_time_now (void)
{
    return clib_time_now (&clib_time);
}

static void
vppgpb_suspend (f64 interval)
{
    struct timespec tv = {
        .tv_sec = 0,
        .tv_nsec = interval,
    };
    nanosleep(&tv, NULL);
}


static void
vl_api_reply_handler (api_buffer_t *reply)
{
    result.value = 1;
    result.reply = reply;
    reply->size = ntohs(reply->size);
}

#define foreach_vppgpb_api_reply_msg                                    \
_(CREATE_LOOPBACK_REPLY, create_loopback_reply)                         \
_(SW_INTERFACE_SET_FLAGS_REPLY, sw_interface_set_flags_reply)           \
_(SW_INTERFACE_ADD_DEL_ADDRESS_REPLY, sw_interface_add_del_address_reply) \
_(IP_ADD_DEL_ROUTE_REPLY, ip_add_del_route_reply)                       \

static void
creply_api_hookup (void)
{
#define _(N,n)                                                   \
    vl_msg_api_set_handlers(USE_GPB | VL_API_##N, #n,            \
                            vl_api_reply_handler,                \
                            vl_noop_handler,                     \
                            vl_noop_handler,                     \
                            vl_noop_handler,                     \
                            sizeof(api_buffer_t), 1);
  foreach_vppgpb_api_reply_msg;
#undef _
}

int
capi_connect (const char *name,
              api_connection_t *conn)
{
  api_main_t *am = &api_main;

  if (vl_client_connect_to_vlib ("/vpe-api", name, 32) < 0)
    return -1;

  conn->vl_input_queue = am->shmem_hdr->vl_input_queue;
  conn->my_client_index = am->my_client_index;

  return 0;
}

void
capi_disconnect (void)
{
   vl_client_disconnect_from_vlib ();   
}

api_buffer_t*
capi_transmit (const api_connection_t *conn,
               api_buffer_t *buffer)
{
    buffer->type = ntohs(buffer->type);
    buffer->size = ntohs(buffer->size);
    buffer->client_index = ntohl(conn->my_client_index);

    SEND(buffer);
    WAIT();

    return (result.value ?
            result.reply:
            NULL);
}

void
capi_init (void)
{
    clib_mem_init(0, 128 << 15);
    clib_time_init(&clib_time);    
    creply_api_hookup();
}
