// viwindowmanager.cpp (name ours) -- SETUP 0x0056D170 .. 0x0056D508.
#include "setup.h"


// 0x0056D170.
VIWindowManager::VIWindowManager()
{
}

// 0x0056D1A8.
VIWindowManager::~VIWindowManager()
{
}

// 0x0056D1D0.
int VIWindowManager::Init(VIPad *pad, VIKeyboard *kb)
{
    m_pad = pad;
    m_keyboard = kb;
    memset(&m_lastPad, 0, sizeof(m_lastPad));
    return 0;
}

// 0x0056D208.
void VIWindowManager::Free()
{
}

// 0x0056D210.  Not in the 2002 betas.
VIWnd *VIWindowManager::GetRoot()
{
    return m_root;
}

// 0x0056D218.
void VIWindowManager::SetRoot(VIWnd *w)
{
    m_root = w;
}

// 0x0056D220.  Returns whether any window still has the focus.
int VIWindowManager::Update()
{
    if (mFocus)
    {
        if (m_keyboard)
        {
            VIConsoleKey key;
            if (m_keyboard->Read(key) == 1)
            {
                if (key.m_b)
                {
                    if (mFocus)
                        mFocus->OnKeyDown(key.m_a);
                }
                else
                {
                    if (mFocus)
                        mFocus->OnKeyUp(key.m_a);
                }
            }
        }
        if (m_pad)
        {
            VIConsolePad pad;
            if (m_pad->Read(pad))
            {
                for (int i = 0; i < 20; i++)
                {
                    if (((&m_lastPad.m_rx)[i] != 0.0f) != ((&pad.m_rx)[i] != 0.0f))
                    {
                        if ((&pad.m_rx)[i] != 0.0f)
                        {
                            if (mFocus)
                                mFocus->OnKeyDown(i + 128);
                        }
                        else
                        {
                            if (mFocus)
                                mFocus->OnKeyUp(i + 128);
                        }
                    }
                }
                m_lastPad = pad;
            }
        }
    }
    if (m_root && m_root->GetVisible())
        m_root->OnDraw();
    return mFocus != 0;
}
