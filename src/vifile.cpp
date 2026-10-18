// VIFile -- SUPPORT 0x003E5E88 .. 0x003E7150, plus the memory back ends at
// 0x003E8A28 .. 0x003E9140.
//
// The engine's one file abstraction, with four back ends behind it.  Nothing
// here is polymorphic: every entry point re-tests `m_fd >= 0`, then `m_memory`,
// then `m_segms`, and calls the matching implementation by name.
#include "vi.h"

extern "C" void *memcpy(void *, const void *, int);
extern "C" int   printf(const char *, ...);

extern "C" int sceLseek(int fd, int offset, int whence);
extern "C" int sceWrite(int fd, void *buf, int n);
extern "C" int sceCdDiskReady(int mode);
extern "C" int sceCdGetError();
extern "C" int sceCdSearchFile(sceCdlFILE *f, const char *name);
extern "C" int sceCdSync(int mode);
extern "C" int sceCdRead(unsigned lsn, unsigned sectors, void *buf, sceCdRMode *mode);
extern "C" void FlushCache(int mode);

int VIEEFileDelay(int n);
int VICdSearchFile(const char *name, unsigned *lsn, unsigned *size);
int VICdStreamSync(int mode);
int VICdStreamStatus();
int VICdStreamDiskReady();
int VICdStreamRead(unsigned lsn, unsigned sectors, void *buf, int mode);

int VIFile::sIopStream;

void VIFile::EnableIopStreaming(int b)
{
    sIopStream = b;
}

int VIFile::IsIopStreamingEnabled()
{
    return sIopStream;
}

VIFile::VIFile()
{
    m_fd = -1;
    m_flags = 0;
    m_status = 0;
    m_position = 0;
    m_osPosition = 0;
    m_memSize = 0;
    m_memLength = 0;
    m_memory = 0;
    m_numSegms = 0;
    m_segms = 0;
    m_segm = 0;
    m_segmOffset = 0;
    m_cdPending = 0;
    BufferClear();
    m_buffer = 0;
    m_bufferAligned = 0;
}

VIFile::~VIFile()
{
    Close();
}

// Wave 7 (flagsweep names no pass; every spelling below was re-scored): the
// `dev` hunk is a LOCAL-ALLOC fact.  `dev` is a pseudo set in two arms, so
// combine cannot fold its lo_sum into the a3 argument move (the fmt string can:
// `lui a2; addiu a2,a2`), and the hi part is a separate one-set temp that
// local-alloc puts in v0; the target's `lui a3; addiu a3,a3` needs the hi temp
// tied to a3, i.e. `dev` set ONCE or directly into the argument.  A ternary,
// two snprintf calls (42), `(void)dev` and `goto ok` for the flag tests (30)
// all fail.  The third flag test is `bnezl L+4; b ret; li -1` in the target
// (the -1 block stays where the source put it) and `beqz ret` in ours (jump1
// threads past it).
// OPEN, 19/93 (wave 6).  Two sites: the first flag test (`!(flags & 1)` is
// xori/andi/beqz; `(flags & 1) == 0` is the target's andi/bnez but then the
// third test and the return -1 copies cost 11 more), and the `dev` literal --
// the target builds it in a3 with its own lui (`dev` folded to a constant, yet
// the dead `andi` survives to jump2, so the arms were not empty at flow);
// `const char *dev = ""; if (m_flags & 8) dev = "";` deletes the arm and the
// andi with it (32).
int VIFile::Open(const char *path, int flags)
{
    char name[256];

    m_flags = flags;
    if (!(flags & 1))
    {
        if (!(flags & 2))
        {
            if ((flags & 4) == 0)
                return -1;
        }
    }
    const char *dev;
    if (m_flags & 8)
        dev = "";
    else
        dev = "";
    VIString::snprintf(name, 256, "%s\\%s;1", dev, path);
    VIString::strupr(name);
    // INDEXED: loop.c's reduced pointer is set up after the hoisted 47/92
    // (`move v1,sp` last); a `char *p` walker sets it up first.
    for (int i = 0; name[i]; i++)
        if (name[i] == '/')
            name[i] = '\\';
    CDDiskReady(0);
    if (CDSearchFile(&m_cdFile, name) == 0)
    {
        printf("VIEEFile: Unable to locate %s\n", name);
        return -1;
    }
    if (!sIopStream)
    {
        m_cdMode.trycount = 20;
        m_cdMode.spindlctrl = 1;
        m_cdMode.datapattern = 0;
    }
    printf("VIEEFile: %s opened\n", name);
    m_fd = 0;
    m_position = 0;
    BufferClear();
    if (!(m_flags & 0x40))
    {
        m_buffer = new char[0x8010];
        if (m_buffer == 0)
            return -1;
        m_bufferAligned = (char *) (((unsigned) m_buffer + 16) & 0xFFFFFFF0);
    }
    m_status = 0;
    return 0;
}

