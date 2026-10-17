#line 1 "C:/code-frontiers-gm/game/VIWndReadGram.cpp"
// VIWndReadGram (name RTTI's) -- CLIENT 0x00C89A90 .. 0x00C8B588: one gram read from
// the inbox, VIWndReadCS's twin for the gram client (VIClient+0x2EFD0).  From,
// Subject and Date over the text in one TextBox, scrolled seven rows at a time; Enter
// offers Reply (the embedded VIWndComposeGram, addressed back with the subject
// prefixed) and Delete.  The VIMap<int, int> and VIArray<VIItemStat> copies between
// Next and __tf are template instantiations this TU does not write.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// The "..." after a status line: one dot more every 15 frames, three at most.
static char sDots;                 // 0x00CD79A4
static char sTicks;                // 0x00CD79A5
// The option list's two lines, Reply and Delete: Init allocates them.
static unsigned short **sLines;    // 0x00CD79A8

// 0x00C89A90.
VIWndReadGram::VIWndReadGram()
{
    m_options = 0;
    m_dirty = 1;
    sLines = 0;
}

// 0x00C89B00.
VIWndReadGram::~VIWndReadGram()
{
    Free();
}

// 0x00C89B88.  The option list, the message box and the reply, all hidden.
int VIWndReadGram::Init()
{
    Free();
    m_options = new VIWndOptionList;
    if (!m_options)
        return -1;
    m_options->Init();
    m_options->SetVisible(false);
    m_options->SetAppPtr(GetAppPtr());
    AddChild(m_options);
    m_message.Init();
    m_message.SetVisible(false);
    AddChild(&m_message);
    m_compose.SetAppPtr(GetAppPtr());
    m_compose.Init(1);
    m_compose.SetVisible(false);
    AddChild(&m_compose);
    sLines = new unsigned short *[2];
    sLines[0] = new unsigned short[32];
    sLines[1] = new unsigned short[32];
    sLines[0][0] = 0;
    sLines[1][0] = 0;
    return 0;
}

// 0x00C89CB0.
void VIWndReadGram::Free()
{
    if (m_options)
    {
        RemoveChild(&m_compose);
        m_compose.Free();
        RemoveChild(&m_message);
        m_message.Free();
        RemoveChild(m_options);
        delete m_options;
        m_options = 0;
    }
    if (sLines)
    {
        delete [] sLines[0];
        delete [] sLines[1];
        delete [] sLines;
    }
    m_from[0] = 0;
    m_subject[0] = 0;
    m_date[0] = 0;
    m_text[0] = 0;
    m_full[0] = 0;
}

// 0x00C89DA8.
void VIWndReadGram::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart2EFD0 *gram = app->GetP2EFD0();
    if (--sTicks <= 0)
    {
        sTicks = 15;
        if (++sDots == 3)
            sDots = 0;
    }
    ui->Begin2D();
    m_window.Draw();
    if (m_dirty)
    {
        m_full[0] = 0;
        VIString::snprintf16(m_full, sizeof(m_full), "%S: %S\n%S: %S\n%S: %S\n\n%S",
                             Word(ui, 882), m_from, Word(ui, 883), m_subject,
                             Word(ui, 884), m_date, m_text);
        m_rows = m_window.CountLines(m_full, 360);
        m_top = 0;
        m_dirty = 0;
    }
    float t;
    if (m_rows < 8)
        t = -1.0f;
    else
        t = (float)m_top / (float)(m_rows - 7);
    m_window.DrawFrameVScroll(40, 1, m_window.Width() - 80, 7, t);
    m_window.TextBox(m_full, 40, 1, m_window.Width() - 80, 7, (VIUICOLOR)0, m_top << 24);
    unsigned short buf[128];
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 422), Word(ui, 217));
    m_window.Text(buf, 40, 9, (VIUICOLOR)0, 0);
    unsigned short back[32];
    VIString::snprintf16(back, sizeof(back), "|t%S", Word(ui, 161));
    switch (gram->GetState())
    {
    case 4:
        VIString::strcpy16(buf, sizeof(buf), m_status.T(988));
        for (char d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        m_status.Draw();
        m_status.TextBox(buf, 40, 1, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        m_status.TextBox(back, 40, 3, m_status.Width() - 40, 1, (VIUICOLOR)0, 0);
        m_deleting = 1;
        break;
    case -8:
        if (m_deleting == 1)
        {
            m_deleting = -1;
            m_message.Setup(Word(ui, 934), Word(ui, 893), VIMESSAGEPROMPT_OK,
                            VIVect2I(320, 180), 300);
            m_message.SetVisible(true);
            app->SetActiveWindow(&m_message);
        }
        break;
    case 0:
        if (m_deleting)
        {
            m_deleting = 0;
            app->PlaySound(8);
            GetParent()->OnMessage(this, 260, 0, 0);
        }
        break;
    }
    ui->End2D();
    if (m_keys.Pulse(138))
        Prev();
    if (m_keys.Pulse(139))
        Next();
}

// 0x00C8A248.  Nothing moves while the gram client is busy.
void VIWndReadGram::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIClientPart2EFD0 *gram = app->GetP2EFD0();
    switch (key)
    {
    case 19:
    case 138:
        if (gram->GetState() > 0)
            break;
        m_keys.Press(138);
        break;
    case 20:
    case 139:
        if (gram->GetState() > 0)
            break;
        m_keys.Press(139);
        break;
    case 13:
    case 134:
        if (gram->GetState() > 0)
            break;
        app->PlaySound(11);
        m_options->SetVisible(true);
        app->SetActiveWindow(m_options);
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    }
}

