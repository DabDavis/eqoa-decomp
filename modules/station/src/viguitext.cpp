// viguitext.cpp (name ours) -- STATION 0x002C73D0 .. 0x002C8B00: a page element
// that shows a string model's text and edits it in the renderer's VIWndEdit.
// Then the item-header template copies, the implicit destructor, __tf9VIGUIText
// and the inline members.
#include "station.h"

// 0x002C73D0.
VIGUIText::VIGUIText(const char *name, VIWndGenericRenderer *r, VIXMLParser *reader)
    : VIGUIObject(name, 1, r, reader)
{
    m_model = new VIGUIStringDataModel;
    SetTextColor(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));
    SetRollColor(VIColor32F(0.0f, 0.0f, 200.0f, 1.0f));
    m_center = 0;
    m_wrap = 0;
    m_password = 0;
    m_maxChars = -1;
    m_shadow = 1;
}

// 0x002C74C8.  The subclasses' constructor.
VIGUIText::VIGUIText(const char *name, int type, VIWndGenericRenderer *r, VIXMLParser *reader)
    : VIGUIObject(name, type, r, reader)
{
    m_model = new VIGUIStringDataModel;
    SetTextColor(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));
    SetRollColor(VIColor32F(0.0f, 0.0f, 200.0f, 1.0f));
    m_center = 0;
    m_wrap = 0;
    m_password = 0;
    m_maxChars = -1;
    m_shadow = 1;
}

// 0x002C75B0.
VIGUIStringDataModel *VIGUIText::GetModel()
{
    return (VIGUIStringDataModel *)m_model;
}

// 0x002C75B8.
void VIGUIText::SetEditLabel(const char *s)
{
    if (s)
        m_label.SetString(s);
}

// 0x002C75D8.
void VIGUIText::SetMaxChars(int n)
{
    if (n >= -1)
        m_maxChars = n;
}

// 0x002C75F0.  A malloc'd copy of `s`, cut from the end until it is no wider
// than `width` (a negative width keeps all of it).  The store is indexed, not a
// pointer: loop strength-reduces it, and builds the start as `len + buf`.
char *VIGUIText::Fit(const char *s, int width)
{
    if (!s)
        return 0;
    if (width < 0)
        return strdup(s);
    if (!m_renderer)
        return 0;
    void *app = m_renderer->GetAppPtr();
    if (!app)
        return 0;
    VIUI *ui = ST_UI(app);
    int len = strlen(s);
    char *buf = (char *)malloc(len + 1);
    strcpy(buf, s);
    int i = 0;
    while (width < ui->m_font.GetStringWidth(ui->m_raster, ui->m_dict, buf) && i < len)
    {
        buf[len - i] = 0;
        i++;
    }
    return buf;
}

// Draw's wide-string calls go through these.  Inlined, a parameter bound to a
// frame address is substituted straight into the call's argument move, so each
// call gets its own `addiu a3,sp,16` that gcse never hashes; called directly,
// the address goes through a pseudo that gcse PRE replaces with the register
// it hoisted for str8to16 and the closing white temp.
static inline int WideWidth(VIFont *font, VIUI *ui, const unsigned short *s)
{
    return font->GetStringWidth(ui->m_raster, ui->m_dict, s);
}

static inline void DrawWide(VIFont *font, VIUI *ui, const unsigned short *s, int x, int y)
{
    font->DrawString(ui->m_raster, ui->m_dict, s, x, y);
}

