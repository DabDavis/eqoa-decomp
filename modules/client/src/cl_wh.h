// cl_wh.h -- windows: VIWndGoto, VIWndGramInbox, VIWndGuild, VIWndHUDMenu, VIWndIdle, VIWndIgnoreList, VIWndInfluence, VIWndInitFromMc, VIWndInspect, VIWndItemMenu.
// CLIENT 0x00C58B78..0x00C685F8.
// TUs (one window class each, ending with its __tf):
//   VIWndGoto                  0x00C58B78..0x00C59D80
//   VIWndGramInbox             0x00C59D80..0x00C5B958
//   VIWndGuild                 0x00C5B958..0x00C5DDF8
//   VIWndHUDMenu               0x00C5DDF8..0x00C5F9D8
//   VIWndIdle                  0x00C5F9D8..0x00C60780
//   VIWndIgnoreList            0x00C60780..0x00C62300
//   VIWndInfluence             0x00C62300..0x00C63200
//   VIWndInitFromMc            0x00C63200..0x00C644C8
//   VIWndInspect               0x00C644C8..0x00C658C8
//   VIWndItemMenu              0x00C658C8..0x00C685F8
#ifndef CL_WH_H
#define CL_WH_H

// VIWndGoto: ctor 0x00C58BD8, 60 bytes (`new`), vtable 0x00CE8E30;
//   slots [0]0x00C59D30 [1]0x00C58C40 [2]OnDraw__5VIWnd [3]0x00C58CB0 [4]0x00C59038 [5]0x00C58F08 [6]0x00C590F0
// A debug teleporter: the world (0..5), and a grid square inside the world's bounds
// (gWorldInfo's m_p0C..m_p10 columns, m_p14..m_p18 rows); Enter hands the three to
// VIClient::SetP50308 and closes (260 to the parent).  60 bytes.  Names ours.
class VIWndGoto : public VIWnd
{
public:
    VIWndGoto();                                                 // 0x00C58BD8
    virtual ~VIWndGoto();                                        // 0x00C58C40
    int  Init(VIScene *scene);                                            // 0x00C58C88: no world yet
    int  Free();                                                 // 0x00C58CA0
    virtual void OnDrawSelf();                                   // 0x00C58CB0
    virtual void OnKeyDown(int key);                             // 0x00C58F08
    virtual void OnKeyUp(int key);                               // 0x00C59038: empty
    void ChangeRow(int d);                                       // 0x00C59040: m_y, clamped
    void ChangeColumn(int d);                                    // 0x00C59080: m_x, clamped
    void ChangeWorld(int d);                                     // 0x00C590C0: 0..5, the square back to 0,0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C590F0

    // Inline: its copy follows __tf9VIWndGoto (0x00C59D80).
    void SetPos(const VIVect2I &p) { m_pos = p; }

    VIVect2I m_pos;                                              // +0x24
    int      m_y;                                                // +0x2C: the row
    int      m_x;                                                // +0x30: the column
    int      m_x34;                                              // +0x34: Init's argument
    int      m_world;                                            // +0x38: -1 until 295
};

// 0x00CC4608 (VIEntity.cpp's neighbour): the WORDS.TXT id naming grid square
// (y, x) of a world, 641 outside its bounds.

// VIWndGramInbox: ctor 0x00C59D98, 880 bytes (`new`), vtable 0x00CE9168;
//   slots [0]0x00C5B908 [1]0x00C59DE8 [2]OnDraw__5VIWnd [3]0x00C59F28 [4]0x00C5A8C0 [5]0x00C5A730 [6]0x00C5A938
// The in-game mail (gram) inbox: the grams the gram client (VIClientPart2EFD0, VIClient
// vtable slot 7) lists, eleven rows of sender, subject and date, over a status window
// that says what the client is doing; Enter fetches a gram and opens it in a
// VIWndReadGram.  CLIENT's twin of STATION's VIWndCSInbox, with the same method order.
// 880 bytes.  Names ours.
class VIWndGramInbox : public VIWnd
{
public:
    VIWndGramInbox();                                            // 0x00C59D98
    virtual ~VIWndGramInbox();                                   // 0x00C59DE8
    int  Init();                                                 // 0x00C59E58
    void Free();                                                 // 0x00C59ED0
    virtual void OnDrawSelf();                                   // 0x00C59F28
    virtual void OnKeyDown(int key);                             // 0x00C5A730
    virtual void OnKeyUp(int key);                               // 0x00C5A8C0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C5A938
    void Refresh();                                              // 0x00C5AAC8: re-requests the list
    void End();                                                  // 0x00C5AB40: empty
    void Next();                                                 // 0x00C5AB48
    void Prev();                                                 // 0x00C5ABD8
    void Read();                                                 // 0x00C5AC40: fetches the selected gram

