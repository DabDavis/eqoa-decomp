// visetup.cpp (name ours) -- SETUP 0x00567838 .. 0x0056B088.  The program the
// module runs: it owns the engine objects and every window, steps the intro
// screens, and is the patcher's callback object.
#include "setup.h"

int  VIDisplayLoadingScreen(bool b);
int  VIMovieStartup(int w, int h);
int  VIMoviePlay(const char *name, int (*stop)());
void VIMovieShutdown();
extern "C" int ezNetCtlGetStatus(void *status);
extern "C" int iWakeupThread(int id);
extern "C" int sceGsSyncV(int mode);
extern "C" int GetThreadId();
extern "C" int SetAlarm(unsigned short time, void (*cb)(int, unsigned short, void *), void *arg);
extern "C" int ReleaseAlarm(int id);
int  VIDnasInit();
int  VIDnasGetStatus(sceDNAS2Status *s);
int  VIDnasAuthInstall();
int  VIDnasShutdown();

// SUPPORT's (vimemcard.cpp): one card directory entry, its name only.
struct VIMemCardDirEntry
{
    char m_name[32];
};
int  VIMemCardGetDir(VIMemCardDirEntry *out, int max);
int  VIMemCardReadFileAsync(const char *name, void *buf, int size);
int  VIMemCardWriteFileAsync(const char *name, const void *buf, int size);
int  VIMemCardUnpad();
int  VIMemCardDeleteFile(const char *name);
int  VIMemCardPadAsync(int kb);
extern "C" int sceDNAS2InstPersonalizeDataLength_nohdd(int size, void *data, int *len);
extern "C" int sceDNAS2InstPersonalizeData_nohdd(int size, int len, void *data);

// SLUS's table of the modules it loaded (12 bytes each: SUPPORT 0, SETUP 2,
// STATION 3, CLIENT 4), and whether it found patched modules on the card.
// WritePatchData compares the boot images against what it wrote, and hands
// SLUS the new STATION and CLIENT images to load instead of the disc's.
struct VIRelInfo
{
    int   m_x0;     // +0x00
    int   m_size;   // +0x04
    void *m_data;   // +0x08
};
extern VIRelInfo gRelInfo[];
extern int gPatchPresent;

int  McErrSyncSet(const VIVect2I &center, int screen);
int  McErrSyncIdle(VIPad *pad, VIKeyboard *kb);
int  VIIsMcSameSession();

// What MovieStopCallback reads: VISetup::Init points them at its own pad and
// keyboard.
static VIPad      *sPad = 0;
static VIKeyboard *sKeyboard = 0;
// OnTick's redraw throttle: the last percentage drawn and when.
static int  sTickPct = 0;
static long sTickTime;

// ezNetCtlGetStatus's record; MainLoopEnd reads only the state word.
struct VINetCtlStatus
{
    int m_x0[3];
    int m_state;    // +0x0C  0 detached, 5 the ISP connection lost
    int m_x10[76];
};
static VINetCtlStatus sNetStatus = { { 0 } };

// VIPad's +0x34: 1 once a read has found a DualShock in analog mode.  Read
// through an inline taking the pad by reference: the target loads it off
// `&m_pad` as a value, not off `this`.
static inline int PadAnalog(VIPad &pad)
{
    return pad.m_mode;
}

// 0x00567838.
VISetup::VISetup()
{
}

// 0x00567960.
VISetup::~VISetup()
{
}

