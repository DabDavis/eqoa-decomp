// cl_wb.h -- windows: VIWndBlacksmith, VIWndBuddy, VIWndBuffPC, VIWndCharacterSelect, VIWndCharCreateBlank, VIWndCharCreateClass.
// CLIENT 0x00C1A510..0x00C26E50.
// TUs (one window class each, ending with its __tf):
//   VIWndBlacksmith            0x00C1A510..0x00C1C0E0
//   VIWndBuddy                 0x00C1C0E0..0x00C1E268
//   VIWndBuffPC                0x00C1E268..0x00C1FB90
//   VIWndCharacterSelect       0x00C1FB90..0x00C22C10
//   VIWndCharCreateBlank       0x00C22C10..0x00C240A8
//   VIWndCharCreateClass       0x00C240A8..0x00C26E50
#ifndef CL_WB_H
#define CL_WB_H

#ifndef CHK
#define CHK(n, c)  typedef char n[(c) || VI_PORT ? 1 : -1];
#endif

extern "C" double pow(double, double);
extern VIMenuDef gBuddyMenu;            // 0x00CD03B0: the buddy mini menu (VIWndBuddy::ShowMenu), .data

// VIWndCharacterSelect.cpp (0x00C1FBB8 ..): the character select screen behind
// VIClient+0x2BB84, 1,528 bytes (`new` in InitBody).  Up/down walk the eight slots and
// the delete slot (8), the highlighted character stands in the scene as a preview,
// Enter plays it (or, with Frontiers, first offers play/customise), slot 8 turns
// Enter into delete, an empty slot creates.  Its message box carries the server's
// text (ShowText/ShowWord from VIClient).  Method names provisional.
class VIWndCharacterSelect : public VIWnd
{
public:
    VIWndCharacterSelect();                                     // 0x00C1FBB8
    virtual ~VIWndCharacterSelect();                            // 0x00C1FC50
    int  Init(VIScene &scene, VIDictionary &dict, VIRaster &raster, VICamera &camera,
              VIWalkCamera &walkCam, VICollide &collide);       // 0x00C1FCA8 (LoadWorld)
    int  Free();                                                // 0x00C1FE20
    int  Connecting();                                          // 0x00C1FF08 (ConnectShard): the preview gone
    virtual void OnDrawSelf();                                  // 0x00C1FF88
    virtual void OnKeyDown(int key);                            // 0x00C20F28
    virtual void OnKeyUp(int key);                              // 0x00C211A8
    void MoveSel(int delta);                                    // 0x00C211E8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);  // 0x00C21320
    void ShowText(const char *s);                               // 0x00C21588
    void ShowText(const unsigned short *s);                     // 0x00C215F8
    void ShowWord(unsigned short id);                           // 0x00C21668 (CharSelectMsg masks to 16 bits)
    void ShowCharacter(unsigned int model, int race, int cls, int hairColor, int hairLength,
                       int hairStyle, int face, int robe, unsigned char robeTint, int pos,
                       int *weapons, unsigned char *attack, unsigned char *armor,
                       unsigned char *tints);                   // 0x00C216A8 (CreateCharacter's preview, CharacterList)
    void ApplyCamera();                                         // 0x00C21B08: the slot's race camera
    void Option(int on);                                        // 0x00C21C28: Enter (CharacterList passes 1)
    void ShowMessage(const unsigned short *s, int type, int p1, int p2);  // 0x00C21E68
    void ShowMessage(const char *s, int type, int p1, int p2);  // 0x00C21F10
    VICharRecord *GetSel();                                     // 0x00C21FB8

    // Inline: its copy follows __tf20VIWndCharacterSelect (0x00C22C10).
    void SetSel(int i) { m_sel = i; }

    unsigned short m_text[512];         // +0x024: the server's message
    int            m_deleting;          // +0x424: Enter deletes (the delete slot turned it on)
    int            m_sel;               // +0x428: the slot; 8 is "delete"
    int            m_actor;             // +0x42C: the preview's scene actor, -1 none
    int            m_attackSlot;        // +0x430: the preview's attack hand, -1 none
    long           m_showTime;          // +0x438: VIClient+0x7568 + 5000 when it was put up
    int            m_lastRace;          // +0x440: whose music is playing
    VIWndMBox     *m_mbox;              // +0x444
    VIRaster      *m_raster;            // +0x448
    int            m_p44C;              // +0x44C
    VIDictionary  *m_dict;              // +0x450
    VICamera      *m_camera;            // +0x454
    VIWalkCamera  *m_walkCam;           // +0x458
    VIScene       *m_scene;             // +0x45C
    VICollide     *m_collide;           // +0x460
    VIWndCharCustFrontiers *m_cust;     // +0x464
    VIUIWindow     m_window;            // +0x468
    int            m_busy;              // +0x5E8: a selection is under way (read with m_p5EC as one `ld`)
    int            m_p5EC;              // +0x5EC
    int            m_choosing;          // +0x5F0: the play / customise choice is up
    int            m_choice;            // +0x5F4: 0 play, 1 customise
};
CHK(c_VIWndCharacterSelect, sizeof(VIWndCharacterSelect) == 1528)

