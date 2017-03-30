
#include <vppgpb/connection.h>

Connection::Connection(const char *name)
{
    int rv;

    rv = capi_connect(name, &m_conn);

    if (0 != rv)
        throw rv;
}

Connection::~Connection()
{
    capi_disconnect();
}
