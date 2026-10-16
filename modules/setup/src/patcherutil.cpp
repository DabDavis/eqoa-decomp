// patcherutil.cpp (name ours) -- SETUP 0x00589078 .. 0x00589828: the patcher's
// time formatters, FormatString, the temp-name probe, and the uthread shim the
// Win32 thread calls were ported onto.
#include "patcher.h"

// 0x00589078.  "h:mm:ss" with the blanks of %2d turned into zeros.
char *FormatSeconds(long secs)
{
    short hours = secs / 3600;
    short mins = (secs % 3600) / 60;
    short s = secs % 60;
    char *buf = (char *)malloc(32);
    sprintf(buf, "%2d:%2d:%2d", hours, mins, s);
    for (int i = 0; i < (int)strlen(buf); i++)
        if (buf[i] == ' ')
            buf[i] = '0';
    return buf;
}

// 0x00589180.  Rounded to the minute: 16 seconds or more counts as the next.
char *FormatSecondsToMinutes(long secs)
{
    short hours = secs / 3600;
    short mins = (secs % 3600) / 60;
    short s = secs % 60;
    if (s > 15)
    {
        mins++;
        if (mins == 60)
        {
            mins = 0;
            hours++;
        }
    }
    char *buf = (char *)malloc(32);
    if (hours == 0)
    {
        if (mins == 0)
            sprintf(buf, "< 1 minute");
        else if (mins == 1)
            sprintf(buf, "About 1 minute", hours, mins);
        else if (mins > 1)
            sprintf(buf, "%d minutes", mins);
    }
    else if (hours == 1)
    {
        if (mins == 0)
            sprintf(buf, "%d hr", hours);
        else if (mins == 1)
            sprintf(buf, "%d hr %d minute", hours, mins);
        else if (mins > 1)
            sprintf(buf, "%d hr %d minutes", hours, mins);
    }
    else if (hours > 1)
    {
        if (mins == 0)
            sprintf(buf, "%d hrs", hours);
        else if (mins == 1)
            sprintf(buf, "%d hrs %d minute", hours, mins);
        else if (mins > 1)
            sprintf(buf, "%d hrs %d minutes", hours, mins);
    }
    return buf;
}

// 0x00589378.  A 12-hour clock, "hh:mm:ssam".
char *FormatTime(long t)
{
    time_t tt = t;
    struct tm *tm = localtime(&tt);
    short sec = tm->tm_sec;
    short min = tm->tm_min;
    short hour = tm->tm_hour;
    char ampm[3] = "am";
    if (hour >= 12)
    {
        ampm[0] = 'p';
        if (hour >= 13)
            hour -= 12;
    }
    char *buf = (char *)malloc(32);
    sprintf(buf, "%2d:%2d:%2d%2s", hour, min, sec, ampm);
    for (int i = 0; i < (int)strlen(buf); i++)
        if (buf[i] == ' ')
            buf[i] = '0';
    return buf;
}

// 0x00589488.  "hh:mmam", rounded like FormatSecondsToMinutes.
char *FormatTimeMinutes(long t)
{
    time_t tt = t;
    struct tm *tm = localtime(&tt);
    short sec = tm->tm_sec;
    short min = tm->tm_min;
    short hour = tm->tm_hour;
    char ampm[3] = "am";
    if (sec > 15)
    {
        min++;
        if (min == 60)
        {
            min = 0;
            hour++;
            if (hour == 24)
                hour = 0;
        }
    }
    if (hour >= 12)
    {
        ampm[0] = 'p';
        if (hour >= 13)
            hour -= 12;
    }
    if (hour == 0)
        hour = 12;
    char *buf = (char *)malloc(32);
    sprintf(buf, "%2d:%2d%2s", hour, min, ampm);
    for (int i = 0; i < (int)strlen(buf); i++)
        if (buf[i] == ' ')
            buf[i] = '0';
    return buf;
}

// 0x005895D0.
char *FormatString(char *buf, char *fmt, ...)
{
    if (!buf)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    return buf;
}

// 0x00589630.  The first ~VPatchTemp<n><ext> that does not exist yet, created
// empty to claim it.
char *GetTempName(char *ext)
{
    static char name[2048];
    int i = 0;
    for (;;)
    {
        sprintf(name, "~VPatchTemp%d%s", i, ext);
        FILE *f = fopen(name, "r");
        if (f)
            fclose(f);
        else
        {
            f = fopen(name, "w");
            if (f)
            {
                fclose(f);
                return name;
            }
        }
        i++;
    }
}

// 0x005896F0.
int GetCurrentExecutableName(char *buf)
{
    sprintf(buf, "host0:/main.elf");
    return 1;
}

// 0x00589718.  The idle thread: keep the ready queue turning.
void defMain(void *arg)
{
    for (;;)
        RotateThreadReadyQueue(100);
}

// 0x00589740.
extern "C" void uthread_init(uthread *t)
{
    memset(t, 0, sizeof(uthread));
    t->id = -1;
    t->param.entry = 0;
    t->stack = memalign(64, 16384);
    t->param.stack = t->stack;
    t->param.stackSize = 16384;
    t->param.initPriority = 100;
    t->param.gpReg = _gp;
    t->param.option = 0;
}

// 0x005897B0.
extern "C" int uthread_create(uthread *t, void (*func)(void *), void *arg)
{
    t->param.entry = func;
    t->id = CreateThread(&t->param);
    return StartThread(t->id, arg);
}

// 0x005897F8.
extern "C" void uthread_destroy(uthread *t)
{
    TerminateThread(t->id);
    DeleteThread(t->id);
}
