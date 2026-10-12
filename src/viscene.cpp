// VIScene -- SUPPORT 0x004613A8 .. 0x0046CC80.  Phase 54.
//
// The scene.  Its layout is in `vi.h`; what is worth saying here is the calling
// convention the whole file is built on: **an actor "index" is a pointer**.
// Every Set/Get/IsActor* takes an `int` and dereferences it without a bounds
// check, exactly the way a VIZone room's actor list does (phase 52).
#include "vi.h"

// ------------------------------------------------------------------- time --

int VIScene::SetTime(long t)
{
    m_time = t;
    return 0;
}

long VIScene::GetTime()
{
    return m_time;
}

// --------------------------------------------------------------- the world --

int VIScene::GetWorldGridNumRows()
{
    return m_world.m_numRows;
}

int VIScene::GetWorldGridNumCols()
{
    return m_world.m_numCols;
}

float VIScene::GetWorldGridSize()
{
    return m_world.m_cellSize;
}

VIBBox *VIScene::GetWorldBBox()
{
    return &m_world.m_bbox;
}

// ------------------------------------------------------------- the sprites --

VISprite *VIScene::Sprite(int i)
{
    return m_sprites.Value(i);
}

int VIScene::GetSpriteType(int i)
{
    return Sprite(i)->m_type;
}

int VIScene::ShareSprite(int i)
{
    m_sprites.Value(i)->m_link++;
    return 0;
}

int VIScene::GetNullSprite()
{
    return m_nullSprite;
}

int VIScene::SetLightUISprite(int i)
{
    m_lightUISprite = i;
    return 0;
}

int VIScene::GetLightUISprite()
{
    return m_lightUISprite;
}

int VIScene::SetSoundUISprite(int i)
{
    m_soundUISprite = i;
    return 0;
}

int VIScene::GetSoundUISprite()
{
    return m_soundUISprite;
}

int VIScene::SetParticleUISprite(int i)
{
    m_particleUISprite = i;
    return 0;
}

int VIScene::GetParticleUISprite()
{
    return m_particleUISprite;
}

int VIScene::SetPointUISprite(int i)
{
    m_pointUISprite = i;
    return 0;
}

int VIScene::GetPointUISprite()
{
    return m_pointUISprite;
}

int VIScene::SetStreamAudioUISprite(int i)
{
    m_streamAudioUISprite = i;
    return 0;
}

int VIScene::GetStreamAudioUISprite()
{
    return m_streamAudioUISprite;
}

// ---------------------------------------------------- animations / refmaps --

VIHSpriteAnim *VIScene::Animation(int i)
{
    return m_anims.Value(i);
}

int VIScene::ShareAnimation(int i)
{
    m_anims.Value(i)->m_userData++;
    return 0;
}

VIRefMap *VIScene::RefMap(int i)
{
    return m_refMaps.Value(i);
}

int VIScene::ShareRefMap(int i)
{
    m_refMaps.Value(i)->m_link++;
    return 0;
}

VIStaticLighting *VIScene::StaticLighting(int i)
{
    return m_lightings.Value(i);
}

int VIScene::ShareStaticLighting(int i)
{
    m_lightings.Value(i)->m_link++;
    return 0;
}

// ------------------------------------------------------------ misc setters --

int VIScene::SetPersonalLightMode(VIPersonalLightMode m)
{
    m_personalLightMode = m;
    return 0;
}

int VIScene::SetRadialFloraMode(VIRadialFloraMode m)
{
    m_floraMode = m;
    return 0;
}

void VIScene::ResetRadialFlora()
{
    m_flora.Reset();
}

void VIScene::ClearShadowQueue()
{
    m_numShadows = 0;
}

int VIScene::SetActorBBoxDrawStyle(VISceneBBoxDrawStyle s)
{
    m_bboxDrawStyle = s;
    return 0;
}

int VIScene::SetPortals(int on)
{
    if (!on)
        m_p00 |= 1;
    else
        m_p00 &= ~1u;
    return 0;
}

// ------------------------------------------------------------- the actors --

int VIScene::SetActorID(int i, int id)
{
    ((VIActor *)i)->m_p84 = id;
    return 0;
}

int VIScene::GetActorID(int i)
{
    return ((VIActor *)i)->m_p84;
}

int VIScene::IsActorVisible(int i)
{
    return ((((VIActor *)i)->m_p40 >> 0) ^ 1) & 1;
}

int VIScene::IsActorBBoxVisible(int i)
{
    return (((VIActor *)i)->m_p40 >> 1) & 1;
}

int VIScene::IsActorProxy(int i)
{
    return (((VIActor *)i)->m_p40 >> 2) & 1;
}

int VIScene::IsActorStatic(int i)
{
    return (((VIActor *)i)->m_p40 >> 3) & 1;
}

int VIScene::IsActorCollideable(int i)
{
    return ((((VIActor *)i)->m_p40 >> 4) ^ 1) & 1;
}

int VIScene::IsActorAudible(int i)
{
    return ((((VIActor *)i)->m_p40 >> 5) ^ 1) & 1;
}

int VIScene::SetActorVisibility(int i, int on)
{
    if (on)
        ((VIActor *)i)->m_p40 &= ~1u;
    else
        ((VIActor *)i)->m_p40 |= 1;
    return 0;
}

int VIScene::SetActorBBoxVisibility(int i, int on)
{
    if (on)
        ((VIActor *)i)->m_p40 |= 2;
    else
        ((VIActor *)i)->m_p40 &= ~2u;
    return 0;
}

int VIScene::SetActorCollideable(int i, int on)
{
    if (on)
        ((VIActor *)i)->m_p40 &= ~0x10u;
    else
        ((VIActor *)i)->m_p40 |= 0x10;
    return 0;
}

int VIScene::SetActorAudible(int i, int on)
{
    if (on)
        ((VIActor *)i)->m_p40 &= ~0x20u;
    else
        ((VIActor *)i)->m_p40 |= 0x20;
    return 0;
}

int VIScene::SetActorScale(int i, float f)
{
    ((VIActor *)i)->m_scale = f;
    if (UpdateActor(i) < 0)
        return -1;
    return 0;
}

int VIScene::SetActorLocation(int i, const VIVect3 &v)
{
    ((VIActor *)i)->m_location = v;
    if (UpdateActor(i) < 0)
        return -1;
    return 0;
}

int VIScene::SetActorHPR(int i, const VIVect3 &v)
{
    ((VIActor *)i)->m_hpr = v;
    if (UpdateActor(i) < 0)
        return -1;
    return 0;
}

// The two vectors come in ROTATION first: the second one lands in m_location.
int VIScene::SetActorTransform(int i, const VIVect3 &hpr, float scale,
                               const VIVect3 &at)
{
    ((VIActor *)i)->m_location = at;
    ((VIActor *)i)->m_hpr = hpr;
    ((VIActor *)i)->m_scale = scale;
    if (UpdateActor(i) < 0)
        return -1;
    return 0;
}

// ------------------------------------------------------- sprite delegates --

int VIScene::CopySprite(int i, int n)
{
    return m_sprites.Value(i)->Copy(*this, m_raster, m_sound, *m_collide, n);
}

int VIScene::RasterSprite(VICamera &cam, int i, unsigned int flags)
{
    if (m_raster == 0)
        return -1;
    return m_sprites.Value(i)->Raster(cam, *this, *m_raster, flags);
}

int VIScene::CollideSprite(VICollSphere &s, VISceneFilter &f, int i)
{
    int hit = m_sprites.Value(i)->Collide(s, f, *this, *m_collide);

    if (hit)
    {
        f.m_hitSprite = i;
        f.m_hitActor = -1;
        f.m_hitAttachment = -1;
    }
    return hit;
}

int VIScene::CollideSprite(VICollRay &r, VISceneFilter &f, int i)
{
    int hit = m_sprites.Value(i)->Collide(r, f, *this, *m_collide);

    if (hit)
    {
        f.m_hitSprite = i;
        f.m_hitActor = -1;
        f.m_hitAttachment = -1;
    }
    return hit;
}

