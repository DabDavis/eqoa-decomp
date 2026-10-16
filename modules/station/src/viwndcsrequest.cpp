// viwndcsrequest.cpp (name ours) -- STATION 0x002ECF00 .. 0x002F0128: a request
// to customer service, or the reply to a message.  Three fields picked in turn
// (a category and a subcategory from the client's lists, the text in a
// VIWndEdit), then Submit; a status window says what the client is doing.
#include "station.h"
#include "viarray_t.h"

// One WORDS.TXT entry in the current language, as SETUP's windows read them.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// 0x002ECF00.
VIWndCSRequest::VIWndCSRequest()
{
    m_replyTo = -1;
    m_edit = 0;
    m_mbox = 0;
    m_picking = 0;
    m_pickingSub = 0;
    m_categoryId = 0;
    m_subcategoryId = 0;
}

// 0x002ECF80.
VIWndCSRequest::~VIWndCSRequest()
{
    Free();
}

// 0x002ED008.  The editor for the text and a message box, both hidden.
int VIWndCSRequest::Init(int reply)
{
    Free();
    m_reply = reply;
    m_edit = new VIWndEdit;
    if (!m_edit)
        return -1;
    m_edit->SetVisible(false);
    m_edit->SetAppPtr(GetAppPtr());
    AddChild(m_edit);
    m_mbox = new VIWndMBox;
    if (!m_mbox)
        return -1;
    m_mbox->SetVisible(false);
    m_mbox->SetAppPtr(GetAppPtr());
    AddChild(m_mbox);
    if (m_mbox->Init(0, 1))
        return -1;
    m_mbox->m_pos = VIVect2I(100, 100);
    m_sel = 0;
    m_catSel = 0;
    m_subSel = 0;
    m_picking = 0;
    m_pickingSub = 0;
    m_x4CC = 0;
    m_categoryId = 0;
    m_subcategoryId = 0;
    m_replyTo = -1;
    m_catIds.Init(0);
    m_subIds.Init(0);
    return 0;
}

// 0x002ED138.
int VIWndCSRequest::Free()
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
    m_replyTo = -1;
    m_sel = 0;
    m_category[0] = 0;
    m_subcategory[0] = 0;
    m_text[0] = 0;
    m_categoryId = 0;
    m_subcategoryId = 0;
    return 0;
}

// The "..." after a status line: one dot more every 15 frames, three at most.
static char sDots;    // 0x0035037E
static char sTicks;   // 0x0035037F

