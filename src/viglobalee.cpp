// VIGlobalEE -- SUPPORT 0x003E3E00 .. 0x003E41F8 and 0x003E54E8 .. 0x003E5E88.
//
// The file's name is in its own .rodata: ClearVRAM's out-of-memory printf
// passes "C:/code-frontiers-gm/ee_support_dll/VIGlobalEE.cpp".  VIDictionary
// and its VIMap (vimap.cpp) sit between the two halves in the image.  (A
// string's %lo addend in the image is relative to the MODULE's .rodata base,
// 0x004E9880, not the TU's, so it does not date a TU boundary.)
//
// The platform odds and ends: the boot-time VRAM clear, the assert sink, the
// disc id read out of SYSTEM.CNF, thin wrappers over libdma / sifdev, and the
// client side of the IOP CD-streaming RPC (server 0x2114, function 0x80F0) --
// one 64-byte command block out, one 64-byte reply in, completion signalled
// by a SIF command handler that clears the busy flag.
#include "vi.h"

extern "C" void *memset(void *, int, int);
extern "C" char *strcpy(char *, const char *);
extern "C" char *strncpy(char *, const char *, int);
extern "C" char *strchr(const char *, int);
extern "C" int   strcmp(const char *, const char *);
extern "C" int   printf(const char *, ...);
extern "C" void  exit(int);
extern "C" void *memalign(int, int);
extern "C" void  free(void *);

struct _reent_head { int _errno; void *_stdin; void *_stdout; void *_stderr; };
extern "C" _reent_head *_impure_ptr;
extern "C" int fprintf(void *, const char *, ...);

struct mallinfo_t
{
    int arena, ordblks, smblks, hblks, hblkhd;
    int usmblks, fsmblks, uordblks, fordblks, keepcost;
};
extern "C" mallinfo_t mallinfo();

// libgraph / libpkt / libdma
struct sceDmaChan;
struct sceGifPacket
{
    unsigned int *pCurrent;
    unsigned int *pBase;
    unsigned int *pGifTag;
    unsigned int  pad;
};
extern "C" int  sceGsSyncV(int mode);
extern "C" int  sceGsSyncPath(int mode, unsigned short timeout);
extern "C" int  sceGszbufaddr(int psm, int w, int h);
extern "C" void sceGifPkInit(sceGifPacket *p, void *buf);
extern "C" void sceGifPkReset(sceGifPacket *p);
extern "C" void sceGifPkAddGsData(sceGifPacket *p, unsigned long d);
extern "C" void sceGifPkTerminate(sceGifPacket *p);
extern "C" int  sceGifPkSize(sceGifPacket *p);
extern "C" sceDmaChan *sceDmaGetChan(int id);
extern "C" int  sceDmaReset(int mode);
extern "C" void sceDmaSend(sceDmaChan *d, void *tag);
extern "C" void sceDmaSendN(sceDmaChan *d, void *addr, int size);
extern "C" int  sceDmaSync(sceDmaChan *d, int mode, int timeout);
extern "C" void FlushCache(int mode);
extern "C" int  DIntr();
extern "C" int  EIntr();
extern "C" int  RotateThreadReadyQueue(int prio);

// sifdev / sifrpc / sifcmd
struct sceSifRpcData { void *paddr; unsigned pid; int tid; unsigned mode; };
struct sceSifClientData
{
    sceSifRpcData rpcd;
    unsigned      command;
    void         *buff;
    void         *cbuff;
    void        (*func)(void *);
    void         *para;
    void         *serve;
};
struct sceSifCmdData { void (*func)(void *, void *); void *data; };
extern "C" int  sceSifLoadModule(const char *path, int argLen, const char *args);
extern "C" int  sceSifStopModule(int id, int argLen, const char *args, int *result);
extern "C" int  sceSifUnloadModule(int id);
extern "C" sceSifCmdData *sceSifSetCmdBuffer(sceSifCmdData *b, int n);
extern "C" void sceSifAddCmdHandler(int pos, void (*f)(void *, void *), void *data);
extern "C" int  sceSifBindRpc(sceSifClientData *c, unsigned rpc, unsigned mode);
extern "C" int  sceSifCheckStatRpc(sceSifRpcData *c);
extern "C" int  sceSifCallRpc(sceSifClientData *c, unsigned fno, unsigned mode,
                              void *send, int ssize, void *recv, int rsize,
                              void (*end)(void *), void *endParam);

extern "C" void _VIAssert(const char *expr, const char *file, int line);

const char *VIGetCurrentDiscId();
static int  CdStreamCall(int nowait);
static void CdStreamDone(void *pkt, void *data);

sceDmaChan *dmaGIF = 0;
const char *__VIAssertf_file = 0;
int         __VIAssertf_line = 0;