int VIFile::CDSearchFile(sceCdlFILE *f, const char *name)
{
    unsigned lsn;
    unsigned size;
    int r;

    if (sIopStream == 0)
    {
        while ((r = sceCdSearchFile(f, name)) == 0 && sceCdGetError())
        {
            printf("CDSearchFile: Error %d\n", sceCdGetError());
            while (sceCdDiskReady(0) != 2)
            {
                printf("CDSearchFile: Waiting for disk ready\n");
                VIEEFileDelay(480);
            }
        }
    }
    else
    {
        if (VICdSearchFile(name, &lsn, &size) < 0)
            return 0;
        r = 1;
        f->lsn = lsn;
        f->size = size;
    }
    return r;
}

int VIFile::CDSync(int mode)
{
    int r;

    if (sIopStream == 0)
    {
        r = sceCdSync(mode);
        if (r == 0)
        {
        while (sceCdGetError() && m_cdPending)
        {
            printf("CDSync: Error %d, restarting previous read\n", sceCdGetError());
            while (sceCdDiskReady(0) != 2)
            {
                printf("CDSync: Waiting for disk ready\n");
                VIEEFileDelay(480);
            }
            sceCdSync(0);
            sceCdRead(m_cdLsn, m_cdSectors, m_cdBuffer, m_cdReadMode);
            r = sceCdSync(0);
        }
        m_cdPending = 0;
        }
    }
    else
    {
        r = VICdStreamSync(mode);
        if (r == 0)
        {
        while (VICdStreamStatus() && m_cdPending)
        {
            printf("CDSync: Error %d, restarting previous read\n", VICdStreamStatus());
            while (VICdStreamDiskReady() < 0)
            {
                printf("CDSync: Waiting for disc ready\n");
                VIEEFileDelay(480);
            }
            VICdStreamSync(0);
            VICdStreamRead(m_cdLsn, m_cdSectors, m_cdBuffer, 2);
            r = VICdStreamSync(0);
        }
        m_cdPending = 0;
        }
    }
    return r;
}

int VIFile::CDRead(unsigned lsn, unsigned sectors, void *buf, sceCdRMode *mode)
{
    m_cdLsn = lsn;
    m_cdSectors = sectors;
    m_cdBuffer = buf;
    m_cdReadMode = mode;
    m_cdPending = 1;
    if (sIopStream == 0)
    {
        while (sceCdRead(lsn, sectors, buf, mode) == 0)
        {
            printf("CDRead: Error %d\n", sceCdGetError());
            while (sceCdDiskReady(0) != 2)
            {
                printf("CDRead: Waiting for disk ready\n");
                VIEEFileDelay(480);
            }
        }
    }
    else
    {
        FlushCache(0);
        while (VICdStreamRead(lsn, sectors, buf, 2) < 0)
            VICdStreamSync(0);
    }
    return 1;
}

