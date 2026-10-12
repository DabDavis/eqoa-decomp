// usocket.cpp -- SUPPORT 0x004ad480..0x004ae088.  The socket.
//
// One `usocket_t` is a tagged 200-byte allocation whose first eleven bytes this
// file owns and whose remaining 189 belong to `VIInitUSocketEx`: the PS2 port
// does not talk to sceInet from the caller's thread at all, it hands a send or a
// receive to a pair of EE threads hung off the back of the same block.
//
// The file's asserts date it: `_uassert`'s third argument is `__LINE__`, an
// immediate that has to match, so the source below is padded to the line layout
// the original shipped with -- 553, 729, 945, 1116 and 1208.
//
// The interesting half is `forward`.  When a caller passes a non-zero delay,
// `usocket_sendto_addr` does not send the packet to its address: it copies the
// payload into an alloca'd buffer, appends an EIGHT-byte trailer -- the real
// destination's four octets, its port in NETWORK order, the delay in 32ms units
// clamped to 255, and a zero -- and sends the whole thing to `forward` instead.
// `usocket_recvfrom_addr` unwraps the same trailer off the tail of anything that
// arrives FROM `forward`.  It is a lag-and-loss simulator with a relay on the
// other end of it, shipped in the retail binary; the dump this was read out of
// still had 64.37.152.90:10164 sitting in the global.
#include "drdp.h"

extern "C" int   memcmp(const void *, const void *, unsigned);
extern "C" void *memset(void *, int, unsigned);
extern "C" int   sceInetClose(void *cd, void *buf, int fd, int flags);
extern "C" int   sceInetCreate(void *cd, void *buf, void *param);
extern "C" int   sceInetOpen(void *cd, void *buf, int fd, int flags);
extern "C" int   sceInetControl(void *cd, void *buf, int fd, int code, void *data, int size);
extern "C" int   sceInetRecv(void *cd, void *buf, int fd, void *data, int size,
                             int *flags, int a);
extern "C" int   rand();

extern char *net_buf;
extern char  cd[64];

// Phase 44's lever, twice: a four-byte `memcpy` out of a char ARRAY comes out
// `lwl`/`lwr`, where the same text written as a literal in place is word
// aligned and comes out `lw`.  Both tags are copied that way here.
static const char SOCKET_TAG[] = "SOCKET";
static const char NULL_TAG[] = "NULL";

// The port goes into the trailer NETWORK-ordered, and comes back out of it the
// same way.  Written `(x << 8) | (x >> 8)` the two shifts come out in the wrong
// order; the mask is what puts the `srl` first, and it is the same macro at both
// ends of the wrap.
#define BSWAP16(x) ((uns16)((((x) << 8) & 0xff00) | ((x) >> 8)))

// The relay every delayed packet is sent to instead of its real destination.
ipaddress_t forward;

// The SCE socket parameter block `usocket_open_internal` fills in: the protocol
// in the first word and the local port in the second, or -1 for "any".
struct usocket_param_t
{
    int m_iProtocol;
    int m_iPort;
    int m_aiPad[14];
};

int usocket_is_valid(usocket_t *pSocket)
{
    return memcmp(pSocket, SOCKET_TAG, 4) == 0;
}

static int usocket_close_internal(int iFd)
{
    return sceInetClose(cd, net_buf, iFd, -1);
}

