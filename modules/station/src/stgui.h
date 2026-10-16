// stgui.h -- OWNER: lane gui.  The GUI object classes STATION.PAK's pages are
// built from.  TUs, in link order: the three data models (0x002BD010 ..
// 0x002BD5C0: viguidatamodel, viguistringdatamodel, viguitoggledatamodel),
// viguibutton (.. 0x002BE3E8), viguicombobox (.. 0x002BFD18), viguiobject
// (0x002C10B8 .. 0x002C2610), viguitext (0x002C73D0 .. 0x002C8B00),
// viguitogglebutton (.. 0x002C98B8).  Sizes are the page parser's `new`s
// (0x002C5E78), each followed by its constructor; the attribute names in the
// comments are the PS2GUIPage XML ones that parser maps onto each setter.
// Method names are ours unless a string names them; where lane page's parser
// calls one, the name is the one page chose (mostly the XML attribute's).
#ifndef STGUI_H
#define STGUI_H

// Lane misc's XML reader (stwnd.h): the page parser builds every object with it,
// and the object keeps it at +0x2C.
class VIXMLParser;

// What the GUI objects reach through their renderer window.  The window's app
// pointer is the VIStation (lane core owns its layout), and the renderer's
// first field is its VIWndEdit (wave 2 owns the renderer, lane core VIWndEdit).
// Until those layouts are written these are offsets: each is the same RTL as
// the member access it stands for.  [lane gui]
#define ST_RASTER(app)      ((VIRaster *)((char *)(app) + 0x200))
#define ST_UI(app)          ((VIUI *)((char *)(app) + 0xAFE0))
#define ST_WM(app)          ((VIWnd *)((char *)(app) + 0xB388))   // its window manager
#define ST_FOCUSWND(app)    (*(VIWnd **)((char *)(app) + 0xB40C))
#define RENDERER_EDIT(r)    (*(VIWndEdit **)((char *)(r) + 0x28))
#define EDIT_PASSWORD(e)    (*(int *)((char *)(e) + 0x18C))
#define EDIT_TEXT(e)        (*(unsigned short **)((char *)(e) + 0x1A8))
// VIStation's focus switch: the window is loaded ONCE for the store and the
// call, so it is an inline taking it as a parameter (a VIStation member in the
// original; lane core owns that class).  [lane gui]
inline void ST_SetActiveWindow(void *app, VIWnd *w)
{
    ST_FOCUSWND(app) = w;
    ST_WM(app)->SetFocus(w, true);
}

// ---- viguidatamodel.cpp -- 0x002BD010 .. 0x002BD178 ------------------------
// A value a GUI object edits.  0x28 bytes, vptr at +0x24.  The destructor is
// its only virtual and it is inline, so the class has no key function: its
// vtable, __tf and destructor are emitted in each of the three data-model TUs
// (__tf 0x002BD108 / 0x002BD340 / 0x002BD550, dtor 0x002BD148 / 0x002BD380 /
// 0x002BD590), and the linker resolved everything to the first.
class VIGUIDataModel
{
public:
    VIGUIDataModel(int type);                                   // 0x002BD010
    VIGUIDataModel(const char *name, int type);                 // 0x002BD048
    virtual ~VIGUIDataModel() {}

    void  Init();                                               // 0x002BD098
    void  SetName(const char *name);                            // 0x002BD0D0
    char *GetName();                                            // 0x002BD0F0
    void  SetType(int type);                                    // 0x002BD0F8
    int   GetType();                                            // 0x002BD100

    char m_name[32];                                            // +0x00
    int  m_type;                                                // +0x20  1 a string, 2 a toggle
};
CHK(c_VIGUIDataModel, sizeof(VIGUIDataModel) == 0x28)

// ---- viguistringdatamodel.cpp -- 0x002BD178 .. 0x002BD428 ------------------
// 0x30 bytes.  __tf 0x002BD3B0 (the base's __tf, emitted first here, is
// inlined into it).
class VIGUIStringDataModel : public VIGUIDataModel
{
public:
    VIGUIStringDataModel();                                     // 0x002BD178
    VIGUIStringDataModel(const char *name);                     // 0x002BD1B8
    void  Reset();                                              // 0x002BD1F8
    virtual ~VIGUIStringDataModel();                            // 0x002BD208
    void  SetString(const char *s);                             // 0x002BD278  a copy, cut to m_maxLen
    const char *GetString();                                    // 0x002BD328
    void  SetMaxLength(int n);                                  // 0x002BD330
    int   GetMaxLength();                                       // 0x002BD338