// 0x00567B00.  `full` is 0 on the re-initialisations Process does after the
// language screen and after the movie: the game options are loaded once.
int VISetup::Init(int full)
{
    m_x2189C = -1;
    m_x218B4 = 0;
    printf("INIT\n");
    m_pad.Init(0, 0);
    m_keyboard.Init();
    if (m_raster.Init(0, 640, 448, 1) < 0)
        return -1;
    if (m_collide.Init() < 0)
        return -1;
    if (m_dict.Init(0) < 0)
        return -1;
    if (m_sound.Init() < 0)
        return -1;
    if (m_scene.Init(VITimer::GetTime(), m_dict, &m_raster, &m_sound, m_collide) < 0)
        return -1;
    if (full)
        VIGetGameOptions()->Load();
    if (m_ui.Init(&m_raster, &m_sound, &m_dict) < 0)
        return -1;
    VISetWindowUI(&m_ui);

    VIESFParse parse;
    VICSFFile csf;
    if (csf.Parse("data2\\ArialUI.csf", parse, m_scene) < 0)
        return -1;
    VIDisplayLoadingScreen(true);
    if (csf.Parse("data2\\ui.csf", parse, m_scene) < 0)
        return -1;
    if (VIGetFeatureFlags() & 0x400)
    {
        char name[256];
        sprintf(name, "lang%s\\setup.csf",
                VILanguage::GetName((VILanguageID)(char)VIGetGameOptions()->m_region));
        printf("Loading setup resources (%s)\n", name);
        if (csf.Parse(name, parse, m_scene) < 0)
            return -1;
    }
    else if (csf.Parse("data2\\setup.csf", parse, m_scene) < 0)
        return -1;
    char art[16384];
    int n = VIMemCardReadFile("addart.csf", art, sizeof(art));
    if (n >= 0)
        csf.Parse(art, n, parse, m_scene);
    else
        csf.Parse("data2\\addart.csf", parse, m_scene);
    m_sound.Flush();
    m_scene.SetTimeOfDay(12.0f);
    m_raster.SetWireframe(0);
    m_raster.SetClearColor(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));
    VISplashFxInit(&m_raster, &m_dict);
    m_xAF70 = 0;
    m_wm.Init(&m_pad, &m_keyboard);
    m_splash.Init(&m_scene, &m_pad);
    m_patcherWnd.Init();
    m_dnas.Init(&m_scene);
    m_legal.Init();
    m_controller.Init();
    m_eula.Init();
    m_state = 0;
    // Let the pad settle.
    for (int i = 0; i < 30; i++)
    {
        VIConsolePad pad;
        m_pad.Read(pad);
        sceGsSyncV(0);
    }
    if (!PadAnalog(m_pad))
    {
        m_wm.SetRoot(&m_controller);
        m_wm.SetFocus(&m_controller, true);
    }
    else if (VIGetFeatureFlags() & 0x400)
    {
        m_wm.SetRoot(&m_language);
        m_wm.SetFocus(&m_language, true);
    }
    else
    {
        m_wm.SetRoot(&m_legal);
        m_wm.SetFocus(&m_legal, true);
    }
    m_fader.Init(&m_raster, &m_dict, 1.0f);
    m_fader.Begin(0.0f, 0.5f);
    m_fadeIn = 1;
    m_restart = 0;
    m_ambient = -1;
    sPad = &m_pad;
    sKeyboard = &m_keyboard;
    return 0;
}

// 0x00567FF0.
void VISetup::Free()
{
    StopAmbientSoundtrack();
    m_fader.Free();
    m_eula.Free();
    m_controller.Free();
    m_legal.Free();
    m_dnas.Free();
    m_patcherWnd.Free();
    m_splash.Free();
    VISplashFxFree();
    m_ui.Close();
    m_scene.Clear();
    m_sound.Close();
    m_dict.Clear();
    m_collide.Close();
    m_raster.Close();
    m_pad.Close();
    m_keyboard.Free();
}

// 0x005680D0.  ESC, START or X stops the intro movie.
int MovieStopCallback()
{
    VIConsolePad pad;
    VIConsoleKey key;
    if (sKeyboard->Read(key) == 1 && key.m_a == 27)
        return 1;
    if (sPad->Read(pad) && (pad.m_start != 0.0f || pad.m_cross != 0.0f))
        return 1;
    return 0;
}