// libkernel's alarm and semaphore calls.  Only the three SemaParam fields the
// delay sets are named; the rest are the kernel's.
struct SemaParam
{
    int      currentCount;
    int      maxCount;
    int      initCount;
    int      numWaitThreads;
    unsigned attr;
    unsigned option;
};
extern "C" int CreateSema(SemaParam *p);
extern "C" int DeleteSema(int sema);
extern "C" int WaitSema(int sema);
extern "C" int iSignalSema(int sema);
extern "C" int SetAlarm(unsigned short time, void (*cb)(int, unsigned short, void *), void *arg);

// The alarm fires in interrupt context: post the semaphore, then ExitHandler.
static void FileDelayAlarm(int id, unsigned short time, void *sema)
{
    iSignalSema((int)sema);
    VI_EE_ASM("sync.l\n\tei\n");
}

// Sleep `n` horizontal blanks (480 is two fields): a one-shot alarm posts a
// semaphore this thread waits on, so the wait yields the EE.
int VIEEFileDelay(int n)
{
    SemaParam p;
    int sema;

    p.maxCount = 1;
    p.initCount = 0;
    p.option = 0;
    sema = CreateSema(&p);
    SetAlarm(n, FileDelayAlarm, (void *)sema);
    WaitSema(sema);
    return DeleteSema(sema);
}

int VIFile::Open(const void *p, long size, int flags, long len)
{
    return MemoryOpen(p, size, flags, len);
}

int VIFile::Open(int nSegms, MemSegm *segms, int flags, long len)
{
    return MemorySegmOpen(nSegms, segms, flags, len);
}

int VIFile::Close()
{
    if (m_fd >= 0)
    {
        BufferFlush();
        printf("VIEEFile: Close\n");
        // NOT m_flags: the asynchronous drain tests the file DESCRIPTOR for
        // bit 6, so on any real handle it is skipped.  Matched as written.
        if (m_fd & 0x40)
        {
            while (IsAsynchComplete() == 0)
                ;
        }
        m_fd = -1;
    }
    m_flags = 0;
    m_status = 0;
    m_position = 0;
    m_osPosition = 0;
    if (m_buffer)
    {
        delete [] m_buffer;
        m_buffer = 0;
        m_bufferAligned = 0;
    }
    m_memSize = 0;
    m_memLength = 0;
    m_memory = 0;
    m_numSegms = 0;
    if (m_segms)
    {
        delete [] m_segms;
        m_segms = 0;
    }
    m_segm = 0;
    m_segmOffset = 0;
    m_cdPending = 0;
    return 0;
}

int VIFile::Seek(long offset, SeekOrigin origin)
{
    if (m_fd >= 0)
        return BufferedSeek(offset, origin);
    if (m_memory)
        return MemorySeek(offset, origin);
    if (m_segms)
        return MemorySegmSeek(offset, origin);
    return -1;
}

long VIFile::Tell()
{
    return m_position;
}

int VIFile::Flush()
{
    if (m_fd >= 0)
        BufferFlush();
    return 0;
}

int VIFile::Eof()
{
    if (m_fd >= 0 || m_memory || m_segms)
        return m_status;
    return -1;
}

int VIFile::Read(void *dst, int n)
{
    if (m_fd >= 0)
        return BufferedRead(dst, n) == n ? 0 : -1;
    if (m_memory)
        return MemoryRead(dst, n);
    if (m_segms)
        return MemorySegmRead(dst, n);
    return -1;
}

int VIFile::Write(const void *src, int n)
{
    if (m_fd >= 0)
        return BufferedWrite(src, n) == n ? 0 : -1;
    if (m_memory)
        return MemoryWrite(src, n);
    if (m_segms)
        return MemorySegmWrite(src, n);
    return -1;
}

int VIFile::BufferClear()
{
    m_bufLength = 0;
    m_bufEof = 0;
    m_dirtyStart = 0;
    m_dirtyEnd = 0;
    m_bufPos = 0;
    m_bufBase = 0;
    return 0;
}