// VIWndCharCreateBlank.cpp (0x00C22C18 ..): the character-creation form behind
// VIClient+0x2BB88, 156 bytes (`new`).  It holds the form (a VINewChar) and the five
// pages that fill it in turn, each a child window: race -> class -> name -> misc (the
// appearance) -> review.  OnMessage moves between them: 260 a page went back, 262 the
// race (and sex and city) was picked, 263 the class, 265 the stats, 266 the name, 264
// the appearance, 267 create.  VIWndCharCreateRace DERIVES from it (cl_wc.h), so the
// layout below is fixed.  Method names provisional.
class VIWndCharCreateBlank : public VIWnd
{
public:
    VIWndCharCreateBlank();                                     // 0x00C22C18
    virtual ~VIWndCharCreateBlank();                            // 0x00C22C78
    int  Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam);  // 0x00C22CC0 (LoadWorld)
    int  Free();                                                // 0x00C22EF0
    virtual void OnDrawSelf();                                  // 0x00C23038: empty
    virtual void OnKeyDown(int key);                            // 0x00C23040
    virtual void OnKeyUp(int key);                              // 0x00C23080
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);  // 0x00C230A0
    VIWnd *Focus();                                             // 0x00C23360: the race page up, the others down (SelectCharacter)
    void Reset();                                               // 0x00C23398: the form and every page (CharacterList, BackToLogin)
    void SetName(const char *name);                             // 0x00C23460: into the name page's editor (0x0012)

    // Inline: their copies follow __tf20VIWndCharCreateBlank (0x00C240A8 .. 0x00C240E8).
    int        GetRace() { return m_char.m_race; }
    int        GetSex() { return m_char.m_sex; }
    int        GetClass() { return m_char.m_class; }
    int        GetFace() { return m_char.m_face; }
    int        GetHairStyle() { return m_char.m_hairStyle; }
    int        GetHairLength() { return m_char.m_hairLength; }
    int        GetHairColor() { return m_char.m_hairColor; }
    int        GetCity() { return m_char.m_p38; }
    VINewChar *GetChar() { return &m_char; }

    VINewChar              m_char;      // +0x024: CreateCharacter sends it
    VIWndCharCreateRace   *m_race;      // +0x07C
    VIWndCharCreateClass  *m_class;     // +0x080
    VIWndCharCreateMisc   *m_misc;      // +0x084
    VIWndCharCreateName   *m_name;      // +0x088
    VIWndCharCreateReview *m_review;    // +0x08C
    VICamera              *m_camera;    // +0x090: Init's, handed to every page
    VIWalkCamera          *m_walkCam;   // +0x094
    VIScene               *m_scene;     // +0x098
};
CHK(c_VIWndCharCreateBlank, sizeof(VIWndCharCreateBlank) == 156)

// VIWndBlacksmith: ctor 0x00C1A510, 2328 bytes (`new`), vtable 0x00CE23F0;
//   slots [0]0x00C1C090 [1]0x00C1A590 [2]OnDraw__5VIWnd [3]0x00C1A720 [4]0x00C1AEF8 [5]0x00C1ADF0 [6]0x00C1AF60
// One line of the blacksmith's stat comparison: the wide text and its colour.
struct VIBlacksmithLine
{
    unsigned short m_text[64];          // +0x00
    VIColor32F     m_color;             // +0x80
};

// VIWndBlacksmith.cpp (0x00C1A510 ..): the repair window, 2,328 bytes (`new` in
// VIWndGame's init).  A list of the inventory over a panel showing the highlighted
// item's description, its repair cost against the player's money, and up to six
// stat lines coloured by how the item compares with what is worn.  Method names
// provisional.
class VIWndBlacksmith : public VIWnd
{
public:
    VIWndBlacksmith();                                          // 0x00C1A510
    virtual ~VIWndBlacksmith();                                 // 0x00C1A590
    int  Init();                                                // 0x00C1A5F0
    int  Free();                                                // 0x00C1A6B0
    virtual void OnDrawSelf();                                  // 0x00C1A720
    virtual void OnKeyDown(int key);                            // 0x00C1ADF0
    virtual void OnKeyUp(int key);                              // 0x00C1AEF8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);  // 0x00C1AF60: 256 repair item p1, 295 opened
    void Clear();                                               // 0x00C1B060
    void Open();                                                // 0x00C1B0C8 (VIWndGame)
    void Refresh();                                             // 0x00C1B130 (OnDrawSelf, VIWndGame::Msg0060)
    void ShowStats(int item, VIArray<VIItemStat> *stats);       // 0x00C1B200
    void SetCost(int cost);                                     // 0x00C1B470

