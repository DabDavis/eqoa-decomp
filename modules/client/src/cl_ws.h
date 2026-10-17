// cl_ws.h -- windows: VIWndSpellBook, VIWndStartMenu, VIWndTell, VIWndTrade, VIWndTradeList, VIWndTrainer, VIWndWhoList.
// CLIENT 0x00C93680..0x00CA4658.
// TUs (one window class each, ending with its __tf):
//   VIWndSpellBook             0x00C93680..0x00C97D80
//   VIWndStartMenu             0x00C97D80..0x00C991B0
//   VIWndTell                  0x00C991B0..0x00C9A878
//   VIWndTrade                 0x00C9A878..0x00C9D5A8
//   VIWndTradeList             0x00C9D5A8..0x00C9EA18
//   VIWndTrainer               0x00C9EA18..0x00CA26D8
//   VIWndWhoList               0x00CA26D8..0x00CA4658
#ifndef CL_WS_H
#define CL_WS_H

// Foreign free functions (by-address convention; the owner names them).

// Skeletons (lanes of 2026-10-16): fill each in as its TU is written.
// VIWndSpellBook: ctor 0x00C93680, 2016 bytes (`new`), vtable 0x00CEFF60;
//   slots [0]0x00C97D30 [1]0x00C93738 [2]OnDraw__5VIWnd [3]0x00C93970 [4]0x00C94660 [5]0x00C943E0 [6]0x00C94B18
// The spellbook's rows (placeholder names kept: client_viarray.cpp's VIArray members carry
// them).  One ability, 32 bytes (BuildSpells fills it; copied whole).
struct VIRec32_C97888
{
    int m_id;        // +0x00: the ability
    int m_index;     // +0x04: GetAbilityDisplayIndex, the sort key (-1 sorts last)
    int m_valid;     // +0x08: GetAbilityValid; Assign sets it
    int m_slot;      // +0x0C: GetAbilitySlot
    int m_flags;     // +0x10: 1 the icon, 0x10 the front icon, 0x20 the bar (m_x1C == 140)
    int m_back;      // +0x14: GetAbilityIconBack, -1 none
    int m_front;     // +0x18: GetAbilityIconFront
    int m_x1C;       // +0x1C
};
// One slotted ability, 12 bytes (BuildSlots; sorted by m_slot).
struct VIRec12_C979B0
{
    int m_id;        // +0x00
    int m_index;     // +0x04
    int m_slot;      // +0x08
};
// A tab caption: a WORDS.TXT id (DrawTabs: 1729 + n).
struct VIRec2_C97C68_A2
{
    unsigned short m_word;
};

class VIWndSpellBook : public VIWnd
{
public:
    VIWndSpellBook();                                            // 0x00C93680
    virtual ~VIWndSpellBook();                                   // 0x00C93738
    int  Init();                                                 // 0x00C937B0
    int  Free();                                                 // 0x00C93910
    virtual void OnDrawSelf();                                   // 0x00C93970
    virtual void OnKeyDown(int key);                             // 0x00C943E0
    virtual void OnKeyUp(int key);                               // 0x00C94660
    void Pick();                                                 // 0x00C946D8: mode 0 Enter, the popup menu
    void Place();                                                // 0x00C947D0: mode 1 Enter, swap two (Send003A)
    void Assign();                                               // 0x00C94860: mode 2 Enter, parent hears 273 (index, slot)
    void Down();                                                 // 0x00C948E8
    void Up();                                                   // 0x00C94960
    void Slot(int d);                                            // 0x00C949D8: mode 2, the slot by d
    void Tab(int d);                                             // 0x00C94A98: the level tab by d
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C94B18
    void BuildSpells();                                          // 0x00C94E68
    void BuildSlots();                                           // 0x00C95158
    void DrawIcon(VIRec32_C97888 *r, const VIVect2I &pos, int lit, float alpha);  // 0x00C95370
    void BuildText();                                            // 0x00C956E8: m_text and m_desc
    void ClearText();                                            // 0x00C95900
    void Hold();                                                 // 0x00C95910: the selected row into m_held
    void Open();                                                 // 0x00C95988
    void AddScope(unsigned short *buf, unsigned int size, int v);        // 0x00C959E0
    void AddCastTime(unsigned short *buf, unsigned int size, int v);     // 0x00C95AC8
    void AddCooldown(unsigned short *buf, unsigned int size, int v);     // 0x00C95B50
    void AddPowerCost(unsigned short *buf, unsigned int size, int v);    // 0x00C95BD8
    void AddRange(unsigned short *buf, unsigned int size, float v);      // 0x00C95C60
    void AddEquipReq(unsigned short *buf, unsigned int size, int mask);  // 0x00C95CF8
    void AddRecords(unsigned short *buf, unsigned int size, int id);     // 0x00C95E38
    void AddLevel(unsigned short *buf, unsigned int size, int v);        // 0x00C95FC8
    void Equip();                                                // 0x00C96050: into mode 2
    void ShowList();                                             // 0x00C960A8: back to mode 0
    void ResetFade();                                            // 0x00C960E8
    int  Shown(int id);                                          // 0x00C96128: on the current level tab
    int  Slotted(int id);                                        // 0x00C961C8
    void DrawTabs();                                             // 0x00C96228
    void DrawSlots();                                            // 0x00C964E8: five slots
    void DrawSlots10();                                          // 0x00C96888: ten (expansion bit 2)

