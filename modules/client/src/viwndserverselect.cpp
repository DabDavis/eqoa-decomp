#line 1 "C:/code-frontiers-gm/game/VIWndServerSelect.cpp"
// VIWndServerSelect (name RTTI's) -- CLIENT 0x00C910D8 .. 0x00C93680: the server (shard)
// select screen, VIClient+0x2BB80.  It connects to a random shard for the shard list
// (UpdateList), shows it sorted by name with the home shard preselected, and on X saves
// the home shard and counts down to VIClient::ConnectShard.  The VIArray<VIItemStat> and
// VIMap<int, int> copies between UpdateConnect and __tf are template instantiations this
// TU does not write.
#include "game.h"

extern "C" int  memcmp(const void *a, const void *b, int n);
VIWnd *McErrWnd();                                         // SUPPORT
int    McErr(VIWnd *parent, const VIVect2I &center, int x); // SUPPORT
int    VIIsMcSameSession();                                // SUPPORT

// One WORDS.TXT entry in the current language.
static inline const unsigned short *Word(VIUI *ui, int id)
{
    return (const unsigned short *)ui->m_words.GetString(id, ui->m_language);
}

// The window Sort's comparator works for (0x00CF5B38).
static VIWndServerSelect *sThis;

// The "..." after a status line: one dot more every 15 frames, three at most.
static char sDots;                 // 0x00CD79B0
static char sTicks;                // 0x00CD79B1

// 0x00C910D8.
VIWndServerSelect::VIWndServerSelect()
{
    m_message = 0;
    m_child = 0;
}

// 0x00C91130.
VIWndServerSelect::~VIWndServerSelect()
{
    if (m_child)
    {
        printf("ss - removing child (destructor)\n");
        RemoveChild(m_child);
        m_child = 0;
    }
    Free();
}

// 0x00C911C8.
int VIWndServerSelect::Init(VIShardInfo *shards)
{
    int ret;
    m_shards = shards;
    m_message = new VIWndMessage;
    if (m_message)
    {
        m_message->SetVisible(false);
        AddChild(m_message);
        ret = 0;
    }
    else
        ret = -1;
    return ret;
}

// 0x00C91220.
void VIWndServerSelect::Free()
{
    if (m_message)
    {
        RemoveChild(m_message);
        delete m_message;
        m_message = 0;
    }
}

