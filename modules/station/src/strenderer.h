// strenderer.h -- OWNER: lane renderer (wave 2).  VIWndGenericRenderer
// (0x002C98B8 .. 0x002D1F68): the window that loads STATION.PAK, runs the
// pages and answers the account server.  Bases VIWnd (+0, vptr +0x20) and
// VIStationListener (+0x24, its secondary vtable at 0x00357800, every entry's
// delta -0x24).  ctor 0x002C98B8, virtual dtor 0x002C9A78, __tf 0x002D1E88.
// VIStation embeds one.  Method names are ours unless a printf names them.
#ifndef STRENDERER_H
#define STRENDERER_H

// One plan of OnGetSubscriptionList's array, 212 bytes; the renderer keeps a
// VIList of copies (its node is the 220-byte new in 0x002D1D10).  Names ours,
// read off OnGetSubscriptionList's printfs.
struct VISubscriptionPlan
{
    char m_sku[32];          // +0x00
    char m_longDesc[128];    // +0x20
    char m_shortDesc[16];    // +0xA0
    char m_price[16];        // +0xB0
    char m_duration[16];     // +0xC0
    int  m_xD0;              // +0xD0  never read here; an int, because every copy
                             // moves the last word with an aligned lw/sw
};

class VIWndGenericRenderer : public VIWnd, VIStationListener   // the listener base is PRIVATE (its base_info access is 3)
{
public:
    VIWndGenericRenderer();                                      // 0x002C98B8
    virtual ~VIWndGenericRenderer();                             // 0x002C9A78

    void SetLogin(const char *s);                                // 0x002C9C28  strncpy(m_login, s, 63)
    void SetPassword(const char *s);                             // 0x002C9C50
    int  IsLoaded();                                             // 0x002C9C78: loads the pak the first time
    void ReadSavedAccount(const char *text);                     // 0x002C9CB0: username/password/saveaccount lines
    void Load();                                                 // 0x002C9E28: STATION.PAK, the first page
    int  Start();                                                // 0x002CA258  `return 0`
    void AddServers(VIFlatFile *f);                              // 0x002CA260
    void AddServers(const char *text);                           // 0x002CA2C8: "host:port" lines
    void ReadConfig(VIFlatFile *f);                              // 0x002CA3B8
    void ReadConfig(const char *text);                           // 0x002CA420: "NewsURL:..."
    void LoadServers();                                          // 0x002CA520
    void PageBack();                                             // 0x002CA5F0 (lane page's name)
    void PushPage(VIGUIPage *page);                              // 0x002CA970
    void LoadPage(const char *name);                             // 0x002CB4D0

    // VIStationListener, in its slot order (its secondary vtable, delta -0x24).
    // stapi.h's names and signatures.
    virtual void OnReply41(bool success, int code, const char *text);          // 0x002CB6F8  1
    virtual void OnValidateLogin(int a, int b, int c, int d, int e, int f, int g,
                                 const char *s92, const char *s12);           // 0x002CC498  2
    virtual void OnChangePassword(bool success, const char *text);             // 0x002CBD30  3
    virtual void OnReply45(int a, int b);                                      // 0x002CB858  4
    virtual void OnConsumeAccountKey(int a, int b);                            // 0x002CBB58  5
    virtual void OnReply47(int a, int b);                                      // 0x002CB9D8  6
    virtual void OnGetSubscriptionInfo(int a, int b, int c, int d, const char *sku);  // 0x002CC098 7
    virtual void OnCancelAccount(int a, int b);                                // 0x002CBDE8  8
    virtual void OnReply55(int a, int b, const char *s16, const char *s32, int c, int d);  // 0x002CB8E8 9
    virtual void OnGetSubscriptionList(int a, int b, int count, const char *plans);  // 0x002CC950 10
    virtual void OnReply65(int a);                                             // 0x002CB5A0 11
    virtual void OnReply67(int a);                                             // 0x002CB658 12
    virtual void OnReply69(int a, const unsigned short *s12, const unsigned short *s528);   // 0x002CCB10 13
    virtual void OnReply71(int a, const unsigned short *s12, int count,
                           const VIStationRecord132 *recs);                    // 0x002CCB18 14
    virtual void OnReply73(int a, const unsigned short *s12, int count,
                           const VIStationRecord142 *recs);                    // 0x002CCB20 15
    virtual void OnReply75(int a, const unsigned short *s12, int count,
                           const VIStationRecord132 *recs);                    // 0x002CCB28 16
    virtual void OnGetGCPaymentMethodFields(int a, const unsigned short *s12,
                                            const unsigned short *text, int count,
                                            const VIStationRecord204 *recs);   // 0x002CCB30 17
    virtual void OnReply84(int a, const unsigned short *s12, const char *s524,
                           const unsigned short *s588, const unsigned short *s720);  // 0x002CCB38 18
    virtual void OnError(int code, const char *text);                          // 0x002CBF18 19
    virtual void OnStatus(const char *text);                                   // 0x002CBFA8 20
    virtual void OnIdle();                                                     // 0x002CCB40 21
    virtual void OnConnected();                                                // 0x002CC060 22