int VIFile::AsynchAlignment()
{
    return 2048;
}

int VIFile::IsAsynchComplete()
{
    if (m_fd < 0)
        return m_memory == 0 ? -1 : 1;
    return CDSync(1) == 0;
}

int VIFile::CDDiskReady(int mode)
{
    if (!sIopStream)
        return sceCdDiskReady(mode);
    return 2;
}

int VIFile::MemoryOpen(const void *p, long size, int flags, long len)
{
    m_flags = flags;
    if ((flags & 7) == 0)
        return -1;
    m_memSize = size;
    m_memory = (char *) p;
    m_memLength = len;
    if (flags & 8)
    {
        m_memLength = 0;
        m_position = 0;
    }
    else if ((flags & 0x10) && (flags & 6))
        m_position = len;
    else
        m_position = 0;
    m_status = 0;
    return 0;
}

int VIFile::MemorySeek(long offset, SeekOrigin origin)
{
    if (origin == Seek_Set)
        m_position = offset;
    else if (origin == Seek_Cur)
        m_position = m_position + offset;
    else if (origin == Seek_End)
        m_position = m_memLength + offset;
    else
        return -1;
    if (m_position < 0 || m_position >= m_memLength)
        return -1;
    m_status = 0;
    return 0;
}

int VIFile::MemoryRead(void *dst, int n)
{
    m_status = 0;
    long avail = m_memLength - m_position;
    int got = (int) (avail < n ? avail : n);
    if (got > 0)
        memcpy(dst, m_memory + (int) m_position, got);
    m_position = got + m_position;
    if (got < n)
    {
        m_status = 1;
        return -1;
    }
    return 0;
}

int VIFile::MemoryWrite(const void *src, int n)
{
    m_status = 0;
    long avail = m_memSize - m_position;
    int put = (int) (avail < n ? avail : n);
    if (put > 0)
        memcpy(m_memory + (int) m_position, src, put);
    m_position = put + m_position;
    m_memLength = VIMAX(m_memLength, m_position);
    return put == n ? 0 : -1;
}

int VIFile::BufferFlush()
{
    if (m_bufLength)
    {
        if (m_dirtyStart != m_dirtyEnd)
        {
            int n = (int) (m_dirtyEnd - m_dirtyStart);
            long at = m_bufBase + m_dirtyStart;
            if (at != m_osPosition)
            {
                long r = sceLseek(m_fd, (int) at, 0);
                m_osPosition = r;
                if (at != r)
                    return -1;
            }
            int w = sceWrite(m_fd, m_bufferAligned + (int) m_dirtyStart, n);
            m_osPosition = m_osPosition + w;
            if (w != n)
                return -1;
            m_dirtyStart = 0;
            m_dirtyEnd = 0;
        }
    }
    return 0;
}