// 0x00C91278.
void VIWndServerSelect::OnDrawSelf()
{
    unsigned short buf[128];
    VIUI *ui = UI();
    if (m_child)
    {
        RemoveChild(m_child);
        m_child = 0;
    }
    VIClient *app = (VIClient *)GetAppPtr();
    if (!m_failed && UpdateList() != 1 && UpdateConnect() != 1 && app->m_p4F0E8 != 21)
    {
        m_failed = 1;
        int w = m_window.TextWidth(app->ConnectStatusString());
        m_message->Setup(m_window.T(272), app->ConnectStatusString(), VIMESSAGEPROMPT_OK,
                         VIVect2I(320, 224), w > 440 ? 540 : w + 100);
        m_message->SetVisible(true);
        app->SetActiveWindow(m_message);
    }
    if (m_shards->GetCount() && m_home[0])
    {
        for (int i = 0; i < m_shards->GetCount(); i++)
        {
            if (!VIString::strcmp16(m_shards->GetEntry(i)->m_name, m_home) && !m_shards->IsValid(i))
            {
                int f = m_shards->FirstValid();
                if (f != -1)
                {
                    VIString::strcpy16(m_home, sizeof(m_home), m_shards->GetEntry(f)->m_name);
                    break;
                }
            }
        }
    }
    Sort();
    ui->Begin2D();
    if (m_connecting)
        m_window.ResetTickPulse();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(641, 449), (VIAlphaBlendMode)3, 1.0f);
    m_window.Draw();
    if (m_sel - m_top >= m_rows)
        m_top = m_sel - (m_rows - 1);
    else if (m_sel < m_top)
        m_top = m_sel;
    float t;
    if (m_shards->GetCount() == m_rows)
        t = 0.0f;
    else
        t = (float)m_top / (float)(m_shards->GetCount() - m_rows);
    if (m_empty)
        t = 0.0f;
    m_window.DrawFrameVScroll(40, 2, m_window.Width() - 80, m_rows, t);
    int cols[3];
    cols[0] = 58;
    cols[2] = 460;
    cols[1] = cols[0] + (cols[2] - cols[0]) * 65 / 100;
    m_window.TextBox(m_window.T(509), cols[0] - 12, 1, cols[1] - cols[0], 1, (VIUICOLOR)0, 0);
    m_window.TextBox(Word(UI(), 803), cols[1], 1, cols[2] - cols[1], 1, (VIUICOLOR)0, 0);
    if (m_shards->GetCount())
    {
        for (int i = m_top; i < (m_top + m_rows > m_shards->GetCount() ? m_shards->GetCount() : m_top + m_rows); i++)
        {
            m_window.TextBox(m_shards->GetEntry(m_order[i])->m_name, cols[0], i - m_top + 2,
                             cols[1] - cols[0] - 16, 1, (VIUICOLOR)(m_sel == i), 2);
            unsigned short state[256];
            if (!m_shards->IsValid(m_order[i]))
                VIString::strcpy16(state, sizeof(state), Word(UI(), 1605));
            else if (m_home[0] && !VIString::strcmp16(m_home, m_shards->GetEntry(m_order[i])->m_name))
                VIString::strcpy16(state, sizeof(state), Word(UI(), 1607));
            else
                VIString::strcpy16(state, sizeof(state), Word(UI(), 1608));
            m_window.TextBox(state, cols[1], i - m_top + 2, cols[2] - cols[1], 1, (VIUICOLOR)(m_sel == i), 0);
            if (m_sel == i)
                m_window.TickR(56, i - m_top + 2, true);
        }
    }
    if (m_connecting)
        VIString::snprintf16(buf, sizeof(buf), "|-|x%S   |t%S", Word(ui, 212), Word(ui, 217));
    else
        VIString::snprintf16(buf, sizeof(buf), "|x%S   |t%S", Word(ui, 212), Word(ui, 217));
    m_window.Text(buf, 40, 15, (VIUICOLOR)0, 0);
    if (--sTicks <= 0)
    {
        sTicks = 15;
        if (++sDots == 3)
            sDots = 0;
    }
    if (m_fetching)
    {
        m_status.Draw();
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 938));
        for (char d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        m_status.TextBox(buf, 40, 1, m_status.Width(), 1, (VIUICOLOR)0, 0);
    }
    else if (m_connecting)
    {
        m_status.Draw();
        VIString::strcpy16(buf, sizeof(buf), Word(ui, 939));
        for (char d = 0; d < sDots + 1; d++)
            VIString::strcat16(buf, sizeof(buf), ".");
        m_status.TextBox(buf, 40, 1, m_status.Width(), 1, (VIUICOLOR)0, 0);
    }
    if (m_countdown)
    {
        m_countdown--;
        m_box.Init(UI(), VIVect2I(0, 0), m_window.Center(), 400, 3, 0);
        m_box.Open(true);
        m_box.Draw();
        m_box.TextBox(Word(UI(), 1615), 40, 0, m_box.Width() - 80, 3, (VIUICOLOR)0, 8);
        if (!m_countdown)
            Connect();
    }
    if (m_cardError)
    {
        m_box.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 440, 6, 0);
        m_box.Open(true);
        m_box.Draw();
        unsigned short msg[256];
        VIString::snprintf16(msg, sizeof(msg), "%S\n\n%S", Word(UI(), 1609), Word(UI(), 1610));
        m_box.TextBox(msg, 40, 0, m_box.Width() - 80, 4, (VIUICOLOR)0, 8);
        VIString::snprintf16(msg, sizeof(msg), "|q%S|w", Word(UI(), 786));
        m_box.TextBox(msg, 40, 5, m_box.Width() - 80, 1, (VIUICOLOR)0, 8);
    }
    ui->End2D();
}

// 0x00C91BC8.
void VIWndServerSelect::OnKeyUp(int key)
{
}

// 0x00C91BD0.
void VIWndServerSelect::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_fetching)
        return;
    if (m_countdown)
        return;
    if (m_cardError)
    {
        if (key == 13 || key == 134)
        {
            m_cardError = 0;
            m_countdown = 3;
        }
        return;
    }
    switch (key)
    {
    case 19:
    case 138:
        if (m_connecting)
            break;
        Prev();
        break;
    case 20:
    case 139:
        if (m_connecting)
            break;
        Next();
        break;
    case 27:
    case 132:
        if (m_connecting)
            break;
        UI()->SoundPlay((VIUISoundId)8);
        app->m_p4F0EC = -1;
        app->Disconnect(0, 0);
        app->Msg0777Done(3);
        break;
    case 13:
    case 134:
        if (m_connecting)
            break;
        UI()->SoundPlay((VIUISoundId)15);
        if (!m_shards->IsValid(m_order[m_sel]))
        {
            m_invalid = 1;
            m_message->Setup(Word(UI(), 1605), Word(UI(), 666), VIMESSAGEPROMPT_OK, VIVect2I(320, 224), 440);
            m_message->SetVisible(true);
            app->SetActiveWindow(m_message);
        }
        else
        {
            VIGetGameOptions()->SetHomeShardName(m_shards->GetEntry(m_order[m_sel])->m_name);
            m_countdown = 3;
        }
        break;
    }
}