    VIUIWindow     m_window;                                     // +0x028: the list
    VIUIWindow     m_status;                                     // +0x1A8
    VIWndReadGram *m_read;                                       // +0x328
    int            m_top;                                        // +0x32C: first row shown
    int            m_sel;                                        // +0x330
    int            m_open;                                       // +0x334: open the gram once it is in
    VIKeyFlash     m_flash;                                      // +0x338: the arrows' repeat
    int            m_p36C;                                       // +0x36C
};

// VIWndGuild: ctor 0x00C5B958, 520 bytes (`new`), vtable 0x00CE94E0;
//   slots [0]0x00C5DDA8 [1]0x00C5B9D0 [2]OnDraw__5VIWnd [3]0x00C5BC08 [4]0x00C5C608 [5]0x00C5C478 [6]0x00C5C680
// The guild roster: name, rank, race, class, level and online for up to eleven members of
// VIClientGuild (VIClient vtable slot 4); X on a member opens a VIWndMiniMenu of info /
// promote / demote / kick, each checked by VIClientPart2EF24 (slot 5) and confirmed in a
// VIWndMBox, which also shows a guild invitation.  520 bytes.  Names ours.
class VIWndGuild : public VIWnd
{
public:
    VIWndGuild();                                                // 0x00C5B958
    virtual ~VIWndGuild();                                       // 0x00C5B9D0
    int  Init();                                                 // 0x00C5BA30
    int  Free();                                                 // 0x00C5BB50
    virtual void OnDrawSelf();                                   // 0x00C5BC08
    virtual void OnKeyDown(int key);                             // 0x00C5C478
    virtual void OnKeyUp(int key);                               // 0x00C5C608
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C5C680: VIWndGuild.cpp:447/477/503
    void ShowInvite();                                           // 0x00C5CC50: the invitation, 45 s to answer
    void Close();                                                // 0x00C5CD90: back to the roster (mode 2)
    void Reset();                                                // 0x00C5CDE0: mode 1, both boxes hidden
    void Move(int d);                                            // 0x00C5CE00: the selection
    void Clamp();                                                // 0x00C5CF18: to the member count
    void ShowMemberMenu(const char *name);                       // 0x00C5CFE8
    void ResetSel() { m_sel = 0; m_top = 0; }                    // 0x00C5DDF8 (after __tf)

    VIWndMBox     *m_mbox;                                       // +0x024
    VIWndMiniMenu *m_menu;                                       // +0x028
    int            m_sel;                                        // +0x02C
    int            m_top;                                        // +0x030
    int            m_count;                                      // +0x034: the member count last seen
    int            m_mode;                                       // +0x038: 2 shows the roster
    int            m_focus;                                      // +0x03C: 1 while it has the focus
    int            m_fadeIn;                                     // +0x040: 1 brightening, -1 dimming
    float          m_alpha;                                      // +0x044: the selection's pulse
    long           m_time;                                       // +0x048
    VIUIWindow     m_window;                                     // +0x050
    VIKeyFlash     m_flash;                                      // +0x1D0
    int            m_p204;                                       // +0x204
};

// VIWndHUDMenu: ctor 0x00C5DE08, 504 bytes (`new`), vtable 0x00CE9910;
//   slots [0]0x00C5F988 [1]0x00C5DE78 [2]OnDraw__5VIWnd [3]0x00C5DF30 [4]OnKeyUp__5VIWndi [5]0x00C5EAE8 [6]0x00C5EC08
// One of VIWndGame's three quick menus (m_hud[0..2]): a title bar naming the selected
// entry, then the entries of a VIMenuDef, W/S to move, E to use (the parent hears 261
// (m_msg, m_id)).  Lit for 20 s after a key, then dims to 0.3.  504 bytes.  Names ours.
class VIWndHUDMenu : public VIWnd
{
public:
    VIWndHUDMenu();                                              // 0x00C5DE08
    virtual ~VIWndHUDMenu();                                     // 0x00C5DE78
    int  Init();                                                 // 0x00C5DED0
    int  Free();                                                 // 0x00C5DF00
    virtual void OnDrawSelf();                                   // 0x00C5DF30
    void DrawStack();                                            // 0x00C5E0B0: m_index 2
    void DrawList();                                             // 0x00C5E638: m_index 0, 1
    virtual void OnKeyDown(int key);                             // 0x00C5EAE8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C5EC08
    void Move(int d);                                            // 0x00C5ECE0: wraps
    void SetMenu(VIMenuDef *menu);                               // 0x00C5ED58
    void Clamp();                                                // 0x00C5ED60
    void SetTitle(const unsigned short *s);                      // 0x00C5ED88
    void SetPos(const VIVect2I &p) { m_pos = p; }                // 0x00C5F9D8 (after __tf)
    void SetIndex(int i) { m_index = i; }                        // 0x00C5F9F0
    void ResetSel() { m_sel = 0; }                               // 0x00C5F9F8

