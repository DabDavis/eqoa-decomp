// The driver proper and its command handlers: SNDDRV 0x1578 .. 0x402C, one TU.
//
// The module's entry starts one thread, which registers SIF RPC server 0x2112
// and serves it forever.  Every call is a PACKET of commands (VISoundDevice's
// SoundCmd: a count, then {id, argc, args...} each) and is answered with the
// 512-byte status block below.  A second thread, woken by a 100 Hz hardware
// timer, steps the XM players.
#include "snddrv.h"
#include "xm.h"

// .sdata: the BGM's state, which bgm.cpp reaches as externs
int gBgmState = 0;
int gBgmHalf = 0;
int gBgmXferHalf = 0;
int gBgmStop = 0;
u32 gCdSector = 0;

int gXmTicks = 0;               // timer wakes, counted

// The eight XM players.  XmPlayer has a constructor and destructor, so this
// TU ends with the synthesized XmSlot ctor/dtor, the static initializer, and
// the out-of-line copies of xm.h's inline accessors (SNDDRV 0x3C3C .. 0x402C).
XmSlot gXmSlot[8];

struct SdTimer
{
    int thread;                 // the XM thread the timer wakes
    int timer;
    u32 clocks;                 // the period, 10 ms in bus clocks
};

struct SifRpcDataQueue { u8 m_raw[24]; };
struct SifRpcServerData { u8 m_raw[68]; };
struct iop_sys_clock { u32 lo, hi; };

extern "C" {
void sceSifInitRpc(int mode);
void sceSifSetRpcQueue(SifRpcDataQueue *q, int thread);
void sceSifRegisterRpc(SifRpcServerData *s, int id, void *(*func)(int, void *, int),
                       void *buf, void *cfunc, void *cbuf, SifRpcDataQueue *q);
void sceSifRpcLoop(SifRpcDataQueue *q);
int  USec2SysClock(u32 usec, iop_sys_clock *clock);
int  AllocHardTimer(int source, int size, int prescale);
int  FreeHardTimer(int timer);
int  SetTimerHandler(int timer, u32 compare, int (*handler)(void *), void *arg);
int  SetupHardTimer(int timer, int source, int mode, int prescale);
int  StartHardTimer(int timer);
int  StopHardTimer(int timer);
}

// .data (a C++ global with no initializer is .space where it stands)
SdTimer gSdTimer;               // 0x10134
u32     gSndStatus[128];        // 0x10140: the reply every call answers with

// .bss/.sbss: file statics, .lcomm at the end of the file
static int gSdJmp[12];                  // 0x11268, a jmp_buf
static u8  gIopBuf[0x10000];            // 0x11298: the EE DMAs ADPCM here for SdAdpcmTransfer
static u8  gXmPool[0x19000];            // 0x21298: the XM modules' region, carved up by the EE
static u8  gRpcBuf[0x810];              // 0x3A298: the RPC packet lands here
static int gXmThread;                   // .sbss 0x11264

int SdRpcThread();

extern "C" int _start()
{
    iop_thread_t t;
    int id;

    sceSifInitRpc(0);
    printf("VI Sound Driver X (build: %s)\n", "Apr 23 2003");
    t.attr = 0x2000000;
    t.thread = (void *)SdRpcThread;
    t.priority = 30;
    t.stacksize = 0x1000;
    t.option = 0;
    id = CreateThread(&t);
    if (id > 0)
    {
        StartThread(id, 0);
        return 0;
    }
    printf("\nStartup thread terminated. Errors encountered");
    return 1;
}

int SdFree(void *p)
{
    return FreeSysMemory(p);
}

void *SdAlloc(int size)
{
    return AllocSysMemory(0, size, 0);
}

void SdAbort()
{
    longjmp(gSdJmp, 1);
}

void SdExit()
{
    longjmp(gSdJmp, 1);
}

void SdCommand(int fno, int *packet)
{
    int n;
    int i;

    n = *packet++;
    for (i = 0; i < n; i++)
    {
        int argc;
        int id;

        id = *packet++;
        argc = *packet++;
        switch (id)
        {
        case 0:
            SdInitSpu();
            break;
        case 12:
            SdFreeSpu();
            break;
        case 1:
            SdAdpcmTransfer(packet[0], packet[1]);
            break;
        case 2:
            SdPlayWave(packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6]);
            break;
        case 4:
            SdStopVoices(packet[0], packet[1]);
            break;
        case 5:
            SdSetPitch(packet[0], packet[1], packet[2]);
            break;
        case 6:
            SdSetVolume(packet[0], packet[1], packet[2], packet[3]);
            break;
        case 7:
            SdXmLoaded(packet[0], packet[1]);
            break;
        case 8:
            SdXmPlay(packet[0], packet[1], packet[2], packet[3], packet[4], packet[5], packet[6]);
            break;
        case 10:
            SdXmSetVolume(packet[0], packet[1], packet[2], packet[3]);
            break;
        case 11:
            SdXmSetPitch(packet[0], packet[1], packet[2]);
            break;
        case 9:
            SdXmStop(packet[0], packet[1]);
            break;
        case 99:
            SdTestTone(packet[0], packet[1]);
            break;
        case 13:
            SdPlayBgm((char *)packet, packet[8]);
            break;
        case 14:
            SdStopBgm();
            break;
        case 15:
            SdSetBgmVolume(packet[0]);
            break;
        }
        packet += argc;
    }
}

