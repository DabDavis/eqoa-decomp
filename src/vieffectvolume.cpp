// VIEffectVolume -- SUPPORT 0x00429570 .. 0x0042F798.  A box of weather.  The
// box comes from the sprite; m_type says what fills it and m_density how much
// of it there is.  A surface and a sprite are attached by index, and the
// material the volume draws with is built here rather than by the caller.
#include "vi.h"

extern "C" void *malloc(unsigned int n);
extern "C" void  free(void *p);
extern "C" void *memset(void *p, int c, unsigned int n);
extern "C" int   rand();
extern "C" float atanf(float);
extern "C" void  sceVu0CopyMatrix(VIMatrix44 &d, const VIMatrix44 &s);

static inline VIVect3 operator*(const VIVect3 &v, float f)
{
    return VIVect3(v.x * f, v.y * f, v.z * f);
}

static inline VIVect3 operator-(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline VIVect3 Cross(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.y * b.z - a.z * b.y,
                   -a.x * b.z + a.z * b.x,
                   a.x * b.y - a.y * b.x);
}

// Two draws, differenced and scaled by 2^-31: a signed unit random.
#define VIBOIDRAND (((float)rand() - (float)rand()) * 4.65661287e-10f)

VIScene *VIEffectVolume::mScene;

// The billboard cards GlobalInit bakes once: a falling leaf's two swing
// poses (30 frames each), the fog's eight rolls and the dust's sixty.  Four
// corners per card.  sFallEffect is the first object, so it names the TU's
// static constructor.
VIVect3 sFallEffect[2][30][4];
VIVect3 sFogCard[8][4];
VIVect3 sDustCard[60][4];

int VIEffectVolume::GlobalInit(VIScene *sc)
{
    int i;

    mScene = sc;
    VIVect3 v[4];
    VIVect3 t[4];
    for (i = 0; i < 8; i++)
    {
        v[0].Set(-0.5f, 0.5f, 0.0f);
        v[1].Set(0.5f, 0.5f, 0.0f);
        v[2].Set(0.5f, -0.5f, 0.0f);
        v[3].Set(-0.5f, -0.5f, 0.0f);
        VIMatrix44 m;
        m.RotateZ((float)i / 8.0f * 2.0f * 3.1415927f);
        m.Mul(v[0], sFogCard[i][0]);
        m.Mul(v[1], sFogCard[i][1]);
        m.Mul(v[2], sFogCard[i][2]);
        m.Mul(v[3], sFogCard[i][3]);
    }
    for (i = 0; i < 60; i++)
    {
        v[0].Set(-0.5f, 0.5f, 0.0f);
        v[1].Set(0.5f, 0.5f, 0.0f);
        v[2].Set(0.5f, -0.5f, 0.0f);
        v[3].Set(-0.5f, -0.5f, 0.0f);
        VIMatrix44 m;
        m.RotateZ((float)-i / 60.0f * 2.0f * 3.1415927f);
        m.Mul(v[0], sDustCard[i][0]);
        m.Mul(v[1], sDustCard[i][1]);
        m.Mul(v[2], sDustCard[i][2]);
        m.Mul(v[3], sDustCard[i][3]);
    }
    for (i = 0; i < 30; i++)
    {
        float h;

        v[0].Set(-0.1f, -0.5f, -0.1f);
        v[1].Set(0.1f, -0.5f, -0.1f);
        v[2].Set(0.1f, -0.5f, 0.1f);
        v[3].Set(-0.1f, -0.5f, 0.1f);
        h = 0.5f;
        VIMatrix44 m;
        float a = (float)i / 30.0f * 2.0f * 3.1415927f;
        m.RotateY(a);
        m.Mul(v[0], t[0]); v[0] = t[0];
        m.Mul(v[1], t[1]); v[1] = t[1];
        m.Mul(v[2], t[2]); v[2] = t[2];
        m.Mul(v[3], t[3]); v[3] = t[3];
        m.RotateX(sinf(a) * 0.4f);
        m.Mul(v[0], t[0]); v[0] = t[0];
        m.Mul(v[1], t[1]); v[1] = t[1];
        m.Mul(v[2], t[2]); v[2] = t[2];
        m.Mul(v[3], t[3]); v[3] = t[3];
        m.RotateZ(-cosf(a) * 0.4f);
        m.Mul(v[0], sFallEffect[0][i][0]);
        m.Mul(v[1], sFallEffect[0][i][1]);
        m.Mul(v[2], sFallEffect[0][i][2]);
        m.Mul(v[3], sFallEffect[0][i][3]);
        sFallEffect[0][i][0].y += h;
        sFallEffect[0][i][1].y += h;
        sFallEffect[0][i][2].y += h;
        sFallEffect[0][i][3].y += h;

        h = 0.4f;
        v[0].Set(-0.1f, -0.4f, -0.1f);
        v[1].Set(0.1f, -0.4f, -0.1f);
        v[2].Set(0.1f, -0.4f, 0.1f);
        v[3].Set(-0.1f, -0.4f, 0.1f);
        m.RotateX(sinf(a) * h);
        m.Mul(v[0], t[0]); v[0] = t[0];
        m.Mul(v[1], t[1]); v[1] = t[1];
        m.Mul(v[2], t[2]); v[2] = t[2];
        m.Mul(v[3], t[3]); v[3] = t[3];
        m.RotateY(sinf(a) * 0.23f);
        m.Mul(v[0], sFallEffect[1][i][0]);
        m.Mul(v[1], sFallEffect[1][i][1]);
        m.Mul(v[2], sFallEffect[1][i][2]);
        m.Mul(v[3], sFallEffect[1][i][3]);
        sFallEffect[1][i][0].y += h;
        sFallEffect[1][i][1].y += h;
        sFallEffect[1][i][2].y += h;
        sFallEffect[1][i][3].y += h;
    }
    return 0;
}

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

