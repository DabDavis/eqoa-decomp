// VIFrustum -- SUPPORT 0x00442CC8 .. 0x00443268.
#include "vi.h"

int VIFrustum::Init()
{
    return 0;
}

int VIFrustum::CalcPlanes()
{
    VIVect3 v[4];

    v[0].x = m_verts[3].x;
    v[0].y = m_verts[3].y;
    v[0].z = m_verts[3].z;
    v[1].x = m_verts[2].x;
    v[1].y = m_verts[2].y;
    v[1].z = m_verts[2].z;
    v[2].x = m_verts[1].x;
    v[2].y = m_verts[1].y;
    v[2].z = m_verts[1].z;
    v[3].x = m_verts[0].x;
    v[3].y = m_verts[0].y;
    v[3].z = m_verts[0].z;
    m_planes[0](v[2], v[1], v[0]);

    v[0].x = m_verts[6].x;
    v[0].y = m_verts[6].y;
    v[0].z = m_verts[6].z;
    v[1].x = m_verts[7].x;
    v[1].y = m_verts[7].y;
    v[1].z = m_verts[7].z;
    v[2].x = m_verts[4].x;
    v[2].y = m_verts[4].y;
    v[2].z = m_verts[4].z;
    v[3].x = m_verts[5].x;
    v[3].y = m_verts[5].y;
    v[3].z = m_verts[5].z;
    m_planes[1](v[2], v[1], v[0]);

    v[0].x = m_verts[7].x;
    v[0].y = m_verts[7].y;
    v[0].z = m_verts[7].z;
    v[1].x = m_verts[3].x;
    v[1].y = m_verts[3].y;
    v[1].z = m_verts[3].z;
    v[2].x = m_verts[0].x;
    v[2].y = m_verts[0].y;
    v[2].z = m_verts[0].z;
    v[3].x = m_verts[4].x;
    v[3].y = m_verts[4].y;
    v[3].z = m_verts[4].z;
    m_planes[2](v[2], v[1], v[0]);

    v[0].x = m_verts[2].x;
    v[0].y = m_verts[2].y;
    v[0].z = m_verts[2].z;
    v[1].x = m_verts[6].x;
    v[1].y = m_verts[6].y;
    v[1].z = m_verts[6].z;
    v[2].x = m_verts[5].x;
    v[2].y = m_verts[5].y;
    v[2].z = m_verts[5].z;
    v[3].x = m_verts[1].x;
    v[3].y = m_verts[1].y;
    v[3].z = m_verts[1].z;
    m_planes[3](v[2], v[1], v[0]);

    v[0].x = m_verts[0].x;
    v[0].y = m_verts[0].y;
    v[0].z = m_verts[0].z;
    v[1].x = m_verts[1].x;
    v[1].y = m_verts[1].y;
    v[1].z = m_verts[1].z;
    v[2].x = m_verts[5].x;
    v[2].y = m_verts[5].y;
    v[2].z = m_verts[5].z;
    v[3].x = m_verts[4].x;
    v[3].y = m_verts[4].y;
    v[3].z = m_verts[4].z;
    m_planes[4](v[2], v[1], v[0]);

    v[0].x = m_verts[7].x;
    v[0].y = m_verts[7].y;
    v[0].z = m_verts[7].z;
    v[1].x = m_verts[6].x;
    v[1].y = m_verts[6].y;
    v[1].z = m_verts[6].z;
    v[2].x = m_verts[2].x;
    v[2].y = m_verts[2].y;
    v[2].z = m_verts[2].z;
    v[3].x = m_verts[3].x;
    v[3].y = m_verts[3].y;
    v[3].z = m_verts[3].z;
    m_planes[5](v[2], v[1], v[0]);
    return 0;
}

int VIFrustum::Cull(const VIVect3 &p, float r) const
{
    const VIPlane *pl = m_planes;

    for (int i = 0; i < 6; i++)
    {
        float d = p.x * pl->n.x + p.y * pl->n.y + p.z * pl->n.z + pl->d;
        if (r < d)
            return 1;
        pl++;
    }
    return 0;
}

int VIFrustum::Cull(const VISphere &s) const
{
    const VIPlane *pl = m_planes;

    for (int i = 0; i < 6; i++)
    {
        float d = s.c.x * pl->n.x + s.c.y * pl->n.y + s.c.z * pl->n.z + pl->d;
        if (s.r < d)
            return 1;
        pl++;
    }
    return 0;
}

// Open at 19/66.  The target does NOT hoist the four plane loads out of the
// inner loop and enters the loop by jumping into the MIDDLE of its body, with
// the first iteration's head duplicated in the preheader and its vertex
// addresses folded to sp.  Writing the j-exhausted exit as a `return 1` from
// inside the loop reproduces the un-hoisted body exactly -- gcc will not move a
// MEM out of a loop whose body is reachable past a conditional exit -- but it
// costs the post-loop `if (j == 8)` shape the target has.  Neither the natural
// for/break form (31/72, every plane load hoisted), a source-level peel of the
// first iteration (37/73), nor thirteen -fno- flags produce the target's shape.
int VIFrustum::Cull(const VIBBox &b, const VIMatrix44 &m) const
{
    VIVect3 v[8];

    b.Transform(m, v);

    const VIPlane *pl = m_planes;

    for (int i = 0; i < 6; i++)
    {
        const VIVect3 *q = v;
        int j = 0;

        for (;;)
        {
            if (j >= 8)
                return 1;
            if (q->x * pl->n.x + q->y * pl->n.y + q->z * pl->n.z + pl->d < 0.0f)
                break;
            j++;
            q++;
        }
        pl++;
    }
    return 0;
}

VIPlane *VIFrustum::Plane(int i) const
{
    return (VIPlane *)&m_planes[i];
}

void VIFrustum::SetPlane(int i, const VIPlane &p)
{
    m_planes[i] = p;
}

VIVect3 *VIFrustum::Vertex(int i) const
{
    return (VIVect3 *)&m_verts[i];
}

void VIFrustum::SetVertex(int i, const VIVect3 &v)
{
    m_verts[i] = v;
}
