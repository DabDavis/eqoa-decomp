// viwndsplash.cpp (name ours) -- SETUP 0x0057B1A0 .. 0x0057CDD8.  The title
// screen: its three-choice menu (connect, quit, options), the memory card
// check that unlocks it, and the scan of the card's network configurations.
#include "setup.h"

// MCERR's entry points, and the card calls SUPPORT exports.
int    McErr(VIWnd *owner, const VIVect2I &center, int screen);
VIWnd *McErrWnd();
int    VIIsMcSameSession();
VI_RET(int, void)   VIMemCardCopyProhibit();

// One WORDS.TXT entry in the current language (MCERR's Word).
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// The UI's sound device, read through a call: the target loads it before the
// argument's VIGetGameOptions call, which a plain `ui->m_sound` does not.
static inline VISoundDevice *Sound(VIUI *ui)
{
    return ui->m_sound;
}

// The card's combination list, filled by ScanNetworkConfigurations.
static ezNetCnfCombinationList_t sList;

// 0x0057B1A0.
VIWndSplash::VIWndSplash()
{
}

// 0x0057B1F8.
VIWndSplash::~VIWndSplash()
{
}

// 0x0057B270.
int VIWndSplash::Init(VIScene *scene, VIPad *pad)
{
    m_scene = scene;
    m_pad = pad;
    m_sel = 0;
    m_connect.Init();
    m_connect.SetVisible(false);
    AddChild(&m_connect);
    m_message.Init();
    m_message.SetVisible(false);
    AddChild(&m_message);
    m_options.Init(scene);
    m_options.SetVisible(false);
    AddChild(&m_options);
    m_remove = 0;
    m_frames = 0;
    m_ready = 0;
    m_detached = 0;
    m_ispLost = 0;
    m_mcState = 0;
    return 0;
}

// 0x0057B338.
void VIWndSplash::Free()
{
    RemoveChild(&m_options);
    m_options.Free();
    RemoveChild(&m_message);
    m_message.Free();
    RemoveChild(&m_connect);
    m_connect.Free();
}

