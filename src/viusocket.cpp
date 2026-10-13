// viusocket.cpp -- SUPPORT 0x004ae088..0x004aeb70, the PS2 port's datagram path.
//
// `usocket.cpp` does not call sceInet from the caller's thread for a datagram:
// it hands the work to two EE threads that live in the back of the socket's own
// allocation (`usocket_ex_t`).  The send thread sleeps until `VIUSocketSend`
// wakes it, drains the send queue and retries each packet five times; the
// receive thread polls sceInet with a 100 ms timeout and queues what arrives.
// Every packet is a fixed 2,084-byte record out of ONE global pool of sixteen,
// so the whole port never allocates per datagram.
//
// A failure is STICKY: either thread parks the error and then the receive
// thread goes to sleep for good, and every later send or receive reports it.
#include "drdp.h"

extern "C" void *memalign(unsigned align, unsigned size);
extern "C" void *memset(void *, int, unsigned);
extern "C" int   printf(const char *, ...);
extern "C" int   sceGsSyncV(int mode);

// ps2sdk's thread and semaphore blocks.  The thread is 48 bytes here: the
// semaphore beside it lands at sp+48.
struct ee_thread_t
{
    int   status;
    void *func;
    void *stack;
    int   stack_size;
    void *gp_reg;
    int   initial_priority;
    int   current_priority;
    unsigned attr;
    unsigned option;
};

struct ee_sema_t
{
    int count;
    int max_count;
    int init_count;
    int wait_threads;
    unsigned attr;
    unsigned option;
};

extern "C" int  CreateThread(ee_thread_t *thread);
extern "C" int  StartThread(int thid, void *arg);
extern "C" int  ChangeThreadPriority(int thid, int priority);
extern "C" int  TerminateThread(int thid);
extern "C" int  DeleteThread(int thid);
extern "C" int  ReferThreadStatus(int thid, void *info);
extern "C" int  RotateThreadReadyQueue(int priority);
extern "C" int  SleepThread();
extern "C" int  WakeupThread(int thid);
extern "C" int  CreateSema(ee_sema_t *sema);
extern "C" int  DeleteSema(int semid);
extern "C" int  WaitSema(int semid);
extern "C" int  SignalSema(int semid);
extern "C" int  _gp;

extern "C" int sceLibnetInitialize(sceSifMClientData *cd, int buffersize, int stacksize,
                                   int priority);
extern "C" int sceLibnetTerminate(sceSifMClientData *cd);
extern "C" int sceInetRecv(sceSifMClientData *cd, uns32 *net_buf, int cid, void *ptr,
                           int count, int *pflags, int ms);
extern "C" int sceInetRecvFrom(sceSifMClientData *cd, uns32 *net_buf, int cid, void *ptr,
                               int count, int *pflags, sceInetAddress *addr, int *pport,
                               int ms);
extern "C" int sceInetSendTo(sceSifMClientData *cd, uns32 *net_buf, int cid, const void *ptr,
                             int count, int *pflags, sceInetAddress *addr, int port, int ms);

// One datagram.  1,984 is the most either thread moves; the buffer is 2,048.
struct usocket_packet_t
{
    lnode          m_node;          // +0x000
    char           m_data[2048];    // +0x008
    int            m_iSize;         // +0x808
    sceInetAddress m_addr;          // +0x80C  byte-aligned, so copied ldl/ldr
    int            m_iPort;         // +0x81C
    int            m_iFlags;        // +0x820
};                                  //  0x824

static usocket_packet_t *s_pPackets;
static llist s_free;
static llist s_unused;

