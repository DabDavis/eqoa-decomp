// cl_wc.h -- windows: VIWndCharCreateMisc, VIWndCharCreateName, VIWndCharCreateRace, VIWndCharCreateReview, VIWndCharCustFrontiers, VIWndChat.
// CLIENT 0x00C26E50..0x00C338F8.
// TUs (one window class each, ending with its __tf; the inline members emitted AFTER a
// __tf are that TU's, and VIWndChat's tail runs to 0x00C33928):
//   VIWndCharCreateMisc        0x00C26E50..0x00C29968
//   VIWndCharCreateName        0x00C29968..0x00C2B058
//   VIWndCharCreateRace        0x00C2B058..0x00C2D8D8
//   VIWndCharCreateReview      0x00C2D8D8..0x00C2F5E8
//   VIWndCharCustFrontiers     0x00C2F5E8..0x00C31F00
//   VIWndChat                  0x00C31F00..0x00C33928
// The four character-creation pages are children of VIWndCharCreateBlank (cl_wb.h), which
// news them, hands each the VINewChar form (+0x24), and moves between them on 260 (back)
// and 262/263/264/265/266/267.  Method names are ours.
#ifndef CL_WC_H
#define CL_WC_H
#ifndef CHK
#define CHK(n, c)  typedef char n[(c) ? 1 : -1];
#endif

// The walk camera's orbit limits in the character scenes.  VIWndCharCreateMisc::Activate and
// VIWndCharCustFrontiers::SetChar both set the same three with every constant loaded before
// the first store and the camera loaded once: an inline taking them as arguments.
static inline void SetSceneCameraLimits(VIWalkCamera *cam, float minDist, float maxDist, float collide)
{
    cam->m_minOrbitDistance = minDist;
    cam->m_maxOrbitDistance = maxDist;
    cam->m_minCollisionDist = collide;
}

// ---- From other TUs.

void CapitalizeName(char *name);   // vicharacter.cpp: strupr the name, strlwr all but its first letter
int *GetSexWord(int sex);   // 0x00C053F8 (vifont.cpp): 163 Male / 164 Female

// A race's default appearance is game.h's VIRaceInfo (GetRaceInfo, 0x00BFC0A8), 36 bytes:
// a row in one of two tables (expansion bit 2 picks): +0x04 the male face, hair style, hair
// length, hair colour, +0x14 the female's.

// The chat text VIWndChat embeds at +0x1A8 is VIChatText (cl_parts2.h).

// One row of the race page's list (the static table sRaces, 0x00CE3E98, eleven rows in display
// order): a race, its city (1 and 2 split the humans), and two WORDS.TXT ids.  The name stays the
// placeholder client_viarray.cpp's VIArray copies were matched under.
struct VIRec16_C2D7C0
{
    int m_race;        // +0x00
    int m_city;        // +0x04: 0 = the race's own
    int m_cityWord;    // +0x08: -1, or the human city's name
    int m_descWord;    // +0x0C: the description
};