static int usocket_open_internal(int *piFd, int iType, ipaddress_t *pLocal,
                                 const ipaddress_t *pRemote)
{
    usocket_param_t param;
    int iResult = -1;
    int iFd = -1;

    memset(&param, 0, sizeof(param));
    switch (iType)
    {
    case USOCKET_TYPE_UDP:
        param.m_iProtocol = 0;
        param.m_iPort = pLocal->m_usPort;
        break;
    case USOCKET_TYPE_ICMP:
        param.m_iProtocol = 3;
        break;
    case USOCKET_TYPE_TCP_CONNECT:
        param.m_iProtocol = 2;
        param.m_iPort = pLocal->m_usPort;
        break;
    case USOCKET_TYPE_TCP_LISTEN:
        param.m_iProtocol = 1;
        param.m_iPort = pLocal->m_usPort;
        break;
    }
    if (param.m_iPort == 0)
        param.m_iPort = -1;

    iResult = sceInetCreate(cd, net_buf, &param);
    if (iResult > 0)
    {
        iFd = iResult;
        if (sceInetOpen(cd, net_buf, iFd, -1) == 0)
        {
            if (iType == USOCKET_TYPE_UDP || iType == USOCKET_TYPE_TCP_CONNECT ||
                iType == USOCKET_TYPE_TCP_LISTEN)
            {
                char aucInfo[76];

                if (sceInetControl(cd, net_buf, iFd, 1, aucInfo, 76) != 1)
                    iResult = -1;
                else
                {
                    memcpy(pLocal->m_ucIp, aucInfo + 20, 4);
                    pLocal->m_usPort = *(uns16 *)(aucInfo + 32);
                    *piFd = iFd;
                    iResult = 0;
                    iFd = -1;
                }
            }
            else
            {
                *piFd = iFd;
                iResult = 0;
                iFd = -1;
            }
        }
        else
            iResult = -1;
    }
    else
        iResult = -1;

    if (iFd != -1)
        usocket_close_internal(iFd);
    return iResult;
}

static int usocket_set_options(int iFd)
{
    return 0;
}

int usocket_create_internal(usocket_t **ppSocket, int iType, int iFd)
{
    usocket_t *pSocket = 0;
    int iResult;

    if ((unsigned)iType >= 4)
    {
        iResult = -1;
        goto done;
    }
    if (iType != USOCKET_TYPE_ICMP)
    {
        iResult = usocket_set_options(iFd);
        if (iResult)
            goto done;
    }
    pSocket = (usocket_t *)malloc(sizeof(usocket_t));
    if (!pSocket)
    {
        iResult = -2;
        goto done;
    }
    memcpy(pSocket->m_szTag, SOCKET_TAG, 4);
    pSocket->m_ucType = (uns8)iType;
    pSocket->m_iFd = iFd;
    pSocket->m_ucListen = 0;
    pSocket->m_ucFlags = 0;
    iResult = VIInitUSocketEx(pSocket);
    if (iResult)
        goto done;
    *ppSocket = pSocket;
    pSocket = 0;
done:
    if (pSocket)
        usocket_destroy(pSocket);
    return iResult;
}

int usocket_create(usocket_t **ppSocket, int iType, ipaddress_t *pLocal,
                   const ipaddress_t *pRemote)
{
    int iFd = -1;
    usocket_t *pSocket = 0;
    int iResult;

    unet_startup();
    iResult = usocket_open_internal(&iFd, iType, pLocal, pRemote);
    if (!iResult)
    {
        iResult = usocket_create_internal(&pSocket, iType, iFd);
        if (!iResult)
        {
            *ppSocket = pSocket;
            iFd = -1;
            pSocket = 0;
        }
    }
    if (iFd != -1)
        usocket_close_internal(iFd);
    if (pSocket)
        usocket_destroy(pSocket);
    return iResult;
}

void usocket_destroy(usocket_t *pSocket)
{
    memcpy(pSocket->m_szTag, NULL_TAG, 4);
    VIFreeUSocketEx(pSocket);
    if (pSocket->m_iFd != -1)
    {
        usocket_close_internal(pSocket->m_iFd);
        pSocket->m_iFd = -1;
    }
    free(pSocket);
}

// -------------------------------------------------------------------------
// usocket.cpp's asserts are dated: `_uassert`'s third argument is __LINE__,
// an immediate the differ compares, so this file has to be padded to the
// line layout the original shipped with.  The five asserts land on 553,
// 729, 945, 1116 and 1208 -- some 1,300 lines for nineteen functions, so
// the file this was cut from carried a great deal more commentary, and
// very probably the Windows and the PS2 back end side by side under an
// #ifdef: nothing else in this library needs a `usocket_select` that
// returns 1 unconditionally or an `accept` that is a hang.
//
// A banner like this one stands in for what is gone, and it is
// load-bearing: delete a line and the assert below reports a different
// number and its function stops matching.
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// -------------------------------------------------------------------------
static int usocket_sendto_internal(usocket_t *pSocket, const ipaddress_t *pAddress,
                                   const void *pData, int iSize)
{
    sceInetAddress addr;
    int iResult;

    VASSERT(pSocket->m_ucType == USOCKET_TYPE_UDP);
    memset(&addr, 0, sizeof(addr));
    ipaddress_to_sceaddr(pAddress, &addr);
    iResult = VIUSocketSend(pSocket, pData, iSize > 1984 ? 1984 : iSize, 0, &addr,
                            pAddress->m_usPort);
    if (iResult != iSize)
    {
        iResult = uerror_get_net_error(iResult);
        pSocket->m_ucFlags &= ~2;
    }
    return iResult;
}

