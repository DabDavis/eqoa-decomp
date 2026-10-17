#line 1 "C:/code-frontiers-gm/game/VIWndCharCreateBlank.cpp"
// viwndcharcreateblank.cpp -- CLIENT 0x00C22C18 .. 0x00C240F0: the character-creation
// form (VIClient+0x2BB88) and the five pages it walks through.  The pages are cl-wc's
// (race, misc, name, review) and this lane's (class); see cl_wb.h.
#include "game.h"

// 0x00C22C18.
VIWndCharCreateBlank::VIWndCharCreateBlank()
{
    m_race = 0;
    m_class = 0;
    m_misc = 0;
    m_name = 0;
    m_review = 0;
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    Reset();
}

// 0x00C22C78.
VIWndCharCreateBlank::~VIWndCharCreateBlank()
{
    Free();
}

// 0x00C22CC0.  Each page is made hidden, handed the client and initialised, then
// becomes a child; the first page that fails stops it with its code.
int VIWndCharCreateBlank::Init(VIScene &scene, VICamera &camera, VIWalkCamera &walkCam)
{
    m_camera = &camera;
    m_walkCam = &walkCam;
    m_scene = &scene;
    m_char.m_p18 = 1;
    int r;
    m_race = new VIWndCharCreateRace;
    if (!m_race)
        r = -1;
    else
    {
        m_race->SetVisible(false);
        m_race->SetAppPtr(GetAppPtr());
        if ((r = m_race->Init(scene, camera, walkCam)) == 0)
        {
            AddChild(m_race);
            m_class = new VIWndCharCreateClass;
            if (!m_class)
                r = -1;
            else
            {
                m_class->SetVisible(false);
                m_class->SetAppPtr(GetAppPtr());
                if ((r = m_class->Init(scene, camera, walkCam)) == 0)
                {
                    AddChild(m_class);
                    m_misc = new VIWndCharCreateMisc;
                    if (!m_misc)
                        r = -1;
                    else
                    {
                        m_misc->SetVisible(false);
                        m_misc->SetAppPtr(GetAppPtr());
                        if ((r = m_misc->Init(scene, camera, walkCam)) == 0)
                        {
                            AddChild(m_misc);
                            m_name = new VIWndCharCreateName;
                            if (!m_name)
                                r = -1;
                            else
                            {
                                m_name->SetVisible(false);
                                m_name->SetAppPtr(GetAppPtr());
                                if ((r = m_name->Init()) == 0)
                                {
                                    AddChild(m_name);
                                    m_review = new VIWndCharCreateReview;
                                    if (!m_review)
                                        r = -1;
                                    else
                                    {
                                        m_review->SetVisible(false);
                                        m_review->SetAppPtr(GetAppPtr());
                                        if ((r = m_review->Init(scene, camera, walkCam)) == 0)
                                            AddChild(m_review);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return r;
}

// 0x00C22EF0.
int VIWndCharCreateBlank::Free()
{
    if (m_race)
    {
        RemoveChild(m_race);
        delete m_race;
        m_race = 0;
    }
    if (m_class)
    {
        RemoveChild(m_class);
        delete m_class;
        m_class = 0;
    }
    if (m_misc)
    {
        RemoveChild(m_misc);
        delete m_misc;
        m_misc = 0;
    }
    if (m_name)
    {
        RemoveChild(m_name);
        delete m_name;
        m_name = 0;
    }
    if (m_review)
    {
        RemoveChild(m_review);
        delete m_review;
        m_review = 0;
    }
    m_camera = 0;
    m_walkCam = 0;
    m_scene = 0;
    return 0;
}

// 0x00C23038.
void VIWndCharCreateBlank::OnDrawSelf()
{
}

// 0x00C23040.
void VIWndCharCreateBlank::OnKeyDown(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIWnd::OnKeyDown(key);
}

// 0x00C23080.
void VIWndCharCreateBlank::OnKeyUp(int key)
{
    VIClient *app = (VIClient *)GetAppPtr();
}

// 0x00C230A0.  The pages report here; p1 carries their part of the form.
void VIWndCharCreateBlank::OnMessage(VIWnd *w, int msg, int p1, int p2)
{
    VIClient *app = (VIClient *)GetAppPtr();
    switch (msg)
    {
    case 260:   // a page went back
        w->SetVisible(false);
        if (w == m_race)
            app->BackToLogin();
        else if (w == m_class)
        {
            m_race->SetVisible(true);
            app->SetActiveWindow(m_race);
        }
        else if (w == m_misc)
        {
            m_name->SetVisible(true);
            m_name->Activate();
        }
        else if (w == m_name)
        {
            m_class->SetVisible(true);
            app->SetActiveWindow(m_class);
        }
        else if (w == m_review)
        {
            m_misc->Activate();
            m_misc->SetVisible(true);
            app->SetActiveWindow(m_misc);
        }
        break;
    case 262:   // the race page: race, sex and city
        m_char.m_race = ((VINewChar *)p1)->m_race;
        m_char.m_sex = ((VINewChar *)p1)->m_sex;
        m_char.m_p38 = ((VINewChar *)p1)->m_p38;
        m_race->SetVisible(false);
        m_class->SetRace(((VINewChar *)p1)->m_race, ((VINewChar *)p1)->m_p38);
        m_class->SetVisible(true);
        app->SetActiveWindow(m_class);
        break;
    case 263:   // the class
        m_char.m_class = p1;
        m_misc->m_char = &m_char;
        break;
    case 264:   // the appearance
        m_char.m_face = ((VINewChar *)p1)->m_face;
        m_char.m_hairStyle = ((VINewChar *)p1)->m_hairStyle;
        m_char.m_hairColor = ((VINewChar *)p1)->m_hairColor;
        m_char.m_hairLength = ((VINewChar *)p1)->m_hairLength;
        m_misc->SetVisible(false);
        m_review->SetVisible(true);
        m_review->m_char = &m_char;
        app->SetActiveWindow(m_review);
        break;
    case 265:   // the stats
        memcpy(m_char.m_stats, ((VINewChar *)p1)->m_stats, sizeof(m_char.m_stats));
        m_class->SetVisible(false);
        m_name->SetVisible(true);
        m_name->m_char = &m_char;
        m_name->Activate();
        break;
    case 266:   // the name
        VIString::strcpy8(m_char.m_name, 24, (const char *)p1);
        m_name->SetVisible(false);
        m_misc->SetVisible(true);
        m_misc->Activate();
        app->SetActiveWindow(m_misc);
        break;
    case 267:   // create
        m_review->SetVisible(false);
        app->CreateCharacter(&m_char);
        break;
    default:
        VIWnd::OnMessage(w, msg, p1, p2);
        break;
    }
}

// 0x00C23360.
VIWnd *VIWndCharCreateBlank::Focus()
{
    m_review->SetVisible(false);
    m_name->SetVisible(false);
    m_misc->SetVisible(false);
    m_class->SetVisible(false);
    m_race->SetVisible(true);
    return m_race;
}

// 0x00C23398.
void VIWndCharCreateBlank::Reset()
{
    m_char.m_name[0] = 0;
    m_char.m_p38 = 0;
    m_char.m_p18 = 1;
    m_char.m_race = -1;
    m_char.m_class = -1;
    m_char.m_sex = -1;
    m_char.m_face = -1;
    m_char.m_hairStyle = -1;
    m_char.m_hairColor = -1;
    m_char.m_hairLength = -1;
    memset(m_char.m_stats, 0, sizeof(m_char.m_stats));
    if (m_race)
        m_race->Reset();
    if (m_class)
        m_class->Reset();
    if (m_name)
        m_name->Reset();
    if (m_misc)
        m_misc->Reset();
    if (m_review)
        m_review->Reset();
}

// 0x00C23460.
void VIWndCharCreateBlank::SetName(const char *name)
{
    m_name->m_edit->SetText(name);
}
