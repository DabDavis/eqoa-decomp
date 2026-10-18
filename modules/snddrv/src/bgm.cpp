// The streamed BGM: SNDDRV 0x0000 .. 0x0F8C.
//
// A thread of its own (priority 29) runs the stream state machine once a
// vblank.  SdPlayBgm opens the file and arms state 3; state 3 reads both halves
// of the ring off the disc and uploads them, then keys the two voices on and
// moves to 7, where each crossing of the left voice's play address past a
// half's boundary refills the half it just left.  State 6 is stopped.
#include "snddrv.h"

// .sdata, reached through $gp
int sBgmThread = 0;
int sBgmQuit = 0;
BgmVoices sBgmVoice = { 0x2d, 0x2f };   // core 1, voices 22 and 23

// .sdata in driver.cpp; an extern is always reached absolutely
extern int  gBgmState;          // 3 start, 6 stopped, 7 streaming
extern int  gBgmHalf;           // the half the left voice is playing
extern int  gBgmXferHalf;       // the half being uploaded
extern int  gBgmStop;           // a stop request for the thread

// .sbss: file statics, emitted as .lcomm at the END of the file, so the SN
// assembler (which does not relax forward references) reaches them absolutely
static u8  *gBgmBuf[2];         // two 128 KB IOP buffers, one disc read each
static int  gBgmSpuLeft;        // the left ring in SPU RAM, 128 KB
static int  gBgmSpuRight;       // the right ring, 128 KB above it
static int  gBgmFd;             // the open file, -1 when none
static int  gBgmXfer;           // 0 idle, 1 left half going, 2 right half going
static u32  gBgmVolume;

int BgmThread(void *arg);
int BgmRead(int half);
int BgmTransIntr(int channel, void *arg);
void BgmTransfer(int half);
int BgmTransferDone();

int SdInitBgmSystem()
{
    iop_thread_t t;
    int state;

    printf("SdInitBgmSystem()\n");
    printf("BGM CHANNEL SIZE: %d\n", 0x10000);
    sBgmQuit = 0;
    t.attr = 0x2000000;
    t.thread = (void *)BgmThread;
    t.priority = 29;
    t.stacksize = 0x800;
    t.option = 0;
    sBgmThread = CreateThread(&t);
    StartThread(sBgmThread, 0);
    sceSdSetTransIntrHandler(1, BgmTransIntr, 0);
    CpuSuspendIntr(&state);
    gBgmBuf[0] = (u8 *)AllocSysMemory(0, 0x20000, 0);
    gBgmBuf[1] = (u8 *)AllocSysMemory(0, 0x20000, 0);
    if (gBgmBuf[0] == 0 || gBgmBuf[1] == 0)
        printf("BGM AllocSysMemory Failed!\n");
    CpuResumeIntr(state);
    gBgmFd = -1;
    gBgmVolume = 0x3fff;
    gBgmXfer = 0;
    gBgmXferHalf = 0;
    gBgmStop = 0;
    vicdsInit();
    return 0;
}

int SdFreeBgmSystem()
{
    iop_thread_info_t info;
    int state;

    printf("SdFreeBgmSystem()\n");
    sBgmQuit = 1;
    if (SdIsBgmPlaying())
        SdStopBgm();
    vicdsQuit();
    do
        ReferThreadStatus(sBgmThread, &info);
    while (info.status != 16);
    CpuSuspendIntr(&state);
    FreeSysMemory(gBgmBuf[1]);
    FreeSysMemory(gBgmBuf[0]);
    CpuResumeIntr(state);
    sceSdSetTransIntrHandler(1, 0, 0);
    DeleteThread(sBgmThread);
    return 0;
}

int SdPlayBgm(char *name, int spuAddr)
{
    printf("PlayBgm()\n");
    gBgmStop = 0;
    if (gBgmFd != -1)
        SdStopBgm();
    while (!BgmTransferDone())
        WaitVblankEnd();
    gBgmSpuLeft = spuAddr;
    gBgmSpuRight = spuAddr + 0x20000;
    memset(gBgmBuf[0], 0, 0x20000);
    memset(gBgmBuf[1], 0, 0x20000);
    printf("SdPlayBgm (%s)\n", name);
    gBgmFd = CdOpen(name);
    if (gBgmFd < 0)
    {
        printf("SdPlayBgm() failed (fd = %d)\n", gBgmFd);
        return -1;
    }
    gBgmState = 3;
    return 0;
}