#define VIMEMCHECK(p) \
    if (!(p)) { printf("ERROR: Not enough memory! line:(%d), file:(%s)\n", __LINE__, __FILE__); exit(0); }

// GS privileged PMODE: 0xA0 switches both read circuits off for the clear.
#define GS_PMODE  (*(volatile unsigned long *)0x12000000)

// Fill the whole 1024x1024 of VRAM with one colour and ZBUF masked: one
// sprite through PATH3, built in a 240-byte GIF packet.
void ClearVRAM(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    sceGifPacket pk;
    void *buf;

    GS_PMODE = 0xA0;
    sceGsSyncV(0);
    sceGsSyncV(0);
    sceGsSyncV(0);
    sceGsSyncV(0);
    sceGsSyncV(0);

    dmaGIF = sceDmaGetChan(2);

    buf = memalign(64, 240);
    // The line is an immediate (49), so the original's numbering is restored
    // here rather than padded for.
#line 49 "C:/code-frontiers-gm/ee_support_dll/VIGlobalEE.cpp"
    VIMEMCHECK(buf);
#line 126 "viglobalee.cpp"

    sceGifPkInit(&pk, buf);
    sceGifPkReset(&pk);
    sceGifPkAddGsData(&pk, 0x100000000000800AUL);         // NLOOP 10, EOP, NREG 1
    sceGifPkAddGsData(&pk, 14);                           // A+D
    sceGifPkAddGsData(&pk, 1);
    sceGifPkAddGsData(&pk, 0x1A);                         // PRMODECONT
    sceGifPkAddGsData(&pk, 0x100000);
    sceGifPkAddGsData(&pk, 0x4C);                         // FRAME_1, FBW 16
    sceGifPkAddGsData(&pk, sceGszbufaddr(0, 640, 224) | 0x31000000 | (1UL << 32));
    sceGifPkAddGsData(&pk, 0x4E);                         // ZBUF_1, PSMZ24, ZMSK
    sceGifPkAddGsData(&pk, 0);
    sceGifPkAddGsData(&pk, 0x18);                         // XYOFFSET_1
    sceGifPkAddGsData(&pk, 0x0400000004000000UL);
    sceGifPkAddGsData(&pk, 0x40);                         // SCISSOR_1
    sceGifPkAddGsData(&pk, 0x30000);
    sceGifPkAddGsData(&pk, 0x47);                         // TEST_1
    sceGifPkAddGsData(&pk, 6);
    sceGifPkAddGsData(&pk, 0);                            // PRIM sprite
    sceGifPkAddGsData(&pk, (unsigned long)r | ((unsigned long)g << 8)
                         | ((unsigned long)b << 16) | ((unsigned long)a << 24)
                         | (0x3F800000UL << 32));
    sceGifPkAddGsData(&pk, 1);                            // RGBAQ, Q 1.0
    sceGifPkAddGsData(&pk, 0);
    sceGifPkAddGsData(&pk, 4);                            // XYZ2 (0, 0)
    sceGifPkAddGsData(&pk, 0x40004000);
    sceGifPkAddGsData(&pk, 4);                            // XYZ2 (1024, 1024)
    sceGifPkTerminate(&pk);

    FlushCache(0);
    sceDmaSendN(dmaGIF, (void *)((unsigned int)pk.pBase & 0x0FFFFFFF), sceGifPkSize(&pk));
    sceGsSyncPath(0, 0);
    free(buf);

    GS_PMODE = 0xA1;
    sceGsSyncV(0);
    sceGsSyncV(0);
    sceGsSyncV(0);
    sceGsSyncV(0);
    sceGsSyncV(0);
}

// Never returns: the formatted assert parks the EE in a spin.
extern "C" void _VIAssert(const char *expr, const char *file, int line)
{
    fprintf(_impure_ptr->_stderr, "Assertion failed: %s, file %s, line %d\n", expr, file, line);
    for (;;)
        ;
}

typedef char *va_list;
#define va_start(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)

// The VIASSERTF macro parks file and line in the two globals and calls this.
extern "C" void _VIAssertf(int cond, const char *fmt, ...)
{
    if (!cond)
    {
        char buf[4096];
        va_list ap;
        va_start(ap, fmt);
        VIString::vsnprintf(buf, 4096, fmt, ap);
        _VIAssert(buf, __VIAssertf_file, __VIAssertf_line);
    }
}

// File then line, in argument order: the high half born first takes $v0 only
// because VIString::vsnprintf is VOID -- after an `int` call the scheduler
// extends the first pseudo back over the call's $v0 and it gets $v1 (2/28 and
// 2/27 here, with either read order, until vi.h said `void`).
extern "C" void _VIAlert(const char *fmt, ...)
{
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    VIString::vsnprintf(buf, 4096, fmt, ap);
    _VIAssert(buf, __VIAssertf_file, __VIAssertf_line);
}

