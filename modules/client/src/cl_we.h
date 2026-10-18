// cl_we.h -- windows: VIWndDeposit, VIWndDisconnect, VIWndEdit, VIWndExperiment, VIWndFaction, VIWndFishing.
// CLIENT 0x00C41A48..0x00C4E5F8.
// TUs (one window class each, ending with its __tf):
//   VIWndDeposit               0x00C41A48..0x00C43498
//   VIWndDisconnect            0x00C43498..0x00C44448
//   VIWndEdit                  0x00C44448..0x00C47D88
//   VIWndExperiment            0x00C47D88..0x00C49D90
//   VIWndFaction               0x00C49D90..0x00C4BB28
//   VIWndFishing               0x00C4BB28..0x00C4E5F8
#ifndef CL_WE_H
#define CL_WE_H
#define CL_WE_CHK(n, c)  typedef char n[(c) || VI_PORT ? 1 : -1];

#ifndef CHK
#define CHK(n, c)  typedef char n[(c) || VI_PORT ? 1 : -1];
#endif

#define CL_WE_CHK(n, c)  typedef char n[(c) || VI_PORT ? 1 : -1];

#define CL_WE_CHK(n, c)  typedef char n[(c) || VI_PORT ? 1 : -1];

// VIWndDeposit: ctor 0x00C41A60, 528 bytes (`new`), vtable 0x00CE6C38;
//   slots [0]0x00C43448 [1]0x00C41AC0 [2]OnDraw__5VIWnd [3]0x00C41B60 [4]0x00C42310 [5]0x00C42070 [6]0x00C42388
// The money-amount entry (Bank deposit/withdraw, Trade): nine digits picked with the pad,
// the cursor moving over the last m_width of them.  0x210 bytes.  VIWndBankMoney (cl_wa.h) and
// VIWndTrade (cl_ws.h) set m_cursor/m_carried/m_banked directly, then call Setup(mode).
// Mode 1 deposits (limit m_carried), 2 withdraws (limit m_banked), 3 offers in a trade
// (limit m_carried); Done sends the parent 294 (amount) in mode 3, deposits/withdraws
// through the bank otherwise.  Names provisional.
class VIWndDeposit : public VIWnd
{
public:
    VIWndDeposit();                                              // 0x00C41A60
    virtual ~VIWndDeposit();                                     // 0x00C41AC0
    int  Init();                                                 // 0x00C41B20
    int  Free();                                                 // 0x00C41B40
    virtual void OnDrawSelf();                                   // 0x00C41B60
    virtual void OnKeyDown(int key);                             // 0x00C42070
    virtual void OnKeyUp(int key);                               // 0x00C42310
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C42388
    void MoveCursor(int d);                                      // 0x00C42488
    void ClearDigits();                                          // 0x00C42528
    void ChangeDigit(int d);                                     // 0x00C42558
    int  GetValue();                                             // 0x00C42668
    void SetMax();                                               // 0x00C426E0: the digits := the limit
    void Setup(int mode);                                        // 0x00C427C0: m_width := the limit's digit count

    // Inline: emitted after __tf12VIWndDeposit in this order (0x00C43498 ..).
    void SetCursorLast() { m_cursor = 8; }
    void SetCarried(int v) { m_carried = v; }
    void SetBanked(int v) { m_banked = v; }

    int        m_mode;          // +0x024
    int        m_cursor;        // +0x028: 9 - m_width .. 8
    int        m_carried;       // +0x02C: the limit in modes 1 and 3
    int        m_banked;        // +0x030: the limit in mode 2
    int        m_width;         // +0x034
    VIUIWindow m_window;        // +0x038
    VIKeyFlash m_flash;         // +0x1B8
    int        m_digit[9];      // +0x1EC: most significant first
};
CL_WE_CHK(c_VIWndDeposit, sizeof(VIWndDeposit) == 0x210)

