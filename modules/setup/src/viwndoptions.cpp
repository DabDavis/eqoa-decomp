// viwndoptions.cpp (name ours) -- SETUP 0x00575DD8 .. 0x00578DF8.  The Options
// window.  The labels are WORDS.TXT ids (`Word`); the settings it shows are
// listed with VIGameOptions in vi.h.
#include "setup.h"

class VIWndMcErr;
VIWndMcErr *McErrWnd();
int  McErr(VIWnd *owner, const VIVect2I &center, int screen);
void McErrSetMode(int mode);
int  VIIsMcSameSession();

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// The value names, fetched once (OnDrawSelf) and dropped by Init.
static const unsigned short *sTracks[10];    // None, Song 1..8, Random
static const unsigned short *sOutput[2];     // Mono, Stereo
static const unsigned short *sKeyboard[2];   // Standard, Compact
static const unsigned short *sOnOff[2];      // Off, On
static const unsigned short *sCamera[2];     // Tether, Follow
static const unsigned short *sPitch[2];      // Normal, Inverted

// The track playing.  An inline, by its code: the read goes through the
// music's own address, which each arm of OnDrawSelf computes afresh.
static inline int Track(const VIBattleMusic &m)
{
    return m.m_track;
}

// 0x00575DD8.
VIWndOptions::VIWndOptions()
{
    m_child = 0;
}

// 0x00575E38.
VIWndOptions::~VIWndOptions()
{
    if (m_child)
    {
        RemoveChild(m_child);
        m_child = 0;
    }
}

// 0x00575ED0.
int VIWndOptions::Init(VIScene *scene)
{
    m_sel = 0;
    m_x24 = 0;
    sTracks[0] = 0;
    VIBattleMusic &music = m_music;
    music.Init(scene);
    music.m_rate = 0.02f;
    if (m_child)
    {
        RemoveChild(m_child);
        m_child = 0;
    }
    m_message.Init();
    m_message.SetVisible(false);
    AddChild(&m_message);
    return 0;
}

// 0x00575F60.
void VIWndOptions::Free()
{
    if (m_message.GetParent() == this)
    {
        RemoveChild(&m_message);
        m_message.Free();
    }
    if (m_child)
    {
        RemoveChild(m_child);
        m_child = 0;
    }
    m_music.Free();
}

// 0x00575FC8.  The Sound page previews the Battle Music choice.
void VIWndOptions::OnDrawSelf()
{
    VIUI *ui = UI();
    VIGameOptions *opts = VIGetGameOptions();
    if (m_child && !m_mcErr)
    {
        RemoveChild(m_child);
        m_child = 0;
    }
    if (!sTracks[0])
    {
        sTracks[0] = Word(UI(), 517);
        sTracks[1] = Word(UI(), 795);
        sTracks[2] = Word(UI(), 796);
        if (VIGetFeatureFlags() & 0x100)
        {
            sTracks[3] = Word(UI(), 797);
            sTracks[4] = Word(UI(), 798);
            sTracks[5] = Word(UI(), 799);
            sTracks[6] = Word(UI(), 1576);
            sTracks[7] = Word(UI(), 1577);
            sTracks[8] = Word(UI(), 1578);
            sTracks[9] = Word(UI(), 1630);
        }
        sOutput[0] = Word(UI(), 781);
        sOutput[1] = Word(UI(), 804);
        sKeyboard[0] = Word(UI(), 802);
        sKeyboard[1] = Word(UI(), 755);
        sOnOff[0] = Word(UI(), 552);
        sOnOff[1] = Word(UI(), 551);
        sCamera[0] = Word(UI(), 805);
        sCamera[1] = Word(UI(), 68);
        sPitch[0] = Word(UI(), 784);
        sPitch[1] = Word(UI(), 779);
    }
    if (GetFocus() != this)
        m_window.ResetTickPulse();
    if (m_page == 2)
    {
        if (VIGetFeatureFlags() & 0x100)
        {
            unsigned int track = opts->m_language;
            if (track < 9)
                m_music.Play(track);
            else
                m_music.Stop(false);
        }
        else
        {
            if (opts->m_language != Track(m_music))
                m_music.Play(opts->m_language);
            else
                m_music.Play(-1);
        }
    }
    else
        m_music.StopNow();
    m_music.Process();
    ui->Begin2D();
    DrawMenu();
    switch (m_page)
    {
    case 0:
        DrawGamePage();
        break;
    case 1:
        DrawInterfacePage();
        break;
    case 2:
        DrawSoundPage();
        break;
    }
    if (m_saveCount)
    {
        m_saveCount--;
        m_window3.Init(UI(), VIVect2I(0, 0), m_window.Center(), 400, 3, 0);
        m_window3.Open(true);
        m_window3.Draw();
        m_window3.TextBox(Word(UI(), 1615), 40, 0, m_window3.Width() - 80, 3, VIUICOLOR_0, 8);
        if (!m_saveCount)
            SaveOptions();
    }
    if (m_saveFailed)
    {
        m_window3.Init(UI(), VIVect2I(0, 0), m_window.Center(), 440, 6, 0);
        m_window3.Open(true);
        m_window3.Draw();
        unsigned short buf[256];
        VIString::snprintf16(buf, 512, "%S\n\n%S", Word(UI(), 1609), Word(UI(), 1610));
        m_window3.TextBox(buf, 40, 0, m_window3.Width() - 80, 4, VIUICOLOR_0, 8);
        VIString::snprintf16(buf, 512, "|q%S|w", Word(UI(), 786));
        m_window3.TextBox(buf, 40, 5, m_window3.Width() - 80, 1, VIUICOLOR_0, 8);
    }
    ui->End2D();
}

