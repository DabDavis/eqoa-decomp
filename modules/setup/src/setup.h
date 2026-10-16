// setup.h -- SETUP's game half: the title screens and the program that runs them
// (0x00566C00 .. 0x0057CDD8), on top of SUPPORT's vi.h and the patcher's
// patcher.h.  The class and method names are the 2002 betas' own exports
// (decomp-builds/, `snr1map.py`); the classes Frontiers adds (VIWndController,
// VIWndLanguage, VIWndPresents) are named by their RTTI strings.  Layouts are
// what the written functions measure, and each constructor's member roll-call.
//
// The link order is one TU per class, each ending in its template copies, its
// `__tf` and then its inline members:
//   main, visetup, vifader, visplashfx, viwindowmanager, then viwnd*.cpp in
//   alphabetical order (beta connect controller dnas eula language legal
//   message options patcher presents readmessage splash).
#ifndef SETUP_H
#define SETUP_H

#include "patcher.h"   // VISetup is a PatcherAPI; also the PS2 libc headers
#include "vi.h"
#include "vifader.h"

extern "C" unsigned int VIHashResourceID(const char *s);
unsigned int VIHashResourceID_Icon(const char *name);
VIGameOptions *VIGetGameOptions();

class VISetup;

// The window tree's driver: it routes the keyboard and the pad to the focused
// window and draws the root.  132 bytes; VISetup embeds one at +0xAF74.  A pad
// button is a key code 128 + i, i its float's index after m_a.
class VIWindowManager : public VIWnd
{
public:
    VIWindowManager();
    virtual ~VIWindowManager();

    int    Init(VIPad *pad, VIKeyboard *kb);
    void   Free();
    VIWnd *GetRoot();
    void   SetRoot(VIWnd *w);
    int    Update();

    VIWnd        *m_root;       // +0x24
    VIPad        *m_pad;        // +0x28
    VIKeyboard   *m_keyboard;   // +0x2C
    VIConsolePad  m_lastPad;    // +0x30
};

// A full-screen splash shown for 7.5 s: fades in over the first second and
// out from 6 s to 7 s, then hands the focus back.  START skips to the fade.
// VIWndLegal (legal_l/r.tga) and VIWndPresents (presents_l/r.tga) are the same
// code; Presents is new in Frontiers.  48 bytes each.
class VIWndLegal : public VIWnd
{
public:
    VIWndLegal();
    virtual ~VIWndLegal();

    int  Init();
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyDown(int key);
    virtual void OnKeyUp(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    long m_time;   // +0x28  when the focus arrived
};

class VIWndPresents : public VIWnd
{
public:
    VIWndPresents();
    virtual ~VIWndPresents();

    int  Init();
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyDown(int key);
    virtual void OnKeyUp(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    long m_time;   // +0x28
};

// ===========================================================================
// The windows.  Each derives from SUPPORT's VIWnd (vptr at +0x20, own data from
// +0x24/+0x28); VIUIWindow is 0x180 bytes and 8-aligned.  Messages: 295 is
// VIWnd::SetFocus's notice (p1 the old focus, p2 the new).  Key codes: a pad
// button is 128 + its VIConsolePad float index after m_a (144 = START).
// Sizes are fixed by where VISetup and the other windows embed them.
//
// LANE RULE: each class below belongs to one TU; edit only your own block.
// ===========================================================================

// ---- viwndmessage.cpp -- 0x005755D8 .. 0x00575DD8 -------------------------
// A message box: title, text, and a one- or two-choice prompt.  0x480 bytes;
// Connect, Dnas, Eula, Options, Patcher and Splash embed one.
// Which choices the prompt row offers (WORDS.TXT ids in parentheses).
enum VIMESSAGEPROMPT
{
    VIMESSAGEPROMPT_OK = 0,         // "Ok" (786)
    VIMESSAGEPROMPT_OKCANCEL = 1,   // "Ok" (786) / "Cancel" (753)
    VIMESSAGEPROMPT_YESNO = 2       // "Yes" (21) / "No" (22)
};

class VIWndMessage : public VIWnd
{
public:
    VIWndMessage();
    virtual ~VIWndMessage();

