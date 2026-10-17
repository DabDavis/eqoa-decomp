// cl_wm.h -- windows: VIWndMerchant, VIWndMercMenu, VIWndMessage, VIWndMiniMenu, VIWndMoneyMenu, VIWndNetStat, VIWndOptionList, VIWndOptions, VIWndPrivChat, VIWndQuantity.
// CLIENT 0x00C77748..0x00C860C8.
// TUs (one window class each, ending with its __tf):
//   VIWndMerchant              0x00C77748..0x00C79FE0
//   VIWndMercMenu              0x00C79FE0..0x00C7B318
//   VIWndMessage               0x00C7B318..0x00C7BB40
//   VIWndMiniMenu              0x00C7BB40..0x00C7D8D8
//   VIWndMoneyMenu             0x00C7D8D8..0x00C7EB08
//   VIWndNetStat               0x00C7EB08..0x00C7FFC8
//   VIWndOptionList            0x00C7FFC8..0x00C80618
//   VIWndOptions               0x00C80618..0x00C83858
//   VIWndPrivChat              0x00C83858..0x00C84878
//   VIWndQuantity              0x00C84878..0x00C860C8
#ifndef CL_WM_H
#ifndef CHK
#define CHK(n, c)  typedef char n[(c) ? 1 : -1];
#endif
#define CL_WM_H

extern "C" double pow(double x, double y);
extern "C" int sprintf(char *s, const char *fmt, ...);

// Skeletons (lanes of 2026-10-16): fill each in as its TU is written.
// VIWndMerchant: ctor 0x00C77750, 2480 bytes (`new`), vtable 0x00CECB40;
//   slots [0]0x00C79F90 [1]0x00C777F0 [2]OnDraw__5VIWnd [3]0x00C77B78 [4]0x00C77FF0 [5]0x00C77EB8 [6]0x00C78058
// A stat's name entry in cl-uitext's table (0x00CE0188, 19 entries of {word, stat}): 0 if
// the stat has none. by address.

// VIVect2I has no operator- in vi.h (SUPPORT never subtracts one); MiniMenu's pointer offset
// is `pos - VIVect2I(25, h / 2)`: the operand's temp at sp+80 next to the argument copy.
// [merge] now vi.h's member VIVect2I::operator- (lane cl-wh); this free inline was dropped.


// One line of the merchant's stat comparison: the text, coloured by whether wearing the
// item raises (blue) or lowers (red) the stat.  80 bytes.
struct VIMerchantLine
{
    unsigned short m_text[32];    // +0x00
    VIColor32F     m_color;       // +0x40
};

// The merchant: the buy/sell menu (VIWndMercMenu), a VIWndList of the merchant's or the
// inventory's items, the quantity picker and a confirmation box, and the selected item's
// stats, description, price and the player's coin.  VIWndGame::MerchantOpen calls Open.
// 2480 bytes.  Names provisional.
class VIWndMerchant : public VIWnd
{
public:
    VIWndMerchant();                                             // 0x00C77750
    virtual ~VIWndMerchant();                                    // 0x00C777F0

    int  Init();                                                 // 0x00C77850
    int  Free();                                                 // 0x00C77A60

    virtual void OnDrawSelf();                                   // 0x00C77B78
    virtual void OnKeyDown(int key);                             // 0x00C77EB8
    virtual void OnKeyUp(int key);                               // 0x00C77FF0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C78058

    void Buy(int item, int n);                                   // 0x00C783C8
    void Sell(int item, int n);                                  // 0x00C78430: a box first for a worn/quick-slot/no-trade item
    void Open();                                                 // 0x00C785D0: the buy/sell menu
    void ShowList();                                             // 0x00C78670
    void FillInvList();                                          // 0x00C78700
    void FillMerchantList();                                     // 0x00C78870
    void DrawInfo();                                             // 0x00C78990
    void ClearInfo();                                            // 0x00C78DC0
    void UpdateInfo();                                           // 0x00C78E28
    void SetPrice(int price);                                    // 0x00C79318: and the player's coin

