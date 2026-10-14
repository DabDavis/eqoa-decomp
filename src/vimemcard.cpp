// VIMemCard -- SUPPORT 0x003E71E0 .. 0x003E8400.  The memory-card layer over
// libmc: one save directory named after the product id ("BASLUS-20744EQONLINE")
// holding icon.sys, the icon, a marker file named after the directory, the
// game's own files, and pad.dat -- a zero-filled reservation that keeps the
// directory at a fixed size (memcard-world-loading.md measured it at ~2.8 MB).
//
// Every call is synchronous except the three *Async entry points, which open
// the file, start the transfer and leave the descriptor in sAsyncFd for
// VIMemCardAsyncIdle to flush (writes only) and close.
//
// Results: 0 ok, -1 failed, -2 the card said no (full on write, missing on
// read).  VIMemCardStatus answers 0 ready, 1 no card, 2 unformatted, 3 a
// different formatted card, 4 a different unformatted card, 5 not a PS2 card.
#include "vi.h"

extern "C" int   sprintf(char *, const char *, ...);
extern "C" int   printf(const char *, ...);
extern "C" char *strcpy(char *, const char *);
extern "C" int   strcmp(const char *, const char *);
extern "C" void *memalign(int, int);
extern "C" void  free(void *);
extern "C" int   sceGsSyncV(int mode);

// libmc
struct sceMcStDateTime
{
    unsigned char  Resv2, Sec, Min, Hour;
    unsigned char  Day, Month;
    unsigned short Year;
};
struct sceMcTblGetDir
{
    sceMcStDateTime _Create;
    sceMcStDateTime _Modify;
    unsigned int    FileSizeByte;
    unsigned short  AttrFile;
    unsigned short  Reserve1;
    unsigned int    Reserve2;
    unsigned int    PdaAplNo;
    unsigned char   EntryName[32];
};
extern "C" int sceMcGetInfo(int port, int slot, int *type, int *free, int *format);
extern "C" int sceMcSync(int mode, int *cmd, int *result);
extern "C" int sceMcFormat(int port, int slot);
extern "C" int sceMcGetDir(int port, int slot, const char *name, unsigned mode,
                           int maxent, sceMcTblGetDir *table);
extern "C" int sceMcSetFileInfo(int port, int slot, const char *name, const char *info,
                                unsigned valid);
extern "C" int sceMcMkdir(int port, int slot, const char *name);
extern "C" int sceMcOpen(int port, int slot, const char *name, int mode);
extern "C" int sceMcRead(int fd, void *buf, int size);
extern "C" int sceMcWrite(int fd, const void *buf, int size);
extern "C" int sceMcFlush(int fd);
extern "C" int sceMcClose(int fd);
extern "C" int sceMcDelete(int port, int slot, const char *name);

#define SCE_RDONLY  0x0001
#define SCE_WRONLY  0x0002
#define SCE_CREAT   0x0200

// One directory entry handed back to the UI: the name only.
struct VIMemCardDirEntry
{
    char m_name[32];
};

// The icon file is .data in this TU, 96,408 bytes; its contents are not
// reproduced here.
extern unsigned char VIMemCardIconData[];

static char sProductId[32]    = "BASLUS-20470EQONLINE";
static char sProductTitle[64] = "EverQuest Online\nAdventures";
static int  sInited = 0;
static VIIconDefinition sIconDef;
static int  sAsyncMode;     // 0 read, 1 write, 2 pad
static int  sAsyncFd;

int VIMemCardIsPrepared();
int VIMemCardPad(int kb);
int VIMemCardCopyProhibit();
int VIMemCardReadFile(const char *name, void *buf, int size);
int VIMemCardWriteFile(const char *name, const void *buf, int size);
int VIMemCardDeleteFile(const char *name);

const char *VIMemCardProductId()
{
    return sProductId;
}

void VIMemCardSetProductId(const char *id)
{
    strcpy(sProductId, id);
}

const char *VIMemCardProductTitle()
{
    return sProductTitle;
}

void VIMemCardSetProductTitle(const char *title)
{
    strcpy(sProductTitle, title);
}

int VIMemCardLazyInit()
{
    if (!sInited)
    {
        sAsyncFd = -1;
        sIconDef.SetTitle(VIMemCardProductTitle());
        sIconDef.SetIcon(VIMemCardIconData, 0x17898);
        sInited = 1;
    }
    return 0;
}