    char *m_string;                                             // +0x28  malloc'd
    int   m_maxLen;                                             // +0x2C  -1: no limit
};
CHK(c_VIGUIStringDataModel, sizeof(VIGUIStringDataModel) == 0x30)

// ---- viguitoggledatamodel.cpp -- 0x002BD428 .. 0x002BD5C0 ------------------
// 0x2C bytes.  No destructor of its own: the implicit one (0x002BD4D0) is
// synthesized at the end of the TU, and that order -- this class's destructor
// and __tf (0x002BD500, which CALLS the base's __tf) before the base's vtable
// is even known to be needed -- is what an explicit one would not give.
class VIGUIToggleDataModel : public VIGUIDataModel
{
public:
    VIGUIToggleDataModel();                                     // 0x002BD428
    VIGUIToggleDataModel(const char *name);                     // 0x002BD468
    void Reset();                                               // 0x002BD4A8
    void SetValue(int s);                                       // 0x002BD4B0
    int  GetValue();                                            // 0x002BD4B8
    void Toggle();                                              // 0x002BD4C0

    int m_value;                                                // +0x28
};
CHK(c_VIGUIToggleDataModel, sizeof(VIGUIToggleDataModel) == 0x2C)

// ---- viguiobject.cpp -- 0x002C10B8 .. 0x002C2610 ---------------------------
// The base of every page element.  0x98 bytes, vptr at +0x94.  __tf 0x002C25B8,
// then its three inline virtuals.  Slots 8..10 are what the page's own
// OnKeyUp/OnKeyDown/OnMessage (VIGUIPage slots 2..4) hand to the selected
// object; slot 7 is the draw, called with 1.0 by the page and the toggle (an
// alpha the base never reads).
class VIGUIObject
{
public:
    VIGUIObject(const char *name, int type, VIWndGenericRenderer *r,
                VIXMLParser *reader);                                // 0x002C10B8
    void Init();                                                // 0x002C1160
    virtual ~VIGUIObject();                                     // 0x002C1230
    void FillRect(const VIVect2I *r, const VIColor32F &c, VIAlphaBlendMode m);  // 0x002C12C0
    void SetType(int type);                                     // 0x002C13F0
    int  GetType();                                             // 0x002C13F8
    void SetRenderer(VIWndGenericRenderer *r);                  // 0x002C1400
    void SetReader(VIXMLParser *reader);                        // 0x002C1408
    void SetName(const char *name);                             // 0x002C1410
    void SetAlphaStep(float f);                                 // 0x002C1430
    const char *GetName();                                      // 0x002C1438
    void SetXTarget(short x);                                   // 0x002C1440  SelectEffect xtarget
    void SetSelectEffect(int e);                                // 0x002C1448  SelectEffect type
    void SetPos(short x, short y);                              // 0x002C1450  Position
    short GetX();                                               // 0x002C1468
    short GetY();                                               // 0x002C1470
    void SetBGColor(VIColor32F c);                              // 0x002C1478  BGColor
    void SetTileset(int t);                                     // 0x002C14C8  Tileset type
    void SetTileMode(int t);                                    // 0x002C14D0  Image tile
    void SetSize(short w, short h);                             // 0x002C14D8  Size
    void SetButtonPos(short x, short y);                        // 0x002C14E8  ButtonPos
    short GetButtonX();                                         // 0x002C14F8
    short GetButtonY();                                         // 0x002C1500
    void SetImageOffset(short x, short y);                      // 0x002C1508  ImageOffset
    short GetImageOffsetY();                                    // 0x002C1518
    short GetImageOffsetY2();                                   // 0x002C1520  (reads +0x6E too)
    void SetDepth(unsigned char d);                             // 0x002C1528  Depth index
    unsigned char GetDepth();                                   // 0x002C1530
    virtual void SetTexture(int id);                            // 0x002C1538  Image imageType
    virtual void SetFocusTexture(int id);                       // 0x002C1540
    virtual void SetCapTexture(int id);                         // 0x002C1548  Image capTexture
    virtual void SetImage(unsigned int hash);                   // 0x002C1550  Image imageName
    virtual void SetFocusImage(int id);                         // 0x002C1558
    void SetPulse(float f);                                     // 0x002C1560
    void UpdateAlpha();                                         // 0x002C1568
    void UpdateScroll();                                        // 0x002C15E0
    void SetAlphaTarget(float f);                               // 0x002C1640  SelectEffect alphatarget
    void SetAlpha(float f);                                     // 0x002C1648  Image alpha
    virtual void Draw(float alpha);                             // 0x002C1658
    VIGUIDataModel *GetModel();                                 // 0x002C19E0
    void SetSelected(int f);                                    // 0x002C19E8
    int  IsSelected();                                          // 0x002C19F0
    virtual int OnKeyUp(int key) {}
    virtual int OnKeyDown(int key) {}
    virtual int OnMessage(VIWnd *w, int msg, int p1, int p2) { return 0; }

