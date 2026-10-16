// viwnddnas.cpp (name ours) -- SETUP 0x00570F20 .. 0x00572768.  The DNAS
// authentication screen: a status panel over a mock of the title screen.
#include "setup.h"

// One WORDS.TXT entry in the current language (MCERR's own helper).
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00570F20.
VIWndDnas::VIWndDnas()
{
}

// 0x00570F78.
VIWndDnas::~VIWndDnas()
{
}

// 0x00570FF0.
int VIWndDnas::Init(VIScene *scene)
{
    m_scene = scene;
    m_message.Init();
    m_message.SetVisible(false);
    AddChild(&m_message);
    m_icon = -1;
    return 0;
}

// 0x00571048.
void VIWndDnas::Free()
{
    RemoveChild(&m_message);
    m_message.Free();
}

// 0x00571078.  m_scene is never held across a call: the dictionary is read
// into a block local BEFORE the hash call, the raster at the top.
void VIWndDnas::OnDrawSelf()
{
    VIUI *ui = UI();
    VIRaster *raster = m_scene->m_raster;
    if (m_icon == -1)
    {
        VIDictionary *dict = m_scene->m_dict;
        dict->FindTyped(VIHashResourceID_Icon("dnas_64"), (VIResourceType)1, m_icon);
    }
    ui->Begin2D();
    int busy = 0;
    DrawFakeSplashInterface();
    m_error = 0;
    unsigned short buf[512];
    VIString::strcpy16(buf, sizeof(buf), Word(UI(), 803));
    VIString::strcat16(buf, sizeof(buf), " ");
    if (m_status.code == 0 && m_status.sub_code == 0)
    {
        busy = 1;
        VIString::strcat16(buf, sizeof(buf), Word(UI(), 777));
    }
    else
    {
        switch (m_status.code)
        {
        case 1:
            VIString::strcat16(buf, sizeof(buf), Word(UI(), 762));
            break;
        case 2:
        case 3:
        case 4:
            busy = 1;
            switch (m_status.progress)
            {
            case 0:
                VIString::strcat16(buf, sizeof(buf), Word(UI(), 775));
                break;
            case 1:
                VIString::strcat16(buf, sizeof(buf), Word(UI(), 758));
                break;
            case 2:
                VIString::strcat16(buf, sizeof(buf), Word(UI(), 748));
                break;
            case 3:
                busy = 0;
                VIString::strcat16(buf, sizeof(buf), Word(UI(), 749));
                break;
            case 5:
                VIString::strcat16(buf, sizeof(buf), Word(UI(), 766));
                break;
            case 7:
                VIString::strcat16(buf, sizeof(buf), Word(UI(), 765));
                break;
            case 8:
                VIString::strcat16(buf, sizeof(buf), Word(UI(), 774));
                break;
            }
            break;
        case 5:
            VIString::strcat16(buf, sizeof(buf), Word(UI(), 764));
            break;
        case 6:
            m_error = 1;
            VIString::strcpy16(buf, sizeof(buf), ErrorText(m_status));
            break;
        case 7:
            m_error = 1;
            VIString::strcpy16(buf, sizeof(buf), ErrorText(m_status));
            break;
        case 8:     // empty, but the jump table has nine entries
        case 9:
            break;
        }
    }
    if (m_error)
        busy = 0;
    int indent = busy ? 150 : 40;
    // `W - (indent + 40)`: fold re-associates it to (W - 40) - indent, the
    // target's order; `W - 40 - indent` comes out as W - (indent + 40).
    int rows = m_window3.CountLines(buf, m_window3.Width() - (indent + 40));
    if (m_error)
        m_window3.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 470, rows + 3, 0);
    else if (busy)
        m_window3.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 470, 10, 0);
    else
        m_window3.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 470, rows + 2, 0);
    m_window3.Draw();
    if (m_icon >= 0)
    {
        if (busy)
        {
            // Through a reference, so m_open is read off &m_window3's register;
            // TopLeft() twice is the target's two by-value temporaries.
            VIUIWindow &w = m_window3;
            raster->Blit(m_icon, 0, VIVect2I(w.TopLeft().x + 20, w.TopLeft().y + 13),
                         (VIAlphaBlendMode)3, w.m_open);
        }
    }
    if (busy)
    {
        m_window3.TextBox(buf, indent, 3, m_window3.Width() - (indent + 40), 4, VIUICOLOR_0, 0);
        m_window3.TextBox(Word(UI(), 1813), 40, 8, m_window3.Width() - 80, 2, VIUICOLOR_0, 8);
    }
    else
        m_window3.TextBox(buf, indent, 1, m_window3.Width() - (indent + 40), rows, VIUICOLOR_0, 0);
    if (m_error)
    {
        VIString::str8to16(buf, sizeof(buf), "|x ");
        if (m_status.sub_code == -9999)
            VIString::strcat16(buf, sizeof(buf), Word(UI(), 786));
        else
            VIString::strcat16(buf, sizeof(buf), Word(UI(), 760));
        if (busy)
            m_window3.Text(buf, 40, 6, VIUICOLOR_0, 0);
        else
            m_window3.Text(buf, 40, rows + 2, VIUICOLOR_0, 0);
    }
    ui->End2D();
}