int VIMemCardStatus(int *freeOut)
{
    int type, free, format, result;
    int r;

    VIMemCardLazyInit();
    while (sceMcGetInfo(0, 0, &type, &free, &format) != 0)
        ;
    while ((r = sceMcSync(1, 0, &result)) == 0)
        sceGsSyncV(0);

    if (type == 1 || type == 3)
        return 5;
    if (r < 0)
        return 5;
    if (freeOut)
        *freeOut = free;
    if (result < -9)
        return 1;
    switch (result)
    {
    case 0:
        return format ? 0 : 2;
    case -1:
        return format ? 3 : 4;
    case -2:
        return 2;
    case -5:
        return 5;
    }
    return 0;
}

int VIMemCardFormat()
{
    int cmd, result;
    sceMcFormat(0, 0);
    sceMcSync(0, &cmd, &result);
    return result == 0 ? 0 : -1;
}

// Set the directory's copy-prohibit attribute, once.
int VIMemCardCopyProhibit()
{
    char path[256];
    int  cmd, result;
    sceMcTblGetDir *dir;

    sprintf(path, "/%s", VIMemCardProductId());
    dir = (sceMcTblGetDir *)memalign(64, 64);
    if (sceMcGetDir(0, 0, path, 0, 1, dir) != 0 || sceMcSync(0, &cmd, &result) != 1)
    {
        free(dir);
        return -1;
    }
    if (dir->AttrFile & 8)
    {
        printf("<Save Data Already Copy Protected>\n");
        return 0;
    }
    if (result == 1)
    {
        dir->AttrFile |= 8;
        printf("<Copy Protecting Save Data>\n");
        if (sceMcSetFileInfo(0, 0, path, (const char *)dir, 4) == 0)
            sceMcSync(0, &cmd, &result);
    }
    free(dir);
    return 0;
}

// Make the save directory: icon.sys, the icon, then (when asked to) the pad
// and the marker file that VIMemCardIsPrepared looks for.
int VIMemCardPrepare(int padKB)
{
    char path[256];
    int  cmd, result;
    int  fd;

    int status = VIMemCardStatus(0);
    sprintf(path, "/%s", VIMemCardProductId());
    if (status != 0)
        return -1;
    result = VIMemCardIsPrepared();
    if (result < 0)
        return -1;
    if (result == 1)
        return 0;

    sceMcMkdir(0, 0, path);
    sceMcSync(0, &cmd, &result);
    if (result < -9)
        return -1;
    printf("prepare result: %d\n", result);
    switch (result)
    {
    case -3:
        return -2;
    case -2:
    case -5:
        return -1;
    }

    sprintf(path, "/%s/icon.sys", VIMemCardProductId());
    sceMcOpen(0, 0, path, SCE_CREAT | SCE_WRONLY);
    sceMcSync(0, &cmd, &result);
    if (result == -3)
        return -2;
    if (result < 0)
        return -1;
    fd = result;
    sceMcWrite(fd, &sIconDef, 964);
    sceMcFlush(fd);
    sceMcSync(0, &cmd, &result);
    if (result == -3)
        return -2;
    if (result < 0)
        return -1;
    sceMcClose(fd);
    sceMcSync(0, &cmd, &result);
    if (result < 0)
        return -1;

    sprintf(path, "/%s/%s", VIMemCardProductId(), sIconDef.m_fnameView);
    sceMcOpen(0, 0, path, SCE_CREAT | SCE_WRONLY);
    sceMcSync(0, &cmd, &result);
    if (result == -3)
        return -2;
    if (result < 0)
        return -1;
    fd = result;
    sceMcWrite(fd, sIconDef.m_iconData, sIconDef.m_iconSize);
    sceMcFlush(fd);
    sceMcSync(0, &cmd, &result);
    if (result == -3)
        return -2;
    if (result < 0)
        return -1;
    sceMcClose(fd);
    sceMcSync(0, &cmd, &result);
    if (result < 0)
        return -1;

    if (padKB > 0)
    {
        result = VIMemCardPad(padKB);
        if (result < 0)
            return result;
    }
    if (padKB < 0)
        return 0;

    sprintf(path, "/%s/%s", VIMemCardProductId(), VIMemCardProductId());
    sceMcOpen(0, 0, path, SCE_CREAT | SCE_WRONLY);
    sceMcSync(0, &cmd, &result);
    fd = result;
    if (fd == -3)
        return -2;
    if (fd < 0)
        return -1;
    strcpy(path, "12345678");
    sceMcWrite(fd, path, 8);
    sceMcFlush(fd);
    sceMcSync(0, &cmd, &result);
    if (result == -3)
        return -2;
    if (result < 0)
        return -1;
    sceMcClose(fd);
    sceMcSync(0, &cmd, &result);
    if (result < 0)
        return -1;
    VIMemCardCopyProhibit();
    return 0;
}

