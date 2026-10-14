// VIStreamAudioSprite -- SUPPORT 0x0047F090 .. 0x004812E0.  A streaming-audio
// volume.  QueryEmitters is the interesting one: it inverts the caller's
// transform, asks whether the listener is inside the box, turns m_priority into
// a base key through a three-case switch whose default RETURNS -1, and subtracts
// a ten-thousandth of the distance to the box centre so that, among equally
// prioritised volumes, the nearest sorts last.
#include "vi.h"

extern "C" float sqrtf(float x);

VIStreamAudioSprite::VIStreamAudioSprite()
{
    m_type = 12;
    m_looping = true;
    m_priority = 0;
    m_pathname[0] = 0;
}

VIStreamAudioSprite::~VIStreamAudioSprite()
{
}

int VIStreamAudioSprite::Init(const VIBBox &b)
{
    m_bbox = b;
    m_bsphere.Init(m_bbox);
    return 0;
}

int VIStreamAudioSprite::GetInstType(VIScene &sc) const
{
    return 1;
}

int VIStreamAudioSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r,
                                unsigned int flags)
{
    // Phase 35.  Phase 7's if-direction, and it had been the wrong way round
    // here since phase 20: the early `== -1` return orders the last call's
    // argument moves wrong and costs exactly 6 words.  Nested, byte-exact.
    if (sc.GetStreamAudioUISprite() != -1)
    {
        VIMatrix44 m;
        sceVu0CopyMatrix(m, *r.GetModelView());
        ClampScale(m);
        r.SetModelView(m);
        sc.RasterUISprite(cam, sc.GetStreamAudioUISprite(), flags);
    }
    return 0;
}

int VIStreamAudioSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc,
                                 VICollide &c)
{
    return 0;
}

int VIStreamAudioSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc,
                                 VICollide &c)
{
    return 0;
}

int VIStreamAudioSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc,
                              VIRaster &r)
{
    if (sc.GetStreamAudioUISprite() != -1)
    {
        VIMatrix44 m;
        sceVu0CopyMatrix(m, *r.GetModelView());
        ClampScale(m);
        r.SetModelView(m);
        return sc.PickSprite(seg, f, sc.GetStreamAudioUISprite());
    }
    return 0;
}

int VIStreamAudioSprite::QueryEmitters(VIScene &sc, const VIMatrix44 &m,
                                       VIEmitterQuery &q)
{
    if (q.m_type == m_type)
    {
        VIVect3         p;
        VIEmitterEntry  e;
        VIMatrix44      inv;
        float           k;

        inv.InvertAffine(m);
        p.x = inv.m[3].x;
        p.y = inv.m[3].y;
        p.z = inv.m[3].z;
        if (m_bbox.Contains(p))
        {
            switch (m_priority)
            {
            case 0:
                k = 1.0f;
                break;
            case 1:
                k = 2.0f;
                break;
            case 2:
                k = 3.0f;
                break;
            default:
                return -1;
            }
            p.x = p.x - (m_bbox.min.x + m_bbox.max.x) * 0.5f;
            p.y = p.y - (m_bbox.min.y + m_bbox.max.y) * 0.5f;
            p.z = p.z - (m_bbox.min.z + m_bbox.max.z) * 0.5f;
            k = k - p.Magnitude() * 0.0001f;
            e.m_a = q.m_a;
            e.m_b = q.m_b;
            e.m_matrix = m;
            q.m_map.Insert(k, e);
            if (q.m_limit < q.m_map.Size())
                q.m_map.Erase(q.m_map.Begin());
        }
    }
    return 0;
}

int VIStreamAudioSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd,
                              VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VIStreamAudioSprite *s = (VIStreamAudioSprite *)sc.Sprite(i);
    s->m_bbox = m_bbox;
    s->m_bsphere = m_bsphere;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
    s->m_looping = m_looping;
    s->m_priority = m_priority;
    VIString::strcpy8(s->m_pathname, 128, m_pathname);
    return i;
}

int VIStreamAudioSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                                 VISoundDevice *sd, VICollide &c)
{
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }
    return 0;
}

bool VIStreamAudioSprite::IsLooping() const
{
    return m_looping;
}

int VIStreamAudioSprite::GetPriority() const
{
    return m_priority;
}

const char *VIStreamAudioSprite::GetRelativePathname() const
{
    return m_pathname;
}

int VIStreamAudioSprite::ClampScale(VIMatrix44 &m)
{
    VIVect3 s;

    s.x = 1.0f / sqrtf(m.m[0].x * m.m[0].x + m.m[0].y * m.m[0].y + m.m[0].z * m.m[0].z);
    s.y = 1.0f / sqrtf(m.m[1].x * m.m[1].x + m.m[1].y * m.m[1].y + m.m[1].z * m.m[1].z);
    s.z = 1.0f / sqrtf(m.m[2].x * m.m[2].x + m.m[2].y * m.m[2].y + m.m[2].z * m.m[2].z);

    if (s.x < 0.25f)
    {
        m.m[0].x = m.m[0].x * s.x * 4.0f;
        m.m[0].y = m.m[0].y * s.x * 4.0f;
        m.m[0].z = m.m[0].z * s.x * 4.0f;
    }
    if (s.y < 0.25f)
    {
        m.m[1].x = m.m[1].x * s.y * 4.0f;
        m.m[1].y = m.m[1].y * s.y * 4.0f;
        m.m[1].z = m.m[1].z * s.y * 4.0f;
    }
    if (s.z < 0.25f)
    {
        m.m[2].x = m.m[2].x * s.z * 4.0f;
        m.m[2].y = m.m[2].y * s.z * 4.0f;
        m.m[2].z = m.m[2].z * s.z * 4.0f;
    }
    return 0;
}
