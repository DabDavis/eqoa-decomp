// tcpsocket.cpp (name ours) -- STATION 0x002EA1C8 .. 0x002EA680: one TCP socket
// over libinet, on a libnet client of its own (cf. SETUP's VUrl::OpenSocket).
#include "station.h"

// 0x002EA1C8.  Nothing calls it.
TCPSocket::TCPSocket(const char *host, int port)
{
    m_host = 0;
    m_open = 0;
    m_port = -1;
    m_cid = -1;
    printf("TCPSocket <%s:%i>\n", host, port);
    m_netBuf = (unsigned int *)memalign(64, 2048);
    sceLibnetInitialize(&m_client, 2048, 8192, 32);
    SetHost(host);
    SetPort(port);
}

// 0x002EA260.
TCPSocket::TCPSocket()
{
    m_host = 0;
    m_open = 0;
    m_port = -1;
    m_cid = -1;
    m_netBuf = (unsigned int *)memalign(64, 2048);
    sceLibnetInitialize(&m_client, 2048, 8192, 32);
}

// 0x002EA2C0.
TCPSocket::~TCPSocket()
{
    if (m_host)
    {
        free(m_host);
        m_host = 0;
    }
    if (m_netBuf)
    {
        free(m_netBuf);
        m_netBuf = 0;
        sceLibnetTerminate(&m_client);
    }
    m_port = -1;
    m_cid = -1;
}

// 0x002EA340.
void TCPSocket::SetHost(const char *host)
{
    if (m_host)
    {
        free(m_host);
        m_host = 0;
    }
    if (host)
        m_host = strdup(host);
}

// 0x002EA398.
void TCPSocket::SetPort(int port)
{
    m_port = port;
}

// 0x002EA3A0.  Each failure is the else of the step it follows.
int TCPSocket::Open()
{
    sceInetParam param;
    printf("Socket open\n");
    memset(&param, 0, sizeof(param));
    param.type = 1;
    param.local_port = -1;
    printf("Socket open 1\n");
    if (m_host)
    {
        printf("Socket open 2\n");
        if (!sceInetName2Address(&m_client, m_netBuf, 0, &param.remote_addr, m_host, 5000, 1))
        {
            printf("Socket open 3\n");
            param.remote_port = m_port;
            m_cid = sceInetCreate(&m_client, m_netBuf, &param);
            printf("Socket open 4\n");
            if (sceInetOpen(&m_client, m_netBuf, m_cid, 10000))
            {
                printf("sceInetOpen failed\n");
                return -3;
            }
            printf("Socket opened succesfully\n");
        }
        else
        {
            printf("DNS lookup failed\n");
            return -2;
        }
    }
    else
    {
        printf("Socket open 5\n");
        return -1;
    }
    printf("Socket open 6\n");
    m_open = 1;
    return 0;
}

// 0x002EA4F0.
int TCPSocket::IsValid()
{
    return m_cid != -1;
}

// 0x002EA500.
int TCPSocket::IsValid2()
{
    return m_cid != -1;
}

// 0x002EA510.
int TCPSocket::Send(const void *buf, int len, int *flags, int ms)
{
    if (m_cid == -1)
        return -1;
    if (!IsValid())
        return -1;
    printf("In send\n");
    int r = sceInetSend(&m_client, m_netBuf, m_cid, buf, len, flags, ms);
    printf("!!Send returned %i\n", r);
    return r;
}

// 0x002EA5C8.
int TCPSocket::Recv(void *buf, int len, int *flags, int ms)
{
    if (m_cid == -1)
        return -1;
    return sceInetRecv(&m_client, m_netBuf, m_cid, buf, len, flags, ms);
}

// 0x002EA618.
int TCPSocket::Close()
{
    printf("Socket close\n");
    if (m_cid == -1)
        return -1;
    int r = sceInetClose(&m_client, m_netBuf, m_cid, 1000);
    m_cid = -1;
    m_open = 0;
    return r;
}

// 0x002EA678.
int TCPSocket::IsOpen()
{
    return m_open;
}