// The camera's world position is carried into the volume's own space through
// the transposed view matrix -- its translation rebuilt by hand, as
// VIEffectVolumeSprite::Raster does -- composed with the model-view.  Two
// gains come out of it: m_fade with distance (full inside three quarters of
// m_p30, gone past it) and m_dayNight from the scene's hour against the
// [m_p34, m_p38] window, with a five-minute ramp either side.
int VIEffectVolume::Render(VICamera &cam, VIMatrix44 &m)
{
    if (m_dirty)
    {
        AllocParticles();
        m_dirty = 0;
    }

    const VIVect3 &cp = cam.m_position;
    VIVect3 p(cp.x, cp.y, cp.z);
    VIVect3 d;
    VIMatrix44 a;
    VIMatrix44 b;
    VIMatrix44 c;

    sceVu0CopyMatrix(a, cam.m_view);
    b.Transpose(a);
    {
        float x = b.m[0].w * b.m[0].x + b.m[1].w * b.m[1].x + b.m[2].w * b.m[2].x;
        float y = b.m[0].w * b.m[0].y + b.m[1].w * b.m[1].y + b.m[2].w * b.m[2].y;
        float z = b.m[0].w * b.m[0].z + b.m[1].w * b.m[1].z + b.m[2].w * b.m[2].z;
        b.m[0].w = b.m[1].w = b.m[2].w = 0.0f;
        b.m[3].x = -x;
        b.m[3].y = -y;
        b.m[3].z = -z;
    }
    sceVu0CopyMatrix(a, *mScene->m_raster->GetModelView());
    c.Mul(a, b);
    d.x = c.m[3].x;
    d.y = c.m[3].y;
    d.z = c.m[3].z;
    p -= d;

    float dist = p.Magnitude();
    float range = m_p30;
    float near = range * 0.75f;

    if (dist < near)
        m_fade = 1.0f;
    else
    {
        if (range < dist)
        {
            m_fade = 0.0f;
            return 0;
        }
        m_fade = 1.0f - (dist - near) / (range * 0.25f);
        m_fade = VIMIN(VIMAX(0.0f, m_fade), 1.0f);
    }

    float span = fabs(m_p34 - m_p38);
    float t0 = m_p34;
    float t1 = m_p38;

    if (span <= 0.25f)
    {
        if (t0 < t1)
            m_dayNight = 0.0f;
        else
            m_dayNight = 1.0f;
    }
    else if (23.75f <= span)
    {
        if (t0 < t1)
            m_dayNight = 1.0f;
        else
            m_dayNight = 0.0f;
    }
    else
    {
        float h = mScene->m_p1FE0;
        float e = t1;

        if (e < t0)
        {
            if (h < e)
                h += 24.0f;
            e += 24.0f;
        }
        if (t0 <= h && h <= e)
            m_dayNight = 1.0f;
        else
        {
            if (e + 0.083333336f < h)
                h -= 24.0f;
            else if (h < t0 - 0.083333336f)
                h += 24.0f;
            if (t0 - 0.083333336f < h && h <= t0)
                m_dayNight = (h - (t0 - 0.083333336f)) / 0.083333336f;
            else if (e <= h && h < e + 0.083333336f)
                m_dayNight = (e + 0.083333336f - h) / 0.083333336f;
            else
                m_dayNight = 0.0f;
            m_dayNight = VIMIN(VIMAX(0.0f, m_dayNight), 1.0f);
        }
    }

    switch (m_type)
    {
    case VIEFFECTVOLUME_NONE:
        break;
    case VIEFFECTVOLUME_LEAVES:
        RenderFallingLeaves(cam, m);
        break;
    case VIEFFECTVOLUME_SHAFTS:
        RenderLightShafts(cam, m);
        break;
    case VIEFFECTVOLUME_FOG:
        RenderGroundFog(cam, m);
        break;
    case VIEFFECTVOLUME_DUST:
        RenderDustStorm(cam, m);
        break;
    case VIEFFECTVOLUME_FLOCK:
        RenderFlock(cam, m);
        break;
    }
    return 0;
}

