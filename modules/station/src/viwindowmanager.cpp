// viwindowmanager.cpp (name ours) -- STATION 0x002B9240 .. 0x002B95B8.  The
// window tree's driver, as the betas had it: SETUP's (viwindowmanager.cpp
// there) before Frontiers added GetRoot and rewrote Update.
#include "station.h"

// 0x002B9240.
VIWindowManager::VIWindowManager()
{
}

// 0x002B9278.
VIWindowManager::~VIWindowManager()
{
}

// 0x002B92A0.
int VIWindowManager::Init(VIPad *pad, VIKeyboard *kb)
{
    m_pad = pad;
    m_keyboard = kb;
    memset(&m_lastPad, 0, sizeof(m_lastPad));
    return 0;
}

// 0x002B92D8.
void VIWindowManager::Free()
{
}

// 0x002B92E0.
void VIWindowManager::SetRoot(VIWnd *w)
{
    m_root = w;
}

// 0x002B92E8.  Nothing without a focus; otherwise the keyboard, the pad's
// changed buttons as key codes 128 + i, and the root's draw.
int VIWindowManager::Update()
{
    if (!mFocus)
        return 0;
    VIConsoleKey key;
    if (m_keyboard->Read(key) == 1)
    {
        if (key.m_b)
            mFocus->OnKeyDown(key.m_a);
        else
            mFocus->OnKeyUp(key.m_a);
    }
    VIConsolePad pad;
    if (m_pad->Read(pad))
    {
        for (int i = 0; i < 20; i++)
        {
            if (((&m_lastPad.m_rx)[i] != 0.0f) != ((&pad.m_rx)[i] != 0.0f))
            {
                if ((&pad.m_rx)[i] != 0.0f)
                    mFocus->OnKeyDown(i + 128);
                else
                    mFocus->OnKeyUp(i + 128);
            }
        }
        m_lastPad = pad;
    }
    if (m_root && m_root->GetVisible())
        m_root->OnDraw();
    return 1;
}