    char        m_name[32];                                     // +0x00
    int         m_type;                                         // +0x20  1 text, 2 toggle, 3 object, 4 button, 5 combo box
    VIWndGenericRenderer *m_renderer;                           // +0x24  whose app pointer is the VIStation
    VIGUIDataModel *m_model;                                    // +0x28  deleted with the object
    VIXMLParser *m_reader;                                      // +0x2C  the XML reader that built it
    short       m_x;                                            // +0x30
    short       m_y;                                            // +0x32
    short       m_scrollTarget;                                 // +0x34  where the scroll effect is heading
    short       m_normalX;                                      // +0x36  the x the scroll effect returns to
    short       m_focusX;                                       // +0x38  the x it scrolls to when focused
    short       m_width;                                        // +0x3A  -1 unset
    short       m_height;                                       // +0x3C  -1 unset
    int         m_effect;                                       // +0x40  1 scrollleft, 2 fadealpha, 3 fade
    int         m_tileset;                                      // +0x44  -1 none, 0 Parchment
    int         m_tile;                                         // +0x48  1 horiz, 2 vert, 3 both
    unsigned char m_depth;                                      // +0x4C
    int         m_texture;                                      // +0x50  a VIUITexId
    int         m_focusTexture;                                 // +0x54
    int         m_capTexture;                                   // +0x58
    int         m_image;                                        // +0x5C  a resource hash
    int         m_focusImage;                                   // +0x60
    int         m_selected;                                     // +0x64
    short       m_buttonX;                                      // +0x68
    short       m_buttonY;                                      // +0x6A
    short       m_imageOffX;                                    // +0x6C
    short       m_imageOffY;                                    // +0x6E
    float       m_alpha;                                        // +0x70
    float       m_alphaTarget;                                  // +0x74
    float       m_normalAlpha;                                  // +0x78
    float       m_focusAlpha;                                   // +0x7C
    float       m_alphaStep;                                    // +0x80
    VIColor32F  m_bgColor;                                      // +0x84  drawn when its alpha is over 0.001
};
CHK(c_VIGUIObject, sizeof(VIGUIObject) == 0x98)

// ---- viguitext.cpp -- 0x002C73D0 .. 0x002C8B00 -----------------------------
// 0xFC bytes.  Its destructor is the implicit one, emitted with the vtable;
// __tf 0x002C8A90, then the four inline members.
class VIGUIText : public VIGUIObject
{
public:
    VIGUIText(const char *name, VIWndGenericRenderer *r, VIXMLParser *reader);  // 0x002C73D0
    VIGUIText(const char *name, int type, VIWndGenericRenderer *r,
              VIXMLParser *reader);                                  // 0x002C74C8
    VIGUIStringDataModel *GetModel();                           // 0x002C75B0
    void  SetEditLabel(const char *s);                          // 0x002C75B8  EditLabel title
    void  SetMaxChars(int n);                                   // 0x002C75D8  MaxChars size
    char *Fit(const char *s, int width);                        // 0x002C75F0  a malloc'd copy that fits
    virtual void Draw(float alpha);                             // 0x002C76E8
    virtual int  OnKeyDown(int key);                            // 0x002C7B00
    virtual int  OnKeyUp(int key);                              // 0x002C7C50
    void  SetShadow(int on);                                    // 0x002C7C58  Shadow active
    virtual int  OnMessage(VIWnd *w, int msg, int p1, int p2);  // 0x002C7C60
    void  SetTextColor(VIColor32F c);                           // 0x002C7D88  TextColor
    void  SetRollColor(VIColor32F c);                           // 0x002C7DD8  RollColor
    VIColor32F GetTextColor();                                  // 0x002C7E28
    VIColor32F GetRollColor();                                  // 0x002C7E50
    void  SetCenter(int a);                                     // 0x002C7E78  alignment center
    void  SetWrap(int on);                                      // 0x002C7E80  Wrap mode
    // Inline, emitted after the __tf (0x002C8AE0 ..).  Nothing calls the last
    // three out of line; their names are ours.
    void  SetPassword(int on) { m_password = on; }
    char *Text() { return m_name; }
    void  Refresh() {}
    int   IsToggle() { return 0; }

