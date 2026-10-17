#line 1 "C:/code-frontiers-gm/game/VIWndGuild.cpp"
// viwndguild.cpp -- CLIENT 0x00C5B958 .. 0x00C5DDF8: VIWndGuild, the guild roster
// (lane cl-wh).  OnMessage asserts on lines 447, 477 and 503, so the file is padded to
// keep them there.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// The member menu's flag at +0x254, through an int: the constant 1 is then an SImode
// register the stores after AddChild share (s0 across the call).  Stands for an inline
// setter of VIWndMiniMenu's (lane cl-wm's class), which has no address of its own.
static inline void SetFlag(VIWndMiniMenu *m, int v) { m->m_kind = v; }
// One roster cell, centred on (x, y).  The column's x goes through a parameter, so it is
// read before a word is looked up (s0 across GetString) and before the font's raster.
static inline void DrawCell(VIUI *ui, const unsigned short *s, int x, int y)
{
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, s, x, y, 1);
}

static inline void DrawCell(VIUI *ui, const char *s, int x, int y)
{
    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, s, x, y, 1);
}

static inline void DrawWord(VIUI *ui, int id, int x, int y)
{
    DrawCell(ui, Word(ui, id), x, y);
}
static inline void TextColor(VIUI *ui, const VIColor32F &c) { ui->m_font.SetTextColor(c); }

// The rank names, leader first (0x00CE9530).
static const int sRankWords[] = { 541, 540, 539, 538 };

// 0x00C5B958.
VIWndGuild::VIWndGuild()
{
    m_mbox = 0;
    m_menu = 0;
    m_mode = 1;
    m_sel = 0;
    m_top = 0;
    m_focus = 1;
    m_time = 0;
    m_fadeIn = 1;
    m_alpha = 1.0f;
}

// 0x00C5B9D0.
VIWndGuild::~VIWndGuild()
{
    Free();
}

// 0x00C5BA30.  The confirmation box and the member menu, hidden.
int VIWndGuild::Init()
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
    m_mode = 1;
    m_sel = 0;
    m_top = 0;
    m_focus = 1;
    m_fadeIn = 1;
    m_alpha = 1.0f;
    m_time = 0;
done:
    return err;
}

// 0x00C5BB50.
int VIWndGuild::Free()
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
    m_mode = 1;
    m_sel = 0;
    m_top = 0;
    m_focus = 1;
    m_fadeIn = 1;
    m_alpha = 1.0f;
    return 0;
}

// 0x00C5BC08.  Six columns, eleven rows; a name past eleven letters ends "...".
void VIWndGuild::OnDrawSelf()
{
    int x[6];
    unsigned short buf[64];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIClientGuild *guild = app->GetGuild();
    if (m_mode != 2)
        return;
    if (!guild->InGuild())
    {
        app->PlaySound(8);
        app->ShowMsg(570);
        app->GetP2EF24()->HideInvite();
        return;
    }
    if (m_flash.Pulse(138))
        Move(-1);
    if (m_flash.Pulse(139))
        Move(1);
    Clamp();
    long now = app->m_p07568;
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    ui->Begin2D();
    VIVect2I hpos;
    VIColor32F hl(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    int lineH = ui->m_font.GetHeight(ui->m_raster, ui->m_dict) - 4;
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    m_window.SetTitle(guild->GetName());
    m_window.Draw();
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 212), Word(ui, 161));
    m_window.Text(buf, 20, 13, black, 0);
    float t;
    int count = guild->Count();
    if (count > 11)
        t = (float)m_top / (float)(count - 11);
    else
        t = 0.0f;
    m_window.DrawFrameVScroll(20, 1, m_window.Width() - 50, 11, t);
    int y = m_window.RowCenter(0).y;
    x[0] = 65;
    x[1] = 225;
    x[2] = 325;
    x[3] = 395;
    x[4] = 465;
    x[5] = 530;
    TextColor(ui, black);
    if (!guild->InGuild())
    {
        ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, Word(ui, 570), x[0], y, 1);
        return;
    }
    DrawWord(ui, 509, x[0], y);
    DrawWord(ui, 537, x[1], y);
    DrawWord(ui, 510, x[2], y);
    DrawWord(ui, 511, x[3], y);
    DrawWord(ui, 503, x[4], y);
    DrawWord(ui, 521, x[5], y);
    y += lineH;
    hpos.x = x[0] - 25;
    for (int i = 0; i < 11; i++)
    {
        int n = i + m_top;
        if (n == m_sel)
        {
            const VIUITexEntry *e = GetUITexture((VIUITexId)85);
            if (e)
            {
                TextColor(ui, hl);
                hpos.y = y - e->m_uv1.y / 2;
                ui->BlitEx((VIUITexId)e->m_x00, hpos, (VIAlphaBlendMode)3, m_alpha, 0);
            }
        }
        else
            TextColor(ui, black);
        if (n >= guild->Count())
            break;
        char name[64];
        memset(name, 0, sizeof(name));
        VIString::strcpy8(name, sizeof(name), guild->GetMemberName(n));
        if (name[11])
        {
            name[14] = 0;
            name[11] = '.';
            name[12] = '.';
            name[13] = '.';
        }
        DrawCell(ui, name, x[0], y);
        VIString::snprintf16(buf, sizeof(buf), "%S", Word(ui, sRankWords[guild->GetMemberRank(n)]));
        DrawCell(ui, buf, x[1], y);
        const VIRaceDef *race = GetRace(guild->GetMemberRace(n));
        if (!race)
            continue;
        DrawWord(ui, (unsigned short)race->m_shortStrID, x[2], y);
        const VIClassDef *cls = GetClass(guild->GetMemberClass(n));
        if (!cls)
            continue;
        DrawWord(ui, (unsigned short)cls->m_shortStrID, x[3], y);
        VIString::snprintf16(buf, 64, "%u", guild->GetMemberLevel(n));
        DrawCell(ui, buf, x[4], y);
        if (guild->GetMemberOnline(n) > 0)
            DrawWord(ui, 21, x[5], y);
        else
            DrawWord(ui, 22, x[5], y);
        y += lineH;
    }
    ui->End2D();
    if (m_focus)
    {
        if (m_fadeIn == -1)
        {
            m_alpha -= dt * 2.0f;
            m_alpha = m_alpha < 0.0f ? 0.0f : m_alpha;
            if (m_alpha == 0.0f)
                m_fadeIn = 1;
        }
        else
        {
            m_alpha += dt * 3.0f;
            m_alpha = m_alpha < 1.0f ? m_alpha : 1.0f;
            if (m_alpha == 1.0f)
                m_fadeIn = -1;
        }
    }
    else
        m_alpha = 1.0f;
}