    // Inline: emitted after __tf14VIWndSpellBook, in this order.
    void SetPos(const VIVect2I &p) { m_pos = p; }                // 0x00C97D80
    int  GetX3C() { return m_x3C; }                              // 0x00C97D98
    void SetX3C(int v) { m_x3C = v; }                            // 0x00C97DA0

    VIVect2I        m_pos;          // +0x024
    int             m_x2C;          // +0x02C: the held row (Hold), -1 none
    int             m_sel;          // +0x030
    int             m_x34;          // +0x034: mode 2's slot
    int             m_top;          // +0x038
    int             m_x3C;          // +0x03C: 1 the list has the cursor
    int             m_tab;          // +0x040: the level tab (Shown: levels tab*10+1 .. tab*10+10)
    int             m_x44;          // +0x044: 1 while open (the fade runs)
    int             m_fadeDir;      // +0x048
    float           m_fade;         // +0x04C
    long            m_time;         // +0x050
    int             m_mode;         // +0x058: 0 list, 1 holding a spell, 2 choosing a slot
    VIRec32_C97888  m_held;         // +0x05C
    VIArray<VIRec32_C97888> m_spells;   // +0x07C
    int             m_tabCount;     // +0x084: 5, 6 with expansion bit 2
    VIMenuDef       m_menu;         // +0x088: the popup's four entries
    VIMenuEntry     m_entries[4];   // +0x098
    VIWndMiniMenu  *m_popup;        // +0x118
    VIArray<VIRec12_C979B0> m_slots;    // +0x11C
    VIUIWindow      m_window;       // +0x128
    VIKeyFlash      m_flash;        // +0x2A8: held up/down (138, 139)
    unsigned short  m_text[512];    // +0x2DC: name, scope, times, costs, records
    unsigned short  m_desc[128];    // +0x6DC
};

// VIWndStartMenu: ctor 0x00C97DA8, 44 bytes (`new`), vtable 0x00CF02D0;
//   slots [0]0x00C99160 [1]0x00C97DE8 [2]OnDraw__5VIWnd [3]0x00C98138 [4]OnKeyUp__5VIWndi [5]0x00C980C0 [6]0x00C97FA8
class VIWndStartMenu : public VIWnd
{
public:
    VIWndStartMenu();                                            // 0x00C97DA8
    virtual ~VIWndStartMenu();                                   // 0x00C97DE8
    int  Init();                                                 // 0x00C97E30: the two menus
    int  Free();                                                 // 0x00C97F10
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C97FA8
    virtual void OnKeyDown(int key);                             // 0x00C980C0: 27/144 closes (260)
    virtual void OnDrawSelf();                                   // 0x00C98138: the frame
    void ShowMain(int a);                                        // 0x00C98460: hides the item menu
    void ShowItems();                                            // 0x00C984A8: hides the main menu
    void SetMainX24(VIMenuDef *menu);                                      // 0x00C984F8
    void Items34(VIMenuDef *menu);                                              // 0x00C98508: the item menu's +0x34 sub-menu
    void Items38(VIMenuDef *menu);                                              // 0x00C98528
    void Items3C(VIMenuDef *menu);                                              // 0x00C98548
    void Items40(VIMenuDef *menu);                                              // 0x00C98568

