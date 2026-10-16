// Log.cpp (name ours) -- SETUP 0x005821D0 .. 0x00582320: the patcher's log.  The
// PS2 keeps the formatting and drops the output.
#include "patcher.h"

// 0x005821D0.
Log::Log(char *name, int level)
{
    m_level = level;
    m_x0 = 0;
}

// 0x005821E0.
Log::~Log()
{
    close();
}

// 0x00582228.
void Log::level(int l)
{
    m_level = l;
}

// 0x00582230.  A format longer than the buffer is cut at 4096 for the call.
void Log::write(int l, char *fmt, ...)
{
    char buf[4352];
    int copied = 0;
    char *copy = 0;
    char saved;
    if (fmt && (int)strlen(fmt) > 4096)
    {
        copied = 1;
        copy = strdup(fmt);
        saved = copy[4096];
        fmt = copy;
        copy[4096] = 0;
    }
    if (m_level >= l)
    {
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buf, fmt, ap);
    }
    if (copied)
    {
        strlen(fmt);
        copy[4096] = saved;
        free(copy);
    }
}

// 0x00582318.
void Log::close()
{
}