int VIScene::PickSprite(VIPickSegment &seg, VISceneFilter &f, int i)
{
    int hit = m_sprites.Value(i)->Pick(seg, f, *this, *m_raster);

    if (hit)
    {
        f.m_hitSprite = i;
        f.m_hitActor = -1;
        f.m_hitAttachment = -1;
    }
    return hit;
}

// ------------------------------------------------------------- the queries --

int VIScene::Collide(VICollSphere &s, VISceneFilter &f)
{
    VISceneCollVars v;
    VIVect3        &end = s.m_end;

    v.m_scene = this;
    v.m_sphere = &s;
    v.m_filter = &f;
    v.m_radius = s.m_radius;
    v.m_collide = m_collide;
    v.m_start = s.m_start;
    v.m_end = end;
    v.m_stamp = 0;

    int r = m_world.Collide(v);
    m_stamp++;
    return r;
}

int VIScene::Collide(VICollRay &r, VISceneFilter &f)
{
    VISceneCollRayVars v;
    VIVect3           &end = r.m_end;

    v.m_scene = this;
    v.m_ray = &r;
    v.m_filter = &f;
    v.m_collide = m_collide;
    v.m_start = r.m_start;
    v.m_end = end;
    v.m_stamp = 0;

    int rc = m_world.Collide(v);
    m_stamp++;
    return rc;
}

int VIScene::Collide(VIEnvRay &r)
{
    r.m_firstCollide = 1;

    int rc = m_world.Collide(r);
    m_stamp++;
    return rc;
}

int VIScene::Pick(VIPickSegment &seg, VISceneFilter &f)
{
    VIScenePickVars v;
    VIVect3        &end = seg.m_end;

    v.m_scene = this;
    v.m_seg = &seg;
    v.m_filter = &f;
    v.m_raster = m_raster;
    v.m_start = seg.m_start;
    v.m_end = end;
    v.m_stamp = 0;

    int rc = m_world.Pick(v);
    m_stamp++;
    return rc;
}

int VIScene::QueryProximalActors(const VISphere &s, VISceneQuery &q)
{
    VISceneProxQueryVars v;

    v.m_sphere = s;
    v.m_stamp = 0;
    v.m_scene = this;
    v.m_query = &q;

    if (m_world.QueryProximal(v) < 0)
        return -1;

    q.EndQuery();
    m_stamp++;
    return 0;
}

int VIScene::ReleaseStatic(VIZone &z, VILoader &l)
{
    int k;
    int i;

    for (i = 0; i < z.m_rooms.m_size; i++)
    {
        for (k = 0; k < z.m_rooms[i].m_a3C.m_size; k++)
        {
            int actor = z.m_rooms[i].m_a3C[k];

            if (((VIActor *)actor)->m_p68 != -1)
            {
                if (l.Release(((VIActor *)actor)->m_p68, *this) < 0)
                    return -1;

                ((VIActor *)actor)->m_p68 = -1;
            }

            if (ReleaseActor(actor) < 0)
                return -1;
        }

        if (z.m_rooms[i].m_a44.Clear() < 0)
            return -1;

        if (z.m_rooms[i].m_a3C.Clear() < 0)
            return -1;
    }

    for (i = 0; i < z.m_rooms.m_size; i++)
    {
        if (z.m_rooms[i].m_flags & 4)
        {
            int idx;

            if (m_dict->FindTyped(z.m_rooms[i].m_p30, (VIResourceType)3, idx) == 0)
                return -1;

            if (m_raster)
            {
                m_raster->ReleaseMaterialPal(idx, *m_dict);
                z.m_rooms[i].m_flags &= ~4u;
            }
        }

        if (z.m_rooms[i].m_p34 != -1)
        {
            if (l.Release(z.m_rooms[i].m_p34, *this) < 0)
                return -1;

            z.m_rooms[i].m_p34 = -1;
        }

        if (z.m_rooms[i].m_p2C != -1)
        {
            if (l.Release(z.m_rooms[i].m_p2C, *this) < 0)
                return -1;

            z.m_rooms[i].m_p2C = -1;
        }

        if (z.m_rooms[i].m_p38 != -1)
        {
            if (l.Release(z.m_rooms[i].m_p38, *this) < 0)
                return -1;

            z.m_rooms[i].m_p38 = -1;
        }

        if (!(z.m_rooms[i].m_flags & 2))
        {
            if (z.m_rooms[i].m_p1C != -1)
            {
                if (ReleaseSprite(z.m_rooms[i].m_p1C) < 0)
                    return -1;
            }

            z.m_rooms[i].m_p1C = -1;
            z.m_rooms[i].m_flags |= 1;
        }
    }

    return 0;
}

// ------------------------------------------------------------ the streaming --

int VIScene::StreamInStaticActor(int i, VIZone &z, VISceneStreamVars &v)
{
    VIResourceType type;
    int            idx;

    if (v.m_dict->Find(((VIActor *)i)->m_p64, type, idx) == 0)
    {
        if (((VIActor *)i)->m_p68 != -1)
            goto done;

        ((VIActor *)i)->m_p68 =
            z.Load(((VIActor *)i)->m_p64, v.m_zone, *v.m_loader, v.m_file);

        if (((VIActor *)i)->m_p68 < 0)
            return -1;

        goto done;
    }

    ((VIActor *)i)->m_p60 = idx;

    if (m_sprites.Value(idx)->GetInstType(*this) == 1)
    {
        ((VIActor *)i)->m_p60 = CopySprite(idx, 0);
        m_sprites.Value(((VIActor *)i)->m_p60)->m_actor = idx;
        ShareSprite(idx);
    }

    if (ShareSprite(((VIActor *)i)->m_p60) < 0)
        return -1;

    if (((VIActor *)i)->m_p68 == -1)
        goto done;

    if (v.m_loader->Release(((VIActor *)i)->m_p68, *this) < 0)
        return -1;

    ((VIActor *)i)->m_p68 = -1;
done:
    return 0;
}



int VIScene::StreamFillCache(const VIVect3 &at, VILoader &l, int i)
{
    VISceneStreamRadialVars v;

    m_p2A58[2] = m_p2A48[1];
    m_p2A58[1] = m_p2A48[0];
    m_p2A58[4] = m_p2A48[3];
    m_p2A58[3] = m_p2A48[2];

    v.m_stamp = 0;
    v.m_scene = this;
    v.m_dict = m_dict;
    v.m_raster = m_raster;
    v.m_collide = m_collide;
    v.m_loader = &l;
    v.m_file = i;
    v.m_zone = -1;
    v.m_inner = m_p2A48[1];
    v.m_outer = m_p2A48[0];
    v.m_p2C = m_p2A48[3];
    v.m_p30 = m_p2A48[2];
    v.m_radius = 1000000.0f;
    v.m_at = at;

    m_p44C = 0;

    do
    {
        if (l.CompleteAll(*this) < 0)
            return -1;

        if (m_world.Stream(v) < 0)
            return -1;

        m_stamp++;
        v.m_radius = m_p2A58[0];
    }
    while (l.AllComplete() == 0);

    if (UpdateActors() < 0)
        return -1;

    m_p44C = 0;
    return 0;
}

int VIScene::StreamFillCache(const VIBBox &bb, VILoader &l, int i)
{
    VISceneBBoxStreamVars v;
    VIBBox                b;

    v.m_stamp = 0;
    v.m_scene = this;
    v.m_dict = m_dict;
    v.m_raster = m_raster;
    v.m_collide = m_collide;
    v.m_loader = &l;
    v.m_file = i;
    v.m_zone = -1;
    v.m_bbox = bb;

    v.m_bbox.min -= 100.0f;
    v.m_bbox.max += 100.0f;

    v.m_outer = v.m_bbox;
    v.m_outer.min -= 100.0f;
    v.m_outer.max += 100.0f;

    b = v.m_outer;
    b.min -= 1000000.0f;
    b.max += 1000000.0f;
    b.Store(v.m_corners);

    m_p44C = 0;

    do
    {
        if (l.CompleteAll(*this) < 0)
            return -1;

        if (m_world.Stream(v) < 0)
            return -1;

        m_stamp++;

        b = v.m_outer;
        b.min -= 100.0f;
        b.max += 100.0f;
        b.Store(v.m_corners);
    }
    while (l.AllComplete() == 0);

    if (UpdateActors() < 0)
        return -1;

    m_p44C = 0;
    return 0;
}

