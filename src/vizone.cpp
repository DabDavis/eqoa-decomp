// VIZone -- SUPPORT 0x00493830 .. 0x00498CC0, its own TU.  Phase 52.
//
// The sibling of `viworld.cpp`, one level down: where a VIWorld is a BSP over
// ZONES, a VIZone is a BSP over ROOMS.  The two descents are literally the same
// code -- `VIWorld::Collide(int, VISceneCollVars &)` and
// `VIZone::Collide(int, VISceneCollVars &)` differ only in the register that
// holds `node`, in `m_tree`'s offset, and in what the leaf arm does -- so every
// descent here is phase 51's source with the leaf arm replaced by a call to
// this file's own `*Leaf`.
//
// What is new is the LEAF end.  A world leaf names a zone and forwards; a zone
// leaf names a ROOM, and the room is where the pass stamps, the room sprite and
// the room's actor list live:
//
//   * a leaf stamps `m_rooms[room].m_p20` against `v.m_stamp` -- the world's own
//     pass counter, threaded through the Vars -- so a room reached twice in one
//     descent is walked once;
//   * a room stamps `m_rooms[room].m_p24` against the SCENE's counter at +0x544,
//     which is a different stamp on a different clock;
//   * `m_p1C` is the room's own static sprite, -1 when it has none, and it is
//     rastered/collided/picked through `VIZone::IdentityMatrix` -- the zone's
//     geometry is already in world space;
//   * `m_p68` is the head of the room's actor list, a POINTER stored in an int
//     and terminated by -1, not by null.
#include "vi.h"


VIMatrix44 VIZone::IdentityMatrix;

VIZone::VIZone()
{
    IdentityMatrix.Identity();
    Clear();
}

VIZone::~VIZone()
{
}

int VIZone::Init()
{
    m_flags |= 1;

    if (m_tree.Init(0, 1) < 0)
        return -1;

    if (m_rooms.Init(1) < 0)
        return -1;

    m_tree.m_leaves[0].m_a = 0;
    m_tree.m_leaves[0].m_b = 0;

    if (m_rooms[0].Init(0, 0, 0, 0, m_bbox) < 0)
        return -1;

    m_rooms[0].m_flags |= 1;
    return 0;
}

int VIZone::Clear()
{
    int r = -1;

    m_flags = 0;
    m_offset = 0;
    m_size = 0;
    m_trans = -1;
    m_p20.Zero();
    m_bbox.min.Zero();
    m_bbox.max.Zero();

    if (m_flora.Clear() < 0)
        return -1;

    if (m_pretrans.Clear() < 0)
        return -1;

    if (m_rooms.Clear() < 0)
        return -1;

    if (m_tree.Clear() < 0)
        return -1;

    if (0 <= m_resources.Clear())
        r = 0;

    return r;
}

int VIZone::ResetToProxy(VIScene &sc)
{
    int i;

    for (i = 0; i < m_flora.m_models.m_size; i++)
        sc.ReleaseSprite(m_flora.m_models[i].m_a);

    if (m_flora.Clear() < 0)
        return -1;

    if (m_pretrans.Clear() < 0)
        return -1;

    if (m_rooms.Clear() < 0)
        return -1;

    if (m_tree.Clear() < 0)
        return -1;

    if (m_resources.Clear() < 0)
        return -1;

    if (m_statics.Clear() < 0)
        return -1;

    if (Init() < 0)
        return -1;

    return 0;
}

int VIZone::RenderByVolume(VISceneRendVars &v, int zone)
{
    v.m_zone = zone;

    if (SetPretranslations(*v.m_raster, *v.m_camera) < 0)
        return -1;

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

    return 0;
}

int VIZone::RenderByVolume(int node, VISceneRendVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return RenderLeaf(node & 0x0FFFFFFF, v);

        node &= 0x0FFFFFFF;
    }

    int front    = 0;
    int back     = 0;
    int i;

    for (i = 0; i < v.m_numVerts; i++)
    {
        if (0.0f < m_tree.m_nodes[node].m_plane.Distance(v.m_verts[i]))
        {
            front = 1;
            if (back)
                break;
        }
        else
        {
            back = 1;
            if (front)
                break;
        }
    }

    int nearBack = 0;

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

int VIZone::RenderLeaf(int leaf, VISceneRendVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        if ((m_rooms[room].m_flags & 1) || !v.m_p2C)
            if (RenderRoom(room, v) < 0)
                return -1;
    }

    return 0;
}

