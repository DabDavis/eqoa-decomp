// VIBattleMusic -- SUPPORT 0x0049E618 .. 0x0049F668, its own TU.
//
// Combat music.  Feature bit 0x100 selects between two entirely different
// back ends: streamed .bgm files off the disc, or an XM parsed out of the
// track's own ESF and played through the sound device.
#include "vi.h"

extern "C" int rand();
extern "C" int sprintf(char *d, const char *fmt, ...);

VIBattleMusic::VIBattleMusic()
{
    m_scene = 0;
}

VIBattleMusic::~VIBattleMusic()
{
    Free();
}

int VIBattleMusic::Init(VIScene *sc)
{
    // Phase 16's hoist, per CONSTANT: the first -1 store emitted is +0x1C, so
    // m_channel is the source's LAST -1 store and closes the block.
    // Phase 16's hoist, per CONSTANT: the first -1 store emitted is +0x1C, so
    // m_channel is the source's LAST -1 store; and the rate closes the block.
    m_scene = sc;
    m_state = 0;
    m_loaded = 0;
    m_track = -1;
    m_sound = -1;
    m_channel = -1;
    m_rate = 0.01f;
    return 0;
}

void VIBattleMusic::Free()
{
    if (m_scene)
    {
        Unload();
        m_track = -1;
        m_loaded = 0;
        m_scene = 0;
    }
}

void VIBattleMusic::Stop(bool now)
{
    if (m_scene == 0)
        return;

    VISoundDevice *sd = m_scene->m_sound;

    if (sd == 0)
        return;

    if (VIGetFeatureFlags() & 0x100)
    {
        m_state = 0;
        sd->StopBgm(0);
        return;
    }

    m_stopNow = now;
    // Phase 7's if-direction: the arm that lands LAST is the source's `if`.
    if (m_channel == -1)
        m_state = 0;
    else
        m_state = 2;
}

void VIBattleMusic::StopNow()
{
    if (m_scene == 0)
        return;

    VISoundDevice *sd = m_scene->m_sound;

    if (sd == 0)
        return;

    if (VIGetFeatureFlags() & 0x100)
    {
        sd->StopBgm(0);
        m_state = 0;
        return;
    }

    if (m_channel != -1)
    {
        sd->Stop(m_channel);
        m_channel = -1;
    }
    m_state = 0;
}

// Two entirely different back ends behind one entry point.  With feature bit
// 0x100 the track is a streamed .bgm file and track 9 means "pick one of eight
// at random"; without it the track is an XM the caller must have preloaded.
int VIBattleMusic::Play(int track)
{
    char path[256];

    if (m_scene == 0)
        return 0;

    VISoundDevice *sd = m_scene->m_sound;

    if (sd == 0)
        return 0;

    if (VIGetFeatureFlags() & 0x100)
    {
        if (track == 9)
        {
            if (m_state == 0)
            {
                m_state = 3;
                m_track = rand() % 8 + 1;
            }
        }
        else if (track != -1)
        {
            m_track = track;
        }

        if (m_track == 0)
        {
            Stop(0);
            return 0;
        }

        sprintf(path, "bgm\\battle%02d.bgm", m_track);
        sd->PlayBgm(0, path);
        return 0;
    }

    m_stopNow = 0;

    if (track == -1)
    {
        // The GUARD comes first and the assignment after it: gcc then steals
        // `track = m_track` into the guard's delay slot, where assigning first
        // and re-testing costs a reload and 14 of 90 words.
        if (m_track == -1)
            return -1;
        track = m_track;
    }

    if (track != m_track)
    {
        Preload(track);
        m_track = track;
    }

    if (m_state == 0)
    {
        if (m_channel != -1)
            StopNow();
        m_loaded = 0;
    }

    if (m_state != 3)
        m_state = 1;
    return 0;
}

// The XM path: parse the track's own ESF, keep the sound index, and mark it
// repeating.  The parser is a 1072-byte local, which is why the frame is 1408.
int VIBattleMusic::Preload(int track)
{
    char path[256];

    if (VIGetFeatureFlags() & 0x100)
        return 0;
    if (m_scene == 0)
        return 0;

    VISoundDevice *sd = m_scene->m_sound;

    if (sd == 0)
        return 0;

    sprintf(path, "bgm\\battle%d.esf", track);
    Unload();

    if (track > 0)
    {
        VIESFParse parse;

        m_sound = parse.ParseSound(path, *m_scene);
        if (m_sound >= 0)
            sd->Sound(m_sound)->SetRepeat(1);
    }

    m_track = track;
    // One NESTED ternary, not two returns: the target feeds both arms into a
    // single pseudo and moves it to v0 at the bottom, which two `return`s
    // cannot produce (21 of 62 words).
    return track == 0 ? 0 : (m_sound > -1 ? 0 : -1);
}

// The XM path's per-frame fade.  State 1 starts the channel once the sound is
// cached and ramps the volume up by m_rate to 1.0 (then state 3, playing);
// state 2 ramps it down -- ten times faster for a StopNow-style stop -- and
// stops the channel at zero.
int VIBattleMusic::Process()
{
    if (VIGetFeatureFlags() & 0x100)
        return 0;
    if (m_scene == 0)
        return 0;

    VISoundDevice *sd = m_scene->m_sound;

    if (sd == 0)
        return 0;
    if (m_track == 0)
        return 0;

    switch (m_state)
    {
    case 0:
        break;

    case 1:
        if (m_channel == -1)
        {
            if (m_sound == -1)
                return -1;
            if (!sd->Sound(m_sound)->IsCached())
                return 0;
            m_channel = sd->Play(m_sound, 0, VISOUNDCLASS_MUSIC);
            sd->SetVolume(m_channel, m_loaded);
        }
        m_loaded += m_rate;
        if (m_loaded >= 1.0f)
        {
            m_loaded = 1.0f;
            m_state = 3;
        }
        sd->SetVolume(m_channel, m_loaded);
        break;

    case 2:
    {
        float v = m_rate * 0.1f;
        float l = m_loaded;

        if (m_stopNow)
            v = l - v * 10.0f;
        else
            v = l - v;
        m_loaded = v;

        if (v <= 0.0f)
        {
            StopNow();
            m_state = 0;
        }
        else
            sd->SetVolume(m_channel, v);
        break;
    }

    }
    return 0;
}

void VIBattleMusic::Unload()
{
    if (VIGetFeatureFlags() & 0x100)
        return;
    if (m_scene == 0)
        return;

    VISoundDevice *sd = m_scene->m_sound;

    if (sd == 0)
        return;

    StopNow();
    if (m_sound >= 0)
    {
        sd->ReleaseSound(m_sound, *m_scene->m_dict);
        m_sound = -1;
    }
    m_track = -1;
}
