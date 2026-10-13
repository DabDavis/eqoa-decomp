// VIParticleEmitter -- SUPPORT 0x00456910 .. 0x00458480.
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);
extern "C" void sceVu0CopyMatrix(VIMatrix44 &d, const VIMatrix44 &s);

extern "C" float fabsf(float);

// The cross product, returned BY VALUE: `up = Cross(v, a)` builds its result in
// a stack temporary and copies it out through VIVect3::operator=, which is what
// leaves the write-only 12 bytes at sp+48 -- the temporary's address is taken by
// the reference operator= takes, so its dead stores survive.
static inline VIVect3 Cross(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.y * b.z - a.z * b.y,
                   -a.x * b.z + a.z * b.x,
                   a.x * b.y - a.y * b.x);
}

VIParticleEmitter::VIParticleEmitter()
{
    m_system = 0;
    m_definition = 0;
    m_numParticles = 0;
    m_unk4C = 0;
    m_direction.Identity();
    m_location = VIVect3();
    m_lastLocation = VIVect3();
    m_haveLocation = 0;
    m_attractorOn = 0;
    m_space.Identity();
}

VIParticleEmitter::~VIParticleEmitter()
{
    Destroy();
}

int VIParticleEmitter::Create(VIParticleSystem *s, VIParticleDefinition *d)
{
    Destroy();

    m_system = s;
    m_definition = d;
    m_blendVersion = 0;
    m_numParticles = 0;
    m_unk4C = 0;
    m_direction.Identity();
    m_space.Identity();
    m_location = VIVect3();
    m_lastLocation = VIVect3();
    m_haveLocation = 0;
    m_attractorOn = 0;

    m_system->m_emitters.AddTail(this);
    m_attributes = *m_definition;
    RecomputeStaticBBox();

    m_dynBounds.min.Zero();
    m_dynBounds.max.Zero();
    return 0;
}

VIMatrix44 &VIParticleEmitter::Space() const
{
    return (VIMatrix44 &)m_space;
}

void VIParticleEmitter::SetSpace(const VIMatrix44 &m)
{
    sceVu0CopyMatrix(m_space, m);
}

VIVect3 &VIParticleEmitter::Location() const
{
    return (VIVect3 &)m_location;
}

VIVect3 &VIParticleEmitter::LastLocation() const
{
    return (VIVect3 &)m_lastLocation;
}

void VIParticleEmitter::SetLocation(const VIVect3 &v)
{
    m_lastLocation = m_location;
    m_location = v;

    if (m_haveLocation == 0)
    {
        m_haveLocation = 1;
        m_lastLocation = m_location;
    }
}

VIMatrix33 &VIParticleEmitter::Direction() const
{
    return (VIMatrix33 &)m_direction;
}

void VIParticleEmitter::SetDirection(const VIVect3 &v)
{
    VIVect3 a;
    VIVect3 up(0.0f, 1.0f, 0.0f);
    VIVect3 c;

    if (fabsf(v.x * up.x + v.y * up.y + v.z * up.z) > 0.999000013f)
        up = VIVect3(0.0f, 0.0f, 1.0f);

    a = Cross(up, v);
    up = Cross(v, a);
    c = v;

    a.Normalize();
    up.Normalize();
    c.Normalize();

    VIMatrix33 m;

    m.m[0] = a;
    m.m[1] = up;
    m.m[2] = c;

    SetDirection(m);
}

void VIParticleEmitter::SetDirection(const VIMatrix33 &m)
{
    m_direction = m;
}

int VIParticleEmitter::LinkMotifBlender(const char *name, VIMotifBlender &b)
{
    return LinkMotifBlender(VIHashResourceID(name), b);
}

int VIParticleEmitter::LinkMotifBlender(unsigned int hash, VIMotifBlender &b)
{
    VIParticleMotif *m = m_definition->FindMotif(hash);

    if (m != 0)
    {
        if (b.m_motif != 0)
            b.Remove();

        m_blenders.AddTail(&b);

        b.m_motif = m;
        b.m_changed = 1;
        b.m_factor = 0.0f;
        return 0;
    }

    return -1;
}

void VIParticleEmitter::UnlinkAllMotifBlenders()
{
    VIMotifBlender *b;

    while ((b = (VIMotifBlender *)m_blenders.RemHead()) != 0)
        b->m_motif = 0;
}

int VIParticleEmitter::ParticleCount() const
{
    return m_numParticles;
}

void VIParticleEmitter::SetAttractor(const VIVect3 &v, float k)
{
    m_attractor = v;
    m_attractorK = k;
    m_attractorOn = 1;
}

void VIParticleEmitter::ClearAttractor()
{
    m_attractorOn = 0;
}

int VIParticleEmitter::Purge()
{
    VILNode *n;

    while ((n = m_particles.RemHead()) != 0)
        m_system->m_freeParticles.AddTail(n);

    m_numParticles = 0;
}

// Phase 61: VOID, not int.  The body is the same either way -- there was never
// a return statement -- but a CALLER sees the difference: VICSprite::Release
// calls this and then loads a local, and only a call with no value lets that
// local have $v0 (a call_value's REG_UNUSED $v0 blocks it); 3 words to 0.
void VIParticleEmitter::Destroy()
{
    if (m_system != 0)
    {
        Remove();

        VILNode *n;

        while ((n = m_particles.RemHead()) != 0)
            m_system->m_freeParticles.AddTail(n);

        m_system = 0;
        m_definition = 0;
        m_numParticles = 0;
        m_space.Identity();
    }

    UnlinkAllMotifBlenders();
}

int VIParticleEmitter::BlendMotifs()
{
    int changed = 0;
    int version = 0;

    for (VIParticleMotif *m = m_definition->FirstMotif(); m != 0;
         m = m_definition->NextMotif(m))
        m->m_unk2C8 = 0.0f;

    VIMotifBlender *b;

    for (b = (VIMotifBlender *)m_blenders.m_head; b->m_next != 0;
         b = (VIMotifBlender *)b->m_next)
    {
        if (b->m_changed != 0)
        {
            b->m_changed = 0;
            changed = 1;
        }

        b->m_motif->m_unk2C8 = b->Factor();

        if (b->Factor() != 0.0f)
            version += b->m_motif->m_version;
    }

    version += m_definition->m_version;

    if (changed || version != m_blendVersion)
    {
        m_definition->BlendMotifs(&m_attributes);
        RecomputeStaticBBox();
        m_blendVersion = version;
    }
}

VIBBox VIParticleEmitter::StaticBounds() const
{
    VIBBox b;
    VIVect3 p[8];

    m_staticBounds.Store(p);

    for (int i = 0; i < 8; i++)
    {
        p[i] = p[i] * m_direction;
        p[i] += m_location;
        b.Bound(i == 0, p[i]);
    }

    return b;
}

VIBBox VIParticleEmitter::DynamicBounds() const
{
    return m_dynBounds;
}

VIBBox VIParticleEmitter::Bounds() const
{
    VIBBox b = StaticBounds();

    b.Union(m_dynBounds);
    return b;
}
