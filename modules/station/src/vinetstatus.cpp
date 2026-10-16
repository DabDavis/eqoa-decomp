// vinetstatus.cpp (name ours) -- STATION 0x002E5AC8 .. 0x002E5CB8: whether any
// network interface is up, asked of libnet on a private client.
#include "station.h"

// 0x002E5AC8.
VINetStatus::VINetStatus()
{
    memset(&m_client, 0, sizeof(m_client));
    m_netBuf = 0;
    memset(m_ifList, 0, sizeof(m_ifList));
    m_ifCount = 0;
}

// 0x002E5B10.
VINetStatus::~VINetStatus()
{
    if (m_netBuf)
    {
        free(m_netBuf);
        m_netBuf = 0;
    }
    sceLibnetTerminate(&m_client);
    memset(&m_client, 0, sizeof(m_client));
    memset(m_ifList, 0, sizeof(m_ifList));
    m_ifCount = 0;
}

// 0x002E5B90.
int VINetStatus::Init()
{
    if (m_netBuf)
        free(m_netBuf);
    m_netBuf = (unsigned int *)memalign(64, 2048);
    sceLibnetInitialize(&m_client, 2048, 8192, 32);
    if (!m_netBuf)
        return -1;
    m_ifCount = sceInetGetInterfaceList(&m_client, m_netBuf, m_ifList, 10);
    if (m_ifCount < 0)
    {
        m_ifCount = 0;
        return -1;
    }
    return 0;
}

// 0x002E5C18.  State 3 is an interface that has started.
int VINetStatus::GetState()
{
    if (!m_netBuf)
        return 2;
    for (int i = 0; i < m_ifCount; i++)
    {
        int state = 0;
        if (!sceInetCtlGetState(&m_client, m_netBuf, m_ifList[i], &state) && state == 3)
            return 0;
    }
    return 1;
}