// 0x002ED1F0.
void VIWndCSRequest::OnDrawSelf()
{
    VIStation *station = (VIStation *)GetAppPtr();
    VICSClient *client = station->m_renderer.GetCSInterface()->GetClient();
    VIUI *ui = UI();
    int i;
    if (--sTicks <= 0)
    {
        sTicks = 15;
        if (++sDots == 3)
            sDots = 0;
    }

    unsigned short buf[128];
    ui->Begin2D();
    m_window.Draw();
    VIString::snprintf16(buf, sizeof(buf), "%S:", Word(ui, 842));
    m_window.Text(buf, 40, 1, (VIUICOLOR)(m_sel == 0), 0);
    m_window.TextBox(m_window.T(m_category), 140, 1, m_window.Width() - 180, 1, (VIUICOLOR)0, 6);
    VIString::snprintf16(buf, sizeof(buf), "%S:", Word(ui, 863));
    m_window.Text(buf, 40, 3, (VIUICOLOR)(m_sel == 1), 0);
    m_window.TextBox(m_window.T(m_subcategory), 140, 3, m_window.Width() - 180, 1, (VIUICOLOR)0, 6);
    VIString::snprintf16(buf, sizeof(buf), "%S:", Word(ui, 522));
    m_window.Text(buf, 40, 5, (VIUICOLOR)(m_sel == 2), 0);
    m_window.TextBox(m_window.T(m_text), 140, 5, m_window.Width() - 180, 5, (VIUICOLOR)0, 6);
    m_window.Text(m_window.T((unsigned short)218), 40, 11, (VIUICOLOR)(m_sel == 3), 0);
    VIString::snprintf16(buf, sizeof(buf),
                         m_picking || m_pickingSub ? "|-|x%S   |-|t%S" : "|x%S   |t%S",
                         Word(ui, 212), Word(ui, 161));
    m_window.Text(buf, 40, 13, (VIUICOLOR)0, 0);
    bool pulse = !m_picking && !m_pickingSub;
    switch (m_sel)
    {
    case 0:
        m_window.TickR(36, 1, pulse);
        break;
    case 1:
        m_window.TickR(36, 3, pulse);
        break;
    case 2:
        m_window.TickR(36, 5, pulse);
        break;
    case 3:
        m_window.TickR(36, 11, pulse);
        break;
    }

    // The categories offered: the client's, less the hidden ones.
    VIVector<VICSCategory> *cats = client->GetCategories();
    m_catCount = 0;
    m_catIds.Init(cats->m_size);
    int n = 0;
    for (i = 0; i < cats->m_size; i++)
    {
        if (!(*cats)[i].m_x04)
        {
            m_catIds[n] = (*cats)[i].m_id;
            n++;
            m_catCount++;
        }
    }
    if (m_picking)
    {
        m_list.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 330, 10, 0);
        m_list.Draw();
        if (m_list.IsOpen() && !client->GetState())
        {
            if (m_catSel - m_x4CC > 6)
                m_x4CC = m_catSel - 6;
            else if (m_catSel < m_x4CC)
                m_x4CC = m_catSel;
            float t;
            if (m_catCount < 8)
                t = 0.0f;
            else
                t = (float)m_x4CC / (float)(m_catCount - 7);
            m_list.DrawFrameVScroll(25, 1, m_list.Width() - 60, 7, t);
            for (i = m_x4CC; i < VIMIN(m_x4CC + 7, m_catCount); i++)
            {
                VICSCategory *c = FindCategory(m_catIds[i]);
                if (c)
                {
                    m_list.TextBox(m_window.T(c->m_name), 42, i - m_x4CC + 1, 250, 1,
                                   (VIUICOLOR)(m_catSel == i), 2);
                    if (m_catSel == i)
                        m_list.TickR(40, i - m_x4CC + 1, true);
                }
            }
            VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 212), Word(ui, 161));
            m_list.Text(buf, 30, 9, (VIUICOLOR)0, 0);
        }
    }
    if (m_pickingSub)
    {
        m_list.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 330, 10, 0);
        m_list.Draw();
        if (m_list.IsOpen() && !client->GetState())
        {
            if (m_subSel - m_x4CC > 6)
                m_x4CC = m_subSel - 6;
            else if (m_subSel < m_x4CC)
                m_x4CC = m_subSel;
            m_subCount = 0;
            VICSCategory *c = FindCategory(m_catIds[m_catSel]);
            m_subIds.Init(c->m_subs.m_size);
            int k = 0;
            for (i = 0; i < c->m_subs.m_size; i++)
            {
                if (!c->m_subs[i].m_x04)
                {
                    m_subIds[k] = c->m_subs[i].m_id;
                    k++;
                    m_subCount++;
                }
            }
            float t;
            if (m_subCount < 8)
                t = 0.0f;
            else
                t = (float)m_x4CC / (float)(m_subCount - 7);
            m_list.DrawFrameVScroll(25, 1, m_list.Width() - 60, 7, t);
            for (i = m_x4CC; i < VIMIN(m_x4CC + 7, m_subCount); i++)
            {
                VICSSubcategory *s = FindSubcategory(m_catIds[m_catSel], m_subIds[i]);
                if (s)
                {
                    m_list.TextBox(m_window.T(s->m_name), 42, i - m_x4CC + 1, 250, 1,
                                   (VIUICOLOR)(m_subSel == i), 2);
                    if (m_subSel == i)
                        m_list.TickR(40, i - m_x4CC + 1, true);
                }
            }
            VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 212), Word(ui, 161));
            m_list.Text(buf, 30, 9, (VIUICOLOR)0, 0);
        }
    }
    unsigned short back[32];
    VIString::snprintf16(back, sizeof(back), "|t%S", Word(ui, 161));
    if (!client->GetState())
        m_status.Close(true);
    else
        m_status.Open(false);
    switch (client->GetState())
    {
    case 0:
        if (m_x4D0)
        {
            m_x4D0 = 0;
            VIUI *sui = &station->m_ui;
            station->Message(Word(sui, 864));
            station->m_ui.SoundPlay((VIUISoundId)8);
            if (m_reply)
                GetParent()->OnMessage(this, 309, 0, 0);
            else
                station->m_renderer.GetCSInterface()->EndCSRequestSession();
        }
        else if (m_x4D4)
        {
            m_x4D4 = 0;
            client->RequestInbox();
        }
        else if (m_x4D8)
        {
            m_x4D8 = 0;
            CheckFull();
        }
        break;
    case 1:
        m_x4D8 = 1;
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 844));
        for (char d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 2:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 845));
        for (char d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 3:
        m_x4D0 = 1;
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 886));
        for (char d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 4:
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 847));
        for (char d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 5:
        m_x4D0 = 1;
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 848));
        for (char d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        goto box;
    case 6:
        m_x4D4 = 1;
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 849));
        for (char d = 0; d < sDots + 1; d++)
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