    int  Init();
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyDown(int key);
    virtual void OnKeyUp(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    void Setup(const char *title, const char *text, VIMESSAGEPROMPT prompt,
               const VIVect2I &center, int width);
    void Setup(const char *title, const unsigned short *text, VIMESSAGEPROMPT prompt,
               const VIVect2I &center, int width);
    void Setup(const unsigned short *title, const unsigned short *text,
               VIMESSAGEPROMPT prompt, const VIVect2I &center, int width);
    // 0x00575CF0 (name ours): `s` into m_buf, wrapped in |q..|w when `on`.
    unsigned short *Highlight(const unsigned short *s, int on);

    // The betas' inline accessor.  Frontiers emits no copy of it after
    // __tf12VIWndMessage, so the class has no inline members: read m_sel.
    int Selection() const;

    VIUIWindow      m_window;     // +0x028
    unsigned short  m_title[32];  // +0x1A8
    unsigned short  m_text[256];  // +0x1E8
    int             m_sel;        // +0x3E8  0 = the first choice
    VIVect2I        m_center;     // +0x3EC
    int             m_width;      // +0x3F4
    int             m_rows;       // +0x3F8  text rows + 3
    VIMESSAGEPROMPT m_prompt;     // +0x3FC
    unsigned short  m_buf[64];    // +0x400  Highlight's result
};

// ---- viwndreadmessage.cpp -- 0x0057AC60 .. 0x0057B1A0 ---------------------
// A scrolling read-only text page (the patch notes).  0x101B8 bytes; Patcher
// embeds one at +0x948.  Seven rows are visible.
class VIWndReadMessage : public VIWnd
{
public:
    VIWndReadMessage();
    virtual ~VIWndReadMessage();

    int  Init();
    void Clear();

    virtual void OnDrawSelf();
    virtual void OnKeyDown(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    const unsigned short *Message() const;
    void SetMessage(const unsigned short *s);
    void AppendMessage(const unsigned short *s);
    void Prev();
    void Next();

    VIUIWindow      m_window;          // +0x00028
    int             m_x1A8;            // +0x001A8
    unsigned short  m_text[0x8000];    // +0x001AC
    int             m_rows;            // +0x101AC
    int             m_top;             // +0x101B0  first row shown
    int             m_dirty;           // +0x101B4  re-count m_rows
};

// ---- viwndbeta.cpp -- 0x0056D508 .. 0x0056E3F0 ----------------------------
// The "Welcome" notice of the betas, still linked; nothing constructs one.
class VIWndBeta : public VIWnd
{
public:
    VIWndBeta();
    virtual ~VIWndBeta();

    int  Init();
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyUp(int key);
    virtual void OnKeyDown(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    VIUIWindow m_window;   // +0x028
};

// ---- viwndconnect.cpp -- 0x0056E3F0 .. 0x0056FF80 -------------------------
// Bringing the network up: the IRX chain, the ezNetCnf combination, the
// connect progress.  0x7F8 bytes; Splash embeds one at +0x1B8.
int LoadIrxRetry(const char *name, int argLen, const char *args, int tries);
// Frontiers' twin of LoadIrxRetry, off cdrom0:\MODULES2 (name ours, after
// SUPPORT's VIEELoadIrx2).
int LoadIrxRetry2(const char *name, int argLen, const char *args, int tries);

// The network configuration record and Sony's ezNetCnf combination it holds
// are declared with VIWndSplash below, which fills them; VIWndConnect only
// keeps a pointer.  BeginConnect and SetCombination copy the 104-byte
// combination whole onto the stack.
class VINetConfig;

class VIWndConnect : public VIWnd
{
public:
    VIWndConnect();
    virtual ~VIWndConnect();

    int  Init();
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyDown(int key);
    virtual void OnKeyUp(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    void EndConnect();
    int  BeginConnect();
    // 0x0056FB20, new in Frontiers (name ours): set the ezNetCtl combination
    // and bring the interface up.
    int  SetCombination();

    void SetNetConfig(VINetConfig *cfg, int count, int sel)
    {
        m_netConfig = cfg;
        m_count = count;
        m_sel = sel;
    }
    int  Selection() const { return m_sel; }
    // New in Frontiers (names ours): set when the memory card error screen
    // was quit out of, which closes this window too.
    int  Cancelled() { return m_cancelled; }
    void SetCancelled(int c) { m_cancelled = c; }

    VIWndMessage  m_message;      // +0x028
    VIUIWindow    m_window;       // +0x4A8
    VIUIWindow    m_window2;      // +0x628  the progress box
    VIWnd        *m_child;        // +0x7A8  a closed child (McErr), removed next frame
    int           m_sel;          // +0x7AC
    int           m_connecting;   // +0x7B0
    int           m_unetStarted;  // +0x7B4  unet_startup_async is running
    int           m_smap;         // +0x7B8  module ids, -1 = not loaded
    int           m_dev9;         // +0x7BC
    int           m_an986;        // +0x7C0
    int           m_ppp;          // +0x7C4
    int           m_pppoe;        // +0x7C8
    int           m_spduart;      // +0x7CC
    int           m_cxtmdm;       // +0x7D0
    int           m_retry;        // +0x7D4  frames until a retry reconnects
    int           m_cancelled;    // +0x7D8
    int           m_noCancel;     // +0x7DC  set while unet is starting
    int           m_count;        // +0x7E0
    VINetConfig  *m_netConfig;    // +0x7E4
    long          m_timeout;      // +0x7E8  ms the dial may take
    long          m_deadline;     // +0x7F0
};

// ---- viwndcontroller.cpp -- 0x0056FF80 .. 0x00570F20 ----------------------
// New in Frontiers.  0x1A8 bytes; VISetup embeds one at +0x216F0.
class VIWndController : public VIWnd
{
public:
    VIWndController();
    virtual ~VIWndController();

