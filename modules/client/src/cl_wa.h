// cl_wa.h -- windows: VIWndAuction, VIWndAuctionSearch, VIWndBank, VIWndBankItem, VIWndBankMenu, VIWndBankMoney.
// CLIENT 0x00C0D000..0x00C1A510.
// TUs (one window class each, ending with its __tf):
//   VIWndAuction               0x00C0D000..0x00C0F9F0
//   VIWndAuctionSearch         0x00C0F9F0..0x00C152A0
//   VIWndBank                  0x00C152A0..0x00C16438
//   VIWndBankItem              0x00C16438..0x00C18178
//   VIWndBankMenu              0x00C18178..0x00C194B8
//   VIWndBankMoney             0x00C194B8..0x00C1A510
#ifndef CL_WA_H
#define CL_WA_H

extern "C" int atoi(const char *s);

// VIWndAuction: ctor 0x00C0D038, 848 bytes (`new`), vtable 0x00CE0D58;
//   slots [0]0x00C0F9A0 [1]0x00C0D098 [2]OnDraw__5VIWnd [3]0x00C0D3C8 [4]0x00C0E2C8 [5]0x00C0E0C0 [6]0x00C0E370
// The auction window (opened from the bank): the bank's items in a list, and for the one
// picked a sell form of four rows -- minimum bid, buyout, duration -- and Ok.  The form's
// numbers are typed in a VIWndEdit; the sale is confirmed in a VIWndMBox.  0x350 bytes.
// Method names are ours.
class VIWndAuction : public VIWnd
{
public:
    VIWndAuction();                                              // 0x00C0D038
    virtual ~VIWndAuction();                                     // 0x00C0D098
    int  Init();                                                 // 0x00C0D108
    int  Free();                                                 // 0x00C0D2C0
    virtual void OnDrawSelf();                                   // 0x00C0D3C8
    void FillLists();                                            // 0x00C0D490
    void DrawLists();                                            // 0x00C0D6E8
    void DrawSell();                                             // 0x00C0DA48
    void DrawRow(VIUIWindow *w, int row, const unsigned short *a, int xa,
                 const unsigned short *b, int xb, const unsigned short *c, int xc,
                 int sel, int tick);                             // 0x00C0DF68
    virtual void OnKeyDown(int key);                             // 0x00C0E0C0
    virtual void OnKeyUp(int key);                               // 0x00C0E2C8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C0E370
    void Select();                                               // 0x00C0E550
    void Next();                                                 // 0x00C0E688
    void Prev();                                                 // 0x00C0E6F0
    void Accept();                                               // 0x00C0E758
    void EditField();                                            // 0x00C0E940
    void TakeText();                                             // 0x00C0EB00
    int  CanSell(VINetItem *item);                               // 0x00C0EBE8
    int  InMinutes();                                            // 0x00C0EC30: the client's 382E8 part says > 0
    void ShowMessage(const unsigned short *text, int p1, int p2);// 0x00C0EC70
    void Confirm(int p1, int p2);                                // 0x00C0ED20

    int         m_selling;   // +0x024: 0 the bank list, 1 the sell form
    VIWndList  *m_inv;       // +0x028: 40 rows, the inventory (never shown)
    VIWndList  *m_bank;      // +0x02C: 80 rows, the bank
    VIWndEdit  *m_edit;      // +0x030
    VIWndMBox  *m_mbox;      // +0x034
    VIInfoText  m_info;      // +0x038
    VIUIWindow  m_window;    // +0x040
    VIUIWindow  m_sell;      // +0x1C0
    int         m_field;     // +0x340: the sell form's row, 0..4
    int         m_price;     // +0x344
    int         m_buyout;    // +0x348
    int         m_duration;  // +0x34C
};