int VIZone::SetPretranslations(VIRaster &r, VICamera &cam)
{
    int     rc = -1;
    int     i;

    for (i = 0; i < m_pretrans.m_size; i++)
    {
        // Both operands through named references: `c.x` folds into the
        // camera's own base while .y/.z go through `c`, the target's mix.
        const VIVect3 &a = m_pretrans[i];
        const VIVect3 &c = cam.m_position;
        VIVect3 t(a.x - c.x, a.y - c.y, a.z - c.z);
        m_p[i] = t;
    }

    if (0 <= r.SetPreTranslations(m_pretrans.m_size, m_p, cam.m_position))
        rc = 0;

    return rc;
}

int VIZone::Collide(VISceneCollVars &v)
{
    VIArray<VIVect3> &pt = m_pretrans;
    int               hit;

    if (0 < pt.m_size)
        if (v.m_collide->SetPreTranslations(pt.m_size, pt.m_data) < 0)
            return -1;

    if (m_tree.m_nodes.m_size)
        hit = Collide(0, v) != 0;
    else
        hit = Collide(0x40000000, v) != 0;

    return hit;
}

int VIZone::Collide(int node, VISceneCollVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return CollideLeaf(node & 0x0FFFFFFF, v);

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

int VIZone::CollideLeaf(int leaf, VISceneCollVars &v)
{
    int hit  = 0;
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        if (CollideRoom(room, v))
        {
            hit = 1;
            v.m_end = v.m_sphere->GetHitPoint();
        }
    }

    return hit;
}

int VIZone::CollideRoom(int room, VISceneCollVars &v)
{
    int hit = 0;

    if (m_rooms[room].m_p24 != v.m_scene->m_stamp)
    {
        m_rooms[room].m_p24 = v.m_scene->m_stamp;

        int sprite = m_rooms[room].m_p1C;

        if (sprite != -1)
        {
            v.m_collide->SetWorld(IdentityMatrix);
            hit = v.m_scene->CollideSprite(*v.m_sphere, *v.m_filter, sprite)
                      != 0;
        }
    }

    int n = m_rooms[room].m_p68;

    while (n != -1)
    {
        VISceneActorRef *a = (VISceneActorRef *)n;

        if (v.m_scene->CollideActor(*v.m_sphere, *v.m_filter, a->m_actor))
            hit = 1;

        n = a->m_next;
    }

    return hit;
}

int VIZone::Collide(VISceneCollRayVars &v)
{
    VIArray<VIVect3> &pt = m_pretrans;
    int               hit;

    if (0 < pt.m_size)
        if (v.m_collide->SetPreTranslations(pt.m_size, pt.m_data) < 0)
            return -1;

    if (m_tree.m_nodes.m_size)
        hit = Collide(0, v) != 0;
    else
        hit = Collide(0x40000000, v) != 0;

    return hit;
}

int VIZone::Collide(int node, VISceneCollRayVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return CollideLeaf(node & 0x0FFFFFFF, v);

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

int VIZone::CollideLeaf(int leaf, VISceneCollRayVars &v)
{
    int hit  = 0;
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        if (CollideRoom(room, v))
        {
            hit = 1;
            v.m_end = v.m_ray->GetHitPoint();
        }
    }

    return hit;
}

int VIZone::CollideRoom(int room, VISceneCollRayVars &v)
{
    int hit = 0;

    if (m_rooms[room].m_p24 != v.m_scene->m_stamp)
    {
        m_rooms[room].m_p24 = v.m_scene->m_stamp;

        int sprite = m_rooms[room].m_p1C;

        if (sprite != -1)
        {
            v.m_collide->SetWorld(IdentityMatrix);
            hit = v.m_scene->CollideSprite(*v.m_ray, *v.m_filter, sprite) != 0;
        }
    }

    int n = m_rooms[room].m_p68;

    while (n != -1)
    {
        VISceneActorRef *a = (VISceneActorRef *)n;

        if (v.m_scene->CollideActor(*v.m_ray, *v.m_filter, a->m_actor))
            hit = 1;

        n = a->m_next;
    }

    return hit;
}

int VIZone::InsertActor(VISceneInsertVars &v)
{
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

    return 0;
}

// OPEN at 6 of 45, and a SCHEDULER difference, not source: the 0x0FFFFFFF
// mask lands in a0, so its `lui` anti-depends on the `lw t0,72(a0)` that
// reads `this`.  The target issues the two in the SAME cycle; ours waits a
// cycle (.sched2: insn 166 is not ready at t=1), and `li a2,24` takes the slot.
int VIZone::FindLeaf(const VIVect3 &at) const
{
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

        return node & 0x0FFFFFFF;
    }

    return 0;
}

int VIZone::Pick(VIScenePickVars &v)
{
    VIArray<VIVect3> &pt = m_pretrans;
    int               hit;

    if (0 < pt.m_size)
    {
        VIVect3 at;

        if (v.m_raster->SetPreTranslations(pt.m_size, pt.m_data, at) < 0)
            return -1;
    }

    if (m_tree.m_nodes.m_size)
        hit = Pick(0, v) != 0;
    else
        hit = Pick(0x40000000, v) != 0;

    return hit;
}