    int  Init();
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyUp(int key);
    virtual void OnKeyDown(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    VIUIWindow m_window;   // +0x028
};

// ---- viwnddnas.cpp -- 0x00570F20 .. 0x00572768 ----------------------------
// The DNAS authentication screen.  0xD48 bytes; VISetup embeds one at +0x1E150.
// libdnas2's progress report: 16 bytes, copied whole by Status/SetStatus.
// `code` is the stage (1..7, 6 and 7 errors), `sub_code` the error number
// (-9999 has its own text), `progress` the step within stages 2..4.
struct sceDNAS2Status
{
    int           code;          // +0x00
    int           sub_code;      // +0x04
    int           progress;      // +0x08
    unsigned char reserved[4];   // +0x0C
};

class VIWndDnas : public VIWnd
{
public:
    VIWndDnas();
    virtual ~VIWndDnas();

    int  Init(VIScene *scene);
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyDown(int key);
    virtual void OnKeyUp(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    void DrawFakeSplashInterface();
    // 0x00571990, new in Frontiers (name ours): the error text for a failed
    // stage, built into m_errorText.
    const unsigned short *ErrorText(const sceDNAS2Status &s);

    sceDNAS2Status Status() { return m_status; }
    void SetStatus(sceDNAS2Status s) { m_status = s; }

    VIUIWindow     m_window;           // +0x028  the fake title-screen bar
    VIUIWindow     m_window2;          // +0x1A8
    VIScene       *m_scene;            // +0x328
    int            m_icon;             // +0x32C  dnas_64, -1 until found
    VIUIWindow     m_window3;          // +0x330  the status panel
    sceDNAS2Status m_status;           // +0x4B0
    int            m_error;            // +0x4C0  showing an error
    VIWndMessage   m_message;          // +0x4C8
    unsigned short m_errorText[512];   // +0x948
};

// ---- viwndeula.cpp -- 0x00572768 .. 0x00574238 ----------------------------
// The licence agreement: EULA_<language>.TXT out of EULA.WAD on the card, else
// out of data2\eula.wad on the disc, else EULA.TXT on the card, else
// data\EULA.TXT on the disc.  Sixteen rows show; declining asks again through
// m_message.  0x2648 bytes; VISetup embeds one at +0x1F078.
struct VIEulaRow
{
    unsigned short *m_text;   // +0 into VIWndEula::m_text
    short           m_len;    // +4 characters
    short           m_break;  // +6 1 when the row ends in a newline or a wrap
};

class VIWndEula : public VIWnd
{
public:
    VIWndEula();
    virtual ~VIWndEula();

    int  Init();
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyUp(int key);
    virtual void OnKeyDown(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    void ComputeTextRows();
    void Down();
    void Up();

    VIUIWindow      m_window;        // +0x028
    VIWndMessage    m_message;       // +0x1A8
    int             m_sel;           // +0x628  0 accept, 1 decline
    int             m_dirty;         // +0x62C  rows not yet computed
    unsigned short *m_text;          // +0x630
    int             m_top;           // +0x634  first row shown
    VIEulaRow       m_rows[1024];    // +0x638
    int             m_numRows;       // +0x2638
    int             m_repeat;        // +0x263C frames to the next held-key step
    int             m_repeatDir;     // +0x2640 1 down, -1 up
    int             m_asking;        // +0x2644 m_message is up
};

// ---- viwndlanguage.cpp -- 0x00574238 .. 0x00575300 ------------------------
// New in Frontiers: the language picker (English, Francais, Deutsch), the
// choice kept as 1-3 in VIGameOptions +0x96.  Choosing starts a 60-frame
// countdown, at whose end the words are reloaded and the focus handed back.
// 0x1B0 bytes; VISetup embeds one at +0x1EEC8.
class VIWndLanguage : public VIWnd
{
public:
    VIWndLanguage();
    virtual ~VIWndLanguage();