// The status block every call answers with (visounddevice.cpp's gSndStatus):
// [0] the IOP bounce buffer, [1] no ADPCM transfer running, [2]/[3] the XM
// region, [4]/[5] the voices with a non-zero envelope on each core, [6] the BGM
// stream is open, [8..23] each XM slot's {lo, hi} voice mask.
void *SdRpc(int fno, void *data, int size)
{
    int cmd = fno & 0xf;
    int i;

    switch (fno & 0xfff0)
    {
    case 0x80f0:
        SdCommand(cmd, (int *)data);
        break;
    default:
        printf("Driver error: unknown command %d \n", *(int *)data);
        break;
    }
    gSndStatus[0] = (u32)gIopBuf;
    gSndStatus[1] = sceSdVoiceTransStatus(0, 0) == 0;
    gSndStatus[2] = (u32)gXmPool;
    gSndStatus[3] = 0x19000;
    gSndStatus[4] = 0;
    gSndStatus[5] = 0;
    for (i = 0; i < 24; i++)
    {
        if (sceSdGetParam((i << 1) | 0x500))
            gSndStatus[4] |= 1 << i;
        if (sceSdGetParam((i << 1) | 0x501))
            gSndStatus[5] |= 1 << i;
    }
    gSndStatus[6] = SdIsBgmPlaying();
    for (i = 0; i < 8; i++)
    {
        gSndStatus[i * 2 + 8] = gXmSlot[i].m_lo;
        gSndStatus[i * 2 + 9] = gXmSlot[i].m_hi;
    }
    return gSndStatus;
}

int SdRpcThread()
{
    SifRpcDataQueue q;
    SifRpcServerData s;
    int i;

    sceSifInitRpc(0);
    sceSifSetRpcQueue(&q, GetThreadId());
    sceSifRegisterRpc(&s, 0x2112, SdRpc, gRpcBuf, 0, 0, &q);
    sceSifRpcLoop(&q);
    for (i = 0; i < 8; i++)
    {
        gXmSlot[i].m_lo = 0;
        gXmSlot[i].m_hi = 0;
        gXmSlot[i].m_active = 0;
    }
    return 0;
}

// Each wake adds tempo * pitch / 4096 to a slot's accumulator and every 250 of
// it is one XM tick: at 100 Hz that is tempo * 2/5 ticks a second, the
// FastTracker rate.  A module that ends stops its slot.
int SdXmThread(void *arg)
{
    int i;

    while (1)
    {
        SleepThread();
        gXmTicks++;
        for (i = 0; i < 8; i++)
        {
            if (gXmSlot[i].m_active)
            {
                gXmSlot[i].m_tick = gXmSlot[i].m_tick
                    + ((gXmSlot[i].m_player.Tempo() * gXmSlot[i].m_player.Pitch()) >> 12);
                while (gXmSlot[i].m_tick >= 250)
                {
                    gXmSlot[i].m_tick = gXmSlot[i].m_tick - 250;
                    if (!gXmSlot[i].m_player.Process())
                    {
                        SdXmStop(gXmSlot[i].m_lo, gXmSlot[i].m_hi);
                        break;
                    }
                }
            }
        }
    }
    return 0;
}

int SdTimerHandler(void *arg)
{
    SdTimer *t = (SdTimer *)arg;

    iWakeupThread(t->thread);
    return t->clocks;
}

// ---- the command handlers ------------------------------------------------------
// Everything VISoundDevice asks of the SPU2 is one of these, called from
// SdCommand with the packet's arguments.  A voice is 0..47; libsd wants it as
// an entry, (voice << 1) | core.
#define SD_VOICE(ch) ((ch) < 24 ? (ch) << 1 : (((ch) - 24) << 1) | 1)
#define SD_MAX(a, b) ((a) > (b) ? (a) : (b))
#define SD_MIN(a, b) ((a) < (b) ? (a) : (b))
#define SD_CLAMP(x)  SD_MIN(SD_MAX(x, 0), 0x3fff)     // fold makes MAX_EXPR/MIN_EXPR

struct sceSdEffectAttr
{
    int   core;
    int   mode;
    short depth_L;
    short depth_R;
    int   delay;
    int   feedback;
};

