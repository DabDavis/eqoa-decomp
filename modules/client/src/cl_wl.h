// cl_wl.h -- windows: VIWndList, VIWndLogin, VIWndLoot, VIWndMainMenu, VIWndMBox, VIWndMenu, VIWndMenu4, VIWndMenu4Edit.
// CLIENT 0x00C685F8..0x00C77748.
// TUs (one window class each, ending with its __tf):
//   VIWndList                  0x00C685F8..0x00C6A9D0
//   VIWndLogin                 0x00C6A9D0..0x00C6BDD0
//   VIWndLoot                  0x00C6BDD0..0x00C6D2D8
//   VIWndMainMenu              0x00C6D2D8..0x00C6F9C0
//   VIWndMBox                  0x00C6F9C0..0x00C71960
//   VIWndMenu                  0x00C71960..0x00C72D18
//   VIWndMenu4                 0x00C72D18..0x00C75438
//   VIWndMenu4Edit             0x00C75438..0x00C77748
#ifndef CL_WL_H
#define CL_WL_H
#ifndef CHK
#define CHK(n, c)  typedef char n[(c) || VI_PORT ? 1 : -1];
#endif

#ifndef CHK
#define CHK(n, c)  typedef char n[(c) || VI_PORT ? 1 : -1];
#endif

// A menu VIWndMenu shows: a static table of 32-byte entries (the window reads through the
// pointer it is given, SetMenu).  Names ours.
class VIMenuDef;
struct VIMenuEntry
{
    int        m_icon;      // +0x00: -1 none; also the row's icon for a number
    int        m_icon2;     // +0x04: -1 none (the second icon or the arrows)
    int        m_x08;       // +0x08: -1 none (the row's x0C, flag 0x20)
    int        m_word;      // +0x0C: -1 none (the label)
    int        m_number;    // +0x10: shown when >= 2
    VIMenuDef *m_sub;       // +0x14: Enter opens it in a VIWndMiniMenu
    int        m_msg;       // +0x18: Enter sends the parent 261 (m_msg, m_id); 0x08050000: two icons
    int        m_id;        // +0x1C
};
class VIMenuDef
{
public:
    int          m_x00;
    int          m_x04;
    VIMenuEntry *m_entries; // +0x08
    int          m_count;   // +0x0C
};

// One row of a VIWndList, 24 bytes (VIWndList::SetItem fills it; the drawer reads it).
// The placeholder name is kept: client_viarray.cpp's VIArray members carry it.
struct VIRec24_C6A8B8
{
    unsigned int m_flags;     // +0x00: 1 icon, 2 label + word, 4 number, 8 arrows,
                              //        0x10 second icon, 0x20 the "more" mark; 0 = empty row
    int          m_icon;      // +0x04
    int          m_icon2;     // +0x08: the second icon, or the arrows' key (138/140)
    int          m_x0C;       // +0x0C
    int          m_word;      // +0x10: a WORDS.TXT id (118/119: two words)
    int          m_number;    // +0x14: drawn "%i"
};

class VIClientPart06D50;

// Foreign free functions (by address; their owners name them).

