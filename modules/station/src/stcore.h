// stcore.h -- OWNER: lane core.  The program and its two windows of its own:
// vistation.cpp (0x002B7978 .. 0x002B9240; its tail holds the out-of-line
// VICamera ctor gcc synthesizes, as SETUP's visetup.cpp does), viwindowmanager.cpp
// (0x002B9240 .. 0x002B95B8, an older VIWindowManager than SETUP's: no GetRoot,
// another Update) and viwndstationlogin.cpp (0x002BC6A8 .. 0x002BCF80).
#ifndef STCORE_H
#define STCORE_H

// The window tree's driver, as STATION links it: SETUP's class (the same
// layout, ctor, dtor, Init, Free and SetRoot) before Frontiers gave it GetRoot
// and rewrote Update.  132 bytes; VIStation embeds one at +0xB388.  vptr +0x20,
// __tf 0x002B9568.
class VIWindowManager : public VIWnd
{
public:
    VIWindowManager();                          // 0x002B9240
    virtual ~VIWindowManager();                 // 0x002B9278

    int  Init(VIPad *pad, VIKeyboard *kb);      // 0x002B92A0
    void Free();                                // 0x002B92D8
    void SetRoot(VIWnd *w);                     // 0x002B92E0
    int  Update();                              // 0x002B92E8

    VIWnd        *m_root;       // +0x24
    VIPad        *m_pad;        // +0x28
    VIKeyboard   *m_keyboard;   // +0x2C
    VIConsolePad  m_lastPad;    // +0x30
};
CHK(c_VIWindowManager, sizeof(VIWindowManager) == 0x84)

// The program: the engine objects, the window manager and the one window it
// runs, the GUI-page renderer.  0x2B130 bytes (station_main's new).
// station_main copies the login and the password out of its first two fields
// when Process says the player is done.
class VIStation
{
public:
    VIStation();                              // 0x002B7978
    ~VIStation();                             // 0x002B7A30
    int  Init(char *login, char *password);   // 0x002B7B40
    void Close();                             // 0x002B7FC0
    int  Process();                           // 0x002B8038
    // The renderer's calls back into the program (names ours).
    void FadeOut();                                   // 0x002B8328
    int  PlayMusic(const char *name, float volume);   // 0x002B8368
    int  IsPlaying(int handle);                       // 0x002B8420
    void Message(const char *text);                   // 0x002B8440
    void Message(const unsigned short *text);         // 0x002B8498
    // [lane cswnd] Inline: the window that has the focus, recorded and given it.
    void SetActive(VIWnd *w) { m_xB40C = (int)w; m_wm.SetFocus(w, true); }

    char                 m_login[256];      // +0x00000
    char                 m_password[256];   // +0x00100
    VIRaster             m_raster;          // +0x00200
    VICollide            m_collide;         // +0x04E60
    VICamera             m_camera;          // +0x05090
    VIDictionary         m_dict;            // +0x05280
    VISoundDevice        m_sound;           // +0x05298
    VIScene              m_scene;           // +0x06420
    VIFont               m_font;            // +0x0AFD0
    VIUI                 m_ui;              // +0x0AFE0
    VIFader              m_fader;           // +0x0B038
    int                  m_running;         // +0x0B058  0 once FadeOut began; Process ends when the fade is full
    VIPad                m_pad;             // +0x0B060
    VIKeyboard           m_keyboard;        // +0x0B0C0
    VIConsolePad         m_padCur;          // +0x0B2D8  this frame's pad
    VIConsolePad         m_padPrev;         // +0x0B32C  last frame's
    int                  m_xB380;           // +0x0B380
    int                  m_xB384;           // +0x0B384  0 after Init
    VIWindowManager      m_wm;              // +0x0B388
    int                  m_xB40C;           // +0x0B40C  0 at Init and at the end
    VIWndGenericRenderer m_renderer;        // +0x0B410
};
CHK(c_VIStation, sizeof(VIStation) == 0x2B130)

// The plain login screen: two fields and Connect, drawn in a VIUIWindow, edited
// through a VIWndEdit it news in Init.  ctor 0x002BC6A8 (the "joe"/"schmoe"
// defaults), dtor 0x002BC718, __tf 0x002BCF30.  vptr +0x20.  Nothing in the
// module creates one: it is the betas' login, kept.
class VIWndStationLogin : public VIWnd
{
public:
    VIWndStationLogin();
    virtual ~VIWndStationLogin();

    int  Init();                                                 // 0x002BC768
    void Free();                                                 // 0x002BC7D8

    virtual void OnDrawSelf();                                   // 0x002BC7E0
    virtual void OnKeyUp(int key);                               // 0x002BCDF0
    virtual void OnKeyDown(int key);                             // 0x002BCCC0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002BCDF8

    // Inline: their copies follow __tf17VIWndStationLogin (0x002BCF80 .. 0x002BD008),
    // in this order.  Names ours.
    void SetLogin(const char *s) { strcpy(m_login, s); }
    void GetLogin(char *dst) { strcpy(dst, m_login); }
    void SetPassword(const char *s) { strcpy(m_password, s); }
    void GetPassword(char *dst) { strcpy(dst, m_password); }

    char        m_login[256];      // +0x024
    char        m_password[256];   // +0x124
    VIWndEdit  *m_edit;            // +0x224
    VIUIWindow  m_window;          // +0x228
    int         m_sel;             // +0x3A8  0 login, 1 password, 2 Connect
};

#endif