extern "C" int sceSdSetEffectAttr(int core, sceSdEffectAttr *attr);


int SdInitSpu()
{
    sceSdEffectAttr attr;
    u32 usec;
    iop_sys_clock clock;
    SdTimer *t;
    iop_thread_t th;

    printf("Iop : SdInitSpu()\n");
    sceSdInit(0);
    sceSdSetCoreAttr(10, 0x800);
    sceSdSetSwitch(0x1600, 0xffffff);
    sceSdSetSwitch(0x1601, 0xffffff);
    sceSdSetParam(0xffff, 0);
    sceSdSetParam(0xffff, 0);
    sceSdSetParam(0xffff, 0);
    sceSdSetParam(0xffff, 0);
    sceSdSetParam(0x980, 0x3fff);
    sceSdSetParam(0xa80, 0x3fff);
    sceSdSetParam(0x981, 0x3fff);
    sceSdSetParam(0xa81, 0x3fff);
    attr.core = 0;
    attr.mode = 0x100;
    attr.depth_L = 0;
    attr.depth_R = 0;
    attr.delay = 0;
    attr.feedback = 0;
    sceSdSetEffectAttr(0, &attr);
    sceSdSetEffectAttr(1, &attr);
    sceSdSetParam(0xf80, 0);
    sceSdSetParam(0x1080, 0);
    sceSdSetParam(0xf81, 0);
    sceSdSetParam(0x1081, 0);
    sceSdSetParam(0xb80, 0);
    sceSdSetParam(0xc80, 0);
    sceSdSetParam(0xb81, 0);
    sceSdSetParam(0xc81, 0);

    usec = 10000;
    t = &gSdTimer;
    USec2SysClock(usec, &clock);
    t->clocks = clock.lo;
    t->timer = AllocHardTimer(1, 32, 1);
    if (t->timer <= 0)
    {
        printf("Iop : Can't allocate hardware timer.\n");
        return -1;
    }
    SetTimerHandler(t->timer, t->clocks, SdTimerHandler, t);
    SetupHardTimer(t->timer, 1, 0, 1);
    th.attr = 0x2000000;
    th.thread = (void *)SdXmThread;
    th.priority = 29;
    th.stacksize = 0x1000;
    th.option = 0;
    gXmThread = CreateThread(&th);
    StartThread(gXmThread, 0);
    t->thread = gXmThread;
    StartHardTimer(t->timer);
    SdInitBgmSystem();
    return 0;
}

int SdFreeSpu()
{
    SdTimer *t;

    SdFreeBgmSystem();
    printf("Iop : SdFreeSpu()\n");
    t = &gSdTimer;
    StopHardTimer(t->timer);
    TerminateThread(gXmThread);
    DeleteThread(gXmThread);
    FreeHardTimer(t->timer);
    sceSdInit(0);
    sceSdSetCoreAttr(10, 0x800);
    sceSdSetParam(0x980, 0x3fff);
    sceSdSetParam(0xa80, 0x3fff);
    sceSdSetParam(0x981, 0x3fff);
    sceSdSetParam(0xa81, 0x3fff);
    return 0;
}

// The EE has already DMA'd the block into gIopBuf; this moves it on to SPU RAM.
int SdAdpcmTransfer(int size, int spuAddr)
{
    int r;

    r = sceSdVoiceTrans(0, 0, gIopBuf, spuAddr, size);
    if (r == -210)
    {
        sceSdVoiceTransStatus(0, 1);
        r = sceSdVoiceTrans(0, 0, gIopBuf, spuAddr, size);
    }
    if (r <= 0)
        printf("\n\n!!! sceSdVoiceTrans() failed - code (%d) : iop : adpcm xf (f: 0x%x, t: 0x%x, s: 0x%x)\n\n\n",
               r, gIopBuf, spuAddr, size);
    return r;
}

// A wave is one voice, or two for stereo (`second`'s data starts at `addr2`).
int SdPlayWave(u32 first, int addr, u32 second, int addr2, int pitch, int volume, int pan)
{
    u32 lo = 0;
    u32 hi = 0;
    u32 e1 = SD_VOICE(first);
    u32 e2 = SD_VOICE(second);

    SdSetPitch(first, second, pitch);
    SdSetVolume(first, second, volume, pan);
    sceSdSetAddr(e1 | 0x2040, addr);
    if (first < 24)
        lo = 1 << first;
    else
        hi = 1 << (first - 24);
    if (addr2)
    {
        sceSdSetAddr(e2 | 0x2040, addr2);
        if (second < 24)
            lo |= 1 << second;
        else
            hi |= 1 << (second - 24);
    }
    sceSdSetSwitch(0x1500, lo);
    sceSdSetSwitch(0x1501, hi);
    return 0;
}

