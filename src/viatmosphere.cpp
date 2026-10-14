// VIAtmosphere -- SUPPORT 0x00416888 .. 0x0041C7C8, its own TU.  Phase 55.
// The sky and the weather.
#include "vi.h"

extern "C" int rand(void);

static inline VIVect3 operator*(const VIVect3 &v, float f)
{
    return VIVect3(v.x * f, v.y * f, v.z * f);
}

// Out-parameter forms of `o = v * f` and `o = u + v` (phase 61).  VIVect3 has
// a user operator=, so `c = cam.m_dir * 5.0f` builds the by-value result in a
// stack temporary and copies it; the target has no temporary -- the products
// go straight into `c`.  All loads come before the stores.
static inline void ScaleTo(VIVect3 &o, const VIVect3 &v, float f)
{
    float x = v.x * f;
    float y = v.y * f;
    float z = v.z * f;
    o.x = x;
    o.y = y;
    o.z = z;
}

static inline void AddTo(VIVect3 &o, const VIVect3 &u, const VIVect3 &v)
{
    float x = u.x + v.x;
    float y = u.y + v.y;
    float z = u.z + v.z;
    o.x = x;
    o.y = y;
    o.z = z;
}

static inline VIVect3 operator+(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline VIVect3 operator-(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x - b.x, a.y - b.y, a.z - b.z);
}

extern "C" float sinf(float);

// Two draws, differenced and scaled by 2^-31: a signed unit random.
#define VIRAND (((float)rand() - (float)rand()) * 4.65661287e-10f)
extern "C" float cosf(float);
extern "C" unsigned int VIHashResourceID(const char *);

// The scene filter the weather rays go through: it accepts everything.  Its
// vtable and typeinfo are the TU's first and last symbols, which is what puts
// this one-line override at the top of the file.
int WeatherFilter::FilterActor(VIScene &sc, int actor)
{
    return 0;
}

// Noise baked into the module: Init reads the weather field's start
// positions out of 200 triples and the star field's angles out of 128 more.
static unsigned char sNoise[600] =
{
    0x47, 0xEB, 0xAE, 0x15, 0xA5, 0xA4, 0xD0, 0xA6, 0x32, 0xAF, 0x34, 0x04,
    0x62, 0xEB, 0x0F, 0x70, 0xF2, 0x87, 0x3D, 0x1F, 0xA5, 0xCA, 0x45, 0x49,
    0x6B, 0xBA, 0xA6, 0xF6, 0xEF, 0x89, 0x84, 0xC1, 0x88, 0xE9, 0x17, 0x27,
    0x49, 0x5C, 0xBE, 0xC6, 0x64, 0x53, 0xA3, 0x15, 0x90, 0xEC, 0x03, 0x90,
    0xBA, 0x20, 0x85, 0x3C, 0x8B, 0xAA, 0x39, 0x9A, 0xED, 0x7F, 0xD7, 0x23,
    0xBC, 0x8D, 0x54, 0xC1, 0xBD, 0x94, 0x71, 0x3F, 0xCB, 0x5F, 0x5C, 0x9B,
    0x96, 0x52, 0xB4, 0x64, 0xF1, 0x16, 0xE6, 0x70, 0x11, 0x43, 0xC7, 0xB6,
    0x8D, 0x0A, 0x6F, 0x66, 0x98, 0x10, 0x57, 0x3A, 0x17, 0xE6, 0xB4, 0x32,
    0x36, 0xC0, 0x46, 0xB1, 0xBC, 0x5A, 0x43, 0x27, 0x6E, 0x8B, 0x75, 0x6E,
    0x12, 0x37, 0xB3, 0x04, 0xE4, 0xF5, 0xDB, 0x6C, 0x9B, 0xB8, 0x48, 0x3B,
    0x81, 0x54, 0xB7, 0xF6, 0x87, 0x0E, 0x1F, 0x6A, 0x32, 0x91, 0x23, 0x74,
    0x28, 0xAB, 0xEC, 0x1C, 0x1C, 0xF1, 0x80, 0x94, 0x52, 0x03, 0xEE, 0xB9,
    0x80, 0xC4, 0x91, 0xFD, 0xE8, 0x06, 0x88, 0xE8, 0xF8, 0x41, 0xA0, 0xE1,
    0x65, 0xFD, 0x35, 0x54, 0x37, 0x73, 0x34, 0x05, 0x72, 0x6D, 0x60, 0xBF,
    0xD2, 0x22, 0x2C, 0x15, 0xAE, 0xCA, 0x05, 0x59, 0x98, 0x11, 0xBF, 0x01,
    0x7E, 0x1F, 0xB2, 0x7E, 0x6F, 0xA8, 0x26, 0x2A, 0xD2, 0x2B, 0xB5, 0x2E,
    0xB5, 0x86, 0xFE, 0x19, 0x21, 0x43, 0x4F, 0xD4, 0xE3, 0x58, 0x2C, 0xE6,
    0xF0, 0x25, 0xB0, 0x6D, 0xDE, 0xE9, 0x23, 0x3D, 0xC4, 0x20, 0x32, 0xE4,
    0x1E, 0x42, 0xB3, 0x2A, 0x66, 0x37, 0x1F, 0xD3, 0x61, 0x16, 0x7B, 0xFF,
    0xBF, 0x02, 0x1B, 0xF7, 0x01, 0x36, 0xE6, 0x67, 0x5A, 0x92, 0x6C, 0x3C,
    0x64, 0x55, 0x67, 0xBE, 0x52, 0x1A, 0x34, 0x6B, 0x36, 0x05, 0xFE, 0xD1,
    0xBC, 0x2E, 0xE5, 0x0B, 0x5F, 0x8E, 0xB8, 0x18, 0x41, 0xFD, 0x1E, 0xC3,
    0x7E, 0x2B, 0x67, 0xD6, 0x48, 0xAD, 0x9B, 0xE8, 0xB8, 0x94, 0xEE, 0xD6,
    0x10, 0xA4, 0x98, 0xA8, 0x90, 0x0A, 0x70, 0xF4, 0x7F, 0x41, 0x91, 0xD6,
    0x67, 0x3E, 0xA6, 0xA7, 0x58, 0x30, 0x3B, 0x6E, 0x7F, 0x42, 0x47, 0xA9,
    0x39, 0x58, 0xB7, 0x87, 0x9E, 0xB2, 0xAA, 0xFD, 0x42, 0x98, 0xFC, 0xF2,
    0xFE, 0x4A, 0x4E, 0x1F, 0x62, 0x4F, 0xCA, 0x85, 0xF3, 0x6E, 0x93, 0x2D,
    0xE0, 0xDE, 0x3D, 0x7E, 0x76, 0x7E, 0xB3, 0x5A, 0x22, 0x83, 0x22, 0x43,
    0xA5, 0x93, 0xCD, 0x78, 0xD5, 0x3F, 0xCB, 0x45, 0x95, 0x33, 0x6C, 0x97,
    0x00, 0x18, 0x3A, 0xA6, 0x3C, 0xC4, 0x3A, 0x07, 0x7E, 0x1D, 0xD3, 0xE5,
    0x5D, 0x5D, 0x0A, 0x9E, 0x4D, 0x36, 0xEC, 0xD6, 0x69, 0x44, 0xD0, 0x30,
    0xBC, 0x7D, 0xF8, 0x89, 0xEE, 0xBE, 0x52, 0xA7, 0xBE, 0xBF, 0xE6, 0xCC,
    0x99, 0xC5, 0xC6, 0x85, 0xC3, 0x98, 0xAF, 0x73, 0x8C, 0x4A, 0x66, 0x37,
    0x9A, 0xC0, 0x40, 0xBE, 0xFC, 0x4E, 0x5C, 0x5C, 0x31, 0x79, 0x30, 0x05,
    0xA7, 0x77, 0x37, 0xC6, 0xEE, 0x9C, 0x11, 0x77, 0xA6, 0x80, 0x5A, 0x9D,
    0x55, 0xB4, 0x67, 0x2F, 0x50, 0x9C, 0xA3, 0x4C, 0x7E, 0x9B, 0x77, 0x4D,
    0x3F, 0xF6, 0x7F, 0xB5, 0x26, 0xC0, 0x0B, 0xDC, 0x2E, 0xE2, 0x64, 0xA7,
    0x58, 0xEF, 0xDC, 0x1F, 0x87, 0x58, 0x8B, 0x43, 0xFC, 0xE7, 0x7C, 0x90,
    0x2C, 0x6C, 0x8D, 0x1A, 0xE2, 0xBA, 0x5C, 0x8C, 0xEC, 0x84, 0xD8, 0x0F,
    0xFF, 0x8F, 0x06, 0x82, 0xAF, 0x33, 0x92, 0x99, 0x3F, 0xAA, 0xCB, 0x5C,
    0xBA, 0x55, 0xE9, 0xE0, 0x07, 0xD5, 0xB0, 0x72, 0x0B, 0x34, 0xC7, 0x48,
    0xD1, 0xBE, 0xE0, 0xBA, 0xCB, 0xA9, 0x3C, 0x4C, 0x1F, 0x58, 0xF8, 0xDC,
    0x2D, 0xDA, 0x97, 0x6A, 0x42, 0x59, 0xC0, 0x43, 0x24, 0x64, 0x6C, 0x81,
    0xC7, 0xDE, 0xEA, 0xEF, 0x92, 0x83, 0x1A, 0x60, 0x4E, 0x57, 0x54, 0x7D,
    0xEF, 0xBB, 0x06, 0xB4, 0x94, 0x79, 0xF9, 0x43, 0x20, 0x46, 0x6C, 0xDE,
    0x59, 0xCD, 0x2D, 0x83, 0x1E, 0x63, 0x38, 0x1F, 0x13, 0xB6, 0xD0, 0x0F,
    0x75, 0x17, 0xC3, 0x23, 0x5F, 0x63, 0xB4, 0xAE, 0x63, 0x68, 0x8D, 0xBC,
    0xB5, 0x3F, 0x5D, 0x34, 0x6E, 0xE2, 0x7E, 0xAD, 0x06, 0xD2, 0x34, 0x4B,
    0xCD, 0x8F, 0x59, 0x15, 0xF4, 0xE4, 0x9C, 0x6B, 0x09, 0x10, 0x4A, 0xDF,
    0xAD, 0x54, 0x2E, 0xB2, 0xF6, 0x12, 0xEE, 0xF8, 0x6C, 0xAF, 0x2D, 0x88,
};

