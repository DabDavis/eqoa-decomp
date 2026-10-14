// VIWorld -- SUPPORT 0x0048E380 .. 0x004924D0, its own TU.  Phase 51.
//
// The top of the spatial hierarchy: a rectangular TERRAIN grid, an array of
// zones, and a BSP tree over the zones.  Every scene-level query -- render,
// collide, pick, insert, stream, and the five Query* -- arrives here as one
// Vars reference and is walked down the same descent, which then hands the
// leaf to the VIZone that owns it.
//
// The descent is the content, and it is written out thirteen times in THREE
// shapes:
//
//   * SEGMENT (collide sphere, collide ray, pick, the env ray) -- the signed
//     distance of BOTH sweep endpoints, neither clipped, so every node re-tests
//     the whole segment.  A slack `r` is allowed on both sides, and what `r` IS
//     is the only thing separating the three: `m_radius + 0.03125` for the
//     sphere collide, the bare `0.03125` for the ray, `0.0` for the pick.  The
//     near side is chosen by the sign of the FIRST endpoint's distance alone --
//     not by which endpoint is nearer, and not by the direction of travel.
//   * SPHERE (radial stream, proximal query, emitter query) -- one point and a
//     radius, so one distance and no near/far pair to keep.
//   * VOLUME (render, bbox stream, volume query) -- eight corners walked into a
//     front/back flag pair, and for the render the camera's own side on top.
//
// In every shape a child index is TAGGED: any bit of 0xF0000000 says "not an
// ordinary interior index", 0x40000000 on top of that says LEAF, and any other
// tag is stripped and used as an interior index anyway.
//
// The two segment distances are LOCALS, and they are ASSIGNED AGAIN after each
// recursive call.  They have to be: a local puts both dot products in one basic
// block, where the scheduler interleaves them the way the target does, and it
// cannot survive the call -- no callee-saved FP register is used and gcc does
// not rematerialise a float expression.  Spelling the expression out at each
// use instead short-circuits the second dot into the successor block.
#include "vi.h"

VIWorld::VIWorld()
{
    m_bbox.min.Set(-1000.0f, -1000.0f, -1000.0f);
    m_bbox.max.Set(1000.0f, 1000.0f, 1000.0f);
    m_numRows = 1;
    m_numCols = 1;
    m_cellSize = 2000.0f;
    m_origin.Zero();
    m_subdiv = 4;
}

VIWorld::~VIWorld()
{
}

int VIWorld::Init(int numZones, int numRows, int numCols, int subdiv,
                  float cellSize, const VIVect3 &origin, const VIBBox &bb)
{
    int r = -1;

    if (m_zones.Init(numZones) < 0)
        return -1;

    for (int i = 0; i < numZones; i++)
        if (m_zones[i].Init() < 0)
            return -1;

    if (numZones == 1)
    {
        m_zones[0].m_bbox = bb;
        m_zones[0].m_rooms[0].m_bbox = bb;
    }

    if (m_tree.Init(0, 1) < 0)
        return -1;

    m_tree.m_leaves.m_data[0].m_a = 0;

    m_numRows = numRows;
    m_numCols = numCols;
    m_cellSize = cellSize;
    m_subdiv = subdiv;
    m_origin = origin;
    m_bbox = bb;

    if (m_regions.Init(m_numRows * m_numCols * m_subdiv * m_subdiv) < 0)
        return -1;

    if (0 <= SetProfiles())
        r = 0;

    return r;
}

int VIWorld::Clear()
{
    int r = -1;

    if (m_zones.Clear() < 0)
        return -1;

    if (m_tree.Clear() < 0)
        return -1;

    if (0 <= m_regions.Clear())
        r = 0;

    return r;
}

int VIWorld::IsEmpty()
{
    if (m_tree.m_nodes.m_size == 0 && m_tree.m_leaves.m_size == 0)
        return 1;

    return 0;
}

// The two clamps are in the order COL then ROW: the register assignment says
// so, and every other order costs eleven words.
int VIWorld::GetTerrainProfile(int row, int col) const
{
    if (col < 0)
        col = 0;
    if (m_numCols * m_subdiv - 1 < col)
        col = m_numCols * m_subdiv - 1;
    if (row < 0)
        row = 0;
    if (m_numRows * m_subdiv - 1 < row)
        row = m_numRows * m_subdiv - 1;

    return m_regions.m_data[row * m_numCols * m_subdiv + col].m_flags;
}

int VIWorld::CalcTerrainProfile(const VIVect3 &at) const
{
    int row, col;

    CalcTerrainCell(at, row, col);
    return m_regions.m_data[row * m_numCols * m_subdiv + col].m_flags;
}

int VIWorld::CalcTerrainCell(const VIVect3 &at, int &row, int &col) const
{
    VIVect3 v = at;
    float   s = (float)m_subdiv / m_cellSize;

    v -= m_origin;
    v.x += m_cellSize * 0.5f;
    v.z += m_cellSize * 0.5f;

    row = (int)(v.z * s);
    col = (int)(v.x * s);

    int r = row < 0 ? 0 : row;
    if (m_numRows * m_subdiv - 1 < r)
        r = m_numRows * m_subdiv - 1;
    row = r;

    int c = col < 0 ? 0 : col;
    if (m_numCols * m_subdiv - 1 < c)
        c = m_numCols * m_subdiv - 1;
    col = c;

    return 0;
}