// VIWndCharCreateMisc (VIWndCharCreateMisc.cpp, 0x00C26E50 ..): the appearance page, 480 bytes
// (`new`), vtable 0x00CE3810.  Four rows -- face, hair style, hair length, hair colour -- on a
// live model of the form's race and sex; Enter hands the parent 264 with a VINewChar holding the
// four, back is 260.  VIWndCharCreateBlank keeps it at +0x84.
class VIWndCharCreateMisc : public VIWnd
{
public:
    VIWndCharCreateMisc();                                       // 0x00C26E50
    virtual ~VIWndCharCreateMisc();                              // 0x00C26ED0
    int  Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam);  // 0x00C26F28
    int  Free();                                                 // 0x00C26F68
    int  Reset();                                                // 0x00C26FB0 (Blank::Reset)
    virtual void OnDrawSelf();                                   // 0x00C27028
    virtual void OnKeyDown(int key);                             // 0x00C280D8
    virtual void OnKeyUp(int key);                               // 0x00C282F0
    void MoveSel(int d);                                         // 0x00C28330: the row
    void Activate();                                             // 0x00C28408 (Blank: back to this page)
    void Adjust(int d);                                          // 0x00C284B0: the row's value
    void AdjustFace(int d);                                      // 0x00C28580
    void AdjustHairStyle(int d);                                 // 0x00C28610
    void AdjustHairLength(int d);                                // 0x00C286A8
    void AdjustHairColor(int d);                                 // 0x00C28740
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C287D8
    void CreateModel(unsigned int model);                        // 0x00C28A68
    void UpdateCamera();                                         // 0x00C28BC8

    // Inline: its copy follows __tf19VIWndCharCreateMisc (0x00C29960).
    void SetChar(VINewChar *c) { m_char = c; }

    VINewChar    *m_char;        // +0x24  the parent's form
    VICamera     *m_camera;      // +0x28
    VIWalkCamera *m_walkCam;     // +0x2C
    VIScene      *m_scene;       // +0x30
    int           m_sel;         // +0x34  0 face, 1 hair style, 2 hair length, 3 hair colour
    int           m_actor;       // +0x38  the model, -1 none
    int           m_face;        // +0x3C  0..3
    int           m_hairStyle;   // +0x40  0..3
    int           m_hairLength;  // +0x44  0..3
    int           m_hairColor;   // +0x48  0..7
    int           m_pulse;       // +0x4C  set on every change
    long          m_time;        // +0x50  the client clock at the last change
    float         m_glow;        // +0x58  1.0 on every change
    VIUIWindow    m_window;      // +0x60
};
CHK(c_VIWndCharCreateMisc, sizeof(VIWndCharCreateMisc) == 480)

// VIWndCharCreateName (VIWndCharCreateName.cpp, 0x00C29968 .. 0x00C2B058): the name step of
// character creation, 72 bytes (`new`), vtable 0x00CE3B08.  An edit box for the name over a
// legend bar of three pad buttons; key 135 asks the server for a random name (0x0012 with
// the form's race and sex), the edit box's Done (259) validates the name -- four letters or
// more, A-Z only, else a message box with word 875 or 876 -- and hands it to the parent as
// 266 (p1 = the name).  VIWndCharCreateBlank keeps one at +0x88.  Method names ours.
class VIWndCharCreateName : public VIWnd
{
public:
    VIWndCharCreateName();                                       // 0x00C29968
    virtual ~VIWndCharCreateName();                              // 0x00C299B0
    int  Init();                                                 // 0x00C299F8: news the edit box and the message box
    int  Free();                                                 // 0x00C29B10
    int  Reset();                                                // 0x00C29BA8 (VIWndCharCreateBlank::ShowList)
    virtual void OnDrawSelf();                                   // 0x00C29BE0
    virtual void OnKeyDown(int key);                             // 0x00C2A000
    virtual void OnKeyUp(int key);                               // 0x00C2A098
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C2A0E8
    int  Validate(char *name);                                   // 0x00C2A288: -1 rejected
    void Activate();                                             // 0x00C2A340 (VIWndCharCreateBlank's OnMessage)

    // Inline: their copies follow __tf19VIWndCharCreateName (0x00C2B030, 0x00C2B038).
    void SetChar(VINewChar *c) { m_char = c; }
    inline void SetText(const char *s);                          // defined in the TU: VIWndEdit is later

    VINewChar *m_char;       // +0x24  the parent's form (race +0x1C, sex +0x24 go with 0x0012)
    char       m_name[24];   // +0x28
    VIWndEdit *m_edit;       // +0x40
    VIWndMBox *m_mbox;       // +0x44
};
CHK(c_VIWndCharCreateName, sizeof(VIWndCharCreateName) == 72)


