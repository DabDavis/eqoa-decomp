#line 1 "C:/code-frontiers-gm/game/VIWndIgnoreList.cpp"
// viwndignorelist.cpp -- CLIENT 0x00C60780 .. 0x00C62310:
// VIWndIgnoreList, the ignore list.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// The member menu's flag at +0x254, through an int (as VIWndGuild.cpp's).
static inline void SetFlag(VIWndMiniMenu *m, int v) { m->m_kind = v; }

// 0x00C60780.
VIWndIgnoreList::VIWndIgnoreList()
{
    m_mbox = 0;
    m_menu = 0;
    m_sel = 0;
    m_top = 0;
    m_count = 0;
    m_focus = 1;
}

// 0x00C607E8.
VIWndIgnoreList::~VIWndIgnoreList()
{
    Free();
}

// 0x00C60858.  The confirmation box and the member menu, hidden.
int VIWndIgnoreList::Init()
{
    int err;
    m_mbox = new VIWndMBox;
    if (!m_mbox)
    {
        err = -1;
        goto done;
    }
    m_mbox->SetVisible(false);
    m_mbox->SetAppPtr(GetAppPtr());
    AddChild(m_mbox);
    err = m_mbox->Init(1, 1);
    if (err)
    {
        err = -1;
        goto done;
    }
    m_mbox->SetPos(VIVect2I(80, 115));
    m_menu = new VIWndMiniMenu;
    if (!m_menu)
    {
        err = -1;
        goto done;
    }
    m_menu->SetVisible(false);
    m_menu->SetAppPtr(GetAppPtr());
    SetFlag(m_menu, 1);
    AddChild(m_menu);
    m_sel = 0;
    m_top = 0;
    m_count = 0;
    m_focus = 1;
    m_ids.Init(0);
done:
    return err;
}

// 0x00C60970.
int VIWndIgnoreList::Free()
{
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    if (m_menu)
    {
        RemoveChild(m_menu);
        delete m_menu;
        m_menu = 0;
    }
    m_sel = 0;
    m_top = 0;
    m_count = 0;
    m_focus = 1;
    m_ids.Init(0);
    return 0;
}

// 0x00C60A28.  Twelve names a page.
void VIWndIgnoreList::OnDrawSelf()
{
    unsigned short buf[64];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIClientPart38300 *list = app->GetP38300();
    Clamp();
    ui->Begin2D();
    VIVect2I a;
    VIVect2I b;
    VIVect2I c;
    VIVect2I d;
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    VIColor32F hl(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f);
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    m_window.SetTitle(m_window.T((unsigned short)612));
    m_window.Draw();
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 212), Word(ui, 161));
    m_window.Text(buf, 20, 13, black, 0);
    int count = list->Count();
    list->GetIndices(m_ids);
    float t;
    if (count > 12)
        t = (float)m_top / (float)(count - 12);
    else
        t = 0.0f;
    int shown = 0;
    m_window.DrawFrameVScroll(30, 0, m_window.Width() - 70, 12, t);
    int row = 0;
    for (int n = m_top; n < count; n = shown + m_top)
    {
        if (n == m_sel)
            m_window.TickR(45, row, m_focus);
        char name[24];
        memset(name, 0, sizeof(name));
        shown++;
        list->GetName(m_ids[n], name);
        m_window.TextBox(m_window.T(name), 50, row, m_window.Width() - 95, 1, (VIUICOLOR)(n == m_sel), 2);
        row++;
        if (shown >= 12)
            break;
    }
    ui->End2D();
    if (m_flash.Pulse(138))
        Move(-1);
    if (m_flash.Pulse(139))
        Move(1);
}

// 0x00C60D78.
void VIWndIgnoreList::OnKeyDown(int key)
{
    char name[32];
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart38300 *list = app->GetP38300();
    m_window.ResetTickPulse();
    switch (key)
    {
    case 19:
    case 138:
        m_flash.Press(138);
        break;
    case 20:
    case 139:
        m_flash.Press(139);
        break;
    case 13:
    case 134:
        if (m_sel < list->Count())
        {
            app->PlaySound(13);
            list->GetName(m_ids[m_sel], name);
            ShowMenu(name);
        }
        else
            app->PlaySound(10);
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        list->ClosePanel();
        break;
    }
}