// A falling leaf, 36 bytes: dead (0), falling (1) or settling (2).  It falls at
// its own scale and swings through a 30-step wobble; once below the floor it
// finishes the swing it is in and then fades.
struct VIFallingLeaf
{
    int     m_state;
    int     m_side;
    float   m_wobble;
    VIVect3 m_pos;
    float   m_floor;
    float   m_scale;
    float   m_life;
};

int VIEffectVolume::ProcessFallingLeaves(float dt)
{
    int i;

    if (m_dayNight == 0.0f)
        return 0;
    if (m_fade == 0.0f)
        return 0;

    VIVect3 s = m_bbox.Size();

    if (m_particles == 0)
        return -1;

    int n = m_numParticles;
    VIFallingLeaf *p = (VIFallingLeaf *)m_particles;

    for (i = 0; i < n; p++, i++)
    {
        switch (p->m_state)
        {
        case 0:
            p->m_state = 1;
            p->m_pos.x = s.x * 0.5f * VIBOIDRAND;
            if (m_reseed)
                p->m_pos.y = s.y * 0.5f * VIBOIDRAND;
            else
                p->m_pos.y = s.y * 0.5f;
            p->m_pos.z = s.z * 0.5f * VIBOIDRAND;
            p->m_scale = (float)((rand() * 131) & 0xFF) / 255.0f * 0.8f + 0.5f;
            if (m_reseed)
                p->m_floor = -s.y * 0.5f;
            else
                p->m_floor = -s.y * 0.5f;
            p->m_life = 1.0f;
            p->m_wobble = (float)(rand() % 30);
            p->m_side = rand() & 1;
            break;
        case 1:
            p->m_pos.y -= p->m_scale * dt;
            p->m_wobble += dt * 30.0f;
            if (30.0f <= p->m_wobble)
                p->m_wobble = 0.0f;
            if (p->m_pos.y < p->m_floor)
                p->m_state = 2;
            break;
        case 2:
            if (1.0f <= p->m_wobble)
            {
                p->m_wobble += dt * 30.0f;
                if (30.0f <= p->m_wobble)
                    p->m_wobble = 0.0f;
            }
            else
                p->m_life -= dt * 0.6f;
            if (p->m_life < 0.0f)
                p->m_state = 0;
            break;
        }
    }
    m_reseed = 0;
    return 0;
}

// A light shaft, 32 bytes: dead (0) or shining (1).  It hangs from the top of
// the box and lives (1 - m_p28) minutes and change.
struct VILightShaft
{
    int     m_state;
    VIVect3 m_pos;
    float   m_width;
    float   m_height;
    float   m_age;
    float   m_life;
};

int VIEffectVolume::ProcessLightShafts(float dt)
{
    int i;

    if (m_dayNight == 0.0f)
        return 0;
    if (m_fade == 0.0f)
        return 0;

    VIVect3 s = m_bbox.Size();

    if (m_particles == 0)
        return -1;

    int n = m_numParticles;
    VILightShaft *p = (VILightShaft *)m_particles;

    for (i = 0; i < n; p++, i++)
    {
        switch (p->m_state)
        {
        case 0:
            p->m_state = 1;
            p->m_pos.x = s.x * 0.5f * VIBOIDRAND;
            p->m_pos.y = s.y * 0.5f;
            p->m_pos.z = s.z * 0.5f * VIBOIDRAND;
            p->m_width = ((float)((rand() * 131) & 0xFF) / 255.0f * 0.4f + 1.5f) * 0.5f;
            p->m_height = s.y * ((float)((rand() * 131) & 0xFF) / 255.0f * 0.2f + 0.8f);
            p->m_age = 0.0f;
            p->m_life = (1.0f - m_p28) * 60.0f + 4.0f +
                        (float)((rand() * 131) & 0xFF) / 255.0f * ((1.0f - m_p28) * 10.0f + 2.0f);
            break;
        case 1:
            p->m_age += dt;
            if (p->m_life <= p->m_age)
                p->m_state = 0;
            break;
        }
    }
    m_reseed = 0;
    return 0;
}