static unsigned char sStarNoise[384] =
{
    0xF9, 0xF6, 0xA3, 0x48, 0xC7, 0xDB, 0x76, 0x15, 0x91, 0x35, 0x67, 0xE7,
    0xDA, 0xA9, 0x1A, 0x55, 0x02, 0x62, 0x81, 0x22, 0x71, 0x3E, 0x62, 0x4E,
    0x58, 0x46, 0x7B, 0xBF, 0x5D, 0x8F, 0x75, 0xA3, 0x9B, 0xC8, 0xF1, 0x51,
    0x79, 0xDA, 0x39, 0x60, 0xAF, 0x22, 0x78, 0xAB, 0x1B, 0xFE, 0xA8, 0x94,
    0x2D, 0xFD, 0x18, 0x48, 0xC6, 0xC3, 0x0D, 0x88, 0x5F, 0x19, 0x08, 0x53,
    0x15, 0x9C, 0x9C, 0x9B, 0x69, 0x41, 0xFA, 0x45, 0xD7, 0xFE, 0x21, 0xF0,
    0xD6, 0xFE, 0x0C, 0x07, 0x30, 0x41, 0x59, 0x09, 0xA4, 0x0A, 0xDB, 0xF4,
    0x01, 0x70, 0xC2, 0x16, 0x49, 0x3B, 0x1B, 0x6B, 0x08, 0xD1, 0x2A, 0xC1,
    0x15, 0x16, 0xD3, 0xEA, 0x74, 0x92, 0xAC, 0x10, 0xDC, 0x98, 0x20, 0xB5,
    0x12, 0xC3, 0x78, 0xC0, 0x86, 0x67, 0x92, 0x13, 0x9B, 0x71, 0x28, 0x9B,
    0xED, 0x9D, 0x06, 0x84, 0x32, 0x98, 0x77, 0x5E, 0xFB, 0x19, 0x84, 0xD7,
    0xED, 0x73, 0xD9, 0x18, 0x40, 0x78, 0x20, 0x73, 0xC1, 0xE1, 0xAA, 0xD3,
    0x6A, 0x70, 0x71, 0xA6, 0xF9, 0xE6, 0x89, 0xD3, 0xA9, 0xBB, 0xDB, 0xD1,
    0x1C, 0x40, 0x52, 0x2F, 0xBE, 0x93, 0xEC, 0xB0, 0xD0, 0x15, 0x93, 0xAD,
    0x7A, 0x0A, 0x28, 0xFA, 0xA1, 0xA3, 0xD6, 0x42, 0x58, 0x6B, 0x06, 0xE6,
    0xB7, 0x1D, 0x75, 0x69, 0x8E, 0xFB, 0xB9, 0x43, 0x19, 0xD6, 0x37, 0xDC,
    0x20, 0x34, 0x53, 0xDD, 0xFC, 0x66, 0x4C, 0x30, 0xD3, 0x37, 0xAF, 0xF2,
    0xF7, 0x90, 0xF1, 0x71, 0x52, 0x78, 0xB7, 0x2D, 0xF8, 0x69, 0x1A, 0xD5,
    0xE2, 0x05, 0x03, 0xEF, 0xB7, 0xE1, 0x24, 0xE3, 0x41, 0x08, 0xEC, 0xF9,
    0x4F, 0x82, 0x63, 0xB5, 0x4F, 0x53, 0x03, 0xEB, 0x28, 0xE8, 0xAD, 0x4F,
    0xC9, 0x84, 0x5F, 0xA0, 0xD8, 0xDE, 0xB8, 0x25, 0x20, 0x51, 0x54, 0x45,
    0x3D, 0xD8, 0x25, 0x6A, 0x94, 0x38, 0x27, 0x3F, 0x4C, 0x6F, 0x28, 0xB2,
    0xED, 0x0C, 0x23, 0xA0, 0xF7, 0x67, 0x1B, 0xC7, 0x69, 0x3E, 0x49, 0xC2,
    0x48, 0x05, 0x6A, 0xCB, 0xEA, 0x8C, 0xBD, 0xC9, 0x07, 0x54, 0x6A, 0xDD,
    0xB3, 0x05, 0xCF, 0x8F, 0x35, 0xC3, 0x82, 0xE8, 0xD7, 0x51, 0x5B, 0x63,
    0x55, 0xD7, 0xFD, 0x7A, 0x94, 0xBC, 0xF0, 0xC9, 0x59, 0x6C, 0x09, 0x63,
    0xD9, 0xB7, 0x1D, 0x70, 0xF9, 0x22, 0xCA, 0x5D, 0xFF, 0x53, 0x78, 0xC5,
    0x74, 0xCC, 0x68, 0xA5, 0xC2, 0x24, 0x17, 0xC8, 0x7F, 0x27, 0x8B, 0x43,
    0x22, 0x7F, 0x0F, 0xE2, 0x58, 0x4E, 0x01, 0xF2, 0xE4, 0xB8, 0x18, 0x60,
    0xF5, 0x33, 0x32, 0xFE, 0x1C, 0xBB, 0xAB, 0x5C, 0x3D, 0xE8, 0x2E, 0x87,
    0xCF, 0xD3, 0x75, 0x0A, 0x60, 0xBC, 0xDC, 0xED, 0xB6, 0x07, 0xD7, 0x44,
    0x35, 0x08, 0x52, 0xBE, 0xD2, 0x69, 0xC6, 0xB2, 0x48, 0x9F, 0xF3, 0x0F,
};

VIAtmosphere::VIAtmosphere()
{
    m_initialised = 0;
    m_particles = 0;
    m_splashNodes = 0;
    m_fireballNodes = 0;
}

VIAtmosphere::~VIAtmosphere()
{
}

