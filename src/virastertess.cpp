// VIRasterTess -- SUPPORT 0x004138C8 .. 0x00414610, phase 61.  The point-light
// tessellator VIRaster::DrawPrimBufferTessPackVUNCG feeds one triangle at a
// time.
//
// A triangle is clipped through a fixed BSP of sixteen axis planes -- y = 0,
// x = 0, z = 0 and then the +-7 slabs -- with Sutherland-Hodgman (Split), and
// every piece that reaches a leaf is turned from a fan into strip order and
// appended to m_out (StoreResult).  The front halves ping-pong between m_bufA
// and m_bufB, the back halves stack up in m_work, one m_stack entry per level.
// Its constructor is in the raster's own TU, where VIRaster's roll-call
// instantiates it.
#include "vi.h"

int VIRasterTess::Init()
{
    m_depth = -1;
    m_numStrips = 0;
    InitTree();
    return 0;
}

int VIRasterTess::InitTree()
{
    m_nodes[0].m_plane.n.Set(0.0f, 1.0f, 0.0f);
    m_nodes[0].m_plane.d = 0.0f;
    m_nodes[1].m_plane.n.Set(1.0f, 0.0f, 0.0f);
    m_nodes[1].m_plane.d = 0.0f;
    m_nodes[2].m_plane.n.Set(0.0f, 0.0f, 1.0f);
    m_nodes[2].m_plane.d = 0.0f;
    m_nodes[3].m_plane.n.Set(0.0f, 0.0f, 1.0f);
    m_nodes[3].m_plane.d = 0.0f;
    m_nodes[4].m_plane.n.Set(1.0f, 0.0f, 0.0f);
    m_nodes[4].m_plane.d = -7.0f;
    m_nodes[5].m_plane.n.Set(1.0f, 0.0f, 0.0f);
    m_nodes[5].m_plane.d = -7.0f;
    m_nodes[6].m_plane.n.Set(-1.0f, 0.0f, 0.0f);
    m_nodes[6].m_plane.d = -7.0f;
    m_nodes[7].m_plane.n.Set(-1.0f, 0.0f, 0.0f);
    m_nodes[7].m_plane.d = -7.0f;
    m_nodes[8].m_plane.n.Set(0.0f, 0.0f, 1.0f);
    m_nodes[8].m_plane.d = -7.0f;
    m_nodes[9].m_plane.n.Set(0.0f, 0.0f, 1.0f);
    m_nodes[9].m_plane.d = -7.0f;
    m_nodes[10].m_plane.n.Set(0.0f, 0.0f, -1.0f);
    m_nodes[10].m_plane.d = -7.0f;
    m_nodes[11].m_plane.n.Set(0.0f, 0.0f, -1.0f);
    m_nodes[11].m_plane.d = -7.0f;
    m_nodes[12].m_plane.n.Set(0.0f, 0.0f, 1.0f);
    m_nodes[12].m_plane.d = -7.0f;
    m_nodes[13].m_plane.n.Set(0.0f, 0.0f, 1.0f);
    m_nodes[13].m_plane.d = -7.0f;
    m_nodes[14].m_plane.n.Set(0.0f, 0.0f, -1.0f);
    m_nodes[14].m_plane.d = -7.0f;
    m_nodes[15].m_plane.n.Set(0.0f, 0.0f, -1.0f);
    m_nodes[15].m_plane.d = -7.0f;
    m_nodes[0].m_left = 1;
    m_nodes[0].m_right = 1;
    int k = 2;
    int i;
    for (i = 1; i < 8; i++)
    {
        m_nodes[i].m_left = k++;
        m_nodes[i].m_right = k++;
    }
    for (i = 8; i < 16; i++)
    {
        m_nodes[i].m_left = -1;
        m_nodes[i].m_right = -1;
    }
    return 0;
}

int VIRasterTess::Tessellate(int n, VITessVertex *v)
{
    m_depth = -1;
    m_numStrips = 0;
    Tessellate(0, n, v);
    return 0;
}

