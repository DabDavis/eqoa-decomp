// VIKeyboard -- SUPPORT 0x004A0E60 .. 0x004A1AC8, its own TU.
//
// The USB keyboard behind the same GetInput() as the pad.  Events arrive from
// the libkbd driver, get translated and dead-key-combined, and land in a
// 128-entry queue that Dequeue drains from the FRONT -- every read shuffles
// the rest down by one.
#include "vi.h"

extern "C" int  printf(const char *fmt, ...);
extern "C" int  lgKbmInit(int a, int b, int c);
extern "C" void lgKbMPrepareForRebootIOP();
extern "C" int  lgKbdClose(int fd);

int gKeyboardAttached;

static VIConsoleKey sKeyQueue[128];

int VIStartKeyboardDriver()
{
    if (lgKbmInit(1, 0, 1))
        return -1;
    return 0;
}

int VIStopKeyboardDriver()
{
    lgKbMPrepareForRebootIOP();
}

VIKeyboard::VIKeyboard()
{
    m_fd = -1;
}

VIKeyboard::~VIKeyboard()
{
}

int VIKeyboard::Init()
{
    gKeyboardAttached = 0;
    printf("VIKeyboard::Init\n");
    m_fd = -1;
    m_queue = sKeyQueue;
    m_numQueued = 0;
    m_x0 = 0;
    // Phase 16's hoist: the store emitted FIRST after the join is the source's
    // LAST store of that constant, so +0x210 closes the block.
    m_x210 = 0;
    return 0;
}

void VIKeyboard::Free()
{
    printf("VIKeyboard::Free\n");
    if (m_fd != -1)
    {
        lgKbdClose(m_fd);
        m_fd = -1;
    }
}

void VIKeyboard::QueueKeyEvent(VIConsoleKey *k)
{
    // The count is read ONCE -- the same lever as VISpellEffectDriver's
    // AddSecondaryTarget in phase 35: re-reading the member for the increment
    // costs 13 of 15 words and moves `this` onto its own register.
    int n = m_numQueued;

    if (n < 128)
    {
        m_queue[n] = *k;
        m_numQueued = n + 1;
    }
}

int VIKeyboard::DequeueKeyEvent(VIConsoleKey *k)
{
    int i;

    // Phase 7's if-direction: the DRAIN is the `if` body and the empty case is
    // the tail, which is what puts `return 0` at the very end of the function.
    // Written as an early `== 0` return gcc keeps it inline behind a
    // branch-likely and 27 of 29 words move.
    if (m_numQueued != 0)
    {
        *k = m_queue[0];
        m_numQueued--;

        for (i = 0; i < m_numQueued; i++)
            m_queue[i] = m_queue[i + 1];

        return 1;
    }
    return 0;
}
