// VIEffectVolume -- SUPPORT 0x00429570 .. 0x0042F798.  A box of weather.  The
// box comes from the sprite; m_type says what fills it and m_density how much
// of it there is.  A surface and a sprite are attached by index, and the
// material the volume draws with is built here rather than by the caller.
#include "vi.h"

extern "C" void *malloc(unsigned int n);
extern "C" void  free(void *p);
extern "C" void *memset(void *p, int c, unsigned int n);
extern "C" int   rand();

// Two draws, differenced and scaled by 2^-31: a signed unit random.
#define VIBOIDRAND (((float)rand() - (float)rand()) * 4.65661287e-10f)

VIScene *VIEffectVolume::mScene;

// The three billboard vertex tables GlobalInit fills: each card is four
// VIVect3 corners, and the zeroing constructor runs over all of them from the
// TU's static initialiser (phase 61) -- which is what names `_GLOBAL_$I$`.
VIVect3 sFallEffect[2][30][4];
VIVect3 sFogCard[8][4];
VIVect3 sDustCard[60][4];

void VIEffectVolume::GlobalFree()
{
    mScene = 0;
}

VIEffectVolume::VIEffectVolume()
{
    m_type = VIEFFECTVOLUME_NONE;
    m_p28 = 1.0f;
    m_density = 1.0f;
    m_p30 = 20.0f;
    m_p34 = 0.0f;
    m_p38 = 0.0f;
    m_flags = 0;
    m_surface = -1;
    m_sprite = -1;
    m_material = -1;
    m_particles = 0;
    m_reseed = 1;
    m_dirty = 1;
    m_time = 0;
}

VIEffectVolume::~VIEffectVolume()
{
}

int VIEffectVolume::Init(const VIBBox &b)
{
    m_bbox = b;
    m_reseed = 1;
    SetType(VIEFFECTVOLUME_NONE, -1.0f);
    m_dirty = 1;
    m_p28 = 1.0f;
    m_density = 1.0f;
    m_p30 = 20.0f;
    m_p34 = 0.0f;
    m_p38 = 0.0f;
    m_flags = 0;
    m_time = 0;
    m_wind.Zero();
    m_p6C = 0;
    return 0;
}

void VIEffectVolume::Free()
{
    DetachAllResources();
    FreeParticles();
}

VIEffectVolumeType VIEffectVolume::Type() const
{
    return m_type;
}

void VIEffectVolume::SetType(VIEffectVolumeType t, float density)
{
    Free();
    m_type = t;
    if (density >= 0.0f)
        m_density = density;
    m_dirty = 1;
}

float VIEffectVolume::Density() const
{
    return m_density;
}

void VIEffectVolume::SetDensity(float d)
{
    m_density = d;
    m_dirty = 1;
}

unsigned int VIEffectVolume::Flags() const
{
    return m_flags;
}

void VIEffectVolume::SetFlags(unsigned int f)
{
    m_flags = f;
}

void VIEffectVolume::OnResize(const VIBBox &b)
{
    m_bbox = b;
    m_reseed = 1;
    m_dirty = 1;
}

int VIEffectVolume::Surface() const
{
    return m_surface;
}

int VIEffectVolume::Sprite() const
{
    return m_sprite;
}

int VIEffectVolume::AttachSprite(int i)
{
    if (i == -1)
        return -1;
    if (m_sprite != -1)
        DetachSprite();
    m_sprite = i;
    return 0;
}

int VIEffectVolume::DetachSprite()
{
    m_sprite = -1;
    return 0;
}

void VIEffectVolume::DetachAllResources()
{
    DetachSurface();
    DetachSprite();
}

int VIEffectVolume::AttachSurface(int surf)
{
    VIRaster *r = mScene->m_raster;

    if (surf == -1)
        return -1;
    if (m_surface != -1)
        DetachSurface();
    m_surface = surf;
    m_material = r->CreateMaterial(0);
    if (m_material < 0)
    {
        m_surface = -1;
        return -1;
    }

    VIMaterial *m = r->Material(m_material);
    m->Init(1);
    m->SetLayerFillType(0, VIMaterialFillTypeTexture);
    m->SetLayerModulate(0, 1);
    switch (m_type)
    {
    case VIEFFECTVOLUME_NONE:
        m->SetLayerBlendMode(0, VIAlphaBlendModeNone);
        break;
    case VIEFFECTVOLUME_LEAVES:
    case VIEFFECTVOLUME_FOG:
    case VIEFFECTVOLUME_DUST:
        m->SetLayerBlendMode(0, VIAlphaBlendMode3);
        break;
    case VIEFFECTVOLUME_SHAFTS:
        m->SetLayerBlendMode(0, VIAlphaBlendMode1);
        break;
    }
    m->SetZWrite(0);
    m->SetZTest(1);
    m->SetDither(0);
    m->SetLayerTexture(0, m_surface, *r, *mScene->m_dict);
    return 0;
}

int VIEffectVolume::DetachSurface()
{
    VIRaster *r = mScene->m_raster;

    if (m_material != -1)
    {
        VIMaterial *m = r->Material(m_material);
        m->SetLayerTexture(0, -1, *r, *mScene->m_dict);
        r->ReleaseMaterial(m_material, *mScene->m_dict);
        m_material = -1;
    }
    m_surface = -1;
    return 0;
}

