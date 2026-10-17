// cl_wq.h -- windows: VIWndQuestLog, VIWndReadCS, VIWndReadGram, VIWndRecipe, VIWndReply, VIWndResponse, VIWndServerSelect.
// CLIENT 0x00C860C8..0x00C93680.
// TUs (one window class each, ending with its __tf):
//   VIWndQuestLog              0x00C860C8..0x00C87810
//   VIWndReadCS                0x00C87810..0x00C89A58
//   VIWndReadGram              0x00C89A58..0x00C8B578
//   VIWndRecipe                0x00C8B578..0x00C8E360
//   VIWndReply                 0x00C8E360..0x00C8F950
//   VIWndResponse              0x00C8F950..0x00C910C0
//   VIWndServerSelect          0x00C910C0..0x00C93680
#ifndef CL_WQ_H
#define CL_WQ_H
#ifndef CHK
#define CHK(n, c)  typedef char n[(c) ? 1 : -1];
#endif

#ifndef CHK
#define CHK(n, c)  typedef char n[(c) ? 1 : -1];
#endif

// Skeletons (lanes of 2026-10-16): fill each in as its TU is written.
// VIWndQuestLog: ctor 0x00C860D0, 496 bytes (`new`), vtable 0x00CEE500;
//   slots [0]0x00C877C0 [1]0x00C86130 [2]OnDraw__5VIWnd [3]0x00C862C0 [4]0x00C866F8 [5]0x00C865E8 [6]0x00C86770
// The quest journal: the quest titles, one TextBox each, scrolled so the selection shows;
// X on a quest asks (VIWndMBox, word 574) and on yes sends 0x007D with its index.  Opened
// by the game window as its panel 22 (VIWndGame::ClosePanel6DC hides it).  496 bytes.
// Method names ours.
class VIWndQuestLog : public VIWnd
{
public:
    VIWndQuestLog();                                             // 0x00C860D0
    virtual ~VIWndQuestLog();                                    // 0x00C86130
    int  Init();                                                 // 0x00C86190
    int  Free();                                                 // 0x00C86248
    virtual void OnDrawSelf();                                   // 0x00C862C0
    virtual void OnKeyDown(int key);                             // 0x00C865E8
    virtual void OnKeyUp(int key);                               // 0x00C866F8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C86770
    void Open();                                                 // 0x00C868F8: VIWndGame's panel 22
    void HideBox();                                              // 0x00C86938
    void ClampSel();                                             // 0x00C86948
    void Next();                                                 // 0x00C869B8
    void Prev();                                                 // 0x00C86A60
    void Select();                                               // 0x00C86AF0
    int  Count();                                                // 0x00C86B80: the client's quest count
    int  Title(int i, unsigned short *buf);                      // 0x00C86BA8

    // Inline: its copy follows __tf13VIWndQuestLog (0x00C87810).
    void Rewind() { m_sel = 0; m_top = 0; }

    int          m_sel;          // +0x024  the selected quest
    int          m_top;          // +0x028  the first quest shown
    int          m_active;       // +0x02C  TickR's pulse: 1 while the window has the focus
    int          m_pageEnd;      // +0x030  the last quest the first page shows
    int          m_last;         // +0x034  the last quest drawn
    VIUIWindow   m_window;       // +0x038
    VIKeyFlash   m_keys;         // +0x1B8
    VIWndMBox   *m_mbox;         // +0x1EC
};
CHK(c_VIWndQuestLog, sizeof(VIWndQuestLog) == 496)

// VIWndReadCS: ctor 0x00C87820, 46944 bytes (`new`), vtable 0x00CEE928;
//   slots [0]0x00C89A08 [1]0x00C87890 [2]OnDraw__5VIWnd [3]0x00C87B68 [4]0x00C88640 [5]0x00C88470 [6]0x00C88690
// STATION's VIWndReadCS (stwndcs.h; STATION 0x002F2C38 ..), the same source: one message
// read from the customer-service inbox.  CLIENT's differs where STATION reaches through
// VIStation: the CS client is the app's slot-8 sub-object (VIClientI::GetP33950), sounds
// go through VIClient::PlaySound, the clock is VIClient+0x7568, the deleted message is
// VIClient::ShowMsg(987).  0xB760 bytes.
// One row of the wrapped text: where it starts in m_full and how many characters.
struct VIReadCSLine
{
    unsigned short *m_start;   // +0x00  0 past the last row
    short           m_len;     // +0x04
    short           m_break;   // +0x06  1 when a line break or a wrap ends the row
};

