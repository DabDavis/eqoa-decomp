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

    for (int i = 0; i < m_maxParticles; i++)
        m_freeParticles.AddTail(&m_particles[i]);

    for (int j = 0; j < 8; j++)
        m_dtHistory[j] = 1.0f / 30.0f;

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
    for (int i = m_definitions.Begin(); i != m_definitions.End(); i = m_definitions.Begin())
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