    int             m_mode;        // +0x024  0 the menu, 1 selling (the inventory), 2 buying
    int             m_coin;        // +0x028  the player's money (VIPlayer +0x34)
    int             m_price;       // +0x02C  the selected item's, -1 none
    VIVect2I        m_x30;         // +0x030
    VIWndMercMenu  *m_menu;        // +0x038
    VIWndList      *m_list;        // +0x03C
    VIWndQuantity  *m_quantity;    // +0x040
    VIWndMBox      *m_mbox;        // +0x044
    unsigned short  m_stats[512];  // +0x048  the item's text (VIInfoText)
    unsigned short  m_desc[256];   // +0x448  its description
    VIMerchantLine  m_lines[6];    // +0x648
    VIUIWindow      m_window;      // +0x828
    VIInfoText      m_info;        // +0x9A8
};
CHK(c_VIWndMerchant, sizeof(VIWndMerchant) == 0x9B0)

// VIWndMercMenu: ctor 0x00C7A000, 944 bytes (`new`), vtable 0x00CECE38;
//   slots [0]0x00C7B2C8 [1]0x00C7A048 [2]OnDraw__5VIWnd [3]0x00C7A0D0 [4]OnKeyUp__5VIWndi [5]0x00C7A350 [6]0x00C7A4E0
// The merchant's first menu: its greeting over Buy (word 63), Sell (62) and Leave (161).
// Enter sends the parent (VIWndMerchant) 281 to buy, 280 to sell; Leave and Triangle close
// the merchant (VIClient::MerchantClose).  944 bytes.  Names provisional.
class VIWndMercMenu : public VIWnd
{
public:
    VIWndMercMenu();                                             // 0x00C7A000
    virtual ~VIWndMercMenu();                                    // 0x00C7A048

    int  Init();                                                 // 0x00C7A0A0
    int  Free();                                                 // 0x00C7A0B8

    virtual void OnDrawSelf();                                   // 0x00C7A0D0
    virtual void OnKeyDown(int key);                             // 0x00C7A350
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C7A4E0

    void Move(int d);                                            // 0x00C7A618
    void Refresh();                                              // 0x00C7A6A0: the greeting, expanded

    // Inline: emitted after __tf13VIWndMercMenu (0x00C7B318).
    void ResetSel() { m_sel = 0; }

    int            m_sel;         // +0x024  0 Buy, 1 Sell, 2 Leave
    VIUIWindow     m_window;      // +0x028
    unsigned short m_text[256];   // +0x1A8  the merchant's greeting
    int            m_rows;        // +0x3A8  its rows
};
CHK(c_VIWndMercMenu, sizeof(VIWndMercMenu) == 0x3B0)

// VIWndMessage: ctor 0x00C7B320, 1152 bytes (`new`), vtable 0x00CECEB0;
//   slots [0]0x00C7BAF0 [1]0x00C7B368 [2]OnDraw__5VIWnd [3]0x00C7B3D0 [4]0x00C7B840 [5]0x00C7B6F0 [6]0x00C7B848
// STATION's declaration (stwnd.h), copied by the coordinator because wd/wq/ws EMBED it by
// value; CLIENT's `new` says 1152 = 0x480 too.  Addresses in the comments are STATION's.
enum VIMESSAGEPROMPT
{
    VIMESSAGEPROMPT_OK = 0,         // "Ok" (786)
    VIMESSAGEPROMPT_OKCANCEL = 1,   // "Ok" (786) / 213
    VIMESSAGEPROMPT_YESNO = 2       // "Yes" (21) / "No" (22)
};

class VIWndMessage : public VIWnd
{
public:
    VIWndMessage();                                              // 0x002F1DB8
    virtual ~VIWndMessage();                                     // 0x002F1E00

    int  Init();                                                 // 0x002F1E50
    void Free();                                                 // 0x002F1E60

    virtual void OnDrawSelf();                                   // 0x002F1E68
    virtual void OnKeyDown(int key);                             // 0x002F2188
    virtual void OnKeyUp(int key);                               // 0x002F22D8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002F22E0