// VIWndCharCreateRace (VIWndCharCreateRace.cpp, 0x00C2B058 ..): the race page, 608 bytes
// (`new`), vtable 0x00CE3E40.  It DERIVES from VIWndCharCreateBlank (its __tf calls the form's).
// A list of the races the owned expansions allow (BuildList, from sRaces), the sex, and a live
// model; Enter hands the parent 262 with a VINewChar holding the race, sex and city.
class VIWndCharCreateRace : public VIWndCharCreateBlank
{
public:
    VIWndCharCreateRace();                                       // 0x00C2B058
    virtual ~VIWndCharCreateRace();                              // 0x00C2B0D8
    int  Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam);  // 0x00C2B138
    int  Free();                                                 // 0x00C2B198
    int  Reset();                                                // 0x00C2B200 (Blank::Reset)
    virtual void OnDrawSelf();                                   // 0x00C2B270
    virtual void OnKeyDown(int key);                             // 0x00C2BF38
    virtual void OnKeyUp(int key);                               // 0x00C2C150
    void MoveSel(int d);                                         // 0x00C2C190
    void SetSex(int d);                                          // 0x00C2C288
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C2C348
    void CreateModel(unsigned int model);                        // 0x00C2C4F0
    void UpdateCamera();                                         // 0x00C2C8D8
    void BuildList();                                            // 0x00C2C9D8

    VICamera     *m_camera;      // +0x09C
    VIWalkCamera *m_walkCam;     // +0x0A0
    VIScene      *m_scene;       // +0x0A4
    int           m_sel;         // +0x0A8  into m_list
    int           m_actor;       // +0x0AC  the model, -1 none
    int           m_sex;         // +0x0B0  0..1
    float         m_glow;        // +0x0B4
    int           m_pulse;       // +0x0B8
    long          m_time;        // +0x0C0
    VIUIWindow    m_window;      // +0x0C8  (0x180 bytes: its long makes it 8-aligned)
    int           m_music;       // +0x248  0: the next model skips the race's music once
    VIArray<VIRec16_C2D7C0> m_list;  // +0x24C
    int           m_count;       // +0x254  rows in m_list
    int           m_top;         // +0x258  the first row shown (ten at a time)
};
CHK(c_VIWndCharCreateRace, sizeof(VIWndCharCreateRace) == 608)

// VIWndCharCreateReview (VIWndCharCreateReview.cpp, 0x00C2D8D8 ..): the last page, 72 bytes
// (`new`), vtable 0x00CE4208.  The form's race, class, stats and city over the model; Enter
// hands the parent 267 (create), back is 260.
class VIWndCharCreateReview : public VIWnd
{
public:
    VIWndCharCreateReview();                                     // 0x00C2D8D8
    virtual ~VIWndCharCreateReview();                            // 0x00C2D930
    int  Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam);  // 0x00C2D978
    int  Free();                                                 // 0x00C2D990
    int  Reset();                                                // 0x00C2D9C0 (Blank::Reset)
    virtual void OnDrawSelf();                                   // 0x00C2DA18
    virtual void OnKeyDown(int key);                             // 0x00C2E5E8
    virtual void OnKeyUp(int key);                               // 0x00C2E6F8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C2E738
    void CreateModel(unsigned int model);                        // 0x00C2E7A8
    void UpdateCamera();                                         // 0x00C2E8C0

    // Inline: its copy follows __tf21VIWndCharCreateReview (0x00C2F5E0).
    void SetChar(VINewChar *c) { m_char = c; }

    VINewChar    *m_char;        // +0x24
    VICamera     *m_camera;      // +0x28
    VIWalkCamera *m_walkCam;     // +0x2C
    VIScene      *m_scene;       // +0x30
    int           m_actor;       // +0x34
    long          m_idle;        // +0x38  counts down to the model's next idle animation
    long          m_time;        // +0x40  the clock at the last frame, -1 before the first
};
CHK(c_VIWndCharCreateReview, sizeof(VIWndCharCreateReview) == 72)