// VIWndList: ctor 0x00C685F8, 592 bytes (`new`), vtable 0x00CEAFD0 (11 slots: VIWnd's six,
// then SetX60/GetX60/GetX64 (inline) and SetX64).  A scrolling list of up to six visible
// rows of VIRec24_C6A8B8 in a frame; the base of VIWndMenu, VIWndMiniMenu and
// VIWndTradeList.  Its parent hears 296 (the selection moved: old, new) and 256 (Enter:
// the selection).  Names ours.
class VIWndList : public VIWnd
{
public:
    VIWndList();                                                 // 0x00C685F8
    virtual ~VIWndList();                                        // 0x00C686A8
    void DrawItem(VIRec24_C6A8B8 *item, const VIVect2I &pos, int lit, float alpha);  // 0x00C68710
    void DrawArrows(int key, VIVect2I pos);                     // 0x00C68C70
    virtual void OnDrawSelf();                                   // 0x00C68E10
    virtual void OnKeyDown(int key);                             // 0x00C69368
    virtual void OnKeyUp(int key);                               // 0x00C69460
    // slots 7, 8, 9: inline, emitted after __tf9VIWndList
    virtual void SetX60(int v) { m_x60 = v; }                    // 0x00C6AA18
    virtual int  GetX60() { return m_x60; }                      // 0x00C6AA20
    virtual int  GetX64() { return m_x64; }                      // 0x00C6AA28
    virtual void SetX64(int v);                                  // 0x00C694E8: slot 10
    int  Init(int n);                                            // 0x00C69508: n rows (m_items.Init's result)
    void Scroll(int d);                                          // 0x00C69528
    void SetX3C(int a, int b, int c, int d);                     // 0x00C69660: no caller
    void Select(int i);                                          // 0x00C69678: row i into m_cur (-1 clears)
    void SetItem(int i, int flags, int icon, int icon2, int word, int number,
                 int x0C);                                       // 0x00C696E0
    int  Count();                                                // 0x00C69738: rows in use
    int  Fit(int rows);                                          // 0x00C69778
    int  DrawIcon(VIUI *ui, VIClientPart06D50 *icons, int icon, const VIVect2I &pos, int mode,
                  float alpha);                                  // 0x00C69870
    void Open();                                                 // 0x00C69A38
    void Update(float dt);                                       // 0x00C69A80
    float Elapsed();                                             // 0x00C69B68: seconds since the last call

    // Inline: emitted after __tf9VIWndList (0x00C6A9D0 ..), in this order.
    void ResetScroll() { m_sel = 0; m_top = 0; }                 // 0x00C6A9D0
    int  GetSel() { return m_sel; }                              // 0x00C6A9E0
    void SetPos(const VIVect2I &p) { m_pos = p; }                // 0x00C6A9E8
    void Nothing(VIVect2I p) {}                                  // 0x00C6AA00: empty, by value (VIWndTrade's call leaves the argument's dead stores)
    void SetX84(int v) { m_x84 = v; }                            // 0x00C6AA08
    void SetStyle(int v) { m_style = v; }                        // 0x00C6AA10

    VIRec24_C6A8B8 m_cur;          // +0x024: Select's copy of a row
    int            m_x3C;          // +0x03C
    int            m_x40;          // +0x040
    int            m_x44;          // +0x044
    int            m_x48;          // +0x048
    int            m_x4C;          // +0x04C
    int            m_x50;          // +0x050
    VIVect2I       m_pos;          // +0x054
    int            m_selRow;       // +0x05C: Select's row, -1 none
    int            m_x60;          // +0x060
    int            m_x64;          // +0x064
    int            m_sel;          // +0x068: the highlighted row
    int            m_top;          // +0x06C: the first row shown
    int            m_count;        // +0x070: Count() when Fit last ran
    int            m_style;        // +0x074: 0, 1 (the frame and the arrows)
    float          m_alpha;        // +0x078: fades to 1
    float          m_x7C;          // +0x07C
    int            m_x80;          // +0x080
    int            m_x84;          // +0x084
    long           m_time;         // +0x088: Elapsed's clock
    VIArray<VIRec24_C6A8B8> m_items; // +0x090
    VIUIWindow     m_window;       // +0x098
    VIKeyFlash     m_flash; VI_HOST_ONLY(int m_portTail;)  // +0x218: the up/down keys (138, 139); host: no Itanium tail reuse
};
CHK(c_VIWndList, sizeof(VIWndList) == 592)

