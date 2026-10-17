#line 1 "C:/code-frontiers-gm/game/VIWndBuddy.cpp"
// viwndbuddy.cpp -- CLIENT 0x00C1C0E0 .. 0x00C1E268: the buddy list.  Ten rows of the
// client's buddies; Enter opens a mini menu on one (a gram, remove, ...); an incoming
// buddy request is put up in the message box, and its answer goes back to the server.
#include "game.h"

// One WORDS.TXT entry drawn at (x, y).
static inline void DrawWord(VIUI *ui, VIFont *font, int id, int x, int y)
{
    font->DrawString(ui->m_raster, ui->m_dict,
                     (const unsigned short *)ui->m_words.GetString(id, ui->m_language), x, y);
}

// 0x00C1C0E0.
VIWndBuddy::VIWndBuddy()
{
    m_mbox = 0;
    m_menu = 0;
    m_p30 = 0;
    m_sel = 0;
    m_top = 0;
}

// 0x00C1C138.
VIWndBuddy::~VIWndBuddy()
{
    Free();
}

// 0x00C1C198.
int VIWndBuddy::Init()
{
    int r;
    m_mbox = new VIWndMBox;
    if (!m_mbox)
        goto fail;
    m_mbox->SetVisible(false);
    m_mbox->SetAppPtr(GetAppPtr());
    AddChild(m_mbox);
    if ((r = m_mbox->Init(1, 1)) != 0)
        goto fail;
    m_mbox->m_pos = VIVect2I(80, 115);
    m_menu = new VIWndMiniMenu;
    if (!m_menu)
        goto fail;
    m_menu->SetVisible(false);
    m_menu->SetAppPtr(GetAppPtr());
    m_menu->m_kind = 1;
    AddChild(m_menu);
    m_compose = new VIWndComposeGram;
    if (!m_compose)
    {
fail:
        r = -1;
    }
    else
    {
        m_compose->SetAppPtr(GetAppPtr());
        m_compose->Init(1);
        m_compose->SetVisible(false);
        AddChild(m_compose);
        m_p30 = 0;
        ResetSel();
    }
    return r;
}

// 0x00C1C2E8.
int VIWndBuddy::Free()
{
    if (m_compose)
    {
        RemoveChild(m_compose);
        m_compose->Free();
        delete m_compose;
    }
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
    m_p30 = 0;
    ResetSel();
    return 0;
}

