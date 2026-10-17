#line 1 "C:/code-frontiers-gm/game/VIWndQuestLog.cpp"
// VIWndQuestLog (name RTTI's) -- CLIENT 0x00C860D0 .. 0x00C87820: the quest journal.
// Each quest's title is a TextBox of as many rows as it wraps to; the list scrolls so
// the selection is always whole on the page.  X asks (the message box, word 574) and a
// yes sends 0x007D with the quest's index.  The VIMap<int, int> and VIArray<VIItemStat>
// copies between Title and __tf are template instantiations this TU does not write.
#include "game.h"

// 0x00C860D0.
VIWndQuestLog::VIWndQuestLog()
{
    m_active = 1;
    m_sel = 0;
    m_top = 0;
    m_pageEnd = 0;
    m_last = 0;
    m_mbox = 0;
}

// 0x00C86130.
VIWndQuestLog::~VIWndQuestLog()
{
    Free();
}

// 0x00C86190.  The confirmation box, hidden.  Any failure is -1.
int VIWndQuestLog::Init()
{
    int ret;
    m_mbox = new VIWndMBox;
    if (!m_mbox)
        ret = -1;
    else
    {
        m_mbox->SetVisible(false);
        m_mbox->SetAppPtr(GetAppPtr());
        AddChild(m_mbox);
        ret = m_mbox->Init(1, 1);
        if (ret)
            ret = -1;
        else
        {
            m_mbox->SetPos(VIVect2I(100, 115));
            m_active = 1;
            m_sel = 0;
            m_top = 0;
            m_pageEnd = 0;
            m_last = 0;
        }
    }
    return ret;
}

// 0x00C86248.
int VIWndQuestLog::Free()
{
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    m_active = 1;
    m_sel = 0;
    m_top = 0;
    m_pageEnd = 0;
    m_last = 0;
    return 0;
}

// 0x00C862C0.
void VIWndQuestLog::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    ClampSel();
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(641, 332), (VIAlphaBlendMode)0, 1.0f);
    m_window.Draw();
    unsigned short buf[64];
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", m_window.T(516), m_window.T(161));
    m_window.Text(buf, 40, 13, (VIUICOLOR)0, 0);
    float t;
    if (m_top > 0)
    {
        t = (float)m_top / (float)(Count() - m_pageEnd - 1);
        t = VIMIN(1.0f, VIMAX(0.0f, t));
    }
    else
        t = 0.0f;
    m_window.DrawFrameVScroll(30, 0, m_window.Width() - 60, 12, t);
    int y = 0;
    unsigned short title[128];
    for (int i = m_top < 0 ? 0 : m_top; i < 8 && i < Count(); i++)
    {
        title[0] = 0;
        Title(i, title);
        int cut = 0;
        int rows = m_window.CountLines(title, m_window.Width() - 80);
        if (y + rows >= 13)
        {
            cut = 1;
            rows = 12 - y;
        }
        int end = y + rows;
        if (end < 14 && rows > 0)
        {
            if (i == m_sel)
                m_window.TickR(45, y, m_active);
            m_window.TextBox(title, 45, y, m_window.Width() - 80, rows, (VIUICOLOR)(i == m_sel), 0);
            y = end;
            m_last = i;
            if (!m_top)
                m_pageEnd = i;
            if (i == m_sel && cut)
                m_top++;
        }
    }
    ui->End2D();
    if (m_keys.Pulse(138))
        Prev();
    if (m_keys.Pulse(139))
        Next();
}

// 0x00C865E8.
void VIWndQuestLog::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 19:
    case 138:
        m_keys.Press(138);
        break;
    case 20:
    case 139:
        m_keys.Press(139);
        break;
    case 13:
    case 134:
        Select();
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        app->ClosePanel6DC();
        break;
    }
}

// 0x00C866F8.
void VIWndQuestLog::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        m_keys.Release(138);
        break;
    case 20:
    case 139:
        m_keys.Release(139);
        break;
    }
}

// 0x00C86770.  295: the focus moved (p1 from, p2 to); 268/260: the box was answered.
void VIWndQuestLog::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this)
        {
            m_active = 1;
            if ((VIWnd *)p1 != m_mbox)
            {
                m_window.Init(&app->m_ui, VIVect2I(0, 0), VIVect2I(320, 180), 550, 14, 0);
                m_window.SetTitle(m_window.T(573));
                m_window.Open(true);
                m_pageEnd = 0;
                m_last = 0;
                m_keys.Reset();
            }
        }
        if ((VIWnd *)p1 == this)
        {
            m_active = 0;
            m_keys.Reset();
        }
        break;
    case 268:
        if (w == m_mbox)
        {
            app->Send007D(p1);
            m_mbox->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    case 260:
        if (w == m_mbox)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    }
}

// 0x00C868F8.
void VIWndQuestLog::Open()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_sel = 0;
    m_top = 0;
    m_mbox->SetVisible(false);
    app->SetActiveWindow(this);
}

// 0x00C86938.
void VIWndQuestLog::HideBox()
{
    m_mbox->SetVisible(false);
}

// 0x00C86948.  A quest went away: keep the selection on the list.
void VIWndQuestLog::ClampSel()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel >= Count())
    {
        m_sel = m_sel - 1 < 0 ? 0 : m_sel - 1;
        if (m_sel == m_top - 1)
            m_top = m_sel < 0 ? 0 : m_sel;
    }
}

// 0x00C869B8.
void VIWndQuestLog::Next()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_window.ResetTickPulse();
    if (m_sel >= Count() - 1)
    {
        m_sel = Count() - 1;
        app->PlaySound(9);
    }
    else
    {
        m_sel++;
        app->PlaySound(11);
        if (m_sel > m_last)
            m_top++;
    }
}

// 0x00C86A60.
void VIWndQuestLog::Prev()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_window.ResetTickPulse();
    if (m_sel <= 0)
    {
        m_sel = 0;
        app->PlaySound(9);
    }
    else
    {
        m_sel--;
        app->PlaySound(11);
        if (m_sel == m_top - 1)
            m_top = m_sel < 0 ? 0 : m_sel;
    }
}

// 0x00C86AF0.  X on a quest: ask first.
void VIWndQuestLog::Select()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (Count() > 0)
    {
        m_mbox->SetText((unsigned short)574);
        m_mbox->SetParam1(m_sel);
        m_mbox->SetVisible(true);
        app->SetActiveWindow(m_mbox);
        app->PlaySound(13);
    }
    else
        app->PlaySound(9);
}

// 0x00C86B80.
int VIWndQuestLog::Count()
{
    return ((VIClient *)GetAppPtr())->GetQuestCount();
}

// 0x00C86BA8.
int VIWndQuestLog::Title(int i, unsigned short *buf)
{
    return ((VIClient *)GetAppPtr())->GetQuestTitle(i, buf);
}