    int  Init();
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyUp(int key);
    virtual void OnKeyDown(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    VIUIWindow m_window;   // +0x028
    int        m_sel;      // +0x1A8  0..2
    int        m_count;    // +0x1AC  frames left after a choice, 0 = choosing
};

// ---- viwndoptions.cpp -- 0x00575DD8 .. 0x00578DF8 -------------------------
// The Options window: a four-entry menu (Game, Interface, Sound, Save Changes)
// over three pages of settings.  Most settings are edited LIVE in
// VIGameOptions and put back from the m_saved* copies on Cancel; four are
// edited in m_edit* and committed only when "Save Options?" is confirmed.  The
// save itself runs four frames later, behind a notice.  0x980 bytes; Splash
// embeds one at +0x9B0.  Methods past Increment are new in Frontiers; their
// names are ours.
class VIWndOptions : public VIWnd
{
public:
    VIWndOptions();
    virtual ~VIWndOptions();

    int  Init(VIScene *scene);
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyDown(int key);
    int  SaveOptions();
    virtual void OnKeyUp(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    void Prev();
    void Next();
    void Decrement();
    void Increment();
    int  Select();
    int  Back();
    void DrawMenu();
    void DrawGamePage();
    void DrawInterfacePage();
    void DrawSoundPage();

    int           m_x24;        // +0x024  zeroed by Init, never read
    VIUIWindow    m_window;     // +0x028  the menu
    VIUIWindow    m_window2;    // +0x1A8  the open page
    VIUIWindow    m_window3;    // +0x328  the saving / save-failed notice
    int           m_page;       // +0x4A8  -1 = on the menu
    int           m_row;        // +0x4AC  cursor on the page
    int           m_item;       // +0x4B0  the setting under it (Decrement/Increment's case)
    int           m_saved06;    // +0x4B4  VIGameOptions as the window opened,
    unsigned char m_saved07;    // +0x4B8  put back by Cancel
    unsigned char m_saved04;    // +0x4B9
    unsigned char m_saved05;    // +0x4BA
    float         m_saved08;    // +0x4BC
    float         m_saved0C;    // +0x4C0
    unsigned char m_saved91;    // +0x4C4
    unsigned char m_saved92;    // +0x4C5  inverted
    unsigned char m_edit93;     // +0x4C6  Faction Messages (inverted)
    unsigned char m_edit94;     // +0x4C7  Loot Messages (inverted)
    unsigned char m_edit95;     // +0x4C8  Auto Attack
    unsigned char m_edit97;     // +0x4C9  Vibration
    int           m_sel;        // +0x4CC  cursor on the menu
    int           m_mcErr;      // +0x4D0  a McErr window is up
    int           m_saveCount;  // +0x4D4  frames to the save
    int           m_saveFailed; // +0x4D8
    VIWnd        *m_child;      // +0x4DC  McErr's window, removed once it closes
    VIBattleMusic m_music;      // +0x4E0  previews the Battle Music choice
    VIWndMessage  m_message;    // +0x500  "Save Options? Are you sure?"
};

// ---- viwndpatcher.cpp -- 0x00578DF8 .. 0x0057A988 -------------------------
// The patch download screen and its release notes.  0x10B00 bytes; VISetup
// embeds one at +0xD650.  m_state is VISetup's: -1 cancelled, 0..7 the
// patcher's phases (4 = finished, offering the notes; 2 and 7 = failed).
class VIWndPatcher : public VIWnd
{
public:
    VIWndPatcher();
    virtual ~VIWndPatcher();