// 0x00C1C3C8.  Ten rows of name, online, race, class, level and world over the column
// headers; the cursor row is blue with the pulsing arrow.  The held arrows repeat.
void VIWndBuddy::OnDrawSelf()
{
    unsigned short buf[64];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    if (m_p30 != 1)
        return;
    VIFont *font = &ui->m_font;
    Clamp();
    ui->Begin2D();
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    VIColor32F blue(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f);
    VIVect2I u0;
    VIVect2I u1;
    VIVect2I u2;
    VIVect2I a(-1, -1);
    VIVect2I b(641, 332);
    font->GetHeight(ui->m_raster, ui->m_dict);
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), b, (VIAlphaBlendMode)3, 1.0f);
    m_window.Draw();
    font->SetTextColor(black);
    VIVect2I p = m_window.RowCenter(13);
    p.x = m_window.TopLeft().x + 40;
    p.y = p.y - 2;
    ui->Blit((VIUITexId)66, VIVect2I(p.x, p.y - 6));
    DrawWord(ui, font, 212, p.x + 20, p.y - 10);
    p.x += 100;
    ui->Blit((VIUITexId)65, VIVect2I(p.x, p.y - 6));
    DrawWord(ui, font, 161, p.x + 20, p.y - 10);
    DrawWord(ui, font, 509, m_window.TopLeft().x + 40, m_window.RowCenter(0).y - 4);
    DrawWord(ui, font, 521, m_window.TopLeft().x + 170, m_window.RowCenter(0).y - 4);
    DrawWord(ui, font, 510, m_window.TopLeft().x + 240, m_window.RowCenter(0).y - 4);
    DrawWord(ui, font, 511, m_window.TopLeft().x + 302, m_window.RowCenter(0).y - 4);
    DrawWord(ui, font, 503, m_window.TopLeft().x + 364, m_window.RowCenter(0).y - 4);
    DrawWord(ui, font, 512, m_window.TopLeft().x + 426, m_window.RowCenter(0).y - 4);
    for (int i = 0; i < 10; i++)
    {
        int n = i + m_top;
        p.x = m_window.TopLeft().x + 40;
        p.y = m_window.RowCenter(i + 2).y - 10;
        if (n == m_sel)
        {
            ui->m_font.SetTextColor(blue);
            ui->BlitEx((VIUITexId)89, VIVect2I(p.x - 16, p.y + 4), (VIAlphaBlendMode)3, m_window.TickPulse(), 0);
        }
        else
            ui->m_font.SetTextColor(black);
        if (n >= app->GetBuddyCount())
            break;
        ui->m_font.DrawString(ui->m_raster, ui->m_dict, app->GetBuddyName(n), m_window.TopLeft().x + 40, p.y);
        if (app->GetBuddyOnline(n) > 0)
            DrawWord(ui, &ui->m_font, 21, m_window.TopLeft().x + 170, p.y);
        else
            DrawWord(ui, &ui->m_font, 22, m_window.TopLeft().x + 170, p.y);
        const VIRaceDef *race = GetRace(app->GetBuddyRace(n));
        if (!race)
            continue;
        DrawWord(ui, &ui->m_font, (unsigned short)race->m_shortStrID, m_window.TopLeft().x + 240, p.y);
        const VIClassDef *cls = GetClass(app->GetBuddyClass(n));
        if (!cls)
            continue;
        DrawWord(ui, &ui->m_font, (unsigned short)cls->m_shortStrID, m_window.TopLeft().x + 302, p.y);
        VIString::snprintf16(buf, 64, "%u", app->GetBuddyLevel(n));
        ui->m_font.DrawString(ui->m_raster, ui->m_dict, buf, m_window.TopLeft().x + 364, p.y);
        if (!app->GetBuddyWName(n))
            continue;
        VIString::snprintf16(buf, 64, "%S", app->GetBuddyWName(n));
        m_window.TextBox(buf, m_window.TopLeft().x + 406, i + 2, m_window.Width() - 446, 1, n == m_sel ? blue : black, 2);
    }
    ui->m_font.SetTextColor(black);
    ui->End2D();
    if (m_flash.Pulse(138))
        Move(-1);
    if (m_flash.Pulse(139))
        Move(1);
}

// 0x00C1CD70.
void VIWndBuddy::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        app->DeclineBuddyRequest();
        break;
    case 13:
    case 134:
        if (m_sel < app->GetBuddyCount())
        {
            app->PlaySound(13);
            ShowMenu(app->GetBuddyName(m_sel));
        }
        else
            app->PlaySound(10);
        break;
    case 19:
    case 138:
        m_flash.Press(138);
        break;
    case 20:
    case 139:
        m_flash.Press(139);
        break;
    }
}

// 0x00C1CEC0.
void VIWndBuddy::OnKeyUp(int key)
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

// 0x00C1CF38.  45 seconds to answer.
void VIWndBuddy::ShowRequest()
{
    VIClient *app = (VIClient *)GetAppPtr();
    char name[64];
    unsigned short text[512];
    app->GetBuddyRequestName(name);
    VIUI *ui = &app->m_ui;
    VIString::snprintf16(text, 512, "%S\n%s\n\n%S",
                         (const unsigned short *)ui->m_words.GetString(519, ui->m_language), name,
                         (const unsigned short *)ui->m_words.GetString(520, ui->m_language));
    m_p30 = 0;
    m_menu->SetVisible(false);
    m_mbox->Init(1, 1);
    m_mbox->m_timeout = app->m_p07568 + 45000;
    m_mbox->SetText(text);
    m_mbox->SetVisible(true);
    m_mbox->m_p1 = 0;
    app->SetActiveWindow(m_mbox);
}

// 0x00C1D040.
void VIWndBuddy::Close()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_p30 = 1;
    m_mbox->SetVisible(false);
    m_menu->SetVisible(false);
    m_sel = 0;
    m_top = 0;
    app->SetActiveWindow(this);
}