class VIWndReadCS : public VIWnd
{
public:
    VIWndReadCS();                                               // 0x00C87820
    virtual ~VIWndReadCS();                                      // 0x00C87890

    int  Init();                                                 // 0x00C87908
    int  Free();                                                 // 0x00C87A50

    virtual void OnDrawSelf();                                   // 0x00C87B68
    virtual void OnKeyDown(int key);                             // 0x00C88470
    virtual void OnKeyUp(int key);                               // 0x00C88640
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C88690

    void Next();                                                 // 0x00C88A10
    void Prev();                                                 // 0x00C88A78
    const unsigned short *Category();                            // 0x00C88AD0
    void SetCategory(const unsigned short *s);                   // 0x00C88AD8
    const unsigned short *Subcategory();                         // 0x00C88B18
    void SetSubcategory(const unsigned short *s);                // 0x00C88B20
    const unsigned short *Text();                                // 0x00C88B60
    void SetText(const unsigned short *s);                       // 0x00C88B68
    const unsigned short *Date();                                // 0x00C88BA8
    void SetDate(const unsigned short *s);                       // 0x00C88BB0
    int  Wrap(unsigned short *text, VIReadCSLine *lines, int len,
              int maxLines, int width);                          // 0x00C88BF0

    // Inline: their copies follow __tf11VIWndReadCS (0x00C89A58 .. 0x00C89A80).
    void SetCategoryId(int id) { m_categoryId = id; }
    void SetCategoryIds(int id, int sub) { m_categoryId = id; m_subcategoryId = sub; }
    int  GetId() { return m_id; }
    void SetId(int id) { m_id = id; }

    VIUIWindow          m_window;          // +0x0028
    VIUIWindow          m_status;          // +0x01A8
    VIWndMessage        m_message;         // +0x0328
    VIWndCSRequest      m_request;         // +0x07A8  the reply
    VIWndOptionList    *m_options;         // +0x11A8  Reply / Delete
    unsigned short      m_category[64];    // +0x11AC
    unsigned short      m_subcategory[64]; // +0x122C
    unsigned short      m_date[64];        // +0x12AC
    unsigned short      m_text[8192];      // +0x132C
    int                 m_categoryId;      // +0x532C
    int                 m_subcategoryId;   // +0x5330
    unsigned short      m_full[8704];      // +0x5334  the labels and the text, Wrap's input
    VIReadCSLine        m_lines[1024];     // +0x9734
    int                 m_top;             // +0xB734  first row shown
    int                 m_rows;            // +0xB738
    int                 m_deleting;        // +0xB73C  a delete is under way
    int                 m_id;              // +0xB740  the message's
    int                 m_key;             // +0xB744
    float               m_repeat;          // +0xB748
    long                m_time;            // +0xB750
    int                 m_dirty;           // +0xB758  re-wrap the text
};
CHK(c_VIWndReadCS, sizeof(VIWndReadCS) == 0xB760)

// VIWndReadGram: ctor 0x00C89A90, 8672 bytes (`new`), vtable 0x00CEED38;
//   slots [0]0x00C8B528 [1]0x00C89B00 [2]OnDraw__5VIWnd [3]0x00C89DA8 [4]0x00C8A3E0 [5]0x00C8A248 [6]0x00C8A458
// One gram (in-game mail) read from the inbox: VIWndReadCS's twin for the gram client
// (VIClient+0x2EFD0, VIClientPart2EFD0): From/Subject/Date over the text, scrolled seven
// rows at a time; Enter offers Reply (the embedded VIWndComposeGram, as a reply, the
// subject prefixed with word 936) and Delete.  8672 bytes.  Method names ours.
class VIWndReadGram : public VIWnd
{
public:
    VIWndReadGram();                                             // 0x00C89A90
    virtual ~VIWndReadGram();                                    // 0x00C89B00
    int  Init();                                                 // 0x00C89B88
    void Free();                                                 // 0x00C89CB0
    virtual void OnDrawSelf();                                   // 0x00C89DA8
    virtual void OnKeyDown(int key);                             // 0x00C8A248
    virtual void OnKeyUp(int key);                               // 0x00C8A3E0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C8A458
    const unsigned short *From();                                // 0x00C8A7A0
    void SetFrom(const unsigned short *s);                       // 0x00C8A7A8
    const unsigned short *Subject();                             // 0x00C8A7E0
    void SetSubject(const unsigned short *s);                    // 0x00C8A7E8
    const unsigned short *Date();                                // 0x00C8A820
    void SetDate(const unsigned short *s);                       // 0x00C8A828
    const unsigned short *Text();                                // 0x00C8A860
    void SetText(const unsigned short *s);                       // 0x00C8A868
    void Prev();                                                 // 0x00C8A8A0
    void Next();                                                 // 0x00C8A8F0