int VIZone::Pick(int node, VIScenePickVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return PickLeaf(node & 0x0FFFFFFF, v);

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

int VIZone::PickLeaf(int leaf, VIScenePickVars &v)
{
    int hit  = 0;
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        if (PickRoom(room, v))
        {
            hit = 1;
            v.m_end = v.m_seg->GetHitPoint();
        }
    }

    return hit;
}

int VIZone::PickRoom(int room, VIScenePickVars &v)
{
    int hit = 0;

    if (m_rooms[room].m_p24 != v.m_scene->m_stamp)
    {
        m_rooms[room].m_p24 = v.m_scene->m_stamp;

        int sprite = m_rooms[room].m_p1C;

        if (sprite != -1)
        {
            v.m_raster->SetModelView(IdentityMatrix);
            hit = v.m_scene->PickSprite(*v.m_seg, *v.m_filter, sprite);
        }
    }

    int n = m_rooms[room].m_p68;

    while (n != -1)
    {
        VISceneActorRef *a = (VISceneActorRef *)n;

        if (v.m_scene->PickActor(*v.m_seg, *v.m_filter, a->m_actor))
            hit = 1;

        n = a->m_next;
    }

    return hit;
}

int VIZone::InsertActor(int node, VISceneInsertVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
        {
            node &= 0x0FFFFFFF;

            int room = m_tree.m_leaves[node].m_a;

            if (m_rooms[room].m_p20 != v.m_stamp)
            {
                m_rooms[room].m_p20 = v.m_stamp;

                if (v.m_scene->InsertActor(v.m_actor, v.m_zone, room) < 0)
                    return -1;
            }

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

int VIZone::Stream(VISceneStreamRadialVars &v)
{
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

    return 0;
}

int VIZone::Stream(int node, VISceneStreamRadialVars &v)
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

// The radial leaf.  Two independent decisions on the same squared distance: the
// room's own geometry (flags bit 1 says it is proxy) and the room's STATICS
// (bits 3..5), each with an in/halt/out triple.
int VIZone::StreamLeaf(int leaf, VISceneStreamRadialVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        float d = m_rooms[room].m_bbox.DistanceSquared(v.m_at);

        if (m_rooms[room].m_flags & 2)
        {
            if (d < v.m_inner * v.m_inner)
            {
                if (StreamRoomIn(room, v) < 0)
                    return -1;
            }
            else if (StreamRoomHalt(room, v) < 0)
                return -1;
        }
        else if (v.m_outer * v.m_outer < d)
        {
            if (StreamRoomOut(room, v) < 0)
                return -1;
        }

        if (v.m_p30 * v.m_p30 < d)
        {
            if (StreamRoomStaticsOut(room, v) < 0)
                return -1;
        }
        else if ((m_rooms[room].m_flags & 0x38) != 0x38)
        {
            if (d < v.m_p2C * v.m_p2C)
            {
                if (StreamRoomStaticsIn(room, v) < 0)
                    return -1;
            }
            else if (StreamRoomStaticsHalt(room, v) < 0)
                return -1;
        }
    }

    return 0;
}

int VIZone::Stream(VISceneBBoxStreamVars &v)
{
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

    v.m_scene->UpdateActors();
    return 0;
}

int VIZone::Stream(int node, VISceneBBoxStreamVars &v)
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

// The same six decisions as the radial leaf, with a box overlap where that one
// has a squared distance: the INNER box drives the room and the OUTER one the
// statics.
int VIZone::StreamLeaf(int leaf, VISceneBBoxStreamVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        if (m_rooms[room].m_flags & 2)
        {
            if (v.m_bbox.Intersects(m_rooms[room].m_bbox))
            {
                if (StreamRoomIn(room, v) < 0)
                    return -1;
            }
            else if (StreamRoomHalt(room, v) < 0)
                return -1;
        }
        else if (v.m_outer.Intersects(m_rooms[room].m_bbox) == 0)
        {
            if (StreamRoomOut(room, v) < 0)
                return -1;
        }

        if (v.m_outer.Intersects(m_rooms[room].m_bbox) == 0)
        {
            if (StreamRoomStaticsOut(room, v) < 0)
                return -1;
        }
        else if ((m_rooms[room].m_flags & 0x38) != 0x38)
        {
            if (v.m_bbox.Intersects(m_rooms[room].m_bbox))
            {
                if (StreamRoomStaticsIn(room, v) < 0)
                    return -1;
            }
            else if (StreamRoomStaticsHalt(room, v) < 0)
                return -1;
        }
    }

    return 0;
}

