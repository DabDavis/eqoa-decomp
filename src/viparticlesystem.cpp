// VIParticleSystem -- SUPPORT 0x00458780 .. 0x00459250.
#include "vi.h"

VIParticleSystem::VIParticleSystem()
{
    m_particles = 0;
    m_maxParticles = 0;
    m_raster = 0;
    m_time = 0;
}

VIParticleSystem::~VIParticleSystem()
{
}

int VIParticleSystem::Init(long time, VIRaster *r, int maxParticles)
{
    m_maxParticles = maxParticles;
    m_time = time;
    m_raster = r;

    m_particles = new VIParticle[maxParticles];

    m_freeParticles.Init();

    // One counter for both loops: the reversed history loop keeps it in s0.
    int i;

    for (i = 0; i < m_maxParticles; i++)
        m_freeParticles.AddTail(&m_particles[i]);

    for (i = 0; i < 8; i++)
        m_dtHistory[i] = 1.0f / 30.0f;

    m_deltaTime = 0.0f;
    return 0;
}

int VIParticleSystem::Clear(VIDictionary &d)
{
    VIParticleEmitter *e;

    for (e = (VIParticleEmitter *)m_emitters.m_head; e->m_next != 0;
         e = (VIParticleEmitter *)e->m_next)
    {
        if (e->m_system != 0)
        {
            e->m_particles.Init();
            e->m_numParticles = 0;
            e->m_system = 0;
            e->m_definition = 0;
        }
    }

    m_emitters.Init();
    m_freeParticles.Init();

    for (int i = 0; i < m_maxParticles; i++)
        m_freeParticles.AddTail(&m_particles[i]);

    ReleaseAllParticleDefinitions(d);

    if (m_particles != 0)
    {
        delete [] m_particles;
        m_particles = 0;
    }

    m_maxParticles = 0;
    return 0;
}

void VIParticleSystem::Purge()
{
    VIParticleEmitter *e;

    for (e = (VIParticleEmitter *)m_emitters.m_head; e->m_next != 0;
         e = (VIParticleEmitter *)e->m_next)
        e->Purge();
}

int VIParticleSystem::CreateParticleDefinition(int share)
{
    VIParticleDefinition *d = new VIParticleDefinition;

    if (d == 0)
        return -1;

    if (d->Create(*m_raster) < 0)
    {
        delete d;
        return -1;
    }

    if (share)
    {
        VIResource *r = &d->m_resource;

        r->m_link++;
    }

    return m_definitions.Add(d);
}

VIParticleDefinition *VIParticleSystem::ParticleDefinition(int i)
{
    return m_definitions.Value(i);
}

int VIParticleSystem::ShareParticleDefinition(int i)
{
    VIParticleDefinition *d = m_definitions.Value(i);
    VIResource *r = &d->m_resource;

    r->m_link++;
    return 0;
}

int VIParticleSystem::ReleaseParticleDefinition(int i, VIDictionary &d)
{
    VIParticleDefinition *p = m_definitions.Value(i);
    VIResource *r = &p->m_resource;

    r->m_link--;

    if (r->m_link <= 0)
    {
        if (p->m_resource.m_id != 0)
            d.Remove(*r);

        if (p->Destroy(*m_raster, d) < 0)
            return -1;

        delete p;

        int e = m_definitions.Erase(i);

        if (e < 0)
            return -1;
    }

    return 0;
}

int VIParticleSystem::ReleaseAllParticleDefinitions(VIDictionary &d)
{
    int i;

    while ((i = m_definitions.Begin()) != m_definitions.End())
        ReleaseParticleDefinition(i, d);

    m_definitions.Clear();
    return 0;
}

long VIParticleSystem::Time() const
{
    return m_time;
}

float VIParticleSystem::DeltaTime() const
{
    return m_deltaTime;
}

