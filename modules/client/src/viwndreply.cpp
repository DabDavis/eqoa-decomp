#line 1 "C:/code-frontiers-gm/game/VIWndReply.cpp"
// VIWndReply (name RTTI's) -- CLIENT 0x00C8E360 .. 0x00C8F950: the tell reply box.
// To and the text are each typed in a VIWndEdit; Send hands VIClient's chat input the
// line "/t <to> <text>".  The VIMap<int, int> and VIArray<VIItemStat> copies between
// TakeText and __tf are template instantiations this TU does not write.
#include "game.h"

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C8E360.
VIWndReply::VIWndReply()
{
    m_edit = 0;
}

// 0x00C8E3A0.
VIWndReply::~VIWndReply()
{
    Free();
}

// 0x00C8E3F8.  The editor, hidden.
int VIWndReply::Init()
{
    int ret;
    Free();
    m_edit = new VIWndEdit;
    if (m_edit)
    {
        m_edit->SetVisible(false);
        m_edit->SetAppPtr(GetAppPtr());
        AddChild(m_edit);
        ret = 0;
    }
    else
        ret = -1;
    return ret;
}

// 0x00C8E468.
void VIWndReply::Free()
{
    if (m_edit)
    {
        RemoveChild(m_edit);
        delete m_edit;
        m_edit = 0;
    }
    m_sel = 0;
    m_to[0] = 0;
    m_text[0] = 0;
}

// 0x00C8E4D0.
void VIWndReply::OnDrawSelf()
{
    VIUI *ui = UI();
    VIClient *app = (VIClient *)GetAppPtr();
    ui->Begin2D();
    m_window.Draw();
    unsigned short buf[128];
    VIString::snprintf16(buf, sizeof(buf), "%S:", Word(ui, 881));
    m_window.Text(buf, 40, 1, (VIUICOLOR)(m_sel == 0), 0);
    m_window.TextBox(m_window.T(m_to), 140, 1, m_window.Width() - 180, 1, (VIUICOLOR)0, 6);
    VIString::snprintf16(buf, sizeof(buf), "%S:", Word(ui, 522));
    m_window.Text(buf, 40, 3, (VIUICOLOR)(m_sel == 1), 0);
    m_window.TextBox(m_window.T(m_text), 140, 3, m_window.Width() - 180, 1, (VIUICOLOR)0, 6);
    m_window.Text(m_window.T(885), 40, 5, (VIUICOLOR)(m_sel == 2), 0);
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 212), Word(ui, 161));
    m_window.Text(buf, 40, 7, (VIUICOLOR)0, 0);
    m_window.TickR(36, m_sel * 2 + 1, true);
    ui->End2D();
}

// 0x00C8E6E0.
void VIWndReply::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    app->GetP2EFD0();
    switch (key)
    {
    case 19:
    case 138:
        Prev();
        break;
    case 20:
    case 139:
        Next();
        break;
    case 13:
    case 134:
        Select();
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        app->ClosePanel6F8();
        break;
    case 146:
        VIWnd::OnKeyDown(146);
        break;
    }
}

// 0x00C8E818.  295: the focus moved (p1 from, p2 to); 259: the editor is done; 260: it
// was cancelled.
void VIWndReply::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != m_edit)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 500, 8, 0);
            m_window.SetTitle(m_window.T(534));
            m_window.Open(true);
            m_text[0] = 0;
            m_edit->SetVisible(false);
        }
        break;
    case 259:
        if (m_edit)
            TakeText();
    case 260:
        if (m_edit)
        {
            app->SetActiveWindow(this);
            m_edit->SetVisible(false);
        }
        break;
    }
}

// 0x00C8E950.
const unsigned short *VIWndReply::To()
{
    return m_to;
}

// 0x00C8E958.
void VIWndReply::SetTo(const unsigned short *s)
{
    VIString::strcpy16(m_to, sizeof(m_to), s);
}

// 0x00C8E980.
const unsigned short *VIWndReply::Text()
{
    return m_text;
}

// 0x00C8E988.
void VIWndReply::SetText(const unsigned short *s)
{
    VIString::strcpy16(m_text, sizeof(m_text), s);
}

// 0x00C8E9B0.  A reply to someone starts on the text.
void VIWndReply::Start(const unsigned short *to)
{
    m_to[0] = 0;
    if (to && *to)
    {
        VIString::strcpy16(m_to, sizeof(m_to), to);
        m_sel = 1;
    }
    else
        m_sel = 0;
}

// 0x00C8EA00.
void VIWndReply::Stub8EA00()
{
}

// 0x00C8EA08.
void VIWndReply::Next()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel < 2)
    {
        m_sel++;
        m_window.ResetTickPulse();
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}

// 0x00C8EA78.
void VIWndReply::Prev()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel > 0)
    {
        m_sel--;
        m_window.ResetTickPulse();
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}

// 0x00C8EAE0.  X: edit To, edit the text, or send.
void VIWndReply::Select()
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_sel)
    {
    case 0:
        m_edit->SetRows(1);
        m_edit->SetMultiLine(0);
        m_edit->SetCenter(m_window.Center());
        m_edit->SetMaxLength(24);
        m_edit->SetText(m_to);
        m_edit->SetVisible(true);
        app->SetActiveWindow(m_edit);
        break;
    case 1:
        m_edit->SetRows(2);
        m_edit->SetMultiLine(0);
        m_edit->SetCenter(m_window.Center());
        m_edit->SetMaxLength(64);
        m_edit->SetText(m_text);
        m_edit->SetVisible(true);
        app->SetActiveWindow(m_edit);
        break;
    case 2:
    {
        unsigned short sep[2] = { ' ', 0 };
        unsigned short buf[120];
        VIString::str8to16(buf, sizeof(buf), "/t ");
        VIString::strcat16(buf, sizeof(buf), To());
        VIString::strcat16(buf, sizeof(buf), sep);
        VIString::strcat16(buf, sizeof(buf), Text());
        app->ChatLine(buf);
        app->PlaySound(8);
        app->ClosePanel6F8();
        break;
    }
    }
}

// 0x00C8ECB0.  The editor's text goes into the field being edited.
void VIWndReply::TakeText()
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (m_sel)
    {
    case 0:
        VIString::strcpy16(m_to, sizeof(m_to), m_edit->m_text);
        m_sel = 1;
        break;
    case 1:
        VIString::strcpy16(m_text, sizeof(m_text), m_edit->m_text);
        m_sel = 2;
        break;
    }
}