// 0x002EE098.  A list that is up takes the keys; otherwise the field selector,
// and nothing moves while the client is busy.
void VIWndCSRequest::OnKeyDown(int key)
{
    VIStation *station = (VIStation *)GetAppPtr();
    int state = station->m_renderer.GetCSInterface()->GetClient()->GetState();
    if (m_picking && m_list.IsOpen())
    {
        switch (key)
        {
        case 19:
        case 138:
            m_catSel = m_catSel - 1 < 0 ? 0 : m_catSel - 1;
            station->m_ui.SoundPlay((VIUISoundId)11);
            break;
        case 20:
        case 139:
            m_catSel = m_catSel + 1 > m_catCount - 1 ? m_catCount - 1 : m_catSel + 1;
            station->m_ui.SoundPlay((VIUISoundId)11);
            break;
        case 13:
        case 134:
            PickCategory();
            break;
        case 27:
        case 132:
            m_list.Close(true);
            m_picking = 0;
            station->m_ui.SoundPlay((VIUISoundId)8);
            break;
        }
    }
    else if (m_pickingSub && m_list.IsOpen())
    {
        switch (key)
        {
        case 19:
        case 138:
            m_subSel = m_subSel - 1 < 0 ? 0 : m_subSel - 1;
            station->m_ui.SoundPlay((VIUISoundId)11);
            break;
        case 20:
        case 139:
            m_subSel = m_subSel + 1 > m_subCount - 1 ? m_subCount - 1 : m_subSel + 1;
            station->m_ui.SoundPlay((VIUISoundId)11);
            break;
        case 13:
        case 134:
            PickSubcategory();
            break;
        case 27:
        case 132:
            m_list.Close(true);
            m_pickingSub = 0;
            station->m_ui.SoundPlay((VIUISoundId)8);
            break;
        }
    }
    else if (!m_picking && !m_pickingSub)
    {
        switch (key)
        {
        case 19:
        case 138:
            if (state > 0)
                break;
            Prev();
            break;
        case 20:
        case 139:
            if (state > 0)
                break;
            Next();
            break;
        case 13:
        case 134:
            if (state > 0)
                break;
            Select();
            break;
        case 27:
        case 132:
            if (!VIString::strlen16(m_text))
                Cancel();
            else
                ShowMessage(m_window.T((unsigned short)866), 1, 1, 0);
            break;
        }
    }
}

// 0x002EE490.  259: the editor closed; 260: the editor or the message box
// closed; 268: the message box's choice; 295: the focus arrived.
void VIWndCSRequest::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIStation *station = (VIStation *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 != this || (VIWnd *)p1 == m_edit || (VIWnd *)p1 == m_mbox
            || (VIWnd *)p1 == this)
            break;
        m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 500, 14, 0);
        if (m_reply)
            m_window.SetTitle(m_window.T((unsigned short)867));
        else
            m_window.SetTitle(m_window.T((unsigned short)868));
        m_window.Open(true);
        m_list.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 330, 10, 0);
        m_list.Close(true);
        m_status.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 180), 375, 4, 0);
        m_status.SetTitle(m_status.T((unsigned short)271));
        m_status.Open(true);
        m_mbox->SetVisible(false);
        m_catSel = 0;
        m_subSel = 0;
        m_sel = m_reply ? 2 : 0;
        m_category[0] = 0;
        m_subcategory[0] = 0;
        m_text[0] = 0;
        m_picking = 0;
        m_pickingSub = 0;
        m_catCount = 0;
        m_subCount = 0;
        m_catIds.Init(0);
        m_subIds.Init(0);
        m_x4D4 = 0;
        m_x4D8 = 0;
        m_x4D0 = 0;
        break;
    case 259:
        if (m_edit)
        {
            TakeText();
            m_edit->SetVisible(false);
            station->SetActive(this);
        }
        break;
    case 260:
        if (w == m_edit || w == m_mbox)
        {
            w->SetVisible(false);
            station->SetActive(this);
        }
        break;
    case 268:
        if (w != m_mbox)
            break;
        if (p1 == 0)
        {
            w->SetVisible(false);
            station->SetActive(this);
        }
        else if (p1 == 1)
            Cancel();
        break;
    }
}

// 0x002EE798.  Back to the first field; a new request asks for the lists.
void VIWndCSRequest::Refresh()
{
    VIStation *station = (VIStation *)GetAppPtr();
    VICSClient *client = station->m_renderer.GetCSInterface()->GetClient();
    m_sel = m_reply ? 2 : 0;
    if (!m_reply && client->GetState() <= 0)
        client->RequestCategories();
}

