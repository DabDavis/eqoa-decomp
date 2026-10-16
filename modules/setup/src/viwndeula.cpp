// viwndeula.cpp (name ours) -- SETUP 0x00572768 .. 0x00574238.  The licence
// agreement.  The names are the 2002 betas' own.
#include "setup.h"

// One WORDS.TXT entry in the current language (MCERR's helper).
inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00572768.
VIWndEula::VIWndEula()
{
    m_text = 0;
}

// 0x005727B0.
VIWndEula::~VIWndEula()
{
    Free();
}

// 0x00572810.  The text is read as bytes into a 64 KB buffer and widened into
// m_text, tabs turned into spaces.
int VIWndEula::Init()
{
    int len = 0;
    m_text = 0;
    m_dirty = 1;
    char *buf = new char[0x10000];
    if (!buf)
        return -1;
    char name[32];
    sprintf(name, "EULA_%s.TXT", VILanguage::GetName((VILanguageID)(char)VIGetGameOptions()->m_region));
    char *wad = new char[0x40000];
    int n = VIMemCardReadFile("EULA.WAD", wad, 0x40000);
    if (n > 0)
    {
        VIWadFile wf;
        if (wf.Init(wad, n) >= 0)
        {
            len = wf.GetSize(name);
            if (len > 0)
            {
                wf.Load(name, buf, 0x10000);
                wf.Free();
                printf("Loaded (%s) from memcard wad\n", name);
            }
            else
                len = 0;
        }
    }
    if (wad)
        delete [] wad;
    if (len == 0)
    {
        VIWadFile wf;
        if (wf.Init("data2\\eula.wad") >= 0)
        {
            len = wf.GetSize(name);
            if (len > 0)
            {
                wf.Load(name, buf, 0x10000);
                wf.Free();
                printf("Loaded (%s) from disc wad\n", name);
            }
            else
                len = 0;
        }
    }
    if (len == 0)
    {
        len = VIMemCardReadFile("EULA.TXT", buf, 0x10000);
        if (len <= 0)
        {
            VIFile f;
            if (f.Open("data\\EULA.TXT", 33) != 0)
            {
                if (buf)
                    delete [] buf;
                return -1;
            }
            f.Seek(0, VIFile::Seek_End);
            len = f.Tell();
            if (len <= 0)
            {
                if (buf)
                    delete [] buf;
                return -1;
            }
            f.Seek(0, VIFile::Seek_Set);
            f.Read(buf, len);
            f.Close();
        }
    }
    m_text = new unsigned short[len + 1];
    for (int i = 0; i < len; i++)
    {
        m_text[i] = (unsigned char)buf[i];
        if (m_text[i] == 9)
            m_text[i] = ' ';
    }
    m_text[len] = 0;
    if (buf)
        delete [] buf;
    return 0;
}

// 0x00572B28.
void VIWndEula::Free()
{
    if (m_text)
    {
        delete [] m_text;
        m_text = 0;
    }
}

// 0x00572B60.
void VIWndEula::OnDrawSelf()
{
    VIUI *ui = UI();
    if (m_asking)
        m_window.ResetTickPulse();
    if (m_dirty)
    {
        ComputeTextRows();
        m_dirty = 0;
        m_top = 0;
    }
    if (m_repeat)
    {
        if (--m_repeat == 0)
        {
            m_repeat = 2;
            if (m_repeatDir > 0)
                Down();
            else
                Up();
        }
    }
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(641, 449), (VIAlphaBlendMode)3, 1.0f);
    m_window.Draw();
    m_window.DrawFrameVScroll(40, 0, m_window.Width() - 80, 16, (float)m_top / (float)(m_numRows - 17));
    unsigned short line[256];
    for (int i = 0; i < 16; i++)
    {
        VIString::strcpy16(line, m_rows[i + m_top].m_len * 2, m_rows[i + m_top].m_text);
        line[m_rows[i + m_top].m_len] = 0;
        m_window.Text(line, 40, i, VIUICOLOR_0, 0);
    }
    unsigned short buf[512];
    if (m_sel == 0)
        VIString::snprintf16(buf, sizeof(buf), "|q%S|w", Word(ui, 1592));
    else
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 1592));
    m_window.Text(buf, m_sel == 0 ? 34 : 50, 17, (VIUICOLOR)(m_sel == 0), 0);
    if (m_sel == 1)
        VIString::snprintf16(buf, sizeof(buf), "|q%S|w", Word(ui, 1593));
    else
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 1593));
    m_window.Text(buf, m_sel == 1 ? 234 : 250, 17, (VIUICOLOR)(m_sel == 1), 0);
    if (m_asking)
        m_message.OnDrawSelf();
    ui->End2D();
}

// 0x00572E60.
void VIWndEula::OnKeyUp(int key)
{
    m_repeat = 0;
}