// 0x00576518.
void VIWndOptions::OnKeyDown(int key)
{
    VIGameOptions *opts = VIGetGameOptions();
    if (m_mcErr || m_saveCount)
        return;
    if (m_saveFailed)
    {
        if (key == 13 || key == 134)
        {
            m_saveCount = 0;
            m_saveFailed = 0;
        }
        return;
    }
    switch (key)
    {
    case 19:
    case 138:
        Prev();
        break;
    case 20:
    case 139:
        Next();
        break;
    case 17:
    case 136:
        Decrement();
        break;
    case 18:
    case 137:
        Increment();
        break;
    case 13:
    case 134:
        if (Select() == 1)
        {
            m_message.Setup(Word(UI(), 1633), Word(UI(), 1634), (VIMESSAGEPROMPT)2,
                            m_window.Center(), 300);
            m_message.SetVisible(true);
            SetFocus(&m_message, true);
        }
        break;
    case 27:
    case 132:
        if (Back() == 1)
        {
            int camera = m_saved06;
            opts->m_x06 = camera;
            opts->m_x07 = m_saved07;
            opts->m_language = m_saved04;
            opts->m_x05 = m_saved05;
            opts->m_musicVolume = m_saved08;
            opts->m_effectVolume = m_saved0C;
            opts->m_x91 = m_saved91;
            opts->m_x92 = !m_saved92;
            UI()->m_sound->SetSoundClassVolume((VISOUNDCLASS)0, opts->m_musicVolume);
            UI()->m_sound->SetSoundClassVolume((VISOUNDCLASS)1, opts->m_effectVolume);
            SetVisible(false);
            SetFocus(GetParent(), true);
        }
        break;
    }
}

// 0x005767A0.  Not in the 2002 betas, which saved nothing here.
int VIWndOptions::SaveOptions()
{
    if (VIMemCardStatus(0))
    {
        m_mcErr = 1;
        McErr(this, m_window.Center(), 4);
        McErrSetMode(1);
        return -1;
    }
    int same = VIIsMcSameSession();
    if (same != 1)
    {
        m_mcErr = 1;
        McErr(this, m_window.Center(), 4);
        McErrSetMode(1);
        return -1;
    }
    if (VIGetGameOptions()->Save() < 0)
    {
        m_saveFailed = 1;
        return -1;
    }
    SetVisible(false);
    SetFocus(GetParent(), true);
    return 0;
}

// 0x00576890.
void VIWndOptions::OnKeyUp(int key)
{
}