// 0x0057B3B8.
void VIWndSplash::OnDrawSelf()
{
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, 1.0f);
    VIVect2I unused;   // never read: the target zeroes it at entry (sp+32)
    VIUI *ui = UI();
    if (GetFocus() != this)
        m_window.ResetTickPulse();
    if (m_detached)
    {
        m_message.Setup(Word(UI(), 272), Word(UI(), 809), (VIMESSAGEPROMPT)0,
                        VIVect2I(320, 246), 320);
        m_message.SetVisible(true);
        SetFocus(&m_message, true);
        m_detached = 0;
        m_ispLost = 0;
    }
    else if (m_ispLost)
    {
        m_message.Setup(Word(UI(), 272), Word(UI(), 808), (VIMESSAGEPROMPT)0,
                        VIVect2I(320, 246), 320);
        m_message.SetVisible(true);
        SetFocus(&m_message, true);
        m_detached = 0;
        m_ispLost = 0;
    }
    if (m_remove)
    {
        RemoveChild(m_remove);
        m_remove = 0;
    }
    if (m_frames < 30)
    {
        m_ready = 0;
        m_frames++;
    }
    if (m_frames == 30)
    {
        m_frames = 31;
        McErr(this, VIVect2I(320, 236), 30);
    }
    if (m_frames == 32)
    {
        m_frames = 33;
        if (VIMemCardStatus(0) == 0)
        {
            m_ready = 1;
            m_frames++;
            VIMemCardCopyProhibit();
            printf("setting class volume %f, %f\n", VIGetGameOptions()->m_musicVolume,
                   VIGetGameOptions()->m_effectVolume);
            Sound(ui)->SetSoundClassVolume((VISOUNDCLASS)0, VIGetGameOptions()->m_musicVolume);
            Sound(ui)->SetSoundClassVolume((VISOUNDCLASS)1, VIGetGameOptions()->m_effectVolume);
        }
        else
            m_frames = 30;
    }
    ui->Begin2D();
    ui->Blit(VIHashResourceID("splash00.tga"), VIVect2I(0, 0), (VIAlphaBlendMode)0, 1.0f, 0);
    ui->Blit(VIHashResourceID("splash10.tga"), VIVect2I(320, 0), (VIAlphaBlendMode)0, 1.0f, 0);
    VISplashFxRender(1);
    int w = m_window.Width() / 3;
    int x[5] = { 0, w, w * 2, w * 3, w * 4 };
    unsigned short buf[256];
    if (m_sel == 0)
        VIString::str8to16(buf, 512, "|q");
    else
        buf[0] = 0;
    VIString::strcat16(buf, 512, Word(UI(), 790));
    if (m_sel == 0)
        VIString::strcat16(buf, 512, "|w");
    m_window.TextBox(buf, x[0] - 10, 0, x[1] - x[0] + 20, 1, m_sel == 0 ? yellow : white, 9);
    if (m_sel == 1)
        VIString::str8to16(buf, 512, "|q");
    else
        buf[0] = 0;
    VIString::strcat16(buf, 512, Word(UI(), 1635));
    if (m_sel == 1)
        VIString::strcat16(buf, 512, "|w");
    m_window.TextBox(buf, x[1] - 20, 0, x[2] - x[1] + 20, 1, m_sel == 1 ? yellow : white, 9);
    if (m_sel == 2)
        VIString::str8to16(buf, 512, "|q");
    else
        buf[0] = 0;
    VIString::strcat16(buf, 512, Word(UI(), 422));
    if (m_sel == 2)
        VIString::strcat16(buf, 512, "|w");
    m_window.TextBox(buf, x[2] - 10, 0, x[3] - x[2] + 20, 1, m_sel == 2 ? yellow : white, 9);
    if (m_mcState == 2)
    {
        m_mcState = 0;
        Connect();
    }
    ui->End2D();
}

// 0x0057B938.  Left/right (the pad's, or the keyboard's 17/18) move along the
// menu; cross or enter picks.
void VIWndSplash::OnKeyDown(int key)
{
    switch (key)
    {
    case 17:
    case 136:
        if (m_ready)
        {
            if (m_sel > 0)
            {
                m_sel--;
                m_window.ResetTickPulse();
                UI()->SoundPlay((VIUISoundId)11);
            }
        }
        break;
    case 18:
    case 137:
        if (m_ready)
        {
            if (m_sel < 2)
            {
                m_sel++;
                m_window.ResetTickPulse();
                UI()->SoundPlay((VIUISoundId)11);
            }
        }
        break;
    case 13:
    case 134:
        if (m_ready)
        {
            switch (m_sel)
            {
            case 0:
                UI()->SoundPlay((VIUISoundId)15);
                Connect();
                break;
            case 1:
                UI()->SoundPlay((VIUISoundId)13);
                SetFocus(0, true);
                break;
            case 2:
                m_options.SetVisible(true);
                SetFocus(&m_options, true);
                UI()->SoundPlay((VIUISoundId)15);
                break;
            }
        }
        break;
    }
}

// 0x0057BAD0.
void VIWndSplash::OnKeyUp(int key)
{
}

