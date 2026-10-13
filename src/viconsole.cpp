// VIConsole -- SUPPORT 0x004A0A68 .. 0x004A0C20.  The first two functions are
// the disc-flags accessors (phase 61): nothing named sits between them and
// VIConsole::Init, and the code ahead of them is VICSFFile's template tail.
#include "vi.h"

static unsigned int s_discFlags;

unsigned int VIGetDiscFlags()
{
    return s_discFlags;
}

void VISetDiscFlags(unsigned int f)
{
    s_discFlags = f;
}

extern "C" void *memset(void *d, int c, unsigned int n);

int VIConsole::Init()
{
    VIConsolePad pad;

    m_pad.Init(0, 0);

    if (m_pad.Read(pad))
        m_padPresent = 1;
    else
        m_padPresent = 0;

    m_keyboard.Init();

    m_keyReady = 1;
    m_padReady = 1;

    return 0;
}

int VIConsole::Close()
{
    m_keyboard.Free();
    m_pad.Close();

    return 0;
}

int VIConsole::Process(long t)
{
    m_pad.Process(t);

    return 0;
}

int VIConsole::GetInput(int *type, void *data, int size)
{
    int got = 0;

    if (m_keyReady == 1)
    {
        if (m_keyboard.Read(*(VIConsoleKey *)data) == 1)
        {
            *type = 2;
            got = 1;
        }
        else
        {
            m_keyReady = 0;
        }
    }
    else
    {
        m_keyReady = 1;
    }

    if (!got)
    {
        if (m_padReady == 1)
        {
            if (!m_pad.Read(*(VIConsolePad *)data))
            {
                memset((char *)data + 4, 0, 80);
                m_padPresent = 0;
            }
            else
            {
                m_padPresent = 1;
            }

            *type = 4;
            got = 1;
            m_padReady = 0;
        }
        else
        {
            m_padReady = 1;
        }
    }

    return !got;
}