    VIColor32F m_textColor;                                     // +0x98
    VIColor32F m_rollColor;                                     // +0xA8  the colour while focused
    int        m_center;                                        // +0xB8  1 centre
    int        m_wrap;                                          // +0xBC
    int        m_password;                                      // +0xC0  drawn as '*'
    int        m_maxChars;                                      // +0xC4  -1: the editor's 128
    int        m_shadow;                                        // +0xC8
    VIGUIStringDataModel m_label;                               // +0xCC  the editor's title
};
CHK(c_VIGUIText, sizeof(VIGUIText) == 0xFC)

// ---- viguibutton.cpp -- 0x002BD5C0 .. 0x002BE3E8 ---------------------------
// 0x104 bytes.  __tf 0x002BE398.
class VIGUIButton : public VIGUIText
{
public:
    VIGUIButton(const char *name, VIWndGenericRenderer *r, VIXMLParser *reader);   // 0x002BD5C0
    virtual ~VIGUIButton();                                     // 0x002BD608
    void SetNextPage(const char *s);                            // 0x002BD688  nextPage
    void SetAction(const char *s);                              // 0x002BD6D8  action
    virtual int OnKeyDown(int key);                             // 0x002BD728
    virtual int OnKeyUp(int key);                               // 0x002BD7D0

    char *m_nextPage;                                           // +0xFC  strdup'd
    char *m_action;                                             // +0x100 strdup'd
};
CHK(c_VIGUIButton, sizeof(VIGUIButton) == 0x104)

// ---- viguicombobox.cpp -- 0x002BE3E8 .. 0x002BFD18 -------------------------
// 0x120 bytes.  The options are strdup'd strings in a VIList<char *> (its
// copies follow the item templates at 0x002BFA98).  __tf 0x002BFCC8.
class VIGUIComboBox : public VIGUIText
{
public:
    VIGUIComboBox(const char *name, VIWndGenericRenderer *r, VIXMLParser *reader); // 0x002BE3E8
    virtual ~VIGUIComboBox();                                   // 0x002BE458
    void AddItem(const char *s);                                // 0x002BE520
    void MoveSelection(int delta);                              // 0x002BE568
    void Select(int index);                                     // 0x002BE618  into the model
    void SyncSelection();                                       // 0x002BE6C8  from the model
    void ClearItems();                                          // 0x002BE790
    virtual int  OnKeyDown(int key);                            // 0x002BE818
    void SetPopupSize(int w, int h);                            // 0x002BE8E0  PopupSize
    virtual int  OnKeyUp(int key);                              // 0x002BE8F0
    virtual void Draw(float alpha);                             // 0x002BE8F8

    VIList<char *> m_items;                                     // +0xFC
    int  m_selected;                                            // +0x108
    int  m_top;                                                 // +0x10C first visible row
    int  m_open;                                                // +0x110 the popup is showing
    int  m_popupW;                                              // +0x114 -1: the box's own width
    int  m_popupH;                                              // +0x118
    int  m_rows;                                                // +0x11C rows that fit, set by Draw
};
CHK(c_VIGUIComboBox, sizeof(VIGUIComboBox) == 0x120)

// ---- viguitogglebutton.cpp -- 0x002C8B00 .. 0x002C98B8 ---------------------
// 0x9C bytes.  Destructor implicit, emitted with the vtable; __tf 0x002C9868.
class VIGUIToggleButton : public VIGUIObject
{
public:
    VIGUIToggleButton(const char *name, VIWndGenericRenderer *r, VIXMLParser *reader);  // 0x002C8B00
    VIGUIToggleDataModel *GetModel();                           // 0x002C8B60
    virtual void SetToggleImage(int id);                        // 0x002C8B68  ToggleImage imageType
    virtual void Draw(float alpha);                             // 0x002C8B70
    virtual int  OnKeyDown(int key);                            // 0x002C8C38

    int       m_toggleTex;                                      // +0x98 drawn over the object while on
};
CHK(c_VIGUIToggleButton, sizeof(VIGUIToggleButton) == 0x9C)

#endif