// 0x002C76E8.  The text in the text or the roll colour, centred if asked,
// starred if a password, with a two-pixel shadow; or word-wrapped through a
// VIUIWindow.  A string of 1024 or more is converted into a heap buffer that is
// then never drawn: the stack one is.
void VIGUIText::Draw(float alpha)
{
    if (!m_renderer)
        return;
    void *app = m_renderer->GetAppPtr();
    if (!app)
        return;
    VIUI *ui = ST_UI(app);
    VIGUIStringDataModel *model = GetModel();
    if (!model)
        return;
    const char *text = model->GetString();
    if (!text)
        return;
    VIGUIObject::Draw(alpha);
    VIFont *font = &ui->m_font;
    int x = m_x;
    int y = m_y;
    if (m_center == 1)
    {
        int w = font->GetStringWidth(ui->m_raster, ui->m_dict, text);
        int h = font->GetHeight(ui->m_raster, ui->m_dict);
        x = m_x + m_width / 2 - w / 2;
        y = m_y + m_height / 2 - h / 2;
    }
    VIColor32F c = m_selected ? m_rollColor : m_textColor;
    c.a = 1.0f;
    font->SetTextColor(c);
    if (!m_wrap)
    {
        char *s = Fit(text, m_width - 10);
        if (s)
        {
            if (m_password)
                memset(s, '*', strlen(s));
            unsigned short wbuf[1024];
            unsigned short *wide = 0;
            int n = strlen(s);
            if (n >= 1024)
            {
                n++;
                wide = new unsigned short[n];
                VIString::str8to16(wide, n, s);
            }
            else
                VIString::str8to16(wbuf, 2048, s);
            if (x < 0)
                x = -x - WideWidth(font, ui, wbuf);
            if (m_shadow)
            {
                font->SetTextColor(VIColor32F(0.0f, 0.0f, 0.0f, 0.5f));
                DrawWide(font, ui, wbuf, x + 1, y + 1);
                DrawWide(font, ui, wbuf, x + 2, y + 2);
            }
            font->SetTextColor(c);
            DrawWide(font, ui, wbuf, x, y);
            free(s);
            if (wide)
                delete[] wide;
        }
    }
    else
    {
        VIUIWindow win;
        int cx = m_x + m_width / 2;
        int cy = m_y + m_height / 2;
        int rows = m_height / font->GetHeight(ui->m_raster, ui->m_dict);
        win.Init(ui, VIVect2I(0, 0), VIVect2I(cx, cy), m_width, rows, 0);
        win.TextBox(win.T(text), 0, 0, m_width, rows, c, 0);
    }
    font->SetTextColor(VIColor32F(1.0f, 1.0f, 1.0f, 1.0f));
}

// 0x002C7B00.  Enter or the pad's accept opens the renderer's editor on the
// text and hands it the focus.
int VIGUIText::OnKeyDown(int key)
{
    if (!m_renderer)
        return 0;
    void *app = m_renderer->GetAppPtr();
    if (!app)
        return 0;
    if (key == 13 || key == 134)
    {
        VIWndEdit *edit = RENDERER_EDIT(m_renderer);
        if (!edit)
            return 0;
        VIGUIStringDataModel *model = GetModel();
        if (!model)
            return 0;
        const char *s = model->GetString();
        if (m_password)
            EDIT_PASSWORD(edit) = 1;
        else
            EDIT_PASSWORD(edit) = 0;
        if (m_maxChars > 0)
            edit->SetMaxLength(m_maxChars);
        else
            edit->SetMaxLength(128);
        const char *label = m_label.GetString();
        if (label && label[0])
            edit->SetTitle(label);
        else
            edit->SetTitle("");
        if (s)
            edit->SetText(s);
        ST_SetActiveWindow(app, edit);
        edit->SetVisible(true);
    }
    return 0;
}

// 0x002C7C50.
int VIGUIText::OnKeyUp(int key)
{
    return 0;
}

// 0x002C7C58.
void VIGUIText::SetShadow(int on)
{
    m_shadow = on;
}

// 0x002C7C60.  The editor's answer: 259 takes its text, 260 (from the editor
// itself) cancels.  Either way the focus goes back to the renderer.  The cases
// BREAK to one `return 1`: with a return in each, the `li v0,1` shares the
// last case's block and outranks the focus store in sched2.
int VIGUIText::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    if (!m_renderer)
        return 0;
    void *app = m_renderer->GetAppPtr();
    if (!app)
        return 0;
    VIWndEdit *edit = RENDERER_EDIT(m_renderer);
    if (!edit)
        return 0;
    VIGUIStringDataModel *model = GetModel();
    if (!model)
        return 0;
    switch (msg)
    {
    case 260:
        if (w == edit && edit)
        {
            edit->SetVisible(false);
            ST_SetActiveWindow(app, m_renderer);
        }
        break;
    case 259:
    {
        edit->SetVisible(false);
        char buf[1024];
        VIString::str16to8(buf, 1024, EDIT_TEXT(edit));
        model->SetString(buf);
        ST_SetActiveWindow(app, m_renderer);
        break;
    }
    }
    return 1;
}

// 0x002C7D88.
void VIGUIText::SetTextColor(VIColor32F c)
{
    m_textColor = c;
}

// 0x002C7DD8.
void VIGUIText::SetRollColor(VIColor32F c)
{
    m_rollColor = c;
}

// 0x002C7E28.
VIColor32F VIGUIText::GetTextColor()
{
    return m_textColor;
}

// 0x002C7E50.
VIColor32F VIGUIText::GetRollColor()
{
    return m_rollColor;
}

// 0x002C7E78.
void VIGUIText::SetCenter(int a)
{
    m_center = a;
}

// 0x002C7E80.
void VIGUIText::SetWrap(int on)
{
    m_wrap = on;
}
