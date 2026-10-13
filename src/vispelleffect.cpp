// VISpellEventList / VISpellEffect / VISpellActiveEvent -- SUPPORT 0x00474BD0.
//
// The bottom of the spell-effect stack.  A VISpellEventList is a scene plus a
// flat array of 44-byte VISpellEvent records read out of SPELLFX.CSF; a
// VISpellEffect is three of those (caster, target, area) behind a VIResource
// header; and a VISpellActiveEvent is one event of a list while it is running.
#include "vi.h"

VISpellEventList::VISpellEventList()
{
    m_scene = 0;
    m_count = 0;
    m_events = 0;
}

VISpellEventList::~VISpellEventList()
{
    Destroy();
}

int VISpellEventList::Create(int n, VIScene *sc)
{
    m_scene = sc;
    if (m_events)
        Destroy();
    m_count = n;
    m_events = new VISpellEvent[n];
    return 0;
}

// The two resource kinds an event list OWNS: a CreateEmitter event holds a
// particle definition and a CreateSprite event a scene sprite, and both are
// handed back here rather than by the driver that used them.
void VISpellEventList::Destroy()
{
    if (m_events)
    {
        for (int i = 0; i < m_count; i++)
        {
            VISpellEvent *e = m_events + i;
            // Phase 16's switch: the two comparisons come out ahead of both
            // bodies, and each carries its own copy of the m_id load in the
            // delay slot.  An if/else-if chain puts the second test after the
            // first body and is 16 of 63 words wrong.
            switch (e->m_type)
            {
            case 3:
                if (e->m_id != -1)
                    m_scene->ParticleSystem()->ReleaseParticleDefinition(e->m_id, *m_scene->m_dict);
                break;
            case 9:
                if (e->m_id != -1)
                    m_scene->ReleaseSprite(e->m_id);
                break;
            }
        }
        delete [] m_events;
        m_events = 0;
        m_count = 0;
    }
    m_scene = 0;
}

VISpellEffect::VISpellEffect()
{
    m_scene = 0;
}

VISpellEffect::~VISpellEffect()
{
    Destroy();
}

int VISpellEffect::Create(VIScene *sc)
{
    m_scene = sc;
    return 0;
}

int VISpellEffect::Destroy()
{
    if (m_scene)
    {
        m_casterEvents.Destroy();
        m_targetEvents.Destroy();
        m_areaEvents.Destroy();
        m_scene = 0;
        return 0;
    }
    return -1;
}

VISpellActiveEvent::VISpellActiveEvent(VISpellEvent *ev, VIScene *sc)
{
    m_event = ev;
    m_scene = sc;
    m_emitter0 = 0;
    m_emitter1 = 0;
    m_blender = 0;
    m_p84 = -1;
    m_actor0 = -1;
    m_actor1 = -1;
    // m_v14 and m_v20 are NOT zeroed here: their own default constructors run
    // ahead of the body, straight-line, and the body zeroes only the three
    // arrays -- whose elements the ctor loops have already zeroed once.
    m_a[0].Zero();
    m_a[1].Zero();
    m_b[0].Zero();
    m_b[1].Zero();
    m_c[0].Zero();
    m_c[1].Zero();
}

VISpellActiveEvent::~VISpellActiveEvent()
{
    if (m_actor0 != -1)
        m_scene->ReleaseActor(m_actor0);
    if (m_actor1 != -1)
        m_scene->ReleaseActor(m_actor1);
    if (m_emitter0)
        delete m_emitter0;
    if (m_emitter1)
        delete m_emitter1;
    if (m_blender)
        delete m_blender;
}

int VISpellActiveEvent::ParticleCount() const
{
    int n = 0;

    if (m_emitter0)
        n = m_emitter0->ParticleCount();
    if (m_emitter1)
        n = n + m_emitter1->ParticleCount();
    return n;
}

static inline VIVect3 operator-(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x - b.x, a.y - b.y, a.z - b.z);
}

// When the last impact of this effect lands, in ms from the cast: walk the
// caster's events accumulating the delays they schedule, and every projectile
// event adds its flight time -- distance over speed -- to the target, and to
// each secondary target when the event says so.  No projectile at all means
// the impact is the end of the delays.
int VISpellEffect::GetLastImpactTime(VIScene &sc, unsigned int start, int caster,
                                     int target, int numSecondary, int *secondary,
                                     int *out)
{
    VISpellEventList *l = &m_casterEvents;
    int time = 0;
    int last = 0;
    VIMatrix44 m;
    VIVect3 src, dst;
    int pending = 0;

    if (sc.GetActorTransform(caster, m) < 0)
        return -1;
    src.x = m.m[3].x;
    src.y = m.m[3].y;
    src.z = m.m[3].z;

    for (int i = 0; i < l->m_count; i++)
    {
        VISpellEvent *e = &l->m_events[i];

        switch (e->m_type)
        {
        case 2:
        case 15:
            pending = 1;
            break;

        case 1:
            switch (e->m_p04)
            {
            case 0:
                if (pending)
                {
                    pending = 0;
                    time += 1000;
                }
                break;
            case 1:
                time += e->m_id;
                break;
            case 2:
                if (time <= (int)start)
                    time = start;
                break;
            case 3:
                if (pending)
                {
                    pending = 0;
                    time += 500;
                }
                break;
            case 4:
                break;
            }
            break;

        case 7:
        case 13:
            if (last < time)
                last = time;
            break;

        case 8:
            if (e->m_p04 & 1)
            {
                if (sc.GetActorTransform(target, m) >= 0)
                {
                    dst.x = m.m[3].x;
                    dst.y = m.m[3].y;
                    dst.z = m.m[3].z;
                    int t = (int)((dst - src).Magnitude() * (1.0f / e->m_f14) * 1000.0f) + time;
                    if (last < t)
                        last = t;
                }
            }
            if (e->m_p04 & 2)
            {
                for (int k = 0; k < numSecondary; k++)
                {
                    if (sc.GetActorTransform(secondary[k], m) >= 0)
                    {
                        dst.x = m.m[3].x;
                        dst.y = m.m[3].y;
                        dst.z = m.m[3].z;
                        int t = (int)((dst - src).Magnitude() * (1.0f / e->m_f14) * 1000.0f) + time;
                        if (last < t)
                            last = t;
                    }
                }
            }
            break;
        }
    }

    if (last == 0)
        last = time;
    *out = last;
    return 0;
}