int SdStopBgm()
{
    gBgmState = 6;
    gBgmStop = 1;
    while (gBgmStop)
        WaitVblankEnd();
    sceSdSetCoreAttr(5, 0);
    sceSdSetSwitch(0x1601, 0xc00000);
    if (gBgmFd >= 0)
    {
        CdClose(gBgmFd);
        gBgmFd = -1;
    }
    return 0;
}

u32 SdIsBgmPlaying()
{
    return gBgmFd >= 0;
}

int SdSetBgmVolume(u32 volume)
{
    BgmVoices v = sBgmVoice;

    gBgmVolume = volume;
    sceSdSetParam(v.left, gBgmVolume);
    sceSdSetParam(v.left | 0x100, 0);
    sceSdSetParam(v.right, 0);
    sceSdSetParam(v.right | 0x100, gBgmVolume);
    return 0;
}

int BgmThread(void *arg)
{
    BgmVoices v = sBgmVoice;

    while (1)
    {
        WaitVblankEnd();
        if (gBgmStop)
        {
            gBgmState = 6;
            gBgmStop = 0;
        }
        if (sBgmQuit)
            return 0;
        if (gBgmFd == -1)
            continue;
        switch (gBgmState)
        {
        case 6:
            continue;
        case 3:
            gBgmHalf = 0;
            gBgmXfer = 0;
            if (BgmRead(0) < 0)
            {
                if (gBgmFd >= 0)
                {
                    CdClose(gBgmFd);
                    gBgmFd = -1;
                }
                gBgmState = 6;
                break;
            }
            if (BgmRead(1) < 0)
            {
                if (gBgmFd >= 0)
                {
                    CdClose(gBgmFd);
                    gBgmFd = -1;
                }
                gBgmState = 6;
                break;
            }
            BgmTransfer(0);
            while (!BgmTransferDone() && gBgmState != 6)
                WaitVblankEnd();
            if (gBgmState == 6)
                break;
            BgmTransfer(1);
            while (!BgmTransferDone() && gBgmState != 6)
                WaitVblankEnd();
            if (gBgmState == 6)
                break;
            sceSdSetAddr(v.left | 0x2040, gBgmSpuLeft);
            sceSdSetAddr(v.right | 0x2040, gBgmSpuRight);
            sceSdSetParam(v.left | 0x200, 0xeb3);
            sceSdSetParam(v.right | 0x200, 0xeb3);
            sceSdSetParam(v.left, gBgmVolume);
            sceSdSetParam(v.left | 0x100, 0);
            sceSdSetParam(v.right, 0);
            sceSdSetParam(v.right | 0x100, gBgmVolume);
            sceSdSetSwitch(0x1501, 0xc00000);
            if (gBgmState == 6)
                continue;
            gBgmState = 7;
            break;
        case 7:
            if (BgmTransferDone())
            {
                int addr = sceSdGetAddr(v.left | 0x2240);
                int refill = 0;

                if (gBgmHalf == 0 && addr >= gBgmSpuLeft + 0x10000)
                {
                    gBgmHalf = 1;
                    refill = 1;
                }
                else if (gBgmHalf == 1 && addr < gBgmSpuLeft + 0x10000)
                {
                    gBgmHalf = 0;
                    refill = 1;
                }
                if (refill)
                {
                    if (BgmRead(1 - gBgmHalf) < 0)
                    {
                        gBgmState = 6;
                        sceSdSetCoreAttr(5, 0);
                        sceSdSetSwitch(0x1601, 0xc00000);
                        if (gBgmFd >= 0)
                        {
                            CdClose(gBgmFd);
                            gBgmFd = -1;
                        }
                        break;
                    }
                    BgmTransfer(1 - gBgmHalf);
                }
            }
            break;
        }
    }
    return 0;
}

int BgmRead(int half)
{
    if (CdRead(gBgmFd, gBgmBuf[half], 0x20000) < 0x20000)
        return -1;
    return 0;
}

int BgmTransIntr(int channel, void *arg)
{
    return 0;
}

