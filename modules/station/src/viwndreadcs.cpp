// viwndreadcs.cpp (name ours) -- STATION 0x002F2C38 .. 0x002F4EF0: one message
// read from the customer-service inbox.  Its category, subcategory, date and
// text, formatted into one buffer and wrapped eight rows at a time; Enter offers
// Reply (the embedded VIWndCSRequest, as a reply) and Delete.  The VIMap<int,
// int> and VIArray<VIItemStat> copies between Wrap and __tf are template
// instantiations this TU does not write.
#include "station.h"

// One WORDS.TXT entry in the current language, as SETUP's windows read them.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// The "..." after a status line: one dot more every 15 frames, three at most.
static char sDots;                 // 0x00350380
static char sTicks;                // 0x00350381
// The option list's two lines, Reply and Delete: Init allocates them.
static unsigned short **sLines;    // 0x00350384

// 0x002F2C38.
VIWndReadCS::VIWndReadCS()
{
    m_options = 0;
    m_key = 0;
    m_repeat = 0;
    m_time = 0;
}

// 0x002F2CA8.
VIWndReadCS::~VIWndReadCS()
{
    Free();
}

// 0x002F2D20.  The option list, the message box and the reply, all hidden.
int VIWndReadCS::Init()
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
    m_request.SetAppPtr(GetAppPtr());
    m_request.Init(1);
    m_request.SetVisible(false);
    AddChild(&m_request);
    sLines = new unsigned short *[2];
    sLines[0] = new unsigned short[32];
    sLines[1] = new unsigned short[32];
    sLines[0][0] = 0;
    sLines[1][0] = 0;
    m_categoryId = 0;
    m_subcategoryId = 0;
    m_key = 0;
    m_repeat = 0;
    m_time = 0;
    return 0;
}

// 0x002F2E68.
int VIWndReadCS::Free()
{
    if (m_options)
    {
        RemoveChild(&m_request);
        m_request.Free();
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
    m_category[0] = 0;
    m_subcategory[0] = 0;
    m_date[0] = 0;
    m_text[0] = 0;
    m_full[0] = 0;
    m_categoryId = 0;
    m_subcategoryId = 0;
    m_key = 0;
    m_repeat = 0;
    m_time = 0;
    return 0;
}

// 0x002F2F80.
void VIWndReadCS::OnDrawSelf()
{
    VIUI *ui = UI();
    VIStation *station = (VIStation *)GetAppPtr();
    VICSClient *client = station->m_renderer.GetCSInterface()->GetClient();
    long now = VITimer::GetTime();
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    if (--sTicks <= 0)
    {
        sTicks = 15;
        if (++sDots == 3)
            sDots = 0;
    }
    switch (m_key)
    {
    case 19:
    case 20:
    case 138:
    case 139:
        m_repeat -= dt;
        if (m_repeat <= 0.0f)
        {
            m_repeat = 0.05f;
            if (m_key == 138 || m_key == 19)
                Prev();
            else if (m_key == 139 || m_key == 20)
                Next();
        }
        break;
    }

    unsigned short buf[256];
    ui->Begin2D();
    m_window.Draw();
    if (m_dirty)
    {
        m_full[0] = 0;
        VIString::snprintf16(m_full, sizeof(m_full), "%S: %S\n%S: %S\n%S: %S\n\n%S",
                             Word(ui, 842), m_category, Word(ui, 863), m_subcategory,
                             Word(ui, 985), m_date, m_text);
        m_rows = Wrap(m_full, m_lines, 8704, 1024, m_window.Width() - 90);
        m_top = 0;
        m_dirty = 0;
    }
    float t;
    if (m_rows <= 8)
        t = 0.0f;
    else
        t = (float)m_top / (float)(m_rows - 8);
    if (station->m_xB40C == (int)this || station->m_xB40C == (int)&m_message ||
        station->m_xB40C == (int)m_options)
    {
        m_window.DrawFrameVScroll(40, 1, m_window.Width() - 80, 8, t);
        for (int i = 0; i < 8; i++)
        {
            if (m_lines[i + m_top].m_start)
            {
                VIString::strcpy16(buf, sizeof(buf), m_lines[i + m_top].m_start);
                buf[m_lines[i + m_top].m_len] = 0;
                m_window.TextBox(buf, 40, i + 1, m_window.Width() - 80, 1, (VIUICOLOR)0, 0);
            }
        }
    }
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 422), Word(ui, 217));
    m_window.Text(buf, 40, 10, (VIUICOLOR)0, 0);
    unsigned short back[32];
    VIString::snprintf16(back, sizeof(back), "|t%S", Word(ui, 161));
    if (!client->GetState())
        m_status.Close(true);
    else
        m_status.Open(false);
    char d;
    switch (client->GetState())
    {
    case 0:
        if (m_deleting)
        {
            m_deleting = 0;
            VIUI *sui = &station->m_ui;
            station->Message(Word(sui, 987));
            station->m_ui.SoundPlay((VIUISoundId)8);
            GetParent()->OnMessage(this, 260, 0, 0);
        }
        break;
    case 1:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 844));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 2:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 845));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 3:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 846));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 4:
        m_deleting = 1;
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 847));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 5:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 848));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 6:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 849));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
    box:
        m_status.SetTitle(m_status.T(271));
        m_status.Draw();
        m_status.TextBox(buf, 40, 1, m_status.Width() - 60, 1, (VIUICOLOR)0, 0);
        m_status.TextBox(back, 40, 3, m_status.Width() - 60, 1, (VIUICOLOR)0, 0);
        break;
    default:
        VIString::snprintf16(buf, sizeof(buf), "%S. (%S: %i)", Word(ui, 865), Word(ui, 819),
                             client->GetState());
        m_status.SetTitle(m_status.T(1012));
        m_status.Draw();
        m_status.TextBox(buf, 40, 1, m_status.Width() - 60, 2, (VIUICOLOR)0, 0);
        m_status.TextBox(back, 40, 3, m_status.Width() - 60, 1, (VIUICOLOR)0, 0);
        break;
    }
    ui->End2D();
}

