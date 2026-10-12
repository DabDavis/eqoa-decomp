// VIParticleSprite -- SUPPORT 0x00454E38 .. 0x004564E0.
#include "vi.h"

VIParticleSprite::VIParticleSprite()
{
    m_type = 10;
    m_defIndex = -1;
    m_unk430 = 0;
}

VIParticleSprite::~VIParticleSprite()
{
}

int VIParticleSprite::Create(int def, VIScene &sc)
{
    VIParticleSystem *sys = sc.ParticleSystem();

    if (sys == 0)
        return -1;

    if (m_emitter.Create(sys, sys->ParticleDefinition(def)) < 0)
        return -1;

    m_bbox = m_emitter.StaticBounds();
    m_bsphere.Init(m_bbox);
    m_defIndex = def;
    sys->ShareParticleDefinition(def);
    return 0;
}

int VIParticleSprite::GetInstType(VIScene &sc) const
{
    return 1;
}

int VIParticleSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VIParticleSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VIParticleSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);

    if (i < 0)
        return -1;

    VIParticleSprite *d = (VIParticleSprite *)sc.Sprite(i);

    d->m_bbox = m_bbox;
    d->m_bsphere = m_bsphere;
    d->m_type = m_type;
    d->m_offDist = m_offDist;
    d->m_unk430 = m_unk430;

    if (m_defIndex != -1)
    {
        d->m_defIndex = m_defIndex;

        VIParticleEmitter *e = &m_emitter;

        d->m_emitter.Create(e->m_system, e->m_definition);
        d->m_emitter.SetLocation(m_emitter.Location());
        d->m_emitter.SetLocation(m_emitter.Location());
        d->m_emitter.SetDirection(m_emitter.Direction());
        d->m_emitter.SetSpace(m_emitter.Space());

        VIParticleSystem *sys = sc.ParticleSystem();

        if (sys != 0)
            sys->ShareParticleDefinition(m_defIndex);
    }

    return i;
}

VIParticleDefinition *VIParticleSprite::ParticleDefinition()
{
    return m_emitter.m_definition;
}

VIParticleEmitter *VIParticleSprite::ParticleEmitter()
{
    return &m_emitter;
}