int VIScene::StreamClearCache(VILoader &l, int i)
{
    VISceneStreamRadialVars v;

    m_p2A58[1] = m_p2A48[0];
    m_p2A58[2] = m_p2A48[1];
    m_p2A58[3] = m_p2A48[2];
    m_p2A58[4] = m_p2A48[3];

    v.m_stamp = 0;
    v.m_scene = this;
    v.m_dict = m_dict;
    v.m_raster = m_raster;
    v.m_collide = m_collide;
    v.m_loader = &l;
    v.m_file = i;
    v.m_zone = -1;
    v.m_inner = 0.0f;
    v.m_outer = m_p2A48[0];
    v.m_p2C = 0;
    v.m_p30 = m_p2A48[2];
    v.m_radius = 1000000.0f;

    v.m_at = GetWorldBBox()->min;
    v.m_at -= GetWorldGridSize();

    if (m_world.Stream(v) < 0)
        return -1;

    return 0;
}

// ---------------------------------------------------------- the sound track --

int VIScene::StopSoundTrack(VILoader &l, int i)
{
    for (int k = 0; k < 2; k++)
    {
        if (m_soundTrack.m_state[k].m_p1C != -1)
            m_sound->Stop(m_soundTrack.m_state[k].m_p1C);

        if (m_soundTrack.m_state[k].m_p18 != -1)
            l.Release(m_soundTrack.m_state[k].m_p18, *this);

        if (m_soundTrack.m_state[k].m_p24 != -1)
            m_sound->ReleaseSound(m_soundTrack.m_state[k].m_p24, *m_dict);

        m_soundTrack.m_state[k].Clear();
    }

    if (m_soundTrack.m_p24 != -1)
    {
        m_sound->Stop(m_soundTrack.m_p24);
        m_soundTrack.m_p24 = -1;
    }

    m_soundTrack.m_p00 = 1;
    return 0;
}

// The outdoors probe: drop a 15-unit segment straight down from the camera and
// take the baked vertex colour it hits, then walk the stored intensity toward
// it at 0.2 per second.  The filter rejects everything that is not a STATIC
// actor, so only the world's own geometry answers.

int VIScene::CalcOutdoorsIntensity(VICamera &cam)
{
    VIVect3               a;
    VIVect3               b;
    VIPickSegment         seg;
    VISceneOutdoorsFilter f;

    VIVect3 &eye = cam.m_position;

    a = eye;

    b.x = eye.x;
    b.z = eye.z;
    b.y = eye.y - 15.0f;

    seg.Init(a, b);

    if (Pick(seg, f) == 0)
        return 0;

    if (m_soundTrack.m_p28 != 0)
    {
        float lim = (int)(GetTime() - m_soundTrack.m_p28) * 0.001f * 0.2f;
        float d = seg.GetHitColor().a - m_soundTrack.m_p30;

        float c = VIMIN(lim, VIMAX(-lim, d));

        m_soundTrack.m_p30 = m_soundTrack.m_p30 + c;
        m_soundTrack.m_p28 = GetTime();
        return 0;
    }

    m_soundTrack.m_p30 = seg.GetHitColor().a;
    m_soundTrack.m_p28 = GetTime();
    return 0;
}

// ------------------------------------------------------------- the rendering --

int VIScene::RasterActor(VICamera &cam, int i, VIClipStack &cs)
{
    if (((VIActor *)i)->m_p80 == m_stamp)
        goto done;

    if (((VIActor *)i)->m_p40 & 5)
        goto done;

    {
        VISprite *s = m_sprites.Value(((VIActor *)i)->m_p60);
        VIVect3  &eye = cam.m_position;
        VIVect3  &at = ((VIActor *)i)->m_location;

        float dx = at.x - eye.x;
        float dy = at.y - eye.y;
        float dz = at.z - eye.z;
        float d2 = dx * dx + dy * dy + dz * dz;

        if (!(d2 < s->m_offDist * s->m_offDist))
            goto done;

        if (cs.CullByTop(((VIActor *)i)->m_bound))
            goto done;

        ((VIActor *)i)->m_p80 = m_stamp;

        m_p4D0.Mul(((VIActor *)i)->m_objectToWorld, cam.m_view);
        m_raster->SetModelView(m_p4D0);

        if (s->m_type == 5 && d2 < m_farPlane)
        {
            int n = m_numShadows;

            if (n < 128)
            {
                m_shadows[n] = i;
                m_numShadows = n + 1;
            }
        }

        m_p444 = ((VIActor *)i)->m_p6C;
        m_p448 = -1;

        m_raster->SetStaticLighting(((VIActor *)i)->m_p70);

        if (s->Raster(cam, *this, *m_raster, 1))
        {
            VISceneDisplayElem e;

            e.m_b = i;
            e.m_c = ((VIActor *)i)->m_p60;
            e.m_a = -1;

            if (m_display.Add(e) < 0)
                return -1;
        }

        m_p444 = -1;
        m_p448 = -1;
    }
done:
    return 0;
}

// --------------------------------------------------------------- the actors --

int VIScene::CreateActor(const VIVect3 &at, const VIVect3 &hpr, float scale,
                         int sprite)
{
    int actor = m_actors.Add();

    if (actor < 0)
        goto bad;

    ((VIActor *)actor)->m_p80 = 0;
    ((VIActor *)actor)->m_p40 = 0;
    ((VIActor *)actor)->m_p98 = -1;
    ((VIActor *)actor)->m_p9C = -1;
    ((VIActor *)actor)->m_p84 = 0;
    ((VIActor *)actor)->m_location = at;
    ((VIActor *)actor)->m_hpr = hpr;
    ((VIActor *)actor)->m_scale = scale;
    ((VIActor *)actor)->m_p60 = sprite;
    ((VIActor *)actor)->m_p64 = 0;
    ((VIActor *)actor)->m_p68 = -1;
    ((VIActor *)actor)->m_p6C = -1;

    {
        VIColor32F &q = ((VIActor *)actor)->m_p70;

        q.r = 0.0f;
        q.g = 0.0f;
        q.b = 0.0f;
        q.a = 1.0f;
    }

    if (m_sprites.Value(sprite)->GetInstType(*this) == 1)
    {
        ((VIActor *)actor)->m_p60 = CopySprite(sprite, 0);
        m_sprites.Value(((VIActor *)actor)->m_p60)->m_actor = sprite;
        ShareSprite(sprite);
    }

    if (ShareSprite(((VIActor *)actor)->m_p60) < 0)
        goto bad;

    return UpdateActor(actor) < 0 ? -1 : actor;
bad:
    return -1;
}

int VIScene::CreateStaticProxyActor(const VIVect3 &at, const VIVect3 &hpr,
                                    float scale, unsigned int id, VIColor32 &c)
{
    int actor = m_actors.Add();

    if (actor < 0)
        goto bad;

    ((VIActor *)actor)->m_p40 = 12;
    ((VIActor *)actor)->m_p98 = -1;
    ((VIActor *)actor)->m_p9C = -1;
    ((VIActor *)actor)->m_p80 = 0;
    ((VIActor *)actor)->m_p84 = 0;
    ((VIActor *)actor)->m_location = at;
    ((VIActor *)actor)->m_hpr = hpr;
    ((VIActor *)actor)->m_p60 = -1;
    ((VIActor *)actor)->m_p68 = -1;
    ((VIActor *)actor)->m_p6C = -1;
    ((VIActor *)actor)->m_scale = scale;
    ((VIActor *)actor)->m_p64 = id;

    {
        VIColor32F &q = ((VIActor *)actor)->m_p70;

        q.r = c.r * (1.0f / 255.0f);
        q.g = c.g * (1.0f / 255.0f);
        q.b = c.b * (1.0f / 255.0f);
        q.a = c.a * (1.0f / 255.0f);
    }

    return UpdateProxyActor(actor) < 0 ? -1 : actor;
bad:
    return -1;
}