int VIAtmosphere::Init(long time, VIRaster &r, int n)
{
    int i;

    m_range = 10.0f;
    m_numParticles = 200;
    m_p5D0 = m_p5C8 = time;
    for (i = 0; i < 8; i++)
        m_p4AC[i] = 1.0f / 30.0f;
    m_camera.Zero();
    SetEffectMix(0.0f);
    SetIntensity(0.0f);
    m_lavastorm = 0.0f;
    SetLavastormIntensity(0.0f);
    m_overcast = 1;
    m_p88[0] = 0.1f;
    m_particleScale = 1.0f;
    m_p60C = 1;
    m_p610 = 1;
    m_p61C = 1.0f;
    m_p614 = 1;
    m_p88[1] = 0.1f;
    m_skyScale = 1.0f;
    m_p84 = 0.0f;
    m_p5C8 = 0;
    m_effectMix = 0.0f;
    m_skySprite = -1;
    m_p5E0 = -1;
    m_p5E4 = -1;
    m_p5E8 = -1;
    m_p5EC = 0;
    m_p5F0 = 0;
    m_p5F4 = -1;
    m_p5F8 = -1;
    m_p608 = 0.0f;
    m_p5FC = 0;
    m_p600 = 0;
    m_p604 = 0;
    m_sun = -1;
    m_corona = -1;
    m_moon = -1;
    m_starTex = -1;
    m_starMat = -1;
    m_pF50 = -1;
    m_pF24 = 0.0f;
    m_pF28 = 0.0f;
    m_pF2C = 0.0f;
    m_particles = 0;
    m_splashNodes = 0;
    m_fireballNodes = 0;
    m_p1188 = -1;
    m_p1190 = -1;
    m_p1194 = -1;
    m_p1198 = 0.0f;
    m_p119C = 0.0f;
    m_p11A0 = 0.0f;
    m_p618 = 0;
    m_particles = new VIWeatherParticle[m_numParticles];

    for (i = 0; i < 64; i++)
    {
        float a = (float)i * 6.2831855f / 64.0f * 2.0f;

        m_pA8[i].x = sinf(a);
        m_pA8[i].y = cosf(a);
        m_pA8[i].z = cosf(a);
        m_p3A8[i] = (float)((rand() * 131) & 0xFF) / 255.0f + 1.0f;
    }

    for (i = 0; i < 2; i++)
    {
        m_mat[i] = r.CreateMaterial(0);
        if (m_mat[i] < 0)
            return -1;
        VIMaterial *m = r.Material(m_mat[i]);
        m->Init(1);
        m->SetLayerFillType(0, VIMaterialFillTypeTexture);
        m->SetLayerModulate(0, 1);
        m->SetLayerBlendMode(0, VIAlphaBlendMode3);
        m->SetZWrite(0);
        m->SetZTest(1);
        m->SetDither(0);
    }

    m_splashMat = r.CreateMaterial(0);
    if (m_splashMat < 0)
        return -1;
    VIMaterial *m = r.Material(m_splashMat);
    m->Init(1);
    m->SetLayerFillType(0, VIMaterialFillTypeTexture);
    m->SetLayerModulate(0, 1);
    m->SetLayerBlendMode(0, VIAlphaBlendMode1);
    m->SetZWrite(0);
    m->SetZTest(1);
    m->SetDither(0);

    m_starMat = r.CreateMaterial(0);
    if (m_starMat < 0)
        return -1;
    m = r.Material(m_starMat);
    m->Init(1);
    m->SetLayerFillType(0, VIMaterialFillTypeTexture);
    m->SetLayerModulate(0, 1);
    m->SetLayerBlendMode(0, VIAlphaBlendMode1);
    m->SetLayerTextureLODBias(0, 1.0f);
    m->SetZWrite(0);
    m->SetZTest(0);
    m->SetDither(0);

    m_pF50 = r.CreateMaterial(0);
    if (m_pF50 < 0)
        return -1;
    m = r.Material(m_pF50);
    m->Init(1);
    m->SetLayerFillType(0, VIMaterialFillTypeTexture);
    m->SetLayerModulate(0, 1);
    m->SetLayerBlendMode(0, VIAlphaBlendMode1);
    m->SetLayerTextureLODBias(0, 1.0f);
    m->SetZWrite(0);
    m->SetZTest(0);
    m->SetDither(0);

    for (i = 0; i < m_numParticles; i++)
    {
        float a = VIRAND * 0.08f + 0.1f;
        float x = (float)sNoise[i * 3 + 0] / 255.0f;
        float y = (float)sNoise[i * 3 + 1] / 255.0f;
        float z = (float)sNoise[i * 3 + 2] / 255.0f;

        m_particles[i].m_location.x = x * m_range - m_range * 0.5f;
        m_particles[i].m_location.y = y * m_range - m_range * 0.5f;
        m_particles[i].m_location.z = z * m_range - m_range * 0.5f;
        m_particles[i].m_p20 = 0;
        m_particles[i].m_color.Set(1.0f, 1.0f, 1.0f, a);
    }

    for (i = 0; i < 128; i++)
    {
        VIMatrix44 s;

        s.RotateHPR(VIVect3((float)(128 - sStarNoise[i * 3 + 0]) / 128.0f * 3.1415927f * 2.3f,
                            (float)(128 - sStarNoise[i * 3 + 1]) / 128.0f * 0.49f * 3.1415927f,
                            (float)(128 - sStarNoise[i * 3 + 2]) / 128.0f * 0.49f * 3.1415927f));
        s.Mul(VIVect4(0.0f, 100.0f, 0.0f, 1.0f), m_stars[i]);
        m_stars[i].w = (float)((rand() / 131) & 0xFF) / 255.0f * 0.5f + 0.3f;
        m_pE20[i] = (rand() / 131) & 0x1F;
    }

    for (i = 0; i < 32; i++)
        m_pEA0[i] = (sinf((float)i / 16.0f * 3.1415927f) * 0.5f + 0.5f) * 0.4f + 0.5f;

    m_splashNodes = (char *)new VISplash[m_numParticles];
    m_numSplashes = 0;
    m_splashList.Init();
    m_splashFree.Init();
    for (i = 0; i < m_numParticles; i++)
        m_splashFree.AddTail((VILNode *)(m_splashNodes + i * 36));

    m_fireballNodes = new VIFireball[32];
    m_numFireballs = 0;
    m_fireballList.Init();
    m_fireballFree.Init();
    for (i = 0; i < 32; i++)
        m_fireballFree.AddTail((VILNode *)((char *)m_fireballNodes + (i << 11)));

    m_p88[0] = 0.095f;
    m_p98[0] = 0.8f;
    m_p90[0] = 0.0f;
    m_p50[0].Set(0.0025f, 0.008f, 0.0025f);
    m_p68[0].Set(0.0f, -0.21042f, 0.0f);
    m_p88[1] = 0.05f;
    m_p90[1] = 0.0f;
    m_p98[1] = 0.0f;
    m_p50[1].Set(0.0106f, 0.01723f, 0.0106f);
    m_p68[1].Set(0.0f, -0.02199f, 0.0f);
    return 0;
}

