#line 1 "C:/code-frontiers-gm/game/VIWndMenu4.cpp"
// VIWndMenu4 (name the RTTI's) -- CLIENT 0x00C72D18 .. 0x00C75460.  The four-way quick
// menu over one VITextSet: tap a direction to run its line (a "|<n>" command goes to the
// parent as message 258, other text is said), hold it to edit the choice in a
// VIWndMenu4Edit, and a choice with a sub-set opens a deeper VIWndMenu4.  Method names ours.
#include "game.h"

extern "C" int sscanf(const char *, const char *, ...);

// 0x00C72D18.
VIWndMenu4::VIWndMenu4()
{
    m_fade = 1.0f;
    m_set = 0;
    m_sub = 0;
    m_alpha = 0;
    m_dir = -1;
    m_editor = 0;
    m_depth = 0;
    m_hold = -1;
}

// 0x00C72D80.
VIWndMenu4::~VIWndMenu4()
{
    if (m_editor)
        delete m_editor;
    if (m_sub)
    {
        RemoveChild(m_sub);
        delete m_sub;
        m_sub = 0;
    }
}

// 0x00C72E20.  The four bars and their key icons, the four choices, and the hold that
// opens the editor.
void VIWndMenu4::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    ui->Begin2D();
    ui->m_font.GetHeight(ui->m_raster, ui->m_dict);
    float a = m_alpha;
    if (m_fade > 0.0f)
    {
        a += 0.32f;
        a = a < 1.0f ? a : 1.0f;
        m_alpha = a;
    }
    else
    {
        a -= 0.105f;
        if (a < 0.3f)
            a = 0.3f;
        m_alpha = a;
    }
    VIVect2I p = m_pos + VIVect2I(80, 4);
    ui->BlitBar("", VIColor32F(0.0f, 0.0f, 0.0f, 0.0f), p, (VIUITexId)25, (VIUITexId)26, 162,
                (VIAlphaBlendMode)3, m_alpha, 0, 1);
    ui->BlitEx((VIUITexId)225, p + VIVect2I(75, -16), (VIAlphaBlendMode)3, m_alpha, 0);
    p = m_pos + VIVect2I(0, 29);
    ui->BlitBar("", VIColor32F(0.0f, 0.0f, 0.0f, 0.0f), p, (VIUITexId)25, (VIUITexId)26, 162,
                (VIAlphaBlendMode)3, m_alpha, 0, 1);
    ui->BlitEx((VIUITexId)224, p + VIVect2I(-16, 5), (VIAlphaBlendMode)3, m_alpha, 0);
    p = m_pos + VIVect2I(162, 29);
    ui->BlitBar("", VIColor32F(0.0f, 0.0f, 0.0f, 0.0f), p, (VIUITexId)25, (VIUITexId)26, 162,
                (VIAlphaBlendMode)3, m_alpha, 0, 1);
    ui->BlitEx((VIUITexId)224, p + VIVect2I(162, 5), (VIAlphaBlendMode)3, m_alpha, 1);
    p = m_pos + VIVect2I(80, 54);
    ui->BlitBar("", VIColor32F(0.0f, 0.0f, 0.0f, 0.0f), p, (VIUITexId)25, (VIUITexId)26, 162,
                (VIAlphaBlendMode)3, m_alpha, 0, 1);
    ui->BlitEx((VIUITexId)225, p + VIVect2I(75, 25), (VIAlphaBlendMode)3, m_alpha, 2);
    VIVect2I at[4];
    at[0] = m_pos + VIVect2I(160, 15);
    DrawChoice(0, at[0]);
    at[1] = m_pos + VIVect2I(80, 39);
    DrawChoice(1, at[1]);
    at[2] = m_pos + VIVect2I(243, 39);
    DrawChoice(2, at[2]);
    at[3] = m_pos + VIVect2I(160, 65);
    DrawChoice(3, at[3]);
    if (m_hold >= 0)
    {
        m_hold++;
        if (m_hold >= 16)
        {
            m_hold = -1;
            if ((unsigned int)m_dir < 4)
            {
                VITextPair *pair = &m_set->m_pairs[m_dir];
                m_editor = new VIWndMenu4Edit;
                if (!m_editor)
                    return;
                m_editor->SetAppPtr(GetAppPtr());
                m_editor->m_visible = 1;
                m_editor->SetPos(at[m_dir]);
                m_editor->SetTitle(pair->m_p00);
                m_editor->SetText(pair->m_p04);
                m_editor->SetHasSub(m_set->Child(m_dir) != 0);
                int depth = m_depth;
                m_editor->m_depth = depth;
                AddChild(m_editor);
                app->SetActiveWindow(m_editor);
                app->PlaySound(9);
            }
        }
    }
    ui->End2D();
}