int VIScene::UpdateActor(int i)
{
    VIVect3           pts[8];
    VISceneInsertVars v;

    ((VIActor *)i)->CalcObjectToWorld();

    int sprite = ((VIActor *)i)->m_p60;

    ((VIActor *)i)->m_objectToWorld.Mul(m_sprites.Value(sprite)->m_bsphere.c,
                                        ((VIActor *)i)->m_bound.c);

    ((VIActor *)i)->m_bound.r = m_sprites.Value(sprite)->m_bsphere.r *
                                ((VIActor *)i)->m_scale;

    if (RemoveActor(i) < 0)
        return -1;

    v.m_stamp = 0;
    v.m_actor = i;
    v.m_zone = -1;
    v.m_scene = this;

    m_sprites.Value(sprite)->m_bbox.Transform(((VIActor *)i)->m_objectToWorld,
                                              v.m_corners);

    v.m_bound = ((VIActor *)i)->m_bound;

    if (m_world.InsertActor(v) < 0)
        return -1;

    m_stamp++;
    return 0;
}

// ---------------------------------------------------------- the emitters --
//
// The particle-emitter query, in two flavours: by SPHERE (the world walk hands
// every actor in range to QueryEmittersActor) and AT LOCATION, which skips the
// tree entirely -- FindLeaf resolves the camera's own zone and leaf and only
// that room's actors are asked.

int VIScene::QueryEmittersActor(int i, const VICamera &cam, const VISphere &s,
                                VIEmitterQuery &q)
{
    if (((VIActor *)i)->m_p80 == m_stamp)
        goto done;

    if (((VIActor *)i)->m_p40 & 13)
        goto done;

    ((VIActor *)i)->m_p80 = m_stamp;

    {
        int sprite = ((VIActor *)i)->m_p60;

        if (s.PointInSolid(((VIActor *)i)->m_bound.c) == 0)
            goto done;

        q.m_a = i;
        q.m_b = sprite;
        m_p4D0.Mul(((VIActor *)i)->m_objectToWorld, cam.m_view);

        if (m_sprites.Value(sprite)->QueryEmitters(*this, m_p4D0, q) < 0)
            return -1;
    }
done:
    return 0;
}

int VIScene::QueryEmittersAtLocationActor(int i, const VICamera &cam,
                                          VIEmitterQuery &q)
{
    if (((VIActor *)i)->m_p80 == m_stamp)
        goto done;

    if (((VIActor *)i)->m_p40 & 36)
        goto done;

    ((VIActor *)i)->m_p80 = m_stamp;

    {
        int sprite = ((VIActor *)i)->m_p60;

        if (((VIActor *)i)->m_bound.PointInSolid(cam.m_position) == 0)
            goto done;

        q.m_a = i;
        q.m_b = sprite;
        m_p4D0.Mul(((VIActor *)i)->m_objectToWorld, cam.m_view);

        if (m_sprites.Value(sprite)->QueryEmitters(*this, m_p4D0, q) < 0)
            return -1;
    }
done:
    return 0;
}

int VIScene::QueryEmittersAtLocation(const VICamera &cam, VIEmitterQuery &q)
{
    int zone;
    int leaf;

    if (m_world.FindLeaf(cam.m_position, zone, leaf) < 0)
        return -1;

    VIZone &z = m_world.m_zones[zone];
    int     n = z.m_rooms[z.m_tree.m_leaves[leaf].m_a].m_p68;

    while (n != -1)
    {
        VISceneActorRef *a = (VISceneActorRef *)n;

        if (QueryEmittersAtLocationActor(a->m_actor, cam, q) < 0)
            return -1;

        n = a->m_next;
    }

    m_stamp++;
    return 0;
}

// ------------------------------------------------------ the intersections --

int VIScene::QueryIntersectionActor(int i, VIBBox &bb, VIActorFilter &f,
                                    VIQueryTriList &tl)
{
    if (((VIActor *)i)->m_p80 == m_stamp)
        return 0;

    if (((VIActor *)i)->m_p40 & 4)
        return 0;

    ((VIActor *)i)->m_p80 = m_stamp;

    if (f.FilterActor(*this, i))
        return 0;

    int sprite = ((VIActor *)i)->m_p60;

    if (bb.CullIntersect(((VIActor *)i)->m_bound))
        return 0;

    return m_sprites.Value(sprite)->QueryIntersection(
        bb, tl, ((VIActor *)i)->m_objectToWorld, *this);
}

int VIScene::QueryIntersection(VIBBox &bb, VIActorFilter &f, VIQueryTriList &tl)
{
    VISceneIntersectVars v;

    v.m_bbox = bb;
    v.m_scene = this;
    v.m_filter = &f;
    v.m_collide = m_collide;
    v.m_tris = &tl;
    v.m_stamp = 0;

    bb.Store(v.m_corners);
    v.m_sphere.Init(bb);

    if (m_world.QueryIntersection(v) < 0)
        return -1;

    m_stamp++;
    return 0;
}

// ------------------------------------------------------------ the resources --
//
// The two halves of the dictionary's reference count.  The switch is on the
// resource TYPE, and its jump table is 26 entries wide with fourteen of them
// pointing at the sprite arm -- so a resource type outside 1..26, or 18 or 21,
// is an error and everything else that is not named below is a sprite.  (The
// table is a relocation, and the un-relocated image holds zeroes: these labels
// are read out of `memory-dumps/inworld-eqoa.live.eememory`, where the same
// build is linked.)

int VIScene::ShareResource(unsigned int id)
{
    VIResourceType type;
    int            i;

    if (m_dict->Find(id, type, i) == 0)
        return -1;

    switch (type)
    {
    case VIRT_Surface:
        m_raster->ShareSurface(i);
        break;
    case VIRT_Material:
        m_raster->ShareMaterial(i);
        break;
    case VIRT_MaterialPal:
        m_raster->ShareMaterialPal(i);
        break;
    case VIRT_Sprite:
    case VIRT_5:
    case VIRT_7:
    case VIRT_9:
    case VIRT_13:
    case VIRT_15:
    case VIRT_16:
    case VIRT_17:
    case VIRT_19:
    case VIRT_20:
    case VIRT_23:
    case VIRT_24:
    case VIRT_25:
    case VIRT_26:
        ShareSprite(i);
        break;
    case VIRT_Animation:
        ShareAnimation(i);
        break;
    case VIRT_RefMap:
        ShareRefMap(i);
        break;
    case VIRT_StaticLighting:
        ShareStaticLighting(i);
        break;
    case VIRT_CollBuffer:
        m_collide->ShareCollBuffer(i);
        break;
    case VIRT_PrimBuffer:
        m_raster->SharePrimBuffer(i);
        break;
    case VIRT_Font:
        m_raster->ShareFont(i);
        break;
    case VIRT_Sound:
        m_sound->ShareSound(i);
        break;
    default:
        return -1;
    }
    return 0;
}