// 0x00576898.  0x1100..0x1106 are McErr's notices, 307 the message box's
// answer.
void VIWndOptions::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIGameOptions *opts = VIGetGameOptions();
    switch (msg)
    {
    case 295:
        if (p2 == (int)this && p1 != (int)McErrWnd() && p1 != (int)&m_message)
        {
            m_window.Close(true);
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 300, 7, 0);
            m_window.Open(false);
            m_window.SetTitle(Word(UI(), 788));
            m_message.SetVisible(false);
            m_saved06 = opts->m_x06;
            m_saved07 = opts->m_x07;
            m_saved04 = opts->m_language;
            m_saved05 = opts->m_x05;
            m_saved08 = opts->m_musicVolume;
            m_saved0C = opts->m_effectVolume;
            m_saved91 = opts->m_x91;
            m_saved92 = !opts->m_x92;
            m_edit93 = !opts->m_x93;
            m_edit94 = !opts->m_x94;
            m_edit95 = opts->m_x95;
            m_edit97 = opts->m_x97;
            m_sel = 0;
            m_page = -1;
            m_item = -1;
            m_mcErr = 0;
            m_saveCount = 0;
            m_saveFailed = 0;
            if (m_child)
            {
                RemoveChild(m_child);
                m_child = 0;
            }
        }
        if (p1 == (int)this && p2 != (int)McErrWnd())
            m_music.Unload();
        break;
    case 307:
        SetFocus(this, true);
        if (w == &m_message && p1 == 0)
        {
            opts->m_x93 = !m_edit93;
            opts->m_x94 = !m_edit94;
            opts->m_x95 = m_edit95;
            opts->m_x97 = m_edit97;
            m_saveCount = 4;
        }
        break;
    case 0x1104:
        SetFocus(w, true);
        break;
    case 0x1100:
        AddChild(w);
        m_child = (VIWnd *)McErrWnd();
        break;
    case 0x1102:
        m_child = w;
        break;
    case 0x1106:
        SetFocus(this, true);
        m_mcErr = 0;
        switch (p1)
        {
        case 0:
            SetVisible(false);
            SetFocus(GetParent(), true);
            break;
        case 1:
            m_saveCount = 4;
            break;
        }
        break;
    case 259:
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00576BD0.
void VIWndOptions::Prev()
{
    if (m_page == -1)
    {
        if (m_sel > 0)
        {
            m_sel--;
            m_window.ResetTickPulse();
            UI()->SoundPlay((VIUISoundId)11);
        }
        else
            UI()->SoundPlay((VIUISoundId)9);
    }
    else
    {
        if (m_row > 0)
        {
            m_row--;
            m_window.ResetTickPulse();
            UI()->SoundPlay((VIUISoundId)11);
        }
        else
            UI()->SoundPlay((VIUISoundId)9);
    }
}

// 0x00576C60.  The last row of each page; Interface gains Vibration.
void VIWndOptions::Next()
{
    if (m_page == -1)
    {
        if (m_sel < 3)
        {
            m_sel++;
            m_window.ResetTickPulse();
            UI()->SoundPlay((VIUISoundId)11);
        }
        else
            UI()->SoundPlay((VIUISoundId)9);
    }
    else
    {
        int last[3] = { 2, 3, 3 };
        if (VIGetFeatureFlags() & 0x100)
            last[1] = 4;
        if (m_row < last[m_page])
        {
            m_row++;
            m_window.ResetTickPulse();
            UI()->SoundPlay((VIUISoundId)11);
        }
        else
            UI()->SoundPlay((VIUISoundId)9);
    }
}

// 0x00576D48.
void VIWndOptions::Decrement()
{
    VIGameOptions *opts = VIGetGameOptions();
    int tracks = (VIGetFeatureFlags() & 0x100) ? 10 : 3;
    switch (m_item)
    {
    case 0:
        opts->m_x06 = opts->m_x06 != 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 1:
        opts->m_x07 = !opts->m_x07;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 2:
        opts->m_language = (opts->m_language + (tracks - 1)) % tracks;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 3:
        if (opts->m_musicVolume > 0.0f)
        {
            opts->m_musicVolume = VIMAX(0.0f, opts->m_musicVolume - 0.1f);
            UI()->SoundPlay((VIUISoundId)11);
        }
        else
            UI()->SoundPlay((VIUISoundId)9);
        UI()->m_sound->SetSoundClassVolume((VISOUNDCLASS)0, opts->m_musicVolume);
        break;
    case 4:
        if (opts->m_effectVolume > 0.0f)
        {
            opts->m_effectVolume = VIMAX(0.0f, opts->m_effectVolume - 0.1f);
            UI()->SoundPlay((VIUISoundId)11);
        }
        else
            UI()->SoundPlay((VIUISoundId)9);
        UI()->m_sound->SetSoundClassVolume((VISOUNDCLASS)1, opts->m_effectVolume);
        break;
    case 5:
    {
        opts->m_x05 ^= 1;
        int stereo = opts->m_x05;
        UI()->m_sound->SetOutputMode(stereo != 0);
        UI()->SoundPlay((VIUISoundId)11);
        break;
    }
    case 6:
        opts->m_x91 ^= 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 7:
        opts->m_x92 = !opts->m_x92;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 8:
        m_edit93 ^= 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 9:
        m_edit94 ^= 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 10:
        m_edit95 ^= 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 11:
        m_edit97 ^= 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    }
    if (opts->m_musicVolume < 0.05f)
        opts->m_musicVolume = 0.0f;
    if (opts->m_effectVolume < 0.05f)
        opts->m_effectVolume = 0.0f;
}

