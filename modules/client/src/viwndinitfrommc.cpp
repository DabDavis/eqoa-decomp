#line 1 "C:/code-frontiers-gm/game/VIWndInitFromMc.cpp"
// viwndinitfrommc.cpp -- CLIENT 0x00C63218 .. 0x00C644C8: VIWndInitFromMc, the start-up
// window that loads the words, the options and the shard list off the memory card
// (lane cl-wh).
#include "game.h"

extern "C" char *strstr(const char *s, const char *find);

// 0x00C63218.
VIWndInitFromMc::VIWndInitFromMc()
{
    m_child = 0;
}

// 0x00C63258.
VIWndInitFromMc::~VIWndInitFromMc()
{
    if (m_child)
    {
        printf("removing child (destructor)\n");
        RemoveChild(m_child);
        m_child = 0;
    }
}

// 0x00C632D0.
int VIWndInitFromMc::Init()
{
    m_state = 0;
    m_error = 0;
    m_child = 0;
    return 0;
}

// 0x00C632E8.
void VIWndInitFromMc::Free()
{
}

// 0x00C632F0.  One step a frame, each behind a card check.
void VIWndInitFromMc::OnDrawSelf()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIUI *ui = UI();
    if (m_child)
    {
        printf("removing child (main)\n");
        RemoveChild(m_child);
        m_child = 0;
    }
    ui->Begin2D();
    ui->BlitRect((VIUITexId)32, VIVect2I(-1, -1), VIVect2I(641, 449), (VIAlphaBlendMode)3, 1.0f);
    ui->End2D();
    if (m_error)
        return;
    switch (m_state)
    {
    case 0:
        m_error = 1;
        McErr(this, VIVect2I(320, 224), 4);
        m_state = 1;
        break;
    case 1:
        if (VIMemCardStatus(0))
        {
            m_error = 1;
            McErr(this, VIVect2I(320, 224), 4);
        }
        else
        {
            int r;
            do
                r = ui->ReloadWords();
            while (r == -1);
            if (r == -2)
            {
                m_error = 1;
                McErr(this, VIVect2I(320, 224), 4);
            }
            else
            {
                printf("Loaded words OK\n");
                m_state = 2;
            }
        }
        break;
    case 2:
        if (VIMemCardStatus(0))
        {
            m_error = 1;
            McErr(this, VIVect2I(320, 224), 4);
        }
        else
        {
            if (!VIGetGameOptions()->Load())
            {
                printf("Loaded game options OK\n");
                VISoundDevice *snd = ui->m_sound;
                snd->SetSoundClassVolume((VISOUNDCLASS)0, VIGetGameOptions()->m_musicVolume);
                snd = ui->m_sound;
                snd->SetSoundClassVolume((VISOUNDCLASS)1, VIGetGameOptions()->m_effectVolume);
            }
            m_state = 3;
        }
        break;
    case 3:
        if (VIMemCardStatus(0))
        {
            m_error = 1;
            McErr(this, VIVect2I(320, 224), 4);
        }
        else
        {
            app->FreeBuffer();
            if (LoadHosts() < 0)
            {
                m_error = 1;
                McErr(this, VIVect2I(320, 224), 4);
            }
            else
            {
                printf("Read 'shard list' server list.\n");
                m_state = 4;
            }
        }
        break;
    case 4:
        app->ShowLoginWindow();
        break;
    }
}

// 0x00C63540.
void VIWndInitFromMc::OnKeyUp(int key)
{
}

// 0x00C63548.
void VIWndInitFromMc::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
}

// 0x00C63568.  4352/4354/4356 come from the McErr box; 4358 is its answer (3 = quit).
void VIWndInitFromMc::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 295:
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
            m_error = 0;
        }
        break;
    }
}

// 0x00C63650 (STATION's vicsinterface.cpp has the same static, byte for byte): the next
// word of buf from *pos into line, and the separators after it skipped.  0 at the end.
static int GetLine(const char *buf, int size, int *pos, char *line)
{
    if (*pos == size)
    {
        *line = 0;
        return 0;
    }
    while (buf[*pos] != ' ' && buf[*pos] != '\n' && buf[*pos] != '\r')
    {
        *line = buf[*pos];
        line++;
        *line = 0;
        (*pos)++;
        if (*pos == size)
            return 1;
    }
    while (buf[*pos] == ' ' || buf[*pos] == '\n' || buf[*pos] == '\r')
    {
        (*pos)++;
        if (*pos == size)
            break;
    }
    return 1;
}

// 0x00C63708.  althosts.txt if the card has one, else eqahosts.txt: one shard per line,
// host:port or a host and 10070.
int VIWndInitFromMc::LoadHosts()
{
    char line[256];
    VIClient *app = (VIClient *)GetAppPtr();
    char *buf = new char[8192];
    int size = VIMemCardReadFile("althosts.txt", buf, 8192);
    if (size < 0)
    {
        size = VIMemCardReadFile("eqahosts.txt", buf, 8192);
        if (size < 0)
        {
            delete [] buf;
            return -1;
        }
    }
    int n = 0;
    int pos = 0;
    while (GetLine(buf, size, &pos, line))
        n++;
    if (!n)
    {
        delete [] buf;
        return -1;
    }
    app->AllocNames(n);
    n = 0;
    pos = 0;
    int port;
    while (GetLine(buf, size, &pos, line))
    {
        char *colon = strstr(line, ":");
        if (!colon)
            port = 10070;
        else
        {
            *colon = 0;
            sscanf(colon + 1, "%d", &port);
        }
        printf("Adding shard server (%s:%d)\n", line, port);
        app->SetNameEntry(n, line, port);
        n++;
    }
    delete [] buf;
    return 0;
}
