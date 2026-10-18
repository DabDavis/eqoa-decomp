// SNDDRV.IRX -- "VI Sound Driver X", the IOP half of the sound system.
//
// SUPPORT's VISoundDevice (src/visounddevice.cpp) talks to this module and to
// nothing else: one SIF RPC server, 0x2112, takes a packet of commands per call
// and answers with a 512-byte status block.  Everything the EE asks for -- SPU
// voices, ADPCM uploads, XM modules, the streamed BGM -- is done here through
// libsd, plus CDSTREAM.IRX ("vicds") for the BGM's reads off the disc.
//
// The module is C++ built with gcc 2.8.1 at -O0 for the IOP (mips1) and
// assembled with GNU as -O1; modules/snddrv/toolchain/build.sh says why.
#ifndef SNDDRV_H
#define SNDDRV_H

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// ---- IOP kernel and libraries, as SNDDRV imports them -------------------------
struct iop_thread_t
{
    u32   attr;
    u32   option;
    void *thread;
    u32   stacksize;
    u32   priority;
};

struct iop_thread_info_t
{
    u32   attr;
    u32   option;
    int   status;
    void *entry;
    void *stack;
    int   stackSize;
    void *gpReg;
    int   initPriority;
    int   currentPriority;
    int   waitType;
    int   waitId;
    int   wakeupCount;
    u32   regContext;
    u32   reserved[4];
};

extern "C" {
int   printf(const char *fmt, ...);
int   sprintf(char *buf, const char *fmt, ...);
u32   strlen(const char *s);
void *memset(void *p, int c, u32 n);
void  longjmp(void *env, int v);

void *AllocSysMemory(int mode, int size, void *addr);
int   FreeSysMemory(void *p);
int   CpuSuspendIntr(int *state);
int   CpuResumeIntr(int state);
void  FlushDcache(void);

int   CreateThread(iop_thread_t *t);
int   DeleteThread(int id);
int   StartThread(int id, void *arg);
int   TerminateThread(int id);
int   GetThreadId(void);
int   ReferThreadStatus(int id, iop_thread_info_t *info);
int   SleepThread(void);
int   iWakeupThread(int id);
int   DelayThread(int usec);
void  WaitVblankEnd(void);

int   sceSdInit(int flag);
void  sceSdSetParam(u16 entry, u16 value);
u16   sceSdGetParam(u16 entry);
void  sceSdSetSwitch(u16 entry, u32 value);
void  sceSdSetAddr(u16 entry, u32 value);
u32   sceSdGetAddr(u16 entry);
void  sceSdSetCoreAttr(u16 entry, u16 value);
int   sceSdVoiceTrans(short channel, u16 mode, u8 *iopaddr, u32 spuaddr, u32 size);
u32   sceSdVoiceTransStatus(short channel, short flag);
int   sceSdSetTransIntrHandler(int channel, int (*func)(int, void *), void *arg);

// CDSTREAM.IRX ("vicds"), exports 4..10
int   vicdsInit(void);
int   vicdsQuit(void);
int   vicdsSearch(const char *name, u32 *lsn, u32 *size);
int   vicdsRead(u32 lsn, u32 sectors, void *buf, int mode);
int   vicdsSync(int mode);
int   vicdsGetError(void);
}

// ---- the BGM stream (bgm.cpp) --------------------------------------------------
// Two SPU voices on core 1 play one 128 KB ring each, left and right, looped by
// the ADPCM block flags; a disc read of 128 KB is 64 KB of left then 64 KB of
// right, and a half is refilled when the left voice's play address crosses it.
struct BgmVoices
{
    u32 left;
    u32 right;
} __attribute__((aligned(8)));

int  SdInitBgmSystem();
int  SdFreeBgmSystem();
int  SdPlayBgm(char *name, int spuAddr);
int  SdStopBgm();
u32  SdIsBgmPlaying();
int  SdSetBgmVolume(u32 volume);

// The BGM's disc file: one open file at a time, fd 0.
int  CdOpen(char *name);
int  CdRead(int fd, void *buf, int size);
int  CdSeek(int fd, int offset, int whence);
int  CdClose(int fd);

// ---- the command handlers (spu.cpp) ---------------------------------------------
// A voice is 0..47: 0..23 on core 0, 24..47 on core 1.  Masks come as {lo, hi},
// 24 voices a word, as VISoundDevice splits its 46-bit channel mask.
int  SdInitSpu();
int  SdFreeSpu();
int  SdAdpcmTransfer(int size, int spuAddr);
int  SdPlayWave(u32 first, int addr, u32 second, int addr2, int pitch, int volume, int pan);
int  SdStopVoices(u32 lo, u32 hi);
int  SdSetPitch(int first, int second, int pitch);
int  SdSetVolume(int first, int second, int volume, int pan);
int  SdXmLoaded(int xmAddr, int sampleSpuAddr);
int  SdXmPlay(u32 lo, u32 hi, int xmAddr, u32 repeat, int pitch, int volume, int pan);
int  SdXmSetVolume(int lo, int hi, int volume, int pan);
int  SdXmSetPitch(int lo, int hi, int pitch);
int  SdXmStop(int lo, int hi);
int  SdTestTone(int spuAddr, u32 size);

typedef signed char s8;          // xm.h

#endif