    int  Init();
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyDown(int key);
    virtual void OnKeyUp(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    void SetProgress(float f);
    void AppendMessage(const unsigned short *s);
    void ShowPatchMessage();
    void DrawFakeSplashInterface();

    // Inline: emitted out of line after __tf12VIWndPatcher (0x0057A968..).
    int  State() { return m_state; }
    void SetState(int s) { m_state = s; }
    int  Selection() const { return m_sel; }
    void SetRestartRequired(int r) { m_restartRequired = r; }

    VIUIWindow       m_window;          // +0x028  the title bar
    VIUIWindow       m_window2;         // +0x1A8  the failure box
    VIUIWindow       m_window3;         // +0x328  the progress box
    int              m_sel;             // +0x4A8  0 read the notes, 1 continue
    int              m_state;           // +0x4AC
    float            m_progress;        // +0x4B0
    int              m_restartRequired; // +0x4B4
    int              m_confirm;         // +0x4B8  what m_message is asking:
                                        //         0 restart notice, 1 cancel
    int              m_showMessage;     // +0x4BC  open the restart notice
    int              m_restartShown;    // +0x4C0
    VIWndMessage     m_message;         // +0x4C8
    VIWndReadMessage m_notes;           // +0x948
};

// ---- viwndsplash.cpp -- 0x0057B1A0 .. 0x0057CDD8 --------------------------
// The title screen: the main menu, the network configuration scan, and the
// Connect and Options windows it opens.  0x2658 bytes; VISetup embeds one at
// +0xAFF8.

// One combination as Sony's ezNetCnf hands it out of a memory card's network
// configuration file: 104 bytes, its name ending in the combination's number.
struct ezNetCnfCombination_t
{
    int  status;     // +0x00  0 = an empty slot
    int  type;       // +0x04  1 modem, 2 PPPoE, 3 Ethernet (BeginConnect)
    char name[32];   // +0x08  the combination's name on mc0:
    char ifc[32];    // +0x28
    char dev[32];    // +0x48  "SCE..." for Sony's own adaptor
};

struct ezNetCnfCombinationList_t
{
    int                   count;      // +0x000
    ezNetCnfCombination_t list[10];   // +0x004
};

extern "C" int ezNetCnfGetCombinationList(const char *device, ezNetCnfCombinationList_t *list);
extern "C" ezNetCnfCombination_t *ezNetCnfGetDefault(ezNetCnfCombinationList_t *list);
extern "C" ezNetCnfCombination_t *ezNetCnfGetCombination(ezNetCnfCombinationList_t *list, int n);

// One usable combination, as the title screen keeps it for VIWndConnect
// (SetNetConfig).  620 bytes.
class VINetConfig
{
public:
    int                   m_number;    // +0x000  the digit that ends m_comb.name
    char                  m_ifc[256];  // +0x004
    char                  m_dev[256];  // +0x104
    ezNetCnfCombination_t m_comb;      // +0x204
};

class VIWndSplash : public VIWnd
{
public:
    VIWndSplash();
    virtual ~VIWndSplash();

    int  Init(VIScene *scene, VIPad *pad);
    void Free();

    virtual void OnDrawSelf();
    virtual void OnKeyDown(int key);
    virtual void OnKeyUp(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    void RestartConnect();
    ezNetCnfCombination_t NetCnfCombination();
    // 0x0057BDD8 (name ours; new in Frontiers): the menu's first choice --
    // check the card, scan it, and open Connect or say why not.
    void Connect();
    int  ScanNetworkConfigurations();

    int  Selection() const { return m_sel; }
    void NotifyDetached() { m_ispLost = 0; m_detached = 1; }
    void NotifyIspLost() { m_detached = 0; m_ispLost = 1; }
    int  DefaultNetConfig() const { return m_default; }
    ezNetCnfCombination_t DefaultSceCombination() const { return m_configs[m_default].m_comb; }

    VIScene      *m_scene;        // +0x024
    VIPad        *m_pad;          // +0x028
    VIUIWindow    m_window;       // +0x030
    int           m_sel;          // +0x1B0  0 connect, 1 quit (VISetup fades out), 2 options
    VIWndConnect  m_connect;      // +0x1B8
    VIWndOptions  m_options;      // +0x9B0
    VIWndMessage  m_message;      // +0x1330
    int           m_default;      // +0x17B0  index of ezNetCnf's default in m_configs
    int           m_numConfigs;   // +0x17B4
    VINetConfig   m_configs[6];   // +0x17B8
    int           m_frames;       // +0x2640  the start-up sequence: 30 McErr, 32 card check
    VIWnd        *m_remove;       // +0x2644  a child to drop on the next draw
    int           m_detached;     // +0x2648
    int           m_ispLost;      // +0x264C
    int           m_ready;        // +0x2650  the menu takes keys
    int           m_mcState;      // +0x2654  1 waiting on McErr, 2 retry Connect
};

// ---- visplashfx.cpp -- 0x0056B3A0 .. 0x0056D170 ---------------------------
// The title screen's animated overlays: a bird, a flock crossing the sky, and
// (Frontiers only, behind expansion flag 2) the island layer drawn from
// splashAnims1..4.tga and island.tga.  All state is file statics.  Render's
// argument says whether to draw; the animation advances either way, and every
// caller passes 1.
int  VISplashFxInit(VIRaster *r, VIDictionary *d);
void VISplashFxFree();
void VISplashFxReset();
int  VISplashFxRender(int draw);

// ---- visetup.cpp -- 0x00567838 .. 0x0056B088 ------------------------------
// The program: the engine objects, every window, and the patcher callbacks.
// 0x21AE0 bytes, `new`ed by setup_main.  Layout: the constructor's roll-call.
class VISetup : public PatcherAPI
{
public:
    VISetup();
    virtual ~VISetup();

