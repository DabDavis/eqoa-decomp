// VIAtmosphere -- SUPPORT 0x00416888 .. 0x0041C7C8, its own TU.  Phase 55.
// The sky and the weather.
#include "vi.h"

extern "C" int rand(void);

static inline VIVect3 operator*(const VIVect3 &v, float f)
{
    return VIVect3(v.x * f, v.y * f, v.z * f);
}

static inline VIVect3 operator+(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x + b.x, a.y + b.y, a.z + b.z);
}

extern "C" float sinf(float);
extern "C" float cosf(float);
extern "C" unsigned int VIHashResourceID(const char *);
extern "C" void  sceVu0CopyMatrix(VIMatrix44 &d, const VIMatrix44 &s);

// The scene filter the weather rays go through: it accepts everything.  Its
// vtable and typeinfo are the TU's first and last symbols, which is what puts
// this one-line override at the top of the file.
int WeatherFilter::FilterActor(VIScene &sc, int actor)
{
    return 0;
}

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
        float w = a * 0.2617993f;

        m_p4A8 = (int)(time - m_p5C8) * 0.001f;
        m_p5D0 = m_p5C8;
        m_p5C8 = time;
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
                if (1.0f <= m_pF2C)
                {
                    m_pF2C = 0.0f;
                    m_pF28 = (float)((rand() & 0xF) + 10);
                    bb = 0;
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

            float t = m_pF48 * 0.45f;

            b = VIMIN(t, 0.15f);
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

// OPEN, and a DRAFT: 278 of 626 words.  The shape is right -- two passes over
// the particle field, a downward VICollRay and a VIEnvRay per particle, the
// wrap at +-range/2 and the splash at the ground -- but the expression forms
// are not settled, so read this for semantics and not as a matched function.
int VIAtmosphere::ProcessWeather(VIScene &sc, VICamera &cam)
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
        return -1;

    VIVect3       d;
    VICollRay     ray;
    WeatherFilter filter;
    VIVect3       a;
    VIVect3       b;
    VIVect3       c;
    VIEnvRay      env;
    VIVect3      &cm = m_camera;

    c = cam.m_dir * 5.0f;
    a = cam.m_position + c;
    d = a;
    d -= cm;
    n = (int)(m_intensity * m_numParticles);
    k = (int)(n * m_effectMix);
    cm.x = a.x;
    cm.y = a.y;
    cm.z = a.z;

    if (m_range < d.x || m_range < d.y || m_range < d.z)
    {
        d.x = 0.0f;
        d.y = 0.0f;
        d.z = 0.0f;
        for (i = 0; i < m_numParticles; i++)
        {
            a.x = m_particles[i].m_location.x;
            a.y = m_particles[i].m_location.y;
            a.z = m_particles[i].m_location.z;
            a.x = a.x + cm.x;
            a.y = a.y + 5000.0f;
            b.x = m_particles[i].m_location.x;
            b.y = m_particles[i].m_location.y;
            b.z = m_particles[i].m_location.z;
            b.x = b.x + cm.x;
            b.y = b.y - 5000.0f;
            a.y = a.y + cm.y;
            a.z = a.z + cm.z;
            b.y = b.y + cm.y;
            b.z = b.z + cm.z;
            ray.Init(a, b);
            env.Init(a, b, 15);
            if (sc.Collide(ray, filter))
            {
                m_particles[i].m_p20 = 0;

                VIVect3 h = ray.GetHitPoint();

                if (sc.Collide(env))
                {
                    VIVect3 e = env.GetHitPoint();

                    if (h.y < e.y)
                    {
                        m_particles[i].m_p20 = 1;
                        h.x = e.x;
                        h.y = e.y;
                        h.z = e.z;
                    }
                }
                m_particles[i].m_p1C = h.y;
                m_particles[i].m_location.y = h.y - cm.y;
                m_particles[i].m_location.y =
                    m_range * 0.5f +
                    (m_particles[i].m_location.y - m_range * 0.5f) *
                        (((rand() * 131) & 0xFF) / 255.0f);
            }
        }
    }

    j = (int)m_p84;
    m_p84 = m_p84 + m_p4A8 * 10.0f;
    if (64.0f < m_p84)
        m_p84 = m_p84 - 64.0f;

    float half = m_range * 0.5f;
    float rate = m_p4A8 / (1.0f / 60.0f);

    for (i = 0; i < m_numParticles; i++)
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
            a.x = m_particles[i].m_location.x;
            a.y = m_particles[i].m_location.y;
            a.z = m_particles[i].m_location.z;
            a.x = a.x + cm.x;
            a.y = a.y + 5000.0f;
            b.x = m_particles[i].m_location.x;
            b.y = m_particles[i].m_location.y;
            b.z = m_particles[i].m_location.z;
            b.x = b.x + cm.x;
            b.y = b.y - 5000.0f;
            a.y = a.y + cm.y;
            a.z = a.z + cm.z;
            b.y = b.y + cm.y;
            b.z = b.z + cm.z;
            ray.Init(a, b);
            env.Init(a, b, 15);
            if (sc.Collide(ray, filter))
            {
                m_particles[i].m_p20 = 0;

                VIVect3 h = ray.GetHitPoint();

                if (sc.Collide(env))
                {
                    VIVect3 e = env.GetHitPoint();

                    if (h.y < e.y)
                    {
                        h.x = e.x;
                        h.y = e.y;
                        h.z = e.z;
                        m_particles[i].m_p20 = 1;
                    }
                }
                m_particles[i].m_p1C = h.y;
                m_particles[i].m_location.y = h.y - cm.y;
                m_particles[i].m_location.y =
                    m_range * 0.5f +
                    (m_particles[i].m_location.y - m_range * 0.5f) *
                        (((rand() * 131) & 0xFF) / 255.0f);
            }
        }

        if (m_particles[i].m_location.y + cm.y < m_particles[i].m_p1C)
        {
            if (sel == 0)
            {
                if (i < n)
                {
                    VIVect3 at;

                    at.x = m_particles[i].m_location.x + cm.x;
                    at.y = m_particles[i].m_p1C + 0.08f;
                    at.z = m_particles[i].m_location.z + cm.z;
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
                VIMAX(m_particles[i].m_p1C - cm.y, half);
        }
    }
    return 0;
}