// 0x00C91E48.  The options (the home shard) go to the memory card first.
int VIWndServerSelect::Connect()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (VIMemCardStatus(0))
    {
        McErr(this, VIVect2I(320, 224), 4);
        return -1;
    }
    int same = VIIsMcSameSession();
    if (same != 1)
    {
        McErr(this, VIVect2I(320, 224), 4);
        return -1;
    }
    if (VIGetGameOptions()->Save() < 0)
    {
        m_cardError = same;
        return -1;
    }
    m_connecting = same;
    app->ConnectShard(m_shards->GetId(m_order[m_sel]));
    return 0;
}

// 0x00C91F38.  295: the focus moved (p1 from, p2 to); 307: a message was dismissed;
// 4352/4354/4356: a child to add, hold or activate; 4358: the card error's answer.
void VIWndServerSelect::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
        if ((VIWnd *)p2 == this && (VIWnd *)p1 != m_message && (VIWnd *)p1 != McErrWnd())
        {
            m_window.Close(true);
            m_window.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 500, 16, 0);
            m_window.SetTitle(m_window.T(940));
            m_window.Open(true);
            m_sel = 0;
            m_name[0] = 0;
            m_home[0] = 0;
            m_top = 0;
            m_rows = 12;
            Sort();
            m_status.Close(true);
            m_status.Init(UI(), VIVect2I(0, 0), VIVect2I(320, 224), 330, 3, 0);
            m_status.SetTitle(m_status.T(271));
            m_status.Open(false);
            m_fetching = 1;
            m_delay = 50;
            m_retries = 4;
            m_connecting = 0;
            m_reconnectFailed = 0;
            m_failed = 0;
            m_requested = 0;
            m_invalid = 0;
            m_cardError = 0;
            m_empty = 0;
            m_countdown = 0;
            m_child = 0;
        }
        break;
    case 307:
        app->SetActiveWindow(this);
        if (m_invalid)
            m_invalid = 0;
        else if (m_empty)
        {
            app->m_p4F0EC = -1;
            app->Disconnect(0, 0);
            app->Msg0777Done(3);
        }
        else if (app->DisconnectKind() == 2)
            app->Msg0777Done(3);
        else
            app->Msg0777Done(app->DisconnectKind());
        break;
    case 4356:
        app->SetActiveWindow(w);
        break;
    case 4352:
        AddChild(w);
        break;
    case 4354:
        m_child = w;
        break;
    case 4358:
        if (p1 == 3)
            app->Quit();
        else
        {
            app->SetActiveWindow(this);
            m_countdown = 3;
        }
        break;
    }
}

// 0x00C92240.
void VIWndServerSelect::Next()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_sel < m_shards->GetCount() - 1)
    {
        m_sel++;
        m_window.ResetTickPulse();
        app->PlaySound(11);
    }
    else
        app->PlaySound(9);
    if (m_shards->GetCount())
    {
        VIString::strcpy16(m_name, sizeof(m_name), m_shards->GetEntry(m_order[m_sel])->m_name);
        Sort();
    }
}

// 0x00C922F8.
void VIWndServerSelect::Prev()
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
    if (m_shards->GetCount())
    {
        VIString::strcpy16(m_name, sizeof(m_name), m_shards->GetEntry(m_order[m_sel])->m_name);
        Sort();
    }
}

// 0x00C923A0.  The list by name, the selection kept on the shard it was on.
void VIWndServerSelect::Sort()
{
    int i;
    sThis = this;
    if (m_shards->GetCount())
    {
        for (i = 0; i < 256; i++)
            m_order[i] = i;
        int n = VIMIN(256, m_shards->GetCount());
        qsort(m_order, m_shards->GetCount(), 1, Compare);
        int old = m_sel;
        m_sel = -1;
        if (!m_name[0])
            m_sel = 0;
        else
            for (i = 0; i < m_shards->GetCount(); i++)
                if (!memcmp(m_name, m_shards->GetEntry(m_order[i])->m_name, VIString::strlen16(m_name)))
                {
                    m_sel = i;
                    break;
                }
        if (m_sel == -1)
            m_sel = old - 1 < 0 ? 0 : old - 1;
        m_sel = VIMAX(0, VIMIN(m_sel, m_shards->GetCount() - 1));
        VIString::strcpy16(m_name, sizeof(m_name), m_shards->GetEntry(m_order[m_sel])->m_name);
    }
}

