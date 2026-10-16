#line 1 "C:/code-frontiers-gm/ui/utils/VICSInterface.cpp"
// VICSInterface.cpp -- STATION 0x002DF918 .. 0x002E1FF8: the customer-service
// connection.  A server picked at random out of eqahosts.txt on the memory card,
// DRDP to it, a version and an AES-encrypted login, then the CS client's
// protocol on message 0x0AA1/0x07B0 and the two CS windows.  STATION only.
// The asserts sit on their original lines (40 .. 658).
#include "station.h"
#include "vilist_t.h"

extern "C" void srand(unsigned int seed);   // not in the PS2 build's stdlib.h

// The DRDP message types this file names (SUPPORT's drdp.h names five).
#define MESSAGE_TYPE_CONNECTIONLESS 248
#define MESSAGE_TYPE_SYSTEM         249
#define MESSAGE_TYPE_GUARANTEED     251
#define MESSAGE_TYPE_BEST_EFFORT    252
#define MESSAGE_TYPE_DISCONNECT     253
#define MESSAGE_TYPE_CONNECTED      254
#define MESSAGE_TYPE_CONNECT        255

// The CS server's replies (the two names are its log's).
#define VINET_CLIENT_LOGIN_BAD            0x0729
#define VINET_CLIENT_LOGIN_NOT_SUBSCRIBED 0x072B

// 0x00362AB0: the AES-256 key the login's password goes under (main.cpp's, again).
static const unsigned char sKey[32] =
{
    0xEC, 0xCA, 0xBF, 0x71, 0xF4, 0xBF, 0x32, 0x6B, 0x57, 0x29, 0x72, 0x4D, 0xC1, 0xD7, 0xAA, 0xAC,
    0x53, 0x84, 0x37, 0xDA, 0x68, 0xE4, 0x3B, 0xB4, 0xA5, 0xE4, 0xA4, 0xE9, 0xD4, 0x89, 0x6D, 0x44,
};





// 0x002DF918
VICSInterface::VICSInterface(VIWndGenericRenderer *renderer)
    : m_port(0), m_connecting(0), m_loggedIn(0), m_windowsAdded(0), m_open(0),
      m_client(this), m_x4AB4(0), m_renderer(renderer)
{
    VIASSERT(renderer != NULL);
    memset(m_host, 0, sizeof(m_host));
    memset(m_user, 0, sizeof(m_user));
    memset(m_password, 0, sizeof(m_password));
    m_retries = 0;
}

// 0x002DF9E8
VICSInterface::~VICSInterface()
{
    printf("Deleting CSINterface\n");
    if (m_port)
        Disconnect();
    printf("Deleting CSINterface 1\n");
    m_port = 0;
    memset(m_host, 0, sizeof(m_host));
    memset(m_user, 0, sizeof(m_user));
    memset(m_password, 0, sizeof(m_password));
    printf("Deleting CSINterface 2\n");
    if (m_windowsAdded)
    {
        m_request.End();
        printf("Deleting CSINterface 3\n");
        m_inbox.End();
        printf("Deleting CSINterface 4\n");
        m_renderer->RemoveChild(&m_request);
        printf("Deleting CSINterface 5\n");
        m_renderer->RemoveChild(&m_inbox);
        printf("Deleting CSINterface 6\n");
    }
    m_renderer = 0;
    printf("Done deleting CSINterface\n");
}

// 0x002DFB60 (name ours; CLIENT has its own copy, 0x00C63650): the next word of
// buf from *pos into line, and the separators after it skipped.  0 at the end.
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

// 0x002DFC18: once per run.  A line is host:port, or a host and the default port.
int VICSInterface::LoadServerList()
{
    char line[256];
    char *buf = new char[8192];
    srand(VITimer::GetTime());
    static int sLoaded = 0;
    if (sLoaded)
        return 0;
    sLoaded = 1;
    if (VIMemCardStatus(0))
    {
        VIVect2I pos(320, 224);
        McErr(m_renderer, pos, 4);
        return -1;
    }
    VIMemCardPrepare(0);
    printf("!!Loading server list\n");
    int size = VIMemCardReadFile("eqahosts.txt", buf, 8192);
    if (size < 0)
    {
        delete [] buf;
        return -1;
    }
    int n = 0;
    int pos = 0;
    while (GetLine(buf, size, &pos, line))
    {
        n++;
        printf("!!Parsing line\n");
    }
    if (!n)
    {
        delete [] buf;
        return -1;
    }
    pos = 0;
    int port;
    while (GetLine(buf, size, &pos, line))
    {
        char *colon = strstr(line, ":");
        if (!colon)
        {
            port = 10070;
            continue;
        }
        *colon = 0;
        sscanf(colon + 1, "%d", &port);
        printf("Adding shard server (%s:%d)\n", line, port);
        VICSServer server(line, port);
        m_servers.Add(server);
    }
    delete [] buf;
    return 0;
}