void VIUSocketRecvThread(void *pArg)
{
    usocket_t *pSocket = (usocket_t *)pArg;
    usocket_ex_t *pEx = &pSocket->m_ex;
    char *pBuffer = (char *)memalign(64, 2048);

    while (!pEx->m_iQuit)
    {
        usocket_packet_t *pPacket = 0;
        int iRead;

        if (pSocket->m_ucType == 1)
        {
            int iFlags = 0;

            iRead = sceInetRecv(&pEx->m_recvCd, pEx->m_pRecvBuf, pSocket->m_iFd, pBuffer, 1984,
                                &iFlags, 100);
            if (iRead > 1984)
            {
                printf("RECV DATA > 1984 (%d)!!!\n", iRead);
                sceGsSyncV(0);
                sceGsSyncV(0);
            }
            if (iRead < 0)
            {
                if (iRead != -500)
                {
                    pEx->m_iRecvError = iRead;
                    SleepThread();
                }
            }
            else if (iRead > 0)
            {
                WaitSema(pEx->m_iRecvSema);
                pPacket = (usocket_packet_t *)ll_rem_head(&s_free);
                if (pPacket)
                {
                    memcpy(pPacket->m_data, pBuffer, iRead);
                    pPacket->m_iSize = iRead;
                    memset(&pPacket->m_addr, 0, sizeof(pPacket->m_addr));
                    pPacket->m_iPort = 0;
                    pPacket->m_iFlags = iFlags;
                }
                SignalSema(pEx->m_iRecvSema);
            }
        }
        else if (pSocket->m_ucType == 0)
        {
            sceInetAddress addr;
            int iFlags = 0;
            int iPort;

            memset(&addr, 0, sizeof(addr));
            iRead = sceInetRecvFrom(&pEx->m_recvCd, pEx->m_pRecvBuf, pSocket->m_iFd, pBuffer,
                                    1984, &iFlags, &addr, &iPort, 100);
            if (iRead > 1984)
            {
                printf("RECVFROM DATA > 1984 (%d)!!!\n", iRead);
                sceGsSyncV(0);
                sceGsSyncV(0);
            }
            if (iRead < 0)
            {
                if (iRead != -500)
                {
                    pEx->m_iRecvError = iRead;
                    SleepThread();
                }
            }
            else if (iRead > 0)
            {
                WaitSema(pEx->m_iRecvSema);
                pPacket = (usocket_packet_t *)ll_rem_head(&s_free);
                if (pPacket)
                {
                    memcpy(pPacket->m_data, pBuffer, iRead);
                    pPacket->m_iSize = iRead;
                    pPacket->m_addr = addr;
                    pPacket->m_iPort = iPort;
                    pPacket->m_iFlags = iFlags;
                }
                SignalSema(pEx->m_iRecvSema);
            }
        }
        if (pPacket)
        {
            WaitSema(pEx->m_iRecvSema);
            ll_add_tail(&pEx->m_recvList, &pPacket->m_node);
            SignalSema(pEx->m_iRecvSema);
        }
    }
    free(pBuffer);
}

// Sleeps until VIUSocketSend wakes it, then drains the whole queue.  A packet
// is tried five times; a negative result is parked and ends the drain.
void VIUSocketSendThread(void *pArg)
{
    usocket_t *pSocket = (usocket_t *)pArg;
    usocket_ex_t *pEx = &pSocket->m_ex;

    while (!pEx->m_iQuit)
    {
        usocket_packet_t *pPacket;

        SleepThread();
        for (;;)
        {
            int iRetry;
            int iResult;

            WaitSema(pEx->m_iSendSema);
            pPacket = (usocket_packet_t *)ll_rem_head(&pEx->m_sendList);
            SignalSema(pEx->m_iSendSema);
            if (!pPacket)
                break;
            iRetry = 5;
            while (iRetry)
            {
                iResult = sceInetSendTo(&pEx->m_sendCd, pEx->m_pSendBuf, pSocket->m_iFd,
                                        pPacket->m_data, pPacket->m_iSize, &pPacket->m_iFlags,
                                        &pPacket->m_addr, pPacket->m_iPort, 100);
                if (iResult == pPacket->m_iSize)
                    break;
                iRetry--;
            }
            WaitSema(pEx->m_iSendSema);
            ll_add_tail(&s_free, &pPacket->m_node);
            SignalSema(pEx->m_iSendSema);
            if (iResult < 0)
            {
                pEx->m_iSendError = iResult;
                break;
            }
        }
    }
}

