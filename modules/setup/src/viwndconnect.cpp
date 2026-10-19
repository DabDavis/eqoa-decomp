// viwndconnect.cpp (name ours) -- SETUP 0x0056E3F0 .. 0x0056FF80.  Bringing the
// network up: load the adaptor's IRX chain, set the ezNetCtl combination off
// the memory card, and show the dial progress until unet is started.
#include "setup.h"

extern "C" int sceSifLoadModule(const char *path, int argLen, const char *args);
extern "C" int sceGsSyncV(int mode);
extern "C" int ezNetCtlGetStatus(void *status);
extern "C" int ezNetCtlSetCombination(const char *dev, const char *name);
extern "C" int ezNetCtlDownInterface(int n);
extern "C" int ezNetCtlUpInterface(int n);
extern "C" int ezNetCnfGetProperties(const char *dev, const char *name, void *props);

int   VIEELoadIrx(const char *name, int argLen, const char *args);
VI_RET(int, void)  VIEEUnloadIrx(int id);   // void here: an int return moves BeginConnect's -1 to v1
void  preparePowerOff();
VIWnd *McErrWnd();
int   McErr(VIWnd *owner, const VIVect2I &center, int screen);
void  unet_startup_async();
int   unet_startup_async_status();

// ezNetCtl's status: a state and the PPP phase as text.
struct VINetCtlStatus
{
    int  m_x00;
    int  m_x04;
    int  m_x08;
    int  m_state;      // +0x0C  2 redial, 3 up, 5 failed
    char m_text[256];  // +0x10  "PHASE LCP", "CONNECT 57600", ...
};

// The dial-up profile's three phone numbers decide the dial timeout.
struct VINetCnfProperties
{
    char m_x0000[2048];
    char m_phone1[256];   // +0x800
    char m_phone2[256];   // +0x900
    char m_phone3[256];   // +0xA00
    char m_x0B00[2112];
};

static int  sAuthSeen = 0;      // the dial reached "PHASE Auth"
static int  sTimeout = 180;     // frames
static char sDots = 0;
static char sDotTimer = 15;
static VINetCtlStatus sStatus;

static inline const unsigned short *Word(int id)
{
    VIUI *ui = VIWnd::UI();
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// The menu font's width of a string.
static inline int TextWidth(const char *s)
{
    VIUI *ui = VIWnd::UI();
    return ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, s);
}

static inline int TextWidth(const unsigned short *s)
{
    VIUI *ui = VIWnd::UI();
    return ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, s);
}

// 0x0056E3F0.
VIWndConnect::VIWndConnect()
{
    m_smap = -1;
    m_dev9 = -1;
    m_an986 = -1;
    m_ppp = -1;
    m_pppoe = -1;
    m_spduart = -1;
    m_cxtmdm = -1;
    m_child = 0;
    m_retry = 0;
    m_cancelled = 0;
    m_noCancel = 0;
}

// 0x0056E470.
VIWndConnect::~VIWndConnect()
{
    if (m_child)
    {
        RemoveChild(m_child);
        m_child = 0;
    }
}

// 0x0056E4F0.
int VIWndConnect::Init()
{
    m_sel = 0;
    m_connecting = 0;
    m_unetStarted = 0;
    m_child = 0;
    m_retry = 0;
    m_message.Init();
    m_message.SetVisible(false);
    AddChild(&m_message);
    m_cancelled = 0;
    m_noCancel = 0;
    return 0;
}

// 0x0056E558.
void VIWndConnect::Free()
{
    RemoveChild(&m_message);
    m_message.Free();
}

