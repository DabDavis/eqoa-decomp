// vinetbuffer.cpp (name ours) -- STATION 0x002E53F0 .. 0x002E5AC8: a connection's
// send and receive buffers.  Everything goes through malloc/realloc/free, and
// the receive side says what it does at every step.
#include "station.h"

// 0x002E53F0.
VINetSendBuffer::VINetSendBuffer()
{
    m_buf = 0;
    m_length = 0;
    m_socket = 0;
}

// 0x002E5408.
VINetSendBuffer::~VINetSendBuffer()
{
    if (m_buf)
    {
        free(m_buf);
        m_buf = 0;
    }
    m_length = 0;
    m_socket = 0;
}

// 0x002E5460.
int VINetSendBuffer::SetSocket(TCPSocket *s)
{
    if (!s)
        return 1;
    m_socket = s;
    return 0;
}

// 0x002E5478.
void VINetSendBuffer::Clear()
{
    if (m_buf)
    {
        free(m_buf);
        m_buf = 0;
    }
    m_length = 0;
}

// 0x002E54B8.  The success path returns whatever printf did.
int VINetSendBuffer::Add(const void *data, int len)
{
    if (!m_buf)
        m_buf = (char *)malloc(len);
    else
        m_buf = (char *)realloc(m_buf, m_length + len);
    if (!m_buf)
        return 1;
    memcpy(m_buf + m_length, data, len);
    m_length += len;
    printf("Bytes in send buffer = %i\n", m_length); VI_HOST_ONLY(return 0;)
}

// 0x002E5558.  What did not go out stays, at the front of a new buffer.
int VINetSendBuffer::Send()
{
    if (m_length <= 0 || !m_buf)
        return 0;
    if (!m_socket)
        return 0;
    int sent = m_socket->Send(m_buf, m_length, 0, 0);
    printf("!!Sent %i bytes out of %i\n", sent, m_length);
    if (sent < 0)
    {
        m_socket->Close();
        Clear();
        return sent;
    }
    if (sent == m_length)
    {
        free(m_buf);
        m_buf = 0;
        m_length = 0;
    }
    else
    {
        m_length -= sent;
        char *rest = (char *)malloc(m_length);
        if (!rest)
            return sent;
        memcpy(rest, m_buf + sent, m_length);
        free(m_buf);
        m_buf = rest;
    }
    return sent;
}

// 0x002E5650.
VINetBuffer::VINetBuffer()
{
    m_buffer = 0;
    m_x04 = 0;
    m_bufferLength = 0;
    m_x0C = 0;
    m_socket = 0;
}

// 0x002E5670.
VINetBuffer::~VINetBuffer()
{
    if (m_buffer)
    {
        free(m_buffer);
        m_buffer = 0;
    }
    m_x04 = 0;
    m_bufferLength = 0;
    m_x0C = 0;
    m_socket = 0;
}

// 0x002E56D0.
int VINetBuffer::SetSocket(TCPSocket *s)
{
    if (!s)
        return 1;
    m_socket = s;
    return 0;
}

// 0x002E56E8.
void VINetBuffer::Append(const void *data, int len)
{
    printf("RECV: %i bytes\n", len);
    if (!m_buffer)
        m_buffer = (char *)malloc(len);
    else
        m_buffer = (char *)realloc(m_buffer, m_bufferLength + len);
    if (!m_buffer)
    {
        m_x04 = 0;
        m_x0C = 0;
        m_bufferLength = 0;
        printf("ERROR: NULL RECEIVE BUFFER\n");
        return;
    }
    memcpy(m_buffer + m_bufferLength, data, len);
    m_bufferLength += len;
    printf("m_bufferLength = %i\n", m_bufferLength);
}

// 0x002E57B0.  A large read is malloc'd, never freed, and what it read is
// dropped: Append is handed the stack buffer either way.
int VINetBuffer::Receive(int max)
{
    char buf[16384];
    if (!m_socket || !m_socket->IsOpen())
        return 0;
    if (max < 16384)
    {
        memset(buf, 0, 16384);
        int got = m_socket->Recv(buf, max, 0, 0);
        if (got < 0)
        {
            printf("recv returned %i\n", got);
            m_socket->Close();
            Clear();
            return -101;
        }
        if (got)
            Append(buf, got);
    }
    else
    {
        char *big = (char *)malloc(max);
        if (big)
        {
            memset(big, 0, max);
            int got = m_socket->Recv(big, max, 0, 0);
            if (got < 0)
            {
                m_socket->Close();
                Clear();
                return -102;
            }
            if (got)
                Append(buf, got);
        }
    }
    return 0;
}

// 0x002E58E0.
int VINetBuffer::ReceiveAll()
{
    int r;
    while ((r = Receive(1024)) > 0)
        ;
    return r;
}

// 0x002E5928.
void VINetBuffer::Clear()
{
    if (m_buffer)
    {
        free(m_buffer);
        m_buffer = 0;
    }
    m_bufferLength = 0;
    m_x04 = 0;
    m_x0C = 0;
}

// 0x002E5970.
int VINetBuffer::Read(void *dst, int len)
{
    if (!dst)
        return 0;
    if (len <= 0)
        return 0;
    if (m_bufferLength < len)
    {
        int r = Receive(len);
        if (r < 0)
            return r;
    }
    if (!m_buffer)
        return -1;
    if (m_bufferLength < len)
        return m_bufferLength;
    printf("VINetBuffer::1\n");
    memcpy(dst, m_buffer, len);
    if (m_bufferLength == len)
    {
        printf("VINetBuffer::2\n");
        free(m_buffer);
        m_buffer = 0;
        m_x0C = 0;
        m_x04 = 0;
        m_bufferLength = 0;
    }
    else
    {
        printf("VINetBuffer::3\n");
        char *rest = (char *)malloc(m_bufferLength - len);
        if (!rest)
            return 0;
        printf("VINetBuffer::4\n");
        memcpy(rest, m_buffer + len, m_bufferLength - len);
        printf("5\n");
        free(m_buffer);
        m_buffer = rest;
        m_x04 -= len;
        m_bufferLength -= len;
        m_x0C = 0;
    }
    return len;
}