// 0x002EE808.  VICSInterface::EndCSRequestSession calls it.
void VIWndCSRequest::End()
{
}

// 0x002EE810.
void VIWndCSRequest::Next()
{
    VIStation *station = (VIStation *)GetAppPtr();
    if (m_sel < 3)
    {
        m_sel++;
        station->m_ui.SoundPlay((VIUISoundId)11);
    }
    else
        station->m_ui.SoundPlay((VIUISoundId)9);
    m_window.ResetTickPulse();
}

// 0x002EE880.  A reply starts at the text: its category is the message's.
void VIWndCSRequest::Prev()
{
    VIStation *station = (VIStation *)GetAppPtr();
    int first = m_reply ? 2 : 0;
    if (first < m_sel)
    {
        m_sel--;
        station->m_ui.SoundPlay((VIUISoundId)11);
    }
    else
        station->m_ui.SoundPlay((VIUISoundId)9);
    m_window.ResetTickPulse();
}

// 0x002EE8F8.  Open the field's picker or editor, or submit.
void VIWndCSRequest::Select()
{
    VIStation *station = (VIStation *)GetAppPtr();
    VICSClient *client = station->m_renderer.GetCSInterface()->GetClient();
    int sound = 13;
    int state = client->GetState();
    switch (m_sel)
    {
    case 0:
        if (!m_reply && state != -19)
        {
            m_picking = 1;
            m_list.SetTitle(m_list.T((unsigned short)869));
            m_list.Open(false);
            m_catSel = 0;
            m_x4CC = 0;
        }
        else
            sound = 10;
        break;
    case 1:
        if (!m_reply && state != -19)
        {
            if (!VIString::strlen16(m_category))
            {
                sound = 10;
                ShowMessage(m_window.T((unsigned short)870), 0, 0, 0);
            }
            else
            {
                m_pickingSub = 1;
                m_list.SetTitle(m_list.T((unsigned short)871));
                m_list.Open(false);
                m_subSel = 0;
                m_x4CC = 0;
            }
        }
        else
            sound = 10;
        break;
    case 2:
        m_edit->SetRows(4);
        m_edit->SetMultiLine(1);
        m_edit->SetCenter(m_window.Center());
        m_edit->SetMaxLength(512);
        m_edit->SetText(m_text);
        m_edit->SetVisible(true);
        station->SetActive(m_edit);
        break;
    case 3:
        if (!VIString::strlen16(m_category))
        {
            sound = 10;
            ShowMessage(m_window.T((unsigned short)870), 0, 0, 0);
        }
        else if (!VIString::strlen16(m_subcategory))
        {
            sound = 10;
            ShowMessage(m_window.T((unsigned short)872), 0, 0, 0);
        }
        else if (!VIString::strlen16(m_text))
        {
            sound = 10;
            ShowMessage(m_window.T((unsigned short)873), 0, 0, 0);
        }
        else if (m_reply)
            client->SubmitReply(m_replyTo, m_text);
        else
            client->SubmitRequest(m_categoryId, m_subcategoryId, m_text);
        break;
    }
    station->m_ui.SoundPlay((VIUISoundId)sound);
}

// 0x002EEB90.  The editor's text, back into the field it edited.
void VIWndCSRequest::TakeText()
{
    VIStation *station = (VIStation *)GetAppPtr();
    switch (m_sel)
    {
    case 0:
        break;
    case 1:
        break;
    case 2:
        VIString::strcpy16(m_text, sizeof(m_text), m_edit->m_text);
        break;
    }
}

// 0x002EEBE8.  A reply goes back to the reader; a request ends the session.
void VIWndCSRequest::Cancel()
{
    VIStation *station = (VIStation *)GetAppPtr();
    station->m_ui.SoundPlay((VIUISoundId)8);
    if (m_reply)
        GetParent()->OnMessage(this, 260, 0, 0);
    else
        station->m_renderer.GetCSInterface()->EndCSRequestSession();
}

