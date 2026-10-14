// unet.cpp -- SUPPORT 0x004acf58..0x004ad3f0.  The bring-up.
//
// This is the one PS2-specific file in the DRDP library, and it is where the
// port shows: the whole of "start the network" is an EE thread that calls
// `sceLibnetInitialize` over a memalign'd 64 KB, registers a handler, blocks in
// `sceLibnetWaitGetAddress` until DHCP answers, and then caches the local
// address as a STRING.  Nothing here keeps an `ipaddress_t`: `unet_gethostname`
// hands back the text and `unet_namelookup` resolves it again.
//
// The three failure paths do not return an error -- they print and HANG, the
// same `for (;;) ;` `_uassert` ends in.
#include "drdp.h"

extern "C" unsigned strlen(const char *);
extern "C" char    *strcpy(char *, const char *);
extern "C" void    *memset(void *, int, unsigned);
extern "C" void    *memalign(unsigned align, unsigned size);
extern "C" int      printf(const char *, ...);
extern "C" int      pf(const char *, ...);

// ps2sdk's thread block: status +0, func +4, stack +8, stack_size +12,
// gp_reg +16, initial_priority +20.  The startup fills five of the nine.
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

extern "C" int  CreateThread(ee_thread_t *thread);
extern "C" int  StartThread(int thid, void *arg);
extern "C" int  ChangeThreadPriority(int thid, int priority);
extern "C" int  TerminateThread(int thid);
extern "C" int  DeleteThread(int thid);
extern "C" int  ReferThreadStatus(int thid, void *info);
extern "C" void sceSifMInitRpc(int mode);

// SCE's libnet.  `cd` is the connection descriptor every call is threaded
// through and `net_buf` the work area it is handed; both are GLOBAL -- they are
// the only two names in this file the symbol map knows.
extern "C" int sceLibnetInitialize(void *cd, int a, int b, int c);
extern "C" int sceLibnetTerminate(void *cd);
extern "C" int sceLibnetRegisterHandler(void *cd, void *buf);
extern "C" int sceLibnetWaitGetAddress(void *cd, void *buf, sceInetAddress *addr,
                                       int naddr, sceInetAddress *dns, int ndns);
extern "C" int sceInetAddress2String(void *cd, void *buf, char *string, int size,
                                     const sceInetAddress *addr);
extern "C" int sceInetAddress2Name(void *cd, void *buf, int flags, char *name, int size,
                                   const sceInetAddress *addr, int a, int b);
extern "C" int sceInetName2Address(void *cd, void *buf, int flags, sceInetAddress *addr,
                                   const char *name, int a, int b);

extern "C" int _gp;

void VIInitUSocketExApi();
void VIFreeUSocketExApi();

// The gap to the next named data symbol says `cd` is 64 bytes.
char *net_buf;
char  cd[64];

static int  g_iLibnetInitialized;
static int  g_iStarted;
static int  g_iThreadId;
char hostname[128];
static char g_aStack[8192 + 15];

void unet_shutdown()
{
    if (g_iLibnetInitialized)
    {
        sceLibnetTerminate(cd);
        g_iLibnetInitialized = 0;
    }
    VIFreeUSocketExApi();
}

void unet_startup_thread(void *pArg)
{
    sceInetAddress addr;
    char           szAddress[128];
    sceInetAddress dns;

    sceSifMInitRpc(0);
    VIInitUSocketExApi();
    sceLibnetInitialize(cd, 2048, 8192, 32);
    g_iLibnetInitialized = 1;

    net_buf = (char *)memalign(64, 0x10200);
    if (!net_buf)
    {
        pf("malloc failed.\n");
        for (;;)
            ;
    }

    if (sceLibnetRegisterHandler(cd, net_buf) < 0)
    {
        pf("reg_handler() failed.\n");
        for (;;)
            ;
    }

    memset(&addr, 0, sizeof(addr));
    if (sceLibnetWaitGetAddress(cd, net_buf, &dns, 1, &addr, 2) < 0)
    {
        printf("wait_get_addr() failed.\n");
        for (;;)
            ;
    }

    if (sceInetAddress2String(cd, net_buf, szAddress, 128, &addr) != 0)
    {
        printf("invalid addr\n");
        for (;;)
            ;
    }

    if (sceInetAddress2Name(cd, net_buf, 0, hostname, 128, &addr, 0, 0) != 0)
    {
        strcpy(hostname, szAddress);
        printf("My address: %s\n", szAddress);
    }
    else
    {
        printf("My address: %s (%s)\n", hostname, szAddress);
    }
}

void unet_startup_async()
{
    ee_thread_t th;

    if (!g_iStarted)
    {
        unsigned uiStack = (unsigned)g_aStack;

        th.stack_size = 8192;
        th.gp_reg = &_gp;
        th.func = (void *)unet_startup_thread;
        th.stack = (void *)((uiStack + 15) / 16 * 16);
        th.initial_priority = 10;
        g_iThreadId = CreateThread(&th);
        if (g_iThreadId != -1)
        {
            StartThread(g_iThreadId, 0);
            ChangeThreadPriority(g_iThreadId, 1);
        }
    }
}

// The `1` the not-ready path returns is the SAME `1` that goes into g_iStarted:
// gcc materialises it once, in the branch's delay slot, and uses it for both.
// Reading it as `return iStatus` costs two moves and does not match.
int unet_startup_async_status()
{
    if (g_iThreadId != -1)
    {
        if (ReferThreadStatus(g_iThreadId, 0) != 16)
            return 1;
        g_iStarted = 1;
        TerminateThread(g_iThreadId);
        DeleteThread(g_iThreadId);
        g_iThreadId = -1;
    }
    return 0;
}

void unet_startup()
{
    if (!g_iStarted)
    {
        unet_startup_thread(0);
        g_iStarted = 1;
    }
}

int unet_namelookup(const char *pName, ipaddress_t *pAddress)
{
    sceInetAddress addr;

    unet_startup();
    if (sceInetName2Address(cd, net_buf, 0, &addr, pName, 0, 0) >= 0)
    {
        sceaddr_to_ipaddress(&addr, pAddress);
        pAddress->m_usPort = 0;
        return 0;
    }
    return -5;
}

int unet_gethostname(char *pName, int iSize)
{
    unet_startup();
    if ((unsigned)iSize < strlen(hostname) + 1)
        return -1;
    strcpy(pName, hostname);
    return 0;
}

int unet_get_local_address(ipaddress_t *pAddress)
{
    char szName[256];
    int  iResult;

    iResult = unet_gethostname(szName, sizeof(szName));
    if (iResult != 0)
        return iResult;
    return unet_namelookup(szName, pAddress);
}