int VIWorld::CalcTerrainCellBVolume(int row, int col, VIBBox &bb) const
{
    float s = m_cellSize / (float)m_subdiv;

    bb.min.x = (float)col * s;
    bb.min.y = m_bbox.min.y;
    bb.min.z = (float)row * s;
    bb.max.x = (float)(col + 1) * s;
    bb.max.y = m_bbox.max.y;
    bb.max.z = (float)(row + 1) * s;

    if (m_zones.m_size == 1)
    {
        VIVect3 o(-m_origin.x, -m_origin.y, -m_origin.z);

        bb.min += o;
        bb.max += o;
    }

    return 0;
}

int VIWorld::RenderByFrustum(VISceneRendVars &v)
{
    int r = -1;

    v.m_world = this;
    v.m_p28 = 1;

    if (IsEmpty())
        return 0;

    if (v.m_clip->PushFrustum(*v.m_camera) < 0)
        return -1;

    v.m_clip->GetTop(v.m_numVerts, &v.m_verts, v.m_numPlanes, &v.m_planes);

    if (RenderByVolume(v) < 0)
        return -1;

    if (0 <= v.m_clip->Pop())
        r = 0;

    return r;
}

int VIWorld::RenderByVolume(VISceneRendVars &v)
{
    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
    {
        if (RenderByVolume(0, v) < 0)
            return -1;
    }
    else
    {
        if (RenderByVolume(0x40000000, v) < 0)
            return -1;
    }

    m_tree.m_state++;
    return 0;
}

int VIWorld::RenderByVolume(int node, VISceneRendVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;

            int zone = m_tree.m_leaves[node].m_a;

            return m_zones[zone].RenderByVolume(v, zone);
        }

        node &= 0x0FFFFFFF;
    }

    int front    = 0;
    int back     = 0;
    int nearBack = 0;
    int i;

    for (i = 0; i < v.m_numVerts; i++)
    {
        if (0.0f < m_tree.m_nodes[node].m_plane.Distance(v.m_verts[i]))
            front = 1;
        else
            back = 1;
    }

    if (m_tree.m_nodes[node].m_plane.Distance(v.m_camera->m_position) < 0.0f)
        nearBack = 1;

    if (nearBack)
    {
        if (front)
            if (RenderByVolume(m_tree.m_nodes[node].m_front, v) < 0)
                return -1;

        if (back)
            if (RenderByVolume(m_tree.m_nodes[node].m_back, v) < 0)
                return -1;
    }
    else
    {
        if (back)
            if (RenderByVolume(m_tree.m_nodes[node].m_back, v) < 0)
                return -1;

        if (front)
            if (RenderByVolume(m_tree.m_nodes[node].m_front, v) < 0)
                return -1;
    }

    return 0;
}

int VIWorld::Collide(VISceneCollVars &v)
{
    int hit;

    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
        hit = Collide(0, v) != 0;
    else
        hit = Collide(0x40000000, v) != 0;

    m_tree.m_state++;
    return hit;
}

int VIWorld::Collide(int node, VISceneCollVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;
            return m_zones[m_tree.m_leaves[node].m_a]
                       .Collide(v);
        }

        node &= 0x0FFFFFFF;
    }

    float ds = m_tree.m_nodes[node].m_plane.Distance(v.m_start);
    float de = m_tree.m_nodes[node].m_plane.Distance(v.m_end);
    int   hit;

    if (v.m_radius + 0.03125f < ds && v.m_radius + 0.03125f < de)
    {
        hit = Collide(m_tree.m_nodes[node].m_front, v) != 0;
    }
    else if (ds < -v.m_radius - 0.03125f && de < -v.m_radius - 0.03125f)
    {
        hit = Collide(m_tree.m_nodes[node].m_back, v) != 0;
    }
    else if (0.0f < ds)
    {
        hit = 0;
        if (Collide(m_tree.m_nodes[node].m_front, v))
            hit = 1;

        ds = m_tree.m_nodes[node].m_plane.Distance(v.m_start);
        de = m_tree.m_nodes[node].m_plane.Distance(v.m_end);

        if (ds < v.m_radius + 0.03125f || de < v.m_radius + 0.03125f)
            if (Collide(m_tree.m_nodes[node].m_back, v))
                hit = 1;
    }
    else
    {
        hit = 0;
        if (Collide(m_tree.m_nodes[node].m_back, v))
            hit = 1;

        ds = m_tree.m_nodes[node].m_plane.Distance(v.m_start);
        de = m_tree.m_nodes[node].m_plane.Distance(v.m_end);

        if (-v.m_radius - 0.03125f < ds || -v.m_radius - 0.03125f < de)
            if (Collide(m_tree.m_nodes[node].m_front, v))
                hit = 1;
    }

    return hit;
}

int VIWorld::Collide(VISceneCollRayVars &v)
{
    int hit;

    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
        hit = Collide(0, v) != 0;
    else
        hit = Collide(0x40000000, v) != 0;

    m_tree.m_state++;
    return hit;
}

int VIWorld::Collide(int node, VISceneCollRayVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;
            return m_zones[m_tree.m_leaves[node].m_a]
                       .Collide(v);
        }

        node &= 0x0FFFFFFF;
    }

    float ds = m_tree.m_nodes[node].m_plane.Distance(v.m_start);
    float de = m_tree.m_nodes[node].m_plane.Distance(v.m_end);
    int   hit;

    if (0.03125f < ds && 0.03125f < de)
    {
        hit = Collide(m_tree.m_nodes[node].m_front, v) != 0;
    }
    else if (ds < -0.03125f && de < -0.03125f)
    {
        hit = Collide(m_tree.m_nodes[node].m_back, v) != 0;
    }
    else if (0.0f < ds)
    {
        hit = 0;
        if (Collide(m_tree.m_nodes[node].m_front, v))
            hit = 1;

        ds = m_tree.m_nodes[node].m_plane.Distance(v.m_start);
        de = m_tree.m_nodes[node].m_plane.Distance(v.m_end);

        if (ds < 0.03125f || de < 0.03125f)
            if (Collide(m_tree.m_nodes[node].m_back, v))
                hit = 1;
    }
    else
    {
        hit = 0;
        if (Collide(m_tree.m_nodes[node].m_back, v))
            hit = 1;

        ds = m_tree.m_nodes[node].m_plane.Distance(v.m_start);
        de = m_tree.m_nodes[node].m_plane.Distance(v.m_end);

        if (-0.03125f < ds || -0.03125f < de)
            if (Collide(m_tree.m_nodes[node].m_front, v))
                hit = 1;
    }

    return hit;
}