// ------------------------------------------------------------- the disc id --
// "SLUS_207.44" off the BOOT2 line of SYSTEM.CNF, reshaped to "SLUS-20744".
static char sBootDiscId[16] = "";
static char sDiscId[16];

int VIInitDiscId()
{
    memset(sBootDiscId, 0, 16);
    strcpy(sBootDiscId, VIGetCurrentDiscId());
    return sBootDiscId[0] ? 0 : -1;
}

const char *VIGetBootDiscId()
{
    return sBootDiscId;
}

const char *VIGetCurrentDiscId()
{
    VIFile f;
    char   buf[256];
    char  *p;

    memset(sDiscId, 0, 16);
    if (f.Open("system.cnf", 0x21) < 0)
        return sDiscId;
    if (f.Seek(0, VIFile::Seek_End) < 0)
    {
        f.Close();
        return sDiscId;
    }
    int n = f.Tell();
    f.Seek(0, VIFile::Seek_Set);
    if (f.Read(buf, n) < 0)
    {
        f.Close();
        return sDiscId;
    }
    f.Close();
    p = strchr(buf, ';');
    if (!p)
        return sDiscId;
    strncpy(sDiscId, p - 11, 11);
    sDiscId[4]  = '-';
    sDiscId[8]  = sDiscId[9];
    sDiscId[9]  = sDiscId[10];
    sDiscId[10] = 0;
    return sDiscId;
}

// 0 while no boot id was taken, -1 when no disc can be read now, else 1 when
// the disc in the drive is not the one the game booted from.
int VIHasDiscIdChanged()
{
    char cur[16];

    if (!sBootDiscId[0])
        return 0;
    strcpy(cur, VIGetCurrentDiscId());
    if (!cur[0])
        return -1;
    return strcmp(cur, sBootDiscId) != 0;
}

// --------------------------------------------------------------- debugging --
// Shipped empty: the dump loops survive, their bodies do not.
void VIEEDebugVU0Memory(int first, int last)
{
    for (int i = first; i <= last; i++)
        ;
}

void VIEEDebugVU1Memory(int first, int last)
{
    for (int i = first; i <= last; i++)
        ;
}

// --------------------------------------------------------------------- DMA --
#define D_STAT  (*(volatile unsigned int *)0x1000E010)
#define D_PCR   (*(volatile unsigned int *)0x1000E020)

// Starts at 1 (the image's .data): the first channel-1 sync does not wait.
static int sDMASyncSkip = 1;

void VIEEDMAReset()
{
    sceDmaReset(1);
    D_PCR |= 2;
}

// Channel 1 (VIF1) clears its CIS bit first so the COP0 condition that
// VIEEDMASync spins on is armed by this transfer and not a stale one.
void VIEEDMASend(int chan, void *tag)
{
    sceDmaChan *c = sceDmaGetChan(chan);
    if (chan == 1)
    {
        D_STAT = 2;
        sceDmaSend(c, tag);
    }
    else
        sceDmaSend(c, tag);
}

void VIEEDMASendN(int chan, void *addr, int qwc)
{
    sceDmaChan *c = sceDmaGetChan(chan);
    if (chan == 1)
    {
        D_STAT = 2;
        sceDmaSendN(c, addr, qwc);
    }
    else
        sceDmaSendN(c, addr, qwc);
}

void VIEEDMASync(int chan)
{
    if (chan == 1)
    {
        if (sDMASyncSkip == 0)
            __asm__ __volatile__ ("sync.l\n1:\n\tbc0f 1b\n\tnop\n");
        sDMASyncSkip = 0;
    }
    else
        sceDmaSync(sceDmaGetChan(chan), 0, 0);
}

int VIEEDMAIsBusy()
{
    volatile int busy;
    __asm__ __volatile__ ("li $7, 1\n\tsync.l\n\tbc0f 1f\n\tnop\n\taddi $7, $7, -1\n1:\n\tsw $7, %0\n"
                          : "=m" (busy) : : "$7");
    return busy;
}

// -------------------------------------------------------------- IRX loading --
int VIEELoadIrx(const char *name, int argLen, const char *args)
{
    char path[256];
    int  id;
    VIString::snprintf(path, 256, "cdrom0:\\MODULES\\%s;1", name);
    while ((id = sceSifLoadModule(path, argLen, args)) < 0)
        ;
    return id;
}

int VIEELoadIrx2(const char *name, int argLen, const char *args)
{
    char path[256];
    int  id;
    VIString::snprintf(path, 256, "cdrom0:\\MODULES2\\%s;1", name);
    while ((id = sceSifLoadModule(path, argLen, args)) < 0)
        ;
    return id;
}

int VIEEUnloadIrx(int id)
{
    int result;
    sceSifStopModule(id, 0, 0, &result);
    return sceSifUnloadModule(id);
}