// A puff of ground fog, 36 bytes: dead (0) or lingering (1).  It sits on the
// floor of the box, wears one of the eight rolled fog cards and lives 10 to
// 40 seconds; a reseed scatters the ages so the first frame is not uniform.
struct VIFogPuff
{
    int     m_state;
    VIVect3 m_pos;
    int     m_card;
    float   m_size;
    float   m_height;
    float   m_age;
    float   m_life;
};

int VIEffectVolume::ProcessGroundFog(float dt)
{
    int i;

    if (m_dayNight == 0.0f)
        return 0;
    if (m_fade == 0.0f)
        return 0;

    VIVect3 s = m_bbox.Size();
    VIFogPuff *puffs = (VIFogPuff *)m_particles;

    if (puffs == 0)
        return -1;

    int n = m_numParticles;

    for (i = 0; i < n; i++)
    {
        VIFogPuff *p = &puffs[i];

        switch (p->m_state)
        {
        case 0:
            p->m_state = 1;
            p->m_size = ((float)((rand() * 131) & 0xFF) / 255.0f + 2.0f) * 2.0f;
            p->m_height = s.y * ((float)((rand() * 131) & 0xFF) / 255.0f * 0.15f + 0.85f);
            p->m_pos.x = s.x * 0.5f * VIBOIDRAND;
            p->m_pos.y = 0.0f;
            p->m_pos.z = s.z * 0.5f * VIBOIDRAND;
            p->m_card = rand() & 7;
            p->m_life = (float)((rand() * 131) & 0xFF) / 255.0f * 30.0f + 10.0f;
            if (m_reseed)
                p->m_age = p->m_life * ((float)((rand() * 131) & 0xFF) / 255.0f);
            else
                p->m_age = 0.0f;
            break;
        case 1:
            p->m_age += dt;
            if (p->m_life <= p->m_age)
                p->m_state = 0;
            break;
        }
    }
    m_reseed = 0;
    return 0;
}

// A puff is one of the eight rolled fog cards, stretched to the puff's size,
// carried by the billboard transform and lifted as it swells.
int VIEffectVolume::RenderGroundFog(VICamera &cam, VIMatrix44 &m)
{
    int i;

    if (m_dayNight == 0.0f)
        return 0;
    if (m_fade == 0.0f)
        return 0;

    VIFogPuff *puffs = (VIFogPuff *)m_particles;
    VIRaster *r = mScene->m_raster;

    if (puffs == 0)
        return -1;

    int n = m_numParticles;

    if (m_material != -1)
        r->SetMaterial(m_material);
    else
        r->SetSolidFillUIAVGMaterial();

    VIMatrix44 a;

    PrepareBillboardTransform(cam.m_world, m, &a);

    VIVect3 v[4];
    VIVect3 c[4];
    float k = (1.0f - m_density) * 0.5f + 0.5f;

    for (i = 0; i < n; i++)
    {
        VIFogPuff *p = &puffs[i];

        if (p->m_state)
        {
            VIVect3 *card = sFogCard[p->m_card];
            float f = sinf(p->m_age / p->m_life * 3.1415927f) * k;

            c[0] = card[0];
            c[1] = card[1];
            c[2] = card[2];
            c[3] = card[3];
            c[0].x *= p->m_size;
            c[1].x *= p->m_size;
            c[2].x *= p->m_size;
            c[3].x *= p->m_size;
            c[0].y *= p->m_height;
            c[1].y *= p->m_height;
            c[2].y *= p->m_height;
            c[3].y *= p->m_height;
            a.m[3].x = p->m_pos.x;
            a.m[3].y = p->m_pos.y + f * 1.5f;
            a.m[3].z = p->m_pos.z;
            a.Mul(c[0], v[0]);
            a.Mul(c[1], v[1]);
            a.Mul(c[2], v[2]);
            a.Mul(c[3], v[3]);
            r->Color(VIColor32F(1.0f, 1.0f, 1.0f, m_p28 * f * m_fade * m_dayNight));
            r->BeginTriFan();
            r->UV(VIVect2(0.0f, 0.0f));
            r->Vertex(v[0]);
            r->UV(VIVect2(1.0f, 0.0f));
            r->Vertex(v[1]);
            r->UV(VIVect2(1.0f, 1.0f));
            r->Vertex(v[2]);
            r->UV(VIVect2(0.0f, 1.0f));
            r->Vertex(v[3]);
            r->UV(VIVect2(1.0f, 1.0f));
            r->Vertex(v[2]);
            r->UV(VIVect2(1.0f, 0.0f));
            r->Vertex(v[1]);
            r->EndTriFan();
        }
    }
    return 0;
}

