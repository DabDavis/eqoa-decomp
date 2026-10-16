// viwndedit.cpp (name ours) -- STATION 0x002B98D0 .. 0x002BC6A8.  VIWndEdit,
// the on-screen text editor every STATION form edits a field through.  CLIENT
// links the same code (see stedit.h).
#include "station.h"

// 0x00350088: the soft keyboard, 6 rows of 11.  A letter or a digit types
// itself; 1..16 are the special keys (OnKeyDown's second switch), 0 is no key.
// Cell 64 is the newline key, 6 while the editor is multi-line.
static char sKeys[66] =
{
    'A', 'B', 'C', 'D', 'E', '1', '2', '3', '+', '@', 5,
    'F', 'G', 'H', 'I', 'J', '4', '5', '6', 11, 12, '.',
    'K', 'L', 'M', 'N', 'O', '7', '8', '9', '*', 15, '?',
    'P', 'Q', 'R', 'S', 'T', 2, '0', '%', '/', ';', '!',
    'U', 'V', 'W', 'X', 'Y', 1, 3, 7, '=', ':', 5,
    'Z', 14, 16, 13, '(', ')', 9, 8, 10, 0, 0,
};
static float sKeyAlpha = 1.0f;   // 0x003500CC
static int   sPadOnly = 1;       // 0x003500D0  the pad drives the keyboard (1 once focused)
extern int gKeyboardAttached;      // SUPPORT

// 0x002B98D0.
VIWndEdit::VIWndEdit()
{
    m_text = 0;
    m_lines = 0;
    SetMaxLength(64);
    Clear();
    m_x24 = 0;
    m_x28 = 0;
    m_pos = VIVect2I(16, 40);
    m_x2C = 0;
    m_shift = 0;
    m_caps = 0;
    m_key = VIVect2I(0, 0);
    m_lastKey = VIVect2I(0, 0);
    m_x170 = 1;
    m_glide = 0;
    m_x168 = 0;
    m_pulse = 0;
    m_press = 0;
    m_pressed = VIVect2I(0, 0);
    m_x18C = 0;
    m_x190 = 0;
    m_cursor = 0;
    m_lastInput = 0;
    m_multiLine = 0;
    m_allowSpaces = 1;
    SetRows(2);
    SetMultiLine(0);
    m_title[0] = 0;
    m_x178 = 0;
}

// 0x002B9A28.
VIWndEdit::~VIWndEdit()
{
    if (m_text)
        delete[] m_text;
    if (m_lines)
        delete[] m_lines;
}

// 0x002B9A90.
void VIWndEdit::SetMaxLength(int n)
{
    m_maxLen = n;
    if (m_text)
        delete[] m_text;
    if (m_lines)
        delete[] m_lines;
    m_text = new unsigned short[n];
    m_lines = new VIEditLine[n];
    m_text[0] = 0;
    for (int i = 0; i < m_maxLen; i++)
    {
        m_lines[i].m_start = 0;
        m_lines[i].m_len = 0;
        m_lines[i].m_break = 0;
    }
    m_numLines = 0;
    m_x190 = 0;
    SetCursor(0);
}

// 0x002B9B68.
void VIWndEdit::SetRows(int n)
{
    m_rows = n;
}

// 0x002B9B70.
void VIWndEdit::SetMultiLine(int on)
{
    m_multiLine = on;
    sKeys[64] = on ? 6 : 0;
}

// 0x002B9B98.
int VIWndEdit::MultiLine()
{
    return m_multiLine;
}

// 0x002B9BA0.
void VIWndEdit::SetAllowSpaces(int on)
{
    m_allowSpaces = on;
}

// 0x002B9BA8.
int VIWndEdit::AllowSpaces()
{
    return m_allowSpaces;
}

// 0x002B9BB0.
void VIWndEdit::SetTitle(const char *s)
{
    m_title[0] = 0;
    VIString::str8to16(m_title, 128, s);
}

// 0x002B9BE0.
void VIWndEdit::SetTitle(const unsigned short *s)
{
    m_title[0] = 0;
    VIString::strcpy16(m_title, 128, s);
}