// 0x002F38A0.  The arrows scroll (and repeat while held, OnDrawSelf); Enter
// opens the option list; nothing moves while the client is busy.
void VIWndReadCS::OnKeyDown(int key)
{
    VIStation *station = (VIStation *)GetAppPtr();
    VICSClient *client = station->m_renderer.GetCSInterface()->GetClient();
    m_key = 0;
    switch (key)
    {
    case 19:
    case 138:
        if (client->GetState() > 0)
            break;
        m_key = key;
        m_repeat = 0.45f;
        Prev();
        break;
    case 20:
    case 139:
        if (client->GetState() > 0)
            break;
        m_key = key;
        m_repeat = 0.45f;
        Next();
        break;
    case 13:
    case 134:
        if (client->GetState() > 0)
            break;
        station->m_ui.SoundPlay((VIUISoundId)11);
        m_options->SetVisible(true);
        station->SetActive(m_options);
        break;
    case 27:
    case 132:
        station->m_ui.SoundPlay((VIUISoundId)8);
        GetParent()->OnMessage(this, 260, 0, 0);
        break;
    }
}

// 0x002F3A88.
void VIWndReadCS::OnKeyUp(int key)
{
    if (key == m_key)
        m_key = 0;
    switch (key)
    {
    case 19:
    case 20:
    case 138:
    case 139:
        break;
    default:
        m_key = 0;
        break;
    }
}

// 0x002F3AD8.  295: the focus arrived; 308: the option list closed (p1 the
// line, p2 nonzero for Enter); 307 and 309/260: a child is done.
void VIWndReadCS::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIStation *station = (VIStation *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != m_options && (VIWnd *)p1 != &m_request)
        {
            m_window.Close(true);
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 500, 11, 0);
            m_window.SetTitle(m_window.T(933));
            m_window.Open(false);
            VIString::strcpy16(sLines[0], 64, m_window.T(534));
            VIString::strcpy16(sLines[1], 64, m_window.T(986));
            m_request.SetVisible(false);
            m_options->Setup(m_window.T(422), (const unsigned short **)sLines, 2,
                             VIVect2I(320, 180), 256);
            m_status.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 375, 4, 0);
            m_status.SetTitle(m_status.T(271));
            m_status.Open(true);
            m_deleting = 0;
            m_time = VITimer::GetTime();
            m_key = 0;
            m_repeat = 0;
        }
        break;
    case 308:
        if (!p2)
            station->SetActive(this);
        else
        {
            station->SetActive(this);
            if (p1 == 1)
                station->m_renderer.GetCSInterface()->GetClient()->DeleteMessage(m_id);
            else if (p1 == 0)
            {
                m_request.Refresh();
                m_request.SetVisible(true);
                station->SetActive(&m_request);
                m_request.SetCategory(m_category);
                m_request.SetSubcategory(m_subcategory);
                m_request.SetCategoryId(m_categoryId);
                m_request.SetCategoryIds(m_categoryId, m_subcategoryId);
                m_request.SetText(m_window.T(""));
                m_request.SetReplyTo(m_id);
            }
        }
        break;
    case 307:
        if (m_deleting)
            m_deleting = 0;
        station->SetActive(this);
        break;
    case 309:
        m_request.SetVisible(false);
        station->SetActive(this);
        break;
    case 260:
        m_request.SetVisible(false);
        station->SetActive(this);
        break;
    }
}