    // Inline: their copies follow __tf13VIWndReadGram (0x00C8B578, 0x00C8B580).
    int  GetId() { return m_id; }
    void SetId(int id) { m_id = id; }

    VIUIWindow          m_window;          // +0x0028
    VIUIWindow          m_status;          // +0x01A8
    VIWndMessage        m_message;         // +0x0328
    VIWndComposeGram    m_compose;         // +0x07A8  the reply
    VIWndOptionList    *m_options;         // +0x1410  Reply / Delete
    unsigned short      m_from[64];        // +0x1414
    unsigned short      m_subject[64];     // +0x1494
    unsigned short      m_date[64];        // +0x1514
    unsigned short      m_text[512];       // +0x1594
    unsigned short      m_full[1024];      // +0x1994  the labels and the text
    int                 m_rows;            // +0x2194
    int                 m_top;             // +0x2198  first row shown
    int                 m_deleting;        // +0x219C  1 a delete is under way, -1 it failed
    int                 m_id;              // +0x21A0  the gram's
    int                 m_dirty;           // +0x21A4  re-count the rows
    VIKeyFlash          m_keys;            // +0x21A8
};
CHK(c_VIWndReadGram, sizeof(VIWndReadGram) == 8672)

// VIWndRecipe: ctor 0x00C8B588, 1064 bytes (`new`), vtable 0x00CEF0C8;
//   slots [0]0x00C8E310 [1]0x00C8B608 [2]OnDraw__5VIWnd [3]0x00C8B9A0 [4]0x00C8C768 [5]0x00C8C630 [6]0x00C8C7E0
// A recipe row, 144 bytes: VIClientPart3834C fills VIWndRecipe's list with them (it was
// client_viarray.cpp's placeholder; the name is kept, the VIArray copies carry it).
// Field names ours.
// VIRec144_C017C0 is cl-parts2's (game.h).

// The tradeskill window: the recipes of a skill for a component (VIClientPart3834C,
// VIClient's vtable slot 12, supplies them), sortable by three columns, and a pop-up menu
// on a recipe (1525 / 122 / 200 / 516: combine, inspect, ...).  Opened as the game
// window's panel at +0x708 (ClosePanel708 closes it).  1064 bytes.  Method names ours.
class VIWndRecipe : public VIWnd
{
public:
    VIWndRecipe();                                               // 0x00C8B588
    virtual ~VIWndRecipe();                                      // 0x00C8B608
    int  Init();                                                 // 0x00C8B690
    int  Free();                                                 // 0x00C8B8D0
    virtual void OnDrawSelf();                                   // 0x00C8B9A0
    virtual void OnKeyDown(int key);                             // 0x00C8C630
    virtual void OnKeyUp(int key);                               // 0x00C8C768
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C8C7E0
    void Prev();                                                 // 0x00C8CB20
    void Next();                                                 // 0x00C8CBF0
    void Left();                                                 // 0x00C8CCE0: empty
    void Right();                                                // 0x00C8CCE8: empty
    void Select();                                               // 0x00C8CCF0
    void Back();                                                 // 0x00C8CDC8
    void ShowMenu();                                             // 0x00C8CE38
    void Pick();                                                 // 0x00C8CF10
    int  Unused8CF88();                                          // 0x00C8CF88: returns 0
    int  Components();                                           // 0x00C8CF90: by skill
    const unsigned short *ComponentName(int i);                  // 0x00C8CFC8
    const unsigned short *Difficulty(int d);                     // 0x00C8D0D0
    const unsigned short *SkillName(int skill);                  // 0x00C8D198
    void Learn(int id);                                          // 0x00C8D248
    void Forget(int id);                                         // 0x00C8D2B8
    void Confirm(int id);                                        // 0x00C8D318
    void StartPick(int id);                                      // 0x00C8D3E8
    void SetSkill(int skill);                                    // 0x00C8D430
    void Message(const unsigned short *text, int x);             // 0x00C8D438
    void Ask(const unsigned short *text, int p1, int p2);        // 0x00C8D4E0
    void Inspect();                                              // 0x00C8D598

