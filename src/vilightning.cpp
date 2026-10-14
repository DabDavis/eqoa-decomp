// VIBoltHarmonic / VIBolt / VILightning -- SUPPORT 0x0044E1F8 .. 0x0044FA78.
// Phase 33.  The lightning bolt: three harmonics of interpolated noise summed
// along the segment between two points, plus up to three branches off it.
#include "vi.h"

extern "C" int rand(void);
extern "C" void *memcpy(void *d, const void *s, unsigned int n);
extern "C" unsigned int VIHashResourceID(const char *);
extern "C" float sinf(float);

static int sMaterial = -1;

static inline VIVect3 operator-(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline void Cross(VIVect3 &out, const VIVect3 &a, const VIVect3 &b)
{
    out.x =  a.y * b.z - a.z * b.y;
    out.y = -a.x * b.z + a.z * b.x;
    out.z =  a.x * b.y - a.y * b.x;
}

static inline VIVect3 operator+(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline VIVect3 Cross(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3( a.y * b.z - a.z * b.y,
                   -a.x * b.z + a.z * b.x,
                    a.x * b.y - a.y * b.x);
}

int VILightningFxInit(VIRaster &r, VIDictionary &d)
{
    VIResourceType type;
    int            tex;

    if (d.Find(VIHashResourceID("lightning.tga"), type, tex) == 0)
        return -1;

    sMaterial = r.CreateMaterial(0);
    if (sMaterial == -1)
        return -1;

    VIMaterial *m = r.Material(sMaterial);
    m->Init(1);
    m->SetLayerFillType(0, VIMaterialFillTypeTexture);
    m->SetLayerModulate(0, 1);
    m->SetLayerBlendMode(0, VIAlphaBlendMode1);
    m->SetZWrite(0);
    m->SetZTest(1);
    m->SetLayerTexture(0, tex, r, d);
    return 0;
}

int VILightningFxClear(VIRaster &r, VIDictionary &d)
{
    if (sMaterial != -1) {
        r.ReleaseMaterial(sMaterial, d);
        sMaterial = -1;
    }
    return 0;
}

int VIBoltHarmonic::Create(int count, float amplitude, float rate)
{
    m_count = count;
    m_phase = 0.0f;
    m_rate = rate;
    m_amplitude = amplitude;

    GenerateSamples(0);
    GenerateSamples(1);
    return 0;
}

void VIBoltHarmonic::GenerateSamples(int which)
{
    VIVect3 *s = m_samples[which];
    int      i;

    for (i = 0; i < 17; i++) {
        s[i].x = (float)(((rand() / 131) & 255) - 127) / 128.0f * m_amplitude;
        s[i].y = (float)(((rand() / 131) & 255) - 127) / 128.0f * m_amplitude;
        s[i].z = 0.0f;
    }
}

VIVect3 VIBoltHarmonic::Sample(int i)
{
    VIVect3 d;
    float   t;

    t = sinf(m_phase * 3.1415927f + 1.5707964f) * 0.5f + 0.5f;

    d = m_samples[1][i];
    d -= m_samples[0][i];
    d *= 1.0f - t;
    d += m_samples[0][i];
    return d;
}

void VIBoltHarmonic::Update(int dt)
{
    float t = (float)dt * 0.001f;
    int   i;

    if (1.0f <= m_phase) {
        m_phase = 0.0f;
        for (i = 0; i < 17; i++)
            m_samples[0][i] = m_samples[1][i];
        GenerateSamples(1);
        return;
    }
    m_phase = m_phase + t * m_rate;
    m_phase = VIMIN(1.0f, m_phase);
}

VIBolt::VIBolt()
{
    Reset();
}

void VIBolt::Reset()
{
    m_harmonics[0].Create(3, 0.57f, 0.64f);
    m_harmonics[1].Create(3, 0.15f, 10.24f);
    m_harmonics[2].Create(3, 0.08f, 44.0f);
}

void VIBolt::Update(int dt, VIVect3 &a, VIVect3 &b, VIMatrix44 &m)
{
    VIVect3 d;
    VIVect3 t;
    int     i, j;

    m_harmonics[0].Update(dt);
    m_harmonics[1].Update(dt);
    m_harmonics[2].Update(dt);

    // Copy then subtract in place: written per component the three
    // differences are loaded and stored x-first, where the target goes z-first.
    d = b;
    d -= a;

    for (i = 0; i < 17; i++) {
        m_points[i] = d;
        m_points[i] *= (float)i * 0.0625f;
        m_points[i] += a;
    }

    VIVect3 p;

    for (i = 0; i < 4; i++) {
        // start/end are per-loop locals: shared at function scope they are one
        // pseudo each and swap callee-saved registers with the loop counters.
        int start = i * 4;
        int end = start + 4;

        VIVect3 s = m_harmonics[0].Sample(i);
        m.Mul(s, t);

        d = m_points[end];
        if (i < 3)
            d += t;
        d -= m_points[start];

        for (j = start; j <= end; j++) {
            p = d;
            p *= (float)(j - start) / (float)(end - start);
            p += m_points[start];
            m_points[j] = p;
        }
    }

    for (i = 0; i < 8; i++) {
        int start = i * 2;
        int end = start + 2;

        VIVect3 s = m_harmonics[1].Sample(i);
        m.Mul(s, t);

        d = m_points[end];
        d += t;
        d -= m_points[start];

        for (j = start; j <= end; j++) {
            p = d;
            p *= (float)(j - start) / (float)(end - start);
            p += m_points[start];
            m_points[j] = p;
        }
    }

    for (i = 1; i < 16; i++) {
        VIVect3 s = m_harmonics[2].Sample(i);
        m.Mul(s, t);

        m_points[i] += t;
    }
}

VILightning::VILightning()
{
    m_start.Zero();
    m_end.Zero();
    m_ringHead = -1;
    m_strikeTimer = 0;
    m_boltCount = 0;
    m_ringCount = 0;
    m_ringTimer = 0;
    m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_maxBranchCount = 0;
    m_charge = 0.0f;
    m_lastCharge = 0.0f;
}

VILightning::~VILightning()
{
}

void VILightning::SetLocation(const VIVect3 &a, const VIVect3 &b)
{
    m_start = a;
    m_end = b;

    VIVect3 right;
    VIVect3 up(0.0f, 1.0f, 0.0f);
    VIVect3 d = b - a;

    Cross(right, up, d);
    up = Cross(d, right);

    right.Normalize();
    up.Normalize();
    d.Normalize();

    m_matrix.m[0].x = right.x;
    m_matrix.m[0].y = right.y;
    m_matrix.m[0].z = right.z;
    m_matrix.m[0].w = 0.0f;
    m_matrix.m[1].x = up.x;
    m_matrix.m[1].y = up.y;
    m_matrix.m[1].z = up.z;
    m_matrix.m[1].w = 0.0f;
    m_matrix.m[2].x = d.x;
    m_matrix.m[2].y = d.y;
    m_matrix.m[2].z = d.z;
    m_matrix.m[2].w = 0.0f;
    m_matrix.m[3].x = 0.0f;
    m_matrix.m[3].y = 0.0f;
    m_matrix.m[3].z = 0.0f;
    m_matrix.m[3].w = 1.0f;
}

float VILightning::Charge() const
{
    return m_charge;
}

void VILightning::SetCharge(float c)
{
    m_charge = c;
}

void VILightning::SetColor(const VIColor32F &c)
{
    m_color = c;
}

void VILightning::SetMaxBranchCount(int n)
{
    m_maxBranchCount = VIMIN(n, 3);
}

void VILightning::Update(int dt)
{
    int i;

    m_strikeTimer -= dt;
    m_ringTimer -= dt;
    if (m_ringTimer < 0)
        m_ringTimer = 0;

    if (m_strikeTimer <= 0) {
        for (i = 1; i < 4; i++)
            if (i < m_boltCount)
                m_bolts[i].Reset();

        int r = rand() % 351;
        m_strikeTimer = VIMAX(r, 70);

        m_boltCount = rand() % (m_maxBranchCount + 1) + 1;
        if (m_boltCount == 1)
            m_strikeTimer >>= 2;
    }

    for (i = 0; i < m_boltCount; i++) {
        if (i == 0)
            m_bolts[0].Update(dt, m_start, m_end, m_matrix);
        else
            m_bolts[i].Update(dt, m_bolts[0].m_points[i * 3], m_end, m_matrix);
    }

    m_lastCharge = m_charge;
    m_charge = m_charge - (float)(dt * 5) * 0.001f;
    m_charge = VIMAX(0.0f, m_charge);
}

void VILightning::Render(VIRaster &r)
{
    BRANCH *b;
    int     i, j;

    r.SetMaterial(sMaterial);

    if (m_ringTimer == 0) {
        m_ringHead = (m_ringHead + 1) % 3;
        m_ringTimer = 30;
        m_ringCount = VIMIN(3, m_ringCount + 1);
    }

    b = &m_branches[m_ringHead];
    b->m_count = m_boltCount;
    for (i = 0; i < m_boltCount; i++)
        memcpy(b->m_points[i], m_bolts[i].m_points, sizeof(m_bolts[i].m_points));

    for (i = 0; i < m_ringCount; i++) {
        b = &m_branches[(m_ringHead - i + 3) % 3];
        for (j = 0; j < b->m_count; j++)
            RenderBoltVertices(r, b->m_points[j], 3 - i);
    }
}

void VILightning::RenderBoltVertices(VIRaster &r, VIVect3 *pts, int n)
{
    float f = (float)n / 3.0f;
    int   i, j;

    r.Color(VIColor32F(m_color.r * f * m_lastCharge,
                       m_color.g * f * m_lastCharge,
                       m_color.b * f * m_lastCharge, 1.0f));

    VIVect3 p;
    VIVect3 off;

    for (i = 0; i < 4; i++) {
        switch (i) {
        case 0:
            off.Set(0.0f, 0.04f, 0.0f);
            break;
        case 1:
            off.Set(0.0f, -0.04f, 0.0f);
            break;
        case 2:
            p.Set(0.04f, 0.0f, 0.0f);
            m_matrix.Mul(p, off);
            break;
        case 3:
            off.x = -off.x;
            off.y = -off.y;
            off.z = -off.z;
            break;
        }

        r.BeginTriStrip();

        p = pts[0];
        r.UV(VIVect2(0.0f, 1.0f));
        r.Vertex(p);
        r.UV(VIVect2(0.0f, 0.0f));
        r.Vertex(p);

        for (j = 1; j < 9; j++) {
            p = pts[j];
            r.UV(VIVect2((float)j, 1.0f));
            r.Vertex(p - off);
            r.UV(VIVect2((float)j, 0.0f));
            r.Vertex(p + off);
        }

        r.EndTriStrip();
        r.BeginTriStrip();

        p = pts[8];
        r.UV(VIVect2(0.0f, 1.0f));
        r.Vertex(p - off);
        r.UV(VIVect2(0.0f, 0.0f));
        r.Vertex(p + off);

        for (j = 8; j < 17; j++) {
            p = pts[j];
            if (j == 16) {
                r.UV(VIVect2(8.0f, 1.0f));
                r.Vertex(p);
                r.UV(VIVect2(8.0f, 0.0f));
                r.Vertex(p);
            } else {
                r.UV(VIVect2((float)(j - 8), 1.0f));
                r.Vertex(p - off);
                r.UV(VIVect2((float)(j - 8), 0.0f));
                r.Vertex(p + off);
            }
        }

        r.EndTriStrip();
    }
}