// 0x00577030.
void VIWndOptions::Increment()
{
    VIGameOptions *opts = VIGetGameOptions();
    int tracks = (VIGetFeatureFlags() & 0x100) ? 10 : 3;
    switch (m_item)
    {
    case 0:
        opts->m_x06 = opts->m_x06 != 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 1:
        opts->m_x07 = !opts->m_x07;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 2:
        opts->m_language = (opts->m_language + 1) % tracks;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 3:
        if (opts->m_musicVolume < 1.0f)
        {
            opts->m_musicVolume = VIMIN(opts->m_musicVolume + 0.1f, 1.0f);
            UI()->SoundPlay((VIUISoundId)11);
        }
        else
            UI()->SoundPlay((VIUISoundId)9);
        UI()->m_sound->SetSoundClassVolume((VISOUNDCLASS)0, opts->m_musicVolume);
        break;
    case 4:
        if (opts->m_effectVolume < 1.0f)
        {
            opts->m_effectVolume = VIMIN(opts->m_effectVolume + 0.1f, 1.0f);
            UI()->SoundPlay((VIUISoundId)11);
        }
        else
            UI()->SoundPlay((VIUISoundId)9);
        UI()->m_sound->SetSoundClassVolume((VISOUNDCLASS)1, opts->m_effectVolume);
        break;
    case 5:
    {
        opts->m_x05 ^= 1;
        int stereo = opts->m_x05;
        UI()->m_sound->SetOutputMode(stereo != 0);
        UI()->SoundPlay((VIUISoundId)11);
        break;
    }
    case 6:
        opts->m_x91 ^= 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 7:
        opts->m_x92 = !opts->m_x92;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 8:
        m_edit93 ^= 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 9:
        m_edit94 ^= 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 10:
        m_edit95 ^= 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    case 11:
        m_edit97 ^= 1;
        UI()->SoundPlay((VIUISoundId)11);
        break;
    }
    if (opts->m_musicVolume > 0.95f)
        opts->m_musicVolume = 1.0f;
    if (opts->m_effectVolume > 0.95f)
        opts->m_effectVolume = 1.0f;
}

// 0x00577340.  X on the menu: open a page, or (Save Changes) return 1 for
// OnKeyDown to ask.
int VIWndOptions::Select()
{
    if (m_page == -1)
    {
        UI()->SoundPlay((VIUISoundId)8);
        m_row = 0;
        switch (m_sel)
        {
        case 0:
            m_page = 0;
            m_window2.Close(true);
            m_window2.Init(UI(), VIVect2I(0, 0), m_window.Center(), 440, 6, 0);
            m_window2.Open(false);
            m_window2.SetTitle("");
            break;
        case 1:
            m_page = 1;
            m_window2.Close(true);
            if (VIGetFeatureFlags() & 0x200)
                m_window2.Init(UI(), VIVect2I(0, 0), m_window.Center(), 440, 8, 0);
            else
                m_window2.Init(UI(), VIVect2I(0, 0), m_window.Center(), 440, 7, 0);
            m_window2.Open(false);
            m_window2.SetTitle("");
            break;
        case 2:
            m_page = 2;
            m_window2.Close(true);
            m_window2.Init(UI(), VIVect2I(0, 0), m_window.Center(), 440, 7, 0);
            m_window2.Open(false);
            m_window2.SetTitle("");
            break;
        case 3:
            return 1;
        }
        return 0;
    }
    UI()->SoundPlay((VIUISoundId)9);
    return 0;
}

// 0x00577528.  Triangle: back to the menu, or (on the menu) 1 to close.
int VIWndOptions::Back()
{
    if (m_page >= 0)
    {
        m_page = -1;
        UI()->SoundPlay((VIUISoundId)8);
        return 0;
    }
    UI()->SoundPlay((VIUISoundId)8);
    return 1;
}