// 0x002EEC78.  The category under the cursor becomes the request's.
void VIWndCSRequest::PickCategory()
{
    VIStation *station = (VIStation *)GetAppPtr();
    VIVector<VICSCategory> *cats = station->m_renderer.GetCSInterface()->GetClient()->GetCategories();
    VICSCategory *c = 0;
    for (int i = 0; i < cats->m_size; i++)
    {
        c = &cats->m_data[i];
        if (c->m_id == m_catIds[m_catSel])
            break;
    }
    if (c->m_id == m_catIds[m_catSel])
    {
        VIString::strcpy16(m_category, sizeof(m_category), c->m_name);
        m_categoryId = m_catIds[m_catSel];
    }
    else
        m_category[0] = 0;
    station->m_ui.SoundPlay((VIUISoundId)13);
    m_list.Close(true);
    m_picking = 0;
    m_pickingSub = 1;
    m_list.SetTitle(m_list.T((unsigned short)871));
    m_list.Open(false);
    m_subSel = 0;
    m_x4CC = 0;
    m_subcategoryId = 0;
    m_subcategory[0] = 0;
}

// 0x002EEDF0.  The subcategory under the cursor becomes the request's.
void VIWndCSRequest::PickSubcategory()
{
    VIStation *station = (VIStation *)GetAppPtr();
    VIVector<VICSCategory> *cats = station->m_renderer.GetCSInterface()->GetClient()->GetCategories();
    m_subcategory[0] = 0;
    m_subcategoryId = -1;
    for (int i = 0; i < cats->m_size; i++)
    {
        if ((*cats)[i].m_id == m_categoryId)
        {
            for (int j = 0; j < (*cats)[i].m_subs.m_size; j++)
            {
                if ((*cats)[i].m_subs[j].m_id == m_subIds[m_subSel])
                {
                    VIString::strcpy16(m_subcategory, sizeof(m_subcategory), (*cats)[i].m_subs[j].m_name);
                    m_subcategoryId = m_subIds[m_subSel];
                    break;
                }
            }
            break;
        }
    }
    m_sel = 2;
    station->m_ui.SoundPlay((VIUISoundId)13);
    m_list.Close(true);
    m_pickingSub = 0;
}

// 0x002EEF48.
void VIWndCSRequest::SetCategory(const unsigned short *s)
{
    VIString::strcpy16(m_category, sizeof(m_category), s);
}

// 0x002EEF70.
void VIWndCSRequest::SetSubcategory(const unsigned short *s)
{
    VIString::strcpy16(m_subcategory, sizeof(m_subcategory), s);
}

// 0x002EEF98.
void VIWndCSRequest::SetText(const unsigned short *s)
{
    VIString::strcpy16(m_text, sizeof(m_text), s);
}

// 0x002EEFC0.  Only a reply has a message to answer.
int VIWndCSRequest::SetReplyTo(int id)
{
    if (!m_reply)
    {
        m_replyTo = -1;
        return -1;
    }
    m_replyTo = id;
    return 0;
}

// 0x002EEFE8.  A hundred messages is a full inbox: say so and end the session.
void VIWndCSRequest::CheckFull()
{
    VIStation *station = (VIStation *)GetAppPtr();
    VICSInterface *cs = station->m_renderer.GetCSInterface();
    VICSClient *client = cs->GetClient();
    if (!m_reply && client->GetCount() >= 100)
    {
        const unsigned short *s = m_window.T((unsigned short)874);
        VIColor32F red(1.0f, 0.25f, 0.25f, 1.0f);
        station->Message(s);
        cs->EndCSRequestSession();
    }
}

// 0x002EF098.  The category with this id; the last one looked at if none.
VICSCategory *VIWndCSRequest::FindCategory(int id)
{
    VICSCategory *c = 0;
    VIStation *station = (VIStation *)GetAppPtr();
    VIVector<VICSCategory> *cats = station->m_renderer.GetCSInterface()->GetClient()->GetCategories();
    for (int i = cats->Begin(); i != cats->End(); i = cats->Next(i))
    {
        c = &cats->m_data[i];
        if (c->m_id == id)
            break;
    }
    return c;
}

// 0x002EF170.
VICSSubcategory *VIWndCSRequest::FindSubcategory(int category, int id)
{
    VICSSubcategory *s = 0;
    VIStation *station = (VIStation *)GetAppPtr();
    VICSCategory *c = FindCategory(category);
    if (c)
    {
        VIVector<VICSSubcategory> &subs = c->m_subs;
        for (int i = subs.Begin(); i != subs.End(); i = subs.Next(i))
        {
            s = &subs.m_data[i];
            if (s->m_id == id)
                break;
        }
    }
    return s;
}

// 0x002EF258.  The message box over this window, with the focus.
void VIWndCSRequest::ShowMessage(const unsigned short *text, int prompt, int a, int b)
{
    VIStation *station = (VIStation *)GetAppPtr();
    m_mbox->Init(prompt, 1);
    m_mbox->SetText(text);
    m_mbox->m_p1 = a;
    m_mbox->m_p2 = b;
    m_mbox->SetVisible(true);
    station->SetActive(m_mbox);
}