    void Setup(const char *title, const char *text, VIMESSAGEPROMPT prompt,
               const VIVect2I &center, int width);               // 0x002F23A0
    void Setup(const char *title, const unsigned short *text, VIMESSAGEPROMPT prompt,
               const VIVect2I &center, int width);               // 0x002F2418
    void Setup(const unsigned short *title, const unsigned short *text,
               VIMESSAGEPROMPT prompt, const VIVect2I &center, int width);   // 0x002F2498

    // Inline: its copy is emitted after __tf12VIWndMessage (0x002F25D8).
    int Selection() const { return m_sel; }

    VIUIWindow      m_window;     // +0x028
    unsigned short  m_title[32];  // +0x1A8
    unsigned short  m_text[256];  // +0x1E8
    int             m_sel;        // +0x3E8  0 = the first choice
    VIVect2I        m_center;     // +0x3EC
    int             m_width;      // +0x3F4
    int             m_rows;       // +0x3F8  text rows + 3
    VIMESSAGEPROMPT m_prompt;     // +0x3FC
    unsigned short  m_buf[64];    // +0x400  SETUP's Highlight buffer; unused here
};
CHK(c_VIWndMessage, sizeof(VIWndMessage) == 0x480)

// VIWndMiniMenu: ctor 0x00C7BB48, 864 bytes (`new`), vtable 0x00CED1A0;
//   slots [0]0x00C7D888 [1]0x00C7D868 [2]OnDraw__5VIWnd [3]0x00C7BBC0 [4]0x00C7CAD0 [5]0x00C7C9A8 [6]0x00C7CB10 [7]0x00C6AA18 [8]0x00C6AA20 [9]0x00C6AA28 [10]0x00C694E8
// A popup VIWndList over a VIMenuDef (VIWndMenu's submenus; the buddy, guild and recipe
// windows' menus).  Kind 0 grows its frame out of m_pos to fit the rows; kind 1 has a
// title bar (SetTitle) and a button legend under the rows.  Parent hears 261 (an entry's
// m_msg, m_id) and 260 (Escape).  No user destructor.  Method and field names ours.
class VIWndMiniMenu : public VIWndList
{
public:
    VIWndMiniMenu();                                             // 0x00C7BB48
    virtual void OnDrawSelf();                                   // 0x00C7BBC0
    void DrawPlain();                                            // 0x00C7BC50: kind 0
    void DrawTitled();                                           // 0x00C7C118: kind 1
    void DrawItem(VIRec24_C6A8B8 *item, const VIVect2I &pos, int lit);  // 0x00C7C818
    virtual void OnKeyDown(int key);                             // 0x00C7C9A8
    virtual void OnKeyUp(int key);                               // 0x00C7CAD0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C7CB10
    void SetMenu(VIMenuDef *menu);                               // 0x00C7CBE0
    void SetTitle(const unsigned short *s);                      // 0x00C7CBE8
    void SetTitle(const char *s);                                // 0x00C7CC10
    void SetTitle(unsigned short word);                          // 0x00C7CC38: a WORDS.TXT id

    // Inline: emitted after __tf13VIWndMiniMenu (0x00C7D8D8 ..), in this order.
    unsigned short *GetTitle() { return m_title; }               // 0x00C7D8D8
    void SetKind(unsigned char k) { m_kind = k; }                // 0x00C7D8E0

    VIMenuDef      *m_menu;        // +0x250
    unsigned char   m_kind;        // +0x254: 0 DrawPlain, 1 DrawTitled
    unsigned short  m_title[128];  // +0x256
    VIVect2I        m_size;        // +0x358: the frame's size as drawn
};
CHK(c_VIWndMiniMenu, sizeof(VIWndMiniMenu) == 864)

// VIWndMoneyMenu: ctor 0x00C7D8E8, 424 bytes (`new`), vtable 0x00CED4C0;
//   slots [0]0x00C7EAB8 [1]0x00C7D928 [2]OnDraw__5VIWnd [3]0x00C7D9A0 [4]OnKeyUp__5VIWndi [5]0x00C7DBF0 [6]0x00C7DDF8
// A three-way money menu (words 351, 352 and 217 under the title 349): Enter sends the
// parent 291, 292 or 260; Triangle 260.  424 bytes.  Names provisional.
class VIWndMoneyMenu : public VIWnd
{
public:
    VIWndMoneyMenu();                                            // 0x00C7D8E8
    virtual ~VIWndMoneyMenu();                                   // 0x00C7D928