// 0x002B9C10.  The box: title bar, frame, the soft keyboard (or, with a USB
// keyboard and the compact option, a key legend), the wrapped text and a
// blinking caret.  Everything fades in with m_x168.
void VIWndEdit::OnDrawSelf()
{
    VIUI *ui = UI();
    long now = VITimer::GetTime();
    VIColor32F shadow(0.0f, 0.0f, 0.0f, m_x168);
    VIColor32F text(1.0f, 1.0f, 1.0f, m_x168);
    float dt = (float)(now - m_x178) * 0.001f;
    m_x178 = now;

    VIUIFrameParam fp[4];
    memset(fp, 0, sizeof(fp));

    fp[0].m_bottom.m_id = (VIUITexId)37;
    fp[0].m_bottom.m_flags = 0;
    fp[0].m_left.m_id = (VIUITexId)42;
    fp[0].m_bl.m_id = (VIUITexId)36;
    fp[0].m_br.m_id = (VIUITexId)36;
    fp[0].m_br.m_flags = 1;
    fp[0].m_right.m_id = (VIUITexId)42;
    fp[0].m_right.m_flags = 1;
    fp[0].m_top.m_id = (VIUITexId)37;
    fp[0].m_tl.m_id = (VIUITexId)36;
    fp[0].m_tr.m_id = (VIUITexId)36;
    fp[0].m_tr.m_flags = 1;

    fp[1].m_tl.m_id = (VIUITexId)44;
    fp[1].m_tl.m_flags = 0;
    fp[1].m_top.m_id = (VIUITexId)45;
    fp[1].m_top.m_flags = 0;
    fp[1].m_tr.m_id = (VIUITexId)46;
    fp[1].m_tr.m_flags = 0;
    fp[1].m_right.m_id = (VIUITexId)52;
    fp[1].m_right.m_flags = 0;
    fp[1].m_br.m_id = (VIUITexId)50;
    fp[1].m_br.m_flags = 0;
    fp[1].m_bottom.m_id = (VIUITexId)49;
    fp[1].m_bottom.m_flags = 0;
    fp[1].m_bl.m_id = (VIUITexId)48;
    fp[1].m_bl.m_flags = 0;
    fp[1].m_left.m_id = (VIUITexId)47;
    fp[1].m_left.m_flags = 0;

    fp[2].m_bottom.m_id = (VIUITexId)41;
    fp[2].m_left.m_id = (VIUITexId)43;
    fp[2].m_right.m_id = (VIUITexId)43;
    fp[2].m_right.m_flags = 1;
    fp[2].m_top.m_id = (VIUITexId)38;

    fp[3].m_bottom.m_id = (VIUITexId)102;
    fp[3].m_bottom.m_flags = 2;
    fp[3].m_bl.m_id = (VIUITexId)105;
    fp[3].m_br.m_id = (VIUITexId)106;
    fp[3].m_left.m_id = (VIUITexId)104;
    fp[3].m_right.m_id = (VIUITexId)104;
    fp[3].m_right.m_flags = 1;
    fp[3].m_top.m_id = (VIUITexId)102;
    fp[3].m_tl.m_id = (VIUITexId)101;
    fp[3].m_tr.m_id = (VIUITexId)103;

    // The fade in, the typed key's flash out, the Done key's pulse.
    m_x168 += dt * 5.0f;
    m_x168 = VIMIN(m_x168, sKeyAlpha);
    text.a = m_x168;
    shadow.a = m_x168;
    m_press -= dt * 5.0f;
    m_press = (m_press < 0.0f) ? 0.0f : m_press;
    if (m_x170 == -1)
    {
        m_pulse -= dt + dt;
        m_pulse = (m_pulse < 0.0f) ? 0.0f : m_pulse;
        if (m_pulse == 0.0f)
            m_x170 = 1;
    }
    else
    {
        m_pulse += dt * 3.0f;
        m_pulse = VIMIN(m_pulse, 1.0f);
        if (m_pulse == 1.0f)
            m_x170 = -1;
    }

    // A USB keyboard with the compact option shows a legend, not the keys.
    int padOnly = 0;
    if (gKeyboardAttached)
        padOnly = VIGetGameOptions()->m_x91 != 0;
    VIVect2I &where = m_pos;
    VIVect2I pos = where;
    sPadOnly = padOnly;
    VIVect2I size;
    size.x = 452;
    if (padOnly)
        size.y = m_rows * 17 + 42;
    else
        size.y = m_rows * 17 + 203;
    if (sPadOnly)
        size.y += 26;
    m_size = size;

    if (!sPadOnly)
    {
        // The keyboard's cursor glides to its key; a held direction repeats.
        if (m_key.x != m_lastKey.x || m_key.y != m_lastKey.y)
        {
            m_glide += dt * 15.0f;
            if (m_glide >= 1.0f)
            {
                m_lastKey = m_key;
                m_glide = 0;
            }
        }
        if (m_repeatKey < 140 && m_repeatKey >= 136)
        {
            m_repeatDelay -= dt;
            if (m_repeatDelay <= 0.0f)
            {
                m_repeatDelay = 0.05f;
                if (m_repeatKey == 138)
                    MoveKey(0, -1);
                else if (m_repeatKey == 139)
                    MoveKey(0, 1);
                else if (m_repeatKey == 136)
                    MoveKey(-1, 0);
                else if (m_repeatKey == 137)
                    MoveKey(1, 0);
            }
        }
    }

    ui->Begin2D();
    if (m_title[0])
    {
        int w = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, m_title) + 50;
        ui->BlitBar(m_title, text, VIVect2I(pos.x + (size.x >> 1) - (w >> 1), pos.y - 28),
                    (VIUITexId)33, (VIUITexId)34, w, (VIAlphaBlendMode)3, m_x168, 0, 1);
    }
    ui->BlitRect((VIUITexId)51, pos, size, (VIAlphaBlendMode)3, m_x168);
    ui->BlitFrame(pos, size, fp[0], (VIAlphaBlendMode)3, m_x168, 1);
    ui->BlitFillFrame(pos + VIVect2I(16, 16), VIVect2I(420, m_rows * 17 + 8), fp[3],
                      (VIUITexId)96, (VIAlphaBlendMode)3, m_x168, 1);

    unsigned short label[128];
    VIVect2I at;
    if (!sPadOnly)
    {
        // The legend under the box: Done, Cancel, Space, (Newline,) Back.
        pos.y += m_rows * 17 + 28;
        at = VIVect2I(pos.x + 320, pos.y + 4);
        pos.x += 325;
        ui->BlitEx((VIUITexId)193, at, (VIAlphaBlendMode)3, m_x168 * 0.75f, 0);
        VIString::snprintf16(label, 128, "= %S", ui->m_words.GetString(905, ui->m_language));
        ui->DrawStringEx(label, pos.x + 30, pos.y, text, 1);
        at = VIVect2I(pos.x - 5, pos.y + 31);
        pos.y += 27;
        ui->BlitEx((VIUITexId)194, at, (VIAlphaBlendMode)3, m_x168 * 0.75f, 0);
        VIString::snprintf16(label, 128, "= %S", ui->m_words.GetString(909, ui->m_language));
        ui->DrawStringEx(label, pos.x + 30, pos.y, text, 1);
        at = VIVect2I(pos.x + 3, pos.y + 32);
        pos.y += 27;
        ui->BlitEx((VIUITexId)71, at, (VIAlphaBlendMode)3, m_x168 * 0.75f, 0);
        VIString::snprintf16(label, 128, "= %S", ui->m_words.GetString(908, ui->m_language));
        ui->DrawStringEx(label, pos.x + 30, pos.y, text, 1);
        if (AllowSpaces())
        {
            at = VIVect2I(pos.x + 3, pos.y + 32);
            pos.y += 27;
            ui->BlitEx((VIUITexId)72, at, (VIAlphaBlendMode)3, m_x168 * 0.75f, 0);
            VIString::snprintf16(label, 128, "= %S", ui->m_words.GetString(907, ui->m_language));
            ui->DrawStringEx(label, pos.x + 30, pos.y, text, 1);
        }
        at = VIVect2I(pos.x + 3, pos.y + 32);
        pos.y += 27;
        ui->BlitEx((VIUITexId)69, at, (VIAlphaBlendMode)3, m_x168 * 0.75f, 0);
        VIString::snprintf16(label, 128, "= %S", ui->m_words.GetString(753, ui->m_language));
        ui->DrawStringEx(label, pos.x + 30, pos.y, text, 1);

        // The keys.
        VIVect2I cell;
        VIVect2I r;
        int top = where.y + m_rows * 17 + 28;
        pos.x = where.x + 16;
        pos.y = top;
        cell = pos;
        for (int row = 0; row < 6; row++)
        {
            for (int col = 0; col < 11; col++)
            {
                char *k = &sKeys[row * 11 + col];
                cell = VIVect2I(pos.x + col * 27, pos.y + row * 27);
                if (!*k)
                    continue;
                ui->BlitEx((VIUITexId)107, cell, (VIAlphaBlendMode)3, m_x168 * 0.75f, 0);
                if (*k == 5)
                    ui->BlitEx((VIUITexId)204, cell, (VIAlphaBlendMode)3,
                               m_pulse * m_x168 * 0.75f, 0);
                if (*k >= 32)
                {
                    char c[2];
                    c[0] = *k;
                    c[1] = 0;
                    if (c[0] >= 'A' && c[0] <= 'Z')
                    {
                        if ((!m_x2C && !m_shift && !m_caps)
                            || (m_caps == 1 && (m_x2C == 1 || m_shift == 1)))
                            c[0] += 32;
                    }
                    ui->m_font.SetTextColor(shadow);
                    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, c,
                                                  cell.x + 14, cell.y + 12, 0);
                    ui->m_font.SetTextColor(text);
                    ui->m_font.DrawCenteredString(ui->m_raster, ui->m_dict, c,
                                                  cell.x + 13, cell.y + 11, 0);
                }
                else
                {
                    VIVect2I off;
                    switch (*k)
                    {
                    case 1:
                        off = VIVect2I(4, 8);
                        ui->BlitEx((VIUITexId)154, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 2:
                        off = VIVect2I(4, 8);
                        ui->BlitEx((VIUITexId)155, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 3:
                        off = VIVect2I(7, 8);
                        ui->BlitEx((VIUITexId)112, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 5:
                        off = VIVect2I(4, 8);
                        ui->BlitEx((VIUITexId)156, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 6:
                        off = VIVect2I(7, 7);
                        ui->BlitEx((VIUITexId)111, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 7:
                        off = VIVect2I(7, 7);
                        ui->BlitEx((VIUITexId)226, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 8:
                        off = VIVect2I(7, 7);
                        ui->BlitEx((VIUITexId)226, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 2);
                        break;
                    case 9:
                        off = VIVect2I(7, 7);
                        ui->BlitEx((VIUITexId)113, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 10:
                        off = VIVect2I(7, 7);
                        ui->BlitEx((VIUITexId)113, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 1);
                        break;
                    case 11:
                        off = VIVect2I(9, 10);
                        ui->BlitEx((VIUITexId)169, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 12:
                        off = VIVect2I(7, 8);
                        ui->BlitEx((VIUITexId)170, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 13:
                        off = VIVect2I(10, 13);
                        ui->BlitEx((VIUITexId)171, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 14:
                        off = VIVect2I(7, 7);
                        ui->BlitEx((VIUITexId)172, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 15:
                        off = VIVect2I(8, 16);
                        ui->BlitEx((VIUITexId)114, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    case 16:
                        off = VIVect2I(10, 7);
                        ui->BlitEx((VIUITexId)171, cell + off, (VIAlphaBlendMode)3, m_x168 * 0.8f, 0);
                        break;
                    }
                }
                if (m_pressed.x == col && m_pressed.y == row)
                    ui->BlitEx((VIUITexId)108, cell, (VIAlphaBlendMode)3,
                               m_x168 * m_press * 0.6f, 0);
            }
        }

        // The cursor ring, eased from the last key to this one.
        VIVect2I from(m_lastKey.x * 27, m_lastKey.y * 27);
        VIVect2I to(m_key.x * 27, m_key.y * 27);
        float t = 1.0f - m_glide;
        t = 1.0f - t * t;
        VIVect2I base(where.x + 16, where.y + m_rows * 17 + 28);
        VIVect2I b = base + to;
        cell = base + from;
        r = VIVect2I(cell.x + (int)((float)(b.x - cell.x) * t) - 4,
                     cell.y + (int)((float)(b.y - cell.y) * t) + 5);
        ui->BlitEx((VIUITexId)89, r, (VIAlphaBlendMode)3, m_x168, 0);
    }
    else
    {
        pos.y = where.y + m_rows * 17 + 28;
        pos.x = where.x + 16;
        at = VIVect2I(where.x + 19, pos.y + 4);
        ui->BlitEx((VIUITexId)194, at, (VIAlphaBlendMode)3, m_x168 * 0.75f, 0);
        VIString::snprintf16(label, 128, "%S", ui->m_words.GetString(909, ui->m_language));
        ui->DrawStringEx(label, pos.x + 39, pos.y, text, 1);
        pos.y = where.y + m_rows * 17 + 28;
        pos.x = where.x + 121;
        at = VIVect2I(where.x + 124, pos.y + 5);
        ui->BlitEx((VIUITexId)69, at, (VIAlphaBlendMode)3, m_x168 * 0.75f, 0);
        VIString::snprintf16(label, 128, "%S", ui->m_words.GetString(753, ui->m_language));
        ui->DrawStringEx(label, pos.x + 23, pos.y, text, 1);
        if (m_multiLine)
        {
            pos.y = where.y + m_rows * 17 + 28;
            pos.x = where.x + 276;
            const char *a = ui->m_words.GetString(905, ui->m_language);
            const char *b = ui->m_words.GetString(906, ui->m_language);
            VIString::snprintf16(label, 128, "%S+%S = %S", a, b,
                                 ui->m_words.GetString(909, ui->m_language));
            ui->DrawStringEx(label, pos.x, pos.y, text, 1);
        }
    }

    // The text, scrolled to keep the caret's line in view.
    WrapLines();
    VIVect2I lc(0, 0);
    CursorLineCol(lc);
    if (lc.y < m_x190)
        m_x190 = lc.y;
    else if (lc.y >= m_x190 + m_rows)
        m_x190 = lc.y + 1 - m_rows;
    VIVect2I caret[2];
    int y = m_pos.y + 16;
    caret[0] = VIVect2I(m_pos.x + 22, y + 5);
    unsigned short buf[256];
    for (int i = m_x190; i < m_x190 + m_rows; i++)
    {
        if (i < m_numLines)
        {
            memcpy(buf, m_lines[i].m_start, m_lines[i].m_len * 2);
            buf[m_lines[i].m_len] = 0;
            for (int k = 0; k < m_lines[i].m_len; k++)
            {
                if (buf[k] == '\n')
                    buf[k] = ' ';
                else if (m_x18C == 1)
                    buf[k] = '*';
            }
            ui->m_font.SetTextColor(shadow);
            ui->m_font.DrawString(ui->m_raster, ui->m_dict, buf, m_pos.x + 23, y + 1);
            ui->m_font.SetTextColor(text);
            ui->m_font.DrawString(ui->m_raster, ui->m_dict, buf, m_pos.x + 22, y);
        }
        if (lc.y == i)
        {
            buf[lc.x] = 0;
            caret[0] = VIVect2I(m_pos.x + ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf) + 22,
                                y + 5);
        }
        y = y - 4 + ui->m_font.GetHeight(ui->m_raster, ui->m_dict);
    }
    if (!(((VITimer::GetTime() - m_lastInput) / 500) & 1))
    {
        caret[1].x = caret[0].x + 2;
        caret[1].y = caret[0].y + ui->m_font.GetHeight(ui->m_raster, ui->m_dict) - 5;
        ui->FillRectangle(caret, &text);
    }
    ui->End2D();
}

// 0x002BB240.  A space only where spaces are allowed, and one short of the
// buffer so the terminator always fits.
int VIWndEdit::InsertChar(unsigned short c)
{
    if (!AllowSpaces() && c == ' ')
        return 0;
    if (GetCursor() + 1 < m_maxLen)
    {
        m_lastInput = VITimer::GetTime();
        unsigned short *to = &m_text[GetCursor() + 1];
        unsigned short *from = &m_text[GetCursor()];
        memmove(to, from, (m_maxLen - GetCursor() - 1) * 2);
        m_text[m_maxLen - 1] = 0;
        m_text[GetCursor()] = c;
        SetCursor(GetCursor() + 1);
        return 1;
    }
    return 0;
}

// 0x002BB350.
void VIWndEdit::Clear()
{
    m_text[0] = 0;
    SetCursor(0);
}

// 0x002BB378.  The box centred on p, its height following the rows.
void VIWndEdit::SetCenter(const VIVect2I &p)
{
    m_pos.x = p.x - 226;
    m_pos.y = p.y - ((m_rows * 17 + 203) >> 1);
}

// 0x002BB3A8.
void VIWndEdit::SetText(const unsigned short *s)
{
    VIString::strcpy16(m_text, m_maxLen * 2, s);
    SetCursor(VIString::strlen16(m_text));
}

// 0x002BB3F0.
void VIWndEdit::SetText(const char *s)
{
    VIString::str8to16(m_text, m_maxLen * 2, s);
    SetCursor(VIString::strlen16(m_text));
}

// 0x002BB438.
int VIWndEdit::CursorRight()
{
    m_lastInput = VITimer::GetTime();
    return SetCursor(GetCursor() + 1);
}

// 0x002BB478.
int VIWndEdit::CursorLeft()
{
    m_lastInput = VITimer::GetTime();
    return SetCursor(GetCursor() - 1);
}

// 0x002BB4B8.
int VIWndEdit::Backspace()
{
    m_lastInput = VITimer::GetTime();
    if (GetCursor() > 0)
    {
        unsigned short *to = &m_text[GetCursor() - 1];
        unsigned short *from = &m_text[GetCursor()];
        memmove(to, from, (m_maxLen - GetCursor()) * 2);
        SetCursor(GetCursor() - 1);
        return 1;
    }
    return 0;
}

// 0x002BB568.
void VIWndEdit::PlaySound(int ok)
{
    UI()->SoundPlay((VIUISoundId)(ok ? 11 : 9));
}

// 0x002BB598.  A USB keyboard types letters and digits straight in; the rest
// is the switch.  The pad walks the soft keyboard and X types its key.
void VIWndEdit::OnKeyDown(int key)
{
    VIUI *ui = UI();
    m_repeatKey = 0;
    if (key >= 'A' && key <= 'Z')
        PlaySound(InsertChar(key));
    if (key >= 'a' && key <= 'z')
        PlaySound(InsertChar(key));
    if (key >= '0' && key <= '9')
        PlaySound(InsertChar(key));
    switch (key)
    {
    case 143:
        ui->SoundPlay((VIUISoundId)11);
        m_parent->OnMessage(this, 259, m_x24, m_x28);
        break;
    case 4:
        m_parent->OnMessage(this, 259, m_x24, m_x28);
        break;
    case ' ': case '!': case '"': case '%': case '\'': case '(': case ')':
    case '+': case ',': case '-': case '.': case '/': case ':': case ';':
    case '=': case '?': case '@': case '_': case '~':
        PlaySound(InsertChar(key));
        break;
    case 8:
        PlaySound(Backspace());
        break;
    case 27:
        ui->SoundPlay((VIUISoundId)8);
        m_parent->OnMessage(this, 260, 0, 0);
        break;
    case 13:
        if (MultiLine())
            PlaySound(InsertChar('\n'));
        else
            m_parent->OnMessage(this, 259, m_x24, m_x28);
        break;
    case 17:
        PlaySound(CursorLeft());
        break;
    case 18:
        PlaySound(CursorRight());
        break;
    case 19:
        PlaySound(LineUp());
        break;
    case 20:
        PlaySound(LineDown());
        break;
    case 142:
        if (sPadOnly)
            break;
        ui->SoundPlay((VIUISoundId)11);
        m_shift = 1;
        break;
    case 141:
        if (sPadOnly)
            break;
        if (MultiLine())
            PlaySound(InsertChar('\n'));
        break;
    case 146:
        VIWnd::OnKeyDown(146);
        break;
    case 136:
        if (sPadOnly)
            break;
        MoveKey(-1, 0);
        m_repeatKey = key;
        m_repeatDelay = 0.45f;
        break;
    case 137:
        if (sPadOnly)
            break;
        MoveKey(1, 0);
        m_repeatKey = key;
        m_repeatDelay = 0.45f;
        break;
    case 138:
        if (sPadOnly)
            break;
        MoveKey(0, -1);
        m_repeatKey = key;
        m_repeatDelay = 0.45f;
        break;
    case 139:
        if (sPadOnly)
            break;
        MoveKey(0, 1);
        m_repeatKey = key;
        m_repeatDelay = 0.45f;
        break;
    case 135:
        if (sPadOnly)
            break;
        PlaySound(InsertChar(' '));
        break;
    case 133:
        if (sPadOnly)
            break;
        if (!m_text[0])
        {
            ui->SoundPlay((VIUISoundId)9);
            break;
        }
        PlaySound(Backspace());
        break;
    case 132:
        ui->SoundPlay((VIUISoundId)8);
        m_parent->OnMessage(this, 260, 0, 0);
        break;
    case 134:
        if (sPadOnly == 1)
            break;
        {
            m_pressed = m_key;
            m_press = 1.0f;
            char c[2];
            c[1] = 0;
            c[0] = sKeys[m_key.x + m_key.y * 11];
            switch (c[0])
            {
            case 1:
                PlaySound(Backspace());
                break;
            case 2:
                PlaySound(InsertChar(' '));
                break;
            case 3:
                ui->SoundPlay((VIUISoundId)11);
                m_caps = 1 - m_caps;
                break;
            case 5:
                ui->SoundPlay((VIUISoundId)11);
                m_parent->OnMessage(this, 259, m_x24, m_x28);
                break;
            case 6:
                PlaySound(InsertChar('\n'));
                break;
            case 7:
                LineUp();
                break;
            case 8:
                LineDown();
                break;
            case 9:
                CursorLeft();
                break;
            case 10:
                CursorRight();
                break;
            case 11:
                PlaySound(InsertChar('-'));
                break;
            case 12:
                PlaySound(InsertChar('~'));
                break;
            case 13:
                PlaySound(InsertChar(','));
                break;
            case 14:
                PlaySound(InsertChar('"'));
                break;
            case 15:
                PlaySound(InsertChar('_'));
                break;
            case 16:
                PlaySound(InsertChar('\''));
                break;
            default:
                if (c[0] >= 'A' && c[0] <= 'Z')
                {
                    if ((!m_x2C && !m_shift && !m_caps)
                        || (m_caps == 1 && (m_x2C == 1 || m_shift == 1)))
                        c[0] += 32;
                }
                PlaySound(InsertChar(c[0]));
                break;
            }
        }
        break;
    }
}

// 0x002BBAC0.  Releasing the auto-repeating direction stops it; releasing R1
// drops the shift.
void VIWndEdit::OnKeyUp(int key)
{
    if (key == m_repeatKey)
        m_repeatKey = 0;
    if (sPadOnly == 1)
        return;
    switch (key)
    {
    case 136 ... 139:
        break;
    case 142:
        m_shift = 0;
    default:
        m_repeatKey = 0;
        break;
    }
}

// 0x002BBB10.  Taking the focus starts a fresh session on the keyboard.
void VIWndEdit::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (msg == 295)
    {
        if (p2 == (int)this)
        {
            m_x178 = VITimer::GetTime();
            m_x168 = 0;
            m_pulse = 0;
            m_x170 = 1;
            m_press = 0;
            m_pressed.Set(0, 0);
            m_caps = 0;
            m_repeatKey = 0;
            m_repeatDelay = 0;
            m_key.Set(0, 0);
            sKeys[64] = m_multiLine ? 6 : 0;
            m_x2C = 0;
            m_shift = 0;
            sPadOnly = 1;
        }
    }
    else
        VIWnd::OnMessage(w, msg, p1, p2);
}

// 0x002BBBB8.
int VIWndEdit::GetCursor()
{
    return m_cursor;
}

// 0x002BBBC0.  Clamped to the text; forgets the vertical column.
int VIWndEdit::SetCursor(int pos)
{
    int len = m_text ? VIString::strlen16(m_text) : 0;
    int c = pos < len ? pos : len;
    m_wantX = -1;
    if (c < 0)
        c = 0;
    int moved = c != m_cursor;
    m_cursor = c;
    return moved;
}

// 0x002BBC30.  Word wrap at 389 pixels.
void VIWndEdit::WrapLines()
{
    VIUI *ui = UI();
    int lastSpace = 0;
    int i = 0;
    int k;
    for (k = 0; k < m_maxLen; k++)
    {
        m_lines[k].m_start = 0;
        m_lines[k].m_len = 0;
        m_lines[k].m_break = 0;
    }
    unsigned short *start = m_text;
    m_numLines = 0;
    unsigned short buf[256];
    while (m_text[i])
    {
        if (m_text[i] == '\n')
        {
            m_lines[m_numLines].m_start = start;
            m_lines[m_numLines].m_len = &m_text[i + 1] - start;
            m_lines[m_numLines].m_break = 1;
            i++;
            lastSpace = 0;
            start = &m_text[i];
            m_numLines++;
        }
        else
        {
            if (m_text[i] == ' ')
                lastSpace = i;
            memcpy(buf, start, (&m_text[i] - start) * 2);
            buf[&m_text[i] - start] = 0;
            if (ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf) < 389)
                i++;
            else
            {
                m_lines[m_numLines].m_start = start;
                if (lastSpace)
                {
                    m_lines[m_numLines].m_len = &m_text[lastSpace + 1] - start;
                    i = lastSpace + 1;
                    m_lines[m_numLines].m_break = 1;
                    start = &m_text[i];
                }
                else
                {
                    m_lines[m_numLines].m_len = &m_text[i] - start;
                    m_lines[m_numLines].m_break = 1;
                    start = &m_text[i];
                }
                lastSpace = 0;
                m_numLines++;
            }
        }
    }
    if (&m_text[i] - start)
    {
        m_lines[m_numLines].m_start = start;
        m_lines[m_numLines].m_len = &m_text[i] - start;
        m_numLines++;
    }
}

// 0x002BBF08.
int VIWndEdit::CursorLineCol(VIVect2I &out)
{
    int start = 0;
    for (int i = 0; i < m_numLines; i++)
    {
        if (GetCursor() <= start + m_lines[i].m_len)
        {
            if (m_lines[i].m_break && GetCursor() - start == m_lines[i].m_len)
            {
                out.x = 0;
                out.y = i + 1;
            }
            else
            {
                out.x = GetCursor() - start;
                out.y = i;
            }
            return 0;
        }
        start += m_lines[i].m_len;
    }
    return -1;
}

// 0x002BBFF8.
int VIWndEdit::LineUp()
{
    VIUI *ui = UI();
    VIVect2I pos(0, 0);
    int i;
    m_lastInput = VITimer::GetTime();
    CursorLineCol(pos);
    if (!pos.y)
        return 0;
    if (!m_lines[pos.y].m_start)
    {
        m_cursor = 0;
        for (i = 0; i < pos.y - 1; i++)
            m_cursor += m_lines[i].m_len;
        return 1;
    }
    unsigned short buf[256];
    if (m_wantX == -1)
    {
        memcpy(buf, m_lines[pos.y].m_start, pos.x * 2);
        buf[pos.x] = 0;
        m_wantX = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf);
    }
    VIEditLine *line = &m_lines[pos.y - 1];
    for (i = 0; i < line->m_len; i++)
    {
        memcpy(buf, line->m_start, (i + 1) * 2);
        buf[i + 1] = 0;
        if (m_wantX < ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf))
        {
            m_cursor -= pos.x;
            m_cursor -= line->m_len - i;
            return 1;
        }
    }
    m_cursor--;
    m_cursor -= pos.x;
    return 1;
}

// 0x002BC1D8.
int VIWndEdit::LineDown()
{
    VIUI *ui = UI();
    VIVect2I pos(0, 0);
    m_lastInput = VITimer::GetTime();
    CursorLineCol(pos);
    if (!m_lines[pos.y + 1].m_start)
    {
        if (m_lines[pos.y].m_break)
        {
            m_cursor += m_lines[pos.y].m_len - pos.x;
            return 1;
        }
        return 0;
    }
    if (!m_lines[pos.y].m_start)
        return 0;
    unsigned short buf[256];
    if (m_wantX == -1)
    {
        memcpy(buf, m_lines[pos.y].m_start, pos.x * 2);
        buf[pos.x] = 0;
        m_wantX = ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf);
    }
    VIEditLine *line = &m_lines[pos.y + 1];
    for (int i = 0; i < line->m_len; i++)
    {
        memcpy(buf, line->m_start, (i + 1) * 2);
        buf[i + 1] = 0;
        if (m_wantX < ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf))
        {
            m_cursor += m_lines[pos.y].m_len - pos.x;
            m_cursor += i;
            return 1;
        }
    }
    m_cursor += m_lines[pos.y].m_len - pos.x;
    m_cursor += line->m_len - 1;
    return 1;
}

// 0x002BC3D0.  One step of the soft keyboard's cursor, wrapping at the edges
// and stepping over empty cells.
void VIWndEdit::MoveKey(int dx, int dy)
{
    VIUI *ui = UI();
    int d[2] = { dx, dy };
    ui->SoundPlay((VIUISoundId)11);
    for (int i = 0; i < 2; i++)
    {
        if (!d[i])
            continue;
        m_glide = 0;
        if (m_key.x != m_lastKey.x || m_key.y != m_lastKey.y)
            m_lastKey = m_key;
        m_key[i] += d[i];
        int edge, wrap;
        if (d[i] < 0)
        {
            edge = -1;
            wrap = i ? 5 : 10;
        }
        else
        {
            edge = i ? 6 : 11;
            wrap = 0;
        }
        if (m_key[i] == edge)
        {
            m_key[i] = wrap;
            m_lastKey = m_key;
        }
        while (!sKeys[m_key.x + m_key.y * 11])
        {
            m_key[i] += d[i];
            if (m_key[i] == edge)
            {
                m_key[i] = wrap;
                m_lastKey = m_key;
            }
        }
        if (i == 0 && m_key.x == 10)
            m_repeatDelay = 0.25f;
    }
}