// One tick of the sky and weather cross-fades.  m_p608 runs 0 -> 1 over 30
// ticks of m_p4A8 while a change is under way: the sun, corona and moon fade
// with it, the sky dome blends a second layer in and out, rain and snow mix by
// state (0 none, 1 rain, 2 snow) and the intensity follows the seven-step
// table.  At 1 the pending state becomes the current one.  Rain sounds and
// thunder are kept going whenever there is rain to hear.
void VIAtmosphere::ProcessTransitions(VIScene &sc, float hour, float vol)
{
    VIRaster      *r = sc.m_raster;
    VIDictionary  *d = sc.m_dict;
    VISoundDevice *snd = sc.m_sound;
    VIColor32      c;
    int            i, j;
    int            alpha;
    VIMaterial    *m;     // ONE for every loop and arm: its range outlives the colour temp's (29 -> 11)

    if (m_p608 != 0.0f)
    {
        if (hour < 6.0f)
            m_p61C = 1.0f - hour / 6.0f;
        else if (18.0f < hour)
            m_p61C = (hour - 18.0f) / 6.0f;
        else
            m_p61C = 0.0f;
        m_p608 += m_p4A8 * (1.0f / 30.0f);
        m_p608 = VIMIN(m_p608, 1.0f);

        if (m_p5EC == 0 && m_p5F0 == 1)
        {
            alpha = 255 - (int)(m_p608 * 255.0f);
            m_p61C *= 1.0f - m_p608;
        }
        else if (m_p5EC == 1 && m_p5F0 == 0)
        {
            alpha = (int)(m_p608 * 255.0f);
            m_p61C *= m_p608;
        }
        else if (m_p5EC != 0 && m_p5F0 != 0)
        {
            alpha = 0;
            m_p61C = 0.0f;
        }
        else
            alpha = 255;
        if (alpha)
            m_overcast = 1;
        else
            m_overcast = 0;

        float fa = (float)alpha;
        int   pal = ((VISimpleSprite *)sc.Sprite(m_sun))->GetMaterialPal();
        int   n = r->GetMaterialPalSize(pal);

        for (i = 0; i < n; i++)
        {
            m = r->GetMaterialPalEntry(pal, i);

            for (j = 0; j < m->m_numLayers; j++)
            {
                c = m->GetLayerColor(j);
                c.a = alpha;
                m->SetLayerColor(j, c);
            }
        }
        pal = ((VISimpleSprite *)sc.Sprite(m_corona))->GetMaterialPal();
        n = r->GetMaterialPalSize(pal);
        for (i = 0; i < n; i++)
        {
            m = r->GetMaterialPalEntry(pal, i);

            for (j = 0; j < m->m_numLayers; j++)
            {
                // The clamp lives IN the layer loop, through a local `v`:
                // loop.c hoists it to the layer preheader (the `* 2` one
                // level further out), as the target has it (185 -> 29).
                int v = 255 - (255 - alpha) * 2;
                int a = VIMAX(v, 0);
                c = m->GetLayerColor(j);
                c.a = a;
                m->SetLayerColor(j, c);
            }
        }
        pal = ((VISimpleSprite *)sc.Sprite(m_moon))->GetMaterialPal();
        n = r->GetMaterialPalSize(pal);
        for (i = 0; i < n; i++)
        {
            m = r->GetMaterialPalEntry(pal, i);

            for (j = 0; j < m->m_numLayers; j++)
            {
                c = m->GetLayerColor(j);
                c.a = alpha;
                m->SetLayerColor(j, c);
            }
        }
        m_skyScale = fa / 255.0f;

        if (m_p5FC > 0 || m_p600 > 0)
        {
            float lv[7] = { 0.0f, 0.15f, 0.5f, 1.0f, 0.15f, 0.5f, 1.0f };
            int   st[2] = { 0 };

            if (m_p5FC > 0)
                st[0] = 1;
            if (m_p5FC >= 4)
                st[0] = 2;
            if (m_p600 > 0)
                st[1] = 1;
            if (m_p600 >= 4)
                st[1] = 2;
            if (st[0] == 1 && st[1] == 2)
                SetEffectMix(m_p608);
            else if (st[0] == 2 && st[1] == 1)
                SetEffectMix(1.0f - m_p608);
            else if (st[0] == 1 && st[1] == 1)
                SetEffectMix(0.0f);
            else if (st[0] == 2 && st[1] == 2)
                SetEffectMix(1.0f);
            else if (st[0] == 0)
            {
                if (st[1] == 1)
                    SetEffectMix(0.0f);
                else
                    SetEffectMix(1.0f);
            }
            else if (st[0] == 1)
                SetEffectMix(0.0f);
            else
                SetEffectMix(1.0f);
            SetIntensity(lv[m_p5FC] + (lv[m_p600] - lv[m_p5FC]) * m_p608);
            if (m_p1190 == -1)
            {
                m_p1190 = snd->Play(m_p1188, 127, (VISOUNDCLASS)0);
                ResetLightning();
            }
            if (m_p1194 == -1)
            {
                snd->SetPan(m_p1190, -1.0f);
                m_p1194 = snd->Play(m_p1188, 127, (VISOUNDCLASS)0);
                snd->SetPan(m_p1194, 1.0f);
                snd->SetPitch(m_p1194, 4176);
            }
            if (m_p1190)
                snd->SetVolume(m_p1190, Intensity() * (1.0f - EffectMix()) * 0.5f * vol);
            if (m_p1194)
                snd->SetVolume(m_p1194, Intensity() * (1.0f - EffectMix()) * 0.5f * vol);
        }
        else
        {
            SetIntensity(0.0f);
            if (m_p1190)
            {
                snd->Stop(m_p1190);
                m_p1190 = -1;
            }
            if (m_p1194)
            {
                snd->Stop(m_p1194);
                m_p1194 = -1;
            }
        }

        if (m_p608 < 0.5f)
        {
            m = r->GetMaterialPalEntry(
                ((VISimpleSprite *)sc.Sprite(m_skySprite))->GetMaterialPal(), 0);

            m->SetLayerBlendMode(0, VIAlphaBlendMode3);
            if (m->m_numLayers != 3)
            {
                m->SetNumLayers(3, *r, *d);
                m->CopyLayer(1, 2, *r, *d);
                m->CopyLayer(0, 1, *r, *d);
                m->SetLayerTexture(1, m_p5F8, *r, *d);
            }
            c = m->GetLayerColor(1);
            c.a = (int)(m_p608 * 2.0f * 255.0f);
            m->SetLayerColor(1, c);
            c = m->GetLayerColor(2);
            c.a = (int)((0.5f - m_p608) * 2.0f * 64.0f);
            m->SetLayerColor(2, c);
        }
        else if (m_p608 < 1.0f)
        {
            m = r->GetMaterialPalEntry(
                ((VISimpleSprite *)sc.Sprite(m_skySprite))->GetMaterialPal(), 0);

            if (m->m_numLayers != 2)
            {
                m->CopyLayer(1, 0, *r, *d);
                m->CopyLayer(2, 1, *r, *d);
                m->SetNumLayers(2, *r, *d);
                m->SetLayerTexture(1, m_p5F8, *r, *d);
            }
            c = m->GetLayerColor(1);
            c.a = (int)((m_p608 - 0.5f) * 2.0f * 64.0f);
            m->SetLayerColor(1, c);
        }
        if (m_p608 == 1.0f)
        {
            m = r->GetMaterialPalEntry(
                ((VISimpleSprite *)sc.Sprite(m_skySprite))->GetMaterialPal(), 0);

            m->SetLayerTexture(0, m_p5F8, *r, *d);
            m->SetLayerTexture(1, m_p5F8, *r, *d);
            c = m->GetLayerColor(0);
            c.a = 255;
            m->SetLayerColor(0, c);
            c = m->GetLayerColor(1);
            c.a = 64;
            m->SetLayerColor(1, c);
            m_p5EC = m_p5F0;
            m_p5F4 = m_p5F8;
            m_p5FC = m_p600;
            m_p608 = 0.0f;
            m_p60C = m_p610;
            if (m_p614 != m_p60C)
                SetDefaultSky((SKYTYPE)m_p614, false);
            if (m_p604 != m_p5FC)
                SetWeather((WXTYPE)m_p604, false);
        }
    }
    else if (m_p5EC == 0)
    {
        if (hour < 6.0f)
            m_p61C = 1.0f - hour / 6.0f;
        else if (18.0f < hour)
            m_p61C = (hour - 18.0f) / 6.0f;
        else
            m_p61C = 0.0f;
        m_skyScale = 1.0f;
    }
    else
    {
        m_p61C = 0.0f;
        m_skyScale = 0.0f;
    }

    if (Intensity() != 0.0f && EffectMix() < 1.0f)
    {
        if (m_p1198 < m_p119C)
        {
            m_p119C = 0.0f;
            m_p11A0 = 1.0e-5f;
        }
        m_p1198 -= m_p4A8;
        if (m_p1198 <= 0.0f)
        {
            int h = snd->Play(m_p118C, 127, (VISOUNDCLASS)0);

            snd->SetVolume(h, (1.0f - m_p11A8) * Intensity() * (1.0f - EffectMix()) * 0.9f * vol);
            snd->SetPan(h, -1.0f);
            h = snd->Play(m_p118C, 127, (VISOUNDCLASS)0);
            snd->SetVolume(h, (1.0f - m_p11A8) * Intensity() * (1.0f - EffectMix()) * 0.9f * vol);
            snd->SetPan(h, 1.0f);
            snd->SetPitch(h, 2496);
            ResetLightning();
        }
        if (m_p1190)
            snd->SetVolume(m_p1190, Intensity() * (1.0f - EffectMix()) * 0.5f * vol);
        if (m_p1194)
            snd->SetVolume(m_p1194, Intensity() * (1.0f - EffectMix()) * 0.5f * vol);
    }
    else if (Intensity() == 0.0f)
        m_p11A0 = m_p1198 = m_p119C = 0.0f;
}

// The Lavastorm: while there are fewer fireballs than m_lavastorm calls for,
// one is dropped at a random spot 50 m around the camera (pushed out of the
// 10 m the camera stands in) and traced to the ground, with one emitter riding
// down from 40 m above the landing point and a second waiting on it.  The
// first emitter lerps down over the fall; on landing the second fires once,
// and when its particles are spent the fireball goes back to the free list.
void VIAtmosphere::ProcessLavastorm(VIScene &sc, VICamera &cam)
{
    if (m_numFireballs < (int)(m_lavastorm * 32.0f))
    {
        VIVect3 a;
        VIVect3 b;

        a.x = VIRAND * 50.0f;
        a.y = 5000.0f;
        a.z = VIRAND * 50.0f;
        if (fabs(a.x) < 10.0 && fabs(a.z) < 10.0)
        {
            a.x += a.x < 0.0f ? -30.0f : 30.0f;
            a.z += a.z < 0.0f ? -30.0f : 30.0f;
        }
        const VIVect3 &cp = cam.m_position;

        a.Set(a.x + cp.x, a.y + cp.y, a.z + cp.z);
        b = a;
        b.y -= 10000.0f;

        VICollRay     ray;
        WeatherFilter filter;
        VIEnvRay      env;

        ray.Init(a, b);
        env.Init(a, b, 15);
        if (sc.Collide(ray, filter))
        {
            VIVect3 h = ray.GetHitPoint();

            if (sc.Collide(env))
            {
                VIVect3 e = env.GetHitPoint();

                if (h.y < e.y)
                    h = e;
            }

            VIFireball *f = (VIFireball *)m_fireballFree.RemHead();

            if (f)
            {
                m_fireballList.AddTail(f);
                m_numFireballs++;
                f->m_p1C = h;
                f->m_p10 = h;
                f->m_p28 = 0.0f;
                f->m_p10.y += 40.0f;
                f->m_p2C = ((float)((rand() * 131) & 0xFF) / 255.0f * 0.6f + 0.7f) * 2.5f;
                f->m_p30 = 0;
                f->m_e0.Create(m_p00, m_p00->ParticleDefinition(m_p1180));
                f->m_e0.SetLocation(f->m_p10);
                f->m_e0.SetLocation(f->m_p10);
                f->m_e1.Create(m_p00, m_p00->ParticleDefinition(m_p1184));
                f->m_e1.SetLocation(f->m_p1C);
                f->m_e1.SetLocation(f->m_p1C);
            }
        }
    }

    VIFireball *f = (VIFireball *)m_fireballList.m_head;

    while (f->m_next)
    {
        VIFireball *next = (VIFireball *)f->m_next;

        f->m_e0.ParticleCount();
        f->m_p28 += m_p00->DeltaTime() * 0.15f * f->m_p2C;
        if (1.0f <= f->m_p28)
        {
            if (f->m_p30 == 0)
            {
                f->m_p30 = 1;
                f->m_e1.Update();
            }
            else if (f->m_e1.ParticleCount() == 0)
            {
                f->m_e0.Destroy();
                f->m_e1.Destroy();
                f->Remove();
                m_fireballFree.AddTail(f);
                m_numFireballs--;
            }
        }
        else
        {
            VIVect3 d = f->m_p1C - f->m_p10;

            d *= f->m_p28;
            d += f->m_p10;
            f->m_e0.SetLocation(d);
            f->m_e0.Update();
        }
        f = next;
    }
}