// 0x00C734D8.
void VIWndMenu4::OnKeyDown(int key)
{
    GetAppPtr();
    m_hold = 0;
    switch (key)
    {
    case 19:
    case 138:
        m_dir = 0;
        break;
    case 17:
    case 136:
        m_dir = 1;
        break;
    case 18:
    case 137:
        m_dir = 2;
        break;
    case 20:
    case 139:
        m_dir = 3;
        break;
    case 146:
        VIWnd::OnKeyDown(146);
        return;
    }
    if (m_dir == -1)
        VIWnd::OnKeyDown(key);
}

// 0x00C735E8.  The tap: the pressed direction's line runs, and its sub-set opens.
void VIWndMenu4::OnKeyUp(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    int dir;
    unsigned short text[256];
    char cmd[256];
    m_hold = -1;
    switch (key)
    {
    case 19:
    case 138:
        dir = 0;
        break;
    case 17:
    case 136:
        dir = 1;
        break;
    case 18:
    case 137:
        dir = 2;
        break;
    case 20:
    case 139:
        dir = 3;
        break;
    case 146:
        return;
    }
    if (m_dir == -1)
    {
        VIWnd::OnKeyUp(key);
        return;
    }
    if (dir == m_dir)
    {
        if (m_set->m_pairs[dir].m_p04)
        {
            app->PlaySound(13);
            VIString::str16to8(cmd, sizeof(cmd), m_set->m_pairs[m_dir].m_p04);
            if (cmd[0] == '|')
            {
                int id;
                int arg = 0;
                int msg = -1;
                sscanf(cmd, "|%d", &id);
                switch (id)
                {
                case 0: msg = 0x06010000; break;
                case 1: msg = 0x06020000; break;
                case 2: msg = 0x06030000; break;
                case 3: msg = 0x06040000; break;
                case 4: msg = 0x07010000; break;
                case 5: msg = 0x07040000; break;
                case 6: msg = 0x07060000; break;
                case 7: msg = 0x0B053001; break;
                case 8: msg = 0x0B053002; break;
                case 9: msg = 0x0B053000; break;
                case 10: msg = 0x0B030003; arg = 0; break;
                case 11: msg = 0x0B030003; arg = 1; break;
                case 12: msg = 0x0B05100C; arg = 0; break;
                case 13: msg = 0x0B05100C; arg = 1; break;
                case 14: msg = 0x0B010000; break;
                case 15: msg = 0x0B030000; break;
                case 16: msg = 0x0B030001; break;
                case 17: msg = 0x0B030002; break;
                case 18: msg = 0x0B0D0001; break;
                case 19: msg = 0x0B0D0000; break;
                case 20: msg = 0x03010000; break;
                case 21: msg = 0x0B090000; break;
                case 22: msg = 0x0B0E0006; break;
                case 23: msg = 0x0B0E0003; break;
                case 24: msg = 0x050A0000; break;
                case 25: msg = 0x050B0000; break;
                case 26: msg = 0x050D0000; break;
                case 27: msg = 0x050C0000; break;
                case 28: msg = 0x050E0000; break;
                case 29: msg = 0x050F0000; break;
                case 30: msg = 0x050F1000; break;
                case 31: msg = 0x0B0E0007; break;
                case 32: msg = 0x0B0E0008; break;
                case 33: msg = 0x0B0E0009; break;
                }
                if (msg != -1)
                    GetParent()->OnMessage(this, 258, msg, arg);
            }
            else
            {
                if (app->m_text.Expand(text, 256, m_set->m_pairs[m_dir].m_p04, 0) >= 0)
                {
                    app->ChatLine(text);
                    GetParent()->OnMessage(this, 258, -1, 0);
                }
            }
        }
        if (m_set->Child(m_dir))
        {
            if (!m_sub)
            {
                m_sub = new VIWndMenu4;
                m_sub->m_depth = m_depth + 1;
                m_sub->SetAppPtr(GetAppPtr());
                m_sub->SetPos(m_pos + VIVect2I(16, 16));
                AddChild(m_sub);
                if (!m_sub)
                {
                    m_dir = -1;
                    return;
                }
            }
            app->PlaySound(11);
            app->SetActiveWindow(m_sub);
            m_sub->SetSet(m_set->Child(m_dir));
            m_sub->m_visible = 1;
        }
        m_dir = -1;
    }
    else
        VIWnd::OnKeyUp(key);
}

