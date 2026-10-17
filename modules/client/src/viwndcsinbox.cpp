#line 1 "C:/code-frontiers-gm/game/VIWndCSInbox.cpp"
// viwndcsinbox.cpp -- CLIENT 0x00C3C6A8 .. 0x00C3E860: the customer-service inbox, the
// same source as STATION's viwndcsinbox.cpp but for the program it talks to: CLIENT asks
// its VIClient for the CS client (a virtual, slot 8), reads the clock off it
// (VIClient+0x7568), plays its sounds through VIClient::PlaySound and hands the focus with
// SetActiveWindow; Escape closes the inbox panel (ClosePanel700).  OnDrawSelf also dims
// the screen behind it first.  Addresses in the comments are CLIENT's.
#include "game.h"

// One WORDS.TXT entry in the current language, as SETUP's windows read them.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x00C3C6A8.
VIWndCSInbox::VIWndCSInbox()
{
    m_mbox = 0;
    m_read = 0;
    m_key = 0;
    m_repeat = 0;
    m_time = 0;
}

// 0x00C3C700.
VIWndCSInbox::~VIWndCSInbox()
{
    Free();
}

// 0x00C3C760.  The message box for errors and the reader, both hidden.
int VIWndCSInbox::Init()
{
    m_mbox = new VIWndMBox;
    if (!m_mbox)
        return -1;
    m_mbox->SetVisible(false);
    m_mbox->SetAppPtr(GetAppPtr());
    AddChild(m_mbox);
    if (m_mbox->Init(0, 1))
        return -1;
    m_mbox->m_pos = VIVect2I(200, 100);
    m_read = new VIWndReadCS;
    if (!m_read)
        return -1;
    m_read->SetVisible(false);
    m_read->SetAppPtr(GetAppPtr());
    m_read->Init();
    AddChild(m_read);
    m_key = 0;
    m_repeat = 0;
    m_time = 0;
    return 0;
}

// 0x00C3C850.
int VIWndCSInbox::Free()
{
    if (m_mbox)
    {
        RemoveChild(m_mbox);
        delete m_mbox;
        m_mbox = 0;
    }
    if (m_read)
    {
        RemoveChild(m_read);
        delete m_read;
        m_read = 0;
    }
    return 0;
}

// The "..." after a status line: one dot more every 15 frames, three at most.
static char sDots;    // 0x00CD7542
static char sTicks;   // 0x00CD7543

// 0x00C3C8E8.
void VIWndCSInbox::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VICSClient *client = app->GetP33950();
    VIUI *ui = UI();
    long now = app->m_p07568;
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

    unsigned short buf[128];
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(642, 332), (VIAlphaBlendMode)0, 1.0f);
    m_window.Draw();
    int x[2];
    x[0] = 58;
    x[1] = m_window.Width() - 340;
    VIString::snprintf16(buf, sizeof(buf), "%S #", Word(ui, 841));
    m_window.Text(buf, m_column ? x[0] : x[0] - 12, 0, (VIUICOLOR)0, 0);
    VIString::snprintf16(buf, sizeof(buf), "%S", Word(ui, 842));
    m_window.Text(buf, m_column != 1 ? x[1] : x[1] - 12, 0, (VIUICOLOR)0, 0);
    if (m_sel > client->GetCount() - 1)
        m_sel = client->GetCount() - 1;
    m_sel = m_sel < 0 ? 0 : m_sel;
    if (m_sel - m_top > 10)
        m_top = m_sel - 10;
    else if (m_sel < m_top)
        m_top = m_sel;
    float t;
    if (client->GetCount() == 11)
        t = 0.0f;
    else
        t = (float)m_top / (float)(client->GetCount() - 11);
    m_window.DrawFrameVScroll(40, 1, m_window.Width() - 80, 11, t);
    if (app->m_p2BD40 == this || app->m_p2BD40 == m_mbox)
    {
        for (int i = m_top; i < (m_top + 11 > client->GetCount() ? client->GetCount() : m_top + 11); i++)
        {
            VIString::snprintf16(buf, sizeof(buf), "%d", client->GetHeader(i)->m_id);
            m_window.TextBox(buf, x[0], i - m_top + 1, x[1] - x[0] - 10, 1,
                             (VIUICOLOR)(m_sel == i), 2);
            buf[0] = 0;
            if (client->GetHeader(i)->m_unread)
            {
                VIVect2I p(x[1] + 15, m_window.RowCenter(i - m_top + 1).y - 5);
                ui->BlitEx((VIUITexId)222, p, (VIAlphaBlendMode)0, 1.0f, 0);
            }
            unsigned short cat[64];
            unsigned short sub[64];
            cat[0] = 0;
            sub[0] = 0;
            FindCategory(client->GetHeader(i)->m_category, client->GetHeader(i)->m_subcategory,
                         cat, sub);
            m_window.TextBox(cat, x[1], i - m_top + 1, m_window.Width() - 44 - x[1], 1,
                             (VIUICOLOR)(m_sel == i), 2);
            if (m_sel == i)
                m_window.TickR(56, i - m_top + 1, app->m_p2BD40 == this);
        }
    }
    VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S           |s%S",
                         Word(ui, 212), Word(ui, 161), Word(ui, 843));
    m_window.Text(buf, 40, 13, (VIUICOLOR)0, 0);
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
        if (m_read_)
        {
            m_read_ = 0;
            Read();
        }
        else if (m_delete)
        {
            m_delete = 0;
            client->RequestInbox();
        }
        else if (m_reload)
        {
            m_reload = 0;
            int last = client->GetCount() - 1;
            m_sel = last < 0 ? 0 : last;
        }
        break;
    case 1:
        m_reload = 1;
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 844));
        for (d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 2:
        m_read_ = 1;
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
        m_delete = 1;
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

// 0x00C3D3E8.  The arrows repeat while held (OnDrawSelf); nothing moves while
// the client is busy.
void VIWndCSInbox::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VICSClient *client = app->GetP33950();
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
    case 17:
    case 18:
    case 136:
    case 137:
        if (client->GetState() > 0)
            break;
        break;
    case 81:
    case 113:
    case 133:
        if (client->GetState() > 0)
            break;
        Refresh();
        break;
    case 13:
    case 134:
        if (client->GetState() > 0)
            break;
        if (client->GetState() == -19)
            break;
        Select();
        break;
    case 27:
    case 132:
        app->PlaySound(8);
        app->ClosePanel700();
        break;
    }
}