    VIMenuDef     *m_menu;                                       // +0x024
    int            m_index;                                      // +0x028: which of the three
    VIUITextScroll m_text;                                       // +0x030: the selected entry's name
    int            m_sel;                                        // +0x1C8
    int            m_lit;                                        // +0x1CC: 1 brightening, -1 dimming
    float          m_alpha;                                      // +0x1D0
    long           m_time;                                       // +0x1D8: dims after this
    unsigned short m_title[8];                                   // +0x1E0
    VIVect2I       m_pos;                                        // +0x1F0
};

// VIWndIdle: ctor 0x00C5FA00, 36 bytes (`new`), vtable 0x00CE9C10;
//   slots [0]0x00C60730 [1]0x00C5FA38 [2]OnDraw__5VIWnd [3]0x00C5FA60 [4]0x00C5FAA8 [5]0x00C5FA68 [6]0x00C5FAE8
// The window that holds the focus while a world loads (VIClient+0x2BB8C: the loader's
// progress callback runs only while it is active).  Nothing of its own: 36 bytes.
class VIWndIdle : public VIWnd
{
public:
    VIWndIdle();                                                 // 0x00C5FA00
    virtual ~VIWndIdle();                                        // 0x00C5FA38
    virtual void OnDrawSelf();                                   // 0x00C5FA60: empty
    virtual void OnKeyDown(int key);                             // 0x00C5FA68
    virtual void OnKeyUp(int key);                               // 0x00C5FAA8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C5FAE8
};

// VIWndIgnoreList: ctor 0x00C60780, 512 bytes (`new`), vtable 0x00CE9F10;
//   slots [0]0x00C622B0 [1]0x00C607E8 [2]OnDraw__5VIWnd [3]0x00C60A28 [4]0x00C60F08 [5]0x00C60D78 [6]0x00C60F80
// The ignore list (VIClientPart38300, VIClient vtable slot 9): twelve names a page; X on
// one opens a VIWndMiniMenu whose remove entry (0x0B053004) asks in a VIWndMBox, and yes
// sends the removal.  512 bytes.  Names ours.
class VIWndIgnoreList : public VIWnd
{
public:
    VIWndIgnoreList();                                           // 0x00C60780
    virtual ~VIWndIgnoreList();                                  // 0x00C607E8
    int  Init();                                                 // 0x00C60858
    int  Free();                                                 // 0x00C60970
    virtual void OnDrawSelf();                                   // 0x00C60A28
    virtual void OnKeyDown(int key);                             // 0x00C60D78
    virtual void OnKeyUp(int key);                               // 0x00C60F08
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C60F80
    void Move(int d);                                            // 0x00C611B0
    void Clamp();                                                // 0x00C612A8
    void Close();                                                // 0x00C61378: both children hidden, the focus back
    void Reset();                                                // 0x00C613C0: both children hidden
    void ShowMenu(const char *name);                             // 0x00C613D8: gIgnoreMemberMenu
    void ResetSel() { m_sel = 0; m_top = 0; }                    // 0x00C62300 (after __tf)

    int            m_sel;                                        // +0x024
    int            m_top;                                        // +0x028
    int            m_count;                                      // +0x02C: the count last seen
    bool           m_focus;                                      // +0x030
    VIArray<int>   m_ids;                                        // +0x034: the list's pool indices
    VIWndMBox     *m_mbox;                                       // +0x03C
    VIWndMiniMenu *m_menu;                                       // +0x040
    VIUIWindow     m_window;                                     // +0x048
    VIKeyFlash     m_flash;                                      // +0x1C8
};

