// vistation.cpp (name ours) -- STATION 0x002B7978 .. 0x002B9240.  The program
// the module runs: the engine objects, the window manager and the GUI-page
// renderer, a frame at a time until the renderer has a login.  SETUP's VISetup
// is the same shape (visetup.cpp).  The tail holds this TU's VIMap<int, int> /
// VIArray<VIItemStat> copies and the out-of-line VICamera ctor gcc synthesizes
// for the m_camera member (SETUP's visetup.cpp has the same one).
#include "station.h"

int VIDisplayLoadingScreen(bool b);
extern "C" unsigned int VIHashResourceID(const char *s);

// 0x002B7978.
VIStation::VIStation()
{
    printf("VIStation::VIStation\n");
}

// 0x002B7A30.
VIStation::~VIStation()
{
}

// 0x002B7B40.
int VIStation::Init(char *login, char *password)
{
    m_xB40C = 0;
    printf("station::init()\n");
    sceGsSyncV(0);
    sceGsSyncV(0);
    strcpy(m_login, login);
    strcpy(m_password, password);
    printf("%s, %s\n", m_login, m_password);
    sceGsSyncV(0);
    sceGsSyncV(0);
    m_pad.Init(0, 0);
    m_keyboard.Init();
    m_running = 1;
    printf("Pad/Kb init\n");
    sceGsSyncV(0);
    sceGsSyncV(0);
    if (m_raster.Init(0, 640, 448, 0) < 0)
    {
        printf("raster init failed\n");
        return -1;
    }
    printf("Raster Init\n");
    sceGsSyncV(0);
    sceGsSyncV(0);
    VIDisplayLoadingScreen(false);
    printf("Loading Screen\n");
    sceGsSyncV(0);
    sceGsSyncV(0);
    if (m_collide.Init() < 0)
        return -1;
    if (m_dict.Init(0) < 0)
        return -1;
    if (m_sound.Init() < 0)
        return -1;
    if (m_scene.Init(VITimer::GetTime(), m_dict, &m_raster, &m_sound, m_collide) < 0)
        return -1;
    if (m_ui.Init(&m_raster, &m_sound, &m_dict) < 0)
        return -1;
    VISetWindowUI(&m_ui);
    printf("Here, ui = %i\n", &m_ui);

    VIESFParse parse;
    VICSFFile csf;
    if (csf.Parse("data2\\ArialUI.csf", parse, m_scene) < 0)
        return -1;
    if (csf.Parse("data2\\ui.csf", parse, m_scene) < 0)
        return -1;
    if (VIGetFeatureFlags() & 0x400)
    {
        char name[256];
        sprintf(name, "lang%s\\statui.csf",
                VILanguage::GetName((VILanguageID)(char)VIGetGameOptions()->m_region));
        if (csf.Parse(name, parse, m_scene) < 0)
            return -1;
    }
    else if (csf.Parse("data2\\statui.csf", parse, m_scene) < 0)
        return -1;
    if (csf.Parse("data2\\pregame.csf", parse, m_scene) < 0)
        return -1;
    m_scene.SetTimeOfDay(12.0f);
    m_raster.SetWireframe(0);
    m_raster.SetClearColor(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));
    m_xB384 = 0;
    printf("setting class volume %f, %f\n", VIGetGameOptions()->m_musicVolume,
           VIGetGameOptions()->m_effectVolume);
    m_sound.SetSoundClassVolume((VISOUNDCLASS)0, VIGetGameOptions()->m_musicVolume);
    m_sound.SetSoundClassVolume((VISOUNDCLASS)1, VIGetGameOptions()->m_effectVolume);
    m_wm.Init(&m_pad, &m_keyboard);
    m_renderer.SetAppPtr(this);
    m_renderer.SetLogin(login);
    m_renderer.SetPassword(password);
    m_renderer.Start();
    m_wm.SetRoot(&m_renderer);
    m_wm.SetFocus(&m_renderer, true);
    m_fader.Init(&m_raster, &m_dict, 1.0f);
    m_fader.Begin(0.0f, 0.5f);
    return 0;
}

// 0x002B7FC0.
void VIStation::Close()
{
    m_ui.Close();
    m_scene.Clear();
    m_sound.Close();
    m_dict.Clear();
    m_collide.Close();
    m_fader.Free();
    m_raster.Close();
    m_wm.Free();
    m_keyboard.Free();
}

// 0x002B8038.  One frame.  Once FadeOut has run and the fade is full, the
// renderer's login and password are the result, with the renderer's code
// (2: bring the network down).
int VIStation::Process()
{
    int running = 1;
    int result = 0;
    m_sound.Process();
    VIVect3 pos;
    VIVect3 dir;
    pos.Set(-pos.x, -pos.y, -pos.z);
    m_camera.Init(m_raster, dir, pos, 55.0f, 10.0f / 7.0f, 0.1f, 1000.0f);
    m_raster.BeginScene();
    m_scene.SetTime(VITimer::GetTime());
    pos = VIVect3(0.0f, 0.0f, 1.0f);
    dir = VIVect3(0.0f, 0.0f, -1.0f);
    m_camera(pos, dir);
    m_padPrev = m_padCur;
    m_pad.Read(m_padCur);
    m_scene.Render(m_camera);
    m_wm.Update();
    m_fader.Update();
    m_raster.EndScene();
    if (!m_running && m_fader.Alpha() == 1.0f)
    {
        m_xB40C = 0;
        m_wm.SetFocus(0, true);
        running = 0;
    }
    if (!running)
    {
        strcpy(m_login, m_renderer.GetLogin());
        strcpy(m_password, m_renderer.GetPassword());
        result = m_renderer.GetResult();
    }
    return result;
}

// 0x002B8328.
void VIStation::FadeOut()
{
    m_running = 0;
    m_fader.Begin(1.0f, 0.5f);
}

// 0x002B8368.  A looping track at `volume`; its handle, or 0 when the name is
// not in the dictionary.
int VIStation::PlayMusic(const char *name, float volume)
{
    VIResourceType type;
    int id;
    if (m_dict.Find(VIHashResourceID(name), type, id))
    {
        VIXm *xm = m_sound.Sound(id)->AsXm();
        if (xm)
            xm->SetRepeat(1);
        int handle = m_sound.Play(id, 127, (VISOUNDCLASS)0);
        if (handle >= 0)
            m_sound.SetVolume(handle, volume);
        return handle;
    }
    return 0;
}

// 0x002B8420.
int VIStation::IsPlaying(int handle)
{
    return m_sound.IsPlaying(handle) != 0;
}

// 0x002B8440.
void VIStation::Message(const char *text)
{
    VIGUIDialog *d = m_renderer.GetDialog();
    if (d)
    {
        d->Open("Message", 0);
        d->SetText(text);
    }
}

// 0x002B8498.
void VIStation::Message(const unsigned short *text)
{
    VIGUIDialog *d = m_renderer.GetDialog();
    if (d)
    {
        d->Open("Message", 0);
        d->SetText(text);
    }
}

// 0x002B84F0 .. 0x002B9060: this TU's VIMap<int, int> / VIArray<VIItemStat>
// copies (main.cpp writes the live one), then VICamera::VICamera (0x002B90B0),
// which gcc emits here out of line.
