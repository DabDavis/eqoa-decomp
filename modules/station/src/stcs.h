// stcs.h -- customer service: the CS client (vicsclient.cpp,
// 0x002DDC48 .. 0x002DF918; CLIENT has the same TU at 0x00BE7120 .. 0x00BE8DF0,
// its map calls the class VIClientPart33950, a placeholder), VICSInterface.cpp
// (0x002DF918 .. 0x002E1FF8; its asserts name
// C:/code-frontiers-gm/ui/utils/VICSInterface.cpp) and the CS record stream
// operators (vicsrecords.cpp, 0x002EA680 .. 0x002EADA0; CLIENT 0x00BFC890 ..
// 0x00BFCFB0).  No class or method here is named by the image: the names are
// ours unless a comment says otherwise.
#ifndef STCS_H
#define STCS_H

#include "vicsclient.h"        // the CS client and its records (shared with CLIENT)
#include "viclienti.h"         // STATION's VIClientI (this module's own)


// VICSInterface embeds a VIWndCSRequest and a VIWndCSInbox by value, so their
// classes must be complete here; stwndcs.h's own declarations need only the
// records above.  (station.h includes stwndcs.h after this header.)
#include "stwndcs.h"

// ------------------------------------------------------- the interface --
// What VICSInterface.cpp calls in other modules and the C library.
const char *VIGetGameCode();
void McErr(VIWnd *parent, const VIVect2I &pos, int code);   // MCERR's export
extern "C" void _VIAlert(const char *fmt, ...);
extern "C" const char *__VIAssertf_file;
extern "C" int __VIAssertf_line;
// SUPPORT's spelling of a non-fatal assert: the file and line go to two globals
// first, then the message.
#define VIALERT(fmt, args...) \
    { __VIAssertf_file = __FILE__; __VIAssertf_line = __LINE__; _VIAlert(fmt, ## args); }

// A customer-service server out of eqahosts.txt: 132 bytes, a list node's data.
struct VICSServer
{
    VICSServer() {}
    VICSServer(const char *host, int port)
    {
        strncpy(m_host, host, sizeof(m_host));
        m_port = port;
    }

    char m_host[128];   // +0x00
    int  m_port;        // +0x80  10070 when the line has none
};

// The customer-service connection: the CS client and the two CS windows, over
// DRDP to the CS server (its log calls the far end VICSAssistBrain).  0x5890
// bytes, 8-aligned (the windows are); VIWndGenericRenderer embeds one as its
// last member.  ctor 0x002DF918, dtor 0x002DF9E8 (not virtual), __tf 0x002E1F60.
class VICSInterface : public VIClientI
{
public:
    VICSInterface(VIWndGenericRenderer *renderer);                 // 0x002DF918, asserts line 40
    ~VICSInterface();                                              // 0x002DF9E8
    int  LoadServerList();                                         // 0x002DFC18
    VICSServer SelectServer();                                     // 0x002DFE30
    void Connect();                                                // 0x002E0080, lines 232 244 250
    void Start(const char *host, int port, const char *user,
               const char *password);                              // 0x002E0268
    int  IsActive();                                               // 0x002E0370
    void OpenRequest(const char *user, const char *password);      // 0x002E0398, line 308
    void OpenInbox(const char *user, const char *password);        // 0x002E04D8, line 333
    void EndCSRequestSession();                                    // 0x002E0618, line 358
    void EndCSInboxSession();                                      // 0x002E06B0, line 370
    void Disconnect();                                             // 0x002E0748
    VIClientConn GetConn(int type);                                // 0x002E07A0: slot 1
    void SendVersion();                                            // 0x002E07B8: message 0, 18
    void SendLogin();                                              // 0x002E0880: message 0x0995
    void Idle();                                                   // 0x002E09D8: empty
    void ReceivePackets();                                         // 0x002E09E0
    void ProcessMessages();                                        // 0x002E0A68, lines 492 542 550
    void HandleMessage(const void *data, int size);                // 0x002E0D98, line 658
    void Update();                                                 // 0x002E10F8
    // Inline: its copy follows __tf13VICSInterface (0x002E1FB0).
    VICSClient *GetClient() { return &m_client; }

    char                  m_host[256];      // +0x0004  the CS server
    int                   m_port;           // +0x0104
    bool                  m_connecting;     // +0x0108  from Start until the login answer
    bool                  m_loggedIn;       // +0x010C  (bool: IsActive's two tests are not merged into one ld)
    int                   m_windowsAdded;   // +0x0110  the two windows are the renderer's children
    int                   m_open;           // +0x0114  what to open once logged in: 1 request, 2 inbox
    drdp_t               *m_drdp;           // +0x0118
    connection_t         *m_conn;           // +0x011C
    VICSClient            m_client;         // +0x0120
    bool                  m_x4AB4;          // +0x4AB4  long timeouts; sent in the login as a byte
    int                   m_x4AB8;          // +0x4AB8  6 once logged in
    VIWndCSRequest        m_request;        // +0x4AC0
    VIWndCSInbox          m_inbox;          // +0x54C0
    VIWndGenericRenderer *m_renderer;       // +0x5818
    VIList<VICSServer>    m_servers;        // +0x581C
    char                  m_user[32];       // +0x5828
    char                  m_password[32];   // +0x5848
    unsigned char         m_encrypted[32];  // +0x5868  the password, AES-encrypted for the login
    int                   m_retries;        // +0x5888  reconnects after a disconnect, up to 3
};
CHK(c_VICSInterface, sizeof(VICSInterface) == 0x5890)

#endif