// VIWndDisconnect: ctor ?, None bytes (`new`), vtable 0x00CE6F30;
//   slots [0]0x00C443F8 [1]0x00C434F8 [2]OnDraw__5VIWnd [3]0x00C43550 [4]0x00C436F8 [5]0x00C43700 [6]0x00C43758
// VIWndEdit: ctor 0x00C44448, 448 bytes (`new`), vtable 0x00CE7510;
//   slots [0]0x00C47D38 [1]0x00C445A0 [2]OnDraw__5VIWnd [3]0x00C44788 [4]0x00C46640 [5]0x00C46118 [6]0x00C46690
// STATION's class (modules/station/src/stedit.h), the same source: 0x1C0 bytes, vptr at
// +0x20.  CLIENT's copy differs from STATION's in OnDrawSelf and OnMessage only, which read
// the CLIENT clock (VIClient+0x7568) where STATION calls VITimer::GetTime().  A text box
// above an 11 x 6 soft keyboard; Done sends the parent 259 (p1, p2 = m_x24, m_x28), cancel
// 260.  The method names are STATION's (ours).
struct VIEditLine
{
    unsigned short *m_start;   // +0x0
    short           m_len;     // +0x4
    short           m_break;   // +0x6
};

class VIWndEdit : public VIWnd
{
public:
    VIWndEdit();                                 // 0x00C44448
    virtual ~VIWndEdit();                        // 0x00C445A0

    void SetMaxLength(int n);                    // 0x00C44608
    void SetRows(int n);                         // 0x00C446E0
    void SetMultiLine(int on);                   // 0x00C446E8  shows the soft keyboard's newline key
    int  MultiLine();                            // 0x00C44710
    void SetAllowSpaces(int on);                 // 0x00C44718
    int  AllowSpaces();                          // 0x00C44720
    void SetTitle(const char *s);                // 0x00C44728
    void SetTitle(const unsigned short *s);      // 0x00C44758

    virtual void OnDrawSelf();                   // 0x00C44788

    int  InsertChar(unsigned short c);           // 0x00C45DC0
    void Clear();                                // 0x00C45ED0
    void SetCenter(const VIVect2I &p);           // 0x00C45EF8
    void SetText(const unsigned short *s);       // 0x00C45F28
    void SetText(const char *s);                 // 0x00C45F70
    int  CursorRight();                          // 0x00C45FB8
    int  CursorLeft();                           // 0x00C45FF8
    int  Backspace();                            // 0x00C46038
    void PlaySound(int ok);                      // 0x00C460E8

    virtual void OnKeyDown(int key);                             // 0x00C46118
    virtual void OnKeyUp(int key);                               // 0x00C46640
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C46690

    int  GetCursor();                            // 0x00C46778
    int  SetCursor(int pos);                     // 0x00C46780  returns whether it moved
    void WrapLines();                            // 0x00C467F0
    int  CursorLineCol(VIVect2I &out);           // 0x00C46AC8  out = (column, line); -1 past the end
    int  LineUp();                               // 0x00C46BB8
    int  LineDown();                             // 0x00C46D98
    void MoveKey(int dx, int dy);                // 0x00C46F90  the soft keyboard's cursor

    // Inline: emitted after __tf9VIWndEdit in this order (0x00C47D88 ..).
    float KeyGlow() { return m_x168; }
    void  SetPos(const VIVect2I &p) { m_pos = p; }
    unsigned short *GetText() { return m_text; }
    void  SetX24(int v) { m_x24 = v; }
    void  SetX28(int v) { m_x28 = v; }
    void  SetX18C(int v) { m_x18C = v; }
    void  ClearX24() { m_x24 = 0; m_x28 = 0; }
    VIVect2I GetPos() { return m_pos; }
    VIVect2I GetEnd() { return m_pos + m_size; }   // operator+: VIWndCharCreateName reads .y through &m_pos/&m_size
    void  Touch() { m_lastInput = VITimer::GetTime(); }

    int             m_x24;          // +0x024  handed back with 259
    int             m_x28;          // +0x028
    int             m_x2C;          // +0x02C  a shift
    int             m_shift;        // +0x030  R1 held
    int             m_caps;         // +0x034  the caps key's toggle
    VIVect2I        m_pos;          // +0x038
    VIVect2I        m_size;         // +0x040
    unsigned short  m_title[128];   // +0x048
    VIVect2I        m_key;          // +0x148  the soft keyboard's cursor (column, row)
    VIVect2I        m_lastKey;      // +0x150
    VIVect2I        m_pressed;      // +0x158  the key X last typed
    float           m_press;        // +0x160  its flash, 1 when typed
    float           m_glide;        // +0x164  the keyboard cursor's glide, 0..1
    float           m_x168;         // +0x168
    float           m_pulse;        // +0x16C  the Done key's glow, 0..1 and back
    int             m_x170;         // +0x170
    int             m_x174;         // +0x174
    long            m_x178;         // +0x178  when the focus arrived
    int             m_repeatKey;    // +0x180  the pad direction auto-repeating
    float           m_repeatDelay;  // +0x184
    int             m_rows;         // +0x188
    int             m_x18C;         // +0x18C
    int             m_x190;         // +0x190
    int             m_cursor;       // +0x194
    int             m_wantX;        // +0x198  LineUp/LineDown's pixel column, -1 none
    int             m_x19C;         // +0x19C
    long            m_lastInput;    // +0x1A0
    unsigned short *m_text;         // +0x1A8
    int             m_maxLen;       // +0x1AC
    VIEditLine     *m_lines;        // +0x1B0
    int             m_numLines;     // +0x1B4
    int             m_multiLine;    // +0x1B8
    int             m_allowSpaces;  // +0x1BC
};
CL_WE_CHK(c_VIWndEdit, sizeof(VIWndEdit) == 0x1C0)