// The two shard indices a comparison is over (ours: the name and the type).  A 2-byte
// struct with byte alignment is BLKmode here, so the assignment below is a block move
// (`lb lb sb sb`), from the temporary to `p`.
struct VIShardPair
{
    VIShardPair() {}
    VIShardPair(const unsigned char &a, const unsigned char &b) : m_a(a), m_b(b) {}
    unsigned char m_a, m_b;
};

// 0x00C92590.  qsort's comparator over m_order: by shard name.
int VIWndServerSelect::Compare(const void *a, const void *b)
{
    VIShardPair p;
    p = VIShardPair(*(const unsigned char *)a, *(const unsigned char *)b);
    VIShardInfo *s = sThis->m_shards;
    return VIString::strcmp16(s->GetEntry(p.m_a)->m_name, s->GetEntry(p.m_b)->m_name);
}

// 0x00C92608.
int VIWndServerSelect::UpdateList()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_fetching && !m_failed)
    {
        if (!m_requested)
        {
            if (m_delay)
                m_delay--;
            else
            {
                printf("Connecting for server list...\n");
                m_requested = 1;
                app->ConnectRandomShard();
            }
        }
        else
        {
            switch (app->m_p4F0E8)
            {
            case 21:
                m_fetching = 0;
                m_sel = -1;
                if (!m_shards->GetCount())
                {
                    m_empty = 1;
                    m_message->Setup(m_window.T(1605), m_window.T(1606), VIMESSAGEPROMPT_OK,
                                     VIVect2I(320, 224), 400);
                    m_message->SetVisible(true);
                    app->SetActiveWindow(m_message);
                    break;
                }
                for (int i = 0; i < m_shards->GetCount(); i++)
                    if (!VIString::strcmp16(m_shards->GetEntry(i)->m_name,
                                            VIGetGameOptions()->HomeShardName()))
                    {
                        m_sel = i;
                        VIString::strcpy16(m_name, sizeof(m_name), m_shards->GetEntry(i)->m_name);
                        break;
                    }
                if (m_sel == -1)
                {
                    m_sel = m_shards->FirstValid();
                    if (m_sel == -1)
                        m_sel = 0;
                    else
                        VIString::strcpy16(m_home, sizeof(m_home), m_shards->GetEntry(m_sel)->m_name);
                    VIString::strcpy16(m_name, sizeof(m_name), m_shards->GetEntry(m_sel)->m_name);
                }
                Sort();
                m_requested = 0;
                break;
            case 20:
                break;
            case 3:
            case 4:
            case 5:
            case 12:
            case 16:
                m_retries = 0;
            default:
                printf("Connect for server list failed.\n");
                if (m_retries)
                {
                    m_retries--;
                    m_delay = 50;
                    m_requested = 0;
                }
                else
                {
                    m_failed = 1;
                    int w = m_window.TextWidth(app->ConnectStatusString());
                    m_message->Setup(m_window.T(272), app->ConnectStatusString(), VIMESSAGEPROMPT_OK,
                                     VIVect2I(320, 224), w > 440 ? 540 : w + 100);
                    m_message->SetVisible(true);
                    app->SetActiveWindow(m_message);
                }
                break;
            }
        }
    }
    if (m_failed)
        return -1;
    return m_fetching != 0;
}

// 0x00C92918.
int VIWndServerSelect::UpdateConnect()
{
    VIClient *app = (VIClient *)GetAppPtr();
    if (m_connecting && !m_reconnectFailed)
    {
        switch (app->m_p4F0E8)
        {
        case 3:
        case 4:
        case 5:
        case 12:
        case 16:
        default:
        {
            printf("Reconnect (to game server) failed.\n");
            m_reconnectFailed = 1;
            int w = m_window.TextWidth(app->ConnectStatusString());
            m_message->Setup(m_window.T(272), app->ConnectStatusString(), VIMESSAGEPROMPT_OK,
                             VIVect2I(320, 224), w > 440 ? 540 : w + 100);
            m_message->SetVisible(true);
            app->SetActiveWindow(m_message);
            break;
        }
        case 20:
        case 21:
            break;
        }
    }
    if (m_reconnectFailed)
        return -1;
    return m_connecting != 0;
}