// Wave 7: the two `slt` temps of the multi-sector block (sector test r264
// [20,24], min test r175 [26,30], both priority 5000) go a3 then t0 in ours and
// t0 then a3 in the target.  lsim reproduces ours; topo.py finds NO legal
// sched1 order of the block that gives the target's pair, and raising r175's
// refs only moves other temps (v1/v0).  So it is not birth order: the pseudo
// structure differs (the target's `slt a2,a2,a1` also sits after the daddiu,
// a sched2 consequence of the swap).
// OPEN, 21/233 (was 47).  Every `return -1` is a `goto fail` and the `n == r`
// exit a `goto done`, both labelled after the closing `m_status = 1` return:
// that is the target's exit-block order (status block, -1 block, got block,
// epilogue).  The last block computes `sector` BEFORE `need`, which is the
// target's instruction order.  Left: local-alloc's register choice in the two
// sector/need blocks (v0/v1 and a3/t0 swapped) and the m_bufPos/m_bufBase
// store order inside the multi-sector loop.
int VIFile::BufferedRead(void *dst, int n)
{
    int sector;
    int sectors;

    m_status = 0;
    int got = BufferedReadCopy(dst, n);
    n = n - got;
    if (n == 0)
        return got;
    if (m_bufEof)
    {
        m_status = 1;
        return got;
    }
    if (m_position & 2047)
    {
        BufferFlush();
        BufferClear();
        sector = (int) (m_position / 2048);
        m_bufBase = sector * 2048;
        m_bufPos = m_position - sector * 2048;
        if (CDSync(0))
            goto fail;
        if (CDRead(sector + m_cdFile.lsn, 1, m_bufferAligned, &m_cdMode) == 0)
            goto fail;
        if (CDSync(0))
            goto fail;
        if (m_cdFile.size - m_bufBase < 2048)
        {
            m_bufLength = m_cdFile.size - m_bufBase;
            m_bufEof = 1;
        }
        else
        {
            m_bufLength = 2048;
            m_bufEof = 0;
        }
        int r = BufferedReadCopy((char *) dst + got, n);
        n = n - r;
        got = got + r;
        if (n == 0)
            return got;
        if (m_bufEof)
        {
            m_status = 1;
            return got;
        }
    }
    if (n > 2048)
    {
        BufferFlush();
        BufferClear();
        long avail = m_cdFile.size - m_position;
        sector = (int) (m_position / 2048);
        sectors = (int) ((avail < n ? avail : n) / 2048);
        while (sectors > 0)
        {
            int k = sectors < 17 ? sectors : 16;
            if (CDSync(0))
                goto fail;
            if (CDRead(sector + m_cdFile.lsn, k, m_bufferAligned, &m_cdMode) == 0)
                goto fail;
            if (CDSync(0))
                goto fail;
            m_bufPos = m_position - sector * 2048;
            m_bufBase = sector * 2048;
            m_bufLength = k * 2048;
            int r = BufferedReadCopy((char *) dst + got, n);
            sector = sector + k;
            sectors = sectors - k;
            got = got + r;
            n = n - r;
        }
        if (n == 0)
            return got;
    }
    BufferFlush();
    BufferClear();
    if (m_position != m_cdFile.size)
    {
        sector = (int) (m_position / 2048);
        long need = (m_cdFile.size - m_position) / 2048;
        m_bufPos = m_position - sector * 2048;
        m_bufBase = sector * 2048;
        need = need > 0 ? need : 1;
        // The second clamp is a temporary, not a store back into `need`: one
        // fewer tie on `need`'s quantity (BufferedRead 21 -> 20).
        sectors = (int) (need < 17 ? need : 16);
        if (CDSync(0))
            goto fail;
        if (CDRead(sector + m_cdFile.lsn, sectors, m_bufferAligned, &m_cdMode) == 0)
            goto fail;
        if (CDSync(0))
            goto fail;
        if (m_cdFile.size - m_position < sectors * 2048)
        {
            m_bufLength = m_cdFile.size - m_position;
            m_bufEof = 1;
        }
        else
        {
            m_bufLength = sectors * 2048;
            m_bufEof = 0;
        }
        int r = BufferedReadCopy((char *) dst + got, n);
        got = got + r;
        if (n == r)
            goto done;
        if (m_bufEof == 0)
            return got;
    }
    m_status = 1;
    return got;
fail:
    return -1;
done:
    return got;
}

int VIFile::BufferedReadCopy(void *dst, int n)
{
    int got = 0;
    if (m_bufLength)
    {
        got = (int) VIMIN(n, m_bufLength - m_bufPos);
        if (got)
        {
            if (sIopStream)
                memcpy(dst, (char *) VI_UNCACHED((unsigned) (m_bufferAligned + (int) m_bufPos)), got);
            else
                memcpy(dst, m_bufferAligned + (int) m_bufPos, got);
            m_bufPos = m_bufPos + got;
            m_position = m_position + got;
        }
    }
    return got;
}

