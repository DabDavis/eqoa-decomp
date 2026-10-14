// VIParticleEmitter -- SUPPORT 0x00456910 .. 0x00458480.
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);

extern "C" float fabsf(float);
extern "C" int   rand();

// Two draws, differenced and scaled by 2^-31: a signed unit random.
#define VIRAND (((float)rand() - (float)rand()) * 4.65661287e-10f)

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

static inline VIVect3 operator-(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x - b.x, a.y - b.y, a.z - b.z);
}

// One frame of emission.  The birth count is the birthrate (plus its
// variance) over the frame, plus one per unit the emitter moved times the
// delta-spawn rate, plus the fraction carried from last frame; the births are
// spread evenly along the path the emitter took.  Each particle is placed (on
// a random pattern point if there is a pattern), given its life, friction,
// gravity, frame, size and growth, and launched down the nozzle swung by the
// three spread angles and offsets, at its speed, plus the inherited velocity.
int VIParticleEmitter::Update()
{
    int i;

    if (m_system == 0)
        return -1;
    if (m_definition == 0)
        return -1;

    BlendMotifs();

    float dt = m_system->m_deltaTime;
    float invDt = 1.0f / dt;
    float n = (m_attributes.m_birthrate + VIRAND * m_attributes.m_birthrateVar) * dt;

    if (m_attributes.m_deltaSpawn != 0.0f)
        n += m_location.Distance(m_lastLocation) * m_attributes.m_deltaSpawn;
    n += *(float *)&m_unk4C;

    int count = (int)n;

    *(float *)&m_unk4C = n - (float)count;

    float step = 1.0f / (float)count;
    VIVect3 at = m_lastLocation;
    VIVect3 d = m_location - m_lastLocation;

    d *= step;
    VIMatrix44 m;
    VIVect3 off;
    VIVect3 dir;
    VIVect3 a;
    VIVect3 b;
    int first;
    int num;

    switch (m_definition->TextureConfiguration())
    {
    case 1:
        first = 0;
        num = 1;
        break;
    case 2:
        first = 5;
        num = 2;
        break;
    case 3:
        first = 1;
        num = 3;
        break;
    case 4:
        first = 1;
        num = 4;
        break;
    }

    VIParticle *p;

    for (i = 0; i < count && (p = (VIParticle *)m_system->m_freeParticles.RemHead()) != 0; i++)
    {
        if (m_attributes.m_pattern)
        {
            (VIVect3 &)p->m_f20 = m_attributes.m_pattern[rand() % m_attributes.m_patternSize];
            (VIVect3 &)p->m_f20 = (VIVect3 &)p->m_f20 * m_direction;
            (VIVect3 &)p->m_f20 += at;
        }
        else
            (VIVect3 &)p->m_f20 = at;
        at += d;
        m_particles.AddTail(p);
        m_numParticles++;
        p->m_age = 0.0f;
        p->m_ageRate = m_attributes.m_lifespan + VIRAND * m_attributes.m_lifespanVar;
        float life;

        if (p->m_ageRate < 0.0f)
            life = 0.0f;
        else if (p->m_ageRate < 1000.0f)
            life = p->m_ageRate;
        else
            life = 1000.0f;

        p->m_ageRate = 1.0f / life;
        p->m_friction = m_attributes.m_friction;
        p->m_gravity = m_attributes.m_gravity;
        if (first)
            p->m_frame = first + rand() % num;
        else
            p->m_frame = 0;
        p->m_gradRepeat = m_attributes.m_gradientRepeat;
        p->m_size = (m_attributes.m_startSize + VIRAND * m_attributes.m_startSizeVar) * 0.5f;
        p->m_growth = ((m_attributes.m_endSize + VIRAND * m_attributes.m_endSizeVar) * 0.5f -
                       p->m_size) * p->m_ageRate;
        off.Zero();
        dir = m_attributes.m_nozzleAxis;
        if (m_attributes.m_nozzleHprVarSet)
        {
            b = dir;

            float h = m_attributes.m_nozzleHprVar.x * VIRAND;
            float t = m_attributes.m_nozzleHprVar.y * VIRAND;
            float r = m_attributes.m_nozzleHprVar.z * VIRAND;

            m.RotateHPR(VIVect3(h, t, r));
            m.Mul(b, dir);
        }
        if (m_attributes.m_outerHprVarSet | m_attributes.m_outerOffsetSet)
        {
            a = off;
            a += m_attributes.m_outerOffset;
            b = dir;

            float h = m_attributes.m_outerHprVar.x * VIRAND;
            float t = m_attributes.m_outerHprVar.y * VIRAND;
            float r = m_attributes.m_outerHprVar.z * VIRAND;

            m.RotateHPR(VIVect3(h, t, r));
            m.Mul(a, off);
            m.Mul(b, dir);
        }
        if (m_attributes.m_innerHprVarSet | m_attributes.m_innerOffsetSet)
        {
            a = off;
            a += m_attributes.m_innerOffset;
            b = dir;

            float h = m_attributes.m_innerHprVar.x * VIRAND;
            float t = m_attributes.m_innerHprVar.y * VIRAND;
            float r = m_attributes.m_innerHprVar.z * VIRAND;

            m.RotateHPR(VIVect3(h, t, r));
            m.Mul(a, off);
            m.Mul(b, dir);
        }
        dir = dir * m_direction;
        off = off * m_direction;
        (VIVect3 &)p->m_f20 += off;
        dir *= m_attributes.m_velocity + VIRAND * m_attributes.m_velocityVar;
        (VIVect3 &)p->m_f2C = dir;
        if (m_attributes.m_inheritVelocity != 0.0f)
        {
            b = m_location;
            b -= m_lastLocation;
            b *= invDt;
            b *= m_attributes.m_inheritVelocity;
            (VIVect3 &)p->m_f2C += b;
        }
    }
    return 0;
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

static inline VIVect3 operator+(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x + b.x, a.y + b.y, a.z + b.z);
}