float VIAtmosphere::EffectMix() const
{
    return m_effectMix;
}

void VIAtmosphere::SetEffectMix(float f)
{
    m_effectMix = f;
}

float VIAtmosphere::Intensity() const
{
    return m_intensity;
}

float VIAtmosphere::LavastormIntensity() const
{
    if (m_p1180 == -1 || m_p1184 == -1)
        return 0.0f;
    return m_lavastorm;
}

float VIAtmosphere::ParticleScale() const
{
    return m_particleScale;
}

void VIAtmosphere::SetParticleScale(float f)
{
    m_particleScale = f;
}

void VIAtmosphere::SetIntensity(float f)
{
    if (f > 0.0f && m_intensity == 0.0f)
        ClearAllSplashes();
    m_intensity = f;
}

void VIAtmosphere::SetLavastormIntensity(float f)
{
    if (m_p1180 == -1 || m_p1184 == -1)
        return;
    if (f > 0.0f && m_lavastorm == 0.0f)
        ClearAllFireballs();
    m_lavastorm = f;
}

int VIAtmosphere::SetPlaneOfSky(int i)
{
    if (m_p4E4[1] == -1)
        return 0;
    m_p618 = i;
    m_skySprite = m_p4E4[i];
    return 0;
}

void VIAtmosphere::ClearAllSplashes()
{
    int i;

    if (m_initialised == 0)
        return;
    if (m_numSplashes == 0)
        return;
    if (m_splashNodes == 0)
        return;

    m_numSplashes = 0;
    m_splashList.Init();
    m_splashFree.Init();
    for (i = 0; i < m_numParticles; i++)
        m_splashFree.AddTail((VILNode *)(m_splashNodes + i * 36));
}

void VIAtmosphere::ClearAllFireballs()
{
    VILNode *n;
    int      i;

    if (m_initialised == 0)
        return;
    if (m_numFireballs == 0)
        return;
    if (m_fireballNodes == 0)
        return;

    while ((n = m_fireballList.RemHead()) != 0)
    {
        ((VIParticleEmitter *)((char *)n + 64))->Destroy();
        ((VIParticleEmitter *)((char *)n + 1056))->Destroy();
    }
    m_numFireballs = 0;
    m_fireballList.Init();
    m_fireballFree.Init();
    for (i = 0; i < 32; i++)
        m_fireballFree.AddTail((VILNode *)((char *)m_fireballNodes + (i << 11)));
}

void VIAtmosphere::AddSplash(VIVect3 &p)
{
    VILNode *n = m_splashFree.RemHead();

    if (n)
    {
        m_numSplashes++;
        *(VIVect3 *)((char *)n + 16) = p;
        *(int *)((char *)n + 32) = 0;
        *(float *)((char *)n + 28) =
            ((rand() * 131) & 0xFF) / 255.0f * 0.2f + 0.282f;
        m_splashList.AddTail(n);
    }
}

void VIAtmosphere::ResetLightning()
{
    m_p11A8 = ((rand() * 131) & 0xFF) / 255.0f;
    m_p11A8 = VIMIN(VIMAX(0.1f, m_p11A8), 0.8f);
    m_p1198 = 8.0f;
    m_p119C = m_p11A8 * 6.0f;
    m_p11A4 = ((rand() * 131) & 0xFF) / 255.0f * 0.6f + 0.6f;
    RecomputeLightningPatches();
}

float VIAtmosphere::GetOvercastInterp()
{
    if (m_p5EC == 0)
    {
        if (m_p5F0 == 0)
            return 0.0f;
        if (m_p5F0 == 1)
            return m_p608;
    }
    if (m_p5EC == 1 && m_p5F0 == 0)
        return 1.0f - m_p608;
    return 1.0f;
}

int VIAtmosphere::SetWeather(WXTYPE t, bool b)
{
    if (m_initialised == 0)
        return -1;

    if (m_p608 != 0.0f)
        m_p604 = t;
    else if (m_p5FC != t)
    {
        m_p608 = b ? 1.0f : 1.0e-05f;
        m_p604 = t;
        m_p600 = t;
        if (t == 0)
        {
            m_p5F0 = (m_p60C == 2);
            m_p5F8 = m_skyTex[m_p60C];
        }
        else
        {
            m_p5F0 = 1;
            m_p5F8 = m_skyTex[2];
        }
    }
    return 0;
}

int VIAtmosphere::SetDefaultSky(SKYTYPE t, bool b)
{
    if (m_initialised == 0)
        return -1;

    if (m_p608 != 0.0f)
        m_p614 = t;
    else if (m_p60C != t)
    {
        m_p608 = b ? 1.0f : 1.0e-05f;
        m_p614 = t;
        m_p610 = t;
        if (t == 0)
        {
            if (m_p5FC > 0)
            {
                m_p60C = 0;
                m_p608 = 0.0f;
                return 0;
            }
            m_p5F0 = 0;
            m_p5F8 = m_skyTex[0];
        }
        else if (t == 1)
        {
            if (m_p5FC > 0)
            {
                m_p60C = 1;
                m_p608 = 0.0f;
                return 0;
            }
            m_p5F0 = 0;
            m_p5F8 = m_skyTex[1];
        }
        else
        {
            if (m_p5FC > 0)
            {
                m_p60C = t;
                m_p608 = 0.0f;
                return 0;
            }
            m_p5F0 = 1;
            m_p5F8 = m_skyTex[2];
        }
    }
    return 0;
}

void VIAtmosphere::RecomputeLightningPatches()
{
    int i;

    for (i = 0; i < 14; i++)
    {
        VIMatrix44 m;
        float      h = (128 - ((rand() / 137) & 0xFF)) / 128.0f * 0.45f * 3.14159274f;
        float      p = (128 - ((rand() / 137) & 0xFF)) / 128.0f * 0.32f * 3.14159274f;
        float      r = (128 - ((rand() / 137) & 0xFF)) / 128.0f * 0.32f * 3.14159274f;
        VIVect3    hpr(h, p, r);

        m.RotateHPR(hpr);

        VIVect4 v(0.0f, 100.0f, 0.0f, 1.0f);

        m.Mul(v, m_patches[i]);
        m_patches[i].w = (rand() / 131 & 0xFF) / 255.0f * 20.0f + 70.0f;
    }
}

int VIAtmosphere::GetLightning(float *out)
{
    float t;
    float b;
    float c;
    float h;

    if (m_p5EC != 1 || m_p5F0 != 1)
    {
        m_p11A0 = 0.0f;
        goto bad;
    }

    t = m_p11A0;
    if (t != 0.0f)
    {
    m_p11A0 = t + m_p4A8;
    if (m_p11A0 < 0.2f)
    {
        b = m_p11A0 / 0.2f;
        b = b + ((float)rand() - (float)rand()) / 2147483648.0f * 0.2f;
    }
    else
    {
        b = (m_p11A0 - 0.2f) / (m_p11A4 - 0.2f);
        b = b + ((float)rand() - (float)rand()) / 2147483648.0f * 0.3f;
        if ((rand() & 0xF) < 2)
            b = ((rand() * 131) & 0xFF) / 255.0f * 0.2f + 0.8f;
    }

    if (m_p11A0 >= m_p11A4)
        m_p11A0 = 0.0f;

    h = m_p11A4 * 0.5f;
    if (t <= h && h < m_p11A0)
    {
        if ((rand() & 0xF) < 6)
            RecomputeLightningPatches();
    }

    c = VIMIN(VIMAX(0.0f, b), 1.0f);
    b = c * (1.0f - m_p11A8);
    b = b * 0.8f;
    m_pF48 = b;
    *out = b;
    return 1;
    }
    m_pF48 = 0.0f;
bad:
    return 0;
}