// A mote of dust, 60 bytes: dead (0) or blowing (1).  It is born upwind of the
// box (-x) and blown across it to m_end over its life, sized by m_p28, and
// turns through the sixty rolled dust cards from a random base, `m_rate`
// times a life.
struct VIDustMote
{
    int     m_state;
    VIVect3 m_pos;
    int     m_frame;
    int     m_base;
    float   m_sizeX;
    float   m_sizeY;
    float   m_age;
    float   m_rate;
    float   m_life;
    float   m_invLife;
    VIVect3 m_end;
};

int VIEffectVolume::ProcessDustStorm(float dt)
{
    int i;

    if (m_dayNight == 0.0f)
        return 0;
    if (m_fade == 0.0f)
        return 0;

    VIVect3 s = m_bbox.Size();

    if (m_particles == 0)
        return -1;

    int n = m_numParticles;
    VIDustMote *p = (VIDustMote *)m_particles;

    for (i = 0; i < n; p++, i++)
    {
        switch (p->m_state)
        {
        case 0:
            p->m_state = 1;
            p->m_sizeX = ((float)((rand() * 131) & 0xFF) / 255.0f + 2.0f) * 2.0f * (m_p28 * 0.5f + 0.5f);
            p->m_sizeY = ((float)((rand() * 131) & 0xFF) / 255.0f + 2.0f) * 2.0f * (m_p28 * 0.5f + 0.5f);
            p->m_pos.x = -s.x * 0.5f - s.x * 0.25f * ((float)((rand() * 131) & 0xFF) / 255.0f);
            p->m_pos.y = s.y * 0.5f * VIBOIDRAND;
            p->m_pos.z = s.z * 0.5f * VIBOIDRAND;
            p->m_end.x = s.x * 0.5f + s.x * 0.25f * ((float)((rand() * 131) & 0xFF) / 255.0f);
            p->m_end.y = p->m_pos.y;
            p->m_end.z = p->m_pos.z;
            p->m_base = p->m_frame = rand() & 0x3C;
            p->m_rate = (float)((rand() * 131) & 0xFF) / 255.0f * 0.5f + 0.5f;
            p->m_life = ((float)((rand() * 131) & 0xFF) / 255.0f * 0.7f + 1.0f) * (1.0f - m_p28 + 1.0f);
            p->m_invLife = 1.0f / p->m_life;
            if (m_reseed)
                p->m_age = p->m_life * ((float)((rand() * 131) & 0xFF) / 255.0f);
            else
                p->m_age = 0.0f;
            break;
        case 1:
            p->m_age += dt;
            p->m_frame = ((int)(p->m_age * p->m_invLife * 60.0f * p->m_rate) + p->m_base) % 60;
            if (p->m_life <= p->m_age)
                p->m_state = 0;
            break;
        }
    }
    m_reseed = 0;
    return 0;
}

// A mote is one of the sixty rolled dust cards, sized and carried along its
// track from m_pos to m_end by the billboard transform.
int VIEffectVolume::RenderDustStorm(VICamera &cam, VIMatrix44 &m)
{
    int i;

    if (m_dayNight == 0.0f)
        return 0;
    if (m_fade == 0.0f)
        return 0;

    VIDustMote *motes = (VIDustMote *)m_particles;
    VIRaster *r = mScene->m_raster;

    if (motes == 0)
        return -1;

    int n = m_numParticles;

    if (m_material != -1)
        r->SetMaterial(m_material);
    else
        r->SetSolidFillUIAVGMaterial();

    VIMatrix44 a;

    PrepareBillboardTransform(cam.m_world, m, &a);

    VIVect3 v[4];
    VIVect3 c[4];
    float k = (1.0f - m_density) * 0.5f + 0.5f;
    VIVect3 d;

    for (i = 0; i < n; i++)
    {
        VIDustMote *p = &motes[i];

        if (p->m_state)
        {
            VIVect3 *card = sDustCard[p->m_frame];
            float f = sinf(p->m_age * p->m_invLife * 3.1415927f) * k;

            d = p->m_end;
            d -= p->m_pos;
            d *= p->m_age * p->m_invLife;
            d += p->m_pos;
            c[0] = card[0];
            c[1] = card[1];
            c[2] = card[2];
            c[3] = card[3];
            c[0].x *= p->m_sizeX;
            c[1].x *= p->m_sizeX;
            c[2].x *= p->m_sizeX;
            c[3].x *= p->m_sizeX;
            c[0].y *= p->m_sizeY;
            c[1].y *= p->m_sizeY;
            c[2].y *= p->m_sizeY;
            c[3].y *= p->m_sizeY;
            a.m[3].x = d.x;
            a.m[3].y = d.y;
            a.m[3].z = d.z;
            a.Mul(c[0], v[0]);
            a.Mul(c[1], v[1]);
            a.Mul(c[2], v[2]);
            a.Mul(c[3], v[3]);
            r->Color(VIColor32F(0.0f, 0.0f, 0.0f, m_p28 * f * m_fade * m_dayNight));
            r->Normal(VIVect3(0.0f, 1.0f, 0.0f));
            r->BeginTriFan();
            r->UV(VIVect2(0.0f, 0.0f));
            r->Vertex(v[0]);
            r->UV(VIVect2(1.0f, 0.0f));
            r->Vertex(v[1]);
            r->UV(VIVect2(1.0f, 1.0f));
            r->Vertex(v[2]);
            r->UV(VIVect2(0.0f, 1.0f));
            r->Vertex(v[3]);
            r->UV(VIVect2(1.0f, 1.0f));
            r->Vertex(v[2]);
            r->UV(VIVect2(1.0f, 0.0f));
            r->Vertex(v[1]);
            r->EndTriFan();
        }
    }
    return 0;
}