void VIEffectVolume::FreeParticles()
{
    if (m_type == VIEFFECTVOLUME_FLOCK)
    {
        if (m_particles)
            FreeFlock();
        m_particles = 0;
    }
    else
    {
        if (m_particles)
            free(m_particles);
        m_particles = 0;
    }
}

int VIEffectVolume::AllocFlock()
{
    int i;

    m_numParticles = (int)(m_density * 8.0f) + 1;
    m_particles = malloc(m_numParticles * sizeof(VIBoid));
    if (m_particles == 0)
        return -1;
    for (i = 0; i < m_numParticles; i++)
        ((VIBoid *)m_particles)[i].Init();
    return 0;
}

void VIEffectVolume::FreeFlock()
{
    int i;

    if (m_particles)
    {
        for (i = 0; i < m_numParticles; i++)
            ((VIBoid *)m_particles)[i].Free(*mScene);
        free(m_particles);
        m_particles = 0;
    }
}

int VIEffectVolume::AllocParticles()
{
    VIVect3 s = m_bbox.Size();
    int n;
    int c;

    FreeParticles();
    switch (m_type)
    {
    case VIEFFECTVOLUME_NONE:
        FreeParticles();
        return 0;
    case VIEFFECTVOLUME_LEAVES:
        n = 1;
        c = (int)(s.x * s.y * 0.4f * m_density);
        n = (c > 0) ? c : n;
        m_numParticles = n;
        m_particles = malloc(n * 36);
        if (m_particles == 0)
            return -1;
        memset(m_particles, 0, m_numParticles * 36);
        break;
    case VIEFFECTVOLUME_SHAFTS:
        n = 1;
        c = (int)(s.x * 0.5f * m_density);
        n = (c > 0) ? c : n;
        m_numParticles = n;
        m_particles = malloc(n * 32);
        if (m_particles == 0)
            return -1;
        memset(m_particles, 0, m_numParticles * 32);
        break;
    case VIEFFECTVOLUME_FOG:
        n = 1;
        c = (int)(s.x * 20.0f * m_density);
        n = (c > 0) ? c : n;
        m_numParticles = n;
        m_particles = malloc(n * 36);
        if (m_particles == 0)
            return -1;
        memset(m_particles, 0, m_numParticles * 36);
        break;
    case VIEFFECTVOLUME_DUST:
        n = 1;
        c = (int)(s.x * 20.0f * m_density);
        n = (c > 0) ? c : n;
        m_numParticles = n;
        m_particles = malloc(n * 60);
        if (m_particles == 0)
            return -1;
        memset(m_particles, 0, m_numParticles * 60);
        break;
    case VIEFFECTVOLUME_FLOCK:
        if (AllocFlock() < 0)
            return -1;
        break;
    }
    m_reseed = 1;
    return 0;
}

int VIEffectVolume::Process(float dt)
{
    long  now = VITimer::GetTime();
    float t = (float)(int)(now - m_time) * 0.001f;

    if (m_dirty)
    {
        AllocParticles();
        m_dirty = 0;
    }
    if (t >= 0.016666668f)
    {
        m_time = now;
        if (t > 0.1f)
            t = 0.1f;
        switch (m_type)
        {
        case VIEFFECTVOLUME_NONE:
            return 0;
        case VIEFFECTVOLUME_LEAVES:
            ProcessFallingLeaves(t);
            return 0;
        case VIEFFECTVOLUME_SHAFTS:
            ProcessLightShafts(t);
            return 0;
        case VIEFFECTVOLUME_FOG:
            ProcessGroundFog(t);
            return 0;
        case VIEFFECTVOLUME_DUST:
            ProcessDustStorm(t);
            return 0;
        case VIEFFECTVOLUME_FLOCK:
            ProcessFlock(t);
            return 0;
        }
    }
    return 0;
}

void VIBoid::SeekGoal(float dt, VIVect3 &goal)
{
}

void VIBoid::Free(VIScene &sc)
{
    if (m_sprite != -1)
        sc.ReleaseSprite(m_sprite);
    m_sprite = -1;
}

void VIBoid::Apply(float dt)
{
    m_xform.RotateHPR(m_hpr);
    m_xform.m[3].x = m_pos.x;
    m_xform.m[3].y = m_pos.y;
    m_xform.m[3].z = m_pos.z;
    m_dir.Set(m_xform.m[2].x, m_xform.m[2].y, m_xform.m[2].z);
}

void VIBoid::Render(VIScene &sc, VICamera &cam)
{
    if (m_sprite != -1)
    {
        VIMatrix44 mv = *sc.m_raster->GetModelView();
        VIMatrix44 m;

        m.Mul(m_xform, mv);
        sc.m_raster->SetModelView(m);
        sc.RasterSprite(cam, m_sprite, 1);
        sc.m_raster->SetModelView(mv);
    }
}

void VIBoid::Init()
{
    m_sprite = -1;
    m_hpr.Set(VIBOIDRAND * 0.2f, VIBOIDRAND * 0.2f, VIBOIDRAND * 0.2f);
    m_p10 = 0;
    m_dir.Set(0.0f, 0.0f, 1.0f);
    m_pos.Set(VIBOIDRAND * 3.0f, VIBOIDRAND * 3.0f, VIBOIDRAND * 3.0f);
    m_pA0 = -1;
    m_pA4 = VIBOIDRAND * 0.3f + 1.0f;
    m_pA8 = VIBOIDRAND * 0.3f + 1.0f;
    m_xform.Identity();
}