// 0x00568158.  One frame of the intro screens and the title, then what to do
// next: 0 stay, 3 run the patcher, 2 leave (the title faded out).  The frame
// is MainLoopBegin and MainLoopEnd written out, less the network check.
int VISetup::Process()
{
    int result = 0;
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
    m_sound.Process();
    m_scene.Render(m_camera);
    int focus = m_wm.Update();
    m_fader.Update();
    m_raster.EndScene();
    long t = VITimer::GetTime();
    while (VITimer::GetTime() - t < 18)
        ;
    if (!focus)
    {
        switch (m_state)
        {
        case 0:
            if (m_wm.GetRoot() == &m_controller)
            {
                if (VIGetFeatureFlags() & 0x400)
                {
                    m_wm.SetRoot(&m_language);
                    m_wm.SetFocus(&m_language, true);
                }
                else
                {
                    m_wm.SetRoot(&m_legal);
                    m_wm.SetFocus(&m_legal, true);
                }
            }
            else if (m_wm.GetRoot() == &m_language)
            {
                printf("language = %d\n", (char)VIGetGameOptions()->m_region);
                Free();
                Init(0);
                printf("language = %d\n", (char)VIGetGameOptions()->m_region);
                m_wm.SetRoot(&m_legal);
                m_wm.SetFocus(&m_legal, true);
            }
            else
            {
                m_state = 2;
                m_wm.SetRoot(&m_eula);
                m_wm.SetFocus(&m_eula, true);
            }
            break;
        case 2:
            m_raster.Flush();
            if (!VIMovieStartup(640, 448))
            {
                VIMoviePlay("movies\\eqoa.pss", MovieStopCallback);
                VIMovieShutdown();
            }
            Free();
            Init(0);
            m_state = 3;
            m_fader.Begin(1.0f, 0.0f);
            m_wm.SetRoot(&m_splash);
            m_wm.SetFocus(&m_splash, true);
            m_fader.Begin(0.0f, 0.5f);
            StartAmbientSoundtrack();
            break;
        default:
            switch (m_splash.Selection())
            {
            case 0:
                result = 3;
                break;
            case 1:
                if (m_fadeIn == 1)
                {
                    m_fadeIn = 0;
                    m_fader.Begin(1.0f, 0.5f);
                }
                else if (m_fader.Alpha() == 1.0f)
                    result = 2;
                break;
            }
            break;
        }
    }
    return result;
}

// 0x00568678.  The first half of a frame, for the loops that run their own
// screens (RunPatcher, the memory-card checks): returns whether a window still
// has the focus.  The unused read is what the code needs: the target forms
// the `this + 0x8000` base of the pad copy before the first call, and only a
// member access past 0x8000 at the top does that (it is deleted as dead after
// cse has shared its base).  Which member it was is not recoverable; any one
// between 0x8000 and 0xFFFF gives the same code.
int VISetup::MainLoopBegin()
{
    int unused = m_xAF70;
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
    m_sound.Process();
    m_scene.Render(m_camera);
    return m_wm.Update();
}

// 0x005688D8.  The second half: -1 when the network went away (the title is
// told which way).
int VISetup::MainLoopEnd()
{
    m_fader.Update();
    m_raster.EndScene();
    long t = VITimer::GetTime();
    while (VITimer::GetTime() - t < 18)
        ;
    ezNetCtlGetStatus(&sNetStatus);
    switch (sNetStatus.m_state)
    {
    case 0:
        m_splash.NotifyDetached();
        return -1;
    case 1:
    case 2:
    case 3:
    case 4:
        break;
    case 5:
        m_splash.NotifyIspLost();
        return -1;
    }
    return 0;
}

// 0x005689B8.
int VISetup::MainLoop()
{
    int r = MainLoopBegin();
    if (MainLoopEnd() < 0)
        return -1;
    return r;
}

int MovieStopCallback();

// 0x00568A00.  The alarm RunPatcher sleeps on.
void sAlarmCallback(int id, unsigned short time, void *thread)
{
    iWakeupThread((int)thread);
    __asm__ volatile("sync.l; ei");
}