    VIWndItemMenu *m_items;                                      // +0x24 (new 1648)
    VIWndMainMenu *m_main;                                       // +0x28 (new 1072)
};

// VIWndTell: ctor 0x00C991B0, 1768 bytes (`new`), vtable 0x00CF06C8;
//   slots [0]0x00C9A828 [1]0x00C991F8 [2]OnDraw__5VIWnd [3]0x00C99368 [4]OnKeyUp__5VIWndi [5]0x00C99578 [6]0x00C996B0
class VIWndTell : public VIWnd
{
public:
    VIWndTell();                                                 // 0x00C991B0
    virtual ~VIWndTell();                                        // 0x00C991F8
    int  Init();                                                 // 0x00C99258
    void Free();                                                 // 0x00C992E8
    virtual void OnDrawSelf();                                   // 0x00C99368
    virtual void OnKeyDown(int key);                             // 0x00C99578
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C996B0
    unsigned short *GetName();                                   // 0x00C997F8
    void SetName(const unsigned short *s);                       // 0x00C99800
    unsigned short *GetText();                                   // 0x00C99828
    void SetText(const unsigned short *s);                       // 0x00C99830
    void Open(const unsigned short *name);                       // 0x00C99858: to name, or ask for one
    void Close();                                                // 0x00C998A8: empty (VIWndGame::CloseTell)
    void Down();                                                 // 0x00C998B0
    void Up();                                                   // 0x00C99920
    void Select();                                               // 0x00C99988
    void TakeText();                                             // 0x00C99BD8: the editor's text into the field

    VIUIWindow      m_window;     // +0x028
    VIWndEdit      *m_edit;       // +0x1A8
    VIWndMessage    m_msg;        // +0x1B0
    int             m_mode;       // +0x630: 0 the name, 1 the text, 2 Send
    unsigned short  m_name[24];   // +0x634
    unsigned short  m_text[64];   // +0x664
};

// VIWndTrade: ctor 0x00C9A878, 872 bytes (`new`), vtable 0x00CF0D90;
//   slots [0]0x00C9D558 [1]0x00C9A908 [2]OnDraw__5VIWnd [3]0x00C9AE90 [4]0x00C9BE70 [5]0x00C9B698 [6]0x00C9C000
class VIWndTrade : public VIWnd
{
public:
    VIWndTrade();                                                // 0x00C9A878
    virtual ~VIWndTrade();                                       // 0x00C9A908
    int  Init();                                                 // 0x00C9A978
    int  Free();                                                 // 0x00C9ACC0
    virtual void OnDrawSelf();                                   // 0x00C9AE90
    virtual void OnKeyDown(int key);                             // 0x00C9B698
    void Up(int key);                                            // 0x00C9B790
    void Down(int key);                                          // 0x00C9B878
    void Left(int key);                                          // 0x00C9B9B0
    void Right(int key);                                         // 0x00C9BAA8
    void Select(int key);                                        // 0x00C9BBC0: offer the item / take it back / money
    void Cancel(int key);                                        // 0x00C9BD28
    void Accept(int key);                                        // 0x00C9BE00
    virtual void OnKeyUp(int key);                               // 0x00C9BE70
    void UpReleased(int key);                                    // 0x00C9BEF0
    void DownReleased(int key);                                  // 0x00C9BF78
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C9C000
    void Ask();                                                  // 0x00C9C278: "<name> wants to trade" (Yes/No)
    void Start();                                                // 0x00C9C3A8: the trade is on
    void Hide();                                                 // 0x00C9C490
    void TheyAccepted();                                         // 0x00C9C4C8
    void ClearAccepted();                                        // 0x00C9C4D8
    void RefreshLists();                                         // 0x00C9C4E8: the three lists from the client's trade state
    void Chat();                                                 // 0x00C9C8A0: the editor for a line of chat
    void SendChat();                                             // 0x00C9C928