int SdStopVoices(u32 lo, u32 hi)
{
    sceSdSetSwitch(0x1600, lo);
    sceSdSetSwitch(0x1601, hi);
    return 0;
}

int SdSetPitch(int first, int second, int pitch)
{
    int e1;
    int e2;

    e1 = SD_VOICE(first);
    sceSdSetParam(e1 | 0x200, pitch);
    if (second != -1)
    {
        e2 = SD_VOICE(second);
        sceSdSetParam(e2 | 0x200, pitch);
    }
    return 0;
}

// A linear pan law on 0..0x3fff: mono puts all of `pan` on one voice's two
// sides; stereo gives each voice of the pair one side of it.
int SdSetVolume(int first, int second, int volume, int pan)
{
    int e1;
    int e2;
    int l1;
    int r1;
    int l2;
    int r2;
    int centre = (pan + 0x3fff) >> 1;

    if (second == -1)
    {
        l1 = volume - (volume * pan >> 14);
        r1 = volume + (volume * pan >> 14);
    }
    else
    {
        l1 = volume - (volume * pan >> 14);
        r1 = volume * pan >> 14;
        l2 = -volume * pan >> 14;
        r2 = volume + (volume * pan >> 14);
    }
    l1 = SD_CLAMP(l1);
    r1 = SD_CLAMP(r1);
    l2 = SD_CLAMP(l2);
    r2 = SD_CLAMP(r2);
    e1 = SD_VOICE(first);
    sceSdSetParam(e1, l1);
    sceSdSetParam(e1 | 0x100, r1);
    if (second != -1)
    {
        e2 = SD_VOICE(second);
        sceSdSetParam(e2, l2);
        sceSdSetParam(e2 | 0x100, r2);
    }
    return 0;
}

int SdXmLoaded(int xmAddr, int sampleSpuAddr)
{
    ((XmModule *)xmAddr)->Relocate(sampleSpuAddr, 0);
    return 0;
}

int SdXmPlay(u32 lo, u32 hi, int xmAddr, u32 repeat, int pitch, int volume, int pan)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        if (!gXmSlot[i].m_active)
        {
            gXmSlot[i].m_lo = lo;
            gXmSlot[i].m_hi = hi;
            gXmSlot[i].m_tick = 0;
            gXmSlot[i].m_active = xmAddr;
            gXmSlot[i].m_player.Play(lo, (XmModule *)gXmSlot[i].m_active, hi);
            gXmSlot[i].m_player.SetRepeat(repeat & 1);
            gXmSlot[i].m_player.SetVolume(volume);
            gXmSlot[i].m_player.SetPan(pan);
            gXmSlot[i].m_player.SetPitch(pitch);
            break;
        }
    }
    return 0;
}

int SdXmSetVolume(int lo, int hi, int volume, int pan)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        if (gXmSlot[i].m_active && gXmSlot[i].m_lo == lo && gXmSlot[i].m_hi == hi)
        {
            gXmSlot[i].m_player.SetVolume(volume);
            gXmSlot[i].m_player.SetPan(pan);
            break;
        }
    }
    return 0;
}

int SdXmSetPitch(int lo, int hi, int pitch)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        if (gXmSlot[i].m_active && gXmSlot[i].m_lo == lo && gXmSlot[i].m_hi == hi)
        {
            gXmSlot[i].m_player.SetPitch(pitch);
            break;
        }
    }
    return 0;
}

int SdXmStop(int lo, int hi)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        if (gXmSlot[i].m_active && gXmSlot[i].m_lo == lo && gXmSlot[i].m_hi == hi)
        {
            gXmSlot[i].m_player.Stop();
            gXmSlot[i].m_lo = 0;
            gXmSlot[i].m_hi = 0;
            gXmSlot[i].m_active = 0;
            break;
        }
    }
    return 0;
}

// Command 99, never sent by retail: a stereo pair on core 0 voices 4 and 5,
// the two halves of one buffer, hard left and hard right.
int SdTestTone(int spuAddr, u32 size)
{
    int core = 0;
    int left = 4;
    int right = 5;

    sceSdSetParam(core | left << 1 | 0x000, 0x3fff);
    sceSdSetParam(core | left << 1 | 0x100, 0);
    sceSdSetParam(core | left << 1 | 0x200, 0xe80);
    sceSdSetAddr(core | left << 1 | 0x2040, spuAddr);
    sceSdSetParam(core | right << 1 | 0x000, 0);
    sceSdSetParam(core | right << 1 | 0x100, 0x3fff);
    sceSdSetParam(core | right << 1 | 0x200, 0xe80);
    sceSdSetAddr(core | right << 1 | 0x2040, spuAddr + (size >> 1));
    sceSdSetSwitch(0x1500, 1 << left | 1 << right);
    return 0;
}