int VIScene::ReleaseResource(unsigned int id)
{
    VIResourceType type;
    int            i;

    if (m_dict->Find(id, type, i) == 0)
        return -1;

    switch (type)
    {
    case VIRT_Surface:
        m_raster->ReleaseSurface(i, *m_dict);
        break;
    case VIRT_Material:
        m_raster->ReleaseMaterial(i, *m_dict);
        break;
    case VIRT_MaterialPal:
        m_raster->ReleaseMaterialPal(i, *m_dict);
        break;
    case VIRT_Sprite:
    case VIRT_5:
    case VIRT_7:
    case VIRT_9:
    case VIRT_13:
    case VIRT_15:
    case VIRT_16:
    case VIRT_17:
    case VIRT_19:
    case VIRT_20:
    case VIRT_23:
    case VIRT_24:
    case VIRT_25:
    case VIRT_26:
        ReleaseSprite(i);
        break;
    case VIRT_Animation:
        ReleaseAnimation(i);
        break;
    case VIRT_RefMap:
        ReleaseRefMap(i);
        break;
    case VIRT_StaticLighting:
        ReleaseStaticLighting(i);
        break;
    case VIRT_CollBuffer:
        m_collide->ReleaseCollBuffer(i);
        break;
    case VIRT_PrimBuffer:
        m_raster->ReleasePrimBuffer(i, *m_dict);
        break;
    case VIRT_Font:
        m_raster->ReleaseFont(i, *m_dict);
        break;
    case VIRT_Sound:
        m_sound->ReleaseSound(i, *m_dict);
        break;
    default:
        return -1;
    }
    return 0;
}

// --------------------------------------------------------- the sprite pool --

int VIScene::CreateSprite(VISpriteType type, int share)
{
    VISprite *s = 0;

    switch (type)
    {
    case VISpriteTypeSimple:      s = new VISimpleSprite(); break;
    case VISpriteTypePoint:       s = new VIPointSprite(); break;
    case VISpriteTypeSkin:        s = new VISkinSprite(); break;
    case VISpriteTypeH:           s = new VIHSprite(); break;
    case VISpriteTypeC:           s = new VICSprite(); break;
    case VISpriteTypeName:        s = new VINameSprite(); break;
    case VISpriteTypeLOD:         s = new VILODSprite(); break;
    case VISpriteTypePointLight:  s = new VIPointLight(); break;
    case VISpriteTypeSound:       s = new VISoundSprite(); break;
    case VISpriteTypeParticle:    s = new VIParticleSprite(); break;
    case VISpriteTypeGroup:       s = new VIGroupSprite(); break;
    case VISpriteTypeStreamAudio: s = new VIStreamAudioSprite(); break;
    case VISpriteTypeEffectVol:   s = new VIEffectVolumeSprite(); break;
    case VISpriteTypeFlora:       s = new VIFloraSprite(); break;
    }

    if (s == 0)
        return -1;

    if (share)
        s->m_link++;

    return m_sprites.Add(s);
}

int VIScene::ReleaseSprite(int i)
{
    if (m_sprites.Value(i)->m_actor != -1)
        ReleaseSprite(m_sprites.Value(i)->m_actor);

    if (--m_sprites.Value(i)->m_link <= 0)
    {
        if (m_sprites.Value(i)->Release(*this, *m_dict, m_raster, m_sound,
                                        *m_collide) < 0)
            return -1;

        delete m_sprites.Value(i);

        int r = m_sprites.Erase(i);
        if (r < 0)
            return -1;
    }
    return 0;
}

// ------------------------------------------------------- the resource pools --

int VIScene::CreateAnimation(int share)
{
    VIHSpriteAnim *a = new VIHSpriteAnim();

    int i = m_anims.Add(a);
    if (i < 0)
        return -1;

    if (share)
        a->m_userData++;

    return i;
}

int VIScene::ReleaseAnimation(int i)
{
    VIHSpriteAnim *a = m_anims.Value(i);

    if (--a->m_userData <= 0)
    {
        if (a->m_id)
            if (m_dict->Remove(*(VIResource *)a) < 0)
                return -1;

        delete a;

        int r = m_anims.Erase(i);
        if (r < 0)
            return -1;
    }
    return 0;
}

int VIScene::CreateRefMap(int share)
{
    VIRefMap *m = new VIRefMap();

    if (m == 0)
        return -1;

    int i = m_refMaps.Add(m);
    if (i < 0)
        return -1;

    if (share)
        m->m_link++;

    return i;
}

int VIScene::ReleaseRefMap(int i)
{
    VIRefMap *m = m_refMaps.Value(i);

    if (--m->m_link <= 0)
    {
        if (m->m_id)
            if (m_dict->Remove(*m) < 0)
                return -1;

        delete m;

        int r = m_refMaps.Erase(i);
        if (r < 0)
            return -1;
    }
    return 0;
}

int VIScene::CreateStaticLighting(int share)
{
    VIStaticLighting *l = new VIStaticLighting();

    int i = m_lightings.Add(l);
    if (i < 0)
        return -1;

    if (share)
        l->m_link++;

    return i;
}

int VIScene::ReleaseStaticLighting(int i)
{
    VIStaticLighting *l = m_lightings.Value(i);

    if (--l->m_link <= 0)
    {
        if (l->ReleaseResources(*m_raster, *m_dict) < 0)
            return -1;

        if (l->m_id)
            if (m_dict->Remove(*l) < 0)
                return -1;

        delete l;

        int r = m_lightings.Erase(i);
        if (r < 0)
            return -1;
    }
    return 0;
}

// ------------------------------------------------------------- the actors --

int VIScene::RemoveAllActors()
{
    for (int i = m_actors.Begin(); i != m_actors.End(); i = m_actors.Next(i))
        RemoveActor(i);
    return 0;
}

int VIScene::UpdateAllActors()
{
    for (int i = m_actors.Begin(); i != m_actors.End(); i = m_actors.Next(i))
        if (UpdateActor(i) < 0)
            return -1;
    return 0;
}

int VIScene::UpdateActors()
{
    for (int i = 0; i < m_p44C; i++)
        if (UpdateActor(m_p450[i]) < 0)
            return -1;

    m_p44C = 0;
    return 0;
}

int VIScene::UpdateActors(VIArray<int> &a)
{
    for (int i = 0; i < a.m_size; i++)
    {
        int actor = a[i];
        int r;

        if (((VIActor *)actor)->m_p40 & 4)
            r = UpdateProxyActor(actor);
        else
            r = UpdateActor(actor);

        if (r < 0)
            return -1;
    }
    return 0;
}

// ----------------------------------------------------- the actor delegates --
//
// Each of these is called from a VIZone room walk with ONE actor, and each
// stamps the actor with the scene's pass counter so a second room that lists
// the same actor skips it.  m_p40's bits are the actor's own filter: 0x04 is
// the proxy flag, 0x10 "not collideable" and 0x01 "hidden", so the collide
// pair tests 0x14 and the pick tests 0x05.

int VIScene::CollideActor(VICollSphere &s, VISceneFilter &f, int i)
{
    int hit = 0;

    if (m_collide->Cull(s, ((VIActor *)i)->m_bound) == 0)
    {
        if (((VIActor *)i)->m_p80 == m_stamp)
            goto done;

        if (((VIActor *)i)->m_p40 & 0x14)
            goto done;

        ((VIActor *)i)->m_p80 = m_stamp;

        if (f.FilterActor(*this, i))
            goto done;

        m_collide->SetWorld(((VIActor *)i)->m_objectToWorld);

        hit = m_sprites.Value(((VIActor *)i)->m_p60)->Collide(s, f, *this,
                                                              *m_collide);
        if (hit)
        {
            f.m_hitActor = i;
            f.m_hitSprite = ((VIActor *)i)->m_p60;
            f.m_hitAttachment = -1;
        }
        goto done;
    }
    ((VIActor *)i)->m_p80 = m_stamp;
done:
    return hit;
}

int VIScene::CollideActor(VICollRay &r, VISceneFilter &f, int i)
{
    int hit = 0;

    if (((VIActor *)i)->m_p80 == m_stamp)
        goto done;

    if (((VIActor *)i)->m_p40 & 0x14)
        goto done;

    ((VIActor *)i)->m_p80 = m_stamp;

    if (f.FilterActor(*this, i))
        goto done;

    {
        VISprite *sp = m_sprites.Value(((VIActor *)i)->m_p60);

        if (m_collide->Cull(r, ((VIActor *)i)->m_bound))
            goto done;

        m_collide->SetWorld(((VIActor *)i)->m_objectToWorld);

        hit = sp->Collide(r, f, *this, *m_collide);
        if (hit)
        {
            f.m_hitActor = i;
            f.m_hitSprite = ((VIActor *)i)->m_p60;
            f.m_hitAttachment = -1;
        }
    }
done:
    return hit;
}