// VIWndAuctionSearch: ctor 0x00C0F9F0, 5928 bytes (`new`), vtable 0x00CE1328;
//   slots [0]0x00C15250 [1]0x00C0FA60 [2]OnDraw__5VIWnd [3]0x00C0FDC0 [4]OnKeyUp__5VIWndi [5]0x00C10AD8 [6]0x00C117A0
// The auction search (VIBankControl's other half): a form of twelve rows -- name, class,
// race, levels, prices, item type, slot, a flag, seller, Search -- whose pickers are
// states 5..8, then the result pages in a VIWndMenu (state 1) with a bid/inspect submenu
// per lot, the lot's detail (2), the bidder rankings (3), the bid form (4) and a status
// window while VIBankControl waits.  0x1728 bytes.  Method names are ours.
class VIWndAuctionSearch : public VIWnd
{
public:
    VIWndAuctionSearch();                                        // 0x00C0F9F0
    virtual ~VIWndAuctionSearch();                               // 0x00C0FA60
    int  Init();                                                 // 0x00C0FAD8
    int  Free();                                                 // 0x00C0FCB8
    virtual void OnDrawSelf();                                   // 0x00C0FDC0
    virtual void OnKeyDown(int key);                             // 0x00C10AD8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C117A0
    void SetState(int state);                                    // 0x00C11B00
    void FillMenu();                                             // 0x00C11D08
    void FillRow(VIAuctionItem *lot, int row);                   // 0x00C11E20
    void DrawLot();                                              // 0x00C12058: state 2
    void DrawRankings();                                         // 0x00C124C0: state 3
    void DrawBid();                                              // 0x00C12808: state 4
    void DrawClasses();                                          // 0x00C12B78: state 6
    void DrawRaces();                                            // 0x00C12E18: state 5
    void DrawTypes();                                            // 0x00C130C8: state 7
    void DrawSlots();                                            // 0x00C13358: state 8
    void DrawSummary();                                          // 0x00C135E8: states 1..4
    void Select();                                               // 0x00C139B0: Enter on a form row
    void EditField();                                            // 0x00C13BE8
    void TakeText();                                             // 0x00C13D80
    void DrawRow(VIUIWindow *w, int row, const unsigned short *label,
                 const unsigned short *value, int xa, int xb, int sel, int tick,
                 int framed);                                    // 0x00C13E88
    void Bid(int lot);                                           // 0x00C13FA0
    void Inspect(int lot);                                       // 0x00C13FC0
    void Buyout(int p);                                          // 0x00C14130
    void FormatTime(unsigned short *buf, int size, int minutes, int lines);  // 0x00C14200
    void SetMode(int id);                                        // 0x00C143D0: 0xA8020000 / 0xA8030000
    void Open();                                                 // 0x00C14418
    void ShowMessage(const unsigned short *text, int p1, int p2);// 0x00C14468
    void Confirm(int p1, int p2);                                // 0x00C14520

    // Inline: emitted after __tf18VIWndAuctionSearch (0x00C152A0).
    int  GetMode() { return m_mode; }

    int            m_state;        // +0x024: 0 the form, 1 the results, 2..8 a sub-page
    int            m_mode;         // +0x028: SetMode's 1 or 2
    VIWndInspect  *m_inspect;      // +0x02C
    VIWndEdit     *m_edit;         // +0x030
    VIWndMenu     *m_menu;         // +0x034
    VIWndMBox     *m_mbox;         // +0x038
    VIUIWindow     m_window;       // +0x040: the form
    VIUIWindow     m_list;         // +0x1C0: the sub-page
    VIUIWindow     m_status;       // +0x340: "searching..."
    VIInfoText     m_info;         // +0x4C0
    char           m_dots;         // +0x4C4
    char           m_ticks;        // +0x4C5
    int            m_field;        // +0x4C8: the form's row, 0..11
    int            m_cursor;       // +0x4CC: a sub-page's row
    int            m_top;          // +0x4D0: a sub-page's first row shown
    int            m_class;        // +0x4D4: -1 any
    int            m_race;         // +0x4D8: -1 any, -2 none
    int            m_minLevel;     // +0x4DC
    int            m_maxLevel;     // +0x4E0
    int            m_minPrice;     // +0x4E4
    int            m_maxPrice;     // +0x4E8
    int            m_wear;         // +0x4EC: a wear slot, -2 any (FindWearSlotWord)
    int            m_attack;       // +0x4F0: an attack type, -1 any (FindAttackWord)
    int            m_x4F4;         // +0x4F4: rows 1688/1689
    unsigned short m_name[64];     // +0x4F8
    char           m_seller[24];   // +0x578
    int            m_lot;          // +0x590: the lot a bid or a buyout is for
    int            m_bid;          // +0x594
    VIMenuDef      m_def;          // +0x598: the results menu
    VIMenuEntry    m_rows[40];     // +0x5A8
    VIMenuDef      m_subs[40];     // +0xAA8: each row's two options
    VIMenuEntry    m_opts[40][2];  // +0xD28
};
CHK(c_VIWndAuctionSearch, sizeof(VIWndAuctionSearch) == 0x1728)

// VIWndBank: ctor 0x00C152A8, 52 bytes (`new`), vtable 0x00CE1730;
//   slots [0]0x00C163E8 [1]0x00C152F0 [2]OnDraw__5VIWnd [3]OnDrawSelf__5VIWnd [4]OnKeyUp__5VIWndi [5]OnKeyDown__5VIWndi [6]0x00C155B0
// The bank (VIWndGame+0x6AC): four child windows, one shown at a time -- the menu, the
// item lists, the money form and the auction.  Children send 289 (items), 290 (money),
// 310 (auction) and 260 (back to the menu).  Method names are ours.
class VIWndBankMenu;
class VIWndBankItem;
class VIWndBankMoney;
class VIWndBank : public VIWnd
{
public:
    VIWndBank();                                                 // 0x00C152A8
    virtual ~VIWndBank();                                        // 0x00C152F0
    int  Init();                                                 // 0x00C15338
    int  Free();                                                 // 0x00C154A8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C155B0
    void ShowMenu(int unused);                                   // 0x00C157B0: VIWndGame::BankClose passes 1