// 0x00C60F08.
void VIWndIgnoreList::OnKeyUp(int key)
{
    switch (key)
    {
    case 19:
    case 138:
        m_flash.Release(138);
        break;
    case 20:
    case 139:
        m_flash.Release(139);
        break;
    }
}

// 0x00C60F80.  261 is the member menu's choice, 268 and 260 the confirmation box's yes and
// no (p1 the operation, p2 the pool index).
void VIWndIgnoreList::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIClientPart38300 *list = app->GetP38300();
    switch (msg)
    {
    case 261:
        w->SetVisible(false);
        app->PlaySound(13);
        if (p1 == 0x0B053004)
        {
            unsigned short buf[128];
            VIString::snprintf16(buf, sizeof(buf), Word(ui, 614), m_menu->m_title);
            m_mbox->SetText(buf);
            m_mbox->SetVisible(true);
            m_mbox->SetParam1(p1);
            m_mbox->SetParam2(m_ids[m_sel]);
            app->SetActiveWindow(m_mbox);
        }
        break;
    case 268:
        m_mbox->SetVisible(false);
        if (p1 == 0x0B053004)
        {
            list->RemoveAt(p2);
            app->SetActiveWindow(this);
        }
        else
            app->SetActiveWindow(this);
        break;
    case 260:
        if (w == m_mbox)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        else if (w == m_menu)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    case 295:
        if ((VIWnd *)p2 == this)
        {
            m_focus = 1;
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 340, 14, 0);
            m_window.Open(true);
            m_flash.Reset();
        }
        else if ((VIWnd *)p1 == this)
        {
            m_focus = 0;
            m_flash.Reset();
        }
        break;
    }
}

// 0x00C611B0.
void VIWndIgnoreList::Move(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int count = app->GetP38300()->Count();
    if (m_sel + d >= count && count > 0)
    {
        app->PlaySound(9);
        m_sel = count - 1;
    }
    else if (m_sel + d < 0 || count == 0)
    {
        app->PlaySound(9);
        m_sel = 0;
    }
    else
    {
        m_sel += d;
        app->PlaySound(11);
    }
    if (m_top + 11 < m_sel)
        m_top = m_sel - 11;
    if (m_sel < m_top)
        m_top = m_sel;
}

// 0x00C612A8.
void VIWndIgnoreList::Clamp()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart38300 *list = app->GetP38300();
    int sel = m_sel;
    int count = list->Count();
    if (count - 12 < m_top && count > 12)
        m_top--;
    else if (count == 12)
        m_top = 0;
    if (m_top < 0)
        m_top = 0;
    if (m_sel >= count)
        m_sel--;
    if (m_sel < 0)
        m_sel = 0;
    if (m_sel != sel || count != m_count)
        m_count = count;
}

// 0x00C61378.
void VIWndIgnoreList::Close()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_mbox->SetVisible(false);
    m_menu->SetVisible(false);
    m_sel = 0;
    m_top = 0;
    app->SetActiveWindow(this);
}

// 0x00C613C0.
void VIWndIgnoreList::Reset()
{
    m_mbox->SetVisible(false);
    m_menu->SetVisible(false);
}

// 0x00C613D8.
void VIWndIgnoreList::ShowMenu(const char *name)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (!m_menu)
    {
        m_menu = new VIWndMiniMenu;
        m_menu->SetAppPtr(GetAppPtr());
        SetFlag(m_menu, 1);
        AddChild(m_menu);
        if (!m_menu)
            return;
    }
    m_menu->SetMenu(&gIgnoreMemberMenu);
    m_menu->SetPos(VIVect2I(170, 50));
    if (name)
        m_menu->SetTitle(name);
    else
        m_menu->SetTitle("");
    m_menu->ResetScroll();
    m_menu->SetVisible(true);
    app->SetActiveWindow(m_menu);
}
