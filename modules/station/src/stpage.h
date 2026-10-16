// stpage.h -- OWNER: lane page.  A page of GUI objects and the PS2GUIPage XML
// parser that builds one.  TUs: VIGUIDialog (0x002BFD18 .. 0x002C10B8),
// VIGUIPage + VIGUISelectionGrid (0x002C2610 .. 0x002C45F8), the parser
// (0x002C45F8 .. 0x002C73D0).  The method names are ours unless a string
// names them; the field names are what the code does with them.
#ifndef STPAGE_H
#define STPAGE_H

// The cursor grid a page moves its selection over: m_cols columns of m_rows
// cells, each a VIGUIObject or empty.  0x18 bytes, vptr at +0x14.
// ctor 0x002C2610, virtual dtor 0x002C26D8, __tf 0x002C4578.
class VIGUISelectionGrid
{
public:
    VIGUISelectionGrid();
    virtual ~VIGUISelectionGrid();

    void Init(int cols, int rows);                      // 0x002C2630
    void Set(VIGUIObject *o, int col, int row);         // 0x002C27B8
    void Select(const char *name);                      // 0x002C2840
    VIGUIObject *GetSelected();                         // 0x002C2950
    // 0x002C2988: the pad's arrows (136 left, 137 right, 138 up, 139 down)
    // move to the next occupied cell.  0 = it moved, 1 = no cell that way,
    // 2 = no grid.
    int  Move(int key);

    int            m_cols;    // +0x00
    int            m_rows;    // +0x04
    int            m_col;     // +0x08  the selection
    int            m_row;     // +0x0C
    VIGUIObject ***m_cells;   // +0x10  m_cells[col][row]
};
CHK(c_VIGUISelectionGrid, sizeof(VIGUISelectionGrid) == 0x18)

// A page: its objects in drawing order (by depth), the selection grid over
// them and a pulsing selector drawn on the selected one.  0x7C bytes (new in
// 0x002C6438), vptr at +0x78.  __tf 0x002C45B8.
class VIGUIPage
{
public:
    VIGUIPage();                                        // 0x002C2BB8
    VIGUIPage(const char *name);                        // 0x002C2C00
    virtual ~VIGUIPage();                               // 0x002C2C60 "Page destructor for %s"
    // Slots 2..4: what the renderer hands the page, passed on to the
    // selected object (VIGUIObject slots 8, 9, 10).
    virtual void OnKeyUp(int key);                      // 0x002C3518
    virtual void OnKeyDown(int key);                    // 0x002C3560
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002C3668

    void Init(VIWndGenericRenderer *owner, int cols, int rows);  // 0x002C2D80
    void Reset();                                       // 0x002C2DA8
    const char *GetName();                              // 0x002C2E08
    void Select(const char *name);                      // 0x002C2E10
    void SetSelector(VIGUIObject *o);                   // 0x002C2E30
    void SetSelectorX(short x);                         // 0x002C2E78
    void SetSelectorY(short y);                         // 0x002C2E80
    void SetName(const char *name);                     // 0x002C2E88
    void AddObject(VIGUIObject *o);                     // 0x002C2EA8 by depth
    void AddObject(VIGUIObject *o, int col, int row);   // 0x002C2F58 and into the grid
    VIGUIObject *FindObject(const char *name);          // 0x002C2FB8
    const char *GetText(const char *name);              // 0x002C3058 Text or ComboBox
    int  GetToggle(const char *name);                   // 0x002C30C8
    void SetToggle(const char *name, int on);           // 0x002C3130
    void ClearOptions(const char *name);                // 0x002C3198 ComboBox
    void AddOption(const char *name, const char *s);    // 0x002C31E8 ComboBox
    void SetText(const char *name, const char *s);      // 0x002C3240 Text or ComboBox
    void SetTextColor(const char *name, VIColor32F c);  // 0x002C32B0 Text or ComboBox
    void Draw();                                        // 0x002C3328

