// VISimpleSprite -- SUPPORT 0x00471D60 .. 0x00472C68.  A separate TU from
// VISprite: the image emits __tf8VISprite at 0x0047ED10 (after VISprite's own
// members) and __tf14VISimpleSprite at 0x00472C18 (after VISimpleSprite's), and
// with both classes in ONE TU gcc inlines the base's __tf into the derived
// one -- 34 instructions against the target's 20.
//
// One prim buffer, one material palette, one collision object.  Every virtual
// is two or three lines that hand an index to VIRaster or VICollide, which is
// what makes it the commonest sprite class in a live scene.
#include "vi.h"

VISimpleSprite::VISimpleSprite()
{
    m_type = 1;
    m_primBuffer = -1;
    m_objIdx = -1;
    m_materialPal = -1;
}

VISimpleSprite::~VISimpleSprite()
{
}

int VISimpleSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    int lod = sc.DitherLevel();
    if (lod >= 0)
        return r.DrawPrimBuffer(m_primBuffer, lod, m_materialPal, flags);
    return r.DrawPrimBuffer(m_primBuffer, m_materialPal, flags);
}

int VISimpleSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return c.CollideObj(s, m_objIdx);
}

int VISimpleSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return c.CollideObj(r, m_objIdx);
}

int VISimpleSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    int lod = sc.DitherLevel();
    if (lod >= 0)
        return r.PickPrimBuffer(seg, m_primBuffer, lod, m_materialPal);
    return r.PickPrimBuffer(seg, m_primBuffer, m_materialPal);
}

int VISimpleSprite::QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                      const VIMatrix44 &m, VIScene &sc)
{
    VICollide *c = sc.m_collide;
    c->SetWorld(m);
    return c->QueryObj(b, l, m_objIdx);
}

int VISimpleSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VISimpleSprite *s = (VISimpleSprite *)sc.Sprite(i);
    s->m_bbox = m_bbox;
    s->m_bsphere = m_bsphere;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
    s->m_materialPal = m_materialPal;
    s->m_primBuffer = m_primBuffer;
    s->m_objIdx = m_objIdx;
    if (r)
    {
        if (r->AddRefMaterialPal(m_materialPal) < 0)
            return -1;
        if (r->AddRefPrimBuffer(m_primBuffer) < 0)
            return -1;
    }
    if (c.ShareCollBuffer(m_objIdx) < 0)
        return -1;
    return i;
}

int VISimpleSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
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
    c.ReleaseCollBuffer(m_objIdx);
    return 0;
}

int VISimpleSprite::GetPrimBuffer() const
{
    return m_primBuffer;
}

int VISimpleSprite::GetMaterialPal() const
{
    return m_materialPal;
}

int VISimpleSprite::SetMaterialPal(int pal, VIDictionary &d, VIRaster &r)
{
    r.ReleaseMaterialPal(m_materialPal);
    m_materialPal = pal;
    r.ShareMaterialPal(pal);
    return 0;
}

int VISimpleSprite::SetMaterialPalDither(int dither, VIRaster &r)
{
    for (int i = 0; i < r.MaterialPalCount(m_materialPal); i++)
        r.MaterialPalEntry(m_materialPal, i)->SetDither(dither);
    return 0;
}