// VIWndExperiment: ctor 0x00C47E40, 6216 bytes (`new`), vtable 0x00CE7820;
//   slots [0]0x00C49D40 [1]0x00C47E90 [2]OnDraw__5VIWnd [3]0x00C48168 [4]OnKeyUp__5VIWndi [5]0x00C48908 [6]0x00C48728
// The experiment window (crafting): the items VIClientPart3834C holds, listed through a
// VIWndMenu with three options each (0xA7030000..2: use, remove, inspect), the pool of
// known recipes beside it, an inspect window and a message box.  0x1848 bytes.  Names
// provisional.
class VIWndExperiment : public VIWnd
{
public:
    VIWndExperiment();                                           // 0x00C47E40
    virtual ~VIWndExperiment();                                  // 0x00C47E90
    int  Init();                                                 // 0x00C47EF0
    int  Free();                                                 // 0x00C48098
    virtual void OnDrawSelf();                                   // 0x00C48168
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C48728
    virtual void OnKeyDown(int key);                             // 0x00C48908
    void Open();                                                 // 0x00C489C8: VIWndGame opens it
    void Refresh();                                              // 0x00C48A98: the rows from the part's items
    void SetRow(int item, int row);                              // 0x00C48BA8: item -1 empties the row
    void Use(int item);                                          // 0x00C48E30: option 0xA7030000
    void Remove(int item);                                       // 0x00C48E78: option 0xA7030001
    void Inspect(int item);                                      // 0x00C48EC0: option 0xA7030002
    void Notify(const unsigned short *text, int x);              // 0x00C48F60: VIClientPart3834C calls it
    void Ask(const unsigned short *text, int p1, int p2, int x); // 0x00C49010: ... and this (yes/no)

    VIWndMenu    *m_menu;          // +0x024
    VIWndMBox    *m_mbox;          // +0x028
    VIWndInspect *m_inspect;       // +0x02C
    VIUIWindow    m_window;        // +0x030
    VIInfoText    m_info;          // +0x1B0
    VIMenuDef     m_list;          // +0x1B4: its entries are m_rows
    VIMenuEntry   m_rows[40];      // +0x1C4
    VIMenuDef     m_subs[40];      // +0x6C4: row i's options are m_opts[i]
    VIMenuEntry   m_opts[40][3];   // +0x944
    int           m_p1844;         // +0x1844
};
CL_WE_CHK(c_VIWndExperiment, sizeof(VIWndExperiment) == 0x1848)

// VIWndFaction: ctor 0x00C49D90, 1256 bytes (`new`), vtable 0x00CE7DD0;
//   slots [0]0x00C4BAD8 [1]0x00C49DF0 [2]OnDraw__5VIWnd [3]0x00C49E80 [4]0x00C4A7E0 [5]0x00C4A6A8 [6]0x00C4A858
// The faction standings: a page of factions (VIClientPart4F078 asks the server for
// each page) with a coloured bar per standing, a detail box (m_win3) for the selected one
// and a "waiting" box (m_win2).  0x4E8 bytes.  Names provisional.
class VIWndFaction : public VIWnd
{
public:
    VIWndFaction();                                              // 0x00C49D90
    virtual ~VIWndFaction();                                     // 0x00C49DF0
    int  Init();                                                 // 0x00C49E68: VIWndGame calls it
    virtual void OnDrawSelf();                                   // 0x00C49E80
    virtual void OnKeyDown(int key);                             // 0x00C4A6A8
    virtual void OnKeyUp(int key);                               // 0x00C4A7E0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C4A858
    void NextPage();                                             // 0x00C4A9D8
    void PrevPage();                                             // 0x00C4AA88
    void Up();                                                   // 0x00C4AB20
    void Down();                                                 // 0x00C4ABA0
    void Select();                                               // 0x00C4AC40: m_mode 1 shows the detail box
    void DrawStanding(int value, int x, int row, int w);         // 0x00C4ACD0