    char                  m_name[64];     // +0x00
    VIList<VIGUIObject *> m_objects;      // +0x40  by depth, the deepest first
    VIGUISelectionGrid    m_grid;         // +0x4C
    VIWndGenericRenderer *m_owner;        // +0x64
    VIGUIObject          *m_selector;     // +0x68  drawn over the selection
    short                 m_selectorX;    // +0x6C  -1: the selection's own x
    short                 m_selectorY;    // +0x6E  -1: the selection's own y
    float                 m_pulse;        // +0x70  0.3 .. 1.0, the selector's alpha
    float                 m_pulseStep;    // +0x74  +-0.03 a frame
};
CHK(c_VIGUIPage, sizeof(VIGUIPage) == 0x7C)

// A message dialog over a page: a VIUIWindow of up to 1,024 characters of
// text, scrolled seven rows at a time, and the action it answers with
// ("DisconnectConfirmed").  0xE18 bytes (new in 0x002C9E28).  ctor 0x002BFD18,
// dtor 0x002BFD68, __tf 0x002C1068.
class VIGUIDialog : public VIGUIPage
{
public:
    VIGUIDialog();
    virtual ~VIGUIDialog();
    virtual void OnKeyUp(int key);                      // 0x002C02D8 (empty)
    virtual void OnKeyDown(int key);                    // 0x002C02E0
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x002C04A0 (empty)

    void Init(VIWndGenericRenderer *owner, int cols, int rows);  // 0x002BFDE0
    void SetText(const unsigned short *s);              // 0x002BFE80
    void SetText(const char *s);                        // 0x002BFEE0
    int  IsOpen();                                      // 0x002BFF40
    void SetConfirm();                                  // 0x002BFF98
    void ClearConfirm();                                // 0x002BFFA8
    void Draw();                                        // 0x002BFFB0
    void Open(const unsigned short *title, const char *action);  // 0x002C01D8
    void Open(const char *title, const char *action);   // 0x002C0230
    void Close();                                       // 0x002C02B0
    void SetPrompt(int on);                             // 0x002C02D0

    VIUIWindow     m_window;        // +0x080
    unsigned short m_text[1024];    // +0x200
    char           m_action[1024];  // +0xA00
    int            m_confirm;       // +0xE00  X closes, O sends m_action
    int            m_rows;          // +0xE04  the text's
    int            m_scroll;        // +0xE08  the first row shown
    int            m_xE0C;          // +0xE0C  only ever cleared
    int            m_prompt;        // +0xE10  a prompt row under the text
};
CHK(c_VIGUIDialog, sizeof(VIGUIDialog) == 0xE18)

// The PS2GUIPage parser (name ours): what one load of a page file carries
// through the XML reader's handlers.  12 bytes, on the renderer's stack.
// ctor 0x002C45F8; the accessors 0x002C6740 .. 0x002C6760 are defined at
// the end of the TU, after every use, so they are called and not inlined.
class VIGUIParser
{
public:
    VIGUIParser(VIWndGenericRenderer *owner);
    VIGUIPage *LoadFile(const char *path);              // 0x002C6588
    VIGUIPage *LoadMemory(const char *buf, int len);    // 0x002C66A8

    void SetPage(VIGUIPage *p);                         // 0x002C6740
    VIGUIPage *GetPage();                               // 0x002C6748
    VIWndGenericRenderer *GetOwner();                   // 0x002C6750
    void SetObject(VIGUIObject *o);                     // 0x002C6758
    VIGUIObject *GetObject();                           // 0x002C6760

    VIGUIPage            *m_page;     // +0x00 what the file built
    VIGUIObject          *m_object;   // +0x04 the element being read
    VIWndGenericRenderer *m_owner;    // +0x08
};
CHK(c_VIGUIParser, sizeof(VIGUIParser) == 12)

// 0x002C4610: a UI texture's name in a page file, else its number.
int VIGUITextureId(const char *name);

#endif