// 0x002DFE30: the server after the one drawn (the walk steps before it tests),
// and the first when the draw is past the end.
VICSServer VICSInterface::SelectServer()
{
    int n = m_servers.Size();
    srand(VITimer::GetTime());
    int k = rand() % (n - 1);
    printf("chose server %i\n", k);
    int i = 0;
    int it = m_servers.Begin();
    while (it != m_servers.End())
    {
        it = m_servers.Next(it);
        if (i == k)
        {
            VICSServer &s = *(VICSServer *)it;
            printf("!!Selected server (%s, %i)\n", s.m_host, s.m_port);
            return s;
        }
        i++;
    }
    return *(VICSServer *)m_servers.Begin();
}














































// 0x002E0080.  The original's drdp_addr_t calls its octets `addr` (this assert's
// text); SUPPORT's drdp.h calls them m_ucIp, so the assert is spelled out.
void VICSInterface::Connect()
{
    drdp_addr_t statusServerAddress;
    int result = drdp_namelookup(m_host, &statusServerAddress);
    if (!(result == 0 && statusServerAddress.m_ucIp[0] != 0)) _VIAssert("result == 0 && statusServerAddress.addr[0] != 0", __FILE__, __LINE__);
    printf("1) drdp_namelookup(%s) result = %i\n", m_host, result);
    statusServerAddress.m_usPort = m_port;
    for (int i = 0; i < 64; i++)
    {
        drdp_addr_t local;
        memset(&local, 0, sizeof(local));
        local.m_usPort = i + 10070;
        result = drdp_create_client(&m_drdp, &local, VITimer::GetTime(), false, 0);
        printf("2) result = %i\n", result);
        if (result != -3) break;
    }
    VIASSERT(result == 0);
    unsigned long endpoint;
    drdp_address_to_endpoint(statusServerAddress.m_ucIp, statusServerAddress.m_usPort,
                             0xFFFE, &endpoint);
    result = connection_open(&m_conn, drdp_get_system_endpoint(m_drdp),
                             endpoint);
    VIASSERT(result == 0);
    if (m_x4AB4)
        connection_set_failure(m_conn, 300000, 300000, 300000);
    else
        connection_set_failure(m_conn, 1000, 30000, 60000);
    connection_set_simulated(m_conn, 0, 0, 0);
    SendVersion();
    SendLogin();
    m_x4AB8 = 0;
}

// 0x002E0268
void VICSInterface::Start(const char *host, int port, const char *user, const char *password)
{
    if (!host || port <= 0 || !user || !password)
        return;
    m_port = port;
    strncpy(m_host, host, sizeof(m_host));
    strncpy(m_user, user, sizeof(m_user));
    strncpy(m_password, password, sizeof(m_password));
    if (!m_windowsAdded)
    {
        m_request.SetVisible(false);
        m_request.SetAppPtr(m_renderer->GetAppPtr());
        m_renderer->AddChild(&m_request);
        m_request.Init(0);
        m_inbox.SetVisible(false);
        m_inbox.SetAppPtr(m_renderer->GetAppPtr());
        m_renderer->AddChild(&m_inbox);
        m_inbox.Init();
    }
    m_windowsAdded = 1;
    Connect();
    m_connecting = 1;
}

// 0x002E0370
int VICSInterface::IsActive()
{
    return m_connecting || m_loggedIn;
}