int VIScene::PickActor(VIPickSegment &seg, VISceneFilter &f, int i)
{
    int hit = 0;

    if (((VIActor *)i)->m_p80 == m_stamp)
        goto done;

    if (((VIActor *)i)->m_p40 & 0x5)
        goto done;

    ((VIActor *)i)->m_p80 = m_stamp;

    if (f.FilterActor(*this, i))
        goto done;

    {
    int sprite = ((VIActor *)i)->m_p60;

    if (m_raster->Cull(seg, ((VIActor *)i)->m_bound))
        goto done;

    m_raster->SetModelView(((VIActor *)i)->m_objectToWorld);

    int lighting = ((VIActor *)i)->m_p6C;

    m_p448 = -1;
    m_p444 = lighting;

    m_raster->SetStaticLighting(*(const VIColor32F *)&((VIActor *)i)->m_p70);

    hit = m_sprites.Value(sprite)->Pick(seg, f, *this, *m_raster);
    if (hit)
    {
        f.m_hitActor = i;
        f.m_hitSprite = sprite;
        f.m_hitAttachment = -1;
    }
    m_p444 = -1;
    m_p448 = -1;
    }
done:
    return hit;
}

int VIScene::QueryVolumeActor(int i, VIBBox &bb, VISceneQuery &q)
{
    if (((VIActor *)i)->m_p80 == m_stamp)
        return 0;

    if (((VIActor *)i)->m_p40 & 4)
        return 0;

    ((VIActor *)i)->m_p80 = m_stamp;

    if (bb.Contains(((VIActor *)i)->m_location) == 0)
        return 0;

    if (q.FilterActor(*this, i) == 0)
    {
        int r = q.m_objects.Add(i);
        if (r < 0)
            return -1;
    }
    return 0;
}

int VIScene::GetActorTransform(int i, VIMatrix44 &m)
{
    m.Transform(((VIActor *)i)->m_hpr, ((VIActor *)i)->m_scale,
                ((VIActor *)i)->m_location);
    return 0;
}

int VIScene::SetActorLighting(int i, int lighting)
{
    if (((VIActor *)i)->m_p6C != -1)
        ReleaseStaticLighting(((VIActor *)i)->m_p6C);

    ((VIActor *)i)->m_p6C = lighting;

    if (lighting != -1)
        ShareStaticLighting(lighting);

    return 0;
}

int VIScene::StreamInCompleteActor(int i, VISceneStreamVars &v)
{
    ((VIActor *)i)->m_p40 &= ~4u;
    m_p450[m_p44C] = i;
    m_p44C++;
    return 0;
}

int VIScene::StreamHaltStaticActor(int i, VISceneStreamVars &v)
{
    if (((VIActor *)i)->m_p68 != -1)
    {
        if (v.m_loader->Release(((VIActor *)i)->m_p68, *this) < 0)
            return -1;
        ((VIActor *)i)->m_p68 = -1;
    }
    return 0;
}

int VIScene::RenderShadows()
{
    for (int i = 0; i < m_numShadows; i++)
        RenderShadow(m_shadows[i]);
    return 0;
}

int VIScene::SetFarPlane(VICamera &cam)
{
    if (m_raster)
        cam(*m_raster, CalcFarPlane(cam.m_position));
    return 0;
}

int VIScene::SetTimeOfDay(float t)
{
    float lo = 0.0f;

    // Read off the branches: the upper clamp is GUARDED by the lower test and
    // the tail re-tests it, so the two arms share one comparison result.
    if (!(t < lo) && 23.99999f < t)
        m_p1FE0 = 23.99999f;
    else if (t < lo)
        m_p1FE0 = lo;
    else
        m_p1FE0 = t;
    return 0;
}

float VIScene::CalcFarPlane(const VIVect3 &at)
{
    VITerrainProfileBlend b;
    float                 nearZ;
    float                 farZ;

    m_world.CalcProfileBlend(at, b);
    CalcTimeOfDayClipPlanes(b, nearZ, farZ);
    return farZ;
}

unsigned int VIScene::GetEnvFlags(const VIVect3 &at)
{
    int zone;
    int leaf;

    if (m_world.FindLeaf(at, zone, leaf) < 0)
        return ~0u;

    return m_world.m_zones[zone].m_tree.m_leaves[leaf].m_b;
}

int VIScene::GetNextColorBuffer()
{
    if (m_p444 != -1)
    {
        m_p448++;

        if (m_p448 < StaticLighting(m_p444)->m_buffers.m_size)
            return StaticLighting(m_p444)->m_buffers[m_p448];
    }

    return -1;
}

int VIScene::RasterUISprite(VICamera &cam, int i, unsigned int flags)
{
    int save = m_p444;

    m_p444 = -1;

    if (m_raster == 0)
        return -1;

    if (m_sprites.Value(i)->Raster(cam, *this, *m_raster, flags) < 0)
        return -1;

    m_p444 = save;
    return 0;
}

// ----------------------------------------------------- the zone's own actors --

int VIScene::ReplaceProxyRooms(VIZone &z)
{
    for (int i = 0; i < z.m_rooms.m_size; i++)
    {
        if (z.m_rooms[i].m_flags & 2)
        {
            VIResourceType type;
            int            idx;

            if (m_dict->Find(z.m_rooms[i].m_p28, type, idx))
            {
                if (type != VIRT_Sprite)
                    return -1;

                z.m_rooms[i].m_flags &= ~2u;
                z.m_rooms[i].m_p1C = idx;
            }
        }
    }
    return 0;
}

int VIScene::ReplaceStaticProxyActors(VIZone &z)
{
    for (int i = 0; i < z.m_rooms.m_size; i++)
    {
        for (int j = 0; j < z.m_rooms[i].m_a3C.m_size; j++)
        {
            int actor = z.m_rooms[i].m_a3C[j];

            if (((VIActor *)actor)->m_p40 & 4)
            {
                VIResourceType type;
                int            idx;

                if (m_dict->Find(((VIActor *)actor)->m_p64, type, idx) == 0)
                    return -1;

                if (ReplaceProxyActor(actor, idx) < 0)
                    return -1;
            }
        }
    }
    return 0;
}

int VIScene::ReplaceProxyActor(int i, int n)
{
    VISprite *sp = m_sprites.Value(n);

    ((VIActor *)i)->m_p60 = n;

    if (sp->GetInstType(*this) == 1)
    {
        ((VIActor *)i)->m_p60 = CopySprite(n, 0);
        m_sprites.Value(((VIActor *)i)->m_p60)->m_actor = n;
        ShareSprite(n);
    }

    if (ShareSprite(((VIActor *)i)->m_p60) < 0)
        return -1;

    if (UpdateActor(i) < 0)
        return -1;

    ((VIActor *)i)->m_p40 &= ~4u;
    return 0;
}

int VIScene::SetActorSprite(int i, int sprite)
{
    if (ReleaseSprite(((VIActor *)i)->m_p60) < 0)
        return -1;

    ((VIActor *)i)->m_p60 = sprite;

    if (m_sprites.Value(sprite)->GetInstType(*this) == 1)
    {
        ((VIActor *)i)->m_p60 = CopySprite(sprite, 0);
        m_sprites.Value(((VIActor *)i)->m_p60)->m_actor = sprite;
        ShareSprite(sprite);
    }

    if (ShareSprite(((VIActor *)i)->m_p60) < 0)
        return -1;

    if (UpdateActor(i) < 0)
        return -1;

    return 0;
}

int VIScene::ReleaseActor(int i)
{
    if (RemoveActor(i) < 0)
        return -1;

    if (((VIActor *)i)->m_p60 != -1)
        if (ReleaseSprite(((VIActor *)i)->m_p60) < 0)
            return -1;

    if (((VIActor *)i)->m_p6C != -1)
        if (ReleaseStaticLighting(((VIActor *)i)->m_p6C) < 0)
            return -1;

    return m_actors.Erase(i);
}

