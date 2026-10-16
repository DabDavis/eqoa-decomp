// ps2port.cpp (name ours) -- SETUP 0x0057CDD8 .. 0x0057CEB0: the Win32 and
// POSIX calls the PC patcher made, stood in for on the PS2.  The directory
// calls all claim success and do nothing; the names and argument types are the
// 2002 betas' exports.
#include "patcher.h"

// 0x0057CDD8.
int rmdir(char *path)
{
    return 1;
}

// 0x0057CDE0.
int chdir(const char *path)
{
    return 1;
}

// 0x0057CDE8.
int mkdir(char *path)
{
    return 1;
}

// 0x0057CDF0.
int mkdir(char *path, int mode)
{
    return 1;
}

// 0x0057CDF8.
char *getcwd(char *buf, int size)
{
    return "/";
}

// 0x0057CE08.
char *_strlwr(char *s)
{
    if (!s)
        return 0;
    for (int i = 0; s[i]; i++)
        s[i] = tolower(s[i]);
    return s;
}

// 0x0057CE60.  The time is ignored: the thread sleeps until woken.  It returns
// a value, which the callers' register choice shows (a void callee frees v0).
int Sleep(int ms)
{
    return SleepThread();
}

// 0x0057CE80.
const char *GetCommandLine()
{
    return "host0:/main.elf";
}

// 0x0057CE90.  There is no clock: every time is 0.  The 16-byte frame with
// nothing in it is an unused struct local -- gcc 2.95 gives a BLKmode local
// its slot whether or not anything touches it.
extern "C" time_t time(time_t *t)
{
    struct timeval { long tv_sec; long tv_usec; } tv;
    *t = 0;
    return 0;
}

// 0x0057CEA8.
extern "C" int link(const char *from, const char *to)
{
    return 1;
}