// VIWndLogin: ctor 0x00C6AA30, 880 bytes (`new`), vtable 0x00CEB2F0.  The account window,
// VIClient+0x2BB7C: three rows (name, password, connect), each edited in a VIWndEdit it
// owns; ConnectShard sends m_user and m_pass.  Names ours.
class VIWndLogin : public VIWnd
{
public:
    VIWndLogin();                                                // 0x00C6AA30
    virtual ~VIWndLogin();                                       // 0x00C6AA78
    int  Init(const char *user, const char *pass);               // 0x00C6AAD8: InitBody's
    void Free();                                                 // 0x00C6AB98
    virtual void OnDrawSelf();                                   // 0x00C6ABF0
    virtual void OnKeyUp(int key);                               // 0x00C6AE40: nothing
    virtual void OnKeyDown(int key);                             // 0x00C6AE48
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C6AFE0

    // Inline: emitted after __tf10VIWndLogin (0x00C6BDD0 ..), in this order.
    char *GetUser() { return m_user; }                           // 0x00C6BDD0
    char *GetPass() { return m_pass; }                           // 0x00C6BDD8

    VIUIWindow m_window;           // +0x028
    VIUIWindow m_info;             // +0x1A8: the "connecting" box (word 271)
    VIWndEdit *m_edit;             // +0x328
    char       m_user[32];         // +0x32C
    char       m_pass[32];         // +0x34C: empty means "send VIClient+0x502DC"
    int        m_field;            // +0x36C: 0 name, 1 password, 2 connect
};
CHK(c_VIWndLogin, sizeof(VIWndLogin) == 880)

// VIWndLoot: ctor 0x00C6BDE0, 1464 bytes (`new`), vtable 0x00CEB5E0.  The corpse window:
// the inventory and the corpse as two VIWndLists side by side, the selected corpse item's
// description in a box below.  Names ours.
class VIWndLoot : public VIWnd
{
public:
    VIWndLoot();                                                 // 0x00C6BDE0
    virtual ~VIWndLoot();                                        // 0x00C6BE30
    int  Init();                                                 // 0x00C6BE90
    virtual void OnDrawSelf();                                   // 0x00C6BFA8
    virtual void OnKeyDown(int key);                             // 0x00C6C358
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C6C3B8
    void Open();                                                 // 0x00C6C518
    void UpdateDesc();                                           // 0x00C6C628

    VIWndList     *m_inv;          // +0x024: 40 rows
    VIWndList     *m_loot;         // +0x028: 42 rows (row 0 "take all", the items, a last row)
    VIUIWindow     m_window;       // +0x030: the description box
    unsigned short m_desc[512];    // +0x1B0
    VIInfoText     m_info;         // +0x5B0
};
CHK(c_VIWndLoot, sizeof(VIWndLoot) == 1464)

// VIWndMainMenu: ctor 0x00C6D2D8, 1072 bytes (`new`), vtable 0x00CEB928.  The game's menu
// over a VIMenuDef (seven rows), the character's name/class/level bar or the menu title,
// the stats column (class icon, HP, power, the 0x9C stat, location, the effects scrolling)
// and two key hints.  An entry with a submenu opens another VIWndMainMenu as m_sub.
// Names ours.
class VIWndMainMenu : public VIWnd
{
public:
    VIWndMainMenu();                                             // 0x00C6D2D8
    virtual ~VIWndMainMenu();                                    // 0x00C6D378
    int  Init(int sub);                                          // 0x00C6D3E8
    int  Free();                                                 // 0x00C6D428
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C6D4B0
    virtual void OnKeyDown(int key);                             // 0x00C6D680
    virtual void OnKeyUp(int key);                               // 0x00C6D7D8
    void Select();                                               // 0x00C6D850
    virtual void OnDrawSelf();                                   // 0x00C6D9D0
    void Scroll(int d);                                          // 0x00C6E098
    void Activate(int reset);                                    // 0x00C6E210
    void DrawStats();                                            // 0x00C6E298
    void Wrap(unsigned short *lines, int width, int chars, int rows,
              const unsigned short *text);                       // 0x00C6E868: 32-char lines
    void UpdateLocation();                                       // 0x00C6EAD8