// 0x00568A28.  The title's "Connect": DNAS authentication (retried once its
// status has not moved for a minute), then the patch.  The patch images come
// off the card, the patcher reads the manifest data\psvrpath.dat names, and
// what it changed goes back onto the card.  3 the game may go on (the screen
// faded out), 4 back to the title, 5 the card or the patch failed for good.
int VISetup::RunPatcher()
{
    sTickTime = 0;
    m_x218B0 = 0;
    m_x218B4 = 0;
    m_restart = 0;
    m_progress = 0.0f;
    m_x218AC = 0.0f;
    sceDNAS2Status status;
    int retry;
    int failed;
    int i;
    do
    {
        memset(&status, 0, sizeof(status));
        retry = 0;
        failed = 0;
        m_wm.SetRoot(&m_dnas);
        m_wm.SetFocus(&m_dnas, true);
        m_dnas.SetStatus(status);
        for (i = 0; i < 20; i++)
            MainLoop();
        VIDnasInit();
        while (VIDnasGetStatus(&status))
        {
            m_dnas.SetStatus(status);
            if (MainLoop() < 0)
            {
                VIDnasShutdown();
                return 4;
            }
        }
        VIDnasAuthInstall();
        sceDNAS2Status last;
        memset(&last, 0, sizeof(last));
        long start = VITimer::GetTime();
        while (!VIDnasGetStatus(&status))
        {
            int thread = GetThreadId();
            int alarm = SetAlarm(250, sAlarmCallback, (void *)thread);
            SleepThread();
            ReleaseAlarm(alarm);
            MainLoop();
            if (memcmp(&last, &status, sizeof(status)))
            {
                *(long long *)&last = *(long long *)&status;
                start = VITimer::GetTime();
                if (status.code == 6 && (status.sub_code == -105 || status.sub_code == -101))
                {
                    m_dnas.SetStatus(status);
                    failed = 1;
                    MainLoop();
                    MainLoop();
                    printf("server timeout\n");
                    goto done;
                }
                m_dnas.SetStatus(status);
                MainLoop();
                MainLoop();
                if (status.code == 5 || status.code == 8)
                {
                    printf("finished successfully\n");
                    goto done;
                }
                if (status.code == 6 || status.code == 7)
                {
                    failed = 1;
                    printf("error during process\n");
                    goto done;
                }
                printf("------------------\n");
                printf("code     : %d\n", status.code);
                printf("sub-code : %d\n", status.sub_code);
                printf("progress : %d\n", status.progress);
            }
            if ((int)(VITimer::GetTime() - start) > 60000)
            {
                retry = 1;
                printf("FORCED TIMEOUT!\n");
                failed = 0;
                goto done;
            }
        }
        failed = 1;
        printf("error getting status!\n");
    done:
        VIDnasShutdown();
    } while (retry);
    if (failed)
    {
        int r;
        while ((r = MainLoop()) != 0)
        {
            if (r < 0)
                return 4;
        }
        return 4;
    }
    for (i = 0; i < 60; i++)
        MainLoop();

    VIFile file;
    m_wm.SetRoot(&m_patcherWnd);
    m_wm.SetFocus(&m_patcherWnd, true);
    m_patcherWnd.SetState(-1);
    for (i = 0; i < 20; i++)
    {
        if (MainLoop() < 0)
            return 4;
    }
    m_patcherWnd.SetState(5);
    for (i = 0; i < 10; i++)
    {
        if (MainLoop() < 0)
            return 4;
    }
    VIPatchImage *images = 0;
    printf("reading patch data\n");
    if (ReadPatchData(&images) < 0)
    {
        printf("error\n");
        DeletePatchImage(images);
        images = 0;
        m_restart = 1;
        return 5;
    }
    printf("success\n");
    m_patcherWnd.SetState(0);
    for (i = 0; i < 32; i++)
    {
        if (MainLoop() < 0)
        {
            DeletePatchImage(images);
            images = 0;
            return 4;
        }
    }
    SetPatchImage(images);
    m_x218B0 = 1;
    char path[256];
    while (file.Open("data\\psvrpath.dat", 1) < 0)
    {
        printf("Error opening patch server path file (data\\psvrpath.dat)\n");
        for (i = 0; i < 30; i++)
            sceGsSyncV(0);
    }
    file.Seek(0, VIFile::Seek_End);
    int n = file.Tell();
    if (n > 255)
        n = 255;
    file.Seek(0, VIFile::Seek_Set);
    file.Read(path, n);
    file.Close();
    path[n] = 0;
    char *c;
    if ((c = strchr(path, '\r')) != 0)
        *c = 0;
    if ((c = strchr(path, '\n')) != 0)
        *c = 0;
    if ((c = strchr(path, ' ')) != 0)
        *c = 0;
    printf("Patching with XML: %s\n", path);
    int ok = Initialize("patch.everquestonlineadventures.com:7000", 7000, path, 2);
    m_x218B0 = 0;
    if (!ok)
    {
        printf("Patcher (Initialize) Failed\n");
        m_patcherWnd.SetState(2);
        m_restart = 1;
        goto end;
    }
    m_patcherWnd.SetState(1);
    for (i = 0; i < 32; i++)
    {
        if (MainLoop() < 0)
        {
            DeletePatchImage(images);
            images = 0;
            return 4;
        }
    }
    if (!DownloadAll(0))
    {
        printf("Patcher (DownloadAll) Failed\n");
        m_patcherWnd.SetState(2);
        m_restart = 1;
        goto end;
    }
    if (!DeleteOldFiles())
    {
        printf("Patcher (DeleteOldFiles) Failed\n");
        m_patcherWnd.SetState(2);
        m_restart = 1;
        goto end;
    }
    {
        PatcherChanges *changes = GetProductChanges();
        if (changes)
        {
            char *text = changes->GetTextChanges();
            if (text)
            {
                unsigned short wide[0x8000];
                VIString::str8to16(wide, sizeof(wide), text);
                m_patcherWnd.AppendMessage(wide);
            }
        }
    }
    m_patcherWnd.SetState(6);
    for (i = 0; i < 10; i++)
    {
        if (MainLoop() < 0)
        {
            DeletePatchImage(images);
            images = 0;
            return 4;
        }
    }
    {
        int w = WritePatchData(&images);
        if (w < 0)
        {
            m_patcherWnd.SetState(w == -2 ? 7 : 2);
            m_restart = 1;
            DeletePatchImage(images);
            images = 0;
            if (w == -1)
                return 5;
        }
        else
        {
            DeletePatchImage(images);
            images = 0;
            m_patcherWnd.SetProgress(1.0f);
            m_patcherWnd.SetState(4);
            if (m_x218B4)
                m_patcherWnd.ShowPatchMessage();
        }
    }
end:
    DeletePatchImage(images);
    images = 0;
    {
        int r;
        while ((r = MainLoop()) != 0)
        {
            if (r < 0)
                return 4;
        }
    }
    if (m_patcherWnd.State() == -1)
        return 4;
    m_fader.Begin(1.0f, 0.5f);
    while (m_fader.Alpha() != 1.0f)
    {
        if (MainLoop() < 0)
            return 4;
    }
    return 3;
}