// Free heap in KB.
int VIEEKBFree()
{
    mallinfo_t mi = mallinfo();
    return mi.fordblks >> 10;
}

// ------------------------------------------------------------ CD streaming --
// The command block's word 0 is the opcode: 1 init, 3 search, 4 read,
// 5 disk ready, 6 cancel.  A reply's word 0 is the result, +4/+8 the search's
// lsn and size.
static sceSifCmdData    sCmdBuffer[16];
static sceSifClientData sCdClient;
static int              sCdSend[16];
static int              sCdRecv[16];
static int              sCdBusy;
static int              sCdStatus;
void                   *last_buffer = 0;

int VICdStreamInit()
{
    DIntr();
    sceSifSetCmdBuffer(sCmdBuffer, 16);
    sceSifAddCmdHandler(0, CdStreamDone, 0);
    EIntr();

    for (;;)
    {
        if (sceSifBindRpc(&sCdClient, 0x2114, 0) < 0)
        {
            printf("VICdStreamInit() : error( sceSifBindRpc )\n");
            return -1;
        }
        int i = 10000;
        while (i--)
            ;
        if (sCdClient.serve != 0)
            break;
    }

    // The command word goes in last, as in every other caller, through a
    // pointer taken FIRST: the expander emits a global's lui when its name
    // is expanded and a constant when the value is, and the target's
    // `lui sCdSend` leads the five luis while its `li 1` trails them.  sched1
    // and sched2 both break this all-equal tie on insn order, so only an
    // address expanded before the other stores and a value after them gives
    // that (`sCdSend[0] = 1` first: li second; last: lui last).  The value
    // cannot come from a variable set earlier: then update_equiv_regs moves
    // the li in after sched1, `li a0` falls behind `sw sCdRecv` (weight 0
    // beats 1), and sCdRecv's lui takes a0.
    int *send = sCdSend;
    sCdRecv[0] = 0;
    sCdBusy = 0;
    sCdStatus = 0;
    last_buffer = 0;
    send[0] = 1;
    CdStreamCall(0);
    return 0;
}

void VICdStreamFree()
{
}

int VICdSearchFile(const char *name, unsigned *lsn, unsigned *size)
{
    if (sCdBusy)
    {
        sCdStatus = 2;
        return -1;
    }
    sCdSend[0] = 3;
    strcpy((char *)&sCdSend[1], name);
    if (CdStreamCall(0) < 0)
    {
        sCdStatus = 1;
        return -1;
    }
    sCdStatus = sCdRecv[0];
    if (sCdRecv[0])
        return -1;
    *lsn = sCdRecv[1];
    *size = sCdRecv[2];
    return 0;
}

int VICdStreamRead(unsigned lsn, unsigned sectors, void *buf, int mode)
{
    if (sCdBusy)
    {
        sCdStatus = 2;
        return -1;
    }
    sCdSend[0] = 4;
    sCdSend[1] = lsn;
    sCdSend[2] = sectors;
    sCdSend[3] = mode;
    sCdSend[4] = (int)buf;
    last_buffer = buf;
    if (CdStreamCall(0) < 0)
    {
        sCdStatus = 1;
        return -1;
    }
    sCdBusy = 1;
    sCdStatus = 0;
    return 0;
}

int VICdStreamCancel()
{
    int busy = sCdBusy;
    if (busy != 1)
    {
        sCdStatus = 0;
        return -1;
    }
    sCdSend[0] = 6;
    if (CdStreamCall(0) < 0)
    {
        sCdStatus = busy;
        return -1;
    }
    sCdBusy = 2;
    sCdStatus = 0;
    do
        sceGsSyncV(0);
    while (sCdBusy);
    return 0;
}

int VICdStreamDiskReady()
{
    if (sCdBusy)
    {
        sCdStatus = 2;
        return -1;
    }
    sCdSend[0] = 5;
    if (CdStreamCall(0) < 0)
    {
        sCdStatus = 1;
        return -1;
    }
    sCdStatus = sCdRecv[0];
    return sCdRecv[0] ? -1 : 0;
}

int VICdStreamSync(int nowait)
{
    if (!nowait)
    {
        for (;;)
        {
            if (!sCdBusy)
                return 0;
            RotateThreadReadyQueue(1);
        }
    }
    return sCdBusy != 0;
}

int VICdStreamStatus()
{
    return sCdStatus;
}

static int CdStreamCall(int nowait)
{
    if (sceSifCheckStatRpc(&sCdClient.rpcd))
        return -1;
    sceSifCallRpc(&sCdClient, 0x80F0, nowait != 0, sCdSend, 64, sCdRecv, 64, 0, 0);
    return 0;
}

static void CdStreamDone(void *pkt, void *data)
{
    // The packet is read before either store: a local, not an inline read.
    int status = ((int *)pkt)[3];
    sCdBusy = 0;
    sCdStatus = status;
}
