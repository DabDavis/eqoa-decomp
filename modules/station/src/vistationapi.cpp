// vistationapi.cpp (name ours) -- STATION 0x002E60A8 .. 0x002E9CD8: the Station
// API.  One VITCPConnection to an account server chosen at random from the
// list, requests that refuse to overlap ("Please wait for the previous request
// to complete."), and a dispatcher that swaps each reply to host order in
// place and hands it to the listener.  Passwords go out DES-encrypted under a
// key made from a fixed string; billing data under AES with the 32-byte
// session key the server sends first (type 4), itself DES-encrypted.
#include "station.h"
#include "vilist_t.h"

// The key every DES-wrapped field is made with.
#define sDesPassword "LPSS-V1.0$@Z>.ADJ"

// Each message on the wire: big-endian length (all of it) and type.
struct VIStationHeader
{
    int m_length;   // +0x00
    int m_type;     // +0x04
};

// 0x002E60A8.
void NetToHostString16(unsigned short *s)
{
    if (s)
        while (*s)
        {
            unsigned int c = *s;
            *s = (c << 8) | (c >> 8);
            s++;
        }
}

// 0x002E60E8.
VIStationAPI::VIStationAPI()
{
    m_listener = 0;
    m_connection.SetListener(this);
    m_haveKey = 0;
    m_busy = 0;
    m_requestTime = 0;
    m_threadRun = 0;
    m_thread = -1;
    memset(m_key, 0, 32);
    srand(VITimer::GetTime());
}

// 0x002E6180.
VIStationAPI::~VIStationAPI()
{
    printf("VIStationAPI::~VIStationAPI\n");
    m_connection.Close();
    printf("End VIStationAPI::~VIStationAPI\n");
}

// 0x002E6210.
void VIStationAPI::AddServer(const char *host, int port)
{
    if (!host || port <= 0)
        return;
    VIStationServer server;
    memset(&server, 0, sizeof(server));
    strncpy(server.m_host, host, 255);
    server.m_port = port;
    printf("VIStationAPI::AddServer(%s, %i)\n", host, port);
    m_servers.Add(server);
}

// 0x002E6298.
void VIStationAPI::BeginRequest()
{
    m_busy = 1;
    m_requestTime = VITimer::GetTime();
}

// 0x002E62C8.
int VIStationAPI::Initialize()
{
    printf("!!VIStationAPI::Initialize()\n");
    m_connection.Initialize();
    return 0;
}

// 0x002E6300.  The case order is the source's (the bodies are in it).
void VIStationAPI::OnMessage(char *data, int size, int connection)
{
    if (!data || size < 4)
        return;
    int type = VISwap32(*(unsigned int *)data);
    printf("!!Message, type = %i\n", type);
    m_busy = 0;
    switch (type)
    {
    case 37:
        OnReply37(data, size);
        break;
    case 41:
        OnReply41(data, size);
        break;
    case 43:
        OnReply43(data, size);
        break;
    case 45:
        OnReply45(data, size);
        break;
    case 49:
        OnReply49(data, size);
        break;
    case 47:
        OnReply47(data, size);
        break;
    case 51:
        printf("GetSubscriptionInfoReply\n");
        OnReply51(data, size);
        break;
    case 53:
        OnReply53(data, size);
        break;
    case 55:
        OnReply55(data, size);
        break;
    case 63:
        OnReply63(data, size);
        break;
    case 4:
        OnSessionKey(data, size);
        break;
    case 65:
        OnReply65(data, size);
        break;
    case 67:
        OnReply67(data, size);
        break;
    case 69:
        OnReply69(data, size);
        break;
    case 71:
        OnReply71(data, size);
        break;
    case 73:
        OnReply73(data, size);
        break;
    case 75:
        OnReply75(data, size);
        break;
    case 77:
        OnReply77(data, size);
        break;
    case 84:
        OnReply84(data, size);
        break;
    }
}

// The replies.  Each is swapped to host order in place, then handed on.
struct VIStationReply12
{
    VIStationHeader m_header;
    int m_x08;
};
struct VIStationReply16
{
    VIStationHeader m_header;
    int m_x08;
    int m_x0C;
};

// 0x002E6580.
void VIStationAPI::OnReply67(char *msg, int size)
{
    bool ok = size == sizeof(VIStationReply12);
    if (!ok)
        return;
    if (m_listener)
        m_listener->OnReply67(((VIStationReply12 *)msg)->m_x08);
}

// 0x002E65C0.
void VIStationAPI::OnReply65(char *msg, int size)
{
    bool ok = size == sizeof(VIStationReply12);
    if (!ok)
        return;
    if (m_listener)
        m_listener->OnReply65(((VIStationReply12 *)msg)->m_x08);
}