    int             m_focus;        // +0x024: -1 closed, 0 inventory, 1 my offer, 2 my money, 3 their offer
    int             m_fadeDir;      // +0x028: 1 up, -1 down (the accept glow)
    float           m_fade;         // +0x02C
    long            m_time;         // +0x030: the client clock at the last draw
    VIWndList      *m_list;         // +0x038: the inventory
    VIWndTradeList *m_mine;         // +0x03C
    VIWndTradeList *m_theirs;       // +0x040
    VIWndMBox      *m_mbox;         // +0x044
    VIWndDeposit   *m_deposit;      // +0x048: the money amount
    VIWndQuantity  *m_quantity;     // +0x04C: how many of a stack
    VIWndEdit      *m_edit;         // +0x050: the chat line
    VIUIWindow      m_window;       // +0x058
    VIUIWindow      m_offer;        // +0x1D8: drawn twice, the two offer frames
    VIInfoText      m_info;         // +0x358
    int             m_theyAccepted; // +0x35C
    int             m_iAccepted;    // +0x360
};

// VIWndTradeList: ctor 0x00C9D5A8, 616 bytes (`new`), vtable 0x00CF1080;
//   slots [0]0x00C9E9C8 [1]0x00C9D620 [2]OnDraw__5VIWnd [3]0x00C9D6D0 [4]0x00C9DC60 [5]0x00C9DBD8 [6]OnMessage__5VIWndP5VIWndiii [7]0x00C6AA18 [8]0x00C6AA20 [9]0x00C6AA28 [10]0x00C694E8
class VIWndTradeList : public VIWndList
{
public:
    VIWndTradeList();                                            // 0x00C9D5A8
    virtual ~VIWndTradeList();                                   // 0x00C9D620
    int  Init();                                                 // 0x00C9D668
    int  Free();                                                 // 0x00C9D6A0
    virtual void OnDrawSelf();                                   // 0x00C9D6D0
    virtual void OnKeyDown(int key);                             // 0x00C9DBD8
    virtual void OnKeyUp(int key);                               // 0x00C9DC60: its default is VIWnd::OnKeyDown
    void Move(int d);                                            // 0x00C9DCE8: the selection by d, clamped

    // Inline: emitted after __tf14VIWndTradeList (0x00C9EA18 ..), in this order.
    void ResetList() { m_sel = 0; m_top = 0; }                   // 0x00C9EA18
    void SetRows(int n) { m_rows = n; }                          // 0x00C9EA28
    void SetTitle(unsigned short id) { m_title = id; }           // 0x00C9EA30
    void SetColor(VIColor32F c) { m_color = c; }                 // 0x00C9EA38

    int            m_rows;       // +0x250: the rows shown (Fit's argument)
    unsigned short m_title;      // +0x254: a WORDS.TXT id centred above the rows, 0xFFFF none
    VIColor32F     m_color;      // +0x258: the title's colour
};

// VIWndTrainer: ctor 0x00C9EA88, 968 bytes (`new`), vtable 0x00CF1708;
//   slots [0]0x00CA2688 [1]0x00C9EB50 [2]OnDraw__5VIWnd [3]0x00C9ECA8 [4]0x00CA10E0 [5]0x00CA09E8 [6]0x00CA11B0
// The stat trainer.  Modes (m_mode): 0 the seven trainable stats (left/right spend a point),
// 1 "Train" highlighted, 2 the next button, 3 the nine derived stats, 4 trained (wait for the
// server).  Each page lists the equipped items that give the selected stat.
class VIWndTrainer : public VIWnd
{
public:
    VIWndTrainer();                                              // 0x00C9EA88
    virtual ~VIWndTrainer();                                     // 0x00C9EB50
    int  Init();                                                 // 0x00C9EBC0
    int  Free();                                                 // 0x00C9EC30
    virtual void OnDrawSelf();                                   // 0x00C9ECA8
    void DrawStats();                                            // 0x00C9F3B8: every mode but 3
    void DrawDetails();                                          // 0x00C9FCD0: mode 3
    void DrawHeader(VIVect2I pos);                               // 0x00CA03D8: "name: Level n race class"
    void DrawItems(VIVect2I pos, int stat);                      // 0x00CA0540: the equipped items giving `stat`
    virtual void OnKeyDown(int key);                             // 0x00CA09E8
    void Up(int key);                                            // 0x00CA0AD8
    void Down(int key);                                          // 0x00CA0B98
    void Left(int key);                                          // 0x00CA0C70
    void Right(int key);                                         // 0x00CA0D00
    void Select(int key);                                        // 0x00CA0D90: mode 1 sends the points
    void Cancel(int key);                                        // 0x00CA0E98
    void PageUp(int key);                                        // 0x00CA0F60: 'E', 'e', 142
    void PageDown(int key);                                      // 0x00CA1018: 'C', 'c', 143
    virtual void OnKeyUp(int key);                               // 0x00CA10E0
    void LeftReleased();                                         // 0x00CA1150
    void RightReleased();                                        // 0x00CA1180
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00CA11B0
    void Move(int d);                                            // 0x00CA12E0: the selection by d, clamped
    void Adjust(int d);                                          // 0x00CA1420: a point onto / off the selected stat
    void Open();                                                 // 0x00CA1550: the player's stats, mode 0
    void Refresh();                                              // 0x00CA15B8: VIWndGame::Msg001D (stats changed)
    int  CountItems(int stat);                                   // 0x00CA1628
    int  BaseStat(int stat);                                     // 0x00CA16B0: m_base[], resists derived
    int  CurStat(int stat);                                      // 0x00CA1728: m_cur[], resists derived
    void ItemDown(int stat);                                     // 0x00CA17A0
    void ItemUp(int stat);                                       // 0x00CA1898
    VIItemStat *FindStat(int i, int stat);                       // 0x00CA1988: inventory item i's bonus to stat
    void ResetFade();                                            // 0x00CA1A70