// The tail is written out in BOTH arms, each with its own `return n`: jump2
// cross-jumps the identical tails back together, memcpy included, into the
// target's single `jal`.  With one shared tail the falling-through arm ends
// in the call, flow puts a `(use (const_int 0))` after it, and nothing merges.
int VIFile::BufferedWrite(const void *src, int n)
{
    m_status = 0;
    if (n > 2048)
    {
        BufferFlush();
        BufferClear();
        if (m_osPosition != m_position)
        {
            long r = sceLseek(m_fd, (int) m_position, 0);
            m_osPosition = r;
            if (r != m_position)
                return -1;
        }
        int w = sceWrite(m_fd, (void *) src, n);
        m_osPosition = m_osPosition + w;
        m_position = m_position + w;
        return w == n ? w : -1;
    }
    if (m_bufLength == 0)
        m_bufBase = m_position;
    if (m_bufPos >= 0 && m_bufPos + n <= 32767)
    {
        memcpy(m_bufferAligned + (int) m_bufPos, src, n);
        long end = m_bufPos + n;
        m_bufLength = VIMAX(end, m_bufLength);
        m_dirtyStart = VIMIN(m_bufPos, m_dirtyStart);
        m_dirtyEnd = VIMAX(m_dirtyEnd, end);
        m_bufPos = end;
        m_position = m_position + n;
        return n;
    }
    else
    {
        BufferFlush();
        int keep = (int) VIMIN(0, m_bufLength);
        int i = (int) m_position - keep;
        if (i < m_position)
        {
            do
            {
                *m_bufferAligned = m_bufferAligned[i];
                i++;
            }
            while (i < m_position);
        }
        m_dirtyEnd = keep;
        m_bufLength = keep;
        m_dirtyStart = keep;
        m_bufBase = m_position - keep;
        m_bufPos = keep;
        memcpy(m_bufferAligned + (int) m_bufPos, src, n);
        long end = m_bufPos + n;
        m_bufLength = VIMAX(end, m_bufLength);
        m_dirtyStart = VIMIN(m_bufPos, m_dirtyStart);
        m_dirtyEnd = VIMAX(m_dirtyEnd, end);
        m_bufPos = end;
        m_position = m_position + n;
        return n;
    }
}

int VIFile::BufferedSeek(long offset, SeekOrigin origin)
{
    if (origin == Seek_Set)
        m_position = offset;
    else if (origin == Seek_Cur)
        m_position = m_position + offset;
    else if (origin == Seek_End)
        m_position = m_cdFile.size + offset;
    else
        return -1;
    if (m_bufLength)
    {
        if (m_position < m_bufBase || !(m_position < m_bufBase + m_bufLength))
        {
            BufferFlush();
            BufferClear();
        }
        else
            m_bufPos = m_position - m_bufBase;
    }
    m_status = 0;
    return 0;
}

int VIFile::ReadAsynch(long offset, void *dst, int n)
{
    if (m_fd >= 0)
    {
        if (offset & 0x7FF)
            return -1;
        if (n & 0x7FF)
            return -1;
        if ((int) dst & 0xF)
            return -1;
        if (CDSync(0))
            return -1;
        return CDRead(offset / 2048 + m_cdFile.lsn, n / 2048, dst, &m_cdMode) ? 0 : -1;
    }
    if (m_memory == 0)
        return -1;
    if (Seek(offset, Seek_Set) < 0)
        return -1;
    if (Read(dst, n) < 0)
        return -1;
    return 0;
}

int VIFile::WriteAsynch(long offset, void *src, int n)
{
    if (m_fd >= 0)
        return -1;
    if (m_memory == 0)
        return -1;
    if (Seek(offset, Seek_Set) < 0)
        return -1;
    if (Write(src, n) < 0)
        return -1;
    return 0;
}

