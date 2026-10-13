// VIPointSprite -- SUPPORT 0x0045C950 .. 0x0045D960.  Its own TU.
//
// The editor's marker for an object that draws nothing of its own: it holds a
// type, a 0.1-unit bounding sphere it derives itself, and it forwards the whole
// draw to whatever sprite the SCENE has registered as its point UI sprite.  If
// the scene has none (-1) every virtual is a no-op.
//
// ClampScale is the reason the class exists: the model-view row lengths are
// clamped to 4, so a marker on a hugely scaled actor stays a readable size.
#include "vi.h"

extern "C" void sceVu0CopyMatrix(VIMatrix44 &d, const VIMatrix44 &s);

VIPointSprite::VIPointSprite()
{
    m_type = 2;
    m_pointType = 0;
}

VIPointSprite::~VIPointSprite()
{
}

int VIPointSprite::Init(VIPointType type)
{
    // VIPointLight::Init's pointer, and the literal radius goes BEFORE Zero().
    VISphere *b = &m_bsphere;
    m_pointType = type;
    m_bsphere.r = 0.1f;
    m_bsphere.c.Zero();
    m_bbox.min = b->c;
    m_bbox.min -= b->r;
    m_bbox.max = b->c;
    m_bbox.max += b->r;
    return 0;
}

int VIPointSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    // Phase 35.  Phase 7's if-direction, and it had been the wrong way round
    // here since phase 20: the early `== -1` return orders the last call's
    // argument moves wrong and costs exactly 6 words.  Nested, byte-exact.
    if (sc.GetPointUISprite() != -1)
    {
        VIMatrix44 m;
        sceVu0CopyMatrix(m, *r.GetModelView());
        ClampScale(m);
        r.SetModelView(m);
        sc.RasterUISprite(cam, sc.GetPointUISprite(), flags);
    }
    return 0;
}

int VIPointSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VIPointSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VIPointSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    if (sc.GetPointUISprite() != -1)
    {
        VIMatrix44 m;
        sceVu0CopyMatrix(m, *r.GetModelView());
        ClampScale(m);
        r.SetModelView(m);
        return sc.PickSprite(seg, f, sc.GetPointUISprite());
    }
    return 0;
}

int VIPointSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VIPointSprite *s = (VIPointSprite *)sc.Sprite(i);
    s->m_bbox = m_bbox;
    s->m_bsphere = m_bsphere;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
    return i;
}

int VIPointSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                           VISoundDevice *sd, VICollide &c)
{
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }
    return 0;
}

// 0.25f is 1/4 and 4.0f is its reciprocal: `s` holds the RECIPROCAL of each row
// length, so `s.x < 0.25f` is "this row is longer than 4" and the row is scaled
// by s.x*4, which lands it at exactly 4.
int VIPointSprite::ClampScale(VIMatrix44 &m)
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

int VIPointSprite::GetPointType()
{
    return m_pointType;
}