// `iSize` is never read: the packet carries its own.  -500 is
// sceINETE_TIMEOUT, "nothing yet".
int VIUSocketRecv(usocket_t *pSocket, void *pData, int iSize, int *piFlags,
                  sceInetAddress *pFrom, int *piPort)
{
    usocket_ex_t *pEx = &pSocket->m_ex;
    int iResult = -500;
    usocket_packet_t *pPacket;

    if (pEx->m_iRecvError < 0)
    {
        printf("RECV ERROR\n");
        return pEx->m_iRecvError;
    }
    WaitSema(pEx->m_iRecvSema);
    pPacket = (usocket_packet_t *)ll_rem_head(&pEx->m_recvList);
    if (pPacket)
    {
        memcpy(pData, pPacket->m_data, pPacket->m_iSize);
        *piFlags = pPacket->m_iFlags;
        *pFrom = pPacket->m_addr;
        *piPort = pPacket->m_iPort;
        iResult = pPacket->m_iSize;
        ll_add_tail(&s_free, &pPacket->m_node);
    }
    SignalSema(pEx->m_iRecvSema);
    return iResult;
}

// The size is reported as sent the moment it is queued; a packet the pool
// cannot hold is dropped without a word.
int VIUSocketSend(usocket_t *pSocket, const void *pData, int iSize, int iFlags,
                  sceInetAddress *pTo, int iPort)
{
    usocket_ex_t *pEx = &pSocket->m_ex;
    usocket_packet_t *pPacket;

    if (iSize > 1984)
    {
        printf("SEND DATA > 1984 (%d)!!!\n", iSize);
        sceGsSyncV(0);
        sceGsSyncV(0);
    }
    if (pEx->m_iSendError < 0)
    {
        printf("SEND ERROR\n");
        return pEx->m_iSendError;
    }
    WaitSema(pEx->m_iSendSema);
    pPacket = (usocket_packet_t *)ll_rem_head(&s_free);
    if (pPacket)
    {
        memcpy(pPacket->m_data, pData, iSize);
        pPacket->m_iSize = iSize;
        pPacket->m_addr = *pTo;
        pPacket->m_iPort = iPort;
        pPacket->m_iFlags = iFlags;
        ll_add_tail(&pEx->m_sendList, &pPacket->m_node);
        WakeupThread(pEx->m_iSendThread);
    }
    SignalSema(pEx->m_iSendSema);
    return iSize;
}

// Sixteen packets for every socket in the process.  The second list is
// initialised and never used.
void VIInitUSocketExApi()
{
    int i;

    s_pPackets = (usocket_packet_t *)malloc(16 * sizeof(usocket_packet_t));
    ll_init(&s_free);
    ll_init(&s_unused);
    for (i = 0; i < 16; i++)
        ll_add_tail(&s_free, &s_pPackets[i].m_node);
}

void VIFreeUSocketExApi()
{
    free(s_pPackets);
}

int VIInitUSocketEx(usocket_t *pSocket)
{
    usocket_ex_t *pEx = &pSocket->m_ex;
    ee_thread_t thread;
    ee_sema_t sema;

    pEx->m_pSendBuf = 0;
    pEx->m_pRecvBuf = 0;
    pEx->m_pSendStack = 0;
    pEx->m_pRecvStack = 0;
    pEx->m_iSendThread = -1;
    pEx->m_iRecvThread = -1;
    pEx->m_iSendError = 0;
    pEx->m_iRecvError = 0;
    pEx->m_iQuit = 0;
    pEx->m_iSendSema = -1;
    pEx->m_iRecvSema = -1;
    ll_init(&pEx->m_sendList);
    ll_init(&pEx->m_recvList);
    if (sceLibnetInitialize(&pEx->m_sendCd, 2048, 2048, 32))
        return -1;
    if (sceLibnetInitialize(&pEx->m_recvCd, 2048, 2048, 32))
    {
        sceLibnetTerminate(&pEx->m_sendCd);
        return -1;
    }
    pEx->m_pSendBuf = (uns32 *)memalign(64, 2112);
    pEx->m_pRecvBuf = (uns32 *)memalign(64, 2112);
    if (!pEx->m_pSendBuf || !pEx->m_pRecvBuf)
        goto fail;
    sema.init_count = 1;
    sema.max_count = 1;
    sema.option = 0;
    pEx->m_iSendSema = CreateSema(&sema);
    sema.init_count = 1;
    sema.max_count = 1;
    sema.option = 0;
    pEx->m_iRecvSema = CreateSema(&sema);
    if (pEx->m_iSendSema == -1 || pEx->m_iRecvSema == -1)
        goto fail;
    pEx->m_pSendStack = memalign(16, 8192);
    pEx->m_pRecvStack = memalign(16, 8192);
    if (!pEx->m_pSendStack || !pEx->m_pRecvStack)
        goto fail;
    thread.stack_size = 8192;
    thread.gp_reg = &_gp;
    thread.func = (void *)VIUSocketSendThread;
    thread.stack = pEx->m_pSendStack;
    thread.initial_priority = 10;
    pEx->m_iSendThread = CreateThread(&thread);
    thread.stack_size = 8192;
    thread.gp_reg = &_gp;
    thread.func = (void *)VIUSocketRecvThread;
    thread.stack = pEx->m_pRecvStack;
    thread.initial_priority = 10;
    pEx->m_iRecvThread = CreateThread(&thread);
    if (pEx->m_iSendThread == -1 || pEx->m_iRecvThread == -1)
    {
fail:
        VIFreeUSocketEx(pSocket);
        return -1;
    }
    StartThread(pEx->m_iSendThread, pSocket);
    StartThread(pEx->m_iRecvThread, pSocket);
    ChangeThreadPriority(pEx->m_iSendThread, 1);
    ChangeThreadPriority(pEx->m_iRecvThread, 1);
    return 0;
}

