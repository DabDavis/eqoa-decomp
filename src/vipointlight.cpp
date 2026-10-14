// VIPointLight -- SUPPORT 0x0045B878 .. 0x0045C950.  Its own TU, and the last
// sprite leaf in the engine.
//
// VIPointSprite with a colour instead of a type: the editor marker for a point
// light.  Raster, Pick, Collide, Release and ClampScale come out byte-identical
// to VIPointSprite's, which is what makes this TU cheap -- the only semantic
// difference is GetLightUISprite for GetPointUISprite, and the un-relocated
// image cannot tell those apart at all (the `jal` is zeroed either way).
#include "vi.h"


VIPointLight::VIPointLight()
{
    m_type = 8;
}

VIPointLight::~VIPointLight()
{
}

int VIPointLight::Init(float r, const VIColor32F &c)
{
    // Phase 9's pointer variable, and the radius is read THROUGH it: the two
    // compound operators load m_bsphere.r as `12(b)`, not `44(this)`.  Written
    // with the member name it comes off `this` and costs 14 of 46 words.
    // (VIPointSprite gets the same pseudo for free from m_bsphere.c.Zero().)
    VISphere *b = &m_bsphere;

    m_bsphere.r = r;
    m_bbox.min = b->c;
    m_bbox.min -= b->r;
    m_bbox.max = b->c;
    m_bbox.max += b->r;
    m_color = c;
    return 0;
}

int VIPointLight::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    // Phase 7's if-direction, per function (phase 20).  Written as an early
    // `== -1` return -- the way Raster reads most naturally, and the way it has
    // stood open in VIPointSprite since phase 20 -- the argument moves come out
    // in the wrong order and cost 6 words.  Pick, three functions down, is the
    // other answer: there the test really is `!= -1`, because it returns the
    // call's value.
    if (sc.GetLightUISprite() != -1)
    {
        VIMatrix44 m;
        sceVu0CopyMatrix(m, *r.GetModelView());
        ClampScale(m);
        r.SetModelView(m);
        sc.RasterUISprite(cam, sc.GetLightUISprite(), flags);
    }
    return 0;
}

int VIPointLight::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VIPointLight::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    if (sc.GetLightUISprite() != -1)
    {
        VIMatrix44 m;
        sceVu0CopyMatrix(m, *r.GetModelView());
        ClampScale(m);
        r.SetModelView(m);
        return sc.PickSprite(seg, f, sc.GetLightUISprite());
    }
    return 0;
}

int VIPointLight::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VIPointLight *s = (VIPointLight *)sc.Sprite(i);
    s->m_bbox = m_bbox;
    s->m_bsphere = m_bsphere;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
    s->m_color = m_color;
    return i;
}

int VIPointLight::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                          VISoundDevice *sd, VICollide &c)
{
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }
    return 0;
}

int VIPointLight::ClampScale(VIMatrix44 &m)
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

int VIPointLight::SetColor(VIColor32F &c)
{
    m_color = c;
    return 0;
}

VIColor32F VIPointLight::GetColor() const
{
    return m_color;
}