// The loop flags go into the ADPCM blocks before they leave: the first half
// starts the loop on both channels, the second half's last block jumps back.
void BgmTransfer(int half)
{
    FlushDcache();
    gBgmXferHalf = half;
    if (gBgmXferHalf == 0)
    {
        gBgmBuf[0][1] = 6;
        gBgmBuf[0][0x11] = 2;
        gBgmBuf[0][0x10001] = 6;
        gBgmBuf[0][0x10011] = 2;
    }
    else
    {
        gBgmBuf[1][0xfff1] = 3;
        gBgmBuf[1][0x1fff1] = 3;
    }
    gBgmXfer = 1;
    while (sceSdVoiceTrans(1, 0, gBgmBuf[gBgmXferHalf],
                           gBgmXferHalf * 0x10000 + gBgmSpuLeft, 0x10000) == -201)
        sceSdVoiceTransStatus(1, 1);
}

// Both channels of a half go as two transfers; the right one is started from
// here once the left one is done.
int BgmTransferDone()
{
    int status;

    if (gBgmXfer == 0)
        return 1;
    status = sceSdVoiceTransStatus(1, 0);
    if (status == 1)
    {
        if (gBgmXfer == 1)
        {
            gBgmXfer = 2;
            while (sceSdVoiceTrans(1, 0, gBgmBuf[gBgmXferHalf] + 0x10000,
                                   gBgmXferHalf * 0x10000 + gBgmSpuRight, 0x10000) == -201)
                sceSdVoiceTransStatus(1, 1);
        }
        else
            return 1;
    }
    return 0;
}

// ---- the BGM's file on the disc ---------------------------------------------
// One file open at a time through CDSTREAM, by sector: fd 0 is the only fd.
// Names are upper-cased and given ISO 9660's `\NAME;1` form.
struct CdFile
{
    u32 lsn;
    u32 size;
};

CdFile gCdFile;                 // .data 0x9610: the open file's first sector and length
extern u32 gCdSector;           // the next sector to read (driver.cpp)

int CdOpen(char *name)
{
    char path[256];
    int i;

    sprintf(path, "\\%s;1", name);
    for (i = strlen(path) - 1; i >= 0; i--)
    {
        if (path[i] >= 'a' && path[i] <= 'z')
            path[i] -= 0x20;
        if (path[i] == '/')
            path[i] = '\\';
    }
    printf("Try open (%s)\n", path);
    if (vicdsSearch(path, &gCdFile.lsn, &gCdFile.size) < 0)
    {
        printf("FAIL\n");
        return -1;
    }
    gCdSector = gCdFile.lsn;
    printf("SUCCESS\n");
    return 0;
}

// A read that fails (the drive is busy or the disc was opened) silences the
// BGM voices -- pitch 0 -- and retries every 33 ms until it goes through.
int CdRead(int fd, void *buf, int size)
{
    BgmVoices v = sBgmVoice;
    int muted = 0;

    if (fd)
        return -1;
    if ((gCdSector - gCdFile.lsn) * 0x800 >= gCdFile.size)
        return 0;
    do
    {
        while (vicdsRead(gCdSector, size / 0x800, buf, 1) < 0)
        {
            if (!muted)
            {
                muted = 1;
                sceSdSetParam(v.left | 0x200, 0);
                sceSdSetParam(v.right | 0x200, 0);
            }
            DelayThread(0x8235);
        }
        vicdsSync(0);
    } while (vicdsGetError());
    if (muted)
    {
        muted = 0;
        sceSdSetParam(v.left | 0x200, 0xeb3);
        sceSdSetParam(v.right | 0x200, 0xeb3);
    }
    gCdSector += size / 0x800;
    if ((gCdSector - gCdFile.lsn) * 0x800 > gCdFile.size)
        return gCdFile.size - (gCdSector - 1 - gCdFile.lsn) * 0x800;
    return size;
}

int CdSeek(int fd, int offset, int whence)
{
    if (fd)
        return -1;
    switch (whence)
    {
    case 0:
        gCdSector = gCdFile.lsn + offset / 0x800;
        break;
    case 1:
        gCdSector += offset / 0x800;
        break;
    case 2:
        gCdSector = gCdFile.lsn - offset / 0x800;
        break;
    }
}

int CdClose(int fd)
{
    if (fd)
        return -1;
}