// 0x0057BAD8.  0x1100..0x1106 are what a child (McErr's window among them)
// sends its owner.
void VIWndSplash::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    switch (msg)
    {
    case 295:
        if (p2 == (int)this && p1 != (int)&m_connect && p1 != (int)&m_options
            && p1 != (int)&m_message)
        {
            if (m_connect.Cancelled())
            {
                m_connect.SetCancelled(0);
                m_frames = 0;
                m_ready = 0;
                m_detached = 0;
                m_ispLost = 0;
                m_mcState = 0;
            }
            m_window.Init(UI(), VIVect2I(0, 350), VIVect2I(304, 360), 440, 1, 0);
            m_window.Open(true);
            if (p1 != (int)McErrWnd())
                m_mcState = 0;
        }
        break;
    case 307:
        m_detached = 0;
        m_ispLost = 0;
        SetFocus(this, true);
        break;
    case 0x1104:
        SetFocus(w, true);
        break;
    case 0x1100:
        AddChild(w);
        break;
    case 0x1102:
        m_remove = w;
        break;
    case 0x1106:
        SetFocus(this, true);
        if (m_mcState == 1)
        {
            if (p1 == 1)
                m_mcState = 2;
            else
            {
                m_mcState = 0;
                m_frames = 0;
                VIGetGameOptions()->SetDefaults();
                VIGetGameOptions()->Load();
            }
        }
        else
            m_frames++;
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x0057BCD0.
void VIWndSplash::RestartConnect()
{
    m_connect.EndConnect();
    m_connect.SetVisible(false);
    m_message.SetVisible(false);
    m_options.SetVisible(false);
}

// 0x0057BD10.
ezNetCnfCombination_t VIWndSplash::NetCnfCombination()
{
    return m_configs[m_connect.Selection()].m_comb;
}

// 0x0057BDD8.
void VIWndSplash::Connect()
{
    if (VIMemCardStatus(0))
    {
        McErr(this, VIVect2I(320, 236), 4);
        m_mcState = 1;
        return;
    }
    int same = VIIsMcSameSession();
    if (same != 1)
    {
        McErr(this, VIVect2I(320, 236), 4);
        m_mcState = 1;
        return;
    }
    int n = ScanNetworkConfigurations();
    switch (n)
    {
    case -15:
    {
        unsigned short buf[512];
        VIString::strcpy16(buf, 1024, Word(UI(), 806));
        VIString::strcat16(buf, 1024, "\n\n");
        VIString::strcat16(buf, 1024, Word(UI(), 984));
        m_message.Setup(Word(UI(), 767), buf, (VIMESSAGEPROMPT)0, VIVect2I(320, 246), 400);
        m_message.SetVisible(true);
        SetFocus(&m_message, true);
        break;
    }
    case 0:
        m_message.Setup(Word(UI(), 767), Word(UI(), 783), (VIMESSAGEPROMPT)0,
                        VIVect2I(320, 246), 400);
        m_message.SetVisible(true);
        SetFocus(&m_message, true);
        break;
    case 1:
        m_connect.SetNetConfig(m_configs, m_numConfigs, m_default);
        m_connect.SetVisible(true);
        SetFocus(&m_connect, true);
        break;
    default:
        printf("ScanNetworkConfigurations() returned %d.\n", n);
        m_message.Setup(Word(UI(), 767), Word(UI(), 1774), (VIMESSAGEPROMPT)0,
                        VIVect2I(320, 246), 400);
        m_message.SetVisible(true);
        SetFocus(&m_message, true);
        break;
    }
}

// 0x0057C090.  Returns ezNetCnf's error, or 1 with at least one usable
// combination and -1 with none.
int VIWndSplash::ScanNetworkConfigurations()
{
    m_default = 0;
    m_numConfigs = 0;
    int r = ezNetCnfGetCombinationList("mc0:", &sList);
    if (r <= 0)
        return r;
    ezNetCnfCombination_t *def = ezNetCnfGetDefault(&sList);
    for (int i = 0; i < sList.count; i++)
    {
        ezNetCnfCombination_t *c = ezNetCnfGetCombination(&sList, i + 1);
        if (c->status == 0)
            continue;
        if (c == def)
            m_default = m_numConfigs;
        m_configs[m_numConfigs].m_comb = *c;
        m_configs[m_numConfigs].m_number = c->name[strlen(c->name) - 1] - '0';
        strcpy(m_configs[m_numConfigs].m_ifc, c->ifc);
        strcpy(m_configs[m_numConfigs].m_dev, c->dev);
        m_numConfigs++;
    }
    return m_numConfigs ? 1 : -1;
}