// 0x0056E588.
void VIWndConnect::OnDrawSelf()
{
    VIUI *ui = UI();
    m_noCancel = 0;
    if (GetFocus() != this || m_connecting)
        m_window.ResetTickPulse();
    if (m_child)
    {
        RemoveChild(m_child);
        m_child = 0;
    }
    if (m_connecting && !m_unetStarted && m_deadline < VITimer::GetTime())
    {
        ezNetCtlDownInterface(0);
        sAuthSeen = 0;
        UI()->SoundPlay((VIUISoundId)8);
        m_deadline = VITimer::GetTime() + 100000;
    }
    ui->Begin2D();
    m_window.Draw();
    m_window.DrawFrame(40, 1, m_window.Width() - 80, 2, 32);
    char buf[256];
    unsigned short text[512];
    sprintf(buf, "%d", m_netConfig[m_sel].m_number);
    m_window.TextBox(m_window.T(buf), 40, 1, 60, 1, (VIUICOLOR)0, 0);
    m_window.TextBox(m_window.T(m_netConfig[m_sel].m_ifc), 60, 1, m_window.Width() - 100, 1,
                     (VIUICOLOR)0, 2);
    m_window.TextBox(m_window.T(m_netConfig[m_sel].m_dev), 60, 2, m_window.Width() - 100, 1,
                     (VIUICOLOR)0, 2);
    VIString::str8to16(text, 1024, "|x ");
    VIString::strcat16(text, 1024, Word(756));
    VIString::strcat16(text, 1024, "   |t ");
    VIString::strcat16(text, 1024, Word(750));
    m_window.Text(text, 40, 4, (VIUICOLOR)0, 0);
    if (--sDotTimer <= 0)
    {
        sDotTimer = 15;
        if (++sDots == 3)
            sDots = 0;
    }
    if (m_connecting)
    {
        int w;
        ezNetCtlGetStatus(&sStatus);
        if (m_unetStarted)
        {
            if (unet_startup_async_status() == 0 && sTimeout == 0)
                SetFocus(0, true);
            else
            {
                if (sTimeout)
                    sTimeout--;
                m_window2.SetTitle(m_window2.T(""));
                VIString::strcpy16(text, 1024, Word(776));
                m_noCancel = 1;
                w = TextWidth(text);
                if (w < 200)
                    w = 200;
                m_window2.Init(UI(), VIVect2I(0, 0), m_window.Center(), w + 120, 3, 0);
                for (char i = 0; i < sDots + 1; i++)
                    VIString::strcat16(text, 1024, ".");
                m_window2.Draw();
                m_window2.TextBox(text, 40, 1, m_window2.Width(), 1, (VIUICOLOR)0, 0);
            }
        }
        else
        {
            m_window2.SetTitle(m_netConfig[m_sel].m_dev);
            w = TextWidth(m_netConfig[m_sel].m_dev);
            if (w < 500)
                w = 500;
            strcpy(buf, sStatus.m_text);
            if (!strcmp(buf, "PHASE Auth"))
            {
                sAuthSeen = 1;
                VIString::str16to8(buf, 256, Word(1793));
            }
            if (!strcmp(buf, "PHASE Starting"))
                VIString::str16to8(buf, 256, Word(1794));
            if (!strcmp(buf, "PHASE LCP"))
                VIString::str16to8(buf, 256, Word(1795));
            if (!strcmp(buf, "PHASE IPCP"))
                VIString::str16to8(buf, 256, Word(1795));
            int translated = strcmp(buf, sStatus.m_text) != 0;
            if (strcmp(buf, "Connected") && strncmp(buf, "CONNECT ", 8))
                for (char i = 0; i < sDots + 1; i++)
                    strcat(buf, ".");
            VIString::strcpy16(text, 1024, Word(803));
            VIString::strcat16(text, 1024, " ");
            VIString::strcat16(text, 1024, buf);
            VIString::strcat16(text, 1024, "\n");
            if (translated)
            {
                VIString::strcat16(text, 1024, "\n");
                VIString::strcat16(text, 1024, sStatus.m_text);
                VIString::strcat16(text, 1024, "\n");
            }
            VIString::strcat16(text, 1024, "\n");
            VIString::strcat16(text, 1024, "|t ");
            VIString::strcat16(text, 1024, Word(753));
            int rows = m_window2.CountLines(text, w - 80);
            m_window2.Init(UI(), VIVect2I(0, 0), m_window.Center(), w, rows + 1, 0);
            m_window2.Draw();
            m_window2.TextBox(text, 40, 1, w - 80, rows, (VIUICOLOR)0, 0);
            switch (sStatus.m_state)
            {
            case 3:
                if (--sTimeout == 0)
                {
                    sTimeout = 90;
                    m_unetStarted = 1;
                    unet_startup_async();
                }
                break;
            case 2:
                sTimeout = 180;
                printf("retrying... (reset timeout)\n");
                m_deadline = VITimer::GetTime() + m_timeout;
                break;
            case 5:
                sTimeout = 180;
                VIString::snprintf16(text, 1024, "%S\n\n%S\n\n%s", Word(1624),
                                     sAuthSeen ? Word(1669) : Word(1625), sStatus.m_text);
                m_message.Setup(Word(771), text, (VIMESSAGEPROMPT)0, m_window.Center(), 400);
                m_message.SetVisible(true);
                SetFocus(&m_message, true);
                m_connecting = 0;
                m_unetStarted = 0;
                break;
            default:
                sTimeout = 180;
                break;
            }
        }
    }
    if (m_retry && --m_retry == 0)
    {
        UI()->SoundPlay((VIUISoundId)15);
        m_connecting = 1;
        BeginConnect();
        m_window2.Close(true);
        int w = TextWidth(m_netConfig[m_sel].m_dev);
        if (w < 300)
            w = 300;
        m_window2.Init(UI(), VIVect2I(0, 0), m_window.Center(), w + 120, 4, 0);
        m_window2.Open(false);
    }
    ui->End2D();
}