// 0x002E6600.  The session key, DES-encrypted.
struct VIStationSessionKey
{
    int           m_length;
    unsigned char m_key[32];
};
void VIStationAPI::OnSessionKey(char *msg, int size)
{
    bool ok = size == sizeof(VIStationSessionKey) + 0;
    if (!ok)
        return;
    des_soe_set_key(sDesPassword);
    des_soe_decrypt((des_cblock *)((VIStationSessionKey *)msg)->m_key, (des_cblock *)m_key, 32);
    m_haveKey = 1;
}

// 0x002E6658.
struct VIStationReply63
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_x0C;
    int  m_count;
};
void VIStationAPI::OnReply63(char *msg, int size)
{
    VIStationReply63 *r = (VIStationReply63 *)msg;
    bool ok = size >= sizeof(VIStationReply63);
    if (!ok)
        return;
    r->m_x08 = VISwap32(r->m_x08);
    r->m_x0C = VISwap32(r->m_x0C);
    r->m_count = VISwap32(r->m_count);
    const char *plans = msg + sizeof(VIStationReply63);
    if (m_listener)
        m_listener->OnGetSubscriptionList(r->m_x08, r->m_x0C, r->m_count, plans);
}

// 0x002E6728.
struct VIStationReply45
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_x0C;
    char m_x10[108];
};
void VIStationAPI::OnReply45(char *msg, int size)
{
    VIStationReply45 *r = (VIStationReply45 *)msg;
    bool ok = size == sizeof(VIStationReply45);
    if (!ok)
        return;
    r->m_x08 = VISwap32(r->m_x08);
    r->m_x0C = VISwap32(r->m_x0C);
    if (m_listener)
        m_listener->OnReply45(r->m_x08, r->m_x0C);
}

// 0x002E67C8.
struct VIStationReply55
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_x0C;
    char m_x10[16];
    char m_x20[32];
    int  m_x40;
    int  m_x44;
    int  m_x48;
};
void VIStationAPI::OnReply55(char *msg, int size)
{
    VIStationReply55 *r = (VIStationReply55 *)msg;
    bool ok = size == sizeof(VIStationReply55);
    if (!ok)
        return;
    r->m_x08 = VISwap32(r->m_x08);
    r->m_x0C = VISwap32(r->m_x0C);
    r->m_x40 = VISwap32(r->m_x40);
    r->m_x44 = VISwap32(r->m_x44);
    r->m_x48 = VISwap32(r->m_x48);
    if (m_listener)
        m_listener->OnReply55(r->m_x08, r->m_x0C, r->m_x10, r->m_x20, r->m_x40, r->m_x44);
}

// 0x002E6920.
void VIStationAPI::OnReply53(char *msg, int size)
{
    VIStationReply16 *r = (VIStationReply16 *)msg;
    bool ok = size == sizeof(VIStationReply16);
    if (!ok)
        return;
    r->m_x08 = VISwap32(r->m_x08);
    r->m_x0C = VISwap32(r->m_x0C);
    if (m_listener)
        m_listener->OnCancelAccount(r->m_x08, r->m_x0C);
}

// 0x002E69C0.
struct VIStationReply51
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_x0C;
    int  m_x10;
    int  m_x14;
    char m_sku[32];
};
void VIStationAPI::OnReply51(char *msg, int size)
{
    VIStationReply51 *r = (VIStationReply51 *)msg;
    bool ok = size == sizeof(VIStationReply51);
    if (!ok)
        return;
    r->m_x08 = VISwap32(r->m_x08);
    r->m_x0C = VISwap32(r->m_x0C);
    r->m_x10 = VISwap32(r->m_x10);
    r->m_x14 = VISwap32(r->m_x14);
    if (m_listener)
        m_listener->OnGetSubscriptionInfo(r->m_x08, r->m_x0C, r->m_x10, r->m_x14, r->m_sku);
}

// 0x002E6AC8.
void VIStationAPI::OnReply47(char *msg, int size)
{
    VIStationReply16 *r = (VIStationReply16 *)msg;
    bool ok = size == sizeof(VIStationReply16);
    if (!ok)
        return;
    r->m_x08 = VISwap32(r->m_x08);
    r->m_x0C = VISwap32(r->m_x0C);
    if (m_listener)
        m_listener->OnReply47(r->m_x08, r->m_x0C);
}

// 0x002E6B68.
void VIStationAPI::OnReply49(char *msg, int size)
{
    VIStationReply16 *r = (VIStationReply16 *)msg;
    bool ok = size == sizeof(VIStationReply16);
    if (!ok)
        return;
    r->m_x08 = VISwap32(r->m_x08);
    r->m_x0C = VISwap32(r->m_x0C);
    if (m_listener)
        m_listener->OnConsumeAccountKey(r->m_x08, r->m_x0C);
}