// 0x00577578.  The menu: Game, Interface, Sound, Save Changes.
void VIWndOptions::DrawMenu()
{
    unsigned short buf[256];
    m_item = -1;
    m_window.Draw();
    if (m_sel == 0)
        VIString::str8to16(buf, 512, "|q");
    else
        buf[0] = 0;
    VIString::strcat16(buf, 512, Word(UI(), 773));
    if (m_sel == 0)
        VIString::strcat16(buf, 512, "|w");
    m_window.TextBox(buf, 40, 1, m_window.Width() - 80, 1, (VIUICOLOR)(m_sel == 0), 8);
    if (m_sel == 1)
        VIString::str8to16(buf, 512, "|q");
    else
        buf[0] = 0;
    VIString::strcat16(buf, 512, Word(UI(), 778));
    if (m_sel == 1)
        VIString::strcat16(buf, 512, "|w");
    m_window.TextBox(buf, 40, 2, m_window.Width() - 80, 1, (VIUICOLOR)(m_sel == 1), 8);
    if (m_sel == 2)
        VIString::str8to16(buf, 512, "|q");
    else
        buf[0] = 0;
    VIString::strcat16(buf, 512, Word(UI(), 800));
    if (m_sel == 2)
        VIString::strcat16(buf, 512, "|w");
    m_window.TextBox(buf, 40, 3, m_window.Width() - 80, 1, (VIUICOLOR)(m_sel == 2), 8);
    if (m_sel == 3)
        VIString::str8to16(buf, 512, "|q");
    else
        buf[0] = 0;
    VIString::strcat16(buf, 512, Word(UI(), 482));
    if (m_sel == 3)
        VIString::strcat16(buf, 512, "|w");
    m_window.TextBox(buf, 40, 4, m_window.Width() - 80, 1, (VIUICOLOR)(m_sel == 3), 8);
    VIString::str8to16(buf, 512, "|x ");
    VIString::strcat16(buf, 512, Word(UI(), 212));
    VIString::strcat16(buf, 512, "   |t ");
    VIString::strcat16(buf, 512, Word(UI(), 753));
    m_window.Text(buf, 40, 6, VIUICOLOR_0, 0);
}

// 0x005778C0.  Game: Faction Messages, Loot Messages, Damage Numbers.
void VIWndOptions::DrawGamePage()
{
    VIGameOptions *opts = VIGetGameOptions();
    switch (m_row)
    {
    case 0:
        m_item = 8;
        break;
    case 1:
        m_item = 9;
        break;
    case 2:
        m_item = 7;
        break;
    }
    m_window2.Draw();
    m_window2.TextBox(Word(UI(), 738), 40, 1, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 0), 0);
    m_window2.TextBox(Word(UI(), 739), 40, 2, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 1), 0);
    m_window2.TextBox(Word(UI(), 759), 40, 3, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 2), 0);
    m_window2.TickR(36, m_row + 1, true);
    m_window2.DrawFrame(m_window2.Width() - 176, 1, 132, 1, m_row == 0 ? 96 : 64);
    m_window2.TextBox(sOnOff[m_edit93], m_window2.Width() - 176, 1, 132, 1, VIUICOLOR_0, 8);
    m_window2.DrawFrame(m_window2.Width() - 176, 2, 132, 1, m_row == 1 ? 96 : 64);
    m_window2.TextBox(sOnOff[m_edit94], m_window2.Width() - 176, 2, 132, 1, VIUICOLOR_0, 8);
    m_window2.DrawFrame(m_window2.Width() - 176, 3, 132, 1, m_row == 2 ? 96 : 64);
    int damage = !opts->m_x92;
    m_window2.TextBox(sOnOff[damage], m_window2.Width() - 176, 3, 132, 1, VIUICOLOR_0, 8);
    unsigned short buf[256];
    VIString::str8to16(buf, 512, "|t ");
    VIString::strcat16(buf, 512, Word(UI(), 750));
    m_window2.Text(buf, 40, 5, VIUICOLOR_0, 0);
}