// 0x002F3E68.
void VIWndReadCS::Next()
{
    VIStation *station = (VIStation *)GetAppPtr();
    if (m_top < m_rows - 8)
    {
        m_top++;
        station->m_ui.SoundPlay((VIUISoundId)11);
    }
    else
        station->m_ui.SoundPlay((VIUISoundId)9);
}

// 0x002F3EE0.
void VIWndReadCS::Prev()
{
    VIStation *station = (VIStation *)GetAppPtr();
    if (m_top > 0)
    {
        m_top--;
        station->m_ui.SoundPlay((VIUISoundId)11);
    }
    else
        station->m_ui.SoundPlay((VIUISoundId)9);
}

// 0x002F3F48.
const unsigned short *VIWndReadCS::Category()
{
    return m_category;
}

// 0x002F3F50.  Each setter marks the text for a re-wrap.
void VIWndReadCS::SetCategory(const unsigned short *s)
{
    VIString::strcpy16(m_category, sizeof(m_category), s);
    m_dirty = 1;
}

// 0x002F3F90.
const unsigned short *VIWndReadCS::Subcategory()
{
    return m_subcategory;
}

// 0x002F3F98.
void VIWndReadCS::SetSubcategory(const unsigned short *s)
{
    VIString::strcpy16(m_subcategory, sizeof(m_subcategory), s);
    m_dirty = 1;
}

// 0x002F3FD8.
const unsigned short *VIWndReadCS::Text()
{
    return m_text;
}

// 0x002F3FE0.
void VIWndReadCS::SetText(const unsigned short *s)
{
    VIString::strcpy16(m_text, sizeof(m_text), s);
    m_dirty = 1;
}

// 0x002F4020.
const unsigned short *VIWndReadCS::Date()
{
    return m_date;
}

// 0x002F4028.
void VIWndReadCS::SetDate(const unsigned short *s)
{
    VIString::strcpy16(m_date, sizeof(m_date), s);
    m_dirty = 1;
}

// 0x002F4068.  A row ends at a line break (kept in the row), or before the
// character that makes it wider than width -- after the last space, if the
// row has one.
int VIWndReadCS::Wrap(unsigned short *text, VIReadCSLine *lines, int len, int maxLines,
                      int width)
{
    unsigned short buf[256];
    VIUI *ui = UI();
    int space = 0;
    int i = 0;
    int n = 0;
    for (int k = 0; k < maxLines; k++)
    {
        lines[k].m_start = 0;
        lines[k].m_len = 0;
        lines[k].m_break = 0;
    }
    unsigned short *start = text;
    while (i < len - 1 && n < maxLines && text[i])
    {
        if (text[i] == 13)
            text[i] = 32;
        if (text[i] == 10)
        {
            lines[n].m_start = start;
            lines[n].m_len = &text[i + 1] - start;
            lines[n].m_break = 1;
            i++;
            start = &text[i];
            n++;
            space = 0;
        }
        else
        {
            if (text[i] == 32)
                space = i;
            int count = &text[i] - start;
            memcpy(buf, start, count * 2);
            buf[&text[i] - start] = 0;
            if (ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf) > width)
            {
                lines[n].m_start = start;
                if (space)
                {
                    lines[n].m_len = &text[space + 1] - start;
                    lines[n].m_break = 1;
                    i = space + 1;
                    start = &text[i];
                }
                else
                {
                    lines[n].m_len = count;
                    lines[n].m_break = 1;
                    start = &text[i];
                }
                n++;
                space = 0;
            }
            else
                i++;
        }
    }
    int rest = &text[i] - start;
    if (rest)
    {
        lines[n].m_start = start;
        lines[n].m_len = rest;
        n++;
    }
    return n;
}
