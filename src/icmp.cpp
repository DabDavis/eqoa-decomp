// icmp.cpp -- SUPPORT 0x004baa20..0x004bad40, the ICMP unreachable parser.
//
// The drdp listens on a second, raw socket for ICMP, so a datagram the far
// machine refused can terminate the route at once instead of waiting out the
// timeout.  An ICMP error quotes the IP header and the first eight bytes of the
// datagram that caused it, which for a DRDP packet is the UDP header -- so the
// two ports, and the far address, come back out of the QUOTED packet.
//
// Five statics open the file and the symbol map names none of them, which is
// why endpoint_close needs a SPAN_CAP.  They are not inlined: -O2 in gcc 2.95
// inlines only what is declared `inline`.
#include "drdp.h"

extern "C" void *memset(void *, int, unsigned int);

struct ip_header_t
{
    uns8  m_ucVersionLength;    // +0x00  version << 4 | header words
    uns8  m_ucTos;              // +0x01
    uns16 m_usLength;           // +0x02
    uns16 m_usId;               // +0x04
    uns16 m_usFragment;         // +0x06
    uns8  m_ucTtl;              // +0x08
    uns8  m_ucProtocol;         // +0x09  1 ICMP, 17 UDP
    uns16 m_usChecksum;         // +0x0A
    uns8  m_ucSrc[4];           // +0x0C
    uns8  m_ucDst[4];           // +0x10
};                              //  0x14

struct icmp_header_t
{
    uns8  m_ucType;             // +0x00
    uns8  m_ucCode;             // +0x01
    uns16 m_usChecksum;         // +0x02
    uns32 m_uiRest;             // +0x04
};

struct udp_header_t
{
    uns16 m_usSrcPort;          // +0x00
    uns16 m_usDstPort;          // +0x02
    uns16 m_usLength;           // +0x04
    uns16 m_usChecksum;         // +0x06
};

static int ip_header_get_length(const ip_header_t *pHeader)
{
    return (pHeader->m_ucVersionLength & 15) * 4;
}

static int ip_header_get_version(const ip_header_t *pHeader)
{
    return pHeader->m_ucVersionLength >> 4;
}

// The options, if any, are skipped by seeking past the header's own length.
static void ip_header_read(buffer_t *pBuffer, ip_header_t *pHeader)
{
    int iStart = buffer_get_position(pBuffer);

    buffer_read_uns8(pBuffer, &pHeader->m_ucVersionLength);
    buffer_read_uns8(pBuffer, &pHeader->m_ucTos);
    buffer_read_uns16(pBuffer, &pHeader->m_usLength);
    buffer_read_uns16(pBuffer, &pHeader->m_usId);
    buffer_read_uns16(pBuffer, &pHeader->m_usFragment);
    buffer_read_uns8(pBuffer, &pHeader->m_ucTtl);
    buffer_read_uns8(pBuffer, &pHeader->m_ucProtocol);
    buffer_read_uns16(pBuffer, &pHeader->m_usChecksum);
    buffer_readm_uns8(pBuffer, pHeader->m_ucSrc, 4);
    buffer_readm_uns8(pBuffer, pHeader->m_ucDst, 4);
    buffer_set_position(pBuffer, iStart + ip_header_get_length(pHeader));
}

static void icmp_header_read(buffer_t *pBuffer, icmp_header_t *pHeader)
{
    buffer_read_uns8(pBuffer, &pHeader->m_ucType);
    buffer_read_uns8(pBuffer, &pHeader->m_ucCode);
    buffer_read_uns16(pBuffer, &pHeader->m_usChecksum);
    buffer_read_uns32(pBuffer, &pHeader->m_uiRest);
}

static void udp_header_read(buffer_t *pBuffer, udp_header_t *pHeader)
{
    buffer_read_uns16(pBuffer, &pHeader->m_usSrcPort);
    buffer_read_uns16(pBuffer, &pHeader->m_usDstPort);
    buffer_read_uns16(pBuffer, &pHeader->m_usLength);
    buffer_read_uns16(pBuffer, &pHeader->m_usChecksum);
}

// Types 3 (unreachable), 4 (source quench) and 11 (time exceeded) quote the
// offending datagram; anything else is reported with its type and code only.
// The quoted packet's destination is the far end -- unless it is zero, and
// then the router that sent the error stands in for it.
int icmp_parse(const void *pData, int iSize, icmp_info_t *pInfo)
{
    buffer_t buffer;
    ip_header_t ip;
    icmp_header_t icmp;
    ip_header_t quoted;
    udp_header_t udp;
    int iVersion;

    memset(pInfo, 0, sizeof(*pInfo));
    buffer_init_read(&buffer, pData, iSize, BUFFER_BIG_ENDIAN);
    ip_header_read(&buffer, &ip);
    icmp_header_read(&buffer, &icmp);
    if (buffer.overflow)
        return -1;
    if (ip.m_ucProtocol != 1)
        return -1;
    iVersion = ip_header_get_version(&ip);
    if (iVersion != 4)
        return -1;
    pInfo->m_ucType = icmp.m_ucType;
    pInfo->m_ucCode = icmp.m_ucCode;
    switch (icmp.m_ucType)
    {
    case 3:
    case 4:
    case 11:
        ip_header_read(&buffer, &quoted);
        udp_header_read(&buffer, &udp);
        if (buffer.overflow)
            return -1;
        if (quoted.m_ucProtocol != 17)
            return -1;
        if (ip_header_get_version(&quoted) != iVersion)
            return -1;
        memcpy(pInfo->m_local.m_ucIp, quoted.m_ucSrc, 4);
        pInfo->m_local.m_usPort = udp.m_usSrcPort;
        if (quoted.m_ucDst[0] == 0)
            memcpy(pInfo->m_remote.m_ucIp, ip.m_ucSrc, 4);
        else
            memcpy(pInfo->m_remote.m_ucIp, quoted.m_ucDst, 4);
        pInfo->m_remote.m_usPort = udp.m_usDstPort;
        break;
    }
    return 0;
}