// 0x002E6C08.
struct VIStationReply41
{
    VIStationHeader m_header;
    bool m_success;
    int  m_code;
    char m_text[256];
};
void VIStationAPI::OnReply41(char *msg, int size)
{
    VIStationReply41 *r = (VIStationReply41 *)msg;
    bool ok = size == sizeof(VIStationReply41);
    if (!ok)
        return;
    r->m_header.m_length = VISwap32(r->m_header.m_length);
    r->m_code = VISwap32(r->m_code);
    r->m_success = VISwap32(r->m_success) != 0;
    printf("success = %i\n", r->m_success);
    if (m_listener)
        m_listener->OnReply41(r->m_success, r->m_code, r->m_text);
}

// 0x002E6D00.
struct VIStationReply43
{
    VIStationHeader m_header;
    bool m_success;
    char m_text[256];
};
void VIStationAPI::OnReply43(char *msg, int size)
{
    VIStationReply43 *r = (VIStationReply43 *)msg;
    bool ok = size == sizeof(VIStationReply43);
    if (!ok)
        return;
    r->m_header.m_length = VISwap32(r->m_header.m_length);
    r->m_success = VISwap32(r->m_success) != 0;
    if (m_listener)
        m_listener->OnChangePassword(r->m_success, r->m_text);
}

// 0x002E6DA0.
struct VIStationReply37
{
    VIStationHeader m_header;
    int  m_x08;
    char m_x0C[64];
    int  m_x4C;
    int  m_x50;
    int  m_x54;
    int  m_x58;
    char m_x5C[64];
    int  m_x9C;
    int  m_xA0;
};
void VIStationAPI::OnReply37(char *msg, int size)
{
    VIStationReply37 *r = (VIStationReply37 *)msg;
    bool ok = size == sizeof(VIStationReply37);
    if (!ok)
        return;
    r->m_header.m_length = VISwap32(r->m_header.m_length);
    r->m_x08 = VISwap32(r->m_x08);
    r->m_x4C = VISwap32(r->m_x4C);
    r->m_x50 = VISwap32(r->m_x50);
    r->m_x54 = VISwap32(r->m_x54);
    r->m_x58 = VISwap32(r->m_x58);
    r->m_x9C = VISwap32(r->m_x9C);
    r->m_xA0 = VISwap32(r->m_xA0);
    if (m_listener)
        m_listener->OnValidateLogin(r->m_x08, r->m_x4C, r->m_x50, r->m_x54, r->m_x58,
                                    r->m_x9C, r->m_xA0, r->m_x5C, r->m_x0C);
}

// 0x002E6FF0.  NetToHostString16 again; nothing calls it.
void HostToNetString16(unsigned short *s)
{
    if (s)
        while (*s)
        {
            unsigned int c = *s;
            *s = (c << 8) | (c >> 8);
            s++;
        }
}

// The replies with a wide title and a counted tail.
struct VIStationReplyList
{
    VIStationHeader m_header;
    int            m_x08;
    unsigned short m_title[256];   // +0x00C
    int            m_count;        // +0x20C
};

// 0x002E7030.
void VIStationAPI::OnReply69(char *msg, int size)
{
    VIStationReplyList *r = (VIStationReplyList *)msg;
    bool ok = size >= sizeof(VIStationReplyList);
    if (!ok)
        return;
    r->m_header.m_length = VISwap32(r->m_header.m_length);
    r->m_x08 = VISwap32(r->m_x08);
    NetToHostString16(r->m_title);
    r->m_count = VISwap32(r->m_count);
    if (size != sizeof(VIStationReplyList) + r->m_count * 2)
        return;
    unsigned short *text = (unsigned short *)(r + 1);
    NetToHostString16(text);
    if (m_listener)
        m_listener->OnReply69(r->m_x08, r->m_title, text);
}

// 0x002E7150.
void VIStationAPI::OnReply71(char *msg, int size)
{
    VIStationReplyList *r = (VIStationReplyList *)msg;
    bool ok = size >= sizeof(VIStationReplyList);
    if (!ok)
        return;
    r->m_header.m_length = VISwap32(r->m_header.m_length);
    r->m_x08 = VISwap32(r->m_x08);
    NetToHostString16(r->m_title);
    r->m_count = VISwap32(r->m_count);
    printf("arraysize = %d\n", r->m_count);
    if (size != sizeof(VIStationReplyList) + r->m_count * sizeof(VIStationRecord132))
        return;
    msg += sizeof(VIStationReplyList);
    VIStationRecord132 *recs = (VIStationRecord132 *)msg;
    for (int i = 0; i < r->m_count; i++)
        NetToHostString16(recs[i].m_name);
    if (m_listener)
        m_listener->OnReply71(r->m_x08, r->m_title, r->m_count, recs);
}