    int              m_money;           // +0x024: VIPlayer +0x34 when the cost was set
    int              m_cost;            // +0x028: -1 nothing to repair
    VIWndList       *m_list;            // +0x02C
    VIUIWindow       m_window;          // +0x030
    unsigned short   m_text[512];       // +0x1B0: the item's description
    VIBlacksmithLine m_lines[6];        // +0x5B0
    VIInfoText       m_info;            // +0x910
};
CHK(c_VIWndBlacksmith, sizeof(VIWndBlacksmith) == 2328)

// VIWndBuddy: ctor 0x00C1C0E0, 504 bytes (`new`), vtable 0x00CE27D8;
//   slots [0]0x00C1E218 [1]0x00C1C138 [2]OnDraw__5VIWnd [3]0x00C1C3C8 [4]0x00C1CEC0 [5]0x00C1CD70 [6]0x00C1D0A8
// VIWndBuddy.cpp (0x00C1C0E0 ..): the buddy list, 504 bytes (`new` in VIWndGame's
// init).  Ten rows of names; Enter on one opens a mini menu (send a gram, remove,
// ...), and an incoming buddy request is put up in the message box.  Method names
// provisional.
class VIWndBuddy : public VIWnd
{
public:
    VIWndBuddy();                                               // 0x00C1C0E0
    virtual ~VIWndBuddy();                                      // 0x00C1C138
    int  Init();                                                // 0x00C1C198
    int  Free();                                                // 0x00C1C2E8
    virtual void OnDrawSelf();                                  // 0x00C1C3C8
    virtual void OnKeyDown(int key);                            // 0x00C1CD70
    virtual void OnKeyUp(int key);                              // 0x00C1CEC0
    void ShowRequest();                                         // 0x00C1CF38: "x wants to be your buddy" in the message box
    void Close();                                               // 0x00C1D040
    void HideChildren();                                        // 0x00C1D090
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);  // 0x00C1D0A8
    void Move(int delta);                                       // 0x00C1D398
    void Clamp();                                               // 0x00C1D488: after the list changed
    void ShowMenu(const char *name);                            // 0x00C1D538

    // Inline: its copy follows __tf10VIWndBuddy (0x00C1E268).
    void ResetSel() { m_sel = 0; m_top = 0; }

    int               m_sel;            // +0x024
    int               m_top;            // +0x028: the first row shown
    int               m_count;          // +0x02C: GetBuddyCount when last clamped
    int               m_p30;            // +0x030
    VIWndMBox        *m_mbox;           // +0x034
    VIWndMiniMenu    *m_menu;           // +0x038
    VIWndComposeGram *m_compose;        // +0x03C
    VIUIWindow        m_window;         // +0x040
    VIKeyFlash        m_flash;          // +0x1C0
};
CHK(c_VIWndBuddy, sizeof(VIWndBuddy) == 504)

// VIWndBuffPC: ctor 0x00C1E278, 88 bytes (`new`), vtable 0x00CE2AD8;
//   slots [0]0x00C1FB40 [1]0x00C1E2E0 [2]OnDraw__5VIWnd [3]0x00C1E378 [4]OnKeyUp__5VIWndi [5]0x00C1E9F8 [6]0x00C1EB80
// VIWndBuffPC.cpp (0x00C1E278 ..): a two-digit number picker, 88 bytes (`new` in
// VIWndGame's init).  Left/right pick the digit, up/down change it (the tens 0..7, the
// units to 5 under 70, else 9 -- at most 75); Enter hands the number to
// VIClient::ShowA110.  Method names provisional.
class VIWndBuffPC : public VIWnd
{
public:
    VIWndBuffPC();                                              // 0x00C1E278
    virtual ~VIWndBuffPC();                                     // 0x00C1E2E0
    int  Init();                                                // 0x00C1E328
    int  Free();                                                // 0x00C1E360
    virtual void OnDrawSelf();                                  // 0x00C1E378
    virtual void OnKeyDown(int key);                            // 0x00C1E9F8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);  // 0x00C1EB80
    void MoveSel(int delta);                                    // 0x00C1EC30: the digit
    void ClearDigits();                                         // 0x00C1ECF8
    void AdjustDigit(int delta);                                // 0x00C1ED28
    int  Value();                                               // 0x00C1EE38
    void SetDefault();                                          // 0x00C1EEB0: 50; nothing calls it

    // Inline: their copies follow __tf11VIWndBuffPC (0x00C1FB90, 0x00C1FBA8).
    void SetPos(const VIVect2I &p) { m_pos = p; }
    void SelectUnits() { m_sel = 1; }