// 0x00572E68.  17/136 left, 18/137 right, 19/138 up, 20/139 down, 13/134
// Enter or X.
void VIWndEula::OnKeyDown(int key)
{
    VIUI *ui = UI();
    GetAppPtr();
    m_repeat = 0;
    switch (key)
    {
    case 17:
    case 136:
        if (m_asking)
            break;
        if (m_sel == 1)
        {
            UI()->SoundPlay((VIUISoundId)11);
            m_sel = 0;
            m_window.ResetTickPulse();
        }
        break;
    case 18:
    case 137:
        if (m_asking)
            break;
        if (m_sel == 0)
        {
            UI()->SoundPlay((VIUISoundId)11);
            m_sel = 1;
            m_window.ResetTickPulse();
        }
        break;
    case 19:
    case 138:
        if (m_asking)
            break;
        m_repeat = 10;
        m_repeatDir = -1;
        Up();
        break;
    case 20:
    case 139:
        if (m_asking)
            break;
        m_repeat = 10;
        m_repeatDir = 1;
        Down();
        break;
    case 13:
    case 134:
        if (!m_asking)
        {
            if (m_sel == 0)
            {
                unsigned short msg[2048];
                unsigned short a[512];
                unsigned short b[512];
                VIVect2I c(320, 224);
                VIString::snprintf16(a, sizeof(a), Word(ui, 1594),
                                     Word(UI(), VIGetExpansionFlags() & 2 ? 1803 : 1802));
                VIString::snprintf16(b, sizeof(b), Word(ui, 1595),
                                     Word(UI(), VIGetExpansionFlags() & 2 ? 1803 : 1802));
                VIString::snprintf16(msg, sizeof(msg), "%S\n\n%S", a, b);
                m_message.Setup(Word(ui, 785), msg, VIMESSAGEPROMPT_OK, c, 500);
                m_message.OnMessage(this, 295, (int)this, (int)&m_message);
                m_asking = 1;
                UI()->SoundPlay((VIUISoundId)15);
            }
            else
            {
                UI()->SoundPlay((VIUISoundId)15);
                SetFocus(0, true);
            }
        }
        else
        {
            m_asking = 0;
            UI()->SoundPlay((VIUISoundId)8);
        }
        break;
    }
}

// 0x005731C0.
void VIWndEula::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    GetAppPtr();
    if (msg == 295 && p2 == (int)this)
    {
        m_window.Close(true);
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 600, 18, 0);
        m_window.SetTitle(Word(UI(), 1596));
        m_window.Open(false);
        m_asking = 0;
        m_sel = 0;
    }
}

// 0x00573290.  Splits m_text into rows that fit the window, at a newline or
// else at the last space; a row with no space breaks one character early.
// Carriage returns become spaces.  Only the first 32 rows are cleared.
void VIWndEula::ComputeTextRows()
{
    VIUI *ui = UI();
    int i = 0;
    int space = 0;
    for (int k = 0; k < 32; k++)
    {
        m_rows[k].m_text = 0;
        m_rows[k].m_len = 0;
        m_rows[k].m_break = 0;
    }
    m_numRows = 0;
    unsigned short *start = m_text;
    while (m_text[i] != 0)
    {
        if (m_text[i] == 13)
            m_text[i] = ' ';
        if (m_text[i] == 10)
        {
            m_rows[m_numRows].m_text = start;
            m_rows[m_numRows].m_len = &m_text[i + 1] - start;
            m_rows[m_numRows].m_break = 1;
            i++;
            space = 0;
            start = &m_text[i];
            m_numRows++;
        }
        else
        {
            unsigned short line[256];
            if (m_text[i] == ' ')
                space = i;
            memcpy(line, start, (&m_text[i] - start) * sizeof(unsigned short));
            line[&m_text[i] - start] = 0;
            if (ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, line) > m_window.Width() - 100)
            {
                m_rows[m_numRows].m_text = start;
                if (space)
                {
                    m_rows[m_numRows].m_len = &m_text[space + 1] - start;
                    i = space + 1;
                    m_rows[m_numRows].m_break = 1;
                    start = &m_text[i];
                }
                else
                {
                    m_rows[m_numRows].m_len = &m_text[i] - start;
                    i--;
                    m_rows[m_numRows].m_break = 1;
                    start = &m_text[i];
                }
                m_numRows++;
                space = 0;
            }
            else
                i++;
        }
    }
    if (&m_text[i] - start)
    {
        m_rows[m_numRows].m_text = start;
        m_rows[m_numRows].m_len = &m_text[i] - start;
        m_numRows++;
    }
}

// 0x00573568.
void VIWndEula::Down()
{
    if (m_top < m_numRows - 17)
    {
        m_top++;
        UI()->SoundPlay((VIUISoundId)11);
    }
    else
        UI()->SoundPlay((VIUISoundId)9);
}

// 0x005735C0.
void VIWndEula::Up()
{
    if (m_top > 0)
    {
        m_top--;
        UI()->SoundPlay((VIUISoundId)11);
    }
    else
        UI()->SoundPlay((VIUISoundId)9);
}