// 0x005693F8.  Back to the title, with a fresh patcher window.
void VISetup::RestartConnect()
{
    m_wm.SetRoot(&m_splash);
    m_wm.SetFocus(&m_splash, true);
    m_patcherWnd.Free();
    m_patcherWnd.Init();
    m_splash.RestartConnect();
}

// 0x00569478.
int VISetup::OnFile(const char *name)
{
    printf("On File: %s\n", name);
    if (!m_x218B0)
    {
        m_patcherWnd.SetState(3);
        for (int i = 0; i < 4; i++)
            MainLoop();
    }
    return 1;
}

// 0x00569500.  The bar: the steps already done count 0.2 each and the download
// 0.6; a frame is drawn when the percentage moves or 60 ms have passed.
int VISetup::OnTick(int n)
{
    float p = m_progress * 0.2f;
    if (n)
        p += (float)n / 100.0f * 0.6f;
    if (n == 100)
        p += m_x218AC * 0.2f;
    p = VIMIN(VIMAX(0.0f, p), 1.0f);
    n = (int)(p * 100.0f);
    m_patcherWnd.SetProgress(p);
    int ms = VITimer::GetTime() - sTickTime;
    if (n != sTickPct || ms > 60)
    {
        MainLoop();
        MainLoop();
        sTickPct = n;
        sTickTime = VITimer::GetTime();
    }
    return 1;
}

// 0x00569660.
int VISetup::OnError(int code, const char *text)
{
    printf("ERROR CODE %d / '%s'\n", text);
    return 1;
}

// 0x00569688.  0 when the card is the one this session wrote, -1 when the
// player gave up on it.
int VISetup::ValidatePatchMemCard()
{
    if (VIIsMcSameSession() <= 0)
    {
        McErrSyncSet(VIVect2I(320, 224), 4);
        int r;
        do
        {
            MainLoopBegin();
            r = McErrSyncIdle(&m_pad, &m_keyboard);
            MainLoopEnd();
        } while (!r);
        if (r == 3)
            return -1;
    }
    return 0;
}