int VIZone::StreamRoomHalt(int room, VISceneStreamVars &v)
{
    int a;

    if (m_rooms[room].m_flags & 4)
    {
        if (v.m_dict->FindTyped(m_rooms[room].m_p30, (VIResourceType)3, a) == 0)
            return -1;

        if (v.m_raster)
        {
            v.m_raster->ReleaseMaterialPal(a, *v.m_dict);
            m_rooms[room].m_flags &= 0xFFFFFFFB;
        }
    }

    if (m_rooms[room].m_p34 != -1)
    {
        if (v.m_loader->Release(m_rooms[room].m_p34, *v.m_scene) < 0)
            return -1;

        m_rooms[room].m_p34 = -1;
    }

    if (m_rooms[room].m_p2C != -1)
    {
        if (v.m_loader->Release(m_rooms[room].m_p2C, *v.m_scene) < 0)
            return -1;

        m_rooms[room].m_p2C = -1;
    }

    return 0;
}

int VIZone::StreamRoomOut(int room, VISceneStreamVars &v)
{
    if (m_rooms[room].m_p1C != -1)
        v.m_scene->ReleaseSprite(m_rooms[room].m_p1C);

    m_rooms[room].m_flags |= 2;
    m_rooms[room].m_p1C = -1;
    return 0;
}

int VIZone::Find(unsigned int id, unsigned long &off, unsigned int &size)
{
    VIArray<VIResourceElem> &res = m_resources;
    int lo  = 0;
    int hi  = res.m_size - 1;
    int mid = 0;

    while (lo <= hi && m_resources[mid = (lo + hi) >> 1].m_id != id)
    {
        if (id < m_resources[mid].m_id)
            hi = mid - 1;
        else
            lo = mid + 1;
    }

    if (lo <= hi)
    {
        off  = m_resources[mid].m_offset;
        size = m_resources[mid].m_size;
        return 1;
    }

    return 0;
}

int VIZone::QueryProximal(VISceneProxQueryVars &v)
{
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

    return 0;
}

int VIZone::QueryProximal(int node, VISceneProxQueryVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return QueryProximalLeaf(node & 0x0FFFFFFF, v);

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

int VIZone::QueryProximalLeaf(int leaf, VISceneProxQueryVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        if (QueryProximalRoom(room, v) < 0)
            return -1;
    }

    return 0;
}

int VIZone::QueryProximalRoom(int room, VISceneProxQueryVars &v)
{
    if (m_rooms[room].m_bbox.CullIntersect(v.m_sphere))
        return 0;

    int n = m_rooms[room].m_p68;

    while (n != -1)
    {
        VISceneActorRef *a = (VISceneActorRef *)n;

        if (v.m_scene->QueryProximalActor(a->m_actor, v.m_sphere, *v.m_query) < 0)
            return -1;

        n = a->m_next;
    }

    return 0;
}

int VIZone::QueryEmitters(VISceneEmitterQueryVars &v)
{
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

    return 0;
}

int VIZone::QueryEmitters(int node, VISceneEmitterQueryVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return QueryEmittersLeaf(node & 0x0FFFFFFF, v);

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

int VIZone::QueryEmittersLeaf(int leaf, VISceneEmitterQueryVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        if (QueryEmittersRoom(room, v) < 0)
            return -1;
    }

    return 0;
}

int VIZone::QueryEmittersRoom(int room, VISceneEmitterQueryVars &v)
{
    int n = m_rooms[room].m_p68;

    while (n != -1)
    {
        VISceneActorRef *a = (VISceneActorRef *)n;

        if (v.m_scene->QueryEmittersActor(a->m_actor, *v.m_camera, v.m_sphere,
                                          *v.m_query) < 0)
            return -1;

        n = a->m_next;
    }

    return 0;
}

int VIZone::QueryVolume(VISceneBBoxQueryVars &v)
{
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

    return 0;
}

int VIZone::QueryVolume(int node, VISceneBBoxQueryVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return QueryVolumeLeaf(node & 0x0FFFFFFF, v);

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

int VIZone::QueryVolumeLeaf(int leaf, VISceneBBoxQueryVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        if (QueryVolumeRoom(room, v) < 0)
            return -1;
    }

    return 0;
}

int VIZone::QueryVolumeRoom(int room, VISceneBBoxQueryVars &v)
{
    int n = m_rooms[room].m_p68;

    while (n != -1)
    {
        VISceneActorRef *a = (VISceneActorRef *)n;

        if (v.m_scene->QueryVolumeActor(a->m_actor, v.m_bbox, *v.m_query) < 0)
            return -1;

        n = a->m_next;
    }

    return 0;
}

int VIZone::QueryVisible(VISceneVisQueryVars &v)
{
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

    return 0;
}

int VIZone::QueryVisibleLeaf(int leaf, VISceneVisQueryVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        if ((m_rooms[room].m_flags & 1) || !v.m_p38)
            if (QueryVisibleRoom(room, v) < 0)
                return -1;
    }

    return 0;
}

