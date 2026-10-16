// vinet.h -- VINet.cpp (game.h's header list names VINet.h): send a built stream's
// unread bytes over DRDP, then consume them; plus three small host-name helpers
// and the connection statistics as text.  One source file, linked into two
// modules: STATION 0x002E4E18 .. 0x002E53F0 and CLIENT 0x00CBA278 .. 0x00CBA660,
// byte-identical in both.  CLIENT's game.h declares VIClientConn, VIClientDrdpRef
// and three of these functions today; when the file is shared, this is the one
// declaration.  Names provisional except where CLIENT's phase 62 gave them.
#ifndef VINET_H
#define VINET_H

#include "vimemstream.h"
#include "drdpapi.h"

// A connection and the DRDP message type to send on it (CLIENT's GetConn).
struct VIClientConn
{
    VIClientConn(connection_t *conn, int type) : m_conn(conn), m_type(type) {}
    ~VIClientConn() {}
    connection_t *m_conn;          // +0x00: VIClient::m_conn
    int           m_type;          // +0x04: the DRDP message type (251 = 0xFB, the dispatcher's)
};

// The DRDP instance and a 64-bit address, 16 bytes (CLIENT's GetDrdpRef).
struct VIClientDrdpRef
{
    VIClientDrdpRef(drdp_t *drdp, long addr) : m_drdp(drdp), m_addr(addr) {}
    drdp_t *m_drdp;                // +0x00: VIClient::m_drdp
    long    m_addr;                // +0x08
};

// An endpoint, one destination and a message type, 24 bytes.  Nothing in either
// module builds one.  Name ours.
struct VIClientEndpointRef
{
    endpoint_t *m_endpoint;        // +0x00
    long        m_addr;            // +0x08
    int         m_type;            // +0x10
};

// An endpoint, a list of destinations and a message type, 16 bytes.  Nothing in
// either module builds one.  Name ours.
struct VIClientEndpointList
{
    endpoint_t   *m_endpoint;      // +0x00
    VIArray<long> m_addrs;         // +0x04
    int           m_type;          // +0x0C
};

void VINetSend(VIMemStream &s, const VIClientEndpointRef &r);       // 0x002E4E18, no caller
void VINetSend(VIMemStream &s, const VIClientConn &c);              // 0x002E4E70
void VINetSend(VIMemStream &s, const VIClientEndpointList &r);      // 0x002E4ED0, no caller
void VINetSendTo(VIMemStream &s, const VIClientDrdpRef &r);         // 0x002E4F78
int  VINetSplitHostPort(const char *str, char *host, int size, int *port);   // 0x002E4FF0, no caller
void VINetHostLabel(char *dst, int size, const char *src);          // 0x002E5080, no caller
int  VINetResolve(const char *name, char *buf);                     // 0x002E50F0, no caller
void VINetStats(unsigned short *buf, int size, connection_t *c);    // 0x002E5148

#endif
