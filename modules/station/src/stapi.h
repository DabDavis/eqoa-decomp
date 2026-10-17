// stapi.h -- the account server client, 0x002E53F0 ..
// 0x002EA680, in six TUs (names ours; no assert names them):
//   vinetbuffer.cpp     0x002E53F0  the send and receive buffers of a connection
//   vinetstatus.cpp     0x002E5AC8  "is any interface up": libnet's state, polled
//   virandom.cpp        0x002E5CB8  Shawn Cokus's MT19937 as a class (unused)
//   vistationapi.cpp    0x002E60A8  VIStationAPI, the listener interfaces, and the
//                                   VIList<VIStationServer> it keeps
//   vitcpconnection.cpp 0x002E9CD8  a length-prefixed message stream over a socket
//   tcpsocket.cpp       0x002EA1C8  the socket ("TCPSocket <%s:%i>"), over libinet
// Every message on the wire is big-endian: a 4-byte length (the whole message),
// a 4-byte type, then the body.  Requests are even types, replies odd.
#ifndef STAPI_H
#define STAPI_H

extern "C" void srand(unsigned int seed);   // the PS2 stdlib.h stub has rand only
extern "C" int sceInetGetInterfaceList(sceSifMClientData *cd, unsigned int *buf,
                                       int *list, int n);
extern "C" int sceInetCtlGetState(sceSifMClientData *cd, unsigned int *buf, int id,
                                  int *state);

// The EE kernel's thread parameter block (48 bytes).
#ifndef ST_THREADPARAM
#define ST_THREADPARAM
struct ThreadParam
{
    int   status;           // +0x00
    void (*entry)(void *);  // +0x04
    void *stack;            // +0x08
    int   stackSize;        // +0x0C
    void *gpReg;            // +0x10
    int   initPriority;     // +0x14
    int   currentPriority;  // +0x18
    unsigned int attr;      // +0x1C
    unsigned int option;    // +0x20
    int   waitType;         // +0x24
    int   waitId;           // +0x28
    int   wakeupCount;      // +0x2C
};
extern "C" int  GetThreadId(void);
extern "C" int  ChangeThreadPriority(int id, int priority);
extern "C" int  CreateThread(ThreadParam *param);
extern "C" int  StartThread(int id, void *arg);
extern "C" int  TerminateThread(int id);
extern "C" int  DeleteThread(int id);
extern "C" char _gp[];
#endif

