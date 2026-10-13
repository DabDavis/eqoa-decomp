// Fragments of the VIScene translation unit that stand on their own: the
// outdoors filter (0x004707F8), the display-element comparator (0x0046A950)
// and the zone's ambient sound-track pair (0x00471150).
#include "vi.h"

// Accepts only STATIC actors.  It does not consult m_actor, so unlike the base
// it does not refuse the owner by name; the owner is not static, so it is
// rejected by a different test.
int VISceneOutdoorsFilter::FilterActor(VIScene &scene, int actor)
{
    return scene.IsActorStatic(actor) == 0;
}

// The qsort comparator behind VIScene's display list: a -1 in the second word
// is a sentinel that sorts FIRST, then the first word ascending, then the
// third -- and the last test is spelled `!=` rather than a second compare,
// which is why it comes out as xor + sltu.
int VISceneDisplayElem::Compare(const void *pa, const void *pb)
{
    const VISceneDisplayElem *a = (const VISceneDisplayElem *)pa;
    const VISceneDisplayElem *b = (const VISceneDisplayElem *)pb;

    if (a->m_b == -1)
    {
        if (b->m_b != -1)
            return -1;
    }
    else
    {
        if (b->m_b == -1)
            return 1;
    }
    if (a->m_a < b->m_a)
        return -1;
    if (b->m_a < a->m_a)
        return 1;
    if (a->m_c < b->m_c)
        return -1;
    return a->m_c != b->m_c;
}

int VISceneSoundTrackState::Init()
{
    return Clear();
}

int VISceneSoundTrackState::Clear()
{
    m_p00 = 0;
    m_p08 = 0;
    m_p10 = 0;
    m_p14 = 0;
    m_p18 = -1;
    m_p1C = -1;
    m_p20 = 0;
    m_p24 = -1;
    return 0;
}

int VISceneSoundTrack::Init()
{
    if (Clear() < 0)
        return -1;
    return SetTracks();
}

int VISceneSoundTrack::Clear()
{
    // Phase 28's rule: the hoisted constants keep their source order, and the
    // target hoists 15000 ahead of the 1.
    m_p10 = 15000;
    m_p00 = 1;
    m_p04 = 1.0f;
    m_p08 = -1;
    m_p18 = 0;
    m_p20 = -1;
    m_p24 = -1;
    m_p28 = 0;
    m_p30 = 1.0f;
    m_state[0].Clear();
    m_state[1].Clear();
    return 0;
}

// The (day, night) XM track pairs for the eleven terrain-profile sound indices:
// index 0 is silence and the rest are twenty resource-name hashes, written as
// constants.
int VISceneSoundTrack::SetTracks()
{
    m_tracks[0][0] = 0;
    m_tracks[0][1] = 0;
    m_tracks[1][0] = (int)0x9A5DBACC;
    m_tracks[1][1] = (int)0xA5C60038;
    m_tracks[2][0] = (int)0xEBF2EEC3;
    m_tracks[2][1] = (int)0x909E78E7;
    m_tracks[3][0] = (int)0x92C9A8CE;
    m_tracks[3][1] = (int)0xA0DBE44A;
    m_tracks[4][0] = (int)0xB9422F46;
    m_tracks[4][1] = (int)0x2FEB20F5;
    m_tracks[5][0] = (int)0x864A0682;
    m_tracks[5][1] = (int)0x656F64E9;
    m_tracks[6][0] = (int)0xDB5AA9FB;
    m_tracks[6][1] = (int)0x2143B5DF;
    m_tracks[7][0] = (int)0x42E6CF3A;
    m_tracks[7][1] = (int)0x72F08216;
    m_tracks[8][0] = (int)0x1ADCC066;
    m_tracks[8][1] = (int)0x68B480A2;
    m_tracks[9][0] = (int)0x9E562B5A;
    m_tracks[9][1] = (int)0xCEF71F36;
    m_tracks[10][0] = (int)0x0CE4AB35;
    m_tracks[10][1] = (int)0xFD6FEEE9;
    return 0;
}