int VIAtmosphere::Update(VIScene &sc, VICamera &cam, long time, float a, float b)
{
    if (m_initialised)
    {
        // The clock moves FIRST and the step reads the copy: a value whose
        // store is its last use lowers sched1's register-pressure weight, so
        // with the step computed first both `sd`s outrank the HPR temporary's
        // stores and run ahead of them (4 -> 0).
        m_p5D0 = m_p5C8;
        m_p5C8 = time;
        m_p4A8 = (int)(time - m_p5D0) * 0.001f;

        float w = a * 0.2617993f;

        m_skyRot.RotateHPR(VIVect3(0.0f, 0.0f, w));
        m_skyRot.m[3].x = sinf(w) * 35.0f;
        m_skyRot.m[3].y = cosf(w) * -35.0f;

        VIMatrix44 t;

        t.Scale(m_skyScale);
        m_coronaRot.Mul(t, m_skyRot);
        {
            VIVect3 hpr(0.0f, 0.0f, a * 0.2617993f + 3.14159274f);

            m_moonRot.RotateHPR(hpr);
        }
        m_moonRot.m[3].x = -m_skyRot.m[3].x;
        m_moonRot.m[3].y = -m_skyRot.m[3].y;
        ProcessWeather(sc, cam);
        ProcessTransitions(sc, a, b);
    }
}

int VIAtmosphere::Clear(VIScene &sc)
{
    VISoundDevice *snd = sc.m_sound;
    VIDictionary  *d = sc.m_dict;
    VIRaster      *r = sc.m_raster;
    int            i;

    if (m_initialised)
    {
        if (m_p1190)
        {
            snd->Stop(m_p1190);
            m_p1190 = -1;
        }
        if (m_p1194)
        {
            snd->Stop(m_p1194);
            m_p1194 = -1;
        }
        r->ReleaseMaterial(m_pF50, *d);
        r->ReleaseMaterial(m_starMat, *d);
        r->ReleaseMaterial(m_splashMat, *d);
        for (i = 0; i < 2; i++)
            r->ReleaseMaterial(m_mat[i], *d);
    }
    if (m_particles)
    {
        delete [] m_particles;
        m_particles = 0;
    }
    ClearAllFireballs();
    if (m_fireballNodes)
    {
        delete [] m_fireballNodes;
        m_fireballNodes = 0;
    }
    ClearAllSplashes();
    if (m_splashNodes)
    {
        delete [] m_splashNodes;
        m_splashNodes = 0;
    }
    m_initialised = 0;
}

int VIAtmosphere::SetSkyColor(VIScene &sc, const VIColor32 &c)
{
    VIRaster *r;
    int       pal;
    int       n;
    int       i;
    int       j;

    if (m_initialised == 0)
        return -1;

    VIColor32 col;

    r = sc.m_raster;
    pal = ((VISimpleSprite *)sc.Sprite(m_skySprite))->GetMaterialPal();
    n = r->GetMaterialPalSize(pal);
    for (i = 0; i < n; i++)
    {
        VIMaterial *m = r->GetMaterialPalEntry(pal, i);

        for (j = 0; j < m->m_numLayers; j++)
        {
            col = c;
            col.a = m->GetLayerColor(j).a;
            if (m->SetLayerColor(j, col) < 0)
                return -1;
        }
    }
    return 0;
}

int VIAtmosphere::RenderSplashes(VIScene &sc, VICamera &cam)
{
    VIRaster    *r = sc.m_raster;
    VIBillboard *bb = 0;
    VILNode     *n;
    VILNode     *next;
    int          left;
    int          num;

    num = m_numSplashes;
    r->SetMaterial(m_splashMat);
    for (n = m_splashList.m_head; n->m_next; n = next)
    {
        VISplash *s = (VISplash *)n;

        next = n->m_next;
        if (bb == 0)
            left = r->BeginBillboards(&bb, num);
        bb->m_location.x = s->m_location.x;
        num--;
        left--;
        bb->m_location.y = s->m_location.y;
        bb->m_location.z = s->m_location.z;
        bb->m_width = 0.08f;
        bb->m_height = 0.08f;
        bb->m_frame = (s->m_frame + 1) * 4;

        VIColor32F c(s->m_bright, s->m_bright, s->m_bright, s->m_bright);

        bb->m_color = c;
        bb++;
        if (left == 0)
        {
            r->EndBillboards();
            bb = 0;
        }
        s->m_frame = s->m_frame + 1;
        if (s->m_frame == 4)
        {
            n->Remove();
            m_splashFree.AddTail(n);
            m_numSplashes--;
        }
    }
    if (bb)
    {
        r->EndBillboards();
        bb = 0;
    }
}

int VIAtmosphere::RenderSky(VIScene &sc, VICamera &cam)
{
    VIRaster *r;

    if (m_initialised)
    {
        VIMatrix44 mv;

        r = sc.m_raster;
        sceVu0CopyMatrix(mv, cam.m_view);
        mv.m[3].x = 0.0f;
        mv.m[3].y = 0.0f;
        mv.m[3].z = 0.0f;
        r->SetModelView(mv);
        sc.Sprite(m_skySprite)->Raster(cam, sc, *r, 3);
        if (m_p618 == 0 && m_overcast)
        {
            VIMatrix44 t;

            t.Mul(m_skyRot, mv);
            r->SetModelView(t);
            sc.Sprite(m_sun)->Raster(cam, sc, *r, 3);
            t.Mul(m_coronaRot, mv);
            r->SetModelView(t);
            sc.Sprite(m_corona)->Raster(cam, sc, *r, 3);
            t.Mul(m_moonRot, mv);
            r->SetModelView(t);
            sc.Sprite(m_moon)->Raster(cam, sc, *r, 3);
        }
        r->SetModelView(mv);
        if (m_p618 == 0)
            RenderStars(sc, cam);
    }
}

void VIAtmosphere::RenderWeather(VIScene &sc, VICamera &cam)
{
    VIRaster    *r;
    VIBillboard *bb;
    int          n;
    int          k;
    int          i;
    int          j;
    int          left;
    int          cnt;
    int          first;
    int          last;
    float        aspect;
    float        sz;
    float        wide;
    float        high;

    if (m_initialised == 0)
        return;
    if (m_intensity == 0.0f)
        return;

    VIBBox  box;
    VIPlane pl;
    VIVect3 v;
    VIVect3 w;

    r = sc.m_raster;
    r->SetModelView(cam.m_view);
    n = (int)(m_intensity * m_numParticles);
    n = VIMAX(VIMIN(n, m_numParticles), 0);
    k = (int)(n * m_effectMix);
    pl = *cam.m_frustum.Plane(0);
    v.x = pl.n.x;
    v.y = pl.n.y;
    v.z = pl.n.z;
    w.x = pl.n.x;
    w.y = 0.0f;
    w.z = pl.n.z;
    w.Normalize();
    aspect = 1.0f - (v.x * w.x + v.y * w.y + v.z * w.z);
    if (k > 0 && k < n)
    {
        for (i = 0; i < 2; i++)
        {
            first = i ? k : 0;
            last = i ? n : k;
            r->SetMaterial(m_mat[1 - i]);
            cnt = i ? n - k : k;

            sz = m_p88[1 - i] * m_particleScale;
            bb = 0;
            wide = sz - sz * m_p90[i] * aspect;
            high = sz - sz * m_p98[i] * aspect;
            for (j = first; j < last; j++)
            {
                if (bb == 0)
                    left = r->BeginBillboards(&bb, cnt);
                bb->m_location = m_particles[j].m_location;
                bb->m_location.x = bb->m_location.x + m_camera.x;
                bb->m_location.y = bb->m_location.y + m_camera.y;
                bb->m_location.z = bb->m_location.z + m_camera.z;
                bb->m_width = wide;
                bb->m_height = high;
                cnt--;
                if (-1.5f < pl.Distance(bb->m_location))
                {
                    bb->m_width = 0.0f;
                    bb->m_height = 0.0f;
                }
                left--;
                bb->m_frame = 0;
                bb->m_color = m_particles[j].m_color;
                bb++;
                if (left == 0)
                {
                    r->EndBillboards();
                    bb = 0;
                }
            }
            if (bb)
            {
                r->EndBillboards();
                bb = 0;
            }
        }
    }
    else
    {
        r->SetMaterial(m_mat[k ? 1 : 0]);
        cnt = n;

        sz = m_p88[k ? 1 : 0] * m_particleScale;
        bb = 0;
        wide = sz - sz * m_p90[k ? 1 : 0] * aspect;
        high = sz - sz * m_p98[k ? 1 : 0] * aspect;
        for (j = 0; j < n; j++)
        {
            if (bb == 0)
                left = r->BeginBillboards(&bb, cnt);
            bb->m_location = m_particles[j].m_location;
            bb->m_location.x = bb->m_location.x + m_camera.x;
            bb->m_location.y = bb->m_location.y + m_camera.y;
            bb->m_location.z = bb->m_location.z + m_camera.z;
            bb->m_width = wide;
            bb->m_height = high;
            cnt--;
            if (-1.5f < pl.Distance(bb->m_location))
            {
                bb->m_width = 0.0f;
                bb->m_height = 0.0f;
            }
            left--;
            bb->m_frame = 0;
            bb->m_color = m_particles[j].m_color;
            bb++;
            if (left == 0)
            {
                r->EndBillboards();
                bb = 0;
            }
        }
        if (bb)
        {
            r->EndBillboards();
            bb = 0;
        }
    }
    RenderSplashes(sc, cam);
}