// 0x00C73A78.  The editor closing (259 with its new text, 260 without), and the focus.
void VIWndMenu4::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    m_hold = -1;
    switch (msg)
    {
    case 295:
        if (p2 == (int)this && p1 != (int)m_editor)
        {
            m_alpha = 0;
            m_fade = 1.0f;
            if (m_editor)
            {
                RemoveChild(m_editor);
                delete m_editor;
                m_editor = 0;
            }
        }
        if (p1 == (int)this && p2 != (int)m_editor)
        {
            m_alpha = 1.0f;
            m_fade = -1.0f;
        }
        break;
    case 259:
        if ((unsigned int)m_dir < 4)
        {
            VITextPair *pair = &m_set->m_pairs[m_dir];
            if (pair->m_p04)
                delete[] pair->m_p04;
            if (pair->m_p00)
                delete[] pair->m_p00;
            pair->m_p04 = 0;
            pair->m_p00 = 0;
            VIRec388 rec;
            if (m_editor->GetHasSub() && !m_set->Child(m_dir))
            {
                if (VIString::strlen16(m_editor->GetTitle()))
                {
                    pair->m_p00 = new unsigned short[VIString::strlen16(m_editor->GetTitle()) + 1];
                    VIString::strcpy16(pair->m_p00, (VIString::strlen16(m_editor->GetTitle()) + 1) * 2,
                                       m_editor->GetTitle());
                }
                m_set->GetChild(m_dir);
                FillRec(&rec, m_set, m_dir);
                app->Send006C(m_set->m_id, &rec);
                VIRec6A r;
                r.m_p00 = m_set->ChildId(m_dir);
                r.m_p04.Init(4);
                memset(&r.m_p04.m_data[0], 0, sizeof(VIRec388));
                memset(&r.m_p04.m_data[1], 0, sizeof(VIRec388));
                memset(&r.m_p04.m_data[2], 0, sizeof(VIRec388));
                memset(&r.m_p04.m_data[3], 0, sizeof(VIRec388));
                r.m_p04.m_data[0].m_p00 = 0;
                r.m_p04.m_data[1].m_p00 = 1;
                r.m_p04.m_data[2].m_p00 = 2;
                r.m_p04.m_data[3].m_p00 = 3;
                app->Send006A(&r);
            }
            else if (!m_editor->GetHasSub() && m_set->Child(m_dir))
            {
                if (VIString::strlen16(m_editor->GetTitle()))
                {
                    pair->m_p00 = new unsigned short[VIString::strlen16(m_editor->GetTitle()) + 1];
                    VIString::strcpy16(pair->m_p00, (VIString::strlen16(m_editor->GetTitle()) + 1) * 2,
                                       m_editor->GetTitle());
                }
                if (VIString::strlen16(m_editor->GetText()))
                {
                    pair->m_p04 = new unsigned short[VIString::strlen16(m_editor->GetText()) + 1];
                    VIString::strcpy16(pair->m_p04, (VIString::strlen16(m_editor->GetText()) + 1) * 2,
                                       m_editor->GetText());
                }
                FillRec(&rec, m_set, m_dir);
                app->Send006C(m_set->m_id, &rec);
                m_set->Child(m_dir)->Mark(1);
                for (int i = 0; i < 256; i++)
                {
                    VITextSet *s = VITextSet::GetTable()[i];
                    if (s && s->m_p01 == 2)
                    {
                        s->Remove();
                        app->Send006B(i);
                    }
                }
            }
            else
            {
                if (VIString::strlen16(m_editor->GetTitle()))
                {
                    pair->m_p00 = new unsigned short[VIString::strlen16(m_editor->GetTitle()) + 1];
                    VIString::strcpy16(pair->m_p00, (VIString::strlen16(m_editor->GetTitle()) + 1) * 2,
                                       m_editor->GetTitle());
                }
                if (VIString::strlen16(m_editor->GetText()))
                {
                    pair->m_p04 = new unsigned short[VIString::strlen16(m_editor->GetText()) + 1];
                    VIString::strcpy16(pair->m_p04, (VIString::strlen16(m_editor->GetText()) + 1) * 2,
                                       m_editor->GetText());
                }
                FillRec(&rec, m_set, m_dir);
                app->Send006C(m_set->m_id, &rec);
            }
        }
    case 260:
        app->SetActiveWindow(this);
        RemoveChild(m_editor);
        delete m_editor;
        m_editor = 0;
        m_dir = -1;
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C74050.
void VIWndMenu4::SetSet(VITextSet *set)
{
    m_set = set;
    if (m_sub)
        m_sub->m_visible = 0;
}

// 0x00C74068.  One choice: its title in a one-row box (lit when it is the pressed one),
// the "more" mark when it has a sub-set, the "empty" mark when it has nothing.
void VIWndMenu4::DrawChoice(int i, const VIVect2I &at)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    unsigned short buf[256];
    VITextPair *pair = &m_set->m_pairs[i];
    if (Expand(pair->m_p00, buf, 256) >= 0)
    {
        VIUIWindow w;
        int y = at.y;
        w.Init(ui, VIVect2I(0, 0),
               VIVect2I(at.x, (unsigned int)(i - 1) < 2 ? y + 3 : y + 2), 128, 1, 0);
        w.Open(true);
        int width = w.Width();
        w.TextBox(buf, 0, 0, width, 1, m_dir == i ? VIColor32F(10.0f / 255.0f, 50.0f / 255.0f, 70.0f / 255.0f, m_alpha)
                                          : VIColor32F(0.0f, 0.0f, 0.0f, m_alpha), 10);
    }
    if (m_set->Child(i))
    {
        VIVect2I p = at + VIVect2I(68, 4);
        ui->BlitEx((VIUITexId)123, p, (VIAlphaBlendMode)3, m_alpha, 0);
    }
    if (!pair->m_p00 && !pair->m_p04 && !m_set->Child(i))
    {
        VIVect2I p = at + VIVect2I(-5, -6);
        ui->BlitEx((VIUITexId)124, p, (VIAlphaBlendMode)3, m_alpha, 0);
    }
}