// 0x00C3D550.
void VIWndCSInbox::OnKeyUp(int key)
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

// 0x00C3D590.  260: the reader closed; 295: the focus arrived.
void VIWndCSInbox::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != m_mbox && (VIWnd *)p1 != m_read)
        {
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 560, 14, 0);
            m_window.SetTitle(m_window.T(862));
            m_window.Open(true);
            m_status.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 375, 4, 0);
            m_status.SetTitle(m_status.T(271));
            m_status.Open(true);
            m_read_ = 0;
            m_delete = 0;
            m_reload = 0;
            m_mbox->SetVisible(false);
            m_read->SetVisible(false);
            m_time = app->m_p07568;
            m_key = 0;
            m_repeat = 0;
        }
        break;
    case 260:
        if (w == m_read)
        {
            w->SetVisible(false);
            app->SetActiveWindow(this);
        }
        break;
    }
}

// 0x00C3D738.  Back to the top, and the headers asked for again.
void VIWndCSInbox::Refresh()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (app->GetP33950()->GetState() <= 0)
        app->GetP33950()->RequestCategories();
    m_top = 0;
    m_sel = 0;
    m_column = 0;
}

// 0x00C3D7B8.  VICSInterface::EndCSInboxSession calls it.
void VIWndCSInbox::End()
{
}

// 0x00C3D7C0.
void VIWndCSInbox::Next()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel < app->GetP33950()->GetCount() - 1)
    {
        m_sel++;
        m_window.ResetTickPulse();
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
}

// 0x00C3D850.
void VIWndCSInbox::Prev()
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

// 0x00C3D8B8.
void VIWndCSInbox::NextColumn()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int col = m_column + 1;
    int sound = col > 1 ? 9 : 11;
    m_column = col > 1 ? 1 : col;
    app->PlaySound(sound);
}

// 0x00C3D908.
void VIWndCSInbox::PrevColumn()
{
    VIClient *app = (VIClient *)GetAppPtr();
    int col = m_column - 1;
    int sound = col < 0 ? 9 : 11;
    m_column = col < 0 ? 0 : col;
    app->PlaySound(sound);
}

// 0x00C3D958.  Ask for the selected message; the reader gets its date now and
// the rest when it is in.
void VIWndCSInbox::Select()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VICSClient *client = app->GetP33950();
    unsigned short date[64];
    if (client->GetCount() <= 0)
        return;
    client->RequestMessage(client->GetHeader(m_sel)->m_id);
    client->GetHeader(m_sel)->m_date.Format(date, sizeof(date));
    m_read->SetId(client->GetMessage()->m_id);
    m_read->SetDate(date);
    m_read->SetCategoryId(client->GetMessage()->m_category);
    m_read->SetCategoryIds(client->GetMessage()->m_category, client->GetMessage()->m_subcategory);
}

// 0x00C3DA50.  The message is in: hand it to the reader and give it the focus.
void VIWndCSInbox::Read()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VICSMessage *msg = app->GetP33950()->GetMessage();
    unsigned short cat[64];
    unsigned short sub[64];
    cat[0] = 0;
    sub[0] = 0;
    m_read->SetText(msg->m_text);
    FindCategory(msg->m_category, msg->m_subcategory, cat, sub);
    m_read->SetCategory(cat);
    m_read->SetSubcategory(sub);
    m_read->SetVisible(true);
    app->SetActiveWindow(m_read);
}

// 0x00C3DB10.  The names of a category and subcategory, by id.
int VIWndCSInbox::FindCategory(int category, int subcategory,
                               unsigned short *catName, unsigned short *subName)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIVector<VICSCategory> *cats = app->GetP33950()->GetCategories();
    for (int i = 0; i < cats->m_size; i++)
    {
        VICSCategory *c = &cats->m_data[i];
        if (c->m_id == category)
        {
            VIString::strcpy16(catName, 128, c->m_name);
            VIVector<VICSSubcategory> &subs = cats->m_data[i].m_subs;
            for (int j = 0; j < subs.m_size; j++)
            {
                VICSSubcategory *s = &subs.m_data[j];
                if (s->m_id == subcategory)
                {
                    VIString::strcpy16(subName, 128, s->m_name);
                    return 0;
                }
            }
        }
    }
    return -1;
}