int VIFile::MemorySegmOpen(int nSegms, MemSegm *segms, int flags, long len)
{
    m_flags = flags;
    if ((flags & 7) == 0)
        return -1;
    m_numSegms = nSegms;
    m_segms = (Segm *) new char[nSegms * sizeof(Segm)];
    if (m_segms == 0)
        return -1;
    m_memSize = 0;
    for (int i = 0; i < nSegms; i++)
    {
        m_segms[i].m_start = m_memSize;
        m_segms[i].m_segm = segms[i];
        m_memSize = m_memSize + m_segms[i].m_segm.m_size;
    }
    m_memLength = len;
    if (flags & 8)
    {
        m_memLength = 0;
        m_position = 0;
    }
    else if ((flags & 0x10) && (flags & 6))
        m_position = len;
    else
        m_position = 0;
    MemorySegmCalcSegm();
    m_status = 0;
    return 0;
}

int VIFile::MemorySegmSeek(long offset, SeekOrigin origin)
{
    if (origin == Seek_Set)
        m_position = offset;
    else if (origin == Seek_Cur)
        m_position = m_position + offset;
    else if (origin == Seek_End)
        m_position = m_memLength + offset;
    else
        return -1;
    if (m_position < 0 || m_position >= m_memLength)
        return -1;
    MemorySegmCalcSegm();
    m_status = 0;
    return 0;
}

int VIFile::MemorySegmRead(void *dst, int n)
{
    char *d = (char *) dst;

    m_status = 0;
    long avail = m_memLength - m_position;
    int got = (int) (avail < n ? avail : n);
    if (got > 0)
    {
        int left = got;
        do
        {
            int k = (int) VIMIN(left, m_segms[m_segm].m_segm.m_size - m_segmOffset);
            memcpy(d, (char *) m_segms[m_segm].m_segm.m_addr + (int) m_segmOffset, k);
            m_segmOffset = k + m_segmOffset;
            if (!(m_segmOffset < m_segms[m_segm].m_segm.m_size))
            {
                if (m_segm != m_numSegms)
                {
                    m_segmOffset = 0;
                    m_segm = m_segm + 1;
                }
            }
            left = left - k;
            d = d + k;
        }
        while (left > 0);
    }
    m_position = got + m_position;
    if (got < n)
    {
        m_status = 1;
        return -1;
    }
    return 0;
}

int VIFile::MemorySegmWrite(const void *src, int n)
{
    char *s = (char *) src;

    m_status = 0;
    long avail = m_memSize - m_position;
    int put = (int) (avail < n ? avail : n);
    if (put > 0)
    {
        int left = put;
        do
        {
            int k = (int) VIMIN(left, m_segms[m_segm].m_segm.m_size - m_segmOffset);
            memcpy((char *) m_segms[m_segm].m_segm.m_addr + (int) m_segmOffset, s, k);
            m_segmOffset = k + m_segmOffset;
            if (!(m_segmOffset < m_segms[m_segm].m_segm.m_size))
            {
                if (m_segm != m_numSegms)
                {
                    m_segmOffset = 0;
                    m_segm = m_segm + 1;
                }
            }
            left = left - k;
            s = s + k;
        }
        while (left > 0);
    }
    m_position = put + m_position;
    m_memLength = VIMAX(m_memLength, m_position);
    return put == n ? 0 : -1;
}

int VIFile::MemorySegmCalcSegm()
{
    int lo = 0;
    int hi = m_numSegms - 1;
    int mid;

    while (lo <= hi)
    {
        mid = (lo + hi) >> 1;
        if (m_position >= m_segms[mid].m_start &&
            m_position < m_segms[mid].m_start + m_segms[mid].m_segm.m_size)
            break;
        if (m_position < m_segms[mid].m_start)
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    m_segm = mid;
    m_segmOffset = m_position - m_segms[mid].m_start;
    return 0;
}