// 1 when the marker file reads back "12345678", 0 when it is missing or
// short, -1 when the card cannot be read.
int VIMemCardIsPrepared()
{
    char path[256];
    char marker[8];
    int  r;

    marker[7] = 0;
    sprintf(path, "%s", VIMemCardProductId());
    r = VIMemCardReadFile(path, marker, 8);
    if (r == -2)
        return 0;
    if (r != 8)
        return r < 0 ? -1 : 0;
    return marker[7] == '8';
}

// Write pad.dat so the directory occupies `kb` KB: whatever the other files
// use (rounded up to whole KB, plus a cluster per two entries and two more for
// the directory itself) is subtracted from it.
int VIMemCardPad(int kb)
{
    char path[256];
    int  cmd, result;
    sceMcTblGetDir *dir = (sceMcTblGetDir *)memalign(64, 18 * 64);
    int  used, i;
    char *buf;

    VIMemCardLazyInit();
    sprintf(path, "/%s/*", VIMemCardProductId());
    result = sceMcGetDir(0, 0, path, 0, 18, dir);
    sceMcSync(0, &cmd, &result);
    if (result < 0)
    {
        free(dir);
        return -1;
    }
    used = 0;
    for (i = 0; i < result; i++)
        used += (dir[i].FileSizeByte + 1023) >> 10;
    used += result / 2;
    used += 2;
    used = kb - used;
    buf = new char[used << 10];
    for (i = 0; i < used << 10; i++)
        buf[i] = 0;
    result = VIMemCardWriteFile("pad.dat", buf, used << 10);
    free(dir);
    if (buf)
        delete [] buf;
    return result;
}

int VIMemCardPadAsync(int kb)
{
    char path[256];
    int  cmd, result;
    sceMcTblGetDir *dir;
    int  used, i;
    char *buf;

    sAsyncMode = 2;
    dir = (sceMcTblGetDir *)memalign(64, 18 * 64);
    VIMemCardLazyInit();
    sprintf(path, "/%s/*", VIMemCardProductId());
    result = sceMcGetDir(0, 0, path, 0, 18, dir);
    sceMcSync(0, &cmd, &result);
    if (result < 0)
    {
        free(dir);
        return -1;
    }
    used = 0;
    for (i = 0; i < result; i++)
        used += (dir[i].FileSizeByte + 1023) >> 10;
    used += result / 2;
    used += 2;
    used = kb - used;
    buf = new char[used << 10];
    for (i = 0; i < used << 10; i++)
        buf[i] = 0;

    sprintf(path, "/%s/%s", VIMemCardProductId(), "pad.dat");
    sceMcOpen(0, 0, path, SCE_CREAT | SCE_WRONLY);
    sceMcSync(0, &cmd, &result);
    if (result == -3)
    {
        free(dir);
        if (buf)
            delete [] buf;
        return -2;
    }
    if (result < 0)
    {
        free(dir);
        if (buf)
            delete [] buf;
        return -1;
    }
    sAsyncFd = result;
    sceMcWrite(result, buf, used << 10);
    free(dir);
    if (buf)
        delete [] buf;
    return 0;
}

// Poll an *Async transfer: 1 when nothing is in flight, 0 while it runs, else
// its outcome -- a write's 1, a read's byte count (-1 for none), or -1.
int VIMemCardAsyncIdle()
{
    int cmd, result;
    int ret = 1;
    int r = sceMcSync(1, &cmd, &result);

    if (r == -1)
        return 1;
    if (r == 0)
        return 0;
    if (result < 0)
    {
        sAsyncFd = -1;
        return -1;
    }
    if (sAsyncFd != -1)
    {
        if (sAsyncMode)
        {
            sceMcFlush(sAsyncFd);
            sceMcSync(0, &cmd, &result);
        }
        else
            ret = result ? result : -1;
        if (result < 0)
        {
            sAsyncFd = -1;
            return -1;
        }
        sceMcClose(sAsyncFd);
        sceMcSync(0, &cmd, &result);
        if (result < 0)
        {
            sAsyncFd = -1;
            return -1;
        }
        sAsyncFd = -1;
    }
    return ret;
}

int VIMemCardUnpad()
{
    VIMemCardLazyInit();
    return VIMemCardDeleteFile("pad.dat");
}