    // Inline: emitted after __tf12VIWndTrainer.
    void SetPos(const VIVect2I &p) { m_pos = p; }                // 0x00CA26D8

    int         m_sel;          // +0x024: the row
    int         m_points;       // +0x028: points left to spend
    int         m_mode;         // +0x02C
    int         m_add[7];       // +0x030: points spent per stat, not yet sent
    int        *m_base;         // +0x04C: VIClient::GetPlayerPD8()
    int        *m_cur;          // +0x050: VIClient::GetPlayerStats()
    VIVect2I    m_pos;          // +0x054
    long        m_time;         // +0x060: the client clock at the last draw
    int         m_fadeDir;      // +0x068: the arrows' pulse
    float       m_fade;         // +0x06C
    int         m_colorDir;     // +0x070: the class-stat colour's pulse
    VIColor32F  m_color;        // +0x074
    VIUIWindow  m_window;       // +0x088: the title strip
    VIUIWindow  m_list;         // +0x208: the item list
    int         m_last;         // +0x388: the last item row shown
    int         m_item;         // +0x38C: the first item row shown
    VIKeyFlash  m_flash;        // +0x390: held left / right
};

// VIWndWhoList: ctor 0x00CA26F0, 888 bytes (`new`), vtable 0x00CF1BE0;
//   slots [0]0x00CA4608 [1]0x00CA2758 [2]OnDraw__5VIWnd [3]0x00CA2A60 [4]0x00CA31F0 [5]0x00CA30B8 [6]0x00CA3268
class VIWndWhoList : public VIWnd
{
public:
    VIWndWhoList();                                              // 0x00CA26F0
    virtual ~VIWndWhoList();                                     // 0x00CA2758
    int  Init();                                                 // 0x00CA27C8
    int  Free();                                                 // 0x00CA2930
    virtual void OnDrawSelf();                                   // 0x00CA2A60
    virtual void OnKeyDown(int key);                             // 0x00CA30B8
    virtual void OnKeyUp(int key);                               // 0x00CA31F0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00CA3268
    void Down();                                                 // 0x00CA35C0
    void Up();                                                   // 0x00CA3650
    void Select();                                               // 0x00CA36B8: the option list for the row
    void Tell();                                                 // 0x00CA3738: the editor for a tell
    void SendTell();                                             // 0x00CA37C0: "/t <name> <text>"
    void Invite();                                               // 0x00CA3870: InviteToGroup
    void Refresh();                                              // 0x00CA38E8: asks the server again
    void Mail();                                                 // 0x00CA3938: a tell-gram to the row

    int               m_sel;        // +0x024
    int               m_top;        // +0x028
    VIUIWindow        m_window;     // +0x030
    VIUIWindow        m_status;     // +0x1B0: "searching..."
    VIKeyFlash        m_flash;      // +0x330
    VIWndOptionList  *m_options;    // +0x364: tell / invite / mail
    VIWndEdit        *m_edit;       // +0x368
    VIWndComposeGram *m_compose;    // +0x36C
    unsigned short  **m_choices;    // +0x370: the option list's three strings
};

#endif
