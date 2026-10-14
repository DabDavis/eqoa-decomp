// VIGroupSprite -- SUPPORT 0x00443250 .. 0x00444D30.  Its own TU.
//
// Four members, each a sprite index with its own location, uniform scale and
// heading/pitch/roll, drawn and queried under one transform.  It is the only
// leaf that COMPOSES matrices: every virtual reads the caller's current
// transform, and for each member builds the member transform, multiplies it on,
// pushes the product back and forwards to the member's own virtual.
//
// Which transform "the caller's current" is depends on the query -- the raster
// and pick paths go through VIRaster's model-view, the collide paths through
// VICollide's world, and the two Query paths take one as an argument and touch
// neither.  All three matrix locals are declared base, member, product, which
// is the order the three destructor calls run in reverse.
#include "vi.h"

extern "C" void *memcpy(void *, const void *, unsigned int);

VIGroupSprite::VIGroupSprite()
{
    m_type = 11;
}

VIGroupSprite::~VIGroupSprite()
{
}

int VIGroupSprite::GetInstType(VIScene &sc) const
{
    for (int i = 0; i < m_numMembers; i++)
    {
        if (sc.Sprite(m_members[i].m_sprite)->GetInstType(sc) == 1)
            return 1;
    }
    return 0;
}

int VIGroupSprite::CalcMemberTransform(int i, VIMatrix44 &m)
{
    m.Transform(m_members[i].m_loc, m_members[i].m_scale, m_members[i].m_hpr);
    return 0;
}

int VIGroupSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    VIMatrix44 base;
    VIMatrix44 mem;
    VIMatrix44 out;

    sceVu0CopyMatrix(base, *r.GetModelView());

    int hit = 0;
    for (int i = 0; i < m_numMembers; i++)
    {
        mem.Transform(m_members[i].m_loc, m_members[i].m_scale, m_members[i].m_hpr);
        out.Mul(mem, base);
        r.SetModelView(out);
        if (sc.RasterSprite(cam, m_members[i].m_sprite, flags))
            hit = 1;
    }
    return hit;
}

int VIGroupSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    VIMatrix44 base;
    VIMatrix44 mem;
    VIMatrix44 out;

    sceVu0CopyMatrix(base, *c.GetWorld());

    for (int i = 0; i < m_numMembers; i++)
    {
        mem.Transform(m_members[i].m_loc, m_members[i].m_scale, m_members[i].m_hpr);
        out.Mul(mem, base);
        c.SetWorld(out);
        if (sc.Sprite(m_members[i].m_sprite)->Collide(s, f, sc, c))
            return 1;
    }
    return 0;
}

int VIGroupSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    VIMatrix44 base;
    VIMatrix44 mem;
    VIMatrix44 out;

    sceVu0CopyMatrix(base, *c.GetWorld());

    for (int i = 0; i < m_numMembers; i++)
    {
        mem.Transform(m_members[i].m_loc, m_members[i].m_scale, m_members[i].m_hpr);
        out.Mul(mem, base);
        c.SetWorld(out);
        if (sc.Sprite(m_members[i].m_sprite)->Collide(r, f, sc, c))
            return 1;
    }
    return 0;
}

int VIGroupSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    VIMatrix44 base;
    VIMatrix44 mem;
    VIMatrix44 out;

    sceVu0CopyMatrix(base, *r.GetModelView());

    for (int i = 0; i < m_numMembers; i++)
    {
        mem.Transform(m_members[i].m_loc, m_members[i].m_scale, m_members[i].m_hpr);
        out.Mul(mem, base);
        r.SetModelView(out);
        if (sc.Sprite(m_members[i].m_sprite)->Pick(seg, f, sc, r))
            return 1;
    }
    return 0;
}

int VIGroupSprite::QueryEmitters(VIScene &sc, const VIMatrix44 &m, VIEmitterQuery &q)
{
    VIMatrix44 mem;
    VIMatrix44 out;

    for (int i = 0; i < m_numMembers; i++)
    {
        mem.Transform(m_members[i].m_loc, m_members[i].m_scale, m_members[i].m_hpr);
        out.Mul(mem, m);
        q.m_b = m_members[i].m_sprite;
        if (sc.Sprite(m_members[i].m_sprite)->QueryEmitters(sc, out, q) < 0)
            return -1;
    }
    return 0;
}

int VIGroupSprite::QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                     const VIMatrix44 &m, VIScene &sc)
{
    VIMatrix44 mem;
    VIMatrix44 out;

    for (int i = 0; i < m_numMembers; i++)
    {
        mem.Transform(m_members[i].m_loc, m_members[i].m_scale, m_members[i].m_hpr);
        out.Mul(mem, m);
        if (sc.Sprite(m_members[i].m_sprite)->QueryIntersection(b, l, out, sc) < 0)
            return -1;
    }
    return 0;
}

int VIGroupSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VIGroupSprite *s = (VIGroupSprite *)sc.Sprite(i);
    s->m_bbox = m_bbox;
    s->m_bsphere = m_bsphere;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
    s->m_numMembers = m_numMembers;
    memcpy(s->m_members, m_members, sizeof(m_members));
    // ONE counter for both walks, at function scope (phase 61's k lever).
    int j;
    for (j = 0; j < m_numMembers; j++)
    {
        if (sc.Sprite(m_members[j].m_sprite)->GetInstType(sc) == 1)
        {
            s->m_members[j].m_sprite = sc.CopySprite(m_members[j].m_sprite, n);
            if (s->m_members[j].m_sprite == -1)
                return -1;
        }
    }
    for (j = 0; j < m_numMembers; j++)
        sc.ShareSprite(s->m_members[j].m_sprite);
    return i;
}

int VIGroupSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                           VISoundDevice *sd, VICollide &c)
{
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }
    for (int i = 0; i < m_numMembers; i++)
        sc.ReleaseSprite(m_members[i].m_sprite);
    return 0;
}

int VIGroupSprite::GetNumMembers()
{
    return m_numMembers;
}

int VIGroupSprite::GetMemberSprite(int i)
{
    return m_members[i].m_sprite;
}