// 0x0056F028.
void VIWndConnect::OnKeyDown(int key)
{
    if (m_retry)
        return;
    switch (key)
    {
    case 17:
    case 136:
        if (!m_connecting)
        {
            m_sel = (m_sel + (m_count - 1)) % m_count;
            UI()->SoundPlay((VIUISoundId)11);
        }
        break;
    case 18:
    case 137:
        if (!m_connecting)
        {
            m_sel = (m_sel + 1) % m_count;
            UI()->SoundPlay((VIUISoundId)11);
        }
        break;
    case 13:
    case 134:
        if (!m_connecting)
        {
            UI()->SoundPlay((VIUISoundId)15);
            m_connecting = 1;
            BeginConnect();
            m_window2.Close(true);
            int w = TextWidth(m_netConfig[m_sel].m_dev);
            if (w < 300)
                w = 300;
            m_window2.Init(UI(), VIVect2I(0, 0), m_window.Center(), w + 120, 4, 0);
            m_window2.Open(false);
        }
        break;
    case 27:
    case 132:
        if (!m_connecting)
        {
            UI()->SoundPlay((VIUISoundId)8);
            SetVisible(false);
            SetFocus(GetParent(), true);
        }
        else if (!m_noCancel)
        {
            ezNetCtlDownInterface(0);
            m_connecting = 0;
            UI()->SoundPlay((VIUISoundId)8);
            for (int i = 0; i < 60; i++)
                sceGsSyncV(0);
        }
        break;
    }
}

// 0x0056F2B0.
void VIWndConnect::OnKeyUp(int key)
{
}

// 0x0056F2B8.  0x1100 adds a child, 0x1102 is a child closing, 0x1104 hands it
// the focus and 0x1106 its result (MCERR's ExitMcErr); a 3 quits the screen.
void VIWndConnect::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    switch (msg)
    {
    case 295:
        if (p2 == (int)this)
        {
            m_window.Close(true);
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 400, 5, 0);
            if (p1 == (int)McErrWnd())
                m_window.Open(true);
            else
                m_window.Open(false);
            m_window.SetTitle(Word(815));
            m_unetStarted = 0;
            if (p1 != (int)McErrWnd() && m_child)
            {
                RemoveChild(m_child);
                m_child = 0;
            }
            m_cancelled = 0;
        }
        break;
    case 259:
        break;
    case 307:
        SetFocus(this, true);
        break;
    case 4356:
        SetFocus(w, true);
        break;
    case 4352:
        AddChild(w);
        break;
    case 4354:
        m_child = w;
        break;
    case 4358:
        if (p1 == 3)
        {
            m_cancelled = 1;
            UI()->SoundPlay((VIUISoundId)8);
            SetVisible(false);
            SetFocus(GetParent(), true);
            m_connecting = 0;
        }
        else
        {
            SetFocus(this, true);
            m_retry = 3;
        }
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x0056F4E8.
void VIWndConnect::EndConnect()
{
    ezNetCtlDownInterface(0);
    m_connecting = 0;
    m_unetStarted = 0;
}