    int        m_mode;          // +0x024: 1 while the detail box is up
    VIUIWindow m_window;        // +0x028
    VIUIWindow m_win2;          // +0x1A8: "waiting for the list"
    VIUIWindow m_win3;          // +0x328: the selected faction
    VIClient  *m_app;           // +0x4A8: set on focus
    int        m_sel;           // +0x4AC
    VIKeyFlash m_flash;         // +0x4B0
};
CL_WE_CHK(c_VIWndFaction, sizeof(VIWndFaction) == 0x4E8)

// VIWndFishing: ctor 0x00C4BB28, 2064 bytes (`new`), vtable 0x00CE80D8;
//   slots [0]0x00C4E5A8 [1]0x00C4BBA8 [2]OnDraw__5VIWnd [3]0x00C4BDB0 [4]0x00C4C6C8 [5]0x00C4C4C0 [6]0x00C4C788
// The fishing window: cast with the power meter, the bait list (a VIWndList), a name edit
// box, the chat window while a key is held, and a splash effect (sTargetEmitter, the
// file's static VIParticleEmitter) where the line lands.  0x810 bytes.  Names provisional.
class VIWndFishing : public VIWnd
{
public:
    VIWndFishing();                                              // 0x00C4BB28
    virtual ~VIWndFishing();                                     // 0x00C4BBA8
    int  Init(VIScene *scene);                                   // 0x00C4BC20: VIWndGame calls it
    int  Free();                                                 // 0x00C4BD18
    virtual void OnDrawSelf();                                   // 0x00C4BDB0
    virtual void OnKeyDown(int key);                             // 0x00C4C4C0
    virtual void OnKeyUp(int key);                               // 0x00C4C6C8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C4C788
    void ResetCast();                                            // 0x00C4CA90
    void Cast();                                                 // 0x00C4CAB0
    void Tick();                                                 // 0x00C4CB18: the power meter, the splash
    void Splash();                                               // 0x00C4CCB8
    void FillBait();                                             // 0x00C4CE20
    void OpenBait();                                             // 0x00C4D010
    void CloseBait();                                            // 0x00C4D100
    void Describe();                                             // 0x00C4D148
    void Rename();                                               // 0x00C4D1C8
    void Renamed();                                              // 0x00C4D258
    void SetText(const char *s);                                 // 0x00C4D2B0: shown for 5 s
    void SetText(const unsigned short *s);                       // 0x00C4D300
    int  FindWater(int actor, const VIVect3 &pos, const VIVect3 &hpr, VIScene *sc, VIVect3 &out); // 0x00C4D688
    void ChatOn();                                               // 0x00C4D8E0
    void ChatOff();                                              // 0x00C4D920

    // Inline: emitted after __tf12VIWndFishing (0x00C4E5F8).
    void SetChat(VIWndChat *c) { m_chat = c; }

    int            m_p024;          // +0x024
    VIUIWindow     m_window;        // +0x028
    VIUIWindow     m_win2;          // +0x1A8: the bait box
    VIUIWindow     m_win3;          // +0x328: the message box
    VIWndList     *m_list;          // +0x4A8
    VIWndEdit     *m_edit;          // +0x4AC
    VIWndChat     *m_chat;          // +0x4B0
    unsigned short m_desc[256];     // +0x4B4
    unsigned short m_text[128];     // +0x6B4
    int            m_p7B4;          // +0x7B4
    long           m_textEnd;       // +0x7B8
    VIInfoText     m_info;          // +0x7C0
    int            m_hit;           // +0x7C4
    int            m_chatOn;        // +0x7C8
    int            m_chatKey;       // +0x7CC
    int            m_casting;       // +0x7D0
    int            m_p7D4;          // +0x7D4
    long           m_castTime;      // +0x7D8
    int            m_p7E0;          // +0x7E0
    int            m_power;         // +0x7E4
    int            m_ready;         // +0x7E8
    int            m_splashing;     // +0x7EC
    long           m_splashTime;    // +0x7F0
    int            m_release;       // +0x7F8
    int            m_onWater;       // +0x7FC
    VIVect3        m_target;        // +0x800
    VIScene       *m_scene;         // +0x80C
};
CL_WE_CHK(c_VIWndFishing, sizeof(VIWndFishing) == 0x810)

#endif
