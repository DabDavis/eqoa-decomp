// VIFloraSprite -- SUPPORT 0x00440480 .. 0x004411C0.  Its own TU.
//
// VISimpleSprite with the collision half deleted: no m_objIdx, both Collide
// overloads two-instruction stubs, and no QueryIntersection override at all.
// The radial flora system puts hundreds of these on a terrain patch and nothing
// in the world is ever allowed to collide with one.
#include "vi.h"

VIFloraSprite::VIFloraSprite()
{
    m_type = 13;
    m_primBuffer = -1;
    m_materialPal = -1;
}

VIFloraSprite::~VIFloraSprite()
{
}

int VIFloraSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    return r.DrawPrimBuffer(m_primBuffer, m_materialPal, flags);
}

int VIFloraSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VIFloraSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VIFloraSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    return r.PickPrimBuffer(seg, m_primBuffer, m_materialPal);
}

int VIFloraSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VIFloraSprite *s = (VIFloraSprite *)sc.Sprite(i);
    s->m_bbox = m_bbox;
    s->m_bsphere = m_bsphere;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
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

int VIFloraSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
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

int VIFloraSprite::GetPrimBuffer() const
{
    return m_primBuffer;
}

int VIFloraSprite::GetMaterialPal() const
{
    return m_materialPal;
}

int VIFloraSprite::SetMaterialPal(int pal, VIDictionary &d, VIRaster &r)
{
    r.ReleaseMaterialPal(m_materialPal, d);
    m_materialPal = pal;
    r.ShareMaterialPal(pal);
    return 0;
}

int VIFloraSprite::SetMaterialPalDither(int dither, VIRaster &r)
{
    for (int i = 0; i < r.GetMaterialPalSize(m_materialPal); i++)
        r.GetMaterialPalEntry(m_materialPal, i)->SetDither(dither);
    return 0;
}