// 0x002E72B0.
void VIStationAPI::OnReply73(char *msg, int size)
{
    VIStationReplyList *r = (VIStationReplyList *)msg;
    bool ok = size >= sizeof(VIStationReplyList);
    if (!ok)
        return;
    r->m_header.m_length = VISwap32(r->m_header.m_length);
    r->m_x08 = VISwap32(r->m_x08);
    NetToHostString16(r->m_title);
    r->m_count = VISwap32(r->m_count);
    if (size != sizeof(VIStationReplyList) + r->m_count * sizeof(VIStationRecord142))
        return;
    msg += sizeof(VIStationReplyList);
    VIStationRecord142 *recs = (VIStationRecord142 *)msg;
    for (int i = 0; i < r->m_count; i++)
    {
        NetToHostString16(recs[i].m_name);
        NetToHostString16(recs[i].m_desc);
    }
    if (m_listener)
        m_listener->OnReply73(r->m_x08, r->m_title, r->m_count, recs);
}

// 0x002E7410.
void VIStationAPI::OnReply75(char *msg, int size)
{
    VIStationReplyList *r = (VIStationReplyList *)msg;
    bool ok = size >= sizeof(VIStationReplyList);
    if (!ok)
        return;
    r->m_header.m_length = VISwap32(r->m_header.m_length);
    r->m_x08 = VISwap32(r->m_x08);
    NetToHostString16(r->m_title);
    r->m_count = VISwap32(r->m_count);
    if (size != sizeof(VIStationReplyList) + r->m_count * sizeof(VIStationRecord132))
        return;
    msg += sizeof(VIStationReplyList);
    VIStationRecord132 *recs = (VIStationRecord132 *)msg;
    for (int i = 0; i < r->m_count; i++)
    {
        recs[i].m_id = VISwap32(recs[i].m_id);
        NetToHostString16(recs[i].m_name);
        printf("Got payment method ID: %d\n", recs[i].m_id);
    }
    if (m_listener)
        m_listener->OnReply75(r->m_x08, r->m_title, r->m_count, recs);
}

// 0x002E75B8.
struct VIStationReply77
{
    VIStationHeader m_header;
    int            m_x08;
    unsigned short m_title[256];   // +0x00C
    int            m_count;        // +0x20C
    int            m_textLength;   // +0x210
};
void VIStationAPI::OnReply77(char *msg, int size)
{
    VIStationReply77 *r = (VIStationReply77 *)msg;
    printf("HandleGetGCPaymentMethodFieldsReply()\n");
    bool ok = size >= sizeof(VIStationReply77);
    if (!ok)
    {
        printf("Fail1 (length %d, sizeof %d\n", size, sizeof(VIStationReply77));
        return;
    }
    r->m_header.m_length = VISwap32(r->m_header.m_length);
    r->m_x08 = VISwap32(r->m_x08);
    NetToHostString16(r->m_title);
    r->m_count = VISwap32(r->m_count);
    r->m_textLength = VISwap32(r->m_textLength);
    int expect = sizeof(VIStationReply77) + r->m_count * sizeof(VIStationRecord204) + r->m_textLength * 2;
    if (size != expect)
    {
        printf("Fail2 (length %d, sizeof %d\n", size, expect);
        return;
    }
    msg += sizeof(VIStationReply77);
    VIStationRecord204 *recs = (VIStationRecord204 *)msg;
    for (int i = 0; i < r->m_count; i++)
    {
        NetToHostString16(recs[i].m_name);
        recs[i].m_a = VISwap32(recs[i].m_a);
        recs[i].m_b = VISwap32(recs[i].m_b);
        recs[i].m_c = VISwap32(recs[i].m_c);
    }
    unsigned short *text = (unsigned short *)((char *)r + sizeof(VIStationReply77) + r->m_count * sizeof(VIStationRecord204));
    NetToHostString16(text);
    if (m_listener)
        m_listener->OnGetGCPaymentMethodFields(r->m_x08, r->m_title, text, r->m_count, recs);
}

// 0x002E7830.
struct VIStationReply84
{
    VIStationHeader m_header;
    int            m_x08;
    unsigned short m_title[256];   // +0x00C
    char           m_x20C[64];     // +0x20C
    unsigned short m_secret[64];   // +0x24C  DES-encrypted
    int            m_count;        // +0x2CC
};
void VIStationAPI::OnReply84(char *msg, int size)
{
    VIStationReply84 *r = (VIStationReply84 *)msg;
    bool ok = size >= sizeof(VIStationReply84);
    if (!ok)
        return;
    unsigned short *enc = r->m_secret;
    unsigned short secret[64];
    memcpy(secret, enc, sizeof(secret));
    des_soe_set_key(sDesPassword);
    des_soe_decrypt((des_cblock *)secret, (des_cblock *)enc, 128);
    r->m_header.m_length = VISwap32(r->m_header.m_length);
    r->m_x08 = VISwap32(r->m_x08);
    NetToHostString16(r->m_title);
    NetToHostString16(enc);
    r->m_count = VISwap32(r->m_count);
    if (size != sizeof(VIStationReply84) + r->m_count * 2)
        return;
    unsigned short *text = (unsigned short *)(r + 1);
    NetToHostString16(text);
    if (m_listener)
        m_listener->OnReply84(r->m_x08, r->m_title, r->m_x20C, enc, text);
}