    // Inline: emitted after __tf13VIWndMainMenu (0x00C6F9C0 ..), in this order.
    void SetX44(int v) { m_x44 = v; }                            // 0x00C6F9C0
    void SetMenu(VIMenuDef *m) { m_menu = m; }                   // 0x00C6F9C8
    void ResetScroll() { m_sel = 0; m_top = 0; }                 // 0x00C6F9D0
    void SetPos(const VIVect2I &p) { m_pos = p; }                // 0x00C6F9E0
    int  GetX34() { return m_x34; }                              // 0x00C6F9F8
    void SetX34(int v) { m_x34 = v; }                            // 0x00C6FA00
    void SetScrollPos(int v) { m_scroll.m_p184 = v; }            // 0x00C6FA08
    int  GetScrollPos() { return m_scroll.m_p184; }              // 0x00C6FA10

    VIMenuDef      *m_menu;        // +0x024
    VIWndMainMenu  *m_sub;         // +0x028
    int             m_sel;         // +0x02C
    int             m_top;         // +0x030
    int             m_x34;         // +0x034: the selection is highlighted
    VIVect2I        m_pos;         // +0x038
    int             m_sub2;        // +0x040: Init's argument (a submenu: no name bar)
    int             m_x44;         // +0x044
    int             m_x48;         // +0x048: drawn while set
    int             m_blinkDir;    // +0x04C: 1 brightening, -1 fading
    float           m_blink;       // +0x050
    long            m_time;        // +0x058
    unsigned short  m_lines[2][32];// +0x060: the location, wrapped
    VIUIWindow      m_window;      // +0x0E0
    VIUITextScroll  m_scroll;      // +0x260: the effects
    VIKeyFlash      m_flash;       // +0x3F8
};
CHK(c_VIWndMainMenu, sizeof(VIWndMainMenu) == 1072)

// VIWndMBox: ctor 0x00C6FA18, 1504 bytes (`new`), vtable 0x00CEBCA0.  STATION's class
// (stwnd.h, viwndmbox.cpp): the message box the windows open over themselves -- a black
// backdrop fading in, the frame, an optional item icon (CLIENT's addition: +0x5D8, -1 none,
// drawn through the client's icon cache, which takes two text rows), the text, a countdown
// and one or two buttons.  The parent hears 268 (accept) or 260 (cancel, or the timeout),
// with m_p1 and m_p2.  CLIENT plays its sounds through VIClient::PlaySound and times
// itself off the client's clock (VIClient+0x7568), where STATION's goes to its VIUI and
// VITimer.  Method names STATION's (ours there too).
class VIWndMBox : public VIWnd
{
public:
    VIWndMBox();                                                 // 0x00C6FA18
    virtual ~VIWndMBox();                                        // 0x00C6FAA0

    int  Init(int type, int style);                              // 0x00C6FAF0
    void Clear();                                                // 0x00C6FB38

    virtual void OnDrawSelf();                                   // 0x00C6FB60
    int  DrawIcon(const VIVect2I &pos);                          // 0x00C70498: CLIENT only
    void Update();                                               // 0x00C706D8: fade, timeout
    virtual void OnKeyDown(int key);                             // 0x00C707B0
    void Accept();                                               // 0x00C708A8
    void Cancel();                                               // 0x00C709F8
    void Move(int dir);                                          // 0x00C70A58
    void SetSel(int sel);                                        // 0x00C70B18: CLIENT only
    virtual void OnKeyUp(int key);                               // 0x00C70B78
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C70B98

    void SetText(const char *s);                                 // 0x00C70C60
    void SetText(const unsigned short *s);                       // 0x00C70C88
    void SetText(unsigned short id);                             // 0x00C70CB0: a WORDS.TXT id
    void ResetSel();                                             // 0x00C70D00

    // Inline: emitted after __tf9VIWndMBox (0x00C71960 ..), in this order.
    void SetPos(const VIVect2I &p) { m_pos = p; }                // 0x00C71960
    void SetParam1(int p) { m_p1 = p; }                          // 0x00C71978
    void SetParam2(int p) { m_p2 = p; }                          // 0x00C71980
    void SetTimeout(long t) { m_timeout = t; }                   // 0x00C71988
    void SetIcon(int icon) { m_icon = icon; }                    // 0x00C71990