// 0x0056F518.
int LoadIrxRetry(const char *name, int argLen, const char *args, int tries)
{
    char path[256];
    int  id;
    VIString::snprintf(path, 256, "cdrom0:\\MODULES\\%s;1", name);
    printf("%s\n", path);
    // A for-init copy of `tries`: the parameter itself as the counter takes
    // s0 straight from a3, where the target goes a3 -> v0 -> s0.
    for (int i = tries; (id = sceSifLoadModule(path, argLen, args)) < 0 && i > 0; i--)
        printf("sceSifLoadModule: cannot load '%s'\n", path);
    if (id < -1)
        id = -1;
    return id;
}

// 0x0056F5D8.
int LoadIrxRetry2(const char *name, int argLen, const char *args, int tries)
{
    char path[256];
    int  id;
    VIString::snprintf(path, 256, "cdrom0:\\MODULES2\\%s;1", name);
    printf("%s\n", path);
    // A for-init copy of `tries`: the parameter itself as the counter takes
    // s0 straight from a3, where the target goes a3 -> v0 -> s0.
    for (int i = tries; (id = sceSifLoadModule(path, argLen, args)) < 0 && i > 0; i--)
        printf("sceSifLoadModule: cannot load '%s'\n", path);
    if (id < -1)
        id = -1;
    return id;
}

// 0x0056F698.  Unload what the new combination does not need, load what it
// does, then set it.
int VIWndConnect::BeginConnect()
{
    ezNetCnfCombination_t info = m_netConfig[m_sel].m_comb;
    int sceEthernet = 0;
    int pppoe = 0;
    int sceModem = 0;
    switch (info.type)
    {
    case 2:
        pppoe = 1;
    case 3:
        sceEthernet = !strncmp(info.dev, "SCE", 3);
        break;
    case 1:
        sceModem = 0;
        if (!strncmp(info.dev, "SCE", 3))
            sceModem = 1;
        break;
    }
    if (!sceEthernet && m_smap >= 0)
    {
        printf("unloading smap\n");
        VIEEUnloadIrx(m_smap);
        m_smap = -1;
    }
    if (!sceModem && m_spduart >= 0)
    {
        printf("unloading spduart\n");
        VIEEUnloadIrx(m_spduart);
        m_spduart = -1;
    }
    if (!pppoe && m_pppoe >= 0)
    {
        printf("unloading pppoe\n");
        VIEEUnloadIrx(m_pppoe);
        m_pppoe = -1;
    }
    sceGsSyncV(0);
    sceGsSyncV(0);
    sceGsSyncV(0);
    sceGsSyncV(0);
    switch (info.type)
    {
    case 2:
        if (m_ppp < 0)
            m_ppp = VIEELoadIrx("PPP.IRX", 0, 0);
        if (m_pppoe < 0)
            m_pppoe = VIEELoadIrx("PPPOE.IRX", 0, 0);
    case 3:
        if (!strncmp(info.dev, "SCE", 3))
        {
            if (!(VIGetFeatureFlags() & 0x800) && m_dev9 < 0)
                m_dev9 = VIEELoadIrx("DEV9.IRX", 0, 0);
            if (m_smap < 0)
                m_smap = LoadIrxRetry("SMAP.IRX", 0, 0, 25);
            if (!(VIGetFeatureFlags() & 0x800) && m_smap >= 0)
                preparePowerOff();
        }
        else
        {
            if (!(VIGetFeatureFlags() & 0x800) && m_an986 < 0)
                m_an986 = VIEELoadIrx("AN986.IRX", 0, 0);
        }
        break;
    case 1:
        if (m_ppp < 0)
            m_ppp = VIEELoadIrx("PPP.IRX", 0, 0);
        if (!strncmp(info.dev, "SCE", 3))
        {
            if (!(VIGetFeatureFlags() & 0x800) && m_dev9 < 0)
                m_dev9 = VIEELoadIrx("DEV9.IRX", 0, 0);
            if (m_spduart < 0)
                m_spduart = LoadIrxRetry2("SPDUART.IRX", 33, "dial=cdrom0:\\CONF\\DIAL_SPD.CNF;1", 25);
            if (!(VIGetFeatureFlags() & 0x800) && m_spduart >= 0)
                preparePowerOff();
        }
        else
        {
            if (!(VIGetFeatureFlags() & 0x800) && m_cxtmdm < 0)
                m_cxtmdm = VIEELoadIrx("CXTMDM.IRX", 37, "dial=cdrom0:\\CONF\\NET\\DIAL_CXT.CNF;1");
        }
        break;
    default:
        return -1;
    }
    return SetCombination();
}