int VIZone::QueryVisibleRoom(int room, VISceneVisQueryVars &v)
{
    if (QueryThroughPortals(room, v) < 0)
        return -1;

    int n = m_rooms[room].m_p68;

    while (n != -1)
    {
        VISceneActorRef *a = (VISceneActorRef *)n;

        if (v.m_scene->QueryVisibleActor(a->m_actor, v.m_sphere, *v.m_query) < 0)
            return -1;

        n = a->m_next;
    }

    return 0;
}

int VIZone::QueryIntersection(VISceneIntersectVars &v)
{
    VIArray<VIVect3> &pt = m_pretrans;

    if (0 < pt.m_size)
        if (v.m_collide->SetPreTranslations(pt.m_size, pt.m_data) < 0)
            return -1;

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

    return 0;
}

int VIZone::QueryIntersectionLeaf(int leaf, VISceneIntersectVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 != v.m_stamp)
    {
        m_rooms[room].m_p20 = v.m_stamp;

        if (QueryIntersectionRoom(room, v) < 0)
            return -1;
    }

    return 0;
}

// ================================================================ phase 61 --

int VIZone::Collide(VIEnvRay &r, VIVect3 &a, VIVect3 &b)
{
    int hit;

    if (m_tree.m_nodes.m_size)
        hit = Collide(0, r, a, b) != 0;
    else
        hit = Collide(0x40000000, r, a, b) != 0;

    return hit;
}