// 0x00569728.
void VISetup::DeletePatchImage(VIPatchImage *images)
{
    if (!images)
        return;
    VIPatchImage *p = images;
    printf("Deleting patch images.\n");
    while (p)
    {
        VIPatchImage *next = p->m_next;
        delete p;
        p = next;
    }
}

// 0x00569798.  Every file in the game's card folder, into memory: one
// VIPatchImage each, newest first.  A file that fails to read is retried
// twice; an .ENC (a module the console personalised) is kept under its .AUT
// name, with the size and CRC its header records and its body left zero.
int VISetup::ReadPatchData(VIPatchImage **out)
{
    VIMemCardDirEntry dir[18];
    if (ValidatePatchMemCard() < 0)
    {
        m_restart = 1;
        return -1;
    }
    int n;
    do
    {
        n = VIMemCardGetDir(dir, 18);
        if (n >= 0)
            break;
        if (ValidatePatchMemCard() < 0)
        {
            m_restart = 1;
            return -1;
        }
    } while (1);
    char *buf = (char *)memalign(128, 0x400000);
    int retries = 0;
    VIPatchImage *list = 0;
    for (int i = 0; i < n; i++)
    {
        if (VIMemCardStatus(0))
        {
            McErrSyncSet(VIVect2I(320, 224), 4);
            int r;
            do
            {
                MainLoopBegin();
                r = McErrSyncIdle(&m_pad, &m_keyboard);
                MainLoopEnd();
            } while (!r);
            if (r == 3)
            {
                free(buf);
                DeletePatchImage(list);
                return -1;
            }
        }
        printf("Trying (%s) - ", dir[i].m_name);
        int got = VIMemCardReadFileAsync(dir[i].m_name, buf, 0x400000);
        if (got == -2)
        {
            printf("Memory card file not found (%s)\n", dir[i].m_name);
            retries = 0;
            continue;
        }
        if (got == -1)
        {
            if (retries)
                printf("RETRY - ");
            printf("error on read - doing card status check\n");
            if (retries < 2)
            {
                i--;
                retries++;
            }
            else
                retries = 0;
            continue;
        }
        do
        {
            got = VIMemCardAsyncIdle();
            MainLoopBegin();
            MainLoopEnd();
            if (got < 0)
            {
                // The same retry as above, from inside the wait: the target
                // shares the two from the second printf on (cross-jumped).
                if (retries)
                    printf("RETRY - ");
                printf("error on read - doing card status check\n");
                if (retries < 2)
                {
                    i--;
                    retries++;
                }
                else
                    retries = 0;
                goto next;
            }
        } while (got <= 0);
        {
            printf("read (%s) = %d bytes\n", dir[i].m_name, got);
            retries = 0;
            VIPatchImage *image = new VIPatchImage;
            strcpy(image->m_name, dir[i].m_name);
            if (strstr(image->m_name, ".ENC"))
            {
                strcpy(strstr(image->m_name, ".ENC"), ".AUT");
                int size = *(int *)buf;
                printf("CHANGED TO (%s) - Size = %d\n", image->m_name, size);
                image->m_size = size;
                image->m_data = (char *)memalign(128, size);
                image->m_crc = ((int *)buf)[1];
                printf("Original CRC = %d\n", image->m_crc);
                memset(image->m_data, 0, size);
            }
            else
            {
                image->m_size = got;
                image->m_data = (char *)memalign(128, got);
                image->m_crc = crc32(0, (const Bytef *)buf, got);
                printf("Computed CRC = %d\n", image->m_crc);
                memcpy(image->m_data, buf, got);
            }
            image->m_next = list;
            list = image;
            m_progress = (float)(i + 1) / (float)n;
            OnTick(0);
        }
    next:
        ;
    }
    free(buf);
    *out = list;
    return 0;
}

