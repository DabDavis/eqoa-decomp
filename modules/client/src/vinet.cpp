// VINet.cpp (name provisional: game.h's header list names VINet.h) -- CLIENT
// 0x00CBA2D0.., between VIMemStreamVector.cpp and VINetItem.cpp: the glue between a
// built VIMemStream and a DRDP connection.
#include "game.h"

// 0x00CBA2D0: a stream with no connection is still consumed.
void VINetSend(VIMemStream &s, const VIClientConn &c)
{
    if (c.m_conn)
        connection_send(c.m_conn, s.m_rbuf + s.m_rpos, s.m_size - s.m_rpos, c.m_type);
    s.Skip(s.m_size - s.m_rpos);
}

// 0x00CBA5A8: one line per counter, each appended where the last ended.  `size`
// goes down by characters, not bytes.
void VINetStats(unsigned short *buf, int size, connection_t *c)
{
    for (int i = 0; i < connection_get_cstat_max(); i++)
    {
        VIString::snprintf16(buf, size, "%I: %s\n", connection_get_cstat(c, i), connection_get_cstat_label(i));
        int n = VIString::strlen16(buf);
        size -= n;
        buf += n;
    }
}

// 0x00CBA3D8: the DRDP port the address decodes to is not used.
void VINetSendTo(VIMemStream &s, const VIClientDrdpRef &r)
{
    drdp_addr_t addr;
    unsigned short port;
    drdp_endpoint_to_address(r.m_addr, addr.m_ucIp, &addr.m_usPort, &port);
    drdp_send_addr(r.m_drdp, &addr, 1, s.m_rbuf + s.m_rpos, s.m_size - s.m_rpos);
    s.Skip(s.m_size - s.m_rpos);
}