// A shaft is a quad turned about Y to face the camera's heading, carried
// through the billboard transform and lit by its age.  With bit 1 of m_flags
// the shafts follow the sun: they only shine between 09:00 and 15:00, fading
// in and out over the first and last hour, and lean -1 .. +1 across the day.
int VIEffectVolume::RenderLightShafts(VICamera &cam, VIMatrix44 &m)
{
    int i;

    if (m_dayNight == 0.0f)
        return 0;
    if (m_fade == 0.0f)
        return 0;

    float lean = 0.0f;
    float h = mScene->m_p1FE0;

    if (m_flags & 2)
    {
        if (h < 9.0f)
            lean = -1.0f;
        else if (h < 15.0f)
            lean = (h - 9.0f) / 3.0f + -1.0f;
        else
            lean = 1.0f;

        if (h < 9.0f)
            m_dayNight = 0.0f;
        else if (h < 10.0f)
            m_dayNight = h - 9.0f;
        else if (h < 14.0f)
            m_dayNight = 1.0f;
        else if (h < 15.0f)
            m_dayNight = 1.0f - (h - 14.0f);
        else
            m_dayNight = 0.0f;
        if (m_dayNight == 0.0f)
            return 0;
    }

    VIRaster *r = mScene->m_raster;
    VILightShaft *shafts = (VILightShaft *)m_particles;

    if (shafts == 0)
        return -1;

    int n = m_numParticles;

    if (m_material != -1)
        r->SetMaterial(m_material);
    else
        r->SetSolidFillUIAVGMaterial();

    VIVect3 dir;
    VIMatrix44 a;
    VIMatrix44 b;

    dir = cam.m_dir;
    b.Identity();
    if (0.0f < dir.x)
        b.RotateY(atanf(dir.z / -dir.x) - 1.5707964f);
    else
        b.RotateY(atanf(dir.z / -dir.x) + 1.5707964f);

    VIMatrix44 c = PrepareBillboardTransform(b, m, &a);
    VIVect3 t;

    if (m_flags & 2)
        c.Mul(VIVect3(lean, 0.0f, 0.0f), t);

    VIVect3 v[4];
    float alpha = m_fade * m_dayNight * ((1.0f - m_density) * 0.5f + 0.5f);
    VILightShaft *p = shafts;

    for (i = 0; i < n; i++, p++)
    {
        if (p->m_state)
        {
            a.Mul(VIVect3(-p->m_width, 0.0f, 0.0f), v[0]);
            a.Mul(VIVect3(p->m_width, 0.0f, 0.0f), v[1]);
            a.Mul(VIVect3(p->m_width, -p->m_height, 0.0f), v[2]);
            a.Mul(VIVect3(-p->m_width, -p->m_height, 0.0f), v[3]);
            v[0] += p->m_pos;
            v[1] += p->m_pos;
            v[2] += p->m_pos;
            v[3] += p->m_pos;
            if (m_flags & 2)
            {
                v[2] += t * p->m_height;
                v[3] += t * p->m_height;
            }
            r->Color(VIColor32F(1.0f, 1.0f, 1.0f,
                                sinf(p->m_age / p->m_life * 3.1415927f) * alpha));
            r->BeginTriFan();
            r->UV(VIVect2(0.0f, 0.0f));
            r->Vertex(v[0]);
            r->UV(VIVect2(1.0f, 0.0f));
            r->Vertex(v[1]);
            r->UV(VIVect2(1.0f, 1.0f));
            r->Vertex(v[2]);
            r->UV(VIVect2(0.0f, 1.0f));
            r->Vertex(v[3]);
            r->UV(VIVect2(1.0f, 1.0f));
            r->Vertex(v[2]);
            r->UV(VIVect2(1.0f, 0.0f));
            r->Vertex(v[1]);
            r->EndTriFan();
        }
    }
    return 0;
}

