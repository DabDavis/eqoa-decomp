// VINet.cpp (game.h's header list names VINet.h) -- STATION 0x002E4E18 .. 0x002E53F0,
// CLIENT 0x00CBA278 .. 0x00CBA660: the glue between a built VIMemStream and DRDP,
// between VIMemStreamVector.cpp and VINetItem.cpp in CLIENT's link.  Every sender
// consumes the stream, sent or not.  The VIArray<VIItemStat> copies that end the
// TU in STATION (0x002E5200 ..) come from the item header, as in every TU that
// includes it; they are main.cpp's verified duplicates.
#include "vinet.h"

extern "C" int atoi(const char *);

// 0x002E4E18
void VINetSend(VIMemStream &s, const VIClientEndpointRef &r)
{
    endpoint_send(r.m_endpoint, r.m_addr, s.m_rbuf + s.m_rpos, s.m_size - s.m_rpos, r.m_type);
    s.Skip(s.m_size - s.m_rpos);
}

// 0x002E4E70: a stream with no connection is still consumed.
void VINetSend(VIMemStream &s, const VIClientConn &c)
{
    if (c.m_conn)
        connection_send(c.m_conn, s.m_rbuf + s.m_rpos, s.m_size - s.m_rpos, c.m_type);
    s.Skip(s.m_size - s.m_rpos);
}

// 0x002E4ED0: the same bytes to every address in the list.
void VINetSend(VIMemStream &s, const VIClientEndpointList &r)
{
    for (int i = 0; i < r.m_addrs.m_size; i++)
        endpoint_send(r.m_endpoint, r.m_addrs[i], s.m_rbuf + s.m_rpos, s.m_size - s.m_rpos, r.m_type);
    s.Skip(s.m_size - s.m_rpos);
}

// 0x002E4F78: the DRDP port the address decodes to is not used.
void VINetSendTo(VIMemStream &s, const VIClientDrdpRef &r)
{
    drdp_addr_t addr;
    unsigned short port;
    drdp_endpoint_to_address(r.m_addr, addr.m_ucIp, &addr.m_usPort, &port);
    drdp_send_addr(r.m_drdp, &addr, 1, s.m_rbuf + s.m_rpos, s.m_size - s.m_rpos);
    s.Skip(s.m_size - s.m_rpos);
}

// 0x002E4FF0: "host:port" into the host and the port; -1 when the host does not
// fit.  Without a colon the port is left alone.
int VINetSplitHostPort(const char *str, char *host, int size, int *port)
{
    int i;
    for (i = 0; str[i]; i++)
    {
        if (i >= size)
            return -1;
        if (str[i] == ':')
        {
            *port = atoi(&str[i + 1]);
            break;
        }
        host[i] = str[i];
    }
    host[i] = 0;
    return 0;
}

// 0x002E5080: a dotted name's first label.
void VINetHostLabel(char *dst, int size, const char *src)
{
    int i;
    for (i = 0; i + 1 < size && src[i] && src[i] != '.'; i++)
        dst[i] = src[i];
    dst[i] = 0;
}

// 0x002E50F0: a name resolved to dotted quad text; the lookup's error otherwise.
int VINetResolve(const char *name, char *buf)
{
    drdp_addr_t addr;
    int r = drdp_namelookup(name, &addr);
    if (r)
        return r;
    VIString::snprintf(buf, 32, "%u.%u.%u.%u", addr.m_ucIp[0], addr.m_ucIp[1], addr.m_ucIp[2],
                       addr.m_ucIp[3]);
    return 0;
}

// 0x002E5148: one line per counter, each appended where the last ended.  `size`
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