// 0x00C742D8.  A choice's title with its %W<n> escapes expanded to WORDS.TXT entries.
int VIWndMenu4::Expand(const unsigned short *src, unsigned short *dst, unsigned short size)
{
    VIUI *ui = &((VIClient *)GetAppPtr())->m_ui;
    int full = 0;
    unsigned short *p = dst;
    if (!src)
        return -1;
    while (*src)
    {
        unsigned short c = *src++;
        if (c == '%')
        {
            int type;
            int num;
            src = ParseEscape(src, &type, &num);
            if (type == 'W')
            {
                const unsigned short *w =
                    (const unsigned short *)ui->m_words.GetString(num, ui->m_language);
                if (w)
                {
                    if ((unsigned int)((p - dst) + VIString::strlen16(w)) < size)
                    {
                        VIString::strcpy16(p, size - (p - dst), w);
                        p += VIString::strlen16(w);
                    }
                    else
                        full = 1;
                }
            }
        }
        else if ((p - dst) + 1 < size)
            *p++ = c;
        else
            full = 1;
        if (full)
            break;
    }
    *p = 0;
    return 0;
}

// 0x00C74440.  One escape after a '%': a letter (C, E, P, T or W, either case), digits,
// and the closing '%'.
const unsigned short *VIWndMenu4::ParseEscape(const unsigned short *s, int *type, int *num)
{
    const unsigned short *p = s;
    *type = -1;
    *num = -1;
    int n = 0;
    if (!*p)
        return p;
    int c;
    switch (*p)
    {
    case 'C':
    case 'c':
        c = 'C';
        break;
    case 'E':
    case 'e':
        c = 'E';
        break;
    case 'W':
    case 'w':
        c = 'W';
        break;
    case 'P':
    case 'p':
        c = 'P';
        break;
    case 'T':
    case 't':
        c = 'T';
        break;
    default:
        do
        {
            if (*p == '%')
                return p + 1;
            p++;
        } while (*p);
        return p;
    }
    p++;
    if (!*p || *p == '%')
        return p;
    while (*p)
    {
        if (*p == '%')
        {
            p++;
            break;
        }
        if ((unsigned int)(*p - '0') < 10)
            n = n * 10 + (*p - '0');
        else
        {
            do
            {
                if (*p == '%')
                    return p + 1;
                p++;
            } while (*p);
            return p;
        }
        p++;
    }
    *type = c;
    *num = n;
    return p;
}

// 0x00C745A0.
void VIWndMenu4::FillRec(VIRec388 *out, VITextSet *set, int i)
{
    VITextPair *pair = &set->m_pairs[i];
    out->m_p00 = m_dir;
    if (pair->m_p00)
        VIString::strcpy16(out->m_p104, sizeof(out->m_p104), pair->m_p00);
    else
        out->m_p104[0] = 0;
    if (pair->m_p04)
        VIString::strcpy16(out->m_p04, sizeof(out->m_p04), pair->m_p04);
    else
        out->m_p04[0] = 0;
}
