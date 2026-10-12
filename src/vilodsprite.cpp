// VILODSprite -- SUPPORT 0x00452940 .. 0x00453A50.  Its own TU.
//
// Five (distance, sprite) pairs; every virtual walks them from level 0 while
// the level's switch distance still covers the viewer, and forwards to the
// sprite it lands on through VISprite's vtable.  Running off the end of the
// table -- i == m_numLevels -- is the sprite being too far away to draw at all,
// and every virtual returns 0 there.
//
// Raster's walk is the one with a body, and it is a COLOUR BUFFER cursor:
// every VISimpleSprite consumes one per frame, so a level that is skipped must
// still consume its own or the cursor slides for everything drawn after it.
// The two calls in it were read out of a resident SUPPORT by eqcalls.py --
// VIScene::GetSpriteType and VIScene::GetNextColorBuffer -- not inferred.
#include "vi.h"

extern "C" void *memcpy(void *, const void *, unsigned int);

VILODSprite::VILODSprite()
{
    m_type = 7;
    m_flags = 0;
}

VILODSprite::~VILODSprite()
{
}

int VILODSprite::SetVisible(int visible)
{
    if (visible)
        m_flags &= ~1;
    else
        m_flags |= 1;
    return 0;
}

int VILODSprite::SetLevelSprite(int level, int sprite, VIScene &sc)
{
    if (m_levels[level].m_sprite != -1)
    {
        sc.ReleaseSprite(m_levels[level].m_sprite);
        m_levels[level].m_sprite = -1;
    }
    m_levels[level].m_sprite = sprite;
    sc.ShareSprite(sprite);
    return 0;
}

int VILODSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    VIVect3 p;
    const VIMatrix44 *m = r.GetModelView();

    if (m_flags & 1)
        return 0;

    p.x = m->m[3].x;
    p.y = m->m[3].y;
    p.z = m->m[3].z;

    float d = p.x * p.x + p.y * p.y + p.z * p.z;
    int i = 0;
    while (m_levels[i].m_dist * m_levels[i].m_dist < d)
    {
        if (i >= m_numLevels)
            break;
        if (sc.GetSpriteType(m_levels[i].m_sprite) == 1)
            sc.GetNextColorBuffer();
        i++;
    }
    // Phase 7's lever, and the direction matters: the block gcc moves out of
    // line is the one written INSIDE the `if`, so the call goes there and the
    // `return 0` stays where the test falls through to it.
    if (i != m_numLevels)
        return sc.RasterSprite(cam, m_levels[i].m_sprite, flags);
    return 0;
}

int VILODSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return sc.Sprite(m_levels[0].m_sprite)->Collide(s, f, sc, c);
}

int VILODSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return sc.Sprite(m_levels[0].m_sprite)->Collide(r, f, sc, c);
}

int VILODSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    VIVect3 p;
    const VIMatrix44 *m = r.GetModelView();

    p.x = m->m[3].x;
    p.y = m->m[3].y;
    p.z = m->m[3].z;
    p.x -= seg.m_start.x;
    p.y -= seg.m_start.y;
    p.z -= seg.m_start.z;

    float d = p.x * p.x + p.y * p.y + p.z * p.z;
    int i = 0;
    while (m_levels[i].m_dist * m_levels[i].m_dist < d)
    {
        if (i >= m_numLevels)
            break;
        i++;
    }
    // Note the direction is the OPPOSITE of Raster's five lines above: here the
    // `return 0` goes in the `if` and the call falls through, and that is
    // measured -- swapping them costs one instruction and 12 words.
    if (i == m_numLevels)
        return 0;
    return sc.Sprite(m_levels[i].m_sprite)->Pick(seg, f, sc, r);
}

int VILODSprite::QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                   const VIMatrix44 &m, VIScene &sc)
{
    return sc.Sprite(m_levels[0].m_sprite)->QueryIntersection(b, l, m, sc);
}

int VILODSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VILODSprite *s = (VILODSprite *)sc.Sprite(i);
    s->m_bbox = m_bbox;
    s->m_bsphere = m_bsphere;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
    s->m_flags = m_flags;
    s->m_numLevels = m_numLevels;
    memcpy(s->m_levels, m_levels, sizeof(m_levels));
    for (int j = 0; j < m_numLevels; j++)
        sc.ShareSprite(s->m_levels[j].m_sprite);
    return i;
}

int VILODSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                         VISoundDevice *sd, VICollide &c)
{
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }
    for (int i = 0; i < m_numLevels; i++)
        sc.ReleaseSprite(m_levels[i].m_sprite);
    return 0;
}

int VILODSprite::GetNumLevels()
{
    return m_numLevels;
}

int VILODSprite::GetLevelSprite(int level)
{
    return m_levels[level].m_sprite;
}
