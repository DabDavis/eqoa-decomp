// VISkinSprite -- SUPPORT 0x00472C68 .. 0x00473978.  Its own TU: __tf12VISkinSprite
// is 19 instructions, the shape a derived __tf has when the base's is NOT in the
// same TU (Phase 19 measured 34 against 20 when they share one).
//
// The simplest leaf in the hierarchy.  One prim buffer, one material palette, a
// visibility bit -- and no collision object at all: both Collide overloads are
// two-instruction `return 0` stubs, so a skinned mesh is invisible to the
// collider even though it is drawn and picked.
#include "vi.h"

VISkinSprite::VISkinSprite()
{
    m_type = 3;
    m_flags = 0;
}

VISkinSprite::~VISkinSprite()
{
}

// bit 0 is HIDDEN.  SetVisible(1) clears it, SetVisible(0) sets it, and the
// mask is `lui 0xffff`+`ori 0xfffe` -- Phase 15's tell that the word is
// unsigned.
int VISkinSprite::SetVisible(int visible)
{
    if (visible)
        m_flags &= ~1;
    else
        m_flags |= 1;
    return 0;
}

int VISkinSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    if (!(m_flags & 1))
        return r.DrawPrimBuffer(m_primBuffer, m_materialPal, flags);
    return 0;
}

int VISkinSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VISkinSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VISkinSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    return r.PickPrimBuffer(seg, m_primBuffer, m_materialPal);
}

int VISkinSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VISkinSprite *s = (VISkinSprite *)sc.Sprite(i);
    s->m_bbox = m_bbox;
    s->m_bsphere = m_bsphere;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
    s->m_flags = m_flags;
    s->m_materialPal = m_materialPal;
    s->m_primBuffer = m_primBuffer;
    if (r)
    {
        if (r->ShareMaterialPal(m_materialPal) < 0)
            return -1;
        if (r->SharePrimBuffer(m_primBuffer) < 0)
            return -1;
    }
    return i;
}

int VISkinSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                          VISoundDevice *sd, VICollide &c)
{
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }
    if (r)
    {
        r->ReleasePrimBuffer(m_primBuffer, d);
        r->ReleaseMaterialPal(m_materialPal, d);
    }
    return 0;
}

int VISkinSprite::GetMaterialPal() const
{
    return m_materialPal;
}

int VISkinSprite::SetMaterialPal(int pal, VIRaster &r, VIDictionary &d)
{
    if (r.ReleaseMaterialPal(m_materialPal, d) < 0)
        return -1;
    m_materialPal = pal;
    if (r.ShareMaterialPal(pal) < 0)
        return -1;
    return 0;
}