int VIScene::RemoveActor(int i)
{
    int occ = ((VIActor *)i)->m_p98;

    while (occ != -1)
    {
        VISceneOccup *o = (VISceneOccup *)occ;
        int           next = o->m_actorNext;
        int           zone = o->m_zone;
        int           room = o->m_room;

        if (o->m_next != -1)
            ((VISceneOccup *)o->m_next)->m_prev = o->m_prev;
        else
            m_world.m_zones[zone].m_rooms[room].m_p6C = o->m_prev;

        if (o->m_prev != -1)
            ((VISceneOccup *)o->m_prev)->m_next = o->m_next;
        else
            m_world.m_zones[zone].m_rooms[room].m_p68 = o->m_next;

        if (m_occups.Erase(occ) < 0)
            return -1;

        occ = next;
    }

    ((VIActor *)i)->m_p98 = -1;
    ((VIActor *)i)->m_p9C = -1;
    return 0;
}

int VIScene::UpdateProxyActor(int i)
{
    int zone;
    int leaf;

    if (m_world.FindLeaf(((VIActor *)i)->m_location, zone, leaf) < 0)
        return -1;

    int room = m_world.m_zones[zone].m_tree.m_leaves[leaf].m_a;

    if (RemoveActor(i) < 0)
        return -1;

    if (InsertActor(i, zone, room) < 0)
        return -1;

    return 0;
}

// Two passes over the zone's rooms: the first counts the actors that are in it
// (stamping each so an actor listed by two rooms is counted once), the second
// runs again -- the stamp is still the same value -- and this time collects and
// removes them.  It is the pass counter that makes one walk do the work of a
// set.
int VIScene::RemoveActorsFromZone(VIZone &z, VIArray<int> &a)
{
    int n = 0;

    m_stamp++;

    for (int i = 0; i < z.m_rooms.m_size; i++)
    {
        int occ = z.m_rooms[i].m_p68;

        while (occ != -1)
        {
            VIActor *actor = ((VISceneOccup *)occ)->m_actor;

            if (actor->m_p80 != m_stamp)
            {
                actor->m_p80 = m_stamp;
                n++;
            }
            occ = ((VISceneOccup *)occ)->m_next;
        }
    }

    if (a.Init(n) < 0)
        return -1;

    n = 0;

    for (int j = 0; j < z.m_rooms.m_size; j++)
    {
        int occ = z.m_rooms[j].m_p68;

        while (occ != -1)
        {
            VIActor *actor = ((VISceneOccup *)occ)->m_actor;

            if (actor->m_p80 == m_stamp)
            {
                a.m_data[n] = (int)actor;
                n++;
                RemoveActor((int)actor);
            }
            occ = z.m_rooms[j].m_p68;
        }
    }
    return 0;
}

int VIScene::QueryVisibleActors(const VISphere &s, VISceneQuery &q)
{
    VISceneVisQueryVars v;

    m_clipStack.Reset();

    v.m_sphere = s;
    v.m_clip = &m_clipStack;
    v.m_p38 = 1;
    v.m_stamp = 0;
    v.m_scene = this;
    v.m_query = &q;
    v.m_world = &m_world;
    v.m_numVerts = 0;
    v.m_verts = 0;
    v.m_numPlanes = 0;
    v.m_planes = 0;
    v.m_depth = 0;

    if (m_world.QueryVisibleWithPortals(v) < 0)
        return -1;

    q.EndQuery();
    m_stamp++;
    return 0;
}

int VIScene::QueryActorsInVolume(const VIBBox &bb, VISceneQuery &q)
{
    VISceneBBoxQueryVars v;

    v.m_stamp = 0;
    v.m_scene = this;
    v.m_bbox = bb;
    v.m_query = &q;
    v.m_bbox.Store(v.m_corners);

    if (m_world.QueryVolume(v) < 0)
        return -1;

    m_stamp++;
    return 0;
}

int VIScene::Collide(VICollSphere &s, VISceneFilter &f, VISceneQuery &q)
{
    int hit = 0;

    for (int i = 0; i < q.m_objects.m_size; i++)
    {
        VIVector<int> &objs = q.m_objects;

        int actor = objs.m_data[i];

        if (CollideActor(s, f, actor))
            hit = 1;
    }

    m_stamp++;
    return hit;
}

// -------------------------------------------------------- time of day (2/2) --
//
// The near and far clip planes are the terrain profile's first two floats,
// blended over the four profiles the point falls between.  It reads
// m_world.m_profiles directly -- +0x5C and +0x60 off the SCENE -- which is what
// pins the array to VIWorld+0x58 and puts the colour at profile+0x08.
int VIScene::CalcTimeOfDayClipPlanes(VITerrainProfileBlend &b, float &a,
                                     float &c)
{
    a = m_world.m_profiles[b.m_index[0]].m_p00 * b.m_weight[0];
    a += m_world.m_profiles[b.m_index[1]].m_p00 * b.m_weight[1];
    a += m_world.m_profiles[b.m_index[2]].m_p00 * b.m_weight[2];
    a += m_world.m_profiles[b.m_index[3]].m_p00 * b.m_weight[3];

    c = m_world.m_profiles[b.m_index[0]].m_p04 * b.m_weight[0];
    c += m_world.m_profiles[b.m_index[1]].m_p04 * b.m_weight[1];
    c += m_world.m_profiles[b.m_index[2]].m_p04 * b.m_weight[2];
    c += m_world.m_profiles[b.m_index[3]].m_p04 * b.m_weight[3];
    return 0;
}

int VIScene::QueryProximalActor(int i, VISphere &s, VISceneQuery &q)
{
    if (((VIActor *)i)->m_p80 == m_stamp)
        goto done;

    if (((VIActor *)i)->m_p40 & 4)
        goto done;

    if (q.FilterActor(*this, i))
        goto done;

    ((VIActor *)i)->m_p80 = m_stamp;

    {
        VIVect3 &at = ((VIActor *)i)->m_location;

        float dx = at.x - s.c.x;
        float dy = at.y - s.c.y;
        float dz = at.z - s.c.z;
        float d2 = dx * dx + dy * dy + dz * dz;

        if (d2 < s.r * s.r)
        {
            q.m_sorted.Insert(d2, i);

            if (q.m_maxObjects < q.m_sorted.Size())
                q.m_sorted.Erase(q.m_sorted.Prev(q.m_sorted.End()));
        }
    }
done:
    return 0;
}

int VIScene::QueryVisibleActor(int i, VISphere &s, VISceneQuery &q)
{
    if (((VIActor *)i)->m_p80 == m_stamp)
        goto done;

    if (((VIActor *)i)->m_p40 & 4)
        goto done;

    if (q.FilterActor(*this, i))
    {
        ((VIActor *)i)->m_p80 = m_stamp;
        goto done;
    }

    {
        VIVect3 &at = ((VIActor *)i)->m_location;

        float dx = at.x - s.c.x;
        float dy = at.y - s.c.y;
        float dz = at.z - s.c.z;
        float d2 = dx * dx + dy * dy + dz * dz;

        if (!(d2 < s.r * s.r))
        {
            ((VIActor *)i)->m_p80 = m_stamp;
            goto done;
        }

        if (m_clipStack.CullByTop(((VIActor *)i)->m_bound))
            goto done;

        ((VIActor *)i)->m_p80 = m_stamp;

        q.m_sorted.Insert(d2, i);

        if (q.m_maxObjects < q.m_sorted.Size())
            q.m_sorted.Erase(q.m_sorted.Prev(q.m_sorted.End()));
    }
done:
    return 0;
}

// ------------------------------------------------------ construct and clear --

VIScene::VIScene()
{
    m_stamp = 1;
    m_raster = 0;
    Clear();
}