// VIWndCharCustFrontiers (VIWndCharCustFrontiers.cpp, 0x00C2F5E8 ..): re-styling an existing
// character (character select's option), 496 bytes (`new`), vtable 0x00CE45E8.  The four rows
// of VIWndCharCreateMisc on the character's own model; Enter sends SetCharLook.  No user
// destructor: the synthesized one is 0x00C31E68.
class VIWndCharCustFrontiers : public VIWnd
{
public:
    VIWndCharCustFrontiers();                                    // 0x00C2F5E8
    int  Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam);  // 0x00C2F668
    int  SetChar(int i, const VIVect3 &pos, const VIVect3 &hpr); // 0x00C2F6C8 (VIWndCharacterSelect::Option)
    virtual void OnDrawSelf();                                   // 0x00C2F7B8
    virtual void OnKeyDown(int key);                             // 0x00C2FA28
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C2FB78
    void UpdateCamera();                                         // 0x00C2FC50
    void Down();                                                 // 0x00C2FCF0
    void Up();                                                   // 0x00C2FD90
    void Prev();                                                 // 0x00C2FE28
    void Next();                                                 // 0x00C2FF78
    void Accept();                                               // 0x00C300C8
    void CreateModel();                                          // 0x00C30138
    void Release();                                              // 0x00C30410
    void DrawRows();                                             // 0x00C30458
    void DrawValues();                                           // 0x00C30A78

    VICamera     *m_camera;      // +0x024
    VIWalkCamera *m_walkCam;     // +0x028
    VIScene      *m_scene;       // +0x02C
    VIClient     *m_app;         // +0x030
    VIUIWindow    m_window;      // +0x038
    int           m_sel;         // +0x1B8  0 face, 1 hair style, 2 hair length, 3 hair colour
    unsigned char m_p1BC;        // +0x1BC
    unsigned char m_p1BD;        // +0x1BD  15 on opening
    int           m_actor;       // +0x1C0
    int           m_charIndex;   // +0x1C4  the character slot, -1 none
    VIVect3       m_pos;         // +0x1C8
    VIVect3       m_hpr;         // +0x1D4
    int           m_face;        // +0x1E0
    int           m_hairStyle;   // +0x1E4
    int           m_hairLength;  // +0x1E8
    int           m_hairColor;   // +0x1EC
};
CHK(c_VIWndCharCustFrontiers, sizeof(VIWndCharCustFrontiers) == 496)

// VIWndChat (VIWndChat.cpp, 0x00C31F00 ..): the chat box, 5,592 bytes (`new` in
// VIWndGame's Init), vtable 0x00CE48E8.  A VIUIWindow over the text ring; mode 0 shows the
// last lines (up to five), a non-zero mode a fixed 85-pixel box; up/down scroll with
// auto-repeat.  VIWndGame's AddText/AddWord/LootOption feed it.
class VIWndChat : public VIWnd
{
public:
    VIWndChat();                                                 // 0x00C31F00
    virtual ~VIWndChat();                                        // 0x00C31F68
    int  Init();                                                 // 0x00C31FC8
    int  Free();                                                 // 0x00C32078
    void Reset();                                                // 0x00C320C8 (VIWndGame)
    virtual void OnDrawSelf();                                   // 0x00C32108
    virtual void OnKeyDown(int key);                             // 0x00C32620
    virtual void OnKeyUp(int key);                               // 0x00C326F0
    void AddText(const char *s);                                 // 0x00C32748 (VIWndGame::AddText)
    void AddText(const unsigned short *s);                       // 0x00C327A0
    void AddText(const unsigned short *s, VIColor32F c);         // 0x00C327F8
    void AddWord(unsigned short id);                             // 0x00C32868 (VIWndGame::LootOption)
    void AddWord(unsigned short id, VIColor32F c);               // 0x00C328B8 (VIWndGame::AddWord)
    void SetMode(int mode);                                      // 0x00C32928 (VIWndGame::SetMode)
    int  CountLines();                                           // 0x00C329D8: 0..5
    void SetScroll(int on);                                      // 0x00C32AA0
    void ScrollDown();                                           // 0x00C32B50
    void ScrollUp();                                             // 0x00C32BD8
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);   // 0x00C32C60

    // Inline: their copies follow __tf9VIWndChat (0x00C338F8, 0x00C33900).
    int         GetMode() { return m_mode; }
    VIChatText *GetText() { return &m_text; }

    int           m_mode;        // +0x0024
    VIUIWindow    m_window;      // +0x0028
    VIChatText    m_text;        // +0x01A8
    int           m_lines;       // +0x15B8  shown, 0..5
    int           m_height;      // +0x15BC  the box's height, sliding to m_target
    int           m_target;      // +0x15C0
    int           m_scroll;      // +0x15C4  1 while scrolling
    int           m_top;         // +0x15C8  the scrolled view's first line
    int           m_bottom;      // +0x15CC  ... and one past its last
    int           m_heldKey;     // +0x15D0  the scroll key held down
    float         m_repeat;      // +0x15D4  seconds to its next repeat
};
CHK(c_VIWndChat, sizeof(VIWndChat) == 5592)

#endif
