// vitcpconnection.cpp (name ours) -- STATION 0x002E9CD8 .. 0x002EA1C8: a
// length-prefixed message stream over a TCPSocket.
#include "station.h"

// 0x002E9CD8.
VITCPConnection::VITCPConnection()
{
    m_pending = -1;
    m_socket = 0;
    m_id = 0;
    m_listener = 0;
}

// 0x002E9D20.
VITCPConnection::~VITCPConnection()
{
    Close();
}

// 0x002E9D80.
void VITCPConnection::Close()
{
    printf("VITCPConnection::Close 1\n");
    m_send.Clear();
    printf("VITCPConnection::Close 2\n");
    m_recv.Clear();
    printf("VITCPConnection::Close 3\n");
    if (m_socket)
    {
        printf("VITCPConnection::Close 4\n");
        m_socket->Close();
        printf("VITCPConnection::Close 5\n");
        delete m_socket;
        printf("VITCPConnection::Close 6\n");
        m_socket = 0;
    }
    m_pending = -1;
    printf("VITCPConnection::Close 7\n");
}

// 0x002E9E38.
int VITCPConnection::Initialize()
{
    if (m_socket)
        Close();
    m_socket = new TCPSocket;
    return 1;
}

// 0x002E9E88.
int VITCPConnection::Open()
{
    if (!m_socket)
        return 1;
    m_socket->Close();
    int r = m_socket->Open();
    if (!r)
    {
        m_send.Clear();
        m_recv.Clear();
        m_send.SetSocket(m_socket);
        m_recv.SetSocket(m_socket);
    }
    return r;
}

// 0x002E9F20.
int VITCPConnection::IsConnected()
{
    if (!m_socket)
        return 0;
    return m_socket->IsOpen();
}

// 0x002E9F50.
int VITCPConnection::Send(const void *data, int len)
{
    return m_send.Add(data, len);
}

// 0x002E9F70.
int VITCPConnection::Connect(const char *host, int port)
{
    m_socket->SetHost(host);
    m_socket->SetPort(port);
    if (m_socket->IsOpen())
        m_socket->Close();
    return Open();
}

// 0x002E9FD0.  The length on the wire counts its own four bytes.
int VITCPConnection::Process()
{
    if (!IsConnected())
        return 0;
    VINetBuffer &recv = m_recv;
    int sendret = m_send.Send();
    int recvret;
    if (sendret < 0 || (recvret = recv.ReceiveAll()) < 0)
    {
        printf("Closing!!!\n");
        printf("sendret = %i\n; recvret = %i\n", sendret, recvret);
        sprintf(m_error, "VITCPConnection::Close 1\n");
        m_socket->Close();
        m_send.Clear();
        recv.Clear();
        return -50;
    }
    if (recv.m_bufferLength > 0)
        printf("ReceiveBufferLength = %i\n", recv.m_bufferLength);
    for (;;)
    {
        int pending = m_pending;
        if (pending == -1)
        {
            if (recv.m_bufferLength >= 4)
            {
                int r = recv.Read(&m_pending, 4);
                if (r < 0)
                    return r;
                m_pending = VISwap32(m_pending) - 4;
                printf("Got a pending message, %i bytes\n", m_pending);
            }
            if (m_pending == pending)
                return 0;
        }
        if (recv.m_bufferLength >= m_pending)
        {
            char *msg = (char *)malloc(m_pending);
            if (!msg)
                return 0;
            int r = recv.Read(msg, m_pending);
            if (r < 0)
                return r;
            if (m_listener)
                m_listener->OnMessage(msg, m_pending, m_id);
            free(msg);
            m_pending = -1;
        }
        else
        {
            if (m_pending != -1)
                printf("Mismatch: (%i, %i)\n", m_pending, recv.m_bufferLength);
            break;
        }
    }
    return 0;
}