// VIWorld's env-ray descent with the leaf arm replaced.
int VIZone::Collide(int node, VIEnvRay &r, VIVect3 &a, VIVect3 &b)
{
    VIVect3 mid;

    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return CollideLeaf(node & 0x0FFFFFFF, r, a);

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

// A leaf reports its environment mask to the ray, and the ray's own virtual
// decides; the same mask twice in a row is skipped unless the ray has not hit
// anything yet.  `t` is the hit's position along the ray, from `a`.
int VIZone::CollideLeaf(int leaf, VIEnvRay &r, VIVect3 &a)
{
    int          hit  = 0;
    unsigned int mask = m_tree.m_leaves[leaf].m_b & r.m_mask;

    if (r.m_firstCollide || r.m_hits != mask)
    {
        VIVect3 *d = &r.m_delta;
        float t = (a.x * d->x + a.y * d->y + a.z * d->z)
                  - (r.m_start.x * d->x + r.m_start.y * d->y + r.m_start.z * d->z);
        t *= r.m_invLength;

        hit = r.Collide(mask, t, a);
        if (hit)
            r.m_hitTime = t;
        r.m_hits = mask;
        r.m_firstCollide = 0;
    }

    return hit;
}

// The resource directory resolves the DictID to a file extent, and the loader
// is handed the extent; a DictID this zone does not carry is -1.
int VIZone::Load(unsigned int id, int zone, VILoader &l, int file)
{
    unsigned long off;
    unsigned int  size;

    if (Find(id, off, size))
        return l.Load(file, id, off, size, zone);

    return -1;
}

// The room's statics move through three states on m_flags -- 0x08 the table
// is loaded, 0x10 the actors are in, 0x20 they are complete -- and a halt
// undoes whichever step is in flight: the table's load, or each actor's.
int VIZone::StreamRoomStaticsHalt(int room, VISceneStreamVars &v)
{
    unsigned int flags = m_rooms[room].m_flags;

    if (!(flags & 8))
    {
        if (m_rooms[room].m_p38 != -1)
        {
            if (v.m_loader->Release(m_rooms[room].m_p38, *v.m_scene) < 0)
                return -1;

            m_rooms[room].m_p38 = -1;
        }
    }
    else if (!(flags & 0x10))
    {
        for (int i = 0; i < m_rooms[room].m_a3C.m_size; i++)
            if (v.m_scene->StreamHaltStaticActor(m_rooms[room].m_a3C[i], v) < 0)
                return -1;
    }
    else if (!(flags & 0x20))
    {
        if (m_rooms[room].m_p38 != -1)
        {
            if (v.m_loader->Release(m_rooms[room].m_p38, *v.m_scene) < 0)
                return -1;

            m_rooms[room].m_p38 = -1;
        }
    }

    return 0;
}

int VIZone::StreamRoomStaticsOut(int room, VISceneStreamVars &v)
{
    if (m_rooms[room].m_p38 != -1)
    {
        if (v.m_loader->Release(m_rooms[room].m_p38, *v.m_scene) < 0)
            return -1;

        m_rooms[room].m_p38 = -1;
    }

    for (int i = 0; i < m_rooms[room].m_a3C.m_size; i++)
    {
        if (v.m_scene->StreamOutStaticActor(m_rooms[room].m_a3C[i], v) < 0)
            return -1;

        if (v.m_scene->ReleaseActor(m_rooms[room].m_a3C[i]) < 0)
            return -1;
    }

    if (m_rooms[room].m_a44.Clear() < 0)
        return -1;

    if (m_rooms[room].m_a3C.Clear() < 0)
        return -1;

    m_rooms[room].m_flags &= 0xFFFFFFC7;
    return 0;
}

// A room streams in in two halves -- its material palette and its sprite --
// each resolved in the dictionary first and LOADED only if absent.  A palette
// that arrives before its sprite is pinned (flag 4) so it cannot be evicted in
// between; once both are resident the room takes the sprite, drops the pin and
// retires both loader transactions.
int VIZone::StreamRoomIn(int room, VISceneStreamVars &v)
{
    int pal;
    int sprite;
    int sprIn;
    int palIn;

    sprite = -1;
    pal = -1;
    sprIn = 1;
    palIn = 1;

    if (m_rooms[room].m_p30 && v.m_raster)
    {
        if (!v.m_dict->FindTyped(m_rooms[room].m_p30, (VIResourceType)3, pal))
        {
            palIn = 0;

            if (m_rooms[room].m_p34 == -1)
            {
                m_rooms[room].m_p34 = Load(m_rooms[room].m_p30, v.m_zone,
                                           *v.m_loader, v.m_file);
                if (m_rooms[room].m_p34 < 0)
                    return -1;
            }
        }
    }

    if (m_rooms[room].m_p28)
    {
        if (!v.m_dict->FindTyped(m_rooms[room].m_p28, (VIResourceType)4, sprite))
        {
            sprIn = 0;

            if (m_rooms[room].m_p2C == -1)
            {
                m_rooms[room].m_p2C = Load(m_rooms[room].m_p28, v.m_zone,
                                           *v.m_loader, v.m_file);
                if (m_rooms[room].m_p2C < 0)
                    return -1;
            }
        }
    }

    if (palIn && !sprIn)
    {
        if (!(m_rooms[room].m_flags & 4) && v.m_raster)
        {
            v.m_raster->ShareMaterialPal(pal);
            m_rooms[room].m_flags |= 4;
        }
    }

    if (palIn && sprIn)
    {
        m_rooms[room].m_p1C = sprite;

        if (m_rooms[room].m_p1C != -1)
            v.m_scene->ShareSprite(m_rooms[room].m_p1C);

        m_rooms[room].m_flags &= 0xFFFFFFFD;

        if ((m_rooms[room].m_flags & 4) && v.m_raster)
        {
            v.m_raster->ReleaseMaterialPal(pal, *v.m_dict);
            m_rooms[room].m_flags &= 0xFFFFFFFB;
        }

        if (m_rooms[room].m_p34 != -1)
        {
            if (v.m_loader->Release(m_rooms[room].m_p34, *v.m_scene) < 0)
                return -1;

            m_rooms[room].m_p34 = -1;
        }

        if (m_rooms[room].m_p2C != -1)
        {
            if (v.m_loader->Release(m_rooms[room].m_p2C, *v.m_scene) < 0)
                return -1;

            m_rooms[room].m_p2C = -1;
        }
    }

    return 0;
}

// The statics' three steps, each run as far as it can get on one call: load
// the static table (flag 8), stream every actor it names in (flag 0x10), load
// the static LIGHTING that goes with them (flag 0x20); and once all three are
// done, complete each actor.  An extent of -1 means the room has none.
int VIZone::StreamRoomStaticsIn(int room, VISceneStreamVars &v)
{
    int i;

    if (!(m_rooms[room].m_flags & 8))
    {
        if (m_statics[room].m_offset != (unsigned long)-1)
        {
            if (m_rooms[room].m_p38 == -1)
            {
                m_rooms[room].m_p38 = v.m_loader->Load(v.m_file,
                                                       m_statics[room].m_offset,
                                                       m_statics[room].m_size,
                                                       v.m_zone);
                if (m_rooms[room].m_p38 < 0)
                    return -1;
            }
            else if (v.m_loader->IsComplete(m_rooms[room].m_p38))
            {
                m_rooms[room].m_flags |= 8;

                if (v.m_loader->Release(m_rooms[room].m_p38, *v.m_scene) < 0)
                    return -1;

                m_rooms[room].m_p38 = -1;
            }
        }
        else
            m_rooms[room].m_flags |= 8;
    }

    if ((m_rooms[room].m_flags & 8) && !(m_rooms[room].m_flags & 0x10))
    {
        int done = 1;

        for (i = 0; i < m_rooms[room].m_a3C.m_size; i++)
        {
            int actor = m_rooms[room].m_a3C[i];

            if (v.m_scene->GetActorSprite(actor) == -1)
            {
                if (v.m_scene->StreamInStaticActor(actor, *this, v) < 0)
                    return -1;

                if (v.m_scene->GetActorSprite(actor) == -1)
                    done = 0;
            }
        }

        if (done)
            m_rooms[room].m_flags |= 0x10;
    }

    if ((m_rooms[room].m_flags & 0x10) && !(m_rooms[room].m_flags & 0x20))
    {
        if (m_statics[room].m_offset2 != (unsigned long)-1)
        {
            if (m_rooms[room].m_p38 == -1)
            {
                m_rooms[room].m_p38 = v.m_loader->Load(v.m_file,
                                                       m_statics[room].m_offset2,
                                                       m_statics[room].m_size2,
                                                       v.m_zone);
                if (m_rooms[room].m_p38 < 0)
                    return -1;
            }
            else if (v.m_loader->IsComplete(m_rooms[room].m_p38))
            {
                m_rooms[room].m_flags |= 0x20;

                if (v.m_loader->Release(m_rooms[room].m_p38, *v.m_scene) < 0)
                    return -1;

                m_rooms[room].m_p38 = -1;
            }
        }
        else
            m_rooms[room].m_flags |= 0x20;
    }

    if ((m_rooms[room].m_flags & 0x38) == 0x38)
    {
        for (i = 0; i < m_rooms[room].m_a3C.m_size; i++)
            if (v.m_scene->StreamInCompleteActor(m_rooms[room].m_a3C[i], v) < 0)
                return -1;
    }

    return 0;
}

// A room draws in three steps: through its portals first, then its own static
// sprite in the zone's (world-space) frame with no baked lighting, then every
// actor standing in it that is within the view distance.
int VIZone::RenderRoom(int room, VISceneRendVars &v)
{
    if (v.m_clip->CullByTop(m_rooms[room].m_bbox))
        return 0;

    if (RenderThroughPortals(room, v) < 0)
        return -1;

    if (m_rooms[room].m_p24 != v.m_scene->m_stamp
        && !(m_rooms[room].m_flags & 2))
    {
        m_rooms[room].m_p24 = v.m_scene->m_stamp;

        int sprite = m_rooms[room].m_p1C;

        if (sprite != -1)
        {
            sceVu0CopyMatrix(m_matrix, v.m_camera->m_view);
            m_matrix.m[3].x = 0.0f;
            m_matrix.m[3].y = 0.0f;
            m_matrix.m[3].z = 0.0f;
            m_matrix.m[3].w = 1.0f;
            v.m_raster->SetModelView(m_matrix);
            v.m_raster->SetStaticLighting(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));

            if (v.m_scene->RasterSprite(*v.m_camera, sprite, 1))
            {
                VISceneDisplayElem e;

                e.m_b = -1;
                e.m_a = v.m_zone;
                e.m_c = sprite;

                if (v.m_scene->m_display.Add(e) < 0)
                    return -1;
            }
        }
    }

    if (m_rooms[room].m_bbox.DistanceSquared(v.m_camera->m_position)
        < v.m_viewDist * v.m_viewDist)
    {
        int n = m_rooms[room].m_p68;

        while (n != -1)
        {
            if (v.m_scene->RasterActor(*v.m_camera,
                                       ((VISceneActorRef *)n)->m_actor,
                                       *v.m_clip) < 0)
                return -1;

            n = ((VISceneActorRef *)n)->m_next;
        }
    }

    return 0;
}