// 0x0056FB20.  The dial timeout is 45 s, 120 s when the profile has a phone
// number, 180 s when its properties cannot be read.
int VIWndConnect::SetCombination()
{
    ezNetCnfCombination_t info = m_netConfig[m_sel].m_comb;
    unsigned short text[256];
    if (VIMemCardStatus(0) != 0)
    {
        McErr(this, m_window.Center(), 4);
        m_connecting = 0;
        return -1;
    }
    int r = ezNetCtlSetCombination("mc0:", info.name);
    if (r < 0)
    {
        if (r == -8)
        {
            McErr(this, m_window.Center(), 4);
            m_connecting = 0;
            return -1;
        }
        char buf[256];   // unused, but it is in the frame: 256 bytes above the temps
        m_connecting = 0;
        m_unetStarted = 0;
        int w = TextWidth(m_netConfig[m_sel].m_dev);
        if (w < 300)
            w = 300;
        VIString::str8to16(text, 512, "\"");
        VIString::strcat16(text, 512, m_netConfig[m_sel].m_dev);
        VIString::strcat16(text, 512, "\"\n");
        if (strstr(m_netConfig[m_sel].m_dev, "SCE/Ethernet (Network Adaptor)") ||
            strstr(m_netConfig[m_sel].m_dev, "SCE/Modem (Network Adaptor)"))
            VIString::strcat16(text, 512, Word(1026));
        else
            VIString::strcat16(text, 512, Word(780));
        m_message.Setup(Word(767), text, (VIMESSAGEPROMPT)0, m_window.Center(), w + 80);
        m_message.SetVisible(true);
        SetFocus(&m_message, true);
    }
    else
    {
        VINetCnfProperties *props = (VINetCnfProperties *)memalign(64, sizeof(VINetCnfProperties));
        m_timeout = 180000;
        if (ezNetCnfGetProperties("mc0:", info.name, props) == 0)
        {
            if (props->m_phone1[0] || props->m_phone2[0] || props->m_phone3[0])
                m_timeout = 120000;
            else
                m_timeout = 45000;
        }
        free(props);
        ezNetCtlDownInterface(0);
        sceGsSyncV(0);
        sceGsSyncV(0);
        ezNetCtlUpInterface(0);
        sceGsSyncV(0);
        sceGsSyncV(0);
        ezNetCtlDownInterface(0);
        sceGsSyncV(0);
        sceGsSyncV(0);
        ezNetCtlUpInterface(0);
        sceGsSyncV(0);
        sceGsSyncV(0);
        sAuthSeen = 0;
        m_deadline = VITimer::GetTime() + m_timeout;
    }
VI_HOST_ONLY(return 0;) }