// 0x00C1D090.
void VIWndBuddy::HideChildren()
{
    m_mbox->SetVisible(false);
    m_menu->SetVisible(false);
    m_p30 = 0;
}

// 0x00C1D0A8.
void VIWndBuddy::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 309:   // the gram window is done
        m_compose->SetVisible(false);
        app->SetActiveWindow(this);
        break;
    case 295:   // opened
        if ((VIWndBuddy *)p2 == this)
        {
            m_window.Init(&app->m_ui, VIVect2I(0, 0), VIVect2I(320, 180), 600, 14, 0);
            m_window.SetTitle(m_window.T((unsigned short)513));
            m_window.Open(true);
            m_flash.Reset();
        }
        if ((VIWndBuddy *)p1 == this)
            m_flash.Reset();
        break;
    case 261:   // the mini menu's choice
        w->SetVisible(false);
        app->PlaySound(13);
        switch (p1)
        {
        case 0x0B050001:
            GetParent()->OnMessage(this, 304, m_sel, 0);
            break;
        case 0x0B050002:
        {
            m_compose->SetVisible(true);
            app->SetActiveWindow(m_compose);
            m_compose->Refresh();
            unsigned short to[256];
            VIString::str8to16(to, sizeof(to), app->GetBuddyName(m_sel));
            m_compose->SetTo(to);
            break;
        }
        case 0x0B050003:
            m_mbox->SetText((unsigned short)518);
            m_mbox->SetVisible(true);
            m_mbox->m_p1 = 1;
            app->SetActiveWindow(m_mbox);
            break;
        }
        break;
    case 268:   // the message box: yes
        if (w == m_mbox)
        {
            w->SetVisible(false);
            switch (p1)
            {
            case 0:
                app->AnswerBuddyRequest();
                break;
            case 1:
                app->RequestBuddy(m_sel);
                app->SetActiveWindow(this);
                break;
            }
        }
        break;
    case 260:   // back
        if (w == m_mbox)
        {
            w->SetVisible(false);
            switch (p1)
            {
            case 0:
                app->RejectBuddyRequest();
                break;
            case 1:
                app->SetActiveWindow(this);
                break;
            }
        }
        else if (w == m_menu)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        else if (w == m_compose)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    }
}

// 0x00C1D398.
void VIWndBuddy::Move(int delta)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int n = app->GetBuddyCount();
    if (m_sel + delta >= n && n > 0)
    {
        app->PlaySound(9);
        m_sel = n - 1;
    }
    else if (m_sel + delta < 0 || n == 0)
    {
        app->PlaySound(9);
        m_sel = 0;
    }
    else
    {
        m_sel = m_sel + delta;
        app->PlaySound(11);
    }
    if (m_top + 9 < m_sel)
        m_top = m_sel - 9;
    if (m_sel < m_top)
        m_top = m_sel;
    m_window.ResetTickPulse();
}

// 0x00C1D488.
void VIWndBuddy::Clamp()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int old = m_sel;
    int n = app->GetBuddyCount();
    if ((n - 10 < m_top && n >= 11) || m_sel >= n)
    {
        old = m_sel;
        m_top = m_top - 1;
        if (m_top < 0)
            m_top = 0;
        m_sel = m_sel - 1;
        if (m_sel < 0)
            m_sel = 0;
    }
    if (m_sel != old || n != m_count)
        m_count = n;
}

// 0x00C1D538.
void VIWndBuddy::ShowMenu(const char *name)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    int h = ui->m_font.GetHeight(ui->m_raster, ui->m_dict);
    if (!m_menu)
    {
        m_menu = new VIWndMiniMenu;
        m_menu->SetAppPtr(GetAppPtr());
        m_menu->m_kind = 1;
        AddChild(m_menu);
        if (!m_menu)
            return;
    }
    m_menu->SetMenu(&gBuddyMenu);
    m_menu->SetPos(VIVect2I(170, 50));
    m_menu->SetTitle(name);
    m_menu->ResetScroll();
    m_menu->SetVisible(true);
    app->SetActiveWindow(m_menu);
}