    int  Init();                                                 // 0x00C7D980
    int  Free();                                                 // 0x00C7D990

    virtual void OnDrawSelf();                                   // 0x00C7D9A0
    virtual void OnKeyDown(int key);                             // 0x00C7DBF0
    void Move(int d);                                            // 0x00C7DD70
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C7DDF8

    // Inline: emitted after __tf14VIWndMoneyMenu (0x00C7EB08).
    void ResetSel() { m_sel = 0; }

    int        m_sel;       // +0x024
    VIUIWindow m_window;    // +0x028
};
CHK(c_VIWndMoneyMenu, sizeof(VIWndMoneyMenu) == 0x1A8)

// VIWndNetStat: ctor 0x00C7EB10, 44 bytes (`new`), vtable 0x00CED7C0;
//   slots [0]0x00C7FF78 [1]0x00C7EB50 [2]OnDraw__5VIWnd [3]0x00C7F248 [4]OnKeyUp__5VIWndi [5]OnKeyDown__5VIWndi [6]OnMessage__5VIWndP5VIWndiii
// The network meter in the screen's corner: a 128 x 32 texture of the last 64 sends and
// receives as two rows of ticks, and the ping.  VIWndGame::ShowNetStats feeds it.
// 44 bytes.  Names provisional.
class VIWndNetStat : public VIWnd
{
public:
    VIWndNetStat();                                              // 0x00C7EB10
    virtual ~VIWndNetStat();                                     // 0x00C7EB50

    int  UpdateTexture(unsigned long sent, unsigned long recv, int phase);   // 0x00C7EBB8
    int  CreateTexture();                                        // 0x00C7EF88: no caller in CLIENT
    void SetData(long age, unsigned long sent, unsigned long recv, unsigned short number,
                 int ping);                                      // 0x00C7F1A8
    virtual void OnDrawSelf();                                   // 0x00C7F248

    int m_mat;       // +0x024  the meter's material, -1 none
    int m_ping;      // +0x028
};
CHK(c_VIWndNetStat, sizeof(VIWndNetStat) == 0x2C)

// VIWndOptionList: ctor 0x00C7FFC8, 520 bytes (`new`), vtable 0x00CED838;
//   slots [0]0x00C805C8 [1]0x00C80010 [2]OnDraw__5VIWnd [3]0x00C80078 [4]0x00C80358 [5]0x00C801C8 [6]0x00C80360
// STATION's declaration (stwnd.h); addresses in its comments are STATION's.
class VIWndOptionList : public VIWnd
{
public:
    VIWndOptionList();                                           // 0x002F25E0
    virtual ~VIWndOptionList();                                  // 0x002F2628

    int  Init();                                                 // 0x002F2678
    void Free();                                                 // 0x002F2688

    virtual void OnDrawSelf();                                   // 0x002F2690
    virtual void OnKeyDown(int key);                             // 0x002F27E0
    virtual void OnKeyUp(int key);                               // 0x002F2970
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002F2978

    // 0x002F2A20 (name ours): at least 300 wide, and wide enough for the title
    // and every choice plus 80.
    void Setup(const unsigned short *title, const unsigned short **options, int count,
               const VIVect2I &center, int width);

    // Inline: its copy is emitted after __tf15VIWndOptionList (0x002F2C30).
    int Selection() const { return m_sel; }

    VIUIWindow             m_window;     // +0x028
    unsigned short         m_title[32];  // +0x1A8
    const unsigned short **m_options;    // +0x1E8  the caller's
    int                    m_count;      // +0x1EC
    int                    m_sel;        // +0x1F0
    VIVect2I               m_center;     // +0x1F4
    int                    m_width;      // +0x1FC
    int                    m_rows;       // +0x200  m_count + 3
};
CHK(c_VIWndOptionList, sizeof(VIWndOptionList) == 0x208)