// 0x00C8A3E0.
void VIWndReadGram::OnKeyUp(int key)
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

// 0x00C8A458.  295: the focus moved (p1 from, p2 to); 308: the option list closed (p1
// the line, p2 nonzero for Enter); 307 and 309/260: a child is done.
void VIWndReadGram::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != m_options)
        {
            {
                m_window.Close(true);
                m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 440, 10, 0);
                m_window.SetTitle(m_window.T(935));
                m_window.Open(false);
                VIString::strcpy16(sLines[0], 64, m_window.T(534));
                VIString::strcpy16(sLines[1], 64, m_window.T(986));
                m_options->Setup(m_window.T(422), (const unsigned short **)sLines, 2,
                                 VIVect2I(320, 180), 256);
                m_status.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 375, 4, 0);
                m_status.SetTitle(m_window.T(271));
                m_status.Open(true);
                m_deleting = 0;
            }
        }
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != m_options && (VIWnd *)p1 != &m_message &&
            (VIWnd *)p1 != &m_compose)
        {
            m_options->SetVisible(false);
            m_message.SetVisible(false);
            m_compose.SetVisible(false);
            m_keys.Reset();
        }
        if ((VIWnd *)p1 == this)
            m_keys.Reset();
        break;
    case 308:
        if (!p2)
            app->SetActiveWindow(this);
        else
        {
            app->SetActiveWindow(this);
            if (p1 == 1)
                app->GetP2EFD0()->DeleteMessage(m_id);
            else if (p1 == 0)
            {
                m_compose.SetVisible(true);
                app->SetActiveWindow(&m_compose);
                unsigned short subject[64];
                VIString::snprintf16(subject, sizeof(subject), "%S: %S", Word(UI(), 936),
                                     m_subject);
                m_compose.Refresh();
                m_compose.SetTo(m_from);
                m_compose.SetSubject(subject);
                subject[0] = 0;
                m_compose.SetBody(subject);
            }
        }
        break;
    case 307:
        if (m_deleting)
            m_deleting = 0;
        app->SetActiveWindow(this);
        break;
    case 309:
        m_compose.SetVisible(false);
        app->SetActiveWindow(this);
        break;
    case 260:
        m_compose.SetVisible(false);
        app->SetActiveWindow(this);
        break;
    }
}

// 0x00C8A7A0.  Each setter marks the text for a re-count.
const unsigned short *VIWndReadGram::From()
{
    return m_from;
}

// 0x00C8A7A8.
void VIWndReadGram::SetFrom(const unsigned short *s)
{
    VIString::strcpy16(m_from, sizeof(m_from), s);
    m_dirty = 1;
}

// 0x00C8A7E0.
const unsigned short *VIWndReadGram::Subject()
{
    return m_subject;
}

// 0x00C8A7E8.
void VIWndReadGram::SetSubject(const unsigned short *s)
{
    VIString::strcpy16(m_subject, sizeof(m_subject), s);
    m_dirty = 1;
}

// 0x00C8A820.
const unsigned short *VIWndReadGram::Date()
{
    return m_date;
}

// 0x00C8A828.
void VIWndReadGram::SetDate(const unsigned short *s)
{
    VIString::strcpy16(m_date, sizeof(m_date), s);
    m_dirty = 1;
}

// 0x00C8A860.
const unsigned short *VIWndReadGram::Text()
{
    return m_text;
}

// 0x00C8A868.
void VIWndReadGram::SetText(const unsigned short *s)
{
    VIString::strcpy16(m_text, sizeof(m_text), s);
    m_dirty = 1;
}

// 0x00C8A8A0.
void VIWndReadGram::Prev()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_top > 0)
    {
        m_top--;
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}

// 0x00C8A8F0.
void VIWndReadGram::Next()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_top < m_rows - 7)
    {
        m_top++;
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}
