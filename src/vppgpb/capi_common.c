
#include <vppgpb/capi_types.h>
#include <vlibmemory/api.h>

api_buffer_t*
capi_get_buffer (uint16_t type,
                 uint16_t size)
{
    api_buffer_t *mp = vl_msg_api_alloc(sizeof(api_buffer_t) + size);

    mp->size = size;
    mp->type = (USE_GPB | type);

    return (mp);
}
