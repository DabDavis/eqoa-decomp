// VITimer -- SUPPORT 0x003E8400 .. 0x003E8508.
#include "vi.h"

extern "C" int AddIntcHandler(int cause, int (*handler)(int), int next);
extern "C" int RemoveIntcHandler(int cause, int handler_id);
extern "C" int EnableIntc(int cause);
extern "C" int DisableIntc(int cause);

#define T0_MODE  (*(volatile unsigned int *)0x10000010)
#define T0_COMP  (*(volatile unsigned int *)0x10000020)

long VITimer::SystemTime;
int  VITimer::TimerMode;
int  VITimer::HandlerID;
int  VITimer::Ready;

int VITimer::Init()
{
    SystemTime = 0;
    DisableIntc(9);

    TimerMode = 1473;
    T0_MODE = 1473;
    T0_COMP = 18750;

    HandlerID = AddIntcHandler(9, Handler, -1);
    EnableIntc(9);

    Ready = 1;
    return 1;
}

void VITimer::Close()
{
    Ready = 0;
    DisableIntc(9);
    RemoveIntcHandler(9, HandlerID);
    EnableIntc(9);
}

int VITimer::Handler(int ca)
{
    SystemTime += 2;
    T0_MODE = TimerMode;

    __asm__ __volatile__ ("sync\n\tei\n");
    return 0;
}

long VITimer::GetTime()
{
    return SystemTime;
}