    VIVect2I       m_pos;          // +0x024  the window's centre offset
    VIUIWindow     m_window;       // +0x030
    unsigned short m_text[512];    // +0x1B0
    int            m_type;         // +0x5B0  0 Ok, 1 Yes/No (default No), 2 Cancel
    int            m_sel;          // +0x5B4  0 = the first choice
    int            m_p1;           // +0x5B8  sent with 268/260
    int            m_p2;           // +0x5BC
    long           m_timeout;      // +0x5C0  client clock, -1 = none
    long           m_time;         // +0x5C8  the last Update
    float          m_alpha;        // +0x5D0  the fade, 0 .. 1
    int            m_style;        // +0x5D4  1 or 2: the frame
    int            m_icon;         // +0x5D8  CLIENT: an inventory icon, -1 none
};
CHK(c_VIWndMBox, sizeof(VIWndMBox) == 0x5E0)

// VIWndMenu: ctor 0x00C71998, 600 bytes (`new`), vtable 0x00CEBF90 (slots 7..10 VIWndList's).
// A VIWndList over a VIMenuDef; Enter on an entry with a submenu opens a VIWndMiniMenu
// beside it.  Parent hears 261 (an entry's m_msg, m_id), 277 (m_x60 == 1: the two ids)
// and 260 (Escape).  No user destructor (the synthesized one sits before __tf).
class VIWndMenu : public VIWndList
{
public:
    VIWndMenu();                                                 // 0x00C71998
    virtual void OnDrawSelf();                                   // 0x00C719D8
    virtual void OnKeyDown(int key);                             // 0x00C71B20
    virtual void OnKeyUp(int key);                               // 0x00C71E60
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C71EB0
    void SetMenu(VIMenuDef *menu);                               // 0x00C72028
    void Reset();                                                // 0x00C72040
    int  GetId(int i);                                           // 0x00C72098: entry i's m_id, -1 out of range

    VIMenuDef     *m_menu;      // +0x250
    VIWndMiniMenu *m_sub;       // +0x254
};
CHK(c_VIWndMenu, sizeof(VIWndMenu) == 600)

// VIWndMenu4: ctor 0x00C72D18, 76 bytes (`new`), vtable 0x00CEC418.  The four-way quick
// menu: one VITextSet's four choices on the pad's directions; tapping a direction runs its
// text ("|<n>" is a command, anything else is said), holding it opens a VIWndMenu4Edit on
// the choice, and a choice with a sub-set opens another VIWndMenu4.  Names ours.
class VIWndMenu4Edit;
class VIWndMenu4 : public VIWnd
{
public:
    VIWndMenu4();                                                // 0x00C72D18
    virtual ~VIWndMenu4();                                       // 0x00C72D80
    virtual void OnDrawSelf();                                   // 0x00C72E20
    virtual void OnKeyDown(int key);                             // 0x00C734D8
    virtual void OnKeyUp(int key);                               // 0x00C735E8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C73A78
    void SetSet(VITextSet *set);                                 // 0x00C74050
    void DrawChoice(int i, const VIVect2I &at);                  // 0x00C74068
    int  Expand(const unsigned short *src, unsigned short *dst, unsigned short size);  // 0x00C742D8
    const unsigned short *ParseEscape(const unsigned short *s, int *type, int *num);   // 0x00C74440: "%W<n>" etc.
    void FillRec(VIRec388 *out, VITextSet *set, int i);          // 0x00C745A0

    // Inline: emitted after __tf10VIWndMenu4 (0x00C75438 ..), in this order.
    void SetPos(const VIVect2I &p) { m_pos = p; }                // 0x00C75438
    int  GetDepth() { return m_depth; }                          // 0x00C75450
    void SetDepth(int d) { m_depth = d; }                        // 0x00C75458

