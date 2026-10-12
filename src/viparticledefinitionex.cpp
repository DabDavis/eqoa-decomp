// VIParticleMotif + VIParticleDefinitionEx -- SUPPORT 0x0045A3D8 .. 0x0045B640.
#include "vi.h"

extern "C" char *strcpy(char *, const char *);
extern "C" unsigned int VIHashResourceID(const char *);

VIParticleMotif::VIParticleMotif()
{
    SetName("");
}

VIParticleMotif::~VIParticleMotif()
{
}

const char *VIParticleMotif::Name() const
{
    return m_name;
}

void VIParticleMotif::SetName(const char *name)
{
    strcpy(m_name, name);
    m_nameHash = VIHashResourceID(name);
}

unsigned int VIParticleMotif::NameHash() const
{
    return m_nameHash;
}

VIParticleDefinitionEx::VIParticleDefinitionEx()
{
    m_blendMode = BLENDMODE_0;
    m_zWrite = 0;
    m_zTest = 1;
    m_texConfig = 1;
}

VIParticleDefinitionEx::~VIParticleDefinitionEx()
{
}

VIParticleDefinitionEx::BLENDMODE VIParticleDefinitionEx::BlendMode() const
{
    return m_blendMode;
}

void VIParticleDefinitionEx::SetBlendModeEx(BLENDMODE m)
{
    m_blendMode = m;
}

bool VIParticleDefinitionEx::ZWrite() const
{
    return m_zWrite;
}

void VIParticleDefinitionEx::SetZWriteEx(bool on)
{
    m_zWrite = on;
}

bool VIParticleDefinitionEx::ZTest() const
{
    return m_zTest;
}

void VIParticleDefinitionEx::SetZTestEx(bool on)
{
    m_zTest = on;
}

int VIParticleDefinitionEx::TextureConfiguration() const
{
    return m_texConfig;
}

void VIParticleDefinitionEx::SetTextureConfigurationEx(int c)
{
    m_texConfig = c;
}

VIParticleMotif *VIParticleDefinitionEx::CreateMotif()
{
    VIParticleMotif *m = new VIParticleMotif;

    m_motifs.Add(m);
    return m;
}

void VIParticleDefinitionEx::DestroyMotif(VIParticleMotif *m)
{
    for (int i = m_motifs.Begin(); i < m_motifs.End(); i = m_motifs.Next(i))
    {
        if (m_motifs.Value(i) == m)
        {
            m_motifs.Erase(i);
            delete m;
            return;
        }
    }
}

VIParticleMotif *VIParticleDefinitionEx::FindMotif(const char *name) const
{
    return FindMotif(VIHashResourceID(name));
}

VIParticleMotif *VIParticleDefinitionEx::FindMotif(unsigned int hash) const
{
    for (int i = m_motifs.Begin(); i < m_motifs.End(); i = m_motifs.Next(i))
    {
        if (hash == m_motifs.Value(i)->NameHash())
            return m_motifs.Value(i);
    }

    return 0;
}

VIParticleMotif *VIParticleDefinitionEx::FirstMotif() const
{
    const VIPool<VIParticleMotif *> *p = &m_motifs;

    if (p->Size() != 0)
    {
        const VIPool<VIParticleMotif *>::Node *n = &p->m_nodes[p->Begin()];
        return n->m_data;
    }

    return 0;
}

VIParticleMotif *VIParticleDefinitionEx::NextMotif(VIParticleMotif *m) const
{
    for (int i = m_motifs.Begin(); i < m_motifs.End(); i = m_motifs.Next(i))
    {
        if (m_motifs.Value(i) == m)
        {
            if (m_motifs.Next(i) < m_motifs.End())
                return m_motifs.Value(m_motifs.Next(i));
        }
    }

    return 0;
}

int VIParticleDefinitionEx::MotifCount() const
{
    return m_motifs.Size();
}