// 0x002E0398
void VICSInterface::OpenRequest(const char *user, const char *password)
{
    VIStation *client = (VIStation *)m_renderer->GetAppPtr();
    VIASSERT(client != NULL);
    LoadServerList();
    if (!m_loggedIn)
    {
        m_renderer->m_dialog->Open("Connecting to Customer Service...", 0);
        m_renderer->m_dialog->SetText("Connecting to the Customer Service Server.  Please wait.");
        m_open = 1;
        strncpy(m_user, user, sizeof(m_user));
        strncpy(m_password, password, sizeof(m_password));
        if (!m_connecting)
        {
            m_retries = 0;
            VICSServer server = SelectServer();
            Start(server.m_host, server.m_port, user, password);
        }
    } else {
        m_request.SetVisible(true);
        m_request.Refresh();
        client->m_xB40C = (int)&m_request;
        client->m_wm.SetFocus(&m_request, true);
    }
}
void VICSInterface::OpenInbox(const char *user, const char *password)   // 0x002E04D8
{
    VIStation *client = (VIStation *)m_renderer->GetAppPtr();
    VIASSERT(client != NULL);
    LoadServerList();
    if (!m_loggedIn)
    {
        m_renderer->m_dialog->Open("Connecting to Customer Service...", 0);
        m_renderer->m_dialog->SetText("Connecting to the Customer Service Server.  Please wait.");
        m_open = 2;
        strncpy(m_user, user, sizeof(m_user));
        strncpy(m_password, password, sizeof(m_password));
        if (!m_connecting)
        {
            m_retries = 0;
            VICSServer server = SelectServer();
            Start(server.m_host, server.m_port, user, password);
        }
    } else {
        m_inbox.SetVisible(true);
        m_inbox.Refresh();
        client->m_xB40C = (int)&m_inbox;
        client->m_wm.SetFocus(&m_inbox, true);
    }
}
void VICSInterface::EndCSRequestSession()   // 0x002E0618
{
    VIStation *client = (VIStation *)m_renderer->GetAppPtr();
    VIASSERT(client != NULL);
    printf("EndCSRequestSession\n");
    m_request.End();
    m_request.SetVisible(false);
    VIWnd *w = m_renderer;
    client->m_xB40C = (int)w;
    client->m_wm.SetFocus(w, true);
}

void VICSInterface::EndCSInboxSession()   // 0x002E06B0
{
    VIStation *client = (VIStation *)m_renderer->GetAppPtr();
    VIASSERT(client != NULL);
    printf("EndCSInboxSession\n");
    m_inbox.End();
    m_inbox.SetVisible(false);
    VIWnd *w = m_renderer;
    client->m_xB40C = (int)w;
    client->m_wm.SetFocus(w, true);
}

// 0x002E0748
void VICSInterface::Disconnect()
{
    if (m_conn)
        connection_close(m_conn);
    if (m_drdp)
        drdp_destroy(m_drdp);
    m_drdp = 0;
    m_conn = 0;
    m_loggedIn = 0;
    m_connecting = 0;
}

// 0x002E07A0: VIClientI's one virtual.
VIClientConn VICSInterface::GetConn(int type)
{
    return VIClientConn(m_conn, type);
}

// 0x002E07B8: message 0, the protocol version.
void VICSInterface::SendVersion()
{
    VIMemStream s;
    s.WriteU16(0);
    VINetSend(s << VIMemStreamRaw(18), GetConn(251));
}

// 0x002E0880: message 0x0995, the login.  The fields are ONE full expression:
// the opcode's temporary lives to its end, so both flag bytes share the slot
// after it, and only once it is freed does it merge with theirs and take
// GetConn's result.  Written as separate statements, the second flag reuses the
// merged slot instead (2 words wrong).
void VICSInterface::SendLogin()
{
    VIMemStream s;
    s << VIMemStreamU16(0x0995) << VIMemStreamBool(m_x4AB4) << VIGetGameCode() << m_user << VIMemStreamBool(true);
    symmetric_key skey;
    if (rijndael_setup(sKey, 32, 0, &skey) != CRYPT_ERROR)
        aes_ecb_encrypt_buffer((const unsigned char *)m_password, 32, m_encrypted, 32, &skey);
    s.Write(m_encrypted, 1, 32);
    VINetSend(s, GetConn(251));
}



// 0x002E09D8
void VICSInterface::Idle()
{
}

// 0x002E09E0
void VICSInterface::ReceivePackets()
{
    char buf[65536];
    int n;
    if (!m_drdp)
        return;
    while (!drdp_receive_packet(m_drdp, buf, sizeof(buf), &n, 0))
        drdp_process_packet(m_drdp, buf, n);
}






























