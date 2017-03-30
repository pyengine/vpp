
#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <string>

extern "C" {
    #include <vppgpb/capi_client.h>
}

class Connection
{
public:
    Connection(const char *name);
    ~Connection();

    template <typename T> int transmit(uint32_t type,
                                       const T &t)
    {
        api_buffer_t *buffer;
        size_t size;

        size = t.ByteSize();
        buffer = capi_get_buffer(type, size);
        
        t.SerializeToArray(buffer->data, size);

        return (NULL != capi_transmit(&m_conn, buffer));
    }

    template <typename T, typename U> int transmit(uint32_t type,
                                                   const T &request,
                                                   U &reply) const
    {
        api_buffer_t *buffer, *b_reply;
        size_t size;

        size = request.ByteSize();
        buffer = capi_get_buffer(type, size);

        request.SerializeToArray(buffer->data, size);
        
        b_reply = capi_transmit(&m_conn, buffer);

        if (NULL != b_reply)
        {
            reply.ParseFromArray(b_reply->data, b_reply->size);
            return (1);
        }
        return (0);
    }

private:
    api_connection_t m_conn;
};

#endif