    int  Init(int x);
    void Free();
    int  Process();
    int  MainLoopBegin();
    int  MainLoopEnd();
    int  MainLoop();
    int  RunPatcher();
    void RestartConnect();
    int  ValidatePatchMemCard();
    void DeletePatchImage(VIPatchImage *p);
    int  ReadPatchData(VIPatchImage **list);
    int  WritePatchData(VIPatchImage **list);
    int  EncryptRel(VIPatchImage *p);
    int  HandleMemCardAbnormal();
    void StartAmbientSoundtrack();
    void StopAmbientSoundtrack();

    // Inline in the class body: emitted after __tf7VISetup in this order,
    // because this TU emits the vtable.  setup_main reads RestartRequired()
    // as a plain load.
    int RestartRequired() const { return m_restart; }
    virtual int OnStart() { return 1; }
    virtual int OnProduct(const char *name) { return 1; }
    virtual int OnUnzip(const char *name) { return 1; }
    virtual int OnFileFinished(const char *name) { return 1; }
    virtual int OnCancel() { return 1; }
    virtual int OnForceComputerRestart() { return 1; }
    virtual int OnSetTickCount(int n) { return 1; }
    virtual int OnSetFileTickCount(int n) { return 1; }
    virtual int OnFileTick(int n) { return 1; }
    virtual int OnStatusInfo(const char *text) { return 1; }
    virtual int OnEstimate(long total, long left) { return 1; }
    virtual int OnFinish() { return 1; }
    virtual int OnFatalError(int code, const char *text) { return 1; }
    virtual int OnFile(const char *name);
    virtual int OnTick(int n);
    virtual int OnError(int code, const char *text);

    char            m_x08[8];          // +0x00008
    VIRaster        m_raster;          // +0x00010
    VICollide       m_collide;         // +0x04C70
    VICamera        m_camera;          // +0x04EA0
    VIDictionary    m_dict;            // +0x05090
    VISoundDevice   m_sound;           // +0x050A8
    VIScene         m_scene;           // +0x06230
    VIFont          m_font;            // +0x0ADE0
    VIUI            m_ui;              // +0x0ADF0
    VIFader         m_fader;           // +0x0AE48
    VIPad           m_pad;             // +0x0AE68
    VIConsolePad    m_padCur;          // +0x0AEC8  this frame's pad
    VIConsolePad    m_padPrev;         // +0x0AF1C  last frame's
    int             m_xAF70;           // +0x0AF70
    VIWindowManager m_wm;              // +0x0AF74
    VIWndSplash     m_splash;          // +0x0AFF8
    VIWndPatcher    m_patcherWnd;      // +0x0D650
    VIWndDnas       m_dnas;            // +0x1E150
    VIWndPresents   m_presents;        // +0x1EE98
    VIWndLanguage   m_language;        // +0x1EEC8
    VIWndEula       m_eula;            // +0x1F078
    VIWndLegal      m_legal;           // +0x216C0
    VIWndController m_controller;      // +0x216F0
    int             m_restart;         // +0x21898  a patched SETUP/SUPPORT: restart the game
    int             m_x2189C;          // +0x2189C
    int             m_fadeIn;          // +0x218A0  the title's first fade is still to come
    int             m_state;           // +0x218A4  0 the intro screens, 2 the movie, 3 the title
    float           m_progress;        // +0x218A8  OnTick: the steps done before this one
    float           m_x218AC;          // +0x218AC
    int             m_x218B0;          // +0x218B0
    int             m_x218B4;          // +0x218B4
    int             m_ambient;         // +0x218B8  the ambient track's handle, -1 none
    VIKeyboard      m_keyboard;        // +0x218BC
};

#endif
