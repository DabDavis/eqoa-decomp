// ipaddress.cpp -- SUPPORT 0x004ace90..0x004acf58.  Six bytes and four codecs.
//
// SCE's generic address carries the IPv4 word HOST-ordered at +4, so the two
// converters walk it backwards -- octet 0 is byte 7.  The WIRE form is the
// octets in order followed by the port LITTLE-endian first, which is not
// network order and is why `ipaddress_read` cannot just block-copy six bytes.
#include "drdp.h"

void sceaddr_to_ipaddress(const sceInetAddress *pFrom, ipaddress_t *pTo)
{
    pTo->m_ucIp[0] = pFrom->m_ucData[7];
    pTo->m_ucIp[1] = pFrom->m_ucData[6];
    pTo->m_ucIp[2] = pFrom->m_ucData[5];
    pTo->m_ucIp[3] = pFrom->m_ucData[4];
}

void ipaddress_to_sceaddr(const ipaddress_t *pFrom, sceInetAddress *pTo)
{
    pTo->m_ucData[7] = pFrom->m_ucIp[0];
    pTo->m_ucData[6] = pFrom->m_ucIp[1];
    pTo->m_ucData[5] = pFrom->m_ucIp[2];
    pTo->m_ucData[4] = pFrom->m_ucIp[3];
}

void ipaddress_write(const ipaddress_t *pAddress, void *pData)
{
    uns8 *pOut = (uns8 *)pData;

    pOut[0] = pAddress->m_ucIp[0];
    pOut[1] = pAddress->m_ucIp[1];
    pOut[2] = pAddress->m_ucIp[2];
    pOut[3] = pAddress->m_ucIp[3];
    pOut[4] = (uns8)pAddress->m_usPort;
    pOut[5] = (uns8)(pAddress->m_usPort >> 8);
}

void ipaddress_read(ipaddress_t *pAddress, const void *pData)
{
    const uns8 *pIn = (const uns8 *)pData;

    pAddress->m_ucIp[0] = pIn[0];
    pAddress->m_ucIp[1] = pIn[1];
    pAddress->m_ucIp[2] = pIn[2];
    pAddress->m_ucIp[3] = pIn[3];
    pAddress->m_usPort = (uns16)(pIn[5] << 8);
    pAddress->m_usPort |= pIn[4];
}