    VIVect2I        m_pos;         // +0x24
    VITextSet      *m_set;         // +0x2C
    VIWndMenu4     *m_sub;         // +0x30
    float           m_alpha;       // +0x34
    float           m_fade;        // +0x38: > 0 fading in, else out
    int             m_hold;        // +0x3C: frames a direction is held, -1 none
    VIWndMenu4Edit *m_editor;      // +0x40
    int             m_dir;         // +0x44: the direction pressed (0 up, 1 left, 2 right, 3 down), -1 none
    int             m_depth;       // +0x48
};
CHK(c_VIWndMenu4, sizeof(VIWndMenu4) == 76)

// VIWndMenu4Edit: ctor 0x00C75460, 1592 bytes (`new`), vtable 0x00CEC728.  The editor a
// held VIWndMenu4 direction opens: a menu (edit the name, edit the line or drop the
// sub-menu, make a sub-menu, done), then a "say or command" window, then a scrolling
// list of the 34 quick commands.  Done tells the parent 259, cancel 260 (through
// VIWnd::OnMessage).  Names ours.
class VIWndMenu4Edit : public VIWnd
{
public:
    VIWndMenu4Edit();                                            // 0x00C75460
    virtual ~VIWndMenu4Edit();                                   // 0x00C75568
    virtual void OnDrawSelf();                                   // 0x00C75608
    virtual void OnKeyDown(int key);                             // 0x00C75C18
    virtual void OnKeyUp(int key);                               // 0x00C76158: nothing
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C76160
    void DrawShadowText(const unsigned short *s, const VIVect2I &at, int lit);   // 0x00C76280
    void DrawText(const unsigned short *s, const VIVect2I &at, const VIColor32F &c);  // 0x00C76398
    void SetPos(const VIVect2I &p);                              // 0x00C76430
    unsigned short *GetTitle();                                  // 0x00C76448
    void SetTitle(const unsigned short *s);                      // 0x00C76450
    unsigned short *GetText();                                   // 0x00C76478
    void SetText(const unsigned short *s);                       // 0x00C76480
    int  GetHasSub();                                            // 0x00C764A8
    void SetHasSub(int v);                                       // 0x00C764B0
    void Accept();                                               // 0x00C764B8: X on the menu
    void AcceptKind();                                           // 0x00C76680: X on say/command
    void PickCommand();                                          // 0x00C767F8: X on the command list
    void ResetCommand();                                         // 0x00C76850
    void DrawCommands();                                         // 0x00C76860
    VIUICOLOR RowColor(int i);                                   // 0x00C76B00
    VIUICOLOR KindColor(int i);                                  // 0x00C76B10

    // Inline: emitted after __tf14VIWndMenu4Edit (0x00C77748), the class's only one.
    void SetDepth(int d) { m_depth = d; }                        // 0x00C77748

    VIVect2I        m_pos;         // +0x024: where the window flies in from
    int             m_sel;         // +0x02C: the menu's row
    VIWndEdit      *m_edit;        // +0x030
    unsigned short  m_text[128];   // +0x034: the line
    unsigned short  m_title[32];   // +0x134: the name
    int             m_hasSub;      // +0x174
    int             m_hidden;      // +0x178: never set; skips the menu
    int             m_x17C;        // +0x17C
    int             m_kindOpen;    // +0x180
    int             m_cmdOpen;     // +0x184
    VIUIWindow      m_window;      // +0x188
    VIVect2I        m_center;      // +0x308
    VIUIWindow      m_kindWindow;  // +0x310
    VIVect2I        m_kindCenter;  // +0x490
    int             m_kind;        // +0x498: 0 say, 1 command
    VIUIWindow      m_cmdWindow;   // +0x4A0
    VIVect2I        m_cmdCenter;   // +0x620
    int             m_cmd;         // +0x628: the command list's current entry, 0..33
    float           m_scroll;      // +0x62C: the list's glide, 0..1
    int             m_scrollDir;   // +0x630: -1, 0, 1
    int             m_depth;       // +0x634: VIWndMenu4's depth
};
CHK(c_VIWndMenu4Edit, sizeof(VIWndMenu4Edit) == 1592)

#endif