void VIParticleDefinitionEx::BlendMotifs(VIParticleAttributes *out)
{
    *out = *this;

    for (VIParticleMotif *m = FirstMotif(); m != 0; m = NextMotif(m))
    {
        float f = m->m_unk2C8;

        if (f == 0.0f)
            continue;

        out->m_friction += (m->m_friction - m_friction) * f;
        out->m_birthrate += (m->m_birthrate - m_birthrate) * f;
        out->m_birthrateVar += (m->m_birthrateVar - m_birthrateVar) * f;
        out->m_lifespan += (m->m_lifespan - m_lifespan) * f;
        out->m_lifespanVar += (m->m_lifespanVar - m_lifespanVar) * f;
        out->m_velocity += (m->m_velocity - m_velocity) * f;
        out->m_velocityVar += (m->m_velocityVar - m_velocityVar) * f;
        out->m_unk1C.x += (m->m_unk1C.x - m_unk1C.x) * f;
        out->m_unk1C.y += (m->m_unk1C.y - m_unk1C.y) * f;
        out->m_unk1C.z += (m->m_unk1C.z - m_unk1C.z) * f;
        out->m_startSize += (m->m_startSize - m_startSize) * f;
        out->m_startSizeVar += (m->m_startSizeVar - m_startSizeVar) * f;
        out->m_endSize += (m->m_endSize - m_endSize) * f;
        out->m_endSizeVar += (m->m_endSizeVar - m_endSizeVar) * f;
        out->m_gravity += (m->m_gravity - m_gravity) * f;

        for (int i = 0; i < 32; i++)
        {
            out->m_gradient[i].r += (m->m_gradient[i].r - m_gradient[i].r) * f;
            out->m_gradient[i].g += (m->m_gradient[i].g - m_gradient[i].g) * f;
            out->m_gradient[i].b += (m->m_gradient[i].b - m_gradient[i].b) * f;
            out->m_gradient[i].a += (m->m_gradient[i].a - m_gradient[i].a) * f;
        }

        out->m_startColorVar.r += (m->m_startColorVar.r - m_startColorVar.r) * f;
        out->m_startColorVar.g += (m->m_startColorVar.g - m_startColorVar.g) * f;
        out->m_startColorVar.b += (m->m_startColorVar.b - m_startColorVar.b) * f;
        out->m_startColorVar.a += (m->m_startColorVar.a - m_startColorVar.a) * f;
        out->m_endColorVar.r += (m->m_endColorVar.r - m_endColorVar.r) * f;
        out->m_endColorVar.g += (m->m_endColorVar.g - m_endColorVar.g) * f;
        out->m_endColorVar.b += (m->m_endColorVar.b - m_endColorVar.b) * f;
        out->m_endColorVar.a += (m->m_endColorVar.a - m_endColorVar.a) * f;
        out->m_inheritVelocity += (m->m_inheritVelocity - m_inheritVelocity) * f;
        out->m_deltaSpawn += (m->m_deltaSpawn - m_deltaSpawn) * f;
        out->m_gradientRepeat += (m->m_gradientRepeat - m_gradientRepeat) * f;
        out->m_innerOffset.x += (m->m_innerOffset.x - m_innerOffset.x) * f;
        out->m_innerOffset.y += (m->m_innerOffset.y - m_innerOffset.y) * f;
        out->m_innerOffset.z += (m->m_innerOffset.z - m_innerOffset.z) * f;
        out->m_innerHprVar.x += (m->m_innerHprVar.x - m_innerHprVar.x) * f;
        out->m_innerHprVar.y += (m->m_innerHprVar.y - m_innerHprVar.y) * f;
        out->m_innerHprVar.z += (m->m_innerHprVar.z - m_innerHprVar.z) * f;
        out->m_outerOffset.x += (m->m_outerOffset.x - m_outerOffset.x) * f;
        out->m_outerOffset.y += (m->m_outerOffset.y - m_outerOffset.y) * f;
        out->m_outerOffset.z += (m->m_outerOffset.z - m_outerOffset.z) * f;
        out->m_outerHprVar.x += (m->m_outerHprVar.x - m_outerHprVar.x) * f;
        out->m_outerHprVar.y += (m->m_outerHprVar.y - m_outerHprVar.y) * f;
        out->m_outerHprVar.z += (m->m_outerHprVar.z - m_outerHprVar.z) * f;
        out->m_nozzleAxis.x += (m->m_nozzleAxis.x - m_nozzleAxis.x) * f;
        out->m_nozzleAxis.y += (m->m_nozzleAxis.y - m_nozzleAxis.y) * f;
        out->m_nozzleAxis.z += (m->m_nozzleAxis.z - m_nozzleAxis.z) * f;
        out->m_nozzleHprVar.x += (m->m_nozzleHprVar.x - m_nozzleHprVar.x) * f;
        out->m_nozzleHprVar.y += (m->m_nozzleHprVar.y - m_nozzleHprVar.y) * f;
        out->m_nozzleHprVar.z += (m->m_nozzleHprVar.z - m_nozzleHprVar.z) * f;
    }

    if (out->m_innerOffset.x != 0.0f || out->m_innerOffset.y != 0.0f || out->m_innerOffset.z != 0.0f)
        out->m_innerOffsetSet = 1;
    else
        out->m_innerOffsetSet = 0;
    if (out->m_innerHprVar.x != 0.0f || out->m_innerHprVar.y != 0.0f || out->m_innerHprVar.z != 0.0f)
        out->m_innerHprVarSet = 1;
    else
        out->m_innerHprVarSet = 0;
    if (out->m_outerOffset.x != 0.0f || out->m_outerOffset.y != 0.0f || out->m_outerOffset.z != 0.0f)
        out->m_outerOffsetSet = 1;
    else
        out->m_outerOffsetSet = 0;
    if (out->m_outerHprVar.x != 0.0f || out->m_outerHprVar.y != 0.0f || out->m_outerHprVar.z != 0.0f)
        out->m_outerHprVarSet = 1;
    else
        out->m_outerHprVarSet = 0;
    if (out->m_nozzleHprVar.x != 0.0f || out->m_nozzleHprVar.y != 0.0f || out->m_nozzleHprVar.z != 0.0f)
        out->m_nozzleHprVarSet = 1;
    else
        out->m_nozzleHprVarSet = 0;
}

void VIParticleDefinitionEx::DuplicateEx(VIParticleDefinitionEx *d) const
{
    VIParticleAttributes *a = d;

    *a = *this;
    d->m_blendMode = m_blendMode;
    d->m_zWrite = m_zWrite;
    d->m_zTest = m_zTest;
    d->m_texConfig = m_texConfig;

    VIParticleMotif *m;

    while ((m = d->FirstMotif()) != 0)
        d->DestroyMotif(m);

    for (m = FirstMotif(); m != 0; m = NextMotif(m))
    {
        VIParticleMotif *n = new VIParticleMotif;

        *n = *m;
        d->m_motifs.Add(n);
    }
}