// 0x00577BD0.  Interface: Auto Attack, 3rd Person Camera, Camera Pitch Axis,
// Soft Keyboard and (feature 0x200) Vibration.
void VIWndOptions::DrawInterfacePage()
{
    VIGameOptions *opts = VIGetGameOptions();
    switch (m_row)
    {
    case 0:
        m_item = 10;
        break;
    case 1:
        m_item = 0;
        break;
    case 2:
        m_item = 1;
        break;
    case 3:
        m_item = 6;
        break;
    case 4:
        m_item = 11;
        break;
    }
    m_window2.Draw();
    m_window2.TextBox(Word(UI(), 1792), 40, 1, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 0), 0);
    m_window2.TextBox(Word(UI(), 747), 40, 2, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 1), 0);
    m_window2.TextBox(Word(UI(), 752), 40, 3, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 2), 0);
    m_window2.TextBox(Word(UI(), 794), 40, 4, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 3), 0);
    if (VIGetFeatureFlags() & 0x200)
        m_window2.TextBox(Word(UI(), 1604), 40, 5, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 4), 0);
    m_window2.TickR(36, m_row + 1, true);
    m_window2.DrawFrame(m_window2.Width() - 176, 1, 132, 1, m_row == 0 ? 96 : 64);
    m_window2.TextBox(sOnOff[m_edit95], m_window2.Width() - 176, 1, 132, 1, VIUICOLOR_0, 8);
    m_window2.DrawFrame(m_window2.Width() - 176, 2, 132, 1, m_row == 1 ? 96 : 64);
    m_window2.TextBox(sCamera[opts->m_x06], m_window2.Width() - 176, 2, 132, 1, VIUICOLOR_0, 8);
    m_window2.DrawFrame(m_window2.Width() - 176, 3, 132, 1, m_row == 2 ? 96 : 64);
    m_window2.TextBox(sPitch[opts->m_x07], m_window2.Width() - 176, 3, 132, 1, VIUICOLOR_0, 8);
    m_window2.DrawFrame(m_window2.Width() - 176, 4, 132, 1, m_row == 3 ? 96 : 64);
    m_window2.TextBox(sKeyboard[opts->m_x91], m_window2.Width() - 176, 4, 132, 1, VIUICOLOR_0, 8);
    if (VIGetFeatureFlags() & 0x200)
    {
        m_window2.DrawFrame(m_window2.Width() - 176, 5, 132, 1, m_row == 4 ? 96 : 64);
        m_window2.TextBox(sOnOff[m_edit97], m_window2.Width() - 176, 5, 132, 1, VIUICOLOR_0, 8);
    }
    unsigned short buf[256];
    VIString::str8to16(buf, 512, "|t ");
    VIString::strcat16(buf, 512, Word(UI(), 750));
    if (VIGetFeatureFlags() & 0x200)
        m_window2.Text(buf, 40, 7, VIUICOLOR_0, 0);
    else
        m_window2.Text(buf, 40, 6, VIUICOLOR_0, 0);
}

// 0x00578080.  Sound: Battle Music, Fx Volume, Music Volume, Sound Output.
void VIWndOptions::DrawSoundPage()
{
    VIGameOptions *opts = VIGetGameOptions();
    switch (m_row)
    {
    case 0:
        m_item = 2;
        break;
    case 1:
        m_item = 3;
        break;
    case 2:
        m_item = 4;
        break;
    case 3:
        m_item = 5;
        break;
    }
    m_window2.Draw();
    m_window2.TextBox(Word(UI(), 751), 40, 1, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 0), 0);
    m_window2.TextBox(Word(UI(), 772), 40, 2, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 1), 0);
    m_window2.TextBox(Word(UI(), 782), 40, 3, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 2), 0);
    m_window2.TextBox(Word(UI(), 801), 40, 4, m_window2.Width() - 80, 1, (VIUICOLOR)(m_row == 3), 0);
    m_window2.TickR(36, m_row + 1, true);
    m_window2.DrawFrame(m_window2.Width() - 176, 1, 132, 1, m_row == 0 ? 96 : 64);
    m_window2.TextBox(sTracks[opts->m_language], m_window2.Width() - 176, 1, 132, 1, VIUICOLOR_0, 8);
    m_window2.DrawHScroll(m_window2.Width() - 180, 2, 140, opts->m_musicVolume, m_row == 1 ? 32 : 0);
    m_window2.DrawHScroll(m_window2.Width() - 180, 3, 140, opts->m_effectVolume, m_row == 2 ? 32 : 0);
    m_window2.DrawFrame(m_window2.Width() - 176, 4, 132, 1, m_row == 3 ? 96 : 64);
    m_window2.TextBox(sOutput[opts->m_x05], m_window2.Width() - 176, 4, 132, 1, VIUICOLOR_0, 8);
    unsigned short buf[256];
    VIString::str8to16(buf, 512, "|t ");
    VIString::strcat16(buf, 512, Word(UI(), 750));
    m_window2.Text(buf, 40, 6, VIUICOLOR_0, 0);
}