// The model-view with its translation dropped and each row normalised -- the
// rotation alone -- composed after `a` into *out, and handed back.  The
// second matrix is never used.
VIMatrix44 VIEffectVolume::PrepareBillboardTransform(const VIMatrix44 &a, const VIMatrix44 &b,
                                                     VIMatrix44 *out)
{
    VIMatrix44 m(b);

    m.m[3].x = m.m[3].y = m.m[3].z = 0.0f;
    VIVect3 s;
    s.x = 1.0f / sqrtf(m.m[0].x * m.m[0].x + m.m[0].y * m.m[0].y + m.m[0].z * m.m[0].z);
    s.y = 1.0f / sqrtf(m.m[1].x * m.m[1].x + m.m[1].y * m.m[1].y + m.m[1].z * m.m[1].z);
    s.z = 1.0f / sqrtf(m.m[2].x * m.m[2].x + m.m[2].y * m.m[2].y + m.m[2].z * m.m[2].z);
    m.m[0].x *= s.x;
    m.m[0].y *= s.x;
    m.m[0].z *= s.x;
    m.m[1].x *= s.y;
    m.m[1].y *= s.y;
    m.m[1].z *= s.y;
    m.m[2].x *= s.z;
    m.m[2].y *= s.z;
    m.m[2].z *= s.z;

    VIMatrix44 t;

    out->Mul(a, m);
    return m;
}

