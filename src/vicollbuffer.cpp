// VICollBuffer -- SUPPORT 0x004887F8 .. 0x00489250.
//
// The collision geometry container, and the second class whose semantics were
// settled before the decomp: `go-eqoa-vi/vicoll/collbuffer.go` is a
// console-checked port of the READER half.  What is new here is the WRITER --
// the cursor pair per section, the packing switch, and the one allocation the
// three sections are carved out of.
#include "vi.h"

VICollBuffer::VICollBuffer()
{
    m_data = 0;
    Clear();
}

VICollBuffer::~VICollBuffer()
{
    Clear();
}

int VICollBuffer::Clear()
{
    if (m_data)
        delete [] m_data;
    // m_data is zeroed LAST, and that is readable off the object code: gcc's
    // post-reload scheduler hoists the block's final store to the front, so
    // the FIRST store emitted after the join names the LAST source statement.
    // Every other spelling puts some other field at the head of the run.
    m_floraType = -1;
    m_type = 0;
    m_locked = 0;
    m_scale = 0.0f;
    m_vertexGroup = 0;
    m_numGroups = 0;
    m_maxGroups = 0;
    m_numPrims = 0;
    m_maxPrims = 0;
    m_numVerts = 0;
    m_maxVerts = 0;
    m_groups = 0;
    m_prims = 0;
    m_verts = 0;
    m_dataSize = 0;
    m_data = 0;
    return 0;
}

int VICollBuffer::Init(VICollBufferType t, int groups, int prims, int verts)
{
    m_maxGroups = groups;
    m_maxPrims = prims;
    m_maxVerts = verts;
    m_type = t;
    if (t == VICollBufferTypePackVG)
        m_type = VICollBufferTypePackVGF;
    if (CalcDataSize() < 0)
        return -1;
    if (Allocate() < 0)
        return -1;
    m_groups = (VICollGroup *)((int)m_data + (int)m_groups);
    m_prims = (VICollPrim *)((int)m_data + (int)m_prims);
    m_verts = (char *)((int)m_data + (int)m_verts);
    return 0;
}

int VICollBuffer::Lock()
{
    if (m_locked)
        return -1;
    m_locked = 1;
    return 0;
}

int VICollBuffer::Unlock()
{
    if (!m_locked)
        return -1;
    m_locked = 0;
    return 0;
}

int VICollBuffer::SetPacking(int n)
{
    m_scale = 1.0f / powf(2.0f, (float)n);
    return 0;
}

int VICollBuffer::BeginPrimGroup()
{
    m_groups[m_numGroups].m_numPrims = 0;
    m_groups[m_numGroups].m_numVerts = 0;
    return 0;
}

int VICollBuffer::EndPrimGroup()
{
    m_numGroups++;
    return 0;
}

int VICollBuffer::Begin(int mat)
{
    switch (m_type)
    {
    case VICollBufferTypeV:
        m_prims[m_numPrims].m_numVerts = 0;
        m_prims[m_numPrims].m_matIdx = mat;
        break;
    case VICollBufferTypePackV:
        m_prims[m_numPrims].m_numVerts = 0;
        m_prims[m_numPrims].m_matIdx = mat;
        break;
    case VICollBufferTypePackVGF:
        m_prims[m_numPrims].m_numVerts = 0;
        m_prims[m_numPrims].m_matIdx = mat;
        break;
    default:
        return -1;
    }
    return 0;
}

int VICollBuffer::Vertex(VIVect3 &v)
{
    switch (m_type)
    {
    case VICollBufferTypeV:
        *(VIVect3 *)(m_verts + m_numVerts * 12) = v;
        break;
    default:
        return -1;
    }
    m_prims[m_numPrims].m_numVerts++;
    m_numVerts++;
    return 0;
}

int VICollBuffer::Vertex(VIVect3S16 &v)
{
    // Unlike the VIVect3 overload above, the two counter bumps are written
    // into EACH case and cross-jumped, not shared after the switch.  Shared,
    // the `li 28` they need stays in the merged tail; duplicated, it is
    // hoisted into both predecessors -- which is what the target has, and it
    // moves every register in the function by one.
    switch (m_type)
    {
    case VICollBufferTypePackV:
        *(VIVect3S16 *)(m_verts + m_numVerts * 6) = v;
        m_prims[m_numPrims].m_numVerts++;
        m_numVerts++;
        break;
    case VICollBufferTypePackVGF:
    {
        // One base kept in a local: the target reads m_verts ONCE and
        // m_numVerts three times, which is what a local plus a member read
        // gives.  Both off the member and gcc reloads it after every store.
        char *p = m_verts;
        *(VIVect3S16 *)(p + m_numVerts * 8) = v;
        *(signed char *)(p + m_numVerts * 8 + 6) = m_vertexGroup;
        *(signed char *)(p + m_numVerts * 8 + 7) = m_floraType;
        m_prims[m_numPrims].m_numVerts++;
        m_numVerts++;
        break;
    }
    default:
        return -1;
    }
    return 0;
}

int VICollBuffer::VertexGroup(signed char g)
{
    m_vertexGroup = g;
    return 0;
}

int VICollBuffer::FloraType(signed char f)
{
    m_floraType = f;
    return 0;
}