// One frame of every emitter: the frame delta is the clock difference capped
// at 0.1 s and smoothed over the last eight frames.  Each live particle is
// handed to the raster as a billboard (coloured off the gradient by its age),
// then moved, slowed by its friction, pulled by the emitter's attractor and
// gravity, aged and grown; it is freed once its age reaches 1.  The dynamic
// bounds are rebuilt along the way.
int VIParticleSystem::Render(long time, VIRaster &r)
{
    int i;
    float dt = (float)(int)(time - m_time) * 0.001f;

    m_time = time;
    dt = VIMIN(0.1f, dt);

    float sum = 0.0f;

    for (i = 0; i < 7; i++)
        sum += m_dtHistory[i];
    sum = (sum + dt) * 0.125f;
    for (i = 0; i < 7; i++)
        m_dtHistory[7 - i] = m_dtHistory[6 - i];
    m_dtHistory[0] = dt;
    m_deltaTime = sum;

    VIParticleEmitter *e = (VIParticleEmitter *)m_emitters.m_head;

    while (e->m_next)
    {
        if (e->m_particles.m_head->m_next)
        {
            r.SetMaterial(e->m_definition->m_material);

            VIVect3      v;
            VIVect4      c;
            VIBillboard *bb = 0;
            int          left;

            int n = e->m_numParticles;

            e->m_dynBounds.min.Set(1.0e7f, 1.0e7f, 1.0e7f);
            e->m_dynBounds.max.Set(-1.0e7f, -1.0e7f, -1.0e7f);

            VIParticle *p = (VIParticle *)e->m_particles.m_head;

            while (p->m_next)
            {
                VIVect3    &vel = p->m_vel;
                VIParticle *next = (VIParticle *)p->m_next;

                v = vel;
                v *= m_deltaTime;
                vel *= 1.0f - p->m_friction * m_deltaTime;
                if (bb == 0)
                    left = r.BeginBillboards(&bb, n);

                VIVect3 &pos = p->m_pos;

                e->m_space.Mul(pos, bb->m_location);
                bb->m_width = p->m_size;
                bb->m_height = p->m_size;
                bb->m_frame = p->m_frame << 2;
                bb->m_color = e->m_attributes.m_gradient[(int)(p->m_age * 31.0f * p->m_gradRepeat) & 31];
                pos += v;
                if (p->m_pos.x < e->m_dynBounds.min.x)
                    e->m_dynBounds.min.x = p->m_pos.x;
                if (p->m_pos.y < e->m_dynBounds.min.y)
                    e->m_dynBounds.min.y = p->m_pos.y;
                if (p->m_pos.z < e->m_dynBounds.min.z)
                    e->m_dynBounds.min.z = p->m_pos.z;
                if (p->m_pos.x > e->m_dynBounds.max.x)
                    e->m_dynBounds.max.x = p->m_pos.x;
                if (p->m_pos.y > e->m_dynBounds.max.y)
                    e->m_dynBounds.max.y = p->m_pos.y;
                if (p->m_pos.z > e->m_dynBounds.max.z)
                    e->m_dynBounds.max.z = p->m_pos.z;
                bb++;
                n--;
                if (--left == 0)
                {
                    r.EndBillboards();
                    bb = 0;
                }
                if (e->m_attractorOn)
                {
                    v = e->m_attractor;
                    v -= pos;
                    v.Normalize();
                    v *= e->m_attractorK * m_deltaTime;
                    vel += v;
                }
                p->m_vel.y += p->m_gravity * m_deltaTime;
                p->m_age += p->m_ageRate * m_deltaTime;
                p->m_size += p->m_growth * m_deltaTime;
                if (1.0f <= p->m_age)
                {
                    p->Remove();
                    e->m_numParticles--;
                    m_freeParticles.AddTail(p);
                }
                p = next;
            }
            if (bb)
            {
                r.EndBillboards();
                bb = 0;
            }
        }
        e = (VIParticleEmitter *)e->m_next;
    }
    return 0;
}