// VIWndOptions: ctor 0x00C80620, 2432 bytes (`new`), vtable 0x00CEDBF8;
//   slots [0]0x00C83808 [1]0x00C80680 [2]OnDraw__5VIWnd [3]0x00C80810 [4]0x00C810E8 [5]0x00C80D60 [6]0x00C810F0
// SETUP's declaration (setup.h); CLIENT's `new` says 2432 = 0x980 too.
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
CHK(c_VIWndOptions, sizeof(VIWndOptions) == 0x980)

// VIWndPrivChat: ctor 0x00C83858, 1456 bytes (`new`), vtable 0x00CEDEF8;
//   slots [0]0x00C84828 [1]0x00C838A0 [2]OnDraw__5VIWnd [3]0x00C83910 [4]OnKeyUp__5VIWndi [5]0x00C83A28 [6]0x00C83B38
// A private-chat notice: a text box with an Ok legend (word 280).  Enter sends the parent
// 279, Triangle 260.  1456 bytes.  Names provisional.
class VIWndPrivChat : public VIWnd
{
public:
    VIWndPrivChat();                                             // 0x00C83858
    virtual ~VIWndPrivChat();                                    // 0x00C838A0

    int  Init();                                                 // 0x00C838F0
    void Clear();                                                // 0x00C83900

    virtual void OnDrawSelf();                                   // 0x00C83910
    virtual void OnKeyDown(int key);                             // 0x00C83A28
    void SetText(const unsigned short *s);                       // 0x00C83AF0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C83B38

    int            m_x24;         // +0x024
    VIUIWindow     m_window;      // +0x028
    unsigned short m_text[512];   // +0x1A8
    int            m_rows;        // +0x5A8  the text's rows + 2
    void SetPos(const VIVect2I &p) {}                            // 0x00C84878: empty (VIWndGame::Init builds the argument, nothing is stored)
};
CHK(c_VIWndPrivChat, sizeof(VIWndPrivChat) == 0x5B0)

// VIWndQuantity: ctor 0x00C84880, 504 bytes (`new`), vtable 0x00CEE200;
//   slots [0]0x00C86078 [1]0x00C848E0 [2]OnDraw__5VIWnd [3]0x00C84990 [4]0x00C84F08 [5]0x00C84D58 [6]0x00C853A0
// The quantity picker the item windows open (VIWndBankItem, VIWndItemMenu, VIWndMerchant,
// VIWndTrade: Init, then Open): up to four digits, the cursor on one; up/down change it,
// left/right move between digits (repeating through m_keys); Enter sends the parent 293
// (the value clamped to [0, m_max], m_id), Triangle 260.  With a unit price it shows the
// total.  504 bytes.  Names provisional.
class VIWndQuantity : public VIWnd
{
public:
    VIWndQuantity();                                             // 0x00C84880
    virtual ~VIWndQuantity();                                    // 0x00C848E0

    int  Init();                                                 // 0x00C84940
    int  Free();                                                 // 0x00C84968

    virtual void OnDrawSelf();                                   // 0x00C84990
    virtual void OnKeyDown(int key);                             // 0x00C84D58
    virtual void OnKeyUp(int key);                               // 0x00C84F08

    void Open(int max, int id, int price);                       // 0x00C84FD8: price -1 = no total
    void MoveCursor(int d);                                      // 0x00C850D8
    void ChangeDigit(int d);                                     // 0x00C85180
    int  GetValue();                                             // 0x00C85248
    void SetMax();                                               // 0x00C852C8: the digits := m_max

    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C853A0

    int         m_cursor;       // +0x024  the digit under the cursor, 0..3 (3 = units)
    int         m_max;          // +0x028
    int         m_digitsShown;  // +0x02C  the digits m_max needs, 1..4
    int         m_id;           // +0x030  handed back with 293
    int         m_price;        // +0x034  -1 = no total row
    char        m_digits[4];    // +0x038  most significant first
    VIUIWindow  m_window;       // +0x040
    VIKeyFlash  m_keys;         // +0x1C0  the pad's left/right/up/down repeat
};
CHK(c_VIWndQuantity, sizeof(VIWndQuantity) == 0x1F8)

#endif
