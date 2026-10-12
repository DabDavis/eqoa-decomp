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
    m_bbox.min.Zero();
    m_bbox.max.Zero();
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

    m_tree.m_leaves.m_data[0].m_a = 0;
    m_tree.m_leaves.m_data[0].m_b = 0;

    if (m_rooms.m_data[0].Init(0, 0, 0, 0, m_bbox) < 0)
        return -1;

    m_rooms.m_data[0].m_flags |= 1;
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
    int r = -1;
    int i;

    for (i = 0; i < m_flora.m_models.m_size; i++)
        sc.ReleaseSprite(m_flora.m_models.m_data[i].m_a);

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

    if (0 <= Init())
        r = 0;

    return r;
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

        return 0;
    }

    if (RenderByVolume(0x40000000, v) < 0)
        return -1;

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

        return 0;
    }

    if (back)
        if (RenderByVolume(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;

    if (front)
        if (RenderByVolume(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;

    return 0;
}

int VIZone::RenderLeaf(int leaf, VISceneRendVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 == v.m_stamp)
        return 0;

    m_rooms[room].m_p20 = v.m_stamp;

    if ((m_rooms[room].m_flags & 1) == 0)
        if (v.m_p2C)
            return 0;

    int r = RenderRoom(room, v);

    if (r < 0)
        return -1;

    return 0;
}

int VIZone::SetPretranslations(VIRaster &r, VICamera &cam)
{
    VIArray<VIVect3> &pt = m_pretrans;
    VIVect3           t;
    int               rc = -1;
    int               i;

    for (i = 0; i < pt.m_size; i++)
    {
        t.x = pt.m_data[i].x - cam.m_position.x;
        t.y = pt.m_data[i].y - cam.m_position.y;
        t.z = pt.m_data[i].z - cam.m_position.z;
        m_p[i] = t;
    }

    if (0 <= r.SetPreTranslations(pt.m_size, m_p, cam.m_position))
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

        return 0;
    }

    if (InsertActor(0x40000000, v) < 0)
        return -1;

    return 0;
}

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

            if (m_rooms[room].m_p20 == v.m_stamp)
                return 0;

            m_rooms[room].m_p20 = v.m_stamp;

            if (v.m_scene->InsertActor(v.m_actor, v.m_zone, room) < 0)
                return -1;

            return 0;
        }

        node &= 0x0FFFFFFF;
    }

    float d = m_tree.m_nodes[node].m_plane.Distance(v.m_bound.c);

    if (v.m_bound.r < d)
        return InsertActor(m_tree.m_nodes[node].m_front, v) < 0 ? -1 : 0;

    if (d < -v.m_bound.r)
        return InsertActor(m_tree.m_nodes[node].m_back, v) < 0 ? -1 : 0;

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

    return 0;
}

int VIZone::Stream(VISceneStreamRadialVars &v)
{
    if (m_tree.m_nodes.m_size)
    {
        if (Stream(0, v) < 0)
            return -1;

        return 0;
    }

    if (Stream(0x40000000, v) < 0)
        return -1;

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

        if (back == 0)
            return 0;

        return Stream(m_tree.m_nodes[node].m_back, v) < 0 ? -1 : 0;
    }

    if (back)
        if (Stream(m_tree.m_nodes[node].m_back, v) < 0)
            return -1;

    if (front == 0)
        return 0;

    return Stream(m_tree.m_nodes[node].m_front, v) < 0 ? -1 : 0;
}

// The radial leaf.  Two independent decisions on the same squared distance: the
// room's own geometry (flags bit 1 says it is proxy) and the room's STATICS
// (bits 3..5), each with an in/halt/out triple.
int VIZone::StreamLeaf(int leaf, VISceneStreamRadialVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 == v.m_stamp)
        return 0;

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
        return StreamRoomStaticsOut(room, v) < 0 ? -1 : 0;

    if ((m_rooms[room].m_flags & 0x38) == 0x38)
        return 0;

    if (d < v.m_p2C * v.m_p2C)
        return StreamRoomStaticsIn(room, v) < 0 ? -1 : 0;

    return StreamRoomStaticsHalt(room, v) < 0 ? -1 : 0;
}