// VIWndInfluence: ctor 0x00C62310, 44 bytes (`new`), vtable 0x00CEA208;
//   slots [0]0x00C631B0 [1]0x00C62360 [2]OnDraw__5VIWnd [3]0x00C62390 [4]0x00C625D0 [5]0x00C625B0 [6]OnMessage__5VIWndP5VIWndiii
// The row of active-effect icons (VIClient::m_effectCount / m_effectIcon, state channel
// 0x43) in a capped bar at m_pos.  44 bytes.  Names ours.
class VIWndInfluence : public VIWnd
{
public:
    VIWndInfluence();                                            // 0x00C62310
    virtual ~VIWndInfluence();                                   // 0x00C62360
    int  Init();                                                 // 0x00C62388: returns 0
    virtual void OnDrawSelf();                                   // 0x00C62390
    virtual void OnKeyDown(int key);                             // 0x00C625B0
    virtual void OnKeyUp(int key);                               // 0x00C625D0: empty

    // Inline: its copy follows __tf14VIWndInfluence (0x00C63200).
    void SetPos(const VIVect2I &p) { m_pos = p; }

    VIVect2I m_pos;                                              // +0x24
};

// VIWndInitFromMc: ctor 0x00C63218, 432 bytes (`new`), vtable 0x00CEA5F8;
//   slots [0]0x00C64478 [1]0x00C63258 [2]OnDraw__5VIWnd [3]0x00C632F0 [4]0x00C63540 [5]0x00C63548 [6]0x00C63568
// The start-up window (VIClient+0x2BB90): with the memory card checked each step, it
// loads the words, the game options (and their two volumes) and the shard list
// (althosts.txt, else eqahosts.txt), then shows the login window.  Any card failure puts
// up an McErr box (4) as a child, which the next frame removes.  432 bytes.  Names ours.
class VIWndInitFromMc : public VIWnd
{
public:
    VIWndInitFromMc();                                           // 0x00C63218
    virtual ~VIWndInitFromMc();                                  // 0x00C63258
    int  Init();                                                 // 0x00C632D0
    void Free();                                                 // 0x00C632E8: empty
    virtual void OnDrawSelf();                                   // 0x00C632F0: one step a frame
    virtual void OnKeyUp(int key);                               // 0x00C63540: empty
    virtual void OnKeyDown(int key);                             // 0x00C63548
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C63568
    int  LoadHosts();                                            // 0x00C63708: < 0 on failure

    VIUIWindow     m_window;                                     // +0x028
    unsigned char  m_state;                                      // +0x1A8: 0 card, 1 words, 2 options, 3 hosts, 4 done
    unsigned char  m_error;                                      // +0x1A9: an McErr box is up
    VIWnd         *m_child;                                      // +0x1AC: the closed McErr box, removed next frame
};

// MCERR's export (STATION's stcs.h declares it the same way): a memory-card error box.
int  McErr(VIWnd *parent, const VIVect2I &pos, int code);

// VIWndInspect: ctor 0x00C644C8, 2552 bytes (`new`), vtable 0x00CEA908;
//   slots [0]0x00C65878 [1]0x00C64520 [2]OnDraw__5VIWnd [3]0x00C64610 [4]0x00C64978 [5]0x00C64808 [6]0x00C64A18
// A scrolling text viewer (an item's or an ability's description): Init(depth, p), then
// SetText; X goes back a level (297 to the parent with depth - 1 and p), Triangle closes
// (260).  New'd by VIWndAuctionSearch, VIWndExperiment, VIWndItemMenu and VIWndRecipe.
// 2552 bytes.  Names ours.
class VIWndInspect : public VIWnd
{
public:
    VIWndInspect();                                              // 0x00C644C8
    virtual ~VIWndInspect();                                     // 0x00C64520
    int  Init(int depth, int p);                                 // 0x00C64580
    int  Free();                                                 // 0x00C645D0
    void Clear();                                                // 0x00C64600: no text
    virtual void OnDrawSelf();                                   // 0x00C64610
    virtual void OnKeyDown(int key);                             // 0x00C64808
    virtual void OnKeyUp(int key);                               // 0x00C64978
    void SetText(const unsigned short *s);                       // 0x00C649F0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C64A18
    void Next();                                                 // 0x00C64BC0: scrolls down
    void Prev();                                                 // 0x00C64C30: scrolls up

    int            m_depth;                                      // +0x024: X goes back while >= 2
    int            m_p28;                                        // +0x028: handed back with 297
    unsigned short m_text[1024];                                 // +0x02C
    VIUIWindow     m_window;                                     // +0x830
    int            m_lines;                                      // +0x9B0: the text's lines
    int            m_rows;                                       // +0x9B4: the window's, 17 at most
    int            m_shown;                                      // +0x9B8: m_rows - 2
    int            m_top;                                        // +0x9BC: first line shown
    VIKeyFlash     m_flash;                                      // +0x9C0
    int            m_p9F4;                                       // +0x9F4
};