// 0x00571568.  Enter or X closes it.
void VIWndDnas::OnKeyDown(int key)
{
    if (key == 13 || key == 134)
        SetFocus(0, true);
}

// 0x005715A0.
void VIWndDnas::OnKeyUp(int key)
{
}

// 0x005715A8.
void VIWndDnas::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295)
    {
        if (p2 == (int)this && p1 != (int)&m_message)
        {
            m_window.Init(UI(), VIVect2I(0, 350), VIVect2I(304, 360), 440, 1, 0);
            m_window.Open(true);
            m_window3.Close(true);
            m_window3.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 470, 5, 0);
            m_window3.Open(false);
            m_window3.SetTitle(Word(UI(), 761));
            m_window2.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 246), 480, 6, 0);
            m_window2.Open(false);
            m_window2.SetTitle(Word(UI(), 767));
            m_error = 0;
            memset(&m_status, 0, sizeof(m_status));
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x00571758.  The title screen as it would look, under the panel.
void VIWndDnas::DrawFakeSplashInterface()
{
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F yellow(1.0f, 1.0f, 0.0f, 1.0f);
    VIVect2I unused;            // zeroed at sp+32 and never read
    VIUI *ui = UI();
    ui->Blit(VIHashResourceID("splash00.tga"), VIVect2I(0, 0), (VIAlphaBlendMode)0, 1.0f, 0);
    ui->Blit(VIHashResourceID("splash10.tga"), VIVect2I(320, 0), (VIAlphaBlendMode)0, 1.0f, 0);
    VISplashFxRender(1);
    int w = m_window.Width() / 3;
    int x[5] = { 0, w, w * 2, w * 3, w * 4 };
    unsigned short buf[256];
    VIString::str8to16(buf, sizeof(buf), "|>");
    VIString::strcat16(buf, sizeof(buf), Word(UI(), 790));
    VIString::strcat16(buf, sizeof(buf), "|<");
    m_window.TextBox(m_window.T(buf), x[0] - 10, 0, x[1] - x[0] + 20, 1, yellow, 9);
    m_window.TextBox(Word(UI(), 1635), x[1] - 20, 0, x[2] - x[1] + 20, 1, white, 9);
    m_window.TextBox(Word(UI(), 422), x[2] - 10, 0, x[3] - x[2] + 20, 1, white, 9);
}

// 0x00571990.  The stage-6/7 error: the known sub_codes' explanation, the
// number, and (for most) the call-SCEA line.
const unsigned short *VIWndDnas::ErrorText(const sceDNAS2Status &s)
{
    unsigned short buf[256];
    int contact = 1;
    buf[0] = 0;
    switch (s.sub_code)
    {
    case -9999:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1625));
        break;
    case 0:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 786));
        break;
    case -101:
        contact = 0;
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1636));
        break;
    case -102:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1637));
        break;
    case -103:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1637));
        break;
    case -104:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1638));
        break;
    case -105:
        contact = 0;
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1639));
        break;
    case -106:
        contact = 0;
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1640));
        break;
    case -107:
        contact = 0;
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1640));
        break;
    case -108:
        contact = 0;
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1640));
        break;
    case -109:
        contact = 0;
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 953));
        break;
    case -701:
    case -702:
    case -703:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1645));
        break;
    case -201:
    case -202:
    case -203:
    case -204:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1641));
        break;
    case -401:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1642));
        break;
    case -402:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1643));
        break;
    case -403:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1643));
        break;
    case -404:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1642));
        break;
    case -601:
    case -610: case -611: case -612: case -613: case -614: case -615: case -616: case -617:
    case -618: case -619: case -620: case -621: case -622: case -623: case -624: case -625:
        contact = 0;
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1644));
        break;
    default:
        VIString::strcpy16(buf, sizeof(buf), Word(UI(), 1646));
        break;
    }
    if (s.sub_code == -9999)
        VIString::strcpy16(m_errorText, sizeof(m_errorText), Word(UI(), 1671));
    else
        VIString::snprintf16(m_errorText, sizeof(m_errorText), Word(UI(), 1647), s.sub_code);
    VIString::strcat16(m_errorText, sizeof(m_errorText), "\n\n");
    VIString::strcat16(m_errorText, sizeof(m_errorText), buf);
    if (contact)
        VIString::strcat16(m_errorText, sizeof(m_errorText),
                           "\n\nPlease write down the error code number, and contact SCEA at "
                           "1-866-466-5333 or via SCEA's website, www.us.playstation.com.");
    return m_errorText;
}