void VIAtmosphere::RenderStars(VIScene &sc, VICamera &cam)
{
    VIRaster    *r = sc.m_raster;
    VIBillboard *bb = 0;
    int          i;
    int          idx;

    if (0.0f < m_p61C)
    {
        VIColor32F c;
        int        left;
        int        cnt;
        float      b;

        c.Set(0.0f, 0.0f, 0.0f, 0.0f);
        r->SetFog(0, 0.0f, 0.0f, c);
        r->SetMaterial(m_starMat);
        if (m_pF28 <= 0.0f)
        {
            if (m_pF2C == 0.0f)
            {
                float a = ((rand() * 131) & 0xFF) / 255.0f * 3.14159274f * 3.9f;

                m_pF30.x = sinf(a) * 100.0f;
                m_pF30.y = 100.0f;
                m_pF30.z = cosf(a) * 100.0f;
                a = ((rand() * 131) & 0xFF) / 255.0f * 3.14159274f * 3.9f;
                m_pF3C.x = sinf(a) * 100.0f;
                m_pF3C.y = 0.0f;
                m_pF3C.z = cosf(a) * 100.0f;
                m_pF2C = 1.0e-05f;
            }
            if (0.0f < m_pF2C)
            {
                m_pF2C = m_pF2C + m_p4A8 * 0.2f;
                b = ((float)rand() - (float)rand()) / 2147483648.0f * 0.3f + 0.7f;
                if (m_pF2C < 0.2f)
                    b = b * (m_pF2C * 5.0f);
                else
                    b = b * (1.0f - (m_pF2C - 0.2f) * 1.25f);
                b = VIMIN(VIMAX(0.0f, b), 1.0f);
                r->BeginBillboards(&bb, 1);
                bb->m_location = m_pF3C;
                bb->m_location -= m_pF30;
                bb->m_location *= m_pF2C;
                bb->m_location += m_pF30;
                bb->m_width = 0.5f;
                bb->m_height = 0.5f;
                bb->m_frame = 0;
                c.Set(b, b, b, 1.0f);
                bb->m_color = c;
                r->EndBillboards();
                // Cleared on BOTH paths -- the store sits in the delay slot
                // of the `1.0 <= m_pF2C` branch -- so the star pass below
                // always begins a fresh batch.
                bb = 0;
                if (1.0f <= m_pF2C)
                {
                    m_pF2C = 0.0f;
                    m_pF28 = (float)((rand() & 0xF) + 10);
                }
            }
        }
        else
        {
            m_pF28 = m_pF28 - m_p4A8;
        }

        cnt = 128;
        idx = (char)(int)m_pF24;
        m_pF24 = m_pF24 + m_p4A8 * 15.0f;
        if (32.0f < m_pF24)
            m_pF24 = m_pF24 - 32.0f;
        for (i = 0; i < 128; i++)
        {
            b = m_pEA0[(idx + m_pE20[i]) & 0x1F] * m_p61C;
            if (bb == 0)
                left = r->BeginBillboards(&bb, cnt);
            bb->m_location.x = m_stars[i].x;
            cnt--;
            left--;
            bb->m_location.y = m_stars[i].y;
            bb->m_location.z = m_stars[i].z;
            bb->m_width = m_stars[i].w;
            bb->m_height = m_stars[i].w;
            bb->m_frame = 0;
            c.Set(b, b, b, 1.0f);
            bb->m_color = c;
            bb++;
            if (left == 0)
            {
                r->EndBillboards();
                bb = 0;
            }
        }
        if (bb)
        {
            r->EndBillboards();
            bb = 0;
        }
    }
    if (0.0f < m_pF48)
    {
        VIColor32F c;
        int        left;
        int        cnt;
        float      b;

        c.Set(0.0f, 0.0f, 0.0f, 0.0f);
        r->SetFog(0, 0.0f, 0.0f, c);
        r->SetMaterial(m_pF50);
        cnt = 14;
        for (i = 0; i < 14; i++)
        {
            if (bb == 0)
                left = r->BeginBillboards(&bb, cnt);
            bb->m_location.x = m_patches[i].x;
            bb->m_location.y = m_patches[i].y;
            bb->m_location.z = m_patches[i].z;
            bb->m_width = m_patches[i].w;
            bb->m_height = m_patches[i].w;
            bb->m_frame = 0;

            // `b` is clamped IN PLACE: the select's temporary cannot share a
            // register with the `b` it reads, which is the `mov.s $f0,$f1`
            // before the colour stores (11 -> 0).
            b = m_pF48 * 0.45f;
            b = VIMIN(b, 0.15f);
            cnt--;
            left--;
            c.Set(b, b, b, 1.0f);
            bb->m_color = c;
            bb++;
            if (left == 0)
            {
                r->EndBillboards();
                bb = 0;
            }
        }
        if (bb)
        {
            r->EndBillboards();
            bb = 0;
        }
    }
}

int VIAtmosphere::SetResources(VIScene &sc)
{
    VIDictionary  *d = sc.m_dict;
    VIRaster      *r = sc.m_raster;
    VIResourceType type;
    int            tex[2] = { -1, -1 };
    int            i;
    int            j;
    int            k;
    int            n;
    int            pal;
    VIMaterial    *m;

    m_p00 = &sc.m_particleSystem;

    if (d->Find(VIHashResourceID("rain.tga"), type, tex[0]) == 0)
        return -1;
    if (d->Find(VIHashResourceID("snow.tga"), type, tex[1]) == 0)
        return -1;
    if (d->Find(VIHashResourceID("splash.tga"), type, m_splashTex) == 0)
        return -1;

    if (d->Find(VIHashResourceID("Sun"), type, m_sun) == 0)
        return -1;
    pal = ((VISimpleSprite *)sc.Sprite(m_sun))->GetMaterialPal();
    n = r->GetMaterialPalSize(pal);
    for (i = 0; i < n; i++)
    {
        m = r->GetMaterialPalEntry(pal, i);
        m->SetDither(0);
        for (j = 0; j < m->m_numLayers; j++)
            m->SetLayerTextureLODBias(j, 1.0f);
    }

    if (d->Find(VIHashResourceID("Corona"), type, m_corona) == 0)
        return -1;
    pal = ((VISimpleSprite *)sc.Sprite(m_corona))->GetMaterialPal();
    n = r->GetMaterialPalSize(pal);
    for (i = 0; i < n; i++)
    {
        m = r->GetMaterialPalEntry(pal, i);
        m->SetDither(0);
        for (j = 0; j < m->m_numLayers; j++)
            m->SetLayerTextureLODBias(j, 1.0f);
    }

    if (d->Find(VIHashResourceID("Moon"), type, m_moon) == 0)
        return -1;
    pal = ((VISimpleSprite *)sc.Sprite(m_moon))->GetMaterialPal();
    n = r->GetMaterialPalSize(pal);
    for (i = 0; i < n; i++)
    {
        m = r->GetMaterialPalEntry(pal, i);
        m->SetDither(0);
        for (j = 0; j < m->m_numLayers; j++)
            m->SetLayerTextureLODBias(j, 1.0f);
    }

    if (d->Find(VIHashResourceID("sky"), type, m_p4E4[0]) == 0)
        return -1;
    if (d->Find(VIHashResourceID("EQOA_Skydome_360"), type, m_p4E4[1]) == 0)
        m_p4E4[1] = -1;

    for (k = 0; k < 2; k++)
    {
        m_skySprite = m_p4E4[1 - k];
        if (m_skySprite != -1)
        {
            pal = ((VISimpleSprite *)sc.Sprite(m_skySprite))->GetMaterialPal();
            n = r->GetMaterialPalSize(pal);
            for (i = 0; i < n; i++)
            {
                m = r->GetMaterialPalEntry(pal, i);
                if (m->SetZTest(0) < 0)
                    return -1;
                if (m->SetZWrite(0) < 0)
                    return -1;
                if (m->GetLayerTexture(0) != -1)
                    m_skyTex[1] = m->GetLayerTexture(0);
                for (j = 0; j < m->m_numLayers; j++)
                    m->SetLayerTextureLODBias(j, 1.0f);
            }
        }
    }

    if (d->Find(VIHashResourceID("PS2EQSky_Sunny.tga"), type, m_skyTex[0]) == 0)
        return -1;
    if (d->Find(VIHashResourceID("PS2EQSky_DarkClouds.tga"), type, m_skyTex[2]) == 0)
        return -1;
    if (d->Find(VIHashResourceID("star.tga"), type, m_starTex) == 0)
        return -1;
    if (d->Find(VIHashResourceID("lightning_patch.tga"), type, m_pF4C) == 0)
        return -1;
    if (d->Find(VIHashResourceID("rain_lt"), type, m_p1188) == 0)
        return -1;
    if (d->Find(VIHashResourceID("lightning_far02"), type, m_p118C) == 0)
        return -1;
    if (d->Find(VIHashResourceID("P:\\studio\\eqps2\\team\\brandon\\head.esf"),
                type, m_p1180) == 0)
        m_p1180 = -1;
    if (d->Find(VIHashResourceID("P:\\studio\\eqps2\\team\\brandon\\impact.esf"),
                type, m_p1184) == 0)
        m_p1184 = -1;

    if (m_p1180 != -1)
    {
        m_p00->ParticleDefinition(m_p1180)->SetDeltaSpawn(7.5f);
        m_p00->ParticleDefinition(m_p1180)->SetLifespan(
            m_p00->ParticleDefinition(m_p1180)->Lifespan() * 0.6f);
    }

    m = r->Material(m_starMat);
    if (m == 0)
        return -1;
    m->SetLayerTexture(0, m_starTex, *r, *d);

    m = r->Material(m_pF50);
    if (m == 0)
        return -1;
    m->SetLayerTexture(0, m_pF4C, *r, *d);

    for (i = 0; i < 2; i++)
    {
        m = r->Material(m_mat[i]);
        if (m == 0)
            return -1;
        m->SetLayerTexture(0, tex[i], *r, *d);
    }

    r->Material(m_splashMat)->SetLayerTexture(0, m_splashTex, *r, *d);
    m_initialised = 1;
    return 0;
}