int VIWorld::InsertActor(VISceneInsertVars &v)
{
    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
    {
        if (InsertActor(0, v) < 0)
            return -1;
    }
    else
    {
        if (InsertActor(0x40000000, v) < 0)
            return -1;
    }

    m_tree.m_state++;
    return 0;
}

int VIWorld::Pick(VIScenePickVars &v)
{
    int hit;

    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
        hit = Pick(0, v) != 0;
    else
        hit = Pick(0x40000000, v) != 0;

    m_tree.m_state++;
    return hit;
}

int VIWorld::Pick(int node, VIScenePickVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;
            return m_zones[m_tree.m_leaves[node].m_a]
                       .Pick(v);
        }

        node &= 0x0FFFFFFF;
    }

    float ds = m_tree.m_nodes[node].m_plane.Distance(v.m_start);
    float de = m_tree.m_nodes[node].m_plane.Distance(v.m_end);
    int   hit;

    if (0.0f < ds && 0.0f < de)
    {
        hit = Pick(m_tree.m_nodes[node].m_front, v) != 0;
    }
    else if (ds < 0.0f && de < 0.0f)
    {
        hit = Pick(m_tree.m_nodes[node].m_back, v) != 0;
    }
    else if (0.0f < ds)
    {
        hit = 0;
        if (Pick(m_tree.m_nodes[node].m_front, v))
            hit = 1;

        ds = m_tree.m_nodes[node].m_plane.Distance(v.m_start);
        de = m_tree.m_nodes[node].m_plane.Distance(v.m_end);

        if (ds < 0.0f || de < 0.0f)
            if (Pick(m_tree.m_nodes[node].m_back, v))
                hit = 1;
    }
    else
    {
        hit = 0;
        if (Pick(m_tree.m_nodes[node].m_back, v))
            hit = 1;

        ds = m_tree.m_nodes[node].m_plane.Distance(v.m_start);
        de = m_tree.m_nodes[node].m_plane.Distance(v.m_end);

        if (0.0f < ds || 0.0f < de)
            if (Pick(m_tree.m_nodes[node].m_front, v))
                hit = 1;
    }

    return hit;
}

int VIWorld::Stream(VISceneStreamRadialVars &v)
{
    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
    {
        if (Stream(0, v) < 0)
            return -1;
    }
    else
    {
        if (Stream(0x40000000, v) < 0)
            return -1;
    }

    m_tree.m_state++;
    return 0;
}

int VIWorld::Stream(VISceneBBoxStreamVars &v)
{
    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
    {
        if (Stream(0, v) < 0)
            return -1;
    }
    else
    {
        if (Stream(0x40000000, v) < 0)
            return -1;
    }

    m_tree.m_state++;
    return 0;
}

int VIWorld::FindLeaf(const VIVect3 &at, int &zone, int &leaf) const
{
    const VIArray<VIWorldLeaf> &leaves = m_tree.m_leaves;
    const VIArray<VIZone>      &zones  = m_zones;
    int node = 0;

    if (m_tree.m_nodes.m_size != 0)
    {
        do
        {
            float d = m_tree.m_nodes.m_data[node].m_plane.Distance(at);

            node &= 0x0FFFFFFF;

            if (0.0f <= d)
                node = m_tree.m_nodes.m_data[node].m_front;
            else
                node = m_tree.m_nodes.m_data[node].m_back;
        }
        while ((node & 0x40000000) == 0);

        node &= 0x0FFFFFFF;
    }

    zone = leaves[node].m_a;
    leaf = zones[zone].FindLeaf(at);

    if (leaf < 0)
        return -1;
    return 0;
}

int VIWorld::Collide(VIEnvRay &r)
{
    int hit;

    if (m_tree.m_nodes.m_size)
        hit = Collide(0, r, r.m_start, r.m_end) != 0;
    else
        hit = Collide(0x40000000, r, r.m_start, r.m_end) != 0;

    return hit;
}

int VIWorld::QueryProximal(VISceneProxQueryVars &v)
{
    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
    {
        if (QueryProximal(0, v) < 0)
            return -1;
    }
    else
    {
        if (QueryProximal(0x40000000, v) < 0)
            return -1;
    }

    m_tree.m_state++;
    return 0;
}

int VIWorld::QueryVisible(VISceneVisQueryVars &v)
{
    v.m_stamp = m_tree.m_state;
    v.m_depth++;

    if (m_tree.m_nodes.m_size)
    {
        if (QueryVisible(0, v) < 0)
            return -1;
    }
    else
    {
        if (QueryVisible(0x40000000, v) < 0)
            return -1;
    }

    v.m_depth = 0;
    m_tree.m_state++;
    return 0;
}

int VIWorld::QueryVolume(VISceneBBoxQueryVars &v)
{
    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
    {
        if (QueryVolume(0, v) < 0)
            return -1;
    }
    else
    {
        if (QueryVolume(0x40000000, v) < 0)
            return -1;
    }

    m_tree.m_state++;
    return 0;
}