// 0x002E7A28.  Connects, pings a minute after connecting, times a request out
// after 30 s, and pumps the connection.
void VIStationAPI::Process()
{
    char text[1024];
    if (m_connectResult == -3 || m_connectResult == -2)
        return;
    Connect();
    long now = VITimer::GetTime();
    if (now - m_connectTime > 60000)
    {
        printf("!!PINGING\n");
        Ping();
    }
    if (m_busy && now - m_requestTime > 30000 && m_listener)
    {
        sprintf(text, "Request timed out, please try again.  If the problem persists, please ensure that your network cables are connected.", 0);
        m_listener->OnError(0, text);
        m_busy = 0;
    }
    int r = m_connection.Process();
    if (r < 0 && m_listener)
    {
        sprintf(text, "Connection to server lost.  Please try again.", r);
        m_listener->OnError(r, text);
    }
}

// 0x002E7B58 (type 36).
struct VIStationRequest36
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_x0C;            // 2
    char m_name[32];
    char m_password[32];   // DES
    int  m_x50;            // 1
    char m_gameCode[64];
};
int VIStationAPI::ValidateLogin(const char *name, const char *password)
{
    if (!name || !password)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest36 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(36);
    req.m_x0C = VISwap32(2);
    strncpy(req.m_name, name, 32);
    des_soe_set_key(sDesPassword);
    des_soe_encrypt((des_cblock *)password, (des_cblock *)req.m_password, 32);
    req.m_x50 = VISwap32(1);
    strncpy(req.m_gameCode, VIGetGameCode(), 64);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E7C78.  One of the servers, at random.
VIStationServer VIStationAPI::ChooseServer()
{
    VIStationServer server;
    memset(&server, 0, sizeof(server));
    int n = m_servers.Size();
    if (n > 0)
    {
        int pick = rand() % n;
        printf("chose station server %i\n", pick);
        int k = 0;
        for (int i = m_servers.Begin(); i != m_servers.End(); i = m_servers.Next(i), k++)
        {
            if (k == pick)
            {
                server = ((VIList<VIStationServer>::Node *)i)->m_data;
                break;
            }
        }
    }
    return server;
}

// 0x002E7E88.
static void ConnectThreadEntry(void *api)
{
    if (api)
        ((VIStationAPI *)api)->ConnectThread();
}

static int sThreadActive;   // 0x00350374
static int sRecursion;      // 0x00350378
static char sThreadStack[8192];

// 0x002E7EA8.  Keeps the screen alive while Connect blocks.
void VIStationAPI::ConnectThread()
{
    sThreadActive = 1;
    while (m_threadRun)
    {
        m_listener->OnIdle();
        RotateThreadReadyQueue(1);
        RotateThreadReadyQueue(2);
    }
    sThreadActive = 0;
}

// 0x002E7F20.
void VIStationAPI::StartConnectThread()
{
    ThreadParam param;
    ChangeThreadPriority(GetThreadId(), 2);
    param.stackSize = 8192;
    param.gpReg = _gp;
    param.entry = ConnectThreadEntry;
    param.stack = sThreadStack;
    param.initPriority = 2;
    m_thread = CreateThread(&param);
    StartThread(m_thread, this);
}

// 0x002E7FA0.  1 when connected.  Each way out has its own decrement.
int VIStationAPI::Connect()
{
    if (sRecursion > 0)
    {
        printf("recursion count = %i\n", sRecursion);
        return 0;
    }
    sRecursion++;
    m_connectResult = 0;
    m_connectTime = VITimer::GetTime();
    if (m_connection.IsConnected())
    {
        sRecursion--;
        return 1;
    }
    m_threadRun = 1;
    if (m_listener)
        m_listener->OnStatus("Connecting to server...");
    StartConnectThread();
    VIStationServer server = ChooseServer();
    m_connectResult = m_connection.Connect(server.m_host, server.m_port);
    m_threadRun = 0;
    TerminateThread(m_thread);
    DeleteThread(m_thread);
    m_threadRun = 0;
    if (m_connection.IsConnected())
    {
        m_listener->OnConnected();
        sRecursion--;
        return 1;
    }
    if (m_listener)
    {
        VINetStatus net;
        net.Init();
        if (net.GetState())
            printf("Network disconnected\n");
        else if (m_connectResult < 0)
            m_listener->OnError(0, "Could not connect to the server. Please verify that your network connection is intact.  If the problem persists, return to network setup and try again.");
    }
    m_haveKey = 0;
    sRecursion--;
    return 0;
}

// 0x002E8158 (type 56).
struct VIStationRequest56
{
    VIStationHeader m_header;
    int  m_x08;
    char m_name[32];        // +0x00C
    char m_password[32];    // +0x02C  DES
    char m_x04C[64];        // +0x04C
    char m_x08C[16];        // +0x08C
    char m_x09C[64];        // +0x09C
    char m_x0DC[3];         // +0x0DC
    char m_x0DF[29];
    char m_x0FC[32];        // +0x0FC
    char m_x11C[16];        // +0x11C
    char m_x12C[16];        // +0x12C
    char m_x13C[16];        // +0x13C
    char m_x14C[16];        // +0x14C
    char m_x15C[128];       // +0x15C
};
int VIStationAPI::CreateAccount(const VIStationAccount *a)
{
    if (!a)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest56 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(56);
    strncpy(req.m_name, a->m_name, 32);
    des_soe_set_key(sDesPassword);
    des_soe_encrypt((des_cblock *)a->m_password, (des_cblock *)req.m_password, 32);
    strncpy(req.m_x12C, a->m_x120, 16);
    strncpy(req.m_x11C, a->m_x130, 16);
    strncpy(req.m_x13C, a->m_x140, 16);
    strncpy(req.m_x0DC, a->m_x0E0, 3);
    req.m_x0DC[2] = 0;
    strncpy(req.m_x15C, a->m_x160, 128);
    strncpy(req.m_x04C, a->m_x040, 64);
    strncpy(req.m_x09C, a->m_x0A0, 64);
    strncpy(req.m_x08C, a->m_x080, 16);
    strncpy(req.m_x14C, a->m_x150, 16);
    strncpy(req.m_x0FC, a->m_x100, 32);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E82E0 (type 42).
struct VIStationRequest42
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_id;
    char m_old[32];   // DES
    char m_new[32];   // DES
};
int VIStationAPI::ChangePassword(int id, const char *oldpw, const char *newpw)
{
    if (id < 0 || !oldpw || !newpw)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest42 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(42);
    req.m_id = VISwap32(id);
    des_soe_set_key(sDesPassword);
    des_soe_encrypt((des_cblock *)oldpw, (des_cblock *)req.m_old, 32);
    des_soe_encrypt((des_cblock *)newpw, (des_cblock *)req.m_new, 32);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// An id and two strings (types 44, 48, 54).
struct VIStationRequestIdAB
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_id;
    char m_a[16];
    char m_b[32];
};

// 0x002E8418 (type 44).
int VIStationAPI::Request44(int id, const char *a, const char *b)
{
    if (id < 0 || !a || !b)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequestIdAB req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(44);
    req.m_id = VISwap32(id);
    strncpy(req.m_a, a, 16);
    strncpy(req.m_b, b, 32);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E8540 (type 48).
int VIStationAPI::Request48(int id, const char *a, const char *b)
{
    if (id < 0 || !a || !b)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequestIdAB req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(48);
    req.m_id = VISwap32(id);
    strncpy(req.m_a, a, 16);
    strncpy(req.m_b, b, 32);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E8668 (type 46).  "EQOA Beta" is sent as a real SKU.
struct VIStationRequest46
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_id;           // +0x00C
    char m_sku[64];      // +0x010
    char m_x050[128];    // +0x050
    char m_x0D0[64];     // +0x0D0
    char m_x110[32];     // +0x110
    char m_x130[64];     // +0x130
    char m_x170[32];     // +0x170
    char m_x190[32];     // +0x190
    char m_x1B0[32];     // +0x1B0
    char m_x1D0[16];     // +0x1D0
    char m_x1E0[16];     // +0x1E0
    char m_x1F0[32];     // +0x1F0  AES
    char m_x210[32];
    char m_x230[16];     // +0x230
};
int VIStationAPI::Request46(int id, VIStationBilling b, const char *sku)
{
    if (id < 0 || !sku)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest46 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(46);
    req.m_id = VISwap32(id);
    if (!strcmp(sku, "EQOA Beta"))
        strncpy(req.m_sku, "EQ1RE-SU-SU0402-EQTIME-01MON", 64);
    else
        strncpy(req.m_sku, sku, 64);
    symmetric_key skey;
    if (rijndael_setup(m_key, 32, 0, &skey) == CRYPT_ERROR && m_listener)
        m_listener->OnError(0, "Unable to initialize encryption.");
    aes_ecb_encrypt_buffer((unsigned char *)b.m_x1A0, 32, (unsigned char *)req.m_x1F0, 32, &skey);
    strncpy(req.m_x050, b.m_x000, 127);
    strncpy(req.m_x230, b.m_x1E0, 15);
    strncpy(req.m_x0D0, b.m_x080, 63);
    strncpy(req.m_x110, b.m_x0C0, 31);
    strncpy(req.m_x1E0, b.m_x190, 15);
    strncpy(req.m_x1D0, b.m_x180, 15);
    strncpy(req.m_x170, b.m_x120, 31);
    strncpy(req.m_x190, b.m_x140, 31);
    strncpy(req.m_x1B0, b.m_x160, 31);
    strncpy(req.m_x130, b.m_x0E0, 63);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E8978 (type 66).  Five AES-encrypted fields.
struct VIStationRequest66
{
    VIStationHeader m_header;
    int  m_x08;
    char m_a[32];    // +0x00C
    char m_b[32];    // +0x02C
    char m_c[32];    // +0x04C
    char m_d[32];    // +0x06C
    char m_e[128];   // +0x08C
};
int VIStationAPI::Request66(const char *a, const char *b, const char *c, const char *d,
                            const char *e)
{
    if (!a || !b || !c || !d)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    int err = CRYPT_ERROR;
    VIStationRequest66 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(66);
    symmetric_key skey;
    if (rijndael_setup(m_key, 32, 0, &skey) == err && m_listener)
        m_listener->OnError(0, "Unable to initialize encryption.");
    aes_ecb_encrypt_buffer((const unsigned char *)a, 32, (unsigned char *)req.m_a, 32, &skey);
    aes_ecb_encrypt_buffer((const unsigned char *)b, 32, (unsigned char *)req.m_b, 32, &skey);
    aes_ecb_encrypt_buffer((const unsigned char *)c, 32, (unsigned char *)req.m_c, 32, &skey);
    aes_ecb_encrypt_buffer((const unsigned char *)d, 32, (unsigned char *)req.m_d, 32, &skey);
    aes_ecb_encrypt_buffer((const unsigned char *)e, 128, (unsigned char *)req.m_e, 128, &skey);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// An id and one string (types 50, 52, 62).
struct VIStationRequestIdA
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_id;
    char m_a[16];
};

// 0x002E8B50 (type 50).
int VIStationAPI::Request50(int id, const char *a)
{
    if (id < 0 || !a)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequestIdA req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(50);
    req.m_id = VISwap32(id);
    strncpy(req.m_a, a, 16);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E8C58 (type 52).
int VIStationAPI::Request52(int id, const char *a)
{
    if (id < 0 || !a)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequestIdA req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(52);
    req.m_id = VISwap32(id);
    strncpy(req.m_a, a, 16);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E8D60 (type 54).
struct VIStationRequest54
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_id;
    char m_a[16];
    char m_b[64];
};
int VIStationAPI::Request54(int id, const char *a, const char *b)
{
    if (id < 0 || !a || !b)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest54 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(54);
    req.m_id = VISwap32(id);
    strncpy(req.m_a, a, 16);
    strncpy(req.m_b, b, 32);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E8E88 (type 62).
struct VIStationRequest62
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_id;
    char m_a[32];
};
int VIStationAPI::Request62(int id, const char *a)
{
    if (id < 0 || !a)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest62 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(62);
    req.m_id = VISwap32(id);
    strncpy(req.m_a, a, 16);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E8F90 (type 64).
struct VIStationRequest64
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_id;
    char m_a[32];    // +0x010, a 3-byte code
    char m_b[32];    // +0x030
    char m_c[128];   // +0x050
};
int VIStationAPI::Request64(int id, const char *a, const char *b, const char *c)
{
    if (id < 0 || !a || !b)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest64 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(64);
    req.m_id = VISwap32(id);
    strncpy(req.m_a, a, 3);
    req.m_a[2] = 0;
    strncpy(req.m_b, b, 32);
    strncpy(req.m_c, c, 128);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E90D8 (type 68).
struct VIStationRequest68
{
    VIStationHeader m_header;
    int  m_x08;
    char m_a[4];
};
int VIStationAPI::Request68(const char *a)
{
    if (!a)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest68 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(68);
    strncpy(req.m_a, a, 3);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E91A0 (type 70).
struct VIStationRequest70
{
    VIStationHeader m_header;
    int  m_x08;
    char m_a[16];
    char m_b[4];
};
int VIStationAPI::Request70(const char *a, const char *b)
{
    if (!a || !b)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest70 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(70);
    strncpy(req.m_a, a, 16);
    strncpy(req.m_b, b, 3);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E9290 (type 72).
struct VIStationRequest72
{
    VIStationHeader m_header;
    int  m_x08;
    char m_a[16];
    char m_b[4];
    char m_c[4];
};
int VIStationAPI::Request72(const char *a, const char *b, const char *c)
{
    if (!a || !b || !c)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest72 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(72);
    strncpy(req.m_a, a, 16);
    strncpy(req.m_b, b, 3);
    strncpy(req.m_c, c, 3);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E93A0 (type 74).
struct VIStationRequest74
{
    VIStationHeader m_header;
    int  m_x08;
    char m_a[16];
    char m_b[4];
    char m_c[4];
    char m_d[4];
};
int VIStationAPI::Request74(const char *a, const char *b, const char *c, const char *d)
{
    if (!a || !b || !c || !d)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest74 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(74);
    strncpy(req.m_a, a, 16);
    strncpy(req.m_b, b, 3);
    strncpy(req.m_c, c, 3);
    strncpy(req.m_d, d, 4);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E94D8 (type 76).
struct VIStationRequest76
{
    VIStationHeader m_header;
    int  m_x08;
    char m_a[16];
    char m_b[4];
    int  m_c;
};
int VIStationAPI::Request76(const char *a, const char *b, int c)
{
    if (!a || !b)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest76 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req));
    req.m_header.m_type = VISwap32(76);
    strncpy(req.m_a, a, 16);
    strncpy(req.m_b, b, 3);
    req.m_c = VISwap32(c);
    m_connection.Send(&req, sizeof(req));
    BeginRequest();
    return 0;
}

// 0x002E95F8 (type 83).  The request, then each pair DES-encrypted as its own
// 128-byte send.
struct VIStationRequest83
{
    VIStationHeader m_header;
    int  m_x08;
    int  m_a;        // +0x0C
    int  m_b;        // +0x10
    char m_c[64];    // +0x14
    char m_d[4];     // +0x54
    char m_e[4];     // +0x58
    char m_f[4];     // +0x5C
    int  m_g;        // +0x60
    int  m_count;    // +0x64
};
int VIStationAPI::Request83(int a, int b, const char *c, const char *d, const char *e,
                            const char *f, int g, int count, const VIStationPair *pairs)
{
    if (!a)
        return 1;
    if (!c)
        return 1;
    if (!d)
        return 1;
    if (!e)
        return 1;
    if (!f)
        return 1;
    if (count && !pairs)
        return 1;
    if (!Connect())
        return 1;
    if (m_busy)
    {
        if (m_listener)
            m_listener->OnError(0, "Please wait for the previous request to complete.");
        return 0;
    }
    VIStationRequest83 req;
    memset(&req, 0, sizeof(req));
    req.m_header.m_length = VISwap32(sizeof(req) + count * sizeof(VIStationPair));
    req.m_header.m_type = VISwap32(83);
    req.m_a = VISwap32(a);
    req.m_b = VISwap32(b);
    req.m_g = VISwap32(g);
    strncpy(req.m_c, c, 64);
    strncpy(req.m_d, d, 3);
    strncpy(req.m_e, e, 3);
    strncpy(req.m_f, f, 4);
    req.m_count = VISwap32(count);
    m_connection.Send(&req, sizeof(req));
    des_soe_set_key(sDesPassword);
    for (int i = 0; i < count; i++)
    {
        VIStationPair out;
        des_soe_encrypt((des_cblock *)pairs[i].m_a, (des_cblock *)out.m_a, 64);
        des_soe_encrypt((des_cblock *)pairs[i].m_b, (des_cblock *)out.m_b, 64);
        m_connection.Send(&out, sizeof(out));
    }
    BeginRequest();
    return 0;
}

// 0x002E98B8 (type 2).
void VIStationAPI::Ping()
{
    if (!Connect())
        return;
    VIStationHeader req;
    memset(&req, 0, sizeof(req));
    req.m_length = VISwap32(sizeof(req));
    req.m_type = VISwap32(2);
    m_connection.Send(&req, sizeof(req));
}

// 0x002E9910 .. 0x002E9988: the listener's empty defaults that are not inline,
// in their source order.
void VIStationListener::OnReply41(bool success, int code, const char *text) {}
void VIStationListener::OnValidateLogin(int a, int b, int c, int d, int e, int f, int g,
                                        const char *s92, const char *s12) {}
void VIStationListener::OnChangePassword(bool success, const char *text) {}
void VIStationListener::OnReply45(int a, int b) {}
void VIStationListener::OnConsumeAccountKey(int a, int b) {}
void VIStationListener::OnReply47(int a, int b) {}
void VIStationListener::OnGetSubscriptionInfo(int a, int b, int c, int d, const char *sku) {}
void VIStationListener::OnCancelAccount(int a, int b) {}
void VIStationListener::OnReply55(int a, int b, const char *s16, const char *s32, int c, int d) {}
void VIStationListener::OnGetSubscriptionList(int a, int b, int count, const char *plans) {}
void VIStationListener::OnError(int code, const char *text) {}
void VIStationListener::OnReply65(int a) {}
void VIStationListener::OnStatus(const char *text) {}
void VIStationListener::OnConnected() {}
void VIStationListener::OnReply67(int a) {}
void VIStationListener::OnIdle() {}