// 0x00C5C478.
void VIWndGuild::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientGuild *guild = app->GetGuild();
    VIClientPart2EF24 *cmds = app->GetP2EF24();
    switch (key)
    {
    case 27:
    case 132:
        app->PlaySound(8);
        cmds->HideInvite();
        break;
    case 13:
    case 134:
        if (m_sel < guild->Count())
        {
            app->PlaySound(13);
            ShowMemberMenu(guild->GetMemberName(m_sel));
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

// 0x00C5C608.
void VIWndGuild::OnKeyUp(int key)
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








































































































// 0x00C5C680.  261 is the member menu's choice (p1 the entry's id), 268 and 260 the
// confirmation box's yes and no (p1 the operation, p2 the member).
void VIWndGuild::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIPlayer *player = app->GetPlayer();
    VIClientGuild *guild = app->GetGuild();
    VIClientPart2EF24 *cmds = app->GetP2EF24();
    switch (msg)
    {
    case 261:
        w->SetVisible(false);
        app->PlaySound(13);
        switch (p1)
        {
        case 0x0B051002:
            m_parent->OnMessage(this, 305, m_sel, 0);
            break;
        case 0x0B051003:
        {
            int id = guild->GetMemberId(m_sel);
            int err = cmds->CanPromote(id);
            switch (err)
            {
            case 0:
                m_mbox->SetText((unsigned short)544);
                m_mbox->SetVisible(true);
                m_mbox->SetParam1(p1);
                m_mbox->SetParam2(id);
                app->SetActiveWindow(m_mbox);
                break;
            case 3:
                app->ShowText(Word(ui, 570));
                app->SetActiveWindow(this);
                break;
            case 5:
                app->ShowText(Word(ui, 546));
                app->SetActiveWindow(this);
                break;
            default:
                VIASSERTF(0, "unexpected GuildError 0x%02x", err);
            }
            break;
        }
        case 0x0B051004:
        {
            int id = guild->GetMemberId(m_sel);
            int err = cmds->CanDemote(id);
            switch (err)
            {
            case 0:
                m_mbox->SetText((unsigned short)545);
                m_mbox->SetVisible(true);
                m_mbox->SetParam1(p1);
                m_mbox->SetParam2(id);
                app->SetActiveWindow(m_mbox);
                break;
            case 3:
                app->ShowText(Word(ui, 570));
                app->SetActiveWindow(this);
                break;
            case 5:
                app->ShowText(Word(ui, 546));
                app->SetActiveWindow(this);
                break;
            case 12:
                app->ShowText(Word(ui, 745));
                app->SetActiveWindow(this);
                break;
            default:
                VIASSERTF(0, "unexpected GuildError 0x%02x", err);
            }
            break;
        }
        case 0x0B051005:
        {
            int id = guild->GetMemberId(m_sel);
            int err = cmds->CanKick(id);
            switch (err)
            {
            case 0:
                m_mbox->SetText((unsigned short)536);
                m_mbox->SetVisible(true);
                m_mbox->SetParam1(p1);
                m_mbox->SetParam2(id);
                app->SetActiveWindow(m_mbox);
                break;
            case 3:
                app->ShowText(Word(ui, 570));
                app->SetActiveWindow(this);
                break;
            case 5:
                app->ShowText(Word(ui, 546));
                app->SetActiveWindow(this);
                break;
            default:
                VIASSERTF(0, "unexpected GuildError 0x%02x", err);
            }
            break;
        }
        }
        break;
    case 268:
        m_mbox->SetVisible(false);
        switch (p1)
        {
        case 1:
            cmds->Decline();
            return;
        case 0x0B051003:
            cmds->Promote(guild->GetMemberId(m_sel));
            break;
        case 0x0B051004:
            cmds->Demote(guild->GetMemberId(m_sel));
            break;
        case 0x0B051005:
            cmds->Kick(guild->GetMemberId(m_sel));
            break;
        }
        app->SetActiveWindow(this);
        break;
    case 260:
        if (w == m_mbox)
        {
            w->SetVisible(false);
            if (p1 == 1)
            {
                cmds->Accept();
                break;
            }
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
            m_fadeIn = 1;
            m_alpha = 1.0f;
            m_time = app->m_p07568;
            m_flash.Reset();
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 600, 14, 0);
            m_window.Open(true);
        }
        else if ((VIWnd *)p1 == this)
            m_focus = 0;
        break;
    }
}

