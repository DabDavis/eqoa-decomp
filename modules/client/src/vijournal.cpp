#line 1 "C:/code-frontiers-gm/game/VIJournal.cpp"
// VIJournal -- CLIENT 0x00CB7C30 .. 0x00CB8260: a record of console input and network
// packets (VIClient+0x90).  Mode 0 keeps it in memory, 1 also records it to a file, 2
// plays a file back.  Write and Read return NOTHING: with an `int` return, JournalTick,
// JournalNetwork, JournalConsole and Init (VIClient.cpp) each mis-scheduled the value
// they spill for the inline Write(T) (2026-10-16: all four exact as void).
//
// Asserts on lines 99, 104 (Write) and 158 (WriteString).
#include "game.h"

extern "C" unsigned int strlen(const char *);   // the builtin's own type: preexpand_calls leaves a builtin call in tree order
extern "C" int   sprintf(char *, const char *, ...);
extern "C" void *alloca(unsigned int);
// (blank lines below pad the asserts to their original lines)














// 0x00CB7C30
int VIJournal::Open(int mode, const char *path, int size)
{
    switch (mode)
    {
    case 0:
        m_MemStream.Grow(size);
        break;
    case 1:
        m_MemStream.Grow(size);
        m_File.Open(path, 0);
        break;
    case 2:
        m_File.Open(path, 1);
        break;
    }
    m_mode = mode;
    return 0;
}

// 0x00CB7CE8
VIJournal::VIJournal()
{
    m_written = 0;
    m_read = 0;
}

// 0x00CB7D48
VIJournal::~VIJournal()
{
}
































// 0x00CB7DB0
void VIJournal::Write(const void *p, int size, int count)
{
    if (m_mode == 2)
        return;
    if (m_mode == 1)
    {
        m_File.Write(p, size, count);
        VIASSERT(!m_File.GetError());
        m_written += size * count;
    }

    m_MemStream.Write(p, size, count);
    VIASSERT(!m_MemStream.GetError());
}

// 0x00CB7E88
void VIJournal::Read(void *p, int size, int count)
{
    if (m_mode == 2)
    {
        m_File.Read(p, size, count);
        m_read += size * count;
        return;
    }
    if (m_mode == 1)
        m_File.Flush();
    VIMemStream &s = m_MemStream;
    s.Read(p, size, count);
    if (s.m_rpos == s.m_size)
    {
        s.m_rpos = 0;
        s.m_size = 0;
    }
}





















// 0x00CB7F38
void VIJournal::WriteString(const char *s)
{
    if (m_mode == 2)
        return;
    if (m_mode == 1)
    {
        m_File.WriteString(s);
        m_written += 4 + strlen(s);
    }
    m_MemStream.WriteString(s);
    VIASSERT(!m_MemStream.GetError());
}

// 0x00CB7FD8
void VIJournal::ReadString(char *dst, unsigned int MaxBytes)
{
    if (m_mode == 2)
    {
        m_File.ReadString(dst, MaxBytes);
        m_read += 4 + strlen(dst);
        return;
    }
    if (m_mode == 1)
        m_File.Flush();
    m_MemStream.ReadString(dst, MaxBytes);
}

// 0x00CB8068
int VIJournal::Eof()
{
    int r;
    if (m_mode == 2)
        r = m_File.Eof();
    else
    {
        VIMemStream &s = m_MemStream;
        r = s.m_size == s.m_rpos;
    }
    return r;
}

// 0x00CB80B0: the first name `fmt` makes of 0..999 that no file has yet.
void VIJournal::FreeName(char *buf, const char *fmt)
{
    for (int i = 0; i < 1000; i++)
    {
        sprintf(buf, fmt, i);
        VIFile f;
        if (f.Open(buf, 1))
            break;
    }
}

// 0x00CB8140: what is left of `s`, its length first.
VIJournal &VIJournal::operator<<(VIMemStream &s)
{
    int n = s.m_size - s.m_rpos;
    char *buf = (char *)alloca(n);
    s.Read(buf, 1, n);
    Write(n);
    Write(buf, 1, n);
    return *this;
}

// 0x00CB81D8: the reverse, into `s`.
VIJournal &VIJournal::operator>>(VIMemStream &s)
{
    int n;
    Read(n);
    char *buf = (char *)alloca(n);
    Read(buf, 1, n);
    s.Write(buf, 1, n);
    return *this;
}