// A portal the camera is in front of is clipped against the current top of
// the clip stack and, if anything survives, pushed as the new top while the
// room behind it draws -- unless the camera is within 0.02 of it, when the
// room draws through the unclipped stack.  A portal into an OUTDOOR room (bit
// 0) hands the whole world to RenderByFrustum, once.  The portal's own bit 0
// marks it open for the duration, so a cycle of rooms cannot recurse.
int VIZone::RenderThroughPortals(int room, VISceneRendVars &v)
{
    VIColor32F    unused;
    VIZoneRoom   &r     = m_rooms[room];
    VIVect3      *verts = (VIVect3 *)r.m_verts;
    int           n;
    VIVect3      *out;
    int           i;

    for (i = 0; i < r.m_numPortals; i++)
    {
        VIZonePortal *p    = (VIZonePortal *)(i * 32 + r.m_portals);
        int           nv   = p->m_numVerts;
        int           dest = p->m_destRoom;

        if ((p->m_p1C & 1) == 0)
        {
            float d = p->m_plane.Distance(v.m_camera->m_position);

            if (d < 0.0f)
                goto next;

            int inside = 0;
            if (d < 0.02f)
                inside = 1;

            if (!inside && !v.m_clip->ClipByTop(nv, verts, n, &out))
                goto next;

            if (!(m_rooms[dest].m_flags & 1))
            {
                if (!inside)
                {
                    if (v.m_clip->PushPortal(*v.m_camera, n, out) < 0)
                        return -1;
                    v.m_clip->GetTop(v.m_numVerts, &v.m_verts, v.m_numPlanes,
                                     &v.m_planes);
                }

                ((VIZonePortal *)(i * 32 + r.m_portals))->m_p1C |= 1;

                if (RenderRoom(dest, v) < 0)
                    return -1;

                if (!inside)
                {
                    if (v.m_clip->Pop() < 0)
                        return -1;
                    v.m_clip->GetTop(v.m_numVerts, &v.m_verts, v.m_numPlanes,
                                     &v.m_planes);
                }

                ((VIZonePortal *)(i * 32 + r.m_portals))->m_p1C &= ~1;
            }
            else if (!v.m_p28)
            {
                if (v.m_world->RenderByFrustum(v))
                    return -1;
            }
        }
next:
        verts += nv;
    }

    return 0;
}