    VIWndInspect   *m_inspect;      // +0x024
    VIWndMiniMenu  *m_menu;         // +0x028
    VIWndMBox      *m_mbox;         // +0x02C
    VIUIWindow      m_window;       // +0x030  the recipes, then (re-inited) the components
    VIUIWindow      m_status;       // +0x1B0
    VIInfoText      m_info;         // +0x330
    int             m_skill;        // +0x334
    int             m_comp;         // +0x338  the component selected
    int             m_compTop;      // +0x33C
    int             m_sel;          // +0x340  the recipe selected
    int             m_sort;         // +0x344  0..2: the column shown highlighted
    int             m_top;          // +0x348
    int             m_pick;         // +0x34C  -1 none
    VIKeyFlash      m_keys;         // +0x350
    int             m_mode;         // +0x384  0 components, 1 recipes, 2 picking
    VIArray<VIRec144_C017C0> m_list; // +0x388
    VIMenuDef       m_menuDef;      // +0x390: the pop-up menu (combine, inspect, ...)
    VIMenuEntry     m_menuItems[4]; // +0x3A0: ... and its four entries
    char            m_dots;         // +0x420
    char            m_ticks;        // +0x421
};
CHK(c_VIWndRecipe, sizeof(VIWndRecipe) == 1064)

// VIWndReply: ctor 0x00C8E360, 608 bytes (`new`), vtable 0x00CEF488;
//   slots [0]0x00C8F900 [1]0x00C8E3A0 [2]OnDraw__5VIWnd [3]0x00C8E4D0 [4]OnKeyUp__5VIWndi [5]0x00C8E6E0 [6]0x00C8E818
// The tell reply box: To and the text, each typed in VIWndEdit, then Send sends the chat
// line "/t <to> <text>" (VIClient's chat input, 0x00BCFE78).  The game window's panel at
// +0x6F8 (ClosePanel6F8 closes it).  608 bytes.  Method names ours.
class VIWndReply : public VIWnd
{
public:
    VIWndReply();                                                // 0x00C8E360
    virtual ~VIWndReply();                                       // 0x00C8E3A0
    int  Init();                                                 // 0x00C8E3F8
    void Free();                                                 // 0x00C8E468
    virtual void OnDrawSelf();                                   // 0x00C8E4D0
    virtual void OnKeyDown(int key);                             // 0x00C8E6E0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C8E818
    const unsigned short *To();                                  // 0x00C8E950
    void SetTo(const unsigned short *s);                         // 0x00C8E958
    const unsigned short *Text();                                // 0x00C8E980
    void SetText(const unsigned short *s);                       // 0x00C8E988
    void Start(const unsigned short *to);                        // 0x00C8E9B0: the text first when `to` is given
    void Stub8EA00();                                            // 0x00C8EA00: empty
    void Next();                                                 // 0x00C8EA08
    void Prev();                                                 // 0x00C8EA78
    void Select();                                               // 0x00C8EAE0
    void TakeText();                                             // 0x00C8ECB0

    VIUIWindow      m_window;       // +0x028
    VIWndEdit      *m_edit;         // +0x1A8
    int             m_sel;          // +0x1AC  0 To, 1 the text, 2 Send
    unsigned short  m_to[24];       // +0x1B0
    unsigned short  m_text[64];     // +0x1E0
};
CHK(c_VIWndReply, sizeof(VIWndReply) == 608)

// VIWndResponse: ctor 0x00C8F950, 2512 bytes (`new`), vtable 0x00CEF7A8;
//   slots [0]0x00C91070 [1]0x00C8F9C0 [2]OnDraw__5VIWnd [3]0x00C8FAA0 [4]0x00C8FF78 [5]0x00C8FCF0 [6]0x00C90148
// One answer row of VIWndResponse, 130 bytes: its text, the dialog option index it
// answers with, and what choosing it does (1 answer, 2 more, 3 back, 4 close).
struct VIResponseOption
{
    unsigned short m_text[64];      // +0x00
    signed char    m_index;         // +0x80
    signed char    m_type;          // +0x81
};

// An NPC dialog's answers (VIClient's 0x0034 dialog box, VIDialogBox): the text, then
// three options a page, More/Back, and Close; the choice goes back as 0x0035.  2512
// bytes.  Method names ours.
class VIWndResponse : public VIWnd
{
public:
    VIWndResponse();                                             // 0x00C8F950
    virtual ~VIWndResponse();                                    // 0x00C8F9C0
    int  Init();                                                 // 0x00C8FA10: returns 0
    void Clear();                                                // 0x00C8FA18
    virtual void OnDrawSelf();                                   // 0x00C8FAA0
    virtual void OnKeyDown(int key);                             // 0x00C8FCF0
    virtual void OnKeyUp(int key);                               // 0x00C8FF78: empty
    void Move(int d);                                            // 0x00C8FF80
    void SetDialog(const VIDialogBox &d);                        // 0x00C90058
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C90148
    void Build(const VIDialogBox &d);                            // 0x00C901A0