int VICollBuffer::End()
{
    switch (m_type)
    {
    case VICollBufferTypeV:
        m_groups[m_numGroups].m_numPrims++;
        m_groups[m_numGroups].m_numVerts += m_prims[m_numPrims].m_numVerts;
        m_numPrims++;
        break;
    case VICollBufferTypePackV:
        m_groups[m_numGroups].m_numPrims++;
        m_groups[m_numGroups].m_numVerts += m_prims[m_numPrims].m_numVerts;
        m_numPrims++;
        break;
    case VICollBufferTypePackVGF:
        m_groups[m_numGroups].m_numPrims++;
        m_groups[m_numGroups].m_numVerts += m_prims[m_numPrims].m_numVerts;
        m_numPrims++;
        break;
    default:
        return -1;
    }
    return 0;
}

// The bounding-volume pass: one VIBBox accumulated per prim group, one per
// prim, and the buffer's own union of the group boxes.  The `int` parameter is
// UNUSED -- a1 is written before it is ever read -- and `p` is the vertex-group
// offset table VICollide keeps at +0x90, the same table the walkers index.
int VICollBuffer::CalcBVolumes(int n, VIVect3 *p)
{
    VIBBox gb;
    VIBBox pb;
    // The stack slots go in DECLARATION order -- gi 84, pi 88, vi 92, first 96
    // -- and the initialising stores go in STATEMENT order, first before pi.
    int gi, pi, vi, first;

    first = 1;
    pi = 0;
    vi = 0;

    for (gi = 0; gi < m_numGroups; gi++)
    {
        // Two independent "first" flags: fg spans the whole group, fp resets
        // per prim.  gcc keeps them in a1 and s4 and re-establishes a1 after
        // the second call, which is what makes them separate variables and not
        // one read twice.
        int fg = 1;
        int np = m_groups[gi].m_numPrims;
        while (np != 0)
        {
            int fp = 1;
            int nv = m_prims[pi].m_numVerts;
            while (nv != 0)
            {
                switch (m_type)
                {
                case VICollBufferTypeV:
                {
                    VIVect3 *pv = (VIVect3 *)(m_verts + vi * 12);
                    gb.Bound(fg, *pv);
                    pb.Bound(fp, *pv);
                    break;
                }
                case VICollBufferTypePackV:
                {
                    VIVect3 v;
                    v.x = (float)((VIVect3S16 *)m_verts)[vi].x * m_scale;
                    v.y = (float)((VIVect3S16 *)m_verts)[vi].y * m_scale;
                    v.z = (float)((VIVect3S16 *)m_verts)[vi].z * m_scale;
                    gb.Bound(fg, v);
                    pb.Bound(fp, v);
                    break;
                }
                case VICollBufferTypePackVGF:
                {
                    VIVect3 v;
                    v.x = (float)((VICollVertVGF *)m_verts)[vi].v.x * m_scale;
                    v.y = (float)((VICollVertVGF *)m_verts)[vi].v.y * m_scale;
                    v.z = (float)((VICollVertVGF *)m_verts)[vi].v.z * m_scale;
                    signed char g = ((VICollVertVGF *)m_verts)[vi].m_group;
                    v += p[g];
                    gb.Bound(fg, v);
                    pb.Bound(fp, v);
                    break;
                }
                default:
                    return -1;
                }
                fp = 0;
                fg = 0;
                vi++;
                nv--;
            }
            m_prims[pi].m_bbox = pb;
            pi++;
            np--;
        }
        m_groups[gi].m_bbox = gb;
        if (first)
        {
            m_bbox = gb;
            first = 0;
        }
        else
        {
            m_bbox.Union(m_bbox, gb);
        }
    }
    return 0;
}

int VICollBuffer::CalcDataSize()
{
    switch (m_type)
    {
    case VICollBufferTypeV:
        m_groups = 0;
        m_prims = (VICollPrim *)(m_maxGroups * 32);
        m_verts = (char *)((int)m_prims + m_maxPrims * 28);
        m_dataSize = (int)m_verts + m_maxVerts * 12;
        break;
    case VICollBufferTypePackV:
        m_groups = 0;
        m_prims = (VICollPrim *)(m_maxGroups * 32);
        m_verts = (char *)((int)m_prims + m_maxPrims * 28);
        m_dataSize = (int)m_verts + m_maxVerts * 6;
        break;
    case VICollBufferTypePackVGF:
        m_groups = 0;
        m_prims = (VICollPrim *)(m_maxGroups * 32);
        m_verts = (char *)((int)m_prims + m_maxPrims * 28);
        m_dataSize = (int)m_verts + m_maxVerts * 8;
        break;
    default:
        return -1;
    }
    return 0;
}

int VICollBuffer::Allocate()
{
    // The nesting is load-bearing.  Written flat -- `if (m_dataSize == 0)
    // return 0; ... if (m_data == 0) return -1; return 0;` -- gcc if-converts
    // the closing test to `movn v0,zero,v1` where the target branches.  With
    // ONE `return 0` at the bottom the two exits cross-jump instead, which is
    // Phase 12's lever and is what the target does.
    if (m_dataSize)
    {
        m_data = new char[m_dataSize];
        if (m_data == 0)
            return -1;
    }
    return 0;
}