// Spins -- waking, yielding and signalling both semaphores once a frame --
// until BOTH threads report DORMANT (16), and only then tears them down.  Any
// queued packet goes back to the pool.
int VIFreeUSocketEx(usocket_t *pSocket)
{
    usocket_ex_t *pEx = &pSocket->m_ex;
    lnode *pNode;

    printf("FREEING USOCKET_EX!!!!\n");
    pEx->m_iQuit = 1;
    do
    {
        WakeupThread(pEx->m_iSendThread);
        RotateThreadReadyQueue(1);
        SignalSema(pEx->m_iRecvSema);
        SignalSema(pEx->m_iSendSema);
        sceGsSyncV(0);
    } while (ReferThreadStatus(pEx->m_iSendThread, 0) != 16
             || ReferThreadStatus(pEx->m_iRecvThread, 0) != 16);
    printf("Send and receive threads DORMANT.\n");
    printf("chpri\n");
    ChangeThreadPriority(pEx->m_iSendThread, 10);
    ChangeThreadPriority(pEx->m_iRecvThread, 10);
    printf("term\n");
    if (pEx->m_iSendThread != -1)
    {
        TerminateThread(pEx->m_iSendThread);
        DeleteThread(pEx->m_iSendThread);
    }
    if (pEx->m_iRecvThread != -1)
    {
        TerminateThread(pEx->m_iRecvThread);
        DeleteThread(pEx->m_iRecvThread);
    }
    if (pEx->m_iSendSema != -1)
        DeleteSema(pEx->m_iSendSema);
    if (pEx->m_iRecvSema != -1)
        DeleteSema(pEx->m_iRecvSema);
    printf("free stack\n");
    if (pEx->m_pSendStack)
        free(pEx->m_pSendStack);
    if (pEx->m_pRecvStack)
        free(pEx->m_pRecvStack);
    printf("free buf & unbind\n");
    if (pEx->m_pSendBuf || pEx->m_pRecvBuf)
    {
        if (pEx->m_pSendBuf)
            free(pEx->m_pSendBuf);
        if (pEx->m_pRecvBuf)
            free(pEx->m_pRecvBuf);
        sceLibnetTerminate(&pEx->m_recvCd);
        sceLibnetTerminate(&pEx->m_sendCd);
    }
    while ((pNode = ll_rem_head(&pEx->m_sendList)) != 0)
        ll_add_tail(&s_free, pNode);
    while ((pNode = ll_rem_head(&pEx->m_recvList)) != 0)
        ll_add_tail(&s_free, pNode);
    memset(pEx, 0, sizeof(usocket_ex_t));
    pEx->m_iSendThread = -1;
    pEx->m_iRecvThread = -1;
    return 0;
}