// The wire's byte order.
inline unsigned int VISwap32(unsigned int x)
{
    return (x << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
}

class TCPSocket;

// ---- vinetbuffer.cpp ------------------------------------------------------

// What a connection has yet to send.  12 bytes.
class VINetSendBuffer
{
public:
    VINetSendBuffer();                     // 0x002E53F0
    ~VINetSendBuffer();                    // 0x002E5408
    int  SetSocket(TCPSocket *s);          // 0x002E5460: 1 when s is null
    void Clear();                          // 0x002E5478
    int  Add(const void *data, int len);   // 0x002E54B8 ("Bytes in send buffer")
    int  Send();                           // 0x002E5558: what went out, < 0 a failure

    char      *m_buf;      // +0x00
    int        m_length;   // +0x04
    TCPSocket *m_socket;   // +0x08
};

// What a connection has received and not yet read ("VINetBuffer::1").  20 bytes.
class VINetBuffer
{
public:
    VINetBuffer();                           // 0x002E5650
    ~VINetBuffer();                          // 0x002E5670
    int  SetSocket(TCPSocket *s);            // 0x002E56D0
    void Append(const void *data, int len);  // 0x002E56E8 ("RECV: %i bytes")
    int  Receive(int max);                   // 0x002E57B0
    int  ReceiveAll();                       // 0x002E58E0: Receive(1024) until it stops
    void Clear();                            // 0x002E5928
    int  Read(void *dst, int len);           // 0x002E5970

    char      *m_buffer;         // +0x00
    int        m_x04;            // +0x04  only ever decremented
    int        m_bufferLength;   // +0x08  (its printf's name)
    int        m_x0C;            // +0x0C
    TCPSocket *m_socket;         // +0x10
};

// ---- vinetstatus.cpp ------------------------------------------------------

// Whether any network interface is up: a private libnet client, its interface
// list, and sceInetCtlGetState on each.  108 bytes, built on the caller's stack
// (VIStationAPI::Connect, the renderer).  Name ours.
class VINetStatus
{
public:
    VINetStatus();       // 0x002E5AC8
    ~VINetStatus();      // 0x002E5B10
    int Init();          // 0x002E5B90: 0, or -1
    int GetState();      // 0x002E5C18: 0 an interface is started, 1 none, 2 no Init

    sceSifMClientData m_client;   // +0x00
    unsigned int *m_netBuf;       // +0x3C
    int  m_ifList[10];            // +0x40
    int  m_ifCount;               // +0x68
};

// ---- virandom.cpp ---------------------------------------------------------

// Shawn Cokus's MT19937 (seedMT/reloadMT/randomMT), its globals made members.
// Nothing in STATION constructs one.  2,508 bytes.  Names ours.
class VIRandomMT
{
public:
    VIRandomMT();                           // 0x002E5CB8: left = -1
    void         Seed(unsigned int seed);   // 0x002E5CC8
    unsigned int Reload();                  // 0x002E5D10
    unsigned int Random();                  // 0x002E5EC0
    float        RandomFloat();             // 0x002E5F38: [0, 1), 24 bits
    double       RandomDouble();            // 0x002E5F98: [0, 1), 53 bits
    int          RandomRange(int lo, int hi);   // 0x002E6058: [lo, hi)

    unsigned int  m_state[625];   // +0x000
    unsigned int *m_next;         // +0x9C4
    int           m_left;         // +0x9C8
};

// ---- tcpsocket.cpp --------------------------------------------------------

// A TCP socket over libinet.  80 bytes (VITCPConnection::Initialize's new).
class TCPSocket
{
public:
    TCPSocket(const char *host, int port);   // 0x002EA1C8 (unused)
    TCPSocket();                             // 0x002EA260
    ~TCPSocket();                            // 0x002EA2C0
    void SetHost(const char *host);          // 0x002EA340
    void SetPort(int port);                  // 0x002EA398
    int  Open();                             // 0x002EA3A0: 0, or -1 no host, -2 DNS, -3 open
    int  IsValid();                          // 0x002EA4F0: a connection id exists
    int  IsValid2();                         // 0x002EA500: the same, unused (name ours)
    int  Send(const void *buf, int len, int *flags, int ms);   // 0x002EA510
    int  Recv(void *buf, int len, int *flags, int ms);         // 0x002EA5C8
    int  Close();                            // 0x002EA618
    int  IsOpen();                           // 0x002EA678

    char *m_host;                  // +0x00
    int   m_port;                  // +0x04
    int   m_open;                  // +0x08
    unsigned int *m_netBuf;        // +0x0C  0x800 bytes, 64-aligned
    sceSifMClientData m_client;    // +0x10
    int   m_cid;                   // +0x4C  -1 when closed
};

// ---- vitcpconnection.cpp --------------------------------------------------

// Hears each whole message a VITCPConnection reads.  No data; its vptr is at
// +0.  __tf 0x002E9C98.
class VIConnectionListener
{
public:
    virtual void OnMessage(char *data, int size, int connection) = 0;
};

// A message stream over a TCPSocket: each message starts with its own length,
// big-endian, which counts those four bytes.  0x430 bytes.
class VITCPConnection
{
public:
    VITCPConnection();               // 0x002E9CD8
    ~VITCPConnection();              // 0x002E9D20
    void Close();                    // 0x002E9D80
    int  Initialize();               // 0x002E9E38: a new socket
    int  Open();                     // 0x002E9E88
    int  IsConnected();              // 0x002E9F20
    int  Send(const void *data, int len);         // 0x002E9F50
    int  Connect(const char *host, int port);     // 0x002E9F70
    int  Process();                  // 0x002E9FD0: send, receive, deliver; < 0 lost
    // Inline (VIStationAPI's ctor stores through the member's address).
    void SetListener(VIConnectionListener *l) { m_listener = l; }

    TCPSocket      *m_socket;        // +0x000
    VINetSendBuffer m_send;          // +0x004
    VINetBuffer     m_recv;          // +0x010
    int             m_pending;       // +0x024  the length of the message being read, -1 none
    int             m_id;            // +0x028  handed to the listener
    VIConnectionListener *m_listener;   // +0x02C
    char            m_error[1024];   // +0x030
};

// ---- vistationapi.cpp -----------------------------------------------------

const char *VIGetGameCode();

// The wide strings in replies are big-endian too; one swap serves both ways.
void NetToHostString16(unsigned short *s);   // 0x002E60A8
void HostToNetString16(unsigned short *s);   // 0x002E6FF0 (unused)

// Request 56's argument: the account form (IDCreate).  Offsets are what the
// request copies out of it.  Name ours.
struct VIStationAccount
{
    char m_name[32];        // +0x000
    char m_password[32];    // +0x020  DES-encrypted into the request
    char m_x040[64];        // +0x040
    char m_x080[16];        // +0x080
    char m_x090[16];
    char m_x0A0[64];        // +0x0A0
    char m_x0E0[3];         // +0x0E0
    char m_x0E3[29];
    char m_x100[32];        // +0x100
    char m_x120[16];        // +0x120
    char m_x130[16];        // +0x130
    char m_x140[16];        // +0x140
    char m_x150[16];        // +0x150
    char m_x160[128];       // +0x160
};

// Request 46's argument, passed BY VALUE (the callee copies all 496 bytes).
// Offsets are what the request copies out of it.  Name ours.
struct VIStationBilling
{
    char m_x000[128];       // +0x000
    char m_x080[64];        // +0x080
    char m_x0C0[32];        // +0x0C0
    char m_x0E0[64];        // +0x0E0
    char m_x120[32];        // +0x120
    char m_x140[32];        // +0x140
    char m_x160[32];        // +0x160
    char m_x180[16];        // +0x180
    char m_x190[16];        // +0x190
    char m_x1A0[32];        // +0x1A0  AES-encrypted into the request
    char m_x1C0[32];
    char m_x1E0[16];        // +0x1E0
};

// One element of request 83's array: two 64-byte strings, each DES-encrypted
// and sent after the request itself.  Name ours.
struct VIStationPair
{
    char m_a[64];
    char m_b[64];
};

// One account server: AddServer's argument, a VIList element (its node is the
// 0x10C bytes AddServer's Add news).  Name ours.
struct VIStationServer
{
    char m_host[256];   // +0x000
    int  m_port;        // +0x100
};

// The record arrays four replies carry (sizes from their handlers' strides).
struct VIStationRecord132      // replies 71 and 75: an id and a name
{
    int            m_id;          // +0x00
    unsigned short m_name[64];    // +0x04
};
struct VIStationRecord142      // reply 73: no word in it, so 2-aligned
{
    unsigned short m_x00[2];      // +0x00
    unsigned short m_name[64];    // +0x04
    unsigned short m_desc[5];     // +0x84
};
struct VIStationRecord204      // reply 77 ("GetGCPaymentMethodFields")
{
    char           m_x00[64];     // +0x00
    unsigned short m_name[64];    // +0x40
    int            m_a;           // +0xC0
    int            m_b;           // +0xC4
    int            m_c;           // +0xC8
};

// The account callbacks.  22 virtuals, every default empty, no virtual
// destructor; vtable 0x00363388, __tf 0x002E9BD0.  Signatures are what
// VIStationAPI passes; names are the renderer's printfs where it prints one,
// else ours after the reply type that calls them.  Declaration order is the
// slot order.  Slots 13..18 are defined inline (their copies follow
// __tf17VIStationListener), the rest in vistationapi.cpp.
class VIStationListener
{
public:
    virtual void OnReply41(bool success, int code, const char *text);          // 1  0x002E9910
    virtual void OnValidateLogin(int a, int b, int c, int d, int e, int f, int g,
                                 const char *s92, const char *s12);           // 2  0x002E9918
    virtual void OnChangePassword(bool success, const char *text);             // 3  0x002E9920
    virtual void OnReply45(int a, int b);                                      // 4  0x002E9928
    virtual void OnConsumeAccountKey(int a, int b);                            // 5  0x002E9930
    virtual void OnReply47(int a, int b);                                      // 6  0x002E9938
    virtual void OnGetSubscriptionInfo(int a, int b, int c, int d, const char *sku);  // 7 0x002E9940
    virtual void OnCancelAccount(int a, int b);                                // 8  0x002E9948
    virtual void OnReply55(int a, int b, const char *s16, const char *s32, int c, int d);  // 9 0x002E9950
    virtual void OnGetSubscriptionList(int a, int b, int count, const char *plans);  // 10 0x002E9958
    virtual void OnReply65(int a);                                             // 11 0x002E9968
    virtual void OnReply67(int a);                                             // 12 0x002E9980
    virtual void OnReply69(int a, const unsigned short *s12, const unsigned short *s528) {}   // 13
    virtual void OnReply71(int a, const unsigned short *s12, int count,
                           const VIStationRecord132 *recs) {}                  // 14
    virtual void OnReply73(int a, const unsigned short *s12, int count,
                           const VIStationRecord142 *recs) {}                  // 15
    virtual void OnReply75(int a, const unsigned short *s12, int count,
                           const VIStationRecord132 *recs) {}                  // 16
    virtual void OnGetGCPaymentMethodFields(int a, const unsigned short *s12,
                                            const unsigned short *text, int count,
                                            const VIStationRecord204 *recs) {} // 17
    virtual void OnReply84(int a, const unsigned short *s12, const char *s524,
                           const unsigned short *s588, const unsigned short *s720) {}  // 18
    virtual void OnError(int code, const char *text);                          // 19 0x002E9960
    virtual void OnStatus(const char *text);                                   // 20 0x002E9970
    virtual void OnIdle();                                                     // 21 0x002E9988
    virtual void OnConnected();                                                // 22 0x002E9978
};

// The Station API: a VITCPConnection to one of the account servers, the
// requests, and a reply dispatcher that hands each reply to the listener.
// 0x490 bytes (VIWndGenericRenderer embeds one).  ctor 0x002E60E8, dtor
// 0x002E6180 (not virtual), __tf 0x002E9C40.
class VIStationAPI : public VIConnectionListener
{
public:
    VIStationAPI();
    ~VIStationAPI();
    void AddServer(const char *host, int port);          // 0x002E6210
    void BeginRequest();                                 // 0x002E6298
    int  Initialize();                                   // 0x002E62C8
    virtual void OnMessage(char *data, int size, int connection);   // 0x002E6300

    // The reply handlers, by the type they answer (names ours).
    void OnReply67(char *msg, int size);                 // 0x002E6580
    void OnReply65(char *msg, int size);                 // 0x002E65C0
    void OnSessionKey(char *msg, int size);              // 0x002E6600 (type 4)
    void OnReply63(char *msg, int size);                 // 0x002E6658
    void OnReply45(char *msg, int size);                 // 0x002E6728
    void OnReply55(char *msg, int size);                 // 0x002E67C8
    void OnReply53(char *msg, int size);                 // 0x002E6920
    void OnReply51(char *msg, int size);                 // 0x002E69C0
    void OnReply47(char *msg, int size);                 // 0x002E6AC8
    void OnReply49(char *msg, int size);                 // 0x002E6B68
    void OnReply41(char *msg, int size);                 // 0x002E6C08
    void OnReply43(char *msg, int size);                 // 0x002E6D00
    void OnReply37(char *msg, int size);                 // 0x002E6DA0
    void OnReply69(char *msg, int size);                 // 0x002E7030
    void OnReply71(char *msg, int size);                 // 0x002E7150
    void OnReply73(char *msg, int size);                 // 0x002E72B0
    void OnReply75(char *msg, int size);                 // 0x002E7410
    void OnReply77(char *msg, int size);                 // 0x002E75B8
    void OnReply84(char *msg, int size);                 // 0x002E7830

    void Process();                                      // 0x002E7A28
    int  ValidateLogin(const char *name, const char *password);   // 0x002E7B58 (type 36)
    VIStationServer ChooseServer();                      // 0x002E7C78
    void ConnectThread();                                // 0x002E7EA8
    void StartConnectThread();                           // 0x002E7F20
    int  Connect();                                      // 0x002E7FA0: 1 connected
    // The requests: each returns 1 for a bad argument or no connection, 0 once
    // sent (or refused because one is outstanding).  Names ours, by type.
    int  CreateAccount(const VIStationAccount *a);                        // 0x002E8158 (56)
    int  ChangePassword(int id, const char *oldpw, const char *newpw);    // 0x002E82E0 (42)
    int  Request44(int id, const char *a, const char *b);                 // 0x002E8418
    int  Request48(int id, const char *a, const char *b);                 // 0x002E8540
    int  Request46(int id, VIStationBilling b, const char *sku);          // 0x002E8668
    int  Request66(const char *a, const char *b, const char *c, const char *d,
                   const char *e);                                        // 0x002E8978
    int  Request50(int id, const char *a);                                // 0x002E8B50
    int  Request52(int id, const char *a);                                // 0x002E8C58
    int  Request54(int id, const char *a, const char *b);                 // 0x002E8D60
    int  Request62(int id, const char *a);                                // 0x002E8E88
    int  Request64(int id, const char *a, const char *b, const char *c);  // 0x002E8F90
    int  Request68(const char *a);                                        // 0x002E90D8
    int  Request70(const char *a, const char *b);                         // 0x002E91A0
    int  Request72(const char *a, const char *b, const char *c);          // 0x002E9290
    int  Request74(const char *a, const char *b, const char *c, const char *d);   // 0x002E93A0
    int  Request76(const char *a, const char *b, int c);                  // 0x002E94D8
    int  Request83(int a, int b, const char *c, const char *d, const char *e,
                   const char *f, int g, int count, const VIStationPair *pairs);  // 0x002E95F8
    void Ping();                                         // 0x002E98B8 (type 2)
    // Inline; its copy follows __tf12VIStationAPI (0x002E9C90).
    void SetListener(VIStationListener *l) { m_listener = l; }

    VITCPConnection     m_connection;      // +0x004
    VIStationListener  *m_listener;        // +0x434
    VIList<VIStationServer> m_servers;     // +0x438
    long  m_connectTime;                   // +0x448
    long  m_requestTime;                   // +0x450
    int   m_haveKey;                       // +0x458  the session key arrived
    unsigned char m_key[32];               // +0x45C  ... DES-decrypted into here
    int   m_connectResult;                 // +0x47C
    int   m_busy;                          // +0x480  a request is outstanding
    int   m_thread;                        // +0x484
    int   m_threadRun;                     // +0x488
};
CHK(c_VIStationAPI, sizeof(VIStationAPI) == 0x490)

#endif