void VIAtmosphere::ProcessWeather(VIScene &sc, VICamera &cam)
{
    int i;
    int j;
    int n;
    int k;
    int sel;
    int wrapped;

    if (0.0f < m_lavastorm)
        ProcessLavastorm(sc, cam);
    if (m_intensity == 0.0f)
        return;

    VIVect3       d;
    VICollRay     ray;
    WeatherFilter filter;
    VIVect3       a;
    VIVect3       b;
    VIVect3       c;
    VIEnvRay      env;

    // m_camera is the MEMBER at every use, so the compound operators get it as
    // an unflagged inline parameter -- `.x` folds to 60(this), `.y`/`.z` go
    // through one this+60 pseudo that is spilled and re-read after every stack
    // store, as the target does (221 -> 90).  The camera POSITION is the
    // opposite: a named const reference is a FLAGGED pointer, so its y/z loads
    // rise above c's stores (50 -> 2).  And the sum is built in `d` and copied
    // to `a`: every store here ties at one sched1 priority (each feeds the
    // unflagged m_camera.y load), so LUID order decides, and d's surviving
    // intermediate z must come before a.z (2 -> 0).
    const VIVect3 &pos = cam.m_position;
    ScaleTo(c, cam.m_dir, 5.0f);
    AddTo(d, c, pos);
    a = d;
    d -= m_camera;
    n = (int)(m_intensity * m_numParticles);
    k = (int)(n * m_effectMix);
    m_camera = a;
    if (m_range < d.x || m_range < d.y || m_range < d.z)
    {
        d.x = 0.0f;
        d.y = 0.0f;
        d.z = 0.0f;
        for (i = 0; i < m_numParticles; i++)
        {
            // Copy, offset, then add the camera through operator+=: the x
            // adds fold and run early, y/z wait behind the stack stores.
            a = m_particles[i].m_location;
            a.y += 5000.0f;
            b = m_particles[i].m_location;
            b.y -= 5000.0f;
            a += m_camera;
            b += m_camera;
            ray.Init(a, b);
            env.Init(a, b, 15);
            if (sc.Collide(ray, filter))
            {
                VIVect3 h = ray.GetHitPoint();

                m_particles[i].m_p20 = 0;

                if (sc.Collide(env))
                {
                    VIVect3 e = env.GetHitPoint();

                    if (e.y > h.y)
                    {
                        m_particles[i].m_p20 = 1;
                        h.x = e.x;
                        h.y = e.y;
                        h.z = e.z;
                    }
                }
                m_particles[i].m_p1C = h.y;
                m_particles[i].m_location.y = h.y - m_camera.y;
                m_particles[i].m_location.y =
                    m_range * 0.5f +
                    (m_particles[i].m_location.y - m_range * 0.5f) *
                        (((rand() * 131) & 0xFF) / 255.0f);
            }
        }
    }

    j = (int)m_p84;
    m_p84 = m_p84 + m_p4A8 * 10.0f;

    float rate = m_p4A8 / (1.0f / 60.0f);

    if (64.0f < m_p84)
        m_p84 = m_p84 - 64.0f;

    float half = m_range * 0.5f;

    for (i = 0; i < m_numParticles; j += 29, i++)   // j steps first: the latch order (90 -> 88)
    {
        sel = (i < k);
        m_particles[i].m_location.x =
            m_particles[i].m_location.x +
            (m_pA8[(j + i) & 0x3F].x * m_p50[sel].x + m_p68[sel].x) * rate;
        m_particles[i].m_location.y =
            m_particles[i].m_location.y +
            (m_pA8[(j + i) & 0x3F].y * m_p50[sel].y +
             m_p68[sel].y * m_p3A8[i & 0x3F]) * rate;
        m_particles[i].m_location.z =
            m_particles[i].m_location.z +
            (m_pA8[(j + i) & 0x3F].z * m_p50[sel].z + m_p68[sel].z) * rate;

        m_particles[i].m_location.y = m_particles[i].m_location.y - d.y;
        wrapped = 0;
        if (m_particles[i].m_location.y < -half)
            m_particles[i].m_location.y = m_particles[i].m_location.y + m_range;
        else if (half <= m_particles[i].m_location.y)
            m_particles[i].m_location.y = m_particles[i].m_location.y - m_range;
        m_particles[i].m_location.x = m_particles[i].m_location.x - d.x;
        m_particles[i].m_location.z = m_particles[i].m_location.z - d.z;
        if (m_particles[i].m_location.x < -half)
        {
            wrapped = 1;
            m_particles[i].m_location.x = m_particles[i].m_location.x + m_range;
        }
        else if (half <= m_particles[i].m_location.x)
        {
            wrapped = 1;
            m_particles[i].m_location.x = m_particles[i].m_location.x - m_range;
        }
        if (m_particles[i].m_location.z < -half)
        {
            wrapped = 1;
            m_particles[i].m_location.z = m_particles[i].m_location.z + m_range;
        }
        else if (half <= m_particles[i].m_location.z)
        {
            wrapped = 1;
            m_particles[i].m_location.z = m_particles[i].m_location.z - m_range;
        }

        if (wrapped)
        {
            a = m_particles[i].m_location;
            a.y += 5000.0f;
            b = m_particles[i].m_location;
            b.y -= 5000.0f;
            a += m_camera;
            b += m_camera;
            ray.Init(a, b);
            env.Init(a, b, 15);
            if (sc.Collide(ray, filter))
            {
                VIVect3 h = ray.GetHitPoint();

                m_particles[i].m_p20 = 0;

                if (sc.Collide(env))
                {
                    VIVect3 e = env.GetHitPoint();

                    if (e.y > h.y)
                    {
                        h.x = e.x;
                        h.y = e.y;
                        h.z = e.z;
                        m_particles[i].m_p20 = 1;
                    }
                }
                m_particles[i].m_p1C = h.y;
                m_particles[i].m_location.y = h.y - m_camera.y;
                m_particles[i].m_location.y =
                    m_range * 0.5f +
                    (m_particles[i].m_location.y - m_range * 0.5f) *
                        (((rand() * 131) & 0xFF) / 255.0f);
            }
        }

        if (m_particles[i].m_location.y + m_camera.y < m_particles[i].m_p1C)
        {
            if (sel == 0)
            {
                if (i < n)
                {
                    VIVect3 at;

                    at.x = m_particles[i].m_location.x + m_camera.x;
                    at.y = m_particles[i].m_p1C + 0.08f;
                    at.z = m_particles[i].m_location.z + m_camera.z;
                    AddSplash(at);
                }
            }
            if (m_particles[i].m_p20 == 1)
            {
                m_particles[i].m_location.x =
                    ((rand() * 131) & 0xFF) / 255.0f * m_range - m_range * 0.5f;
                m_particles[i].m_location.z =
                    ((rand() * 131) & 0xFF) / 255.0f * m_range - m_range * 0.5f;
            }
            m_particles[i].m_location.y =
                VIMAX(m_particles[i].m_p1C - m_camera.y, half);
        }
    }
}