// 0x00569B98.  Back onto the card: every dirty image written (a module
// personalised to this console first), every deleted one removed.  A patched
// SUPPORT or SETUP means a restart; the new STATION and CLIENT images go to
// SLUS in place of the disc's.  -1 when the card fails for good, -2 when DNAS
// refuses a module.  The give-up is written out at every card call (cross-jumping
// keeps one copy); a goto to one shared copy lays the loops out differently.
int VISetup::WritePatchData(VIPatchImage **images)
{
    if (ValidatePatchMemCard() < 0)
    {
        m_restart = 1;
        return -1;
    }
    int changed = 0;
    VIPatchImage *p;
    for (p = *images; p; p = p->m_next)
    {
        if (p->m_dirty || p->m_deleted)
        {
            changed = 1;
            break;
        }
    }
    if (changed)
    {
        do
        {
            if (VIMemCardUnpad() != -1)
                break;
            printf("UNPAD ERROR\n");
            if (HandleMemCardAbnormal() < 0)
            {
                DeletePatchImage(*images);
                *images = 0;
                return -1;
            }
        } while (1);
        int done = 0;
        int total = 0;
        for (p = *images; p; p = p->m_next)
            total++;
        for (p = *images; p; p = p->m_next)
        {
            if (p->m_dirty)
            {
                m_x218B4 = 1;
                printf("POST PATCH: Writing (new/modified) file (%s)\n", p->m_name);
                printf("CRC = %d\n", p->m_crc);
                if (strstr(p->m_name, ".AUT") && EncryptRel(p) < 0)
                {
                    DeletePatchImage(*images);
                    *images = 0;
                    return -2;
                }
                int r;
                do
                {
                    do
                    {
                        if (VIMemCardWriteFileAsync(p->m_name, p->m_data, p->m_size) >= 0)
                            break;
                        if (HandleMemCardAbnormal() < 0)
                        {
                            DeletePatchImage(*images);
                            *images = 0;
                            return -1;
                        }
                    } while (1);
                    do
                    {
                        MainLoopBegin();
                        MainLoopEnd();
                        r = VIMemCardAsyncIdle();
                        if (r < 0)
                        {
                            if (HandleMemCardAbnormal() < 0)
                            {
                                DeletePatchImage(*images);
                                *images = 0;
                                return -1;
                            }
                            break;
                        }
                    } while (r <= 0);
                } while (r <= 0);
            }
            if (p->m_deleted)
            {
                m_x218B4 = 1;
                if (strstr(p->m_name, ".AUT"))
                    strcpy(strstr(p->m_name, ".AUT"), ".ENC");
                printf("POST PATCH: Deleting file (%s)\n", p->m_name);
                do
                {
                    if (VIMemCardDeleteFile(p->m_name) >= 0)
                        break;
                    if (HandleMemCardAbnormal() < 0)
                    {
                        DeletePatchImage(*images);
                        *images = 0;
                        return -1;
                    }
                } while (1);
            }
            m_x218AC = (float)done++ / (float)total;
            OnTick(100);
        }
    }
    for (p = *images; p; p = p->m_next)
    {
        if (strstr(p->m_name, "SUPPORT.AUT") || strstr(p->m_name, "SETUP.AUT"))
        {
            if (!gPatchPresent && !p->m_deleted)
            {
                printf("No patches loaded, but %s patched = unconditional restart.\n", p->m_name);
                m_restart = 1;
                m_patcherWnd.SetRestartRequired(1);
            }
        }
        if (strstr(p->m_name, "SUPPORT.ENC"))
        {
            if (p->m_dirty || p->m_deleted)
            {
                m_restart = 1;
                m_patcherWnd.SetRestartRequired(1);
            }
            if (p->m_size != gRelInfo[0].m_size)
            {
                printf("SUPPORT.ENC mismatch with boot image. Restart required.\n");
                m_restart = 1;
                m_patcherWnd.SetRestartRequired(1);
            }
        }
        else if (strstr(p->m_name, "SETUP.ENC"))
        {
            if (p->m_dirty || p->m_deleted)
            {
                m_restart = 1;
                m_patcherWnd.SetRestartRequired(1);
            }
            if (p->m_size != gRelInfo[2].m_size)
            {
                printf("SETUP.ENC mismatch with boot image. Restart required.\n");
                m_restart = 1;
                m_patcherWnd.SetRestartRequired(1);
            }
        }
        else if (strstr(p->m_name, "STATION.ENC"))
        {
            gRelInfo[3].m_data = p->m_data;
            gRelInfo[3].m_x0 = p->m_size;
            gRelInfo[3].m_size = p->m_size;
            p->m_data = 0;
            p->m_size = 0;
        }
        else if (strstr(p->m_name, "CLIENT.ENC"))
        {
            gRelInfo[4].m_data = p->m_data;
            gRelInfo[4].m_x0 = p->m_size;
            gRelInfo[4].m_size = p->m_size;
            p->m_data = 0;
            p->m_size = 0;
        }
    }
    printf("Saving game options.\n");
    do
    {
        if (VIGetGameOptions()->Save() >= 0)
            break;
        if (HandleMemCardAbnormal() < 0)
        {
            DeletePatchImage(*images);
            *images = 0;
            return -1;
        }
    } while (1);
    if (changed)
    {
        printf("Repadding memory card.\n");
        int r;
        do
        {
            do
            {
                if (VIMemCardPadAsync(3000) >= 0)
                    break;
                if (HandleMemCardAbnormal() < 0)
                {
                    DeletePatchImage(*images);
                    *images = 0;
                    return -1;
                }
            } while (1);
            do
            {
                MainLoopBegin();
                MainLoopEnd();
                r = VIMemCardAsyncIdle();
                if (r < 0)
                {
                    if (HandleMemCardAbnormal() < 0)
                    {
                        DeletePatchImage(*images);
                        *images = 0;
                        return -1;
                    }
                    break;
                }
            } while (r <= 0);
        } while (r <= 0);
    }
    m_x218AC = 1.0f;
    OnTick(100);
    printf("Done.\n");
    return 0;
}