// The visibility descent is the SPHERE walk for as long as the query sphere
// lies wholly on one side, and the VOLUME walk once it straddles: the current
// clip volume's corners decide which children are reached.  ONE distance
// variable serves both, so the near side is decided by the LAST corner tested
// (or by the sphere's centre when the clip volume has no corners).
int VIZone::QueryVisible(int node, VISceneVisQueryVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return QueryVisibleLeaf(node & 0x0FFFFFFF, v);

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

// RenderThroughPortals' query twin: the query sphere's centre stands in for
// the camera, an outdoor room hands the whole world to VIWorld::QueryVisible
// unless the query is already inside a world walk, and there is no "too close
// to clip" case -- every portal in front is clipped and pushed.
int VIZone::QueryThroughPortals(int room, VISceneVisQueryVars &v)
{
    VIColor32F    unused;
    VIZoneRoom   &r     = m_rooms[room];
    VIVect3      *verts = (VIVect3 *)r.m_verts;
    int           n;
    VIVect3      *out;
    int           i;
    int           nv;

    for (i = 0; i < r.m_numPortals; i++, verts += nv)
    {
        VIZonePortal *p    = (VIZonePortal *)(i * 32 + r.m_portals);
        nv = p->m_numVerts;
        int           dest = p->m_destRoom;

        if ((p->m_p1C & 1) == 0)
        {
            if (p->m_plane.Distance(v.m_sphere.c) < 0.0f)
                continue;

            if (!v.m_clip->ClipByTop(nv, verts, n, &out))
                continue;

            if (v.m_clip->PushPortal(v.m_sphere.c, v.m_sphere.r, n, out,
                    ((VIZonePortal *)(r.m_portals + i * 32))->m_plane) < 0)
                return -1;
            v.m_clip->GetTop(v.m_numVerts, &v.m_verts, v.m_numPlanes,
                             &v.m_planes);

            ((VIZonePortal *)(i * 32 + r.m_portals))->m_p1C |= 1;

            if (!(m_rooms[dest].m_flags & 1))
            {
                if (QueryVisibleRoom(dest, v) < 0)
                    return -1;
            }
            else if (!v.m_depth)
            {
                if (v.m_world->QueryVisible(v) < 0)
                    return -1;
            }

            if (v.m_clip->Pop() < 0)
                return -1;
            v.m_clip->GetTop(v.m_numVerts, &v.m_verts, v.m_numPlanes,
                             &v.m_planes);

            ((VIZonePortal *)(i * 32 + r.m_portals))->m_p1C &= ~1;
        }
    }

    return 0;
}

// The intersection descent: the SPHERE walk while the query's bounding sphere
// is wholly on one side, the eight-corner VOLUME walk once it straddles.
int VIZone::QueryIntersection(int node, VISceneIntersectVars &v)
{
    if (node & 0xF0000000)
    {
        if (node & 0x40000000)
            return QueryIntersectionLeaf(node & 0x0FFFFFFF, v);

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

int VIZone::QueryIntersectionRoom(int room, VISceneIntersectVars &v)
{
    if (m_rooms[room].m_p24 != v.m_scene->m_stamp)
    {
        m_rooms[room].m_p24 = v.m_scene->m_stamp;

        if (m_rooms[room].m_p1C != -1)
            if (v.m_scene->QueryIntersectionSprite(m_rooms[room].m_p1C, v.m_bbox,
                                                   IdentityMatrix, *v.m_tris) < 0)
                return -1;
    }

    int n = m_rooms[room].m_p68;

    while (n != -1)
    {
        VISceneActorRef *a = (VISceneActorRef *)n;

        if (v.m_scene->QueryIntersectionActor(a->m_actor, v.m_bbox, *v.m_filter,
                                              *v.m_tris) < 0)
            return -1;

        n = a->m_next;
    }

    return 0;
}