int usocket_sendto_addr(usocket_t *pSocket, const ipaddress_t *pAddress,
                        const void *pData, int iSize, int iDrop, int iDelay,
                        int iJitter)
{
    const void *pSend = pData;
    int iWait;
    int iLoss;

    // The drop path returns the COMPARISON, not a literal 1: written
    // `if (... < iDrop) return 1;` gcc has to materialise the constant and the
    // branch carries a `li v0,1` the target does not have.
    iLoss = (int)(rand() * 100.0 / 2147483648.0) < iDrop;
    if (iLoss)
        return iLoss;

    iWait = iDelay + (int)((float)iJitter * (float)rand() / 2147483648.0);
    if (iWait != 0)
    {
        char *pBuffer = (char *)__builtin_alloca(iSize + 8);
        uns16 usPort = BSWAP16(pAddress->m_usPort);
        int iTicks;

        pSend = pBuffer;
        memcpy(pBuffer, pData, iSize);
        memcpy(pBuffer + iSize, pAddress->m_ucIp, 4);
        iSize += 4;
        iTicks = iWait >> 5;
        if (iTicks > 255)
            iTicks = 255;
        memcpy(pBuffer + iSize, &usPort, 2);
        iSize += 2;
        pBuffer[iSize] = (char)iTicks;
        iSize += 1;
        pBuffer[iSize] = 0;
        iSize += 1;
        pAddress = &forward;
    }
    return usocket_sendto_internal(pSocket, pAddress, pSend, iSize);
}

int usocket_sendto(usocket_t *pSocket, const void *pData, int iSize, int iDrop,
                   int iDelay, int iJitter)
{
    ipaddress_t addr;

    ipaddress_read(&addr, pData);
    return usocket_sendto_addr(pSocket, &addr, (const char *)pData + 6, iSize - 6,
                               iDrop, iDelay, iJitter);
}

// -------------------------------------------------------------------------
// The receiving half of the same pair.  Both are file statics -- the symbol
// map has no name for either -- so `usocket_destroy` and `usocket_sendto`,
// the named symbols in front of them, acquire their spans and SPAN_CAP
// shrinks those back.
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// -------------------------------------------------------------------------
static int usocket_recvfrom_internal(usocket_t *pSocket, ipaddress_t *pAddress,
                                     void *pData, int iSize, int *piRead)
{
    sceInetAddress addr;
    int iResult;

    VASSERT(pSocket->m_ucType == USOCKET_TYPE_ICMP || pSocket->m_ucType == USOCKET_TYPE_UDP);

    if (pSocket->m_ucType == USOCKET_TYPE_ICMP)
    {
        int iFlags = 0;
        int iPort;

        iResult = VIUSocketRecv(pSocket, pData, iSize > 1984 ? 1984 : iSize,
                                &iFlags, &addr, &iPort);
        if (iResult < 0)
            goto error;
        memset(pAddress, 0, sizeof(*pAddress));
        *piRead = iResult;
    }
    else
    {
        int iFlags = 0;
        int iPort;

        memset(&addr, 0, sizeof(addr));
        iResult = VIUSocketRecv(pSocket, pData, iSize > 1984 ? 1984 : iSize,
                                &iFlags, &addr, &iPort);
        if (iResult < 0)
            goto error;
        sceaddr_to_ipaddress(&addr, pAddress);
        if (pSocket->m_ucType == USOCKET_TYPE_UDP)
            pAddress->m_usPort = (uns16)iPort;
        else if (pSocket->m_ucType == USOCKET_TYPE_ICMP)
            pAddress->m_usPort = 0;
        *piRead = iResult;
    }
    iResult = 0;
    if (iResult)
        pSocket->m_ucFlags &= ~1;
    return iResult;
error:
    iResult = uerror_remap_os_error_code(iResult);
    if (iResult)
        pSocket->m_ucFlags &= ~1;
    return iResult;
}