// 0x00C5CC50.  45 seconds to answer.
void VIWndGuild::ShowInvite()
{
    unsigned short buf[512];
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    VIClientPart2EF24 *cmds = app->GetP2EF24();
    VIString::snprintf16(buf, 512, "%S\n%s\n%S\n%S", Word(ui, 535), cmds->GetInviterName(),
                         cmds->GetInviteGuild(), Word(ui, 520));
    m_mode = 1;
    m_menu->SetVisible(false);
    m_mbox->Init(1, 1);
    m_mbox->SetTimeout(app->m_p07568 + 45000);
    m_mbox->SetText(buf);
    m_mbox->SetVisible(true);
    m_mbox->SetParam1(1);
    app->SetActiveWindow(m_mbox);
}

// 0x00C5CD90.
void VIWndGuild::Close()
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_mode = 2;
    m_mbox->SetVisible(false);
    m_menu->SetVisible(false);
    m_sel = 0;
    m_top = 0;
    app->SetActiveWindow(this);
}

// 0x00C5CDE0.
void VIWndGuild::Reset()
{
    m_mbox->SetVisible(false);
    m_menu->SetVisible(false);
    m_mode = 1;
}

// 0x00C5CE00.
void VIWndGuild::Move(int d)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int count = app->GetGuild()->Count();
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
    if (m_top + 10 < m_sel)
        m_top = m_sel - 10;
    if (m_sel < m_top)
        m_top = m_sel;
    m_fadeIn = 1;
    m_alpha = 1.0f;
    m_time = app->m_p07568;
}

// 0x00C5CF18.
void VIWndGuild::Clamp()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int sel = m_sel;
    int count = app->GetGuild()->Count();
    if (count - 11 < m_top && count > 11)
        m_top--;
    else if (count == 11)
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

// The member menu (0x00CD7490, 0x00CD7510): info, promote, demote, kick.
static VIMenuEntry sMemberItems[4];
static VIMenuDef sMemberMenu;

// 0x00C5CFE8.
void VIWndGuild::ShowMemberMenu(const char *name)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    ui->m_font.GetHeight(ui->m_raster, ui->m_dict);
    if (!m_menu)
    {
        m_menu = new VIWndMiniMenu;
        m_menu->SetAppPtr(GetAppPtr());
        SetFlag(m_menu, 1);
        AddChild(m_menu);
        if (!m_menu)
            return;
    }
    sMemberMenu.m_x00 = -1;
    sMemberMenu.m_x04 = -1;
    sMemberMenu.m_entries = sMemberItems;
    sMemberMenu.m_count = 4;
    sMemberItems[0].m_word = 522;
    sMemberItems[1].m_word = 526;
    sMemberItems[2].m_word = 527;
    sMemberItems[3].m_word = 516;
    sMemberItems[0].m_msg = 0x0B051002;
    sMemberItems[1].m_msg = 0x0B051003;
    sMemberItems[2].m_msg = 0x0B051004;
    sMemberItems[3].m_msg = 0x0B051005;
    sMemberItems[0].m_icon = -1;
    sMemberItems[1].m_icon = -1;
    sMemberItems[2].m_icon = -1;
    sMemberItems[3].m_icon = -1;
    sMemberItems[0].m_icon2 = -1;
    sMemberItems[1].m_icon2 = -1;
    sMemberItems[2].m_icon2 = -1;
    sMemberItems[3].m_icon2 = -1;
    sMemberItems[0].m_number = -1;
    sMemberItems[1].m_number = -1;
    sMemberItems[2].m_number = -1;
    sMemberItems[3].m_number = -1;
    sMemberItems[0].m_id = 0;
    sMemberItems[1].m_id = 0;
    sMemberItems[2].m_id = 0;
    sMemberItems[3].m_id = 0;
    sMemberItems[0].m_sub = 0;
    sMemberItems[1].m_sub = 0;
    sMemberItems[2].m_sub = 0;
    sMemberItems[3].m_sub = 0;
    m_menu->SetMenu(&sMemberMenu);
    m_menu->SetPos(VIVect2I(170, 50));
    if (name)
        m_menu->SetTitle(name);
    else
        m_menu->SetTitle("");
    m_menu->ResetScroll();
    m_menu->SetVisible(true);
    app->SetActiveWindow(m_menu);
}