int VIZone::Stream(VISceneBBoxStreamVars &v)
{
    int node = 0x40000000;

    if (m_tree.m_nodes.m_size)
        node = 0;

    if (Stream(node, v) < 0)
        return -1;

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

    if (m_rooms[room].m_p20 == v.m_stamp)
        return 0;

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
        return StreamRoomStaticsOut(room, v) < 0 ? -1 : 0;

    if ((m_rooms[room].m_flags & 0x38) == 0x38)
        return 0;

    if (v.m_bbox.Intersects(m_rooms[room].m_bbox))
        return StreamRoomStaticsIn(room, v) < 0 ? -1 : 0;

    return StreamRoomStaticsHalt(room, v) < 0 ? -1 : 0;
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
    int lo  = 0;
    int hi  = m_resources.m_size - 1;
    int mid = 0;

    while (lo <= hi)
    {
        mid = (lo + hi) >> 1;

        if (m_resources[mid].m_id == id)
            break;

        if (id < m_resources[mid].m_id)
            hi = mid - 1;
        else
            lo = mid + 1;
    }

    if (hi < lo)
        return 0;

    off  = m_resources[mid].m_offset;
    size = m_resources[mid].m_size;
    return 1;
}

int VIZone::QueryProximal(VISceneProxQueryVars &v)
{
    if (m_tree.m_nodes.m_size)
    {
        if (QueryProximal(0, v) < 0)
            return -1;

        return 0;
    }

    if (QueryProximal(0x40000000, v) < 0)
        return -1;

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
        return QueryProximal(m_tree.m_nodes[node].m_front, v) < 0 ? -1 : 0;

    if (d < -v.m_sphere.r)
        return QueryProximal(m_tree.m_nodes[node].m_back, v) < 0 ? -1 : 0;

    if (0.0f < d)
    {
        if (QueryProximal(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;

        return QueryProximal(m_tree.m_nodes[node].m_back, v) < 0 ? -1 : 0;
    }

    if (QueryProximal(m_tree.m_nodes[node].m_back, v) < 0)
        return -1;

    if (QueryProximal(m_tree.m_nodes[node].m_front, v) < 0)
        return -1;

    return 0;
}

int VIZone::QueryProximalLeaf(int leaf, VISceneProxQueryVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 == v.m_stamp)
        return 0;

    m_rooms[room].m_p20 = v.m_stamp;

    int r = QueryProximalRoom(room, v);

    if (r < 0)
        return -1;

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

        return 0;
    }

    if (QueryEmitters(0x40000000, v) < 0)
        return -1;

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
        return QueryEmitters(m_tree.m_nodes[node].m_front, v) < 0 ? -1 : 0;

    if (d < -v.m_sphere.r)
        return QueryEmitters(m_tree.m_nodes[node].m_back, v) < 0 ? -1 : 0;

    if (0.0f < d)
    {
        if (QueryEmitters(m_tree.m_nodes[node].m_front, v) < 0)
            return -1;

        return QueryEmitters(m_tree.m_nodes[node].m_back, v) < 0 ? -1 : 0;
    }

    if (QueryEmitters(m_tree.m_nodes[node].m_back, v) < 0)
        return -1;

    if (QueryEmitters(m_tree.m_nodes[node].m_front, v) < 0)
        return -1;

    return 0;
}

int VIZone::QueryEmittersLeaf(int leaf, VISceneEmitterQueryVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 == v.m_stamp)
        return 0;

    m_rooms[room].m_p20 = v.m_stamp;

    int r = QueryEmittersRoom(room, v);

    if (r < 0)
        return -1;

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

        return 0;
    }

    if (QueryVolume(0x40000000, v) < 0)
        return -1;

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

    if (m_rooms[room].m_p20 == v.m_stamp)
        return 0;

    m_rooms[room].m_p20 = v.m_stamp;

    int r = QueryVolumeRoom(room, v);

    if (r < 0)
        return -1;

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

        return 0;
    }

    if (QueryVisible(0x40000000, v) < 0)
        return -1;

    return 0;
}

int VIZone::QueryVisibleLeaf(int leaf, VISceneVisQueryVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 == v.m_stamp)
        return 0;

    m_rooms[room].m_p20 = v.m_stamp;

    if ((m_rooms[room].m_flags & 1) == 0)
        if (v.m_p38)
            return 0;

    int r = QueryVisibleRoom(room, v);

    if (r < 0)
        return -1;

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

        return 0;
    }

    if (QueryIntersection(0x40000000, v) < 0)
        return -1;

    return 0;
}

int VIZone::QueryIntersectionLeaf(int leaf, VISceneIntersectVars &v)
{
    int room = m_tree.m_leaves[leaf].m_a;

    if (m_rooms[room].m_p20 == v.m_stamp)
        return 0;

    m_rooms[room].m_p20 = v.m_stamp;

    int r = QueryIntersectionRoom(room, v);

    if (r < 0)
        return -1;

    return 0;
}
