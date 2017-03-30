
#ifndef __DECODER_H__
#define __DECODER_H__

#include <map>
#include <google/protobuf/io/coded_stream.h>

class DecodeFunctor {
public:
    virtual void* decode(const uint8_t *data,
                         uint32_t size) const
    {
        return (0);
    }
};

class Decoder {
public:
    static void* decode (uint32_t type,
                         const uint8_t *data,
                         uint32_t size);

    static bool register_decoder (uint32_t type,
                                  const DecodeFunctor *functor);

private:
    static std::map<uint32_t, const DecodeFunctor*> m_decoders;
};


template<class T> using decode_dispatch = void*(*)(const T &);

template <uint32_t V, typename T>
class DecodeTemplate: public DecodeFunctor
{
public:
    DecodeTemplate(decode_dispatch<T> f):
        m_f(f)
    {
        Decoder::register_decoder(V, this);
    }

    virtual void* decode(const uint8_t *data,
                         uint32_t size) const
    {
        T msg;

        msg.ParseFromArray(data, size);

        return (m_f(msg));
    }
private:
    decode_dispatch<T> m_f;
};

#endif