// 0x0056A148.  A module is kept on the card personalised to this console:
// the image becomes its original size and CRC, then the personalised body,
// under an .ENC name.  (The copy it personalises is never freed.)
int VISetup::EncryptRel(VIPatchImage *p)
{
    int len;
    int size = p->m_size;
    int r = sceDNAS2InstPersonalizeDataLength_nohdd(size, p->m_data, &len);
    if (r)
    {
        printf("sceDNAS2InstPersonalizeDataLength_nohdd() failed (%d)\n", r);
        return -1;
    }
    char *buf = (char *)malloc(VIMAX(len, p->m_size));
    if (!buf)
    {
        printf("EncryptRel() : malloc failed\n");
        return -1;
    }
    memcpy(buf, p->m_data, p->m_size);
    r = sceDNAS2InstPersonalizeData_nohdd(p->m_size, len, buf);
    if (r < 0)
    {
        printf("sceDNAS2InstPersonalizeData_nohdd() failed (%d)\n", r);
        return -1;
    }
    free(p->m_data);
    p->m_size = len + 8;
    p->m_data = (char *)memalign(128, len + 8);
    if (!p->m_data)
        printf("EncryptRel() : malloc failed\n");
    ((int *)p->m_data)[0] = size;
    ((int *)p->m_data)[1] = p->m_crc;
    memcpy(p->m_data + 8, buf, len);
    strcpy(strstr(p->m_name, ".AUT"), ".ENC");
    return 0;
}

// 0x0056A2A0.
int VISetup::HandleMemCardAbnormal()
{
    McErrSyncSet(VIVect2I(320, 224), 4);
    int r;
    do
    {
        MainLoopBegin();
        r = McErrSyncIdle(&m_pad, &m_keyboard);
        MainLoopEnd();
    } while (!r);
    if (r == 3)
        return -1;
    for (int i = 0; i < 4; i++)
    {
        MainLoopBegin();
        MainLoopEnd();
    }
    return 0;
}

// 0x0056A368.  The title's music, looped.
void VISetup::StartAmbientSoundtrack()
{
    VIUI &ui = m_ui;
    VIDictionary *dict = ui.m_dict;
    VIResourceType type;
    int i;
    if (dict->Find(VIHashResourceID("splash_screen"), type, i))
    {
        StopAmbientSoundtrack();
        ui.m_sound->Sound(i)->SetRepeat(1);
        m_ambient = ui.m_sound->Play(i, 0, (VISOUNDCLASS)0);
    }
}

// 0x0056A410.
void VISetup::StopAmbientSoundtrack()
{
    if (m_ambient != -1)
    {
        VIUI &ui = m_ui;
        ui.m_sound->Stop(m_ambient);
        m_ambient = -1;
    }
}