    void SaveAccount();                                          // 0x002CC288
    void HideDialog();                                           // 0x002CCB70

    void OnAction(const char *action);                           // 0x002CD018
    virtual void OnDrawSelf();                                   // 0x002CFA98
    virtual void OnKeyUp(int key);                               // 0x002D0220
    virtual void OnKeyDown(int key);                             // 0x002D0320
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002D04D8

    void DrawBackground(VIUI *ui, int alt, const unsigned short *title,
                        const unsigned short *text);             // 0x002D0710
    void DrawSpinner(VIUI *ui, const VIVect2I &pos);             // 0x002D09C0
    void ResetSpinner();                                         // 0x002D0A90
    int  OpenCombo(VIWnd *parent, VIOptionItem *item);           // 0x002D0AC8
    VIWndCombo *GetCombo();                                      // 0x002D0B48
    int  ComboOpen();                                            // 0x002D0B58
    void CloseCombo();                                           // 0x002D0B70
    int  BeginEdit(VIWnd *parent, const unsigned short *title, const unsigned short *text,
                   int maxLen, int password);                    // 0x002D0BC0
    VIWndEdit *GetEdit();                                        // 0x002D0CB8
    int  Editing();                                              // 0x002D0CC8
    unsigned short *EndEdit();                                   // 0x002D0CE0

    // [lane core] Inline: emitted after __tf20VIWndGenericRenderer in exactly this
    // order (0x002D1EE8 .. 0x002D1F20), which is why all seven are declared here
    // although VIStation uses four (names ours).
    VIWndEdit     *X28() { return m_editor; }                   // 0x002D1EE8
    VIGUIDialog   *GetDialog() { return m_dialog; }             // 0x002D1EF0
    char          *GetLogin() { return m_login; }               // 0x002D1EF8
    char          *GetPassword() { return m_password; }         // 0x002D1F00
    int            GetResult() { return m_result; }             // 0x002D1F08
    VICSInterface *GetCSInterface() { return &m_cs; }           // 0x002D1F10
    char          *Buf4D8() { return m_sku; }                   // 0x002D1F20

    VIWndEdit           *m_editor;          // +0x00028  Load's own new
    VIList<VIGUIPage *>  m_pages;           // +0x0002C  the page stack, top first
    VIGUIPage           *m_page;            // +0x00038  its top
    VIGUIDialog         *m_dialog;          // +0x0003C  VIStation::Message
    int                  m_x40;             // +0x00040
    int                  m_x44;             // +0x00044
    VIStationAPI         m_api;             // +0x00048
    char                 m_sku[32];         // +0x004D8
    char                 m_gameCard[64];    // +0x004F8  SubmitCheckGameCard's number, upper-cased
    int                  m_x538;            // +0x00538
    int                  m_subStatus;       // +0x0053C  OnGetSubscriptionInfo's status
    int                  m_loginStatus;     // +0x00540
    char                 m_action[1024];    // +0x00544  an action string to run
    int                  m_checkNet;        // +0x00944
    char                 m_login[64];       // +0x00948  VIStation::Process
    char                 m_password[64];    // +0x00988
    char                 m_savedLogin[64];  // +0x009C8
    char                 m_savedPassword[64]; // +0x00A08
    int                  m_saveAccount;     // +0x00A48
    int                  m_xA4C;            // +0x00A4C
    int                  m_uid;             // +0x00A50
    VIStationBilling     m_billing;         // +0x00A54  request 46's
    int                  m_paymentType;     // +0x00C44
    int                  m_loggedIn;        // +0x00C48
    VIFlatFileSystem     m_pak;             // +0x00C4C
    int                  m_result;          // +0x00C70  Process's return
    int                  m_xC74;            // +0x00C74
    VIStationAccount     m_account;         // +0x00C78
    int                  m_loaded;          // +0x00E58
    int                  m_done;            // +0x00E5C  "DONE!" messages
    VIWnd               *m_removeChild;     // +0x00E60
    int                  m_xE64;            // +0x00E64
    char                 m_newsURL[1024];   // +0x00E68
    char                 m_news[0x19000];   // +0x01268
    int                  m_newsActive;      // +0x1A268
    int                  m_newsWait;        // +0x1A26C
    int                  m_newsPoll;        // +0x1A270
    VUrl                *m_newsUrl;         // +0x1A274
    VIList<VISubscriptionPlan> m_plans;     // +0x1A278
    VIWnd               *m_child;           // +0x1A284
    VIWnd               *m_comboParent;     // +0x1A288
    VIWndCombo           m_combo;           // +0x1A28C
    VIWnd               *m_editParent;      // +0x1A2B8
    VIWndEdit            m_edit;            // +0x1A2C0
    long                 m_spinStart;       // +0x1A480
    int                  m_saveDelay;       // +0x1A488
    VICSInterface        m_cs;              // +0x1A490  lane cs's, 8-aligned, 0x5890 bytes
};
CHK(c_VIWndGenericRenderer, sizeof(VIWndGenericRenderer) == 0x1FD20)   // [lane core]

#endif