// One level: split the run by this node's plane, recurse into (or store) the
// front half, then the back half.  The front half goes to whichever of
// m_bufA / m_bufB the input is not; the back half is appended to m_work at
// the top of the stack.
int VIRasterTess::Tessellate(int node, int n, VITessVertex *v)
{
    int nf;
    VITessVertex *front;
    if (v == m_bufA)
    {
        nf = 0;
        front = m_bufB;
    }
    else
    {
        nf = 0;
        front = m_bufA;
    }
    if (m_depth != -1)
    {
        m_depth++;
        m_stack[m_depth].m_start = m_stack[m_depth - 1].m_start +
                                   m_stack[m_depth - 1].m_count;
        m_stack[m_depth].m_count = 0;
    }
    else
    {
        m_depth = 0;
        m_stack[0].m_start = 0;
        m_stack[0].m_count = 0;
    }
    Split(n, v, m_nodes[node].m_plane, nf, front, m_stack[m_depth].m_count,
          &m_work[m_stack[m_depth].m_start]);
    if (m_nodes[node].m_left != -1)
    {
        if (nf >= 3)
            Tessellate(m_nodes[node].m_left, nf, front);
    }
    else
    {
        if (nf >= 3)
            StoreResult(nf, front);
    }
    if (m_nodes[node].m_right != -1)
    {
        if (m_stack[m_depth].m_count >= 3)
            Tessellate(m_nodes[node].m_right, m_stack[m_depth].m_count,
                       &m_work[m_stack[m_depth].m_start]);
    }
    else
    {
        if (m_stack[m_depth].m_count >= 3)
            StoreResult(m_stack[m_depth].m_count,
                        &m_work[m_stack[m_depth].m_start]);
    }
    m_depth--;
    return 0;
}

// Sutherland-Hodgman against one plane.  A vertex ON the plane goes to both
// halves; an edge that crosses it emits the interpolated vertex to both.
// The interpolation is written per member type: each helper's reference
// parameters are what give the sub-objects addresses of their own.
static inline void LerpV3(VIVect3 &o, const VIVect3 &a, const VIVect3 &b,
                          float t)
{
    o.x = a.x + (b.x - a.x) * t;
    o.y = a.y + (b.y - a.y) * t;
    o.z = a.z + (b.z - a.z) * t;
}

static inline void LerpV2(VIVect2 &o, const VIVect2 &a, const VIVect2 &b,
                          float t)
{
    o.x = a.x + (b.x - a.x) * t;
    o.y = a.y + (b.y - a.y) * t;
}

static inline void LerpC(VIColor32F &o, const VIColor32F &a,
                         const VIColor32F &b, float t)
{
    o.r = a.r + (b.r - a.r) * t;
    o.g = a.g + (b.g - a.g) * t;
    o.b = a.b + (b.b - a.b) * t;
    o.a = a.a + (b.a - a.a) * t;
}

int VIRasterTess::Split(int n, VITessVertex *v, VIPlane &p, int &nf,
                        VITessVertex *front, int &nb, VITessVertex *back)
{
    nf = 0;
    nb = 0;
    VITessVertex *prev = v + n - 1;
    float dp = p.Distance(prev->m_pos);
    float t;
    // A cursor of its own: walking the parameter itself hands the incoming
    // register to the strength-reduced +24 pointer instead.
    VITessVertex *c = v;
    for (; n; n--)
    {
        float d = p.Distance(c->m_pos);
        if (dp > 0.0f)
        {
            *front = *prev;
            front++;
            nf++;
        }
        else if (dp < 0.0f)
        {
            *back = *prev;
            back++;
            nb++;
        }
        else
        {
            *front = *prev;
            front++;
            nf++;
            *back = *prev;
            back++;
            nb++;
        }
        if (dp < 0.0f && d > 0.0f)
        {
            t = dp / (dp - d);
            LerpV3(front->m_pos, prev->m_pos, c->m_pos, t);
            LerpV3(front->m_normal, prev->m_normal, c->m_normal, t);
            LerpV2(front->m_uv, prev->m_uv, c->m_uv, t);
            LerpC(front->m_color, prev->m_color, c->m_color, t);
            *back = *front;
            front++;
            back++;
            nf++;
            nb++;
        }
        else if (d < 0.0f && dp > 0.0f)
        {
            t = d / (d - dp);
            LerpV3(front->m_pos, c->m_pos, prev->m_pos, t);
            LerpV3(front->m_normal, c->m_normal, prev->m_normal, t);
            LerpV2(front->m_uv, c->m_uv, prev->m_uv, t);
            LerpC(front->m_color, c->m_color, prev->m_color, t);
            *back = *front;
            front++;
            back++;
            nf++;
            nb++;
        }
        prev = c;
        dp = d;
        c++;
    }
    return 0;
}

// A leaf piece, fan order in, strip order out: v0, v1, then alternately the
// last remaining vertex and the next from the front.
int VIRasterTess::StoreResult(int n, VITessVertex *v)
{
    if (n >= 3)
    {
        m_strips[m_numStrips].m_count = n;
        if (m_numStrips)
            m_strips[m_numStrips].m_start = m_strips[m_numStrips - 1].m_start +
                                            m_strips[m_numStrips - 1].m_count;
        else
            m_strips[0].m_start = 0;
        int k = m_strips[m_numStrips].m_start;
        VITessVertex *back = v + n;
        m_out[k++] = *v;
        v++;
        m_out[k++] = *v;
        v++;
        for (n -= 2; n > 0; n--)
        {
            m_out[k] = back[-1];
            back--;
            k++;
            if (--n == 0)
                break;
            m_out[k] = *v;
            v++;
            k++;
        }
        m_numStrips++;
    }
    return 0;
}