int VIScene::Clear()
{
    m_time = 0;
    m_p00 = 0;
    m_p4BAC = 0;
    m_personalLightMode = VIPERSONALLIGHT_OFF;
    m_p2A38 = 0;
    m_p2A40 = 0;
    m_farPlane = 2500.0f;
    m_p1884 = -1;
    m_nullSprite = -1;
    m_lightUISprite = -1;
    m_soundUISprite = -1;
    m_particleUISprite = -1;
    m_pointUISprite = -1;
    m_streamAudioUISprite = -1;
    m_p1880 = -1;

    VIEffectVolume::GlobalFree();

    m_flora.Clear();
    m_soundTrack.Clear();

    if (m_raster)
        VITrailFxClear(*m_raster, *m_dict);

    m_spellEffects.Clear();

    if (m_raster)
        m_atmosphere.Clear(*this);

    if (m_particleSystem.Clear(*m_dict) < 0)
        return -1;

    if (m_actors.Clear() < 0)
        return -1;

    if (m_occups.Clear() < 0)
        return -1;

    for (int i = m_sprites.Begin(); i != m_sprites.End(); i = m_sprites.Next(i))
        delete m_sprites.Value(i);

    if (m_sprites.Clear() < 0)
        return -1;

    for (int j = m_lightings.Begin(); j != m_lightings.End();
         j = m_lightings.Next(j))
        delete m_lightings.Value(j);

    if (m_lightings.Clear() < 0)
        return -1;

    m_p444 = -1;
    m_p448 = -1;

    for (int k = m_refMaps.Begin(); k != m_refMaps.End(); k = m_refMaps.Next(k))
        delete m_refMaps.Value(k);

    if (m_refMaps.Clear() < 0)
        return -1;

    for (int l = m_anims.Begin(); l != m_anims.End(); l = m_anims.Next(l))
        delete m_anims.Value(l);

    if (m_anims.Clear() < 0)
        return -1;

    if (m_display.Clear() < 0)
        return -1;

    if (m_clipStack.Clear() < 0)
        return -1;

    if (m_queryTris.Clear() < 0)
        return -1;

    m_numShadows = 0;

    if (m_p450.Clear() < 0)
        return -1;

    if (m_world.Clear() < 0)
        return -1;

    m_dict = 0;
    m_raster = 0;
    m_sound = 0;
    m_collide = 0;
    return 0;
}

int VIScene::Init(long time, VIDictionary &d, VIRaster *r, VISoundDevice *sd,
                  VICollide &c)
{
    m_time = time;
    m_dict = &d;
    m_raster = r;
    m_sound = sd;
    m_collide = &c;

    if (m_actors.Init() < 0)
        return -1;

    if (m_occups.Init() < 0)
        return -1;

    if (m_sprites.Init(0) < 0)
        return -1;

    if (m_anims.Init(0) < 0)
        return -1;

    if (m_refMaps.Init(0) < 0)
        return -1;

    if (m_lightings.Init(0) < 0)
        return -1;

    {
        VIVect3 origin;
        VIBBox  bb;
        VIVect3 mn(-1000.0f, -300.0f, -1000.0f);
        VIVect3 mx(1000.0f, 300.0f, 1000.0f);

        bb.min = mn;
        bb.max = mx;

        if (m_world.Init(1, 1, 1, 4, 2000.0f, origin, bb) < 0)
            return -1;
    }

    m_nullSprite = CreateSprite(VISpriteTypePoint, 1);
    if (m_nullSprite < 0)
        return -1;

    m_p490.Identity();
    m_p4D0.Identity();

    if (m_clipStack.Init() < 0)
        return -1;

    if (m_p450.Init(5000) < 0)
        return -1;

    {
        VIBBox  qbb;
        VIVect3 dir(0.0f, 1.0f, 0.0f);

        if (m_queryTris.Init(200, qbb, dir, 80.0f) < 0)
            return -1;
    }

    m_numShadows = 0;

    for (int i = 0; i < 32; i++)
        m_p5B8[i] = -1;

    m_p638 = 0;
    m_p63C = m_particleSystem.Init(m_time, m_raster, 2000);

    if (m_raster)
        m_atmosphere.Init(m_time, *m_raster, 600);

    m_spellEffects.Init(this);

    if (m_raster)
        VITrailFxInit(*m_raster, *m_dict);

    if (InitTimeOfDay() < 0)
        return -1;

    SetPersonalLightMode(VIPERSONALLIGHT_OFF);

    {
        VIVect3    at;
        VIColor32F col(1.0f, 1.0f, 1.0f, 1.0f);

        SetPersonalLight(at, 20.0f, col);
    }

    m_soundTrack.Init();

    m_p2A70 = 0;
    m_bboxDrawStyle = VISCENEBBOX_NONE;

    m_p2A58[0] = 1050.0f;
    m_p2A58[1] = 950.0f;
    m_p2A58[2] = 900.0f;
    m_p2A58[3] = 550.0f;
    m_p2A58[4] = 500.0f;

    m_p2A48[0] = 950.0f;
    m_p2A48[1] = 900.0f;
    m_p2A48[2] = 550.0f;
    m_p2A48[3] = 500.0f;

    if (m_flora.Init(*this) < 0)
        return -1;

    m_floraMode = (VIRadialFloraMode)1;

    VIEffectVolume::GlobalInit(this);
    return 0;
}

int VIScene::SetPersonalLight(const VIVect3 &at, float r, const VIColor32F &c)
{
    m_personalLightAt = at;
    m_personalLightRadius = r;
    m_personalLightColor = c;
    return 0;
}

// The light does not sit ON the actor: it is lifted three units along the
// actor's own up axis, row 1 of its object-to-world.
int VIScene::SetPersonalLight(int i, float r, const VIColor32F &c)
{
    VIMatrix44 m;
    VIVect3    at;

    GetActorTransform(i, m);

    at.x = m.m[3].x + m.m[1].x * 3.0f;
    at.y = m.m[3].y + m.m[1].y * 3.0f;
    at.z = m.m[3].z + m.m[1].z * 3.0f;

    return SetPersonalLight(at, r, c);
}

int VIScene::QueryIntersectionSprite(int i, VIBBox &bb, const VIMatrix44 &m,
                                     VIQueryTriList &tl)
{
    return m_sprites.Value(i)->QueryIntersection(bb, tl, m, *this);
}

int VIScene::SetupTimeOfDay(VICamera &cam, VITerrainProfileBlend &b,
                            unsigned int f)
{
    SetupTimeOfDayLights(cam, b);
    SetupTimeOfDayFog(b, f);
    SetupTimeOfDaySky(b);
    return 0;
}

int VIScene::InsertActor(int actor, int zone, int room)
{
    int occ = m_occups.Add();

    if (occ < 0)
        return -1;

    VIZoneRoom &r = m_world.m_zones[zone].m_rooms[room];

    if (r.m_p68 != -1)
    {
        ((VISceneOccup *)r.m_p6C)->m_next = occ;
        ((VISceneOccup *)occ)->m_prev = r.m_p6C;
    }
    else
    {
        ((VISceneOccup *)occ)->m_prev = r.m_p68;
        r.m_p68 = occ;
    }
    r.m_p6C = occ;
    ((VISceneOccup *)occ)->m_next = -1;

    if (((VIActor *)actor)->m_p98 != -1)
    {
        ((VISceneOccup *)((VIActor *)actor)->m_p9C)->m_actorNext = occ;
        ((VISceneOccup *)occ)->m_actorPrev = ((VIActor *)actor)->m_p9C;
    }
    else
    {
        ((VISceneOccup *)occ)->m_actorPrev = ((VIActor *)actor)->m_p98;
        ((VIActor *)actor)->m_p98 = occ;
    }
    ((VIActor *)actor)->m_p9C = occ;

    ((VISceneOccup *)occ)->m_actor = (VIActor *)actor;
    ((VISceneOccup *)occ)->m_actorNext = -1;
    ((VISceneOccup *)occ)->m_zone = zone;
    ((VISceneOccup *)occ)->m_room = room;
    return 0;
}
