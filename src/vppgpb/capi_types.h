
#ifndef __CAPI_TYPES_H__
#define __CAPI_TYPES_H__

#include <stdint.h>

typedef struct api_buffer_t_
{
    uint16_t type;
    uint16_t size;
    uint32_t client_index;
    uint8_t data[];
} api_buffer_t;

#define USE_GPB ((uint16_t)0x8000)

extern api_buffer_t* capi_get_buffer(uint16_t type,
                                     uint16_t size);

extern void capi_init(void);

#endif