// The game's own files: everything but ".", "..", the marker and pad.dat.
// Two register-allocation facts in the spelling.  Each rejection is its own
// `count--` arm: jump2 cross-jumps them into one AFTER allocation, so
// global-alloc counts four decrements and `count` outranks the name pointer
// for s0.  And the output is `out[j]` with `j++`: loop.c strength-reduces it
// and puts `p = out` in the loop PREHEADER, so it is not in the clamp's block
// between the two `slt`s -- there it widens the gap past local-alloc's fake
// lifetime and the second `slt` takes v0 instead of a0.
int VIMemCardGetDir(VIMemCardDirEntry *out, int max)
{
    char path[256];
    int  cmd, result;
    sceMcTblGetDir *dir = (sceMcTblGetDir *)memalign(64, 18 * 64);
    int  n;
    int  i;
    int  count;
    char *name;

    VIMemCardLazyInit();
    sprintf(path, "/%s/*", VIMemCardProductId());
    result = sceMcGetDir(0, 0, path, 0, 18, dir);
    sceMcSync(0, &cmd, &result);
    if (result == -4)
    {
        free(dir);
        return -2;
    }
    if (result < 0)
    {
        free(dir);
        return -1;
    }
    n = result < 19 ? result : 18;
    n = max < n ? max : n;
    count = n;
    int j = 0;
    for (i = 0; i < n; i++)
    {
        name = (char *)dir[i].EntryName;
        if (!strcmp(name, "."))
            count--;
        else if (!strcmp(name, ".."))
            count--;
        else if (!strcmp(name, VIMemCardProductId()))
            count--;
        else if (!strcmp(name, "pad.dat"))
            count--;
        else
        {
            strcpy(out[j].m_name, name);
            j++;
        }
    }
    free(dir);
    return count;
}

int VIMemCardReadFileAsync(const char *name, void *buf, int size)
{
    char path[256];
    int  cmd, result;

    sAsyncMode = 0;
    VIMemCardLazyInit();
    sprintf(path, "/%s/%s", VIMemCardProductId(), name);
    sceMcOpen(0, 0, path, SCE_RDONLY);
    sceMcSync(0, &cmd, &result);
    if (result == -4)
        return -2;
    if (result < 0)
        return -1;
    sAsyncFd = result;
    sceMcRead(result, buf, size);
    return 0;
}

int VIMemCardReadFile(const char *name, void *buf, int size)
{
    char path[256];
    int  cmd, result;
    int  n;

    VIMemCardLazyInit();
    result = VIMemCardReadFileAsync(name, buf, size);
    if (result < 0)
        return result;
    sceMcSync(0, &cmd, &result);
    n = result;
    if (n == -3)
        return -2;
    if (n < 0)
        return -1;
    sceMcClose(sAsyncFd);
    sceMcSync(0, &cmd, &result);
    if (result < 0)
        return -1;
    return n;
}

int VIMemCardWriteFileAsync(const char *name, const void *buf, int size)
{
    char path[256];
    int  cmd, result;

    sAsyncMode = 1;
    VIMemCardLazyInit();
    sprintf(path, "/%s/%s", VIMemCardProductId(), name);
    sceMcOpen(0, 0, path, SCE_CREAT | SCE_WRONLY);
    sceMcSync(0, &cmd, &result);
    if (result == -3)
        return -2;
    if (result < 0)
    {
        printf("mc write open error: %d\n", result);
        return -1;
    }
    sAsyncFd = result;
    sceMcWrite(result, buf, size);
    return 0;
}

int VIMemCardWriteFile(const char *name, const void *buf, int size)
{
    int cmd, result;

    VIMemCardLazyInit();
    result = VIMemCardWriteFileAsync(name, buf, size);
    if (result < 0)
        return result;
    sceMcFlush(sAsyncFd);
    sceMcSync(0, &cmd, &result);
    if (result == -3)
        return -2;
    if (result < 0)
        return -1;
    sceMcClose(sAsyncFd);
    sceMcSync(0, &cmd, &result);
    if (result < 0)
        return -1;
    return 0;
}

int VIMemCardDeleteFile(const char *name)
{
    char path[256];
    int  result;

    VIMemCardLazyInit();
    sprintf(path, "/%s/%s", VIMemCardProductId(), name);
    sceMcDelete(0, 0, path);
    while (sceMcSync(1, 0, &result) == 0)
        sceGsSyncV(0);
    if (result == -4)
        return -2;
    if (result < 0)
        return -1;
    return 0;
}