// VIWndItemMenu: ctor 0x00C658C8, 1648 bytes (`new`), vtable 0x00CEACB8;
//   slots [0]0x00C685A8 [1]0x00C65960 [2]OnDraw__5VIWnd [3]0x00C65ED8 [4]OnKeyUp__5VIWndi [5]0x00C67238 [6]0x00C66630
// The inventory menu: four VIWndMenu tabs of item categories (tab 0 the one at +0x3C,
// 1 +0x38, 2 +0x40, 3 +0x34), a description box for the selected entry, an inspect window,
// a quantity picker and a confirmation box; and a quick-slot mode (m_menu == -1) where the
// pad moves the item into one of the quick slots.  1648 bytes.  New'd by VIWndStartMenu.
// Names ours.
class VIWndItemMenu : public VIWnd
{
public:
    VIWndItemMenu();                                             // 0x00C658C8
    virtual ~VIWndItemMenu();                                    // 0x00C65960
    int  Init();                                                 // 0x00C659C0
    int  Free();                                                 // 0x00C65CF8
    virtual void OnDrawSelf();                                   // 0x00C65ED8
    void DrawSlots();                                            // 0x00C662A0: the quick-slot mode
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C66630
    void Use(int item, int count);                               // 0x00C66AD0: asks first if equipped/slotted/no-trade
    void ShowMenu(int reset);                                    // 0x00C66CA8: the current tab's menu
    void Inspect(int item);                                      // 0x00C66E50
    void InspectA(int item);                                     // 0x00C66F48
    void InspectB(int item);                                     // 0x00C66F98
    void InspectBack(int p);                                     // 0x00C66FF8: 297 from the inspect window
    void InspectD(int item);                                     // 0x00C67058
    void InspectC(int item);                                     // 0x00C670A8
    int  AssignSlot(VIWndMenu *menu, int item);                  // 0x00C670F8: enters the quick-slot mode
    void BackToMenu();                                           // 0x00C671D8
    virtual void OnKeyDown(int key);                             // 0x00C67238
    void Open(VIWnd *from);                                      // 0x00C67428
    void SetMenuD(VIMenuDef *menu);                              // 0x00C67520
    void SetMenuB(VIMenuDef *menu);                              // 0x00C67540
    void SetMenuA(VIMenuDef *menu);                              // 0x00C67560
    void SetMenuC(VIMenuDef *menu);                              // 0x00C67580
    void ChangeTab(int d);                                       // 0x00C675A0
    void ChangeSlot(int d);                                      // 0x00C67670
    void ClearDesc();                                            // 0x00C67790
    void UpdateSlotName();                                       // 0x00C67798
    void UpdateDesc();                                           // 0x00C677F0
    void DescC();                                                // 0x00C67890
    void DescD();                                                // 0x00C678E0
    void DescA();                                                // 0x00C67930
    void DescB();                                                // 0x00C67980

    int            m_tab;                                        // +0x024: the tab drawn raised
    int            m_p28;                                        // +0x028
    int            m_slot;                                       // +0x02C: the quick slot, 0..3
    int            m_item;                                       // +0x030: the item being slotted
    VIWndMenu     *m_menuD;                                      // +0x034: tab 3
    VIWndMenu     *m_menuB;                                      // +0x038: tab 1
    VIWndMenu     *m_menuA;                                      // +0x03C: tab 0
    VIWndMenu     *m_menuC;                                      // +0x040: tab 2
    VIWndInspect  *m_inspect;                                    // +0x044
    VIWndQuantity *m_quantity;                                   // +0x048
    VIWndMBox     *m_mbox;                                       // +0x04C
    int            m_menu;                                       // +0x050: the tab, -1 slotting, -2 none
    long           m_time;                                       // +0x058
    int            m_fadeIn;                                     // +0x060: 1 brightening, -1 dimming
    float          m_alpha;                                      // +0x064: the slot cursor's pulse
    VIUIWindow     m_window;                                     // +0x068
    unsigned short m_desc[512];                                  // +0x1E8
    unsigned short m_slotName[64];                               // +0x5E8
    VIInfoText     m_info;                                       // +0x668
};

// The quick-slot menu, VIWndGame.cpp's (0x00CD02D0): a VIMenuDef whose count is the slots.
// gHudMenu2 is declared in cl_wg.h (VIHudMenu).
extern VIMenuDef gIgnoreMemberMenu;                                    // VIWndGame.cpp's .data: the ignore list's member menu

#endif
