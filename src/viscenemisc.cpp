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