// 0x002E0A68
void VICSInterface::ProcessMessages()
{
    message_t *msg;
    while (m_drdp && (msg = drdp_receive(m_drdp)) != 0)
    {
        printf("!!\tgot DRDP messages\n");
        message_get_src(msg);
        int type = message_get_type(msg);
        const void *data = message_get_data(msg);
        int size = message_get_size(msg);
        VIMemStream s(data, size);
        switch (type)
        {
        case MESSAGE_TYPE_CONNECT:
            printf("connected to VICSAssistBrain\n");
            connection_close(m_conn);
            m_conn = message_get_connection(msg);
            break;
        case MESSAGE_TYPE_CONNECTED:
            printf("MESSAGE_TYPE_CONNECTED ignored\n");
            break;
        case MESSAGE_TYPE_CONNECTIONLESS:
            VIALERT("unexpected MESSAGE_TYPE_CONNECTIONLESS");
            break;
        case MESSAGE_TYPE_DISCONNECT:
        {
            char text[1024];
            printf("!!Disconnected!\n");
            m_client.Reset();
            Disconnect();
            if (++m_retries < 4)
            {
                VICSServer server = SelectServer();
                Start(server.m_host, server.m_port, m_user, m_password);
            }
            else
            {
                switch (*(unsigned char *)message_get_data(msg))
                {
                case 1:
                case 15:
                    sprintf(text, "Server down for maintenance");
                    break;
                case 14:
                    sprintf(text, "The server is being updated, please try again later");
                    break;
                default:
                    sprintf(text, "Connection problem or server down for maintenance. (%u)",
                            *(unsigned char *)message_get_data(msg));
                    break;
                }
                EndCSInboxSession();
                EndCSRequestSession();
                m_renderer->m_dialog->Open("Customer Service", 0);
                m_renderer->m_dialog->SetText(text);
            }
            break;
        }









        case MESSAGE_TYPE_GUARANTEED:
            printf("!!Got a guaranteed message!\n");
            HandleMessage(data, size);
            break;
        case MESSAGE_TYPE_BEST_EFFORT:
            VIALERT("unexpected MESSAGE_TYPE_BEST_EFFORT");
            break;


        case MESSAGE_TYPE_SYSTEM:
            printf("MESSAGE_TYPE_SYSTEM ignored\n");
            break;
        default:
            VIALERT("unexpected state");
            printf("!!Unexpected state\n");
            break;
        }
        message_destroy(msg);
    }
}




























// 0x002E0D98: one guaranteed message from the CS server.
void VICSInterface::HandleMessage(const void *data, int size)
{
    VIMemStream s(data, size);
    unsigned short id;
    s.Read(&id, 2, 1);
    switch (id)
    {
    case 0x07B0:
        printf("CS control message\n");
        m_client.Recv07B0(s);
        break;
    case 0x07B1:
        printf("!!Login complete\n");
        m_connecting = 0;
        m_x4AB8 = 6;
        m_loggedIn = 1;
        m_renderer->m_dialog->Close();
        if (m_open == 1)
            OpenRequest(m_user, m_password);
        else if (m_open == 2)
            OpenInbox(m_user, m_password);
        m_open = 0;
        break;
    case 0x072A:
        printf("Server not ready, please try again later. (loginto)\n");
        m_renderer->m_dialog->Open("Customer Service", 0);
        m_renderer->m_dialog->SetText("The Customer Service server is being updated.  Please try again later.");
        Disconnect();
        break;
    case VINET_CLIENT_LOGIN_BAD:
        printf("VINET_CLIENT_LOGIN_BAD\n");
        m_renderer->m_dialog->Open("Customer Service", 0);
        m_renderer->m_dialog->SetText("Invalid login.");
        Disconnect();
        break;
    case VINET_CLIENT_LOGIN_NOT_SUBSCRIBED:
        printf("VINET_CLIENT_LOGIN_NOT_SUBSCRIBED\n");
        Disconnect();
        break;
    case 0x0778:
        printf("!!Waiting for old logout!!\n");
        break;
    case 0:
        {
            int version;
            s.Read(&version, 4, 1);
            if (version != 18)
            {
                printf("We are updating the game, please try again later (%i, %i).\n", version, 18);
                m_renderer->m_dialog->Open("Customer Service", 0);
                m_renderer->m_dialog->SetText("The Customer Service server is being updated.  Please try again later.");
                Disconnect();
            }
            else if (m_x4AB4)
                connection_set_failure(m_conn, 300000, 300000, 300000);
            else
                connection_set_failure(m_conn, 30000, 30000, 60000);
        }
        break;
    case 0x00EC:
    case 0x07A4:
        printf("You have logged in from another location. \n");
        EndCSInboxSession();
        EndCSRequestSession();
        m_renderer->m_dialog->Open("Customer Service", 0);
        m_renderer->m_dialog->SetText("You have logged in from another location.  You are only allowed to be log into your account one location at a time.");
        Disconnect();
        break;
    case 0x0700:
        break;
    default:
        printf("unexpected MessageId: %04x", id);
        VIALERT("unexpected MessageId: %04x", id);
        break;
    }
}

// 0x002E10F8
void VICSInterface::Update()
{
    if (m_drdp)
    {
        drdp_set_time(m_drdp, VITimer::GetTime());
        drdp_flush(m_drdp);
    }
    Idle();
    ReceivePackets();
    ProcessMessages();
}

// 0x002E1160 .. 0x002E1FF8: the template copies (the item header's, then
// VIList<VICSServer>'s), the type_info functions and GetClient.