int usocket_recvfrom_addr(usocket_t *pSocket, ipaddress_t *pAddress, void *pData,
                          int iSize, int *piRead)
{
    int iResult = usocket_recvfrom_internal(pSocket, pAddress, pData, iSize, piRead);

    if (!iResult && *piRead >= 8 && memcmp(pAddress, &forward, 4) == 0 &&
        pAddress->m_usPort == forward.m_usPort)
    {
        uns16 usPort;

        *piRead -= 4;
        memcpy(&usPort, (char *)pData + *piRead, 2);
        pAddress->m_usPort = BSWAP16(usPort);
        *piRead -= 4;
        memcpy(pAddress->m_ucIp, (char *)pData + *piRead, 4);
    }
    return iResult;
}

// -------------------------------------------------------------------------
// The public receive entry points.  `usocket_recvfrom` is the one the DRDP
// route layer calls: it prepends the six-byte source address to the
// caller's buffer, which is why every size on this path moves by six.
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// -------------------------------------------------------------------------
int usocket_recvfrom(usocket_t *pSocket, void *pData, int iSize, int *piRead)
{
    ipaddress_t addr;
    int iResult;

    VASSERT(pSocket->m_ucType == USOCKET_TYPE_ICMP || pSocket->m_ucType == USOCKET_TYPE_UDP);
    iResult = usocket_recvfrom_addr(pSocket, &addr, (char *)pData + 6, iSize - 6,
                                    piRead);
    if (!iResult)
    {
        *piRead += 6;
        ipaddress_write(&addr, pData);
    }
    return iResult;
}

int usocket_select(usocket_t **ppSockets, int iCount, int iTimeout)
{
    return 1;
}

// -------------------------------------------------------------------------
// `accept` is a stub that ASSERTS and then does the work anyway -- the
// expression is `!"usocket_accept: not implemented"`, always false, and
// the assert HANGS, so nothing below it runs on a real console.
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// -------------------------------------------------------------------------
int usocket_accept(usocket_t *pSocket, usocket_t **ppAccepted)
{
    usocket_t *pAccepted = 0;
    int iResult;

    VASSERT(!"usocket_accept: not implemented");
    iResult = usocket_create_internal(&pAccepted, USOCKET_TYPE_TCP_LISTEN, -1);
    if (!iResult)
    {
        *ppAccepted = pAccepted;
        pAccepted = 0;
    }
    if (pAccepted)
        usocket_destroy(pAccepted);
    if (iResult)
        pSocket->m_ucFlags &= ~1;
    return iResult;
}

int usocket_read_ready(usocket_t *pSocket)
{
    return pSocket->m_ucFlags & 1;
}

int usocket_mode_listen(usocket_t *pSocket)
{
    return pSocket->m_ucListen == 1;
}

int usocket_get_fd(usocket_t *pSocket)
{
    return pSocket->m_iFd;
}

// -------------------------------------------------------------------------
// The stream receive, and the only entry point that calls sceInet from the
// caller's own thread rather than handing the work to VIUSocketEx.
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
// -------------------------------------------------------------------------
int usocket_recv(usocket_t *pSocket, void *pData, int iSize, int *piRead)
{
    int iFlags;
    int iResult;

    VASSERT(pSocket->m_ucType == USOCKET_TYPE_TCP_CONNECT || pSocket->m_ucType == USOCKET_TYPE_TCP_LISTEN);
    iFlags = 0;
    iResult = sceInetRecv(cd, net_buf, pSocket->m_iFd, pData,
                          iSize > 1984 ? 1984 : iSize, &iFlags, 0);
    if (iResult < 0)
        iResult = uerror_remap_os_error_code(iResult);
    else
    {
        *piRead = iResult;
        iResult = 0;
    }
    if (iResult)
        pSocket->m_ucFlags &= ~1;
    return iResult;
}