// The static box is swept out by twelve test particles: each is aimed down
// the nozzle axis at its full reach (lifespan x speed, both at their extreme),
// swung to one of four fractions of each spread angle in turn, and offset by
// the outer and inner emit offsets.  Every start and end point is bounded, and
// the box is grown by half the largest particle.
void VIParticleEmitter::RecomputeStaticBBox()
{
    int i;

    m_staticBounds.min.Zero();
    m_staticBounds.max.Zero();

    float s0 = m_attributes.m_startSize + fabsf(m_attributes.m_startSizeVar);
    float s1 = m_attributes.m_endSize + fabsf(m_attributes.m_endSizeVar);
    float life = m_attributes.m_lifespan + fabsf(m_attributes.m_lifespanVar);
    float speed = m_attributes.m_velocity + fabsf(m_attributes.m_velocityVar);
    float half = VIMAX(s0, s1) * 0.5f;

    VIMatrix44 m;
    VIVect3    base;
    VIVect3    dir;
    VIVect3    p;
    VIVect3    q;
    float      ang[4] = { -1.0f, -0.33f, 0.33f, 1.0f };

    // The nozzle clamps take pi as a LITERAL and the outer/inner ones as this
    // in-loop local: fold swaps a constant to the right of `*`, so the nozzle
    // arms read `ang * pi` and the others `pi * ang`, and loop.c matches the
    // two equal movables into the one hoisted $f20.
    for (i = 0; i < 12; i++)
    {
        float pi = 3.1415927f;

        base.Zero();
        dir = m_attributes.m_nozzleAxis;
        dir *= life * speed;
        if (m_attributes.m_nozzleHprVarSet)
        {
            q = dir;
            switch (i % 3)
            {
            case 0:
                m.RotateHPR(VIVect3(VIMIN(m_attributes.m_nozzleHprVar.x, 3.1415927f) * ang[i & 3], 0.0f, 0.0f));
                break;
            case 1:
                m.RotateHPR(VIVect3(0.0f, VIMIN(m_attributes.m_nozzleHprVar.y, 3.1415927f) * ang[i & 3], 0.0f));
                break;
            case 2:
                m.RotateHPR(VIVect3(0.0f, 0.0f, VIMIN(m_attributes.m_nozzleHprVar.z, 3.1415927f) * ang[i & 3]));
                break;
            }
            m.Mul(q, dir);
        }
        if (m_attributes.m_outerHprVarSet | m_attributes.m_outerOffsetSet)
        {
            p = base;
            p += m_attributes.m_outerOffset;
            q = dir;
            switch ((i + 4) % 3)
            {
            case 0:
                m.RotateHPR(VIVect3(VIMIN(m_attributes.m_outerHprVar.x, pi) * ang[i & 3], 0.0f, 0.0f));
                break;
            case 1:
                m.RotateHPR(VIVect3(0.0f, VIMIN(m_attributes.m_outerHprVar.y, pi) * ang[i & 3], 0.0f));
                break;
            case 2:
                m.RotateHPR(VIVect3(0.0f, 0.0f, VIMIN(m_attributes.m_outerHprVar.z, pi) * ang[i & 3]));
                break;
            }
            m.Mul(p, base);
            m.Mul(q, dir);
        }
        if (m_attributes.m_innerHprVarSet | m_attributes.m_innerOffsetSet)
        {
            p = base;
            p += m_attributes.m_innerOffset;
            q = dir;
            switch ((i + 8) % 3)
            {
            case 0:
                m.RotateHPR(VIVect3(VIMIN(m_attributes.m_innerHprVar.x, pi) * ang[i & 3], 0.0f, 0.0f));
                break;
            case 1:
                m.RotateHPR(VIVect3(0.0f, VIMIN(m_attributes.m_innerHprVar.y, pi) * ang[i & 3], 0.0f));
                break;
            case 2:
                m.RotateHPR(VIVect3(0.0f, 0.0f, VIMIN(m_attributes.m_innerHprVar.z, pi) * ang[i & 3]));
                break;
            }
            m.Mul(p, base);
            m.Mul(q, dir);
        }

        VIVect3 e = base + dir;

        m_staticBounds.min.x = VIMIN(e.x, m_staticBounds.min.x);
        m_staticBounds.min.y = VIMIN(e.y, m_staticBounds.min.y);
        m_staticBounds.min.z = VIMIN(e.z, m_staticBounds.min.z);
        m_staticBounds.max.x = VIMAX(m_staticBounds.max.x, e.x);
        m_staticBounds.max.y = VIMAX(m_staticBounds.max.y, e.y);
        m_staticBounds.max.z = VIMAX(m_staticBounds.max.z, e.z);
        m_staticBounds.min.x = VIMIN(base.x, m_staticBounds.min.x);
        m_staticBounds.min.y = VIMIN(base.y, m_staticBounds.min.y);
        m_staticBounds.min.z = VIMIN(base.z, m_staticBounds.min.z);
        m_staticBounds.max.x = VIMAX(m_staticBounds.max.x, base.x);
        m_staticBounds.max.y = VIMAX(m_staticBounds.max.y, base.y);
        m_staticBounds.max.z = VIMAX(m_staticBounds.max.z, base.z);
    }
    m_staticBounds.min -= half;
    m_staticBounds.max += half;
}