int VIWorld::QueryEmitters(VISceneEmitterQueryVars &v)
{
    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
    {
        if (QueryEmitters(0, v) < 0)
            return -1;
    }
    else
    {
        if (QueryEmitters(0x40000000, v) < 0)
            return -1;
    }

    m_tree.m_state++;
    return 0;
}

int VIWorld::QueryIntersection(VISceneIntersectVars &v)
{
    v.m_stamp = m_tree.m_state;

    if (m_tree.m_nodes.m_size)
    {
        if (QueryIntersection(0, v) < 0)
            return -1;
    }
    else
    {
        if (QueryIntersection(0x40000000, v) < 0)
            return -1;
    }

    m_tree.m_state++;
    return 0;
}

int VIWorld::InsertActor(int node, VISceneInsertVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;

            v.m_zone = m_tree.m_leaves[node].m_a;

            if (m_zones[m_tree.m_leaves[node].m_a].InsertActor(v) < 0)
                return -1;

            return 0;
        }

        node &= 0x0FFFFFFF;
    }

    float d = m_tree.m_nodes[node].m_plane.Distance(v.m_bound.c);

    if (v.m_bound.r < d)
    {
        if (InsertActor(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;
    }
    else if (d < -v.m_bound.r)
    {
        if (InsertActor(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;
    }
    else
    {
        int front = 0;
        int back  = 0;
        int i;

        for (i = 0; i < 8; i++)
        {
            if (0.0f < m_tree.m_nodes[node].m_plane.Distance(v.m_corners[i]))
                front = 1;
            else
                back = 1;
        }

        if (front)
            if (InsertActor(m_tree.m_nodes[node].m_front, v) < 0)
                return -1;

        if (back)
            if (InsertActor(m_tree.m_nodes[node].m_back, v) < 0)
                return -1;
    }

    return 0;
}

int VIWorld::Stream(int node, VISceneStreamRadialVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return StreamLeaf(node & 0x0FFFFFFF, v);

        node &= 0x0FFFFFFF;
    }

    int front     = 1;
    int back      = 1;
    int nearFront = 0;

    float d = m_tree.m_nodes[node].m_plane.Distance(v.m_at);

    if (d < -v.m_radius)
        front = 0;

    if (v.m_radius < d)
        back = 0;

    if (0.0f < d)
        nearFront = 1;

    if (nearFront)
    {
        if (front)
            if (Stream(m_tree.m_nodes[node].m_front, v) < 0)
                return -1;

        if (back)
            if (Stream(m_tree.m_nodes[node].m_back, v) < 0)
                return -1;
    }
    else
    {
        if (back)
            if (Stream(m_tree.m_nodes[node].m_back, v) < 0)
                return -1;

        if (front)
            if (Stream(m_tree.m_nodes[node].m_front, v) < 0)
                return -1;
    }

    return 0;
}

int VIWorld::StreamLeaf(int leaf, VISceneStreamRadialVars &v)
{
    int zone = m_tree.m_leaves[leaf].m_a;

    float d = m_zones[zone].m_bbox.DistanceSquared(v.m_at);

    if (m_zones[zone].m_flags & 1)
    {
        if (d < v.m_inner * v.m_inner)
        {
            if (m_zones[zone].m_trans == -1)
            {
                m_zones[zone].m_trans =
                    v.m_loader->Load(v.m_file, 0, m_zones[zone].m_offset,
                                     m_zones[zone].m_size, zone);

                if (m_zones[zone].m_trans < 0)
                    return -1;
            }
            else if (v.m_loader->IsComplete(m_zones[zone].m_trans))
            {
                if (v.m_loader->Release(m_zones[zone].m_trans, *v.m_scene) < 0)
                    return -1;

                m_zones[zone].m_trans = -1;
                m_zones[zone].m_flags &= 0xFFFFFFFE;
            }
        }
        else if (m_zones[zone].m_trans != -1)
        {
            if (v.m_loader->Release(m_zones[zone].m_trans, *v.m_scene) < 0)
                return -1;

            m_zones[zone].m_trans = -1;
        }
    }

    if (!(m_zones[zone].m_flags & 1))
    {
        if (v.m_outer * v.m_outer < d)
        {
            VIArray<int> a;

            if (v.m_scene->ReleaseStatic(m_zones[zone], *v.m_loader) < 0)
                return -1;

            if (v.m_scene->RemoveActorsFromZone(m_zones[zone], a) < 0)
                return -1;

            if (m_zones[zone].ResetToProxy(*v.m_scene) < 0)
                return -1;

            if (v.m_scene->UpdateActors(a) < 0)
                return -1;
        }
        else
        {
            v.m_zone = zone;

            if (m_zones[zone].Stream(v) < 0)
                return -1;

            v.m_zone = -1;
        }
    }

    return 0;
}

int VIWorld::QueryProximal(int node, VISceneProxQueryVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;
            return m_zones[m_tree.m_leaves[node].m_a].QueryProximal(v);
        }

        node &= 0x0FFFFFFF;
    }

    float d = m_tree.m_nodes[node].m_plane.Distance(v.m_sphere.c);

    if (v.m_sphere.r < d)
    {
        if (QueryProximal(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;
    }
    else if (d < -v.m_sphere.r)
    {
        if (QueryProximal(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;
    }
    else if (0.0f < d)
    {
        if (QueryProximal(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;

        if (QueryProximal(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;
    }
    else
    {
        if (QueryProximal(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;

        if (QueryProximal(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;
    }

    return 0;
}

int VIWorld::QueryEmitters(int node, VISceneEmitterQueryVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;
            return m_zones[m_tree.m_leaves[node].m_a].QueryEmitters(v);
        }

        node &= 0x0FFFFFFF;
    }

    float d = m_tree.m_nodes[node].m_plane.Distance(v.m_sphere.c);

    if (v.m_sphere.r < d)
    {
        if (QueryEmitters(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;
    }
    else if (d < -v.m_sphere.r)
    {
        if (QueryEmitters(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;
    }
    else if (0.0f < d)
    {
        if (QueryEmitters(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;

        if (QueryEmitters(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;
    }
    else
    {
        if (QueryEmitters(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;

        if (QueryEmitters(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;
    }

    return 0;
}

int VIWorld::Stream(int node, VISceneBBoxStreamVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return StreamLeaf(node & 0x0FFFFFFF, v);

        node &= 0x0FFFFFFF;
    }

    int front = 0;
    int back  = 0;
    int i;

    for (i = 0; i < 8; i++)
    {
        if (0.0f < m_tree.m_nodes[node].m_plane.Distance(v.m_corners[i]))
            front = 1;
        else
            back = 1;
    }

    if (front)
        if (Stream(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;

    if (back)
        if (Stream(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;

    return 0;
}

int VIWorld::QueryVolume(int node, VISceneBBoxQueryVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;
            return m_zones[m_tree.m_leaves[node].m_a].QueryVolume(v);
        }

        node &= 0x0FFFFFFF;
    }

    int front = 0;
    int back  = 0;
    int i;

    for (i = 0; i < 8; i++)
    {
        if (0.0f < m_tree.m_nodes[node].m_plane.Distance(v.m_corners[i]))
            front = 1;
        else
            back = 1;
    }

    if (front)
        if (QueryVolume(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;

    if (back)
        if (QueryVolume(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;

    return 0;
}

int VIWorld::QueryVisibleBySphere(VISceneVisQueryVars &v)
{
    VIBBox bb;
    int    r = -1;

    v.m_world = this;

    if (IsEmpty())
        return 0;

    bb.min = v.m_sphere.c;
    bb.min -= v.m_sphere.r;
    bb.max = v.m_sphere.c;
    bb.max += v.m_sphere.r;

    if (v.m_clip->PushBBox(bb) < 0)
        return -1;

    v.m_clip->GetTop(v.m_numVerts, &v.m_verts, v.m_numPlanes, &v.m_planes);

    if (QueryVisible(v) < 0)
        return -1;

    if (0 <= v.m_clip->Pop())
        r = 0;

    return r;
}

int VIWorld::QueryVisibleWithPortals(VISceneVisQueryVars &v)
{
    int zone;
    int leaf;

    v.m_world = this;

    if (!IsEmpty())
    {
        if (FindLeaf(v.m_sphere.c, zone, leaf) < 0)
            return -1;

        int room = m_zones[zone].m_tree.m_leaves[leaf].m_a;

        if (m_zones[zone].m_rooms[room].m_flags & 1)
        {
            if (QueryVisibleBySphere(v) < 0)
                return -1;
        }
        else
        {
            VIBBox bb;

            bb.min = v.m_sphere.c;
            bb.min -= v.m_sphere.r;
            bb.max = v.m_sphere.c;
            bb.max += v.m_sphere.r;

            if (v.m_clip->PushBBox(bb) < 0)
                return -1;

            v.m_clip->GetTop(v.m_numVerts, &v.m_verts, v.m_numPlanes, &v.m_planes);

            if (m_zones[zone].QueryVisibleRoom(room, v) < 0)
                return -1;

            if (v.m_clip->Pop() < 0)
                return -1;
        }
    }

    return 0;
}

// ================================================================ phase 61 --

// The env-ray descent is the fourth segment walk, and the only one that SPLITS
// the segment: a straddling node cuts it at the plane and hands each child the
// half on its side, so the slack the other three carry is not needed.  Zero
// counts as the front on the way in and as the back for the split.
int VIWorld::Collide(int node, VIEnvRay &r, VIVect3 &a, VIVect3 &b)
{
    VIVect3 mid;

    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;
            return m_zones[m_tree.m_leaves[node].m_a].Collide(r, a, b);
        }

        node &= 0x0FFFFFFF;
    }

    float da = m_tree.m_nodes[node].m_plane.Distance(a);
    float db = m_tree.m_nodes[node].m_plane.Distance(b);
    int   hit;

    if (0.0f <= da && 0.0f <= db)
        return Collide(m_tree.m_nodes[node].m_front, r, a, b) != 0;

    if (da <= 0.0f && db <= 0.0f)
        return Collide(m_tree.m_nodes[node].m_back, r, a, b) != 0;

    float t = da / (da - db);

    mid = b;
    mid -= a;
    mid *= t;
    mid += a;

    if (0.0f < da)
    {
        int one = 1;

        hit = 0;
        if (Collide(m_tree.m_nodes[node].m_front, r, a, mid))
            hit = one;
        if (hit)
            return hit;
        if (!Collide(m_tree.m_nodes[node].m_back, r, mid, b))
            one = 0;
        hit = one;
    }
    else
    {
        int one = 1;

        hit = 0;
        if (Collide(m_tree.m_nodes[node].m_back, r, a, mid))
            hit = one;
        if (hit)
            return hit;
        if (!Collide(m_tree.m_nodes[node].m_front, r, mid, b))
            one = 0;
        hit = one;
    }
    return hit;
}

// The zone's visibility descent, one level up (see vizone.cpp).
int VIWorld::QueryVisible(int node, VISceneVisQueryVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;
            return m_zones[m_tree.m_leaves[node].m_a].QueryVisible(v);
        }

        node &= 0x0FFFFFFF;
    }

    float d = m_tree.m_nodes[node].m_plane.Distance(v.m_sphere.c);

    if (v.m_sphere.r < d)
    {
        if (QueryVisible(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;
    }
    else if (d < -v.m_sphere.r)
    {
        if (QueryVisible(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;
    }
    else
    {
    int front = 0;
    int back  = 0;
    int i;

    for (i = 0; i < v.m_numVerts; i++)
    {
        d = m_tree.m_nodes[node].m_plane.Distance(v.m_verts[i]);

        if (0.0f < d)
            front = 1;
        else
            back = 1;
    }

        if (d < 0.0f)
        {
            if (front)
                if (QueryVisible(m_tree.m_nodes[node].m_front, v) < 0)
                    return -1;

            if (back)
                if (QueryVisible(m_tree.m_nodes[node].m_back, v) < 0)
                    return -1;
        }
        else
        {
            if (back)
                if (QueryVisible(m_tree.m_nodes[node].m_back, v) < 0)
                    return -1;

            if (front)
                if (QueryVisible(m_tree.m_nodes[node].m_front, v) < 0)
                    return -1;
        }
    }

    return 0;
}

int VIWorld::QueryIntersection(int node, VISceneIntersectVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;
            if (m_zones[m_tree.m_leaves[node].m_a].QueryIntersection(v) < 0)
                return -1;
            return 0;
        }

        node &= 0x0FFFFFFF;
    }

    float d = m_tree.m_nodes[node].m_plane.Distance(v.m_sphere.c);

    if (v.m_sphere.r < d)
    {
        if (QueryIntersection(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;
    }
    else if (d < -v.m_sphere.r)
    {
        if (QueryIntersection(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;
    }
    else
    {
        int front = 0;
        int back  = 0;
        int i;

        for (i = 0; i < 8; i++)
        {
            if (0.0f < m_tree.m_nodes[node].m_plane.Distance(v.m_corners[i]))
                front = 1;
            else
                back = 1;
        }

        if (front)
            if (QueryIntersection(m_tree.m_nodes[node].m_front, v) < 0)
                return -1;

        if (back)
            if (QueryIntersection(m_tree.m_nodes[node].m_back, v) < 0)
                return -1;
    }

    return 0;
}

// Portal rendering starts in the camera's own room: an outdoor room (bit 0)
// renders the world by frustum instead, an indoor one renders that room under
// the full view frustum and lets RenderRoom carry on through its portals.
int VIWorld::RenderByPortals(VISceneRendVars &v)
{
    int zone;
    int leaf;

    v.m_world = this;

    if (IsEmpty())
        return 0;

    if (FindLeaf(v.m_camera->m_position, zone, leaf) < 0)
        return -1;

    int room = m_zones[zone].m_tree.m_leaves[leaf].m_a;

    if (m_zones[zone].m_rooms[room].m_flags & 1)
    {
        if (RenderByFrustum(v) < 0)
            return -1;
    }
    else
    {
        v.m_zone = zone;

        if (m_zones[zone].SetPretranslations(*v.m_raster, *v.m_camera) < 0)
            return -1;

        if (v.m_clip->PushFrustum(*v.m_camera) < 0)
            return -1;

        v.m_clip->GetTop(v.m_numVerts, &v.m_verts, v.m_numPlanes, &v.m_planes);

        if (m_zones[zone].RenderRoom(room, v) < 0)
            return -1;

        if (v.m_clip->Pop() < 0)
            return -1;
    }

    return 0;
}

// The radial StreamLeaf with the two distance tests replaced by box overlaps:
// the zone's base data is loaded while it overlaps the inner box, and its
// streamed content is released once it leaves the outer one.
int VIWorld::StreamLeaf(int leaf, VISceneBBoxStreamVars &v)
{
    VIVect3 unused;
    int     zone = m_tree.m_leaves[leaf].m_a;

    if (m_zones[zone].m_flags & 1)
    {
        if (v.m_bbox.Intersects(m_zones[zone].m_bbox))
        {
            if (m_zones[zone].m_trans == -1)
            {
                m_zones[zone].m_trans =
                    v.m_loader->Load(v.m_file, 0, m_zones[zone].m_offset,
                                     m_zones[zone].m_size, zone);

                if (m_zones[zone].m_trans < 0)
                    return -1;
            }
            else if (v.m_loader->IsComplete(m_zones[zone].m_trans))
            {
                if (v.m_loader->Release(m_zones[zone].m_trans, *v.m_scene) < 0)
                    return -1;

                m_zones[zone].m_trans = -1;
                m_zones[zone].m_flags &= 0xFFFFFFFE;
            }
        }
        else if (m_zones[zone].m_trans != -1)
        {
            if (v.m_loader->Release(m_zones[zone].m_trans, *v.m_scene) < 0)
                return -1;

            m_zones[zone].m_trans = -1;
        }
    }

    if (!(m_zones[zone].m_flags & 1))
    {
        if (!v.m_outer.Intersects(m_zones[zone].m_bbox))
        {
            VIArray<int> a;

            if (v.m_scene->ReleaseStatic(m_zones[zone], *v.m_loader) < 0)
                return -1;

            if (v.m_scene->RemoveActorsFromZone(m_zones[zone], a) < 0)
                return -1;

            if (m_zones[zone].ResetToProxy(*v.m_scene) < 0)
                return -1;

            if (v.m_scene->UpdateActors(a) < 0)
                return -1;
        }
        else
        {
            v.m_zone = zone;

            if (m_zones[zone].Stream(v) < 0)
                return -1;

            v.m_zone = -1;
        }
    }

    return 0;
}

// The four terrain sub-cells around a point and their bilinear weights.  The
// point is shifted by half a sub-cell so that it falls between four cell
// CENTRES; on the far edge of the grid a missing neighbour repeats the cell.
int VIWorld::CalcProfileBlend(const VIVect3 &at, VITerrainProfileBlend &b)
{
    int     idx[4];
    VIVect3 v = at;
    float   s = (float)m_subdiv / m_cellSize;

    v -= m_origin;
    v.x += m_cellSize * 0.5f;
    v.z += m_cellSize * 0.5f;

    float c = 1.0f / s;
    float x = v.x - c * 0.5f;
    float z = v.z - c * 0.5f;

    int col = (int)(x * s);
    int row = (int)(z * s);

    col = col < 0 ? 0 : col;
    if (m_numCols * m_subdiv - 1 < col)
        col = m_numCols * m_subdiv - 1;

    row = row < 0 ? 0 : row;
    if (m_numRows * m_subdiv - 1 < row)
        row = m_numRows * m_subdiv - 1;

    float fx = (x - (float)col * c) * s;
    float fz = (z - (float)row * c) * s;

    fx = VIMIN(1.0f, VIMAX(0.0f, fx));
    fz = VIMIN(1.0f, VIMAX(0.0f, fz));

    idx[0] = row * m_numCols * m_subdiv + col;

    if (col != m_numCols * m_subdiv - 1)
        idx[1] = idx[0] + 1;
    else
        idx[1] = idx[0];

    if (row != m_numRows * m_subdiv - 1)
    {
        idx[2] = idx[0] + m_numCols * m_subdiv;
        idx[3] = idx[1] + m_numCols * m_subdiv;
    }
    else
    {
        idx[2] = idx[0];
        idx[3] = idx[1];
    }

    b.m_index[0] = m_regions[idx[0]].m_flags;
    b.m_index[1] = m_regions[idx[1]].m_flags;
    b.m_index[2] = m_regions[idx[2]].m_flags;
    b.m_index[3] = m_regions[idx[3]].m_flags;

    b.m_weight[0] = (1.0f - fx) * (1.0f - fz);
    b.m_weight[1] = fx * (1.0f - fz);
    b.m_weight[2] = (1.0f - fx) * fz;
    b.m_weight[3] = fx * fz;

    return 0;
}

// A colour from bytes, each channel over 255 (VIScene::InitTimeOfDay's
// idiom: the destination is a reference).
static inline void SetColorFrom(VIColor32F &d, const VIColor32 &c)
{
    d.r = c.r * (1.0f / 255.0f);
    d.g = c.g * (1.0f / 255.0f);
    d.b = c.b * (1.0f / 255.0f);
    d.a = c.a * (1.0f / 255.0f);
}

// The 22 terrain profiles: the two fog distances, a tint, the sky type, the
// overcast weight, the ambient-music index and the Lavastorm weight.  Only
// profile 19 storms.
int VIWorld::SetProfiles()
{

    m_profiles[0].m_p00 = 250.0f;
    m_profiles[0].m_p04 = 800.0f;
    m_profiles[0].m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_profiles[0].m_p18 = 1;
    m_profiles[0].m_p1C = 1.0f;
    m_profiles[0].m_p20 = 9;
    m_profiles[0].m_p24 = 0.0f;

    VIColor32 c;

    m_profiles[1].m_p00 = 0.0f;
    m_profiles[1].m_p04 = 400.0f;
    c.Set(210, 230, 180, 255);
    SetColorFrom(m_profiles[1].m_color, c);
    m_profiles[1].m_p18 = 1;
    m_profiles[1].m_p1C = 1.0f;
    m_profiles[1].m_p20 = 4;
    m_profiles[1].m_p24 = 0.0f;

    m_profiles[2].m_p00 = 250.0f;
    m_profiles[2].m_p04 = 800.0f;
    c.Set(227, 247, 247, 255);
    SetColorFrom(m_profiles[2].m_color, c);
    m_profiles[2].m_p18 = 0;
    m_profiles[2].m_p1C = 1.0f;
    m_profiles[2].m_p20 = 6;
    m_profiles[2].m_p24 = 0.0f;

    m_profiles[3].m_p00 = 250.0f;
    m_profiles[3].m_p04 = 800.0f;
    c.Set(233, 218, 166, 255);
    SetColorFrom(m_profiles[3].m_color, c);
    m_profiles[3].m_p18 = 0;
    m_profiles[3].m_p1C = 1.0f;
    m_profiles[3].m_p20 = 3;
    m_profiles[3].m_p24 = 0.0f;

    m_profiles[4].m_p00 = 150.0f;
    m_profiles[4].m_p04 = 800.0f;
    m_profiles[4].m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_profiles[4].m_p18 = 2;
    m_profiles[4].m_p1C = 0.5f;
    m_profiles[4].m_p20 = 5;
    m_profiles[4].m_p24 = 0.0f;

    m_profiles[5].m_p00 = 0.0f;
    m_profiles[5].m_p04 = 400.0f;
    c.Set(90, 140, 65, 255);
    SetColorFrom(m_profiles[5].m_color, c);
    m_profiles[5].m_p18 = 0;
    m_profiles[5].m_p1C = 1.0f;
    m_profiles[5].m_p20 = 7;
    m_profiles[5].m_p24 = 0.0f;

    m_profiles[6].m_p00 = 0.0f;
    m_profiles[6].m_p04 = 350.0f;
    c.Set(140, 170, 120, 255);
    SetColorFrom(m_profiles[6].m_color, c);
    m_profiles[6].m_p18 = 2;
    m_profiles[6].m_p1C = 0.5f;
    m_profiles[6].m_p20 = 10;
    m_profiles[6].m_p24 = 0.0f;

    m_profiles[7].m_p00 = 250.0f;
    m_profiles[7].m_p04 = 800.0f;
    m_profiles[7].m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_profiles[7].m_p18 = 1;
    m_profiles[7].m_p1C = 1.0f;
    m_profiles[7].m_p20 = 2;
    m_profiles[7].m_p24 = 0.0f;

    m_profiles[8].m_p00 = 250.0f;
    m_profiles[8].m_p04 = 800.0f;
    m_profiles[8].m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_profiles[8].m_p18 = 1;
    m_profiles[8].m_p1C = 1.0f;
    m_profiles[8].m_p20 = 1;
    m_profiles[8].m_p24 = 0.0f;

    m_profiles[9].m_p00 = 0.0f;
    m_profiles[9].m_p04 = 400.0f;
    c.Set(210, 230, 180, 255);
    SetColorFrom(m_profiles[9].m_color, c);
    m_profiles[9].m_p18 = 1;
    m_profiles[9].m_p1C = 1.0f;
    m_profiles[9].m_p20 = 1;
    m_profiles[9].m_p24 = 0.0f;

    m_profiles[10].m_p00 = 250.0f;
    m_profiles[10].m_p04 = 800.0f;
    c.Set(227, 247, 247, 255);
    SetColorFrom(m_profiles[10].m_color, c);
    m_profiles[10].m_p18 = 0;
    m_profiles[10].m_p1C = 1.0f;
    m_profiles[10].m_p20 = 1;
    m_profiles[10].m_p24 = 0.0f;

    m_profiles[11].m_p00 = 250.0f;
    m_profiles[11].m_p04 = 800.0f;
    c.Set(233, 218, 166, 255);
    SetColorFrom(m_profiles[11].m_color, c);
    m_profiles[11].m_p18 = 0;
    m_profiles[11].m_p1C = 1.0f;
    m_profiles[11].m_p20 = 1;
    m_profiles[11].m_p24 = 0.0f;

    m_profiles[12].m_p00 = 150.0f;
    m_profiles[12].m_p04 = 800.0f;
    m_profiles[12].m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_profiles[12].m_p18 = 2;
    m_profiles[12].m_p1C = 0.5f;
    m_profiles[12].m_p20 = 1;
    m_profiles[12].m_p24 = 0.0f;

    m_profiles[13].m_p00 = 0.0f;
    m_profiles[13].m_p04 = 400.0f;
    c.Set(90, 140, 65, 255);
    SetColorFrom(m_profiles[13].m_color, c);
    m_profiles[13].m_p18 = 0;
    m_profiles[13].m_p1C = 1.0f;
    m_profiles[13].m_p20 = 1;
    m_profiles[13].m_p24 = 0.0f;

    m_profiles[14].m_p00 = 0.0f;
    m_profiles[14].m_p04 = 350.0f;
    c.Set(140, 170, 120, 255);
    SetColorFrom(m_profiles[14].m_color, c);
    m_profiles[14].m_p18 = 2;
    m_profiles[14].m_p1C = 0.5f;
    m_profiles[14].m_p20 = 1;
    m_profiles[14].m_p24 = 0.0f;

    m_profiles[15].m_p00 = 250.0f;
    m_profiles[15].m_p04 = 800.0f;
    m_profiles[15].m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_profiles[15].m_p18 = 1;
    m_profiles[15].m_p1C = 1.0f;
    m_profiles[15].m_p20 = 8;
    m_profiles[15].m_p24 = 0.0f;

    m_profiles[16].m_p00 = 250.0f;
    m_profiles[16].m_p04 = 800.0f;
    m_profiles[16].m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    m_profiles[16].m_p18 = 1;
    m_profiles[16].m_p1C = 1.0f;
    m_profiles[16].m_p20 = 1;
    m_profiles[16].m_p24 = 0.0f;

    m_profiles[17].m_p00 = 0.0f;
    m_profiles[17].m_p04 = 800.0f;
    c.Set(255, 170, 170, 255);
    SetColorFrom(m_profiles[17].m_color, c);
    m_profiles[17].m_p18 = 0;
    m_profiles[17].m_p1C = 1.0f;
    m_profiles[17].m_p20 = 3;
    m_profiles[17].m_p24 = 0.0f;

    m_profiles[18].m_p00 = 0.0f;
    m_profiles[18].m_p04 = 550.0f;
    c.Set(110, 140, 30, 255);
    SetColorFrom(m_profiles[18].m_color, c);
    m_profiles[18].m_p18 = 1;
    m_profiles[18].m_p1C = 1.0f;
    m_profiles[18].m_p20 = 8;
    m_profiles[18].m_p24 = 0.0f;

    m_profiles[19].m_p00 = 0.0f;
    m_profiles[19].m_p04 = 600.0f;
    c.Set(255, 170, 170, 255);
    SetColorFrom(m_profiles[19].m_color, c);
    m_profiles[19].m_p18 = 2;
    m_profiles[19].m_p1C = 1.0f;
    m_profiles[19].m_p20 = 8;
    m_profiles[19].m_p24 = 1.0f;

    m_profiles[20].m_p00 = 250.0f;
    m_profiles[20].m_p04 = 800.0f;
    c.Set(175, 200, 240, 255);
    SetColorFrom(m_profiles[20].m_color, c);
    m_profiles[20].m_p18 = 1;
    m_profiles[20].m_p1C = 1.0f;
    m_profiles[20].m_p20 = 6;
    m_profiles[20].m_p24 = 0.0f;

    m_profiles[21].m_p00 = 0.0f;
    m_profiles[21].m_p04 = 600.0f;
    c.Set(140, 170, 160, 255);
    SetColorFrom(m_profiles[21].m_color, c);
    m_profiles[21].m_p18 = 1;
    m_profiles[21].m_p1C = 1.0f;
    m_profiles[21].m_p20 = 7;
    m_profiles[21].m_p24 = 0.0f;

    return 0;
}