    // Inline: its copy follows __tf13VIWndResponse (0x00C910C0).
    void SetPos(const VIVect2I &p) { m_pos = p; }

    int              m_sel;         // +0x024  the option chosen (a dialog index)
    int              m_first;       // +0x028  the first option on the page
    int              m_dialogId;    // +0x02C  the dialog's id (0x0035's key)
    unsigned short   m_text[128];   // +0x030
    unsigned char    m_count;       // +0x130  the dialog's option count
    VIResponseOption m_option[6];   // +0x132
    VIDialogBox      m_dialog;      // +0x440  a copy (SetDialog)
    VIVect2I         m_pos;         // +0x848
    VIUIWindow       m_window;      // +0x850
};
CHK(c_VIWndResponse, sizeof(VIWndResponse) == 2512)

// VIWndServerSelect: ctor 0x00C910D8, 1904 bytes (`new`), vtable 0x00CEFC20;
//   slots [0]0x00C93630 [1]0x00C91130 [2]OnDraw__5VIWnd [3]0x00C91278 [4]0x00C91BC8 [5]0x00C91BD0 [6]0x00C91F38
// The server (shard) select screen, VIClient+0x2BB80: the shard list (VIShardInfo, the
// client's +0x38320) sorted by name, the home shard preselected, a countdown and then
// ConnectShard.  Before the list arrives it connects to a random shard for it
// (UpdateList); a failed reconnect is UpdateConnect's.  1904 bytes.  Method names ours.
class VIWndServerSelect : public VIWnd
{
public:
    VIWndServerSelect();                                         // 0x00C910D8
    virtual ~VIWndServerSelect();                                // 0x00C91130
    int  Init(VIShardInfo *shards);                              // 0x00C911C8
    void Free();                                                 // 0x00C91220
    virtual void OnDrawSelf();                                   // 0x00C91278
    virtual void OnKeyUp(int key);                               // 0x00C91BC8: empty
    virtual void OnKeyDown(int key);                             // 0x00C91BD0
    int  Connect();                                              // 0x00C91E48: save the options, ConnectShard
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C91F38
    void Next();                                                 // 0x00C92240
    void Prev();                                                 // 0x00C922F8
    void Sort();                                                 // 0x00C923A0
    static int Compare(const void *a, const void *b);            // 0x00C92590
    int  UpdateList();                                           // 0x00C92608: -1 failed, 1 waiting, 0 done
    int  UpdateConnect();                                        // 0x00C92918: -1 failed, 1 connecting, 0

    VIUIWindow      m_window;       // +0x028
    VIUIWindow      m_status;       // +0x1A8  "fetching the list..." / "connecting..."
    VIUIWindow      m_box;          // +0x328  the countdown, the memory card's problem
    VIWndMessage   *m_message;      // +0x4A8
    VIShardInfo    *m_shards;       // +0x4AC
    VIWnd          *m_child;        // +0x4B0  added by 4352/4354, removed on the next draw
    int             m_connecting;   // +0x4B4
    int             m_sel;          // +0x4B8  an index into m_order
    unsigned short  m_name[32];     // +0x4BC  the selected shard's
    unsigned short  m_home[32];     // +0x4FC  the home shard's
    int             m_rows;         // +0x53C  rows a page: 12
    int             m_top;          // +0x540
    int             m_cardError;    // +0x544  the memory card would not save
    char            m_x548[0x648 - 0x548];
    unsigned char   m_order[256];   // +0x648  shard indices, sorted by name
    int             m_failed;       // +0x748  the list could not be had
    int             m_fetching;     // +0x74C  waiting for the list
    int             m_requested;    // +0x750  ConnectRandomShard called
    int             m_retries;      // +0x754
    int             m_delay;        // +0x758  frames to the next try
    int             m_reconnectFailed; // +0x75C
    int             m_invalid;      // +0x760  the "not available" message is up
    int             m_empty;        // +0x764  the "no shards" message is up
    int             m_countdown;    // +0x768  frames to Connect()
};
CHK(c_VIWndServerSelect, sizeof(VIWndServerSelect) == 1904)

#endif