    VIWndBankMenu  *m_menu;     // +0x024
    VIWndBankItem  *m_item;     // +0x028
    VIWndBankMoney *m_money;    // +0x02C
    VIWndAuction   *m_auction;  // +0x030
};
CHK(c_VIWndBank, sizeof(VIWndBank) == 0x34)

// VIWndBankItem: ctor 0x00C16438, 1464 bytes (`new`), vtable 0x00CE1AC8;
//   slots [0]0x00C18128 [1]0x00C16498 [2]OnDraw__5VIWnd [3]0x00C16770 [4]0x00C16F70 [5]0x00C16CC8 [6]0x00C17038
// Two VIWndList columns, the inventory and the bank; Enter moves the item across (a stack
// asks a VIWndQuantity how many first).  0x5B8 bytes.  Method names are ours.
class VIWndBankItem : public VIWnd
{
public:
    VIWndBankItem();                                             // 0x00C16438
    virtual ~VIWndBankItem();                                    // 0x00C16498
    int  Init();                                                 // 0x00C164F8
    int  Free();                                                 // 0x00C16690
    virtual void OnDrawSelf();                                   // 0x00C16770
    virtual void OnKeyDown(int key);                             // 0x00C16CC8
    virtual void OnKeyUp(int key);                               // 0x00C16F70
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C17038
    void Show();                                                 // 0x00C17380: the inventory column active
    void UpdateText();                                           // 0x00C17418: the selected item's description
    void ClearText();                                            // 0x00C17548

    VIWndList      *m_inv;      // +0x024
    VIWndList      *m_bank;     // +0x028
    VIWndQuantity  *m_qty;      // +0x02C
    VIUIWindow      m_window;   // +0x030
    int             m_from;     // +0x1B0: the list the quantity is for, 0 inventory, 1 bank, -1 none
    unsigned short  m_text[512];// +0x1B4
    VIInfoText      m_info;     // +0x5B4
};
CHK(c_VIWndBankItem, sizeof(VIWndBankItem) == 0x5B8)

// VIWndBankMenu: ctor 0x00C18178, 432 bytes (`new`), vtable 0x00CE1DD0;
//   slots [0]0x00C19468 [1]0x00C181C0 [2]OnDraw__5VIWnd [3]0x00C18238 [4]OnKeyUp__5VIWndi [5]0x00C18530 [6]0x00C186E0
// The bank's first page: money (290), items (289), the auction (310, only when the bank
// says so) and close.  0x1B0 bytes.  Method names are ours.
class VIWndBankMenu : public VIWnd
{
public:
    VIWndBankMenu();                                             // 0x00C18178
    virtual ~VIWndBankMenu();                                    // 0x00C181C0
    int  Init();                                                 // 0x00C18218
    int  Free();                                                 // 0x00C18228
    virtual void OnDrawSelf();                                   // 0x00C18238
    virtual void OnKeyDown(int key);                             // 0x00C18530
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C186E0
    void MoveCursor(int d);                                      // 0x00C187F0

    // Inline: emitted after __tf13VIWndBankMenu (0x00C194B8).
    void ResetCursor() { m_cursor = 0; }

    VIUIWindow m_window;        // +0x028
    int        m_cursor;        // +0x1A8
    int        m_auction;       // +0x1AC: the auction row is offered (VIBankControl::GetP78CC)
};
CHK(c_VIWndBankMenu, sizeof(VIWndBankMenu) == 0x1B0)

// VIWndBankMoney: ctor 0x00C194C0, 44 bytes (`new`), vtable 0x00CE20C0;
//   slots [0]0x00C1A4C0 [1]0x00C19500 [2]OnDraw__5VIWnd [3]OnDrawSelf__5VIWnd [4]OnKeyUp__5VIWndi [5]OnKeyDown__5VIWndi [6]0x00C196B0
// A VIWndMoneyMenu (deposit 291, withdraw 292, back 260) over a VIWndDeposit.  Method
// names are ours.
class VIWndBankMoney : public VIWnd
{
public:
    VIWndBankMoney();                                            // 0x00C194C0
    virtual ~VIWndBankMoney();                                   // 0x00C19500
    int  Init();                                                 // 0x00C19548
    int  Free();                                                 // 0x00C19618
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C196B0
    void Reset();                                                // 0x00C19860: the menu again

    VIWndMoneyMenu *m_menu;     // +0x024
    VIWndDeposit   *m_deposit;  // +0x028
};
CHK(c_VIWndBankMoney, sizeof(VIWndBankMoney) == 0x2C)

#endif
