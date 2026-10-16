// viwndpresents.cpp (name ours) -- the presents splash.
#include "setup.h"

VIWndPresents::VIWndPresents()
{
}

VIWndPresents::~VIWndPresents()
{
}

int VIWndPresents::Init()
{
    return 0;
}

void VIWndPresents::Free()
{
}

void VIWndPresents::OnDrawSelf()
{
    VIUI *ui = UI();
    float t = (int)(VITimer::GetTime() - m_time) * 0.001f;
    float alpha = 1.0f;
    if (t > 7.5f)
        SetFocus(0, true);
    else if (t > 6.0f)
        alpha = 1.0f - (t - 6.0f);
    else if (t < 1.0f)
        alpha = t;
    if (t < 7.0f)
    {
        ui->Begin2D();
        ui->Blit(VIHashResourceID("presents_l.tga"), VIVect2I(0, 0), (VIAlphaBlendMode)3, alpha, 0);
        ui->Blit(VIHashResourceID("presents_r.tga"), VIVect2I(320, 0), (VIAlphaBlendMode)3, alpha, 0);
        ui->End2D();
    }
}

// START skips to the fade-out.
void VIWndPresents::OnKeyDown(int key)
{
    if (key == 144)
    {
        if ((int)(VITimer::GetTime() - m_time) > 5000)
            m_time = VITimer::GetTime() - 7000;
    }
}

void VIWndPresents::OnKeyUp(int key)
{
}

void VIWndPresents::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295 && p2 == (int)this)
        m_time = VITimer::GetTime();
}
