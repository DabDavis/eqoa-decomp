#line 1 "C:/code-frontiers-gm/game/VIWndCharCreateName.cpp"
// VIWndCharCreateName -- CLIENT 0x00C29968 .. 0x00C2B058: the name step of character
// creation (cl_wc.h).
#include "game.h"
VI_HOST_ONLY(extern "C" unsigned int strlen(const char *);)  // gcc 2.95: implicit
// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// A word centred on (x, y) in the font's colour.  Inline: its x and y are evaluated
// before the word is looked up.
static inline void DrawWord(VIUI *ui, VIFont *font, int id, int x, int y)
{
    font->DrawCenteredString(ui->m_raster, ui->m_dict, Word(ui, id), x, y, 1);
}

// 0x00C29968.
VIWndCharCreateName::VIWndCharCreateName()
{
    m_char = 0;
    m_edit = 0;
    m_mbox = 0;
    m_name[0] = 0;
}

// 0x00C299B0.
VIWndCharCreateName::~VIWndCharCreateName()
{
    Free();
}

// 0x00C299F8.  The edit box sits at (90, 75), the message box at (80, 115).
int VIWndCharCreateName::Init()
{
    int r;
    m_edit = new VIWndEdit;
    if (!m_edit)
        r = -1;
    else
    {
        m_edit->SetVisible(true);
        m_edit->SetPos(VIVect2I(90, 75));
        m_edit->SetAppPtr(GetAppPtr());
        m_edit->SetAllowSpaces(0);
        AddChild(m_edit);
        m_mbox = new VIWndMBox;
        if (!m_mbox)
            r = -1;
        else
        {
            m_mbox->SetVisible(false);
            m_mbox->SetPos(VIVect2I(80, 115));
            m_mbox->SetAppPtr(GetAppPtr());
            r = m_mbox->Init(0, 1);
            if (!r)
                AddChild(m_mbox);
        }
    }
    return r;
}

// 0x00C29B10.
int VIWndCharCreateName::Free()
{
    if (m_edit)
    {
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    m_char = 0;
    return 0;
}

// 0x00C29BA8.
int VIWndCharCreateName::Reset()
{
    m_name[0] = 0;
    m_edit->Clear();
    m_mbox->SetVisible(false);
    return 0;
}

// 0x00C29BE0.  The backdrop, then a legend bar under the edit box: three pad buttons
// (68, 66, 65) each followed by its word (548, 212, 217).
void VIWndCharCreateName::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    ui->Begin2D();
    VIFont *font = &app->m_ui.m_font;
    VIVect2I pos;
    VIVect2I size;
    VIColor32F white(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F black(0.0f, 0.0f, 0.0f, 1.0f);
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 450), (VIAlphaBlendMode)0, 1.0f);
    font->SetTextColor(black);
    int w = font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 548)) + 155
          + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 212))
          + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 217));
    pos.Set(((m_edit->GetPos().x + m_edit->GetEnd().x) >> 1) - (w >> 1), m_edit->GetEnd().y);
    ui->BlitBar("", black, pos, (VIUITexId)25, (VIUITexId)26, w, (VIAlphaBlendMode)0, 1.0f, 0, 1);
    pos += VIVect2I(20, 4);
    ui->Blit((VIUITexId)68, pos);
    pos.x += 25;
    DrawWord(ui, font, 548, pos.x, pos.y + 5);
    pos.x += 20 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 548));
    ui->Blit((VIUITexId)66, pos);
    pos.x += 25;
    DrawWord(ui, font, 212, pos.x, pos.y + 5);
    pos.x += 20 + font->GetStringWidth(ui->m_raster, ui->m_dict, Word(ui, 212));
    ui->Blit((VIUITexId)65, pos);
    pos.x += 25;
    DrawWord(ui, font, 217, pos.x, pos.y + 5);
    font->SetTextColor(white);
    ui->End2D();
}

// 0x00C2A000.  135 asks for a random name.
void VIWndCharCreateName::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (key == 135)
    {
        if (m_char)
        {
            app->Send0012(m_char->m_race, m_char->m_sex);
            app->PlaySound(11);
        }
    }
    else if (key != 146)
        m_edit->OnKeyDown(key);
}

// 0x00C2A098.
void VIWndCharCreateName::OnKeyUp(int key)
{
    GetAppPtr();
    m_edit->OnKeyUp(key);
}

// 0x00C2A0E8.
void VIWndCharCreateName::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 259:
        VIString::str16to8(m_name, sizeof(m_name), m_edit->GetText());
        if (Validate(m_name) >= 0)
        {
            m_edit->Clear();
            GetParent()->OnMessage(this, 266, (int)m_name, 0);
        }
        else
        {
            m_edit->SetVisible(false);
            m_mbox->SetVisible(true);
            app->SetActiveWindow(m_mbox);
        }
        break;
    case 260:
        if (w == m_edit)
        {
            m_edit->Clear();
            GetParent()->OnMessage(this, 260, 0, 0);
        }
        else if (w == m_mbox)
        {
            m_edit->SetVisible(true);
            m_mbox->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    case 268:
        m_edit->SetVisible(true);
        m_mbox->SetVisible(false);
        app->SetActiveWindow(this);
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C2A288.  Four letters or more, A-Z only; the name is then capitalised.
int VIWndCharCreateName::Validate(char *name)
{
    GetAppPtr();
    char *p = name;
    if (strlen(name) < 4)
    {
        m_mbox->SetText((unsigned short)875);
        return -1;
    }
    while (*p)
    {
        if ((*p < 'a' || *p > 'z') && (*p < 'A' || *p > 'Z'))
        {
            m_mbox->SetText((unsigned short)876);
            return -1;
        }
        p++;
    }
    CapitalizeName(name);
    return 0;
}

// 0x00C2A340.  Opens the edit box on the form's name, if it has one.
void VIWndCharCreateName::Activate()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = &app->m_ui;
    m_edit->SetMaxLength(13);
    m_edit->SetVisible(true);
    m_edit->SetTitle(Word(ui, 260));
    m_mbox->SetVisible(false);
    app->SetActiveWindow(m_edit);
    app->SetActiveWindow(this);
    char *name = ((VIWndCharCreateBlank *)GetParent())->m_char.m_name;
    if (strlen(name))
        m_edit->SetText(name);
    else
        m_edit->Clear();
}

inline void VIWndCharCreateName::SetText(const char *s)
{
    m_edit->SetText(s);
}
