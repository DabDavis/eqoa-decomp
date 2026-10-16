// viguidialog.cpp (name ours) -- STATION 0x002BFD18 .. 0x002C10B8.  The dialog
// a page opens over itself: a title, up to 1,024 characters of text scrolled
// seven rows at a time, and the action an O press answers with.
#include "station.h"

// VIStation embeds its VIUI at +0xAFE0 (lane core owns VIStation's layout).
static inline VIUI *StationUI(void *app)
{
    return (VIUI *)((char *)app + 0xAFE0);
}

// One WORDS.TXT entry in the current language, as SETUP's windows read it.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x002BFD18.
VIGUIDialog::VIGUIDialog()
{
    memset(m_action, 0, sizeof(m_action));
}

// 0x002BFD68.
VIGUIDialog::~VIGUIDialog()
{
    m_window.Free();
    memset(m_text, 0, sizeof(m_text));
}

// 0x002BFDE0.
void VIGUIDialog::Init(VIWndGenericRenderer *owner, int cols, int rows)
{
    VIGUIPage::Init(owner, cols, rows);
    memset(m_text, 0, sizeof(m_text));
    void *app = m_owner->GetAppPtr();
    if (app)
    {
        m_prompt = 1;
        m_confirm = 0;
        m_rows = 0;
        m_scroll = 0;
        m_xE0C = 0;
        m_window.Init(StationUI(app), VIVect2I(0, 0), VIVect2I(320, 224), 400, 11, 0);
    }
}

// 0x002BFE80.
void VIGUIDialog::SetText(const unsigned short *s)
{
    if (s)
    {
        VIString::strcpy16(m_text, sizeof(m_text), s);
        m_rows = m_window.CountLines(m_text, 320);
        m_scroll = 0;
    }
    m_prompt = 1;
}

// 0x002BFEE0.
void VIGUIDialog::SetText(const char *s)
{
    if (s)
    {
        VIString::str8to16(m_text, sizeof(m_text), s);
        m_rows = m_window.CountLines(m_text, 320);
        m_scroll = 0;
    }
    m_prompt = 1;
}

// 0x002BFF40.
int VIGUIDialog::IsOpen()
{
    return m_window.IsOpening() || m_window.IsOpen();
}

// 0x002BFF98.
void VIGUIDialog::SetConfirm()
{
    m_confirm = 1;
}

// 0x002BFFA8.
void VIGUIDialog::ClearConfirm()
{
    m_confirm = 0;
}

// 0x002BFFB0.  The prompt row is "X <1237>", or "TRIANGLE <753>" while a
// confirmation is up.
void VIGUIDialog::Draw()
{
    if (!m_owner)
        return;
    void *app = m_owner->GetAppPtr();
    if (!app)
        return;
    VIUI *ui = StationUI(app);
    ui->Begin2D();
    if (m_window.IsOpening() || m_window.IsOpen())
    {
        if (m_prompt)
            m_window.Resize(400, 11);
        else
            m_window.Resize(400, 9);
        m_window.Draw();
        // A statement whose result nobody reads: its stack temporary is free
        // again for the prompt row's colour, as the original's was.
        m_window.BottomRight();
        float t;
        if (m_rows < 8)
            t = -1.0f;
        else
            t = (float)m_scroll / (float)(m_rows - 7);
        m_window.DrawFrameVScroll(20, 0, 360, 9, t);
        if (m_text[0])
            m_window.TextBox(m_text, 20, 1, 370, 7, VIColor32F(0, 0, 0, 1.0f), m_scroll << 24);
        if (m_prompt)
        {
            unsigned short buf[128];
            if (!m_confirm)
                VIString::snprintf16(buf, sizeof(buf), "|x %S", Word(ui, 1237));
            else
                VIString::snprintf16(buf, sizeof(buf), "|t %S", Word(ui, 753));
            m_window.Text(buf, 153, 10, VIColor32F(0, 0, 0, 1.0f), 0);
        }
    }
    ui->End2D();
}

// 0x002C01D8.
void VIGUIDialog::Open(const unsigned short *title, const char *action)
{
    char buf[1024];
    VIString::str16to8(buf, sizeof(buf), title);
    Open(buf, action);
    m_prompt = 1;
}

// 0x002C0230.
void VIGUIDialog::Open(const char *title, const char *action)
{
    if (title && *title)
        m_window.SetTitle(title);
    if (action)
        strcpy(m_action, action);
    else
        memset(m_action, 0, sizeof(m_action));
    m_window.Open(false);
    m_prompt = 1;
}

// 0x002C02B0.
void VIGUIDialog::Close()
{
    m_window.Close(true);
}

// 0x002C02D0.
void VIGUIDialog::SetPrompt(int on)
{
    m_prompt = on;
}

// 0x002C02D8.
void VIGUIDialog::OnKeyUp(int key)
{
}

// 0x002C02E0.  While a confirmation is up, X cancels it and O sends the
// disconnect; otherwise O or X closes the dialog (O sending its action first)
// and up and down scroll the text.
void VIGUIDialog::OnKeyDown(int key)
{
    if (!m_prompt)
        return;
    if (!m_owner)
        return;
    void *app = m_owner->GetAppPtr();
    if (!app)
        return;
    VIUI *ui = StationUI(app);
    if (m_confirm)
    {
        if (key == 132)
        {
            ClearConfirm();
            ui->SoundPlay((VIUISoundId)8);
            m_window.Close(false);
        }
        else if (key == 134)
        {
            if (!strcmp(m_action, "DisconnectConfirmed"))
                m_owner->OnAction(m_action);
        }
        return;
    }
    if ((key == 134 || !strcmp(m_action, "DisconnectConfirmed")) && m_action[0])
        m_owner->OnAction(m_action);
    if (key == 134 || key == 132)
    {
        ui->SoundPlay((VIUISoundId)8);
        m_window.Close(false);
    }
    if (key == 138)
    {
        if (m_scroll > 0)
        {
            m_scroll--;
            ui->SoundPlay((VIUISoundId)11);
        }
        else
            ui->SoundPlay((VIUISoundId)9);
    }
    if (key == 139)
    {
        if (m_scroll < m_rows - 7)
        {
            m_scroll++;
            ui->SoundPlay((VIUISoundId)11);
        }
        else
            ui->SoundPlay((VIUISoundId)9);
    }
}

// 0x002C04A0.
void VIGUIDialog::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
}