// A leaf is one of the sixty baked cards in sFallEffect, picked by its swing
// side and wobble step and moved to the leaf, drawn twice so both faces show.
// Only m_dayNight of the leaves are drawn at all.
int VIEffectVolume::RenderFallingLeaves(VICamera &cam, VIMatrix44 &m)
{
    int i;

    if (m_dayNight == 0.0f)
        return 0;
    if (m_fade == 0.0f)
        return 0;

    VIFallingLeaf *leaves = (VIFallingLeaf *)m_particles;
    VIRaster *r = mScene->m_raster;

    if (leaves == 0)
        return -1;

    int n = (int)(m_dayNight * (float)m_numParticles);

    if (m_material != -1)
        r->SetMaterial(m_material);
    else
        r->SetSolidFillUIAVGMaterial();

    VIVect3 v[4];

    for (i = 0; i < n; i++)
    {
        VIFallingLeaf *p = &leaves[i];

        if (p->m_state)
        {
            VIVect3 *c = sFallEffect[p->m_side][(int)p->m_wobble];

            v[0] = c[0];
            v[1] = c[1];
            v[2] = c[2];
            v[3] = c[3];
            v[0] += p->m_pos;
            v[1] += p->m_pos;
            v[2] += p->m_pos;
            v[3] += p->m_pos;
            r->Color(VIColor32F(0.0f, 0.0f, 0.0f, p->m_life * m_fade));
            r->Normal(VIVect3(0.0f, 1.0f, 0.0f));
            r->BeginTriFan();
            r->UV(VIVect2(0.0f, 0.0f));
            r->Vertex(v[0]);
            r->UV(VIVect2(1.0f, 0.0f));
            r->Vertex(v[1]);
            r->UV(VIVect2(1.0f, 1.0f));
            r->Vertex(v[2]);
            r->UV(VIVect2(0.0f, 1.0f));
            r->Vertex(v[3]);
            r->UV(VIVect2(1.0f, 1.0f));
            r->Vertex(v[2]);
            r->UV(VIVect2(1.0f, 0.0f));
            r->Vertex(v[1]);
            r->EndTriFan();
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

int VIEffectVolume::RenderFlock(VICamera &cam, VIMatrix44 &m)
{
    int i;

    if (m_dayNight == 0.0f)
        return 0;
    if (m_fade == 0.0f)
        return 0;
    VIBoid *b = (VIBoid *)m_particles;

    for (i = 0; i < m_numParticles; i++)
        b[i].Render(*mScene, cam);
    return 0;
}

// Reynolds' boids.  Every pair closer than two metres pushes apart (by the
// inverse square), matches heading and pulls together; each boid then also
// heads for the flock's waypoint, which moves every 3 to 12 seconds.  The sum
// is flattened to the horizontal and steered towards by banking, the climb
// follows its sign, and the speed follows the pitch.
int VIEffectVolume::ProcessFlock(float dt)
{
    int i, j;

    if (m_dayNight == 0.0f || m_fade == 0.0f)
        return 0;

    VIBoid *b = (VIBoid *)m_particles;

    if (b == 0)
        return -1;

    if (m_sprite != -1 && b[0].m_sprite == -1)
    {
        for (i = 0; i < m_numParticles; i++)
            b[i].m_sprite = mScene->CopySprite(m_sprite, 0);
    }

    if (m_p6C <= 0.0f)
    {
        m_p6C = (float)(rand() / 131 % 9) + 3.0f;
        VIVect3 s = m_bbox.Size();
        m_wind.x = s.x * 0.5f * VIBOIDRAND;
        m_wind.y = s.y * 0.5f * VIBOIDRAND;
        m_wind.z = s.z * 0.5f * VIBOIDRAND;
    }

    for (i = 0; i < m_numParticles; i++)
    {
        b[i].m_sep.Zero();
        b[i].m_align.Zero();
        b[i].m_coh.Zero();
        b[i].m_count = 0;
        b[i].m_goal.Zero();
    }

    for (i = 0; i < m_numParticles; i++)
    {
        for (j = i + 1; j < m_numParticles; j++)
        {
            VIVect3 d = b[i].m_pos - b[j].m_pos;
            float dist = d.Magnitude();

            if (dist < 2.0f && 0.0f < dist)
            {
                float inv = 1.0f / dist;
                VIVect3 t = d * (1.0f / (dist * dist));

                b[i].m_sep += t;
                b[j].m_sep -= t;
                t = b[j].m_dir;
                t *= inv;
                b[i].m_align += t;
                t = b[i].m_dir;
                t *= inv;
                b[j].m_align += t;
                b[i].m_coh += b[j].m_pos;
                b[j].m_coh += b[i].m_pos;
                b[i].m_count++;
                b[j].m_count++;
            }
        }
        if (b[i].m_count)
        {
            b[i].m_coh *= 1.0f / (float)b[i].m_count;
            VIVect3 d = b[i].m_coh - b[i].m_pos;
            float mag = d.Magnitude();

            if (0.0f < mag)
                b[i].m_coh = d * (1.0f / mag);
            else
                b[i].m_coh.Zero();

            b[i].m_align *= 1.0f / (float)b[i].m_count;
            mag = b[i].m_align.Magnitude();
            if (0.0f < mag)
            {
                float inv = 1.0f / mag;
                b[i].m_align *= inv;
                d = b[i].m_align - b[i].m_dir;
                b[i].m_align = d * inv;
            }
        }
        b[i].m_goal = m_wind - b[i].m_pos;
    }

    for (i = 0; i < m_numParticles; i++)
    {
        VIVect3 a;
        VIVect3 h;
        VIVect3 c;

        a += b[i].m_sep;
        a += b[i].m_align;
        a += b[i].m_coh;
        a += b[i].m_goal;
        h.x = a.x;
        h.y = 0.0f;
        h.z = a.z;
        c = b[i].m_dir;
        if (0.0f < h.Magnitude())
            h.Normalize();

        float dot = h.x * c.x + h.y * c.y + h.z * c.z;

        h = Cross(h, c);

        float turn = (1.0f - dot) * 0.5f * 0.0067f;

        if (0.05f < h.y)
            b[i].m_p10 -= turn;
        else if (h.y < -0.05f)
            b[i].m_p10 += turn;
        b[i].m_p10 *= 0.94f;
        b[i].m_hpr.z = -b[i].m_p10 * 9.0f;
        b[i].m_hpr.x += (b[i].m_p10 + b[i].m_hpr.y * 0.01f) * b[i].m_pA4 * dt * 30.0f;
        if (0.0f < a.y)
            b[i].m_hpr.y -= dt * 0.25f * b[i].m_pA8;
        else
            b[i].m_hpr.y += dt * 0.25f * b[i].m_pA8;
        b[i].m_hpr.y = VIMIN(VIMAX(-3.1415927f * 0.15f, b[i].m_hpr.y), 3.1415927f * 0.15f);
        VIVect3 v = b[i].m_dir;

        v *= dt * (b[i].m_hpr.y * 2.4f + 5.0f) * 0.6f;
        b[i].m_pos += v;
        if (b[i].m_hpr.y < 0.0f)
        {
            if (b[i].m_pA0 != 0)
            {
                b[i].m_pA0 = 0;
                ((VICSprite *)mScene->Sprite(b[i].m_sprite))->SetLocomotion((VICSpriteAnimID)1, *mScene);
            }
        }
        else if (b[i].m_pA0 != 1)
        {
            b[i].m_pA0 = 1;
            ((VICSprite *)mScene->Sprite(b[i].m_sprite))->SetLocomotion((VICSpriteAnimID)0, *mScene);
        }
        b[i].Apply(dt);
    }
    m_p6C -= dt;
    return 0;
}