    int      m_sel;                     // +0x24: the digit, 0 the tens
    int      m_p28;                     // +0x28: 2
    VIVect2I m_pos;                     // +0x2C
    int      m_p34;                     // +0x34
    long     m_time;                    // +0x38: VIClient+0x7568 at the last key
    float    m_p40;                     // +0x40
    float    m_p44;                     // +0x44
    int      m_p48;                     // +0x48
    int      m_digits[2];               // +0x4C: tens, units
    int      m_p54;                     // +0x54
};
CHK(c_VIWndBuffPC, sizeof(VIWndBuffPC) == 88)

// VIWndCharacterSelect: ctor 0x00C1FBB8, 1528 bytes (`new`), vtable 0x00CE2E10;
//   slots [0]0x00C22BC0 [1]0x00C1FC50 [2]OnDraw__5VIWnd [3]0x00C1FF88 [4]0x00C211A8 [5]0x00C20F28 [6]0x00C21320
// VIWndCharCreateBlank: ctor 0x00C22C18, 156 bytes (`new`), vtable 0x00CE3130;
//   slots [0]0x00C24058 [1]0x00C22C78 [2]OnDraw__5VIWnd [3]0x00C23038 [4]0x00C23080 [5]0x00C23040 [6]0x00C230A0
// VIWndCharCreateClass: ctor 0x00C240F0, 1560 bytes (`new`), vtable 0x00CE3438;
//   slots [0]0x00C26E00 [1]0x00C241A0 [2]OnDraw__5VIWnd [3]0x00C24360 [4]0x00C25718 [5]0x00C253A8 [6]0x00C25910
// One row of the class list: a class and the WORDS.TXT id of its description (the
// static table sClasses, 0x00CE3490, fifteen rows in display order).  The name stays
// the placeholder client_viarray.cpp's VIArray copies were matched under.
struct VIRec8_C26D38
{
    int m_class;                    // +0x0
    int m_word;                     // +0x4: 380..394
};

// VIWndCharCreateClass.cpp (0x00C240F0 ..): the class page, 1560 bytes (`new`).  Page 0
// lists the classes the race, city and expansion allow; page 1 spends 20 points over
// the seven stats.  It answers its parent 263 (the class) and 265 (the stats, in a
// VINewChar), 260 to go back.  Method names provisional.
class VIWndCharCreateClass : public VIWnd
{
public:
    VIWndCharCreateClass();                                     // 0x00C240F0
    virtual ~VIWndCharCreateClass();                            // 0x00C241A0
    int  Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam);  // 0x00C24200
    int  Free();                                                // 0x00C24290
    int  Reset();                                               // 0x00C242F0
    virtual void OnDrawSelf();                                  // 0x00C24360
    virtual void OnKeyDown(int key);                            // 0x00C253A8
    virtual void OnKeyUp(int key);                              // 0x00C25718
    void Move(int delta, int sound);                            // 0x00C25758: the class row
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);  // 0x00C25910
    void SetRace(int race, int city);                           // 0x00C25A18 (the form's 262)
    void Refresh();                                             // 0x00C25A58
    void ApplyCamera();                                         // 0x00C25B48: the race camera
    void ClearText();                                           // 0x00C25C38
    void UpdateText();                                          // 0x00C25C40: the description
    void SetPage(int page);                                     // 0x00C25D30
    void SetBaseStats(int cls);                                 // 0x00C25D38: race + class bonuses
    void AdjustStat(int delta);                                 // 0x00C25DC8: the stat row
    void Pulse();                                               // 0x00C25F28
    void BuildList();                                           // 0x00C25F68: the classes on offer

    VICamera      *m_camera;        // +0x024
    VIWalkCamera  *m_walkCam;       // +0x028
    VIScene       *m_scene;         // +0x02C
    int            m_sel;           // +0x030: the highlighted row
    int            m_race;          // +0x034
    int            m_city;          // +0x038
    int            m_class;         // +0x03C: -1 none yet
    int            m_points;        // +0x040: left to spend, 20 to start
    int            m_page;          // +0x044: 0 the classes, 1 the stats
    int            m_stats[7];      // +0x048: STR STA AGI DEX WIS INT CHA
    int            m_p64;           // +0x064
    int            m_p68;           // +0x068
    float          m_p6C;           // +0x06C
    long           m_time;          // +0x070: VIClient+0x7568 when it last pulsed
    VIColor32F     m_color;         // +0x078
    VIUIWindow     m_window;        // +0x088
    unsigned short m_text[512];     // +0x208: the highlighted row's description
    VIArray<VIRec8_C26D38> m_list;  // +0x608
    int            m_count;         // +0x610: rows in m_list
    int            m_p614;          // +0x614
};
CHK(c_VIWndCharCreateClass, sizeof(VIWndCharCreateClass) == 1560)


#endif
