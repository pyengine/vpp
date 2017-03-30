
#include <vppgpb/decoder.h>

void* Decoder::decode (uint32_t type,
                       const uint8_t *data,
                       uint32_t size)
{
    return (Decoder::m_decoders.at(type)->decode(data, size));
}

bool
Decoder::register_decoder (uint32_t type,
                           const DecodeFunctor *functor)
{
    Decoder::m_decoders[type] = functor;

    return true;
}

std::map<uint32_t, const DecodeFunctor*> Decoder::m_decoders;
