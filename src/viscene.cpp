// VIScene -- SUPPORT 0x004613A8 .. 0x0046CC80.  Phase 54.
//
// The scene.  Its layout is in `vi.h`; what is worth saying here is the calling
// convention the whole file is built on: **an actor "index" is a pointer**.
// Every Set/Get/IsActor* takes an `int` and dereferences it without a bounds
// check, exactly the way a VIZone room's actor list does (phase 52).
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);
extern "C" int printf(const char *, ...);
int VIEEKBFree();
extern "C" char *strcpy(char *, const char *);
extern "C" char *strcat(char *, const char *);

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

    v.m_stamp = 0;
    v.m_scene = this;
    v.m_sphere = &s;
    v.m_filter = &f;
    v.m_collide = m_collide;
    v.m_start = s.m_start;
    v.m_end = end;
    v.m_radius = s.m_radius;

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

    if (Pick(seg, f))
    {
        if (m_soundTrack.m_p28 != 0)
        {
            float lim = (int)(GetTime() - m_soundTrack.m_p28) * 0.001f * 0.2f;
            float d = seg.GetHitColor().a - m_soundTrack.m_p30;

            float c = VIMIN(lim, VIMAX(-lim, d));

            m_soundTrack.m_p30 = m_soundTrack.m_p30 + c;
            m_soundTrack.m_p28 = GetTime();
        }
        else
        {
            m_soundTrack.m_p30 = seg.GetHitColor().a;
            m_soundTrack.m_p28 = GetTime();
        }
    }

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

        VISprite *sp = m_sprites.Value(((VIActor *)i)->m_p60);

        m_collide->SetWorld(((VIActor *)i)->m_objectToWorld);

        hit = sp->Collide(s, f, *this, *m_collide);
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
    m_p1FE0 = VIMIN(23.99999f, VIMAX(0.0f, t));
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
        int           next;
        int           room;
        int           zone;

        next = o->m_actorNext;
        zone = o->m_zone;
        room = o->m_room;

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
    int i;
    int occ;

    m_stamp++;

    for (i = 0; i < z.m_rooms.m_size; i++)
    {
        occ = z.m_rooms[i].m_p68;

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

    for (i = 0; i < z.m_rooms.m_size; i++)
    {
        occ = z.m_rooms[i].m_p68;

        while (occ != -1)
        {
            VIActor *actor = ((VISceneOccup *)occ)->m_actor;

            if (actor->m_p80 == m_stamp)
            {
                a[n] = (int)actor;
                n++;
                RemoveActor((int)((VISceneOccup *)occ)->m_actor);
            }
            occ = z.m_rooms[i].m_p68;
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

        int &actor = objs.m_data[i];

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
    if (((VIActor *)i)->m_p80 != m_stamp && !(((VIActor *)i)->m_p40 & 4))
    {
        if (!q.FilterActor(*this, i))
        {
            VIVect3 &at = ((VIActor *)i)->m_location;

            float dx = at.x - s.c.x;
            float dy = at.y - s.c.y;
            float dz = at.z - s.c.z;
            float d2 = dx * dx + dy * dy + dz * dz;

            if (d2 < s.r * s.r)
            {
                if (!m_clipStack.CullByTop(((VIActor *)i)->m_bound))
                {
                    ((VIActor *)i)->m_p80 = m_stamp;

                    q.m_sorted.Insert(d2, i);

                    if (q.m_maxObjects < q.m_sorted.Size())
                        q.m_sorted.Erase(q.m_sorted.Prev(q.m_sorted.End()));
                }
            }
            else
                ((VIActor *)i)->m_p80 = m_stamp;
        }
        else
            ((VIActor *)i)->m_p80 = m_stamp;
    }

    return 0;
}

// ------------------------------------------------------ construct and clear --

VIScene::VIScene()
{
    m_stamp = 1;
    m_raster = 0;
    Clear();
}

VIScene::~VIScene()
{
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
    m_nullSprite = -1;
    m_lightUISprite = -1;
    m_soundUISprite = -1;
    m_particleUISprite = -1;
    m_pointUISprite = -1;
    m_streamAudioUISprite = -1;
    m_p1880 = -1;
    m_p1884 = -1;

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
        VIBBox  bb(VIVect3(-1000.0f, -300.0f, -1000.0f),
                   VIVect3(1000.0f, 300.0f, 1000.0f));

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
    m_p63C = -1;
    m_particleSystem.Init(m_time, m_raster, 2000);

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

    m_p2A48[0] = 950.0f;
    m_p2A48[1] = 900.0f;
    m_p2A48[2] = 550.0f;
    m_p2A48[3] = 500.0f;

    m_p2A58[0] = 1050.0f;
    m_p2A58[1] = 950.0f;
    m_p2A58[2] = 900.0f;
    m_p2A58[3] = 550.0f;
    m_p2A58[4] = 500.0f;

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

    ((VISceneOccup *)occ)->m_actorNext = -1;
    ((VISceneOccup *)occ)->m_zone = zone;
    ((VISceneOccup *)occ)->m_room = room;
    ((VISceneOccup *)occ)->m_actor = (VIActor *)actor;
    return 0;
}

// ================================================================ phase 61 --

// ------------------------------------------------------ the const accessors --

VIRadialFloraMode VIScene::GetRadialFloraMode() const
{
    return m_floraMode;
}

VISceneBBoxDrawStyle VIScene::GetActorBBoxDrawStyle() const
{
    return m_bboxDrawStyle;
}

const VIVect3 &VIScene::GetActorLocation(int i) const
{
    return ((VIActor *)i)->m_location;
}

const VIVect3 &VIScene::GetActorHPR(int i) const
{
    return ((VIActor *)i)->m_hpr;
}

float VIScene::GetActorScale(int i) const
{
    return ((VIActor *)i)->m_scale;
}

int VIScene::GetActorSprite(int i) const
{
    return ((VIActor *)i)->m_p60;
}

VISprite *VIScene::GetActorSpritePtr(int i) const
{
    return m_sprites.Value(((VIActor *)i)->m_p60);
}

int VIScene::GetActorSpriteType(int i) const
{
    return m_sprites.Value(((VIActor *)i)->m_p60)->m_type;
}

int VIScene::GetNumActors() const
{
    return m_actors.Size();
}

int VIScene::GetFirstActor() const
{
    return m_actors.Begin() == m_actors.End() ? -1 : m_actors.Begin();
}

int VIScene::GetNextActor(int i) const
{
    return m_actors.Next(i) == m_actors.End() ? -1 : m_actors.Next(i);
}

// ------------------------------------------------------- the terrain grid --

int VIScene::GetTerrainGridNumRows() const
{
    return m_world.m_subdiv * m_world.m_numRows;
}

int VIScene::GetTerrainGridNumCols() const
{
    return m_world.m_subdiv * m_world.m_numCols;
}

float VIScene::GetTerrainGridSize() const
{
    return m_world.m_cellSize / (float)m_world.m_subdiv;
}

int VIScene::GetTerrainProfile(int row, int col) const
{
    return m_world.GetTerrainProfile(row, col);
}

int VIScene::GetTerrainProfile(const VIVect3 &at) const
{
    return m_world.CalcTerrainProfile(at);
}

int VIScene::GetTerrainGridCell(const VIVect3 &at, int &row, int &col) const
{
    return m_world.CalcTerrainCell(at, row, col);
}

int VIScene::GetTerrainGridCellBVolume(int row, int col, VIBBox &bb) const
{
    return m_world.CalcTerrainCellBVolume(row, col, bb);
}

// --------------------------------------------------------- the world grid --

int VIScene::GetWorldGridOffset(int a, int b, VIVect3 &v) const
{
    if (m_world.m_numRows == 1 && m_world.m_numCols == 1)
        v.Zero();
    else
    {
        v.x = (float)a * m_world.m_cellSize;
        v.y = 0.0f;
        v.z = (float)b * m_world.m_cellSize;
    }
    return 0;
}

int VIScene::GetWorldGridCenter(int a, int b, VIVect3 &v) const
{
    if (m_world.m_numRows == 1 && m_world.m_numCols == 1)
        v.Zero();
    else
    {
        v.x = (float)a * m_world.m_cellSize;
        v.y = 0.0f;
        v.z = (float)b * m_world.m_cellSize;
        v += m_world.m_origin;
    }
    return 0;
}

int VIScene::CalcWorldLocation(int a, int b, const VIVect3 &at, VIVect3 &out) const
{
    VIVect3 off;

    if (m_world.m_numRows == 1 && m_world.m_numCols == 1)
        off.Zero();
    else
    {
        off.x = (float)a * m_world.m_cellSize;
        off.y = 0.0f;
        off.z = (float)b * m_world.m_cellSize;
    }
    out = off;
    out += at;
    return 0;
}

int VIScene::CalcWorldGridOffset(const VIVect3 &v, int &a, int &b, VIVect3 &out) const
{
    if (m_world.m_numRows == 1 && m_world.m_numCols == 1)
    {
        b = 0;
        a = 0;
        out = v;
    }
    else
    {
        a = (int)(v.x / m_world.m_cellSize);
        b = (int)(v.z / m_world.m_cellSize);
        out = v;
        out.x -= (float)a * m_world.m_cellSize;
        out.z -= (float)b * m_world.m_cellSize;
        out -= m_world.m_origin;
    }
    return 0;
}

int VIScene::CalcActorBBox(int i, VIBBox &bb) const
{
    VIBBox  box;
    VIVect3 corners[8];
    VIActor *a = (VIActor *)i;

    if (!(a->m_p40 & 4))
    {
        box = m_sprites.Value(a->m_p60)->m_bbox;
        box.Transform(a->m_objectToWorld, corners);
        for (int k = 0; k < 8; k++)
            bb.Bound(k == 0, corners[k]);
    }
    else
        bb.Bound(1, a->m_location);
    return 0;
}

// The inverse of StreamInStaticActor and StreamInCompleteActor: retire the
// load, the sprite and the static lighting, mark the actor a proxy again
// (flag 4) and put it back where its proxy belongs.
int VIScene::StreamOutStaticActor(int i, VISceneStreamVars &v)
{
    if (((VIActor *)i)->m_p68 != -1)
    {
        if (v.m_loader->Release(((VIActor *)i)->m_p68, *this) < 0)
            return -1;
        ((VIActor *)i)->m_p68 = -1;
    }

    if (((VIActor *)i)->m_p60 != -1)
    {
        if (ReleaseSprite(((VIActor *)i)->m_p60) < 0)
            return -1;
        ((VIActor *)i)->m_p60 = -1;
    }

    if (((VIActor *)i)->m_p6C != -1)
    {
        if (ReleaseStaticLighting(((VIActor *)i)->m_p6C) < 0)
            return -1;
        ((VIActor *)i)->m_p6C = -1;
    }

    ((VIActor *)i)->m_p40 |= 4;

    if (UpdateProxyActor(i) < 0)
        return -1;

    return 0;
}

// ------------------------------------------------------------------ the sky --

// The sky type of the terrain profile last applied.  A file static in the
// image (it has no symbol, just an address in this TU's .bss), shared by
// nothing else.
static int s_skyType;

// The sky follows the terrain profile with the heaviest blend weight at the
// camera; the default sky is only re-set when that profile's sky type changes
// (or when the caller asks for a fade).  Profile 20 is the one that puts the
// sky's PLANE on.
int VIScene::SetSky(VICamera &cam, int fade)
{
    VITerrainProfileBlend b;
    float                 best;
    int                   k;
    int                   j;

    m_world.CalcProfileBlend(cam.m_position, b);

    for (j = 0; j < 4; j++)
    {
        if (j == 0 || best < b.m_weight[j])
        {
            best = b.m_weight[j];
            k = j;
        }
    }

    int sky = m_world.m_profiles[b.m_index[k]].m_p18;

    if (sky != s_skyType || fade)
        m_atmosphere.SetDefaultSky((VIAtmosphere::SKYTYPE)sky, fade != 0);

    if (b.m_index[k] == 20)
        m_atmosphere.SetPlaneOfSky(1);
    else
        m_atmosphere.SetPlaneOfSky(0);

    s_skyType = sky;
    return 0;
}

// The resources the sky and the shadows need, resolved once the dictionary is
// loaded: the atmosphere's own, the shadow material (one additive-blended,
// unlit layer of "shadow.tga") and the underwater ambient track.
int VIScene::SetSky()
{
    VIResourceType type;

    if (!m_raster)
        return -1;

    m_atmosphere.SetResources(*this);

    if (m_dict->Find(VIHashResourceID("shadow.tga"), type, m_p1884))
    {
        m_p1880 = m_raster->CreateMaterial(0);

        VIMaterial *m = m_raster->Material(m_p1880);

        m->Init(1);
        m->SetLayerFillType(0, (VIMaterialFillType)0);
        m->SetLayerModulate(0, 1);
        m->SetLayerBlendMode(0, (VIAlphaBlendMode)3);
        m->SetLayerTextureLODBias(0, 1.0f);
        m->SetZWrite(0);
        m->SetZTest(1);
        m->SetDither(0);
        m->SetLayerTexture(0, m_p1884, *m_raster, *m_dict);
        m->SetLayerWrapMode(0, (VIMaterialWrapMode)3);
    }

    if (!m_dict->Find(VIHashResourceID("amb_underwater"), type, m_soundTrack.m_p20))
        m_soundTrack.m_p20 = -1;

    return 0;
}

// --------------------------------------------------------------- streaming --

// One streaming step around `at`.  The radii adapt to the EE heap: under
// 2,560 KB free the two pairs shrink by 25 and 12.5 (and say so), over 4,096
// KB they grow back by the same steps, at most once every two seconds and
// never past the configured values in m_p2A48.
int VIScene::Stream(const VIVect3 &at, VILoader &l, int i)
{
    VISceneStreamRadialVars v;

    v.m_dict = m_dict;
    v.m_raster = m_raster;
    v.m_collide = m_collide;
    v.m_file = i;
    v.m_zone = -1;
    v.m_at = at;
    v.m_stamp = 0;
    v.m_scene = this;
    v.m_loader = &l;

    if (VIEEKBFree() < 2560)
    {
        m_p2A58[2] -= 25.0f;
        m_p2A58[1] -= 25.0f;
        m_p2A58[4] -= 12.5f;
        m_p2A58[3] -= 12.5f;
        printf("Automatically bringing in stream dist: %f %f %f %f\n",
               (double)m_p2A58[2], (double)m_p2A58[1], (double)m_p2A58[4],
               (double)m_p2A58[3]);
    }
    else if (VIEEKBFree() > 4096)
    {
        if (m_p2A70 + 2000 < m_time)
        {
            m_p2A58[2] += 25.0f;
            m_p2A58[1] += 25.0f;
            m_p2A58[4] += 12.5f;
            m_p2A58[3] += 12.5f;
            m_p2A58[2] = VIMIN(m_p2A48[1], m_p2A58[2]);
            m_p2A58[1] = VIMIN(m_p2A48[0], m_p2A58[1]);
            m_p2A58[4] = VIMIN(m_p2A48[3], m_p2A58[4]);
            m_p2A58[3] = VIMIN(m_p2A48[2], m_p2A58[3]);
            m_p2A70 = m_time;
        }
    }

    v.m_outer = m_p2A58[1];
    v.m_inner = m_p2A58[2];
    v.m_p30 = m_p2A58[3];
    v.m_p2C = m_p2A58[4];
    v.m_radius = m_p2A58[0];

    m_p44C = 0;

    if (m_world.Stream(v) < 0)
        return -1;

    if (l.Process(*this) < 0)
        return -1;

    m_stamp++;

    if (UpdateActors() < 0)
        return -1;

    return 0;
}

// --------------------------------------------------------------- rendering --

// The second pass over what RasterActor and VIZone::RenderRoom queued: each
// element is a (zone, actor, sprite) triple, and an actor of -1 is a room's
// own static sprite, drawn in the zone's world-space frame with no baked
// lighting.  The zone's pre-translations are re-applied whenever the zone
// changes along the list.  `actors` and `rooms` select which half is drawn.
int VIScene::RenderDeferredList(VICamera &cam, int actors, int rooms)
{
    VIMatrix44 m;
    int        zone = -1;
    int        i;

    for (i = 0; i < m_display.m_size; i++)
    {
        VIVector<VISceneDisplayElem> &list = m_display;
        VISceneDisplayElem &e0 = list.m_data[i];
        int z = e0.m_a;

        // -1 is tested FIRST (`li v0,-1; beq` then the spilled `zone`), as
        // the target does; what is left is that `li` being scheduled above the
        // element load and taking a0 instead of v0 (6 words; the other order
        // scores 3 but tests the operands the wrong way round).
        if (z != -1 && z != zone)
            m_world.m_zones[z].SetPretranslations(*m_raster, cam);

        zone = z;

        VISceneDisplayElem &e1 = list.m_data[i];
        int actor  = e1.m_b;
        int sprite = e1.m_c;

        if (actor == -1)
        {
            if (rooms)
            {
                sceVu0CopyMatrix(m, cam.m_view);
                m.m[3].x = 0.0f;
                m.m[3].y = 0.0f;
                m.m[3].z = 0.0f;
                m.m[3].w = 1.0f;
                m_raster->SetModelView(m);
                m_raster->SetStaticLighting(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));
                m_sprites.Value(sprite)->Raster(cam, *this, *m_raster, 2);
            }
        }
        else if (actors)
        {
            m.Mul(((VIActor *)actor)->m_objectToWorld, cam.m_view);
            m_raster->SetModelView(m);

            m_p444 = ((VIActor *)actor)->m_p6C;
            m_p448 = -1;

            m_raster->SetStaticLighting(((VIActor *)actor)->m_p70);
            m_sprites.Value(sprite)->Raster(cam, *this, *m_raster, 2);

            m_p444 = -1;
            m_p448 = -1;
        }
    }

    return 0;
}

// The frame.  Lights, atmosphere and the time-of-day blend first, then the
// world through portals (or by frustum, when bit 0 of m_p00 says the scene has
// no portals), the deferred room and actor passes either side of the radial
// flora, and the overlays -- name plates, particles, shadows, spell effects,
// trails and weather -- last.  `m` is dead: constructed and destroyed only.
int VIScene::Render(VICamera &cam)
{
    VIMatrix44            m;
    VISceneRendVars       v;
    VITerrainProfileBlend b;

    if (!m_raster)
        return -1;

    m_raster->SetTime(m_time);
    ClearShadowQueue();
    m_raster->SetProjection(cam.m_proj);
    CalcOutdoorsIntensity(cam);
    m_raster->SetStaticLighting(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));
    m_atmosphere.Update(*this, cam, m_time, m_p1FE0, m_soundTrack.m_p30);

    if (m_floraMode == 1)
        if (m_flora.Update(cam.m_position) < 0)
            return -1;

    unsigned int flags = GetEnvFlags(cam.m_position);

    m_world.CalcProfileBlend(cam.m_position, b);
    SetupTimeOfDay(cam, b, flags);
    m_atmosphere.RenderSky(*this, cam);
    SetupTimeOfDayFog(b, flags);
    m_raster->SetModelView(cam.m_view);
    m_clipStack.Reset();
    m_display.Init(1000);

    v.m_scene = this;
    v.m_world = 0;
    v.m_camera = &cam;
    v.m_raster = m_raster;
    v.m_clip = &m_clipStack;
    v.m_numVerts = 0;
    v.m_verts = 0;
    v.m_numPlanes = 0;
    v.m_planes = 0;
    v.m_p28 = 0;
    v.m_p2C = !(m_p00 & 1);
    v.m_zone = -1;
    float d = cam.m_far;
    if (500.0f < d)
        d = 500.0f;
    v.m_viewDist = d;

    if (m_p00 & 1)
    {
        if (m_world.RenderByFrustum(v) < 0)
            return -1;
    }
    else
    {
        if (m_world.RenderByPortals(v) < 0)
            return -1;
    }

    RenderDeferredList(cam, 0, 1);

    if (m_floraMode == 1)
        m_flora.Render(cam);

    RenderDeferredList(cam, 1, 0);

    m_stamp++;

    if (m_sound)
    {
        UpdateSoundEmitters(cam);
        m_p638++;
    }

    VINameSprite::RasterDeferred(*m_raster);
    m_raster->SetModelView(cam.m_view);
    m_particleSystem.Render(m_time, *m_raster);
    m_raster->SetModelView(cam.m_view);
    RenderShadows();
    m_spellEffects.Process();
    m_raster->EnableAllLights(0);
    VITrailFxRender(m_time, *m_raster, cam);
    m_atmosphere.RenderWeather(*this, cam);
    m_raster->SetStaticLighting(VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));

    return 0;
}

// ------------------------------------------------------------ time of day --

// Colour arithmetic, in place.  Inline, so each operand is a REFERENCE: the
// address of a table entry is materialised once per operation, which is what
// the target's repeated base registers say.
static inline VIColor32F &operator+=(VIColor32F &a, const VIColor32F &b)
{
    a.r += b.r; a.g += b.g; a.b += b.b; a.a += b.a;
    return a;
}

static inline VIColor32F &operator-=(VIColor32F &a, const VIColor32F &b)
{
    a.r -= b.r; a.g -= b.g; a.b -= b.b; a.a -= b.a;
    return a;
}

static inline VIColor32F &operator*=(VIColor32F &a, float f)
{
    a.r *= f; a.g *= f; a.b *= f; a.a *= f;
    return a;
}

static inline VIColor32F &operator*=(VIColor32F &a, const VIColor32F &b)
{
    a.r *= b.r; a.g *= b.g; a.b *= b.b; a.a *= b.a;
    return a;
}

static inline void ColorScale(VIColor32F &d, const VIColor32F &c, float f)
{
    d.r = c.r * f; d.g = c.g * f; d.b = c.b * f; d.a = c.a * f;
}

static inline void ColorAddScaled(VIColor32F &d, const VIColor32F &c, float f)
{
    d.r += c.r * f; d.g += c.g * f; d.b += c.b * f; d.a += c.a * f;
}

// The sky colour: the hour's two sky keys (m_d) interpolated between this
// hour and the next, then between clear and overcast, then tinted by the
// blended terrain profiles' own colour.  The profiles' Lavastorm weights are
// blended the same way and handed to the atmosphere.
int VIScene::SetupTimeOfDaySky(VITerrainProfileBlend &b)
{
    VIColor32F a;
    VIColor32  c;
    VIColor32F p;
    VIColor32  unused;
    VIColor32F x[2];

    float o = m_atmosphere.GetOvercastInterp();

    int   h    = (int)m_p1FE0;
    float f    = m_p1FE0 - (float)h;
    int   next = h + 1;

    if (next == 24)
        next = 0;

    ColorScale(p, m_world.m_profiles[b.m_index[0]].m_color, b.m_weight[0]);
    ColorAddScaled(p, m_world.m_profiles[b.m_index[1]].m_color, b.m_weight[1]);
    ColorAddScaled(p, m_world.m_profiles[b.m_index[2]].m_color, b.m_weight[2]);
    ColorAddScaled(p, m_world.m_profiles[b.m_index[3]].m_color, b.m_weight[3]);

    x[0] = m_tod[next].m_d[0];
    x[0] -= m_tod[h].m_d[0];
    x[0] *= f;
    x[0] += m_tod[h].m_d[0];

    x[1] = m_tod[next].m_d[1];
    x[1] -= m_tod[h].m_d[1];
    x[1] *= f;
    x[1] += m_tod[h].m_d[1];

    a = x[1];
    a -= x[0];
    a *= o;
    a += x[0];
    a *= p;

    c.r = (int)(a.r * 255.0f + 0.5f);
    c.g = (int)(a.g * 255.0f + 0.5f);
    c.b = (int)(a.b * 255.0f + 0.5f);
    c.a = (int)(a.a * 255.0f + 0.5f);

    m_atmosphere.SetSkyColor(*this, c);
    m_atmosphere.SetLavastormIntensity(
        m_world.m_profiles[b.m_index[0]].m_p24 * b.m_weight[0]
        + m_world.m_profiles[b.m_index[1]].m_p24 * b.m_weight[1]
        + m_world.m_profiles[b.m_index[2]].m_p24 * b.m_weight[2]
        + m_world.m_profiles[b.m_index[3]].m_p24 * b.m_weight[3]);

    return 0;
}

// The fog.  Four special cases on the environment flags GetEnvFlags returned,
// each a fixed tint over the current light colour (m_p1FE4, floored at 0.4)
// pulled in to 20 units, or to 5 -- bit 0 blue, bit 2 red, bit 1 a murky
// green-brown and bit 3 a fixed (70, 65, 50) brown -- and otherwise the sky
// colour SetupTimeOfDaySky computes, at the profiles' own distances.  The far
// plane never exceeds the current streaming radius.
int VIScene::SetupTimeOfDayFog(VITerrainProfileBlend &b, unsigned int f)
{
    VIColor32F fog;
    VIColor32F x[2];
    VIColor32F p;
    float      nearZ;
    float      farZ;

    float o    = m_atmosphere.GetOvercastInterp();
    int   h    = (int)m_p1FE0;
    float t    = m_p1FE0 - (float)h;
    int   next = h + 1;

    if (next == 24)
        next = 0;

    CalcTimeOfDayClipPlanes(b, nearZ, farZ);

    if (f & 1)
    {
        p = m_p1FE4;
        p.r = VIMAX(p.r, 0.4f);
        p.g = VIMAX(p.g, 0.4f);
        p.b = VIMAX(p.b, 0.4f);
        p.a = VIMAX(p.a, 0.4f);
        fog.Set(0.0f, 0.5f, 1.0f, 1.0f);
        fog.r = p.r * fog.r;
        fog.g = p.g * fog.g;
        fog.b = p.b * fog.b;
        fog.a = p.a * fog.a;
        nearZ = 0.0f;
        farZ = VIMIN(20.0f, farZ);
    }
    else if (f & 4)
    {
        fog.Set(1.0f, 0.0f, 0.0f, 1.0f);
        nearZ = 0.0f;
        farZ = VIMIN(20.0f, farZ);
    }
    else if (f & 2)
    {
        p = m_p1FE4;
        p.r = VIMAX(p.r, 0.4f);
        p.g = VIMAX(p.g, 0.4f);
        p.b = VIMAX(p.b, 0.4f);
        p.a = VIMAX(p.a, 0.4f);
        fog.r = p.r * 0.18f;
        fog.g = p.g * 0.56f;
        fog.b = p.b * 0.313f;
        fog.a = p.a;
        nearZ = 0.0f;
        farZ = VIMIN(20.0f, farZ);
    }
    else if (f & 8)
    {
        p = m_p1FE4;
        p.r = VIMAX(p.r, 0.4f);
        p.g = VIMAX(p.g, 0.4f);
        p.b = VIMAX(p.b, 0.4f);
        p.a = VIMAX(p.a, 0.4f);

        VIColor32 c(70, 65, 50, 255);

        fog.r = p.r * (c.r * (1.0f / 255.0f));
        fog.g = p.g * (c.g * (1.0f / 255.0f));
        fog.b = p.b * (c.b * (1.0f / 255.0f));
        fog.a = p.a * (c.a * (1.0f / 255.0f));
        nearZ = 0.0f;
        farZ = VIMIN(5.0f, farZ);
    }
    else
    {
        ColorScale(p, m_world.m_profiles[b.m_index[0]].m_color, b.m_weight[0]);
        ColorAddScaled(p, m_world.m_profiles[b.m_index[1]].m_color, b.m_weight[1]);
        ColorAddScaled(p, m_world.m_profiles[b.m_index[2]].m_color, b.m_weight[2]);
        ColorAddScaled(p, m_world.m_profiles[b.m_index[3]].m_color, b.m_weight[3]);

        x[0] = m_tod[next].m_d[0];
        x[0] -= m_tod[h].m_d[0];
        x[0] *= t;
        x[0] += m_tod[h].m_d[0];

        x[1] = m_tod[next].m_d[1];
        x[1] -= m_tod[h].m_d[1];
        x[1] *= t;
        x[1] += m_tod[h].m_d[1];

        fog = x[1];
        fog -= x[0];
        fog *= o;
        fog += x[0];
        fog *= p;
    }

    farZ = VIMIN(m_p2A58[2], farZ);

    m_raster->SetFog(1, nearZ, farZ, fog);
    return 0;
}

// The lights.  Ambient and sun colour are each the hour's two keys blended
// between this hour and the next and then between clear and overcast, the
// overcast weight being the atmosphere's overcast scaled by the terrain
// profiles' own; the sun's direction is the hour's, and a lightning flash
// whitens the sun (red and green by 0.9 of the flash, blue by all of it) and
// pins it overhead.  Then the personal light, fading in and out at
// 0.002/tick, and up to MaxPointLights() - 1 point-light sprites around the
// camera.
int VIScene::SetupTimeOfDayLights(VICamera &cam, VITerrainProfileBlend &b)
{
    VIColor32F              amb;
    VIEmitterQuery          q;
    VIColor32F              unused;
    VISceneEmitterQueryVars v;
    VIRasterPointLight      pl;
    VIRasterDirLight        sun;
    VIColor32F              x[2];
    float                   flash;
    int                     i;

    float o    = m_atmosphere.GetOvercastInterp();
    int   h    = (int)m_p1FE0;
    float t    = m_p1FE0 - (float)h;
    int   next = h + 1;

    if (next == 24)
        next = 0;

    float k = o * (m_world.m_profiles[b.m_index[0]].m_p1C * b.m_weight[0]
                   + m_world.m_profiles[b.m_index[1]].m_p1C * b.m_weight[1]
                   + m_world.m_profiles[b.m_index[2]].m_p1C * b.m_weight[2]
                   + m_world.m_profiles[b.m_index[3]].m_p1C * b.m_weight[3]);

    x[0] = m_tod[next].m_a[0];
    x[0] -= m_tod[h].m_a[0];
    x[0] *= t;
    x[0] += m_tod[h].m_a[0];

    x[1] = m_tod[next].m_a[1];
    x[1] -= m_tod[h].m_a[1];
    x[1] *= t;
    x[1] += m_tod[h].m_a[1];

    amb = x[1];
    amb -= x[0];
    amb *= k;
    amb += x[0];

    m_raster->SetAmbient(amb);
    m_raster->EnableAmbient(1);

    x[0] = m_tod[next].m_c[0];
    x[0] -= m_tod[h].m_c[0];
    x[0] *= t;
    x[0] += m_tod[h].m_c[0];

    x[1] = m_tod[next].m_c[1];
    x[1] -= m_tod[h].m_c[1];
    x[1] *= t;
    x[1] += m_tod[h].m_c[1];

    sun.m_color = x[1];
    sun.m_color -= x[0];
    sun.m_color *= k;
    sun.m_color += x[0];

    sun.m_direction = m_tod[next].m_b;
    sun.m_direction -= m_tod[h].m_b;
    sun.m_direction *= t;
    sun.m_direction += m_tod[h].m_b;
    sun.m_direction.Normalize();

    if (m_atmosphere.GetLightning(&flash))
    {
        VIColor32F c = sun.m_color;

        c.r += (1.0f - c.r) * flash * 0.9f * m_soundTrack.m_p30;
        c.g += (1.0f - c.g) * flash * 0.9f * m_soundTrack.m_p30;
        c.b += (1.0f - c.b) * flash * m_soundTrack.m_p30;
        sun.m_color = c;
        sun.m_direction = m_tod[10].m_b;
    }

    m_raster->SetSunLight(cam.m_view, sun);
    m_raster->EnableSunLight(1);
    m_p1FE4 = sun.m_color;
    m_raster->EnableAllDirLights(0);

    if (0 < m_raster->MaxPointLights())
    {
    for (i = 0; i < m_raster->MaxPointLights(); i++)
        m_raster->EnablePointLight(i, 0);

    m_raster->EnableTessellation(0);

    VIAtmosphere &atm = m_atmosphere;

    if ((unsigned int)m_personalLightMode < 2)
    {
        long  dt = GetTime() - m_p2A38;
        float step;

        if (dt > 0)
            step = (float)dt * 0.002f;
        else
            step = 0.0f;

        if (m_personalLightMode == 1 || m_p1FE0 < 6.0f || 16.0f < m_p1FE0
            || (0.0f < atm.m_intensity && 0.05f < m_soundTrack.m_p30 && 0.5f < o))
            m_p2A40 += step;
        else
            m_p2A40 -= step;

        m_p2A40 = VIMIN(m_personalLightRadius, VIMAX(0.0f, m_p2A40));

        if (0.0f < m_p2A40)
        {
            pl.m_position = m_personalLightAt;
            pl.m_range = m_p2A40;
            pl.m_color = m_personalLightColor;
            m_raster->SetPointLight(0, cam.m_view, pl);
            m_raster->EnablePointLight(0, 1);
            m_raster->EnableTessellation(1);
        }

        m_p2A38 = GetTime();
    }

    if (m_raster->MaxPointLights() >= 2)
    {
        q.Init(m_raster->MaxPointLights() - 1, (VISpriteType)8);

        const VIVect3 &at = cam.m_position;
        v.m_stamp = 0;
        v.m_scene = this;
        v.m_camera = &cam;
        v.m_sphere.r = 500.0f;
        v.m_sphere.c.x = at.x;
        v.m_sphere.c.y = at.y;
        v.m_sphere.c.z = at.z;
        v.m_query = &q;

        if (m_world.QueryEmitters(v) < 0)
            return -1;

        int n = 1;

        for (int it = q.m_map.Begin(); it != q.m_map.End(); it = q.m_map.Next(it))
        {
            VIPointLight *l =
                (VIPointLight *)m_sprites.Value(q.m_map.Value(it).m_b);

            pl.m_position.Zero();
            pl.m_range = l->m_bsphere.r;

            VIColor32F c = l->m_color;

            pl.m_color = c;
            m_raster->SetPointLight(n, q.m_map.Value(it).m_matrix, pl);
            m_raster->EnablePointLight(n, 1);
            n++;
        }
    }

    m_stamp++;
    }

    return 0;
}

// ------------------------------------------------------------------ sound --

// Three passes.  The streamed music: of the stream-audio emitters at the
// camera, the highest priority one plays on BGM slot 1 (restarted when it
// changes, or when it loops), and with none the slot stops.  The sound
// emitters: each starts once, and then every frame has its volume attenuated
// by distance over its scaled radius (linear, quartic, or inverted quartic)
// and its pan taken from the listener's right vector.  And the emitters heard
// LAST frame that are no longer heard are stopped -- m_p5B8 is that list.
int VIScene::UpdateSoundEmitters(VICamera &cam)
{
    VIEmitterQuery q;
    int            it;

    if (VIFile::IsIopStreamingEnabled())
    {
        q.Init(32, (VISpriteType)12);

        if (QueryEmittersAtLocation(cam, q) >= 0)
        {
            int best = -1;
            int pri  = 0;

            for (it = q.m_map.Begin(); it != q.m_map.End(); it = q.m_map.Next(it))
            {
                int s = q.m_map.Value(it).m_b;

                if (!(((VIStreamAudioSprite *)m_sprites.Value(s))->GetPriority() < pri))
                {
                    best = s;
                    pri = ((VIStreamAudioSprite *)m_sprites.Value(s))->GetPriority();
                }
            }

            if (best >= 0)
            {
                if (m_p63C != best
                    || ((VIStreamAudioSprite *)m_sprites.Value(best))->IsLooping())
                {
                    char path[256];

                    m_p63C = best;
                    strcpy(path, "Music\\");
                    strcat(path,
                           ((VIStreamAudioSprite *)m_sprites.Value(best))->GetRelativePathname());
                    m_sound->PlayBgm(1, path);
                }
            }
            else
            {
                m_p63C = -1;
                m_sound->StopBgm(1);
            }
        }
    }

    q.Init(32, (VISpriteType)9);

    int ok = !(QueryEmittersAtLocation(cam, q) < 0);

    if (ok)
    {
        for (it = q.m_map.Begin(); it != q.m_map.End(); it = q.m_map.Next(it))
        {
            int            s   = q.m_map.Value(it).m_b;
            VISoundSprite *snd = (VISoundSprite *)m_sprites.Value(s);

            if (snd->m_flags != m_p638 - 1)
            {
                snd->m_channel = m_sound->Play(snd->m_sound, 0, VISOUNDCLASS_SFX);

                if (((VISoundSprite *)m_sprites.Value(s))->m_channel <= 0)
                    continue;
            }

            VIVect3 d;
            VIVect3 right;
            VIActor *a = (VIActor *)q.m_map.Value(it).m_a;

            d.x = cam.m_position.x - a->m_location.x;
            d.y = 0.0f;
            d.z = cam.m_position.z - a->m_location.z;
            right.x = cam.m_dir.z;
            right.y = 0.0f;
            right.z = -cam.m_dir.x;
            d.Normalize();
            right.Normalize();

            float pan = d.x * right.x + d.z * right.z;

            snd = (VISoundSprite *)m_sprites.Value(s);

            float t = cam.m_position.Distance(((VIActor *)q.m_map.Value(it).m_a)->m_location)
                      / (((VIActor *)q.m_map.Value(it).m_a)->m_scale
                         * m_sprites.Value(s)->m_bsphere.r);

            switch (snd->AttenuationMode())
            {
            case 0:
                break;
            case 1:
                t *= t * t * t;
                break;
            case 2:
                t = 1.0f - t;
                t *= t * t * t;
                t = 1.0f - t;
                break;
            }

            m_sound->SetVolume(snd->m_channel, (1.0f - t) * snd->m_volume);

            float p = 0.0f;
            if (snd->IsPanEnabled())
                p = pan;

            m_sound->SetPan(snd->m_channel, p);
            ((VISoundSprite *)m_sprites.Value(s))->m_flags = m_p638;
        }
    }

    for (int k = 0; k < 32; k++)
    {
        if (m_p5B8[k] != -1)
        {
            int found = 0;

            for (it = m_sprites.Begin(); it != m_sprites.End(); it = m_sprites.Next(it))
                if (it == m_p5B8[k])
                {
                    found = 1;
                    break;
                }

            if (found)
            {
                VISoundSprite *snd = (VISoundSprite *)m_sprites.Value(m_p5B8[k]);

                if (snd->m_flags != m_p638)
                    m_sound->Stop(snd->m_channel);
            }

            m_p5B8[k] = -1;
        }
    }

    if (ok)
    {
        int j = 0;

        for (it = q.m_map.Begin(); it != q.m_map.End(); it = q.m_map.Next(it))
            m_p5B8[j++] = q.m_map.Value(it).m_b;
    }

    return 0;
}

// The ambient music: the terrain profile with the heaviest weight at the
// camera names a day and a night track.  A change of track (at most once per
// m_p10 ms, or straight away on the first call) moves the playing state to
// the second slot to fade out and starts loading the new one; each slot then
// fades over m_p10 ms, a slot that has finished fading out is retired, and a
// loaded one starts playing.  Under water (env flags 0xF) the music is muted
// and the underwater ambience plays instead.
int VIScene::SetSoundTrack(VICamera &cam, VILoader &l, int file)
{
    VITerrainProfileBlend b;
    VIPickSegment         unused;
    int                   idx;
    float                 best;
    float                 v;
    int                   k;
    int                   j;

    m_world.CalcProfileBlend(cam.m_position, b);

    for (j = 0; j < 4; j++)
    {
        if (j == 0 || best < b.m_weight[j])
        {
            best = b.m_weight[j];
            k = j;
        }
    }

    unsigned int env   = GetEnvFlags(cam.m_position);
    int          prof  = m_world.m_profiles[b.m_index[k]].m_p20;
    int          night;

    if (m_p1FE0 < 5.0f || 20.0f < m_p1FE0)
        night = 1;
    else
        night = 0;

    int          track = m_soundTrack.m_tracks[prof][night];

    if (m_soundTrack.m_p10 < m_time - m_soundTrack.m_p18 || m_soundTrack.m_p00)
    {
        if (track != m_soundTrack.m_state[0].m_p20)
        {
            if (m_soundTrack.m_state[1].m_p1C != -1)
                m_sound->Stop(m_soundTrack.m_state[1].m_p1C);

            if (m_soundTrack.m_state[1].m_p18 != -1)
                l.Release(m_soundTrack.m_state[1].m_p18, *this);

            if (m_soundTrack.m_state[1].m_p24 != -1)
                m_sound->ReleaseSound(m_soundTrack.m_state[1].m_p24, *m_dict);

            if (m_soundTrack.m_state[0].m_p24 != -1)
            {
                m_soundTrack.m_state[0].m_p00 = 1;
                m_soundTrack.m_state[0].m_p08 = m_time;
                m_soundTrack.m_state[0].m_p10 = m_soundTrack.m_state[0].m_p14;
            }

            m_soundTrack.m_state[1].Clear();
            m_soundTrack.m_state[1] = m_soundTrack.m_state[0];
            m_soundTrack.m_state[0].Clear();
            m_soundTrack.m_state[0].m_p20 = track;
            m_soundTrack.m_p18 = m_time;
        }
    }

    if (m_soundTrack.m_state[0].m_p18 != -1)
    {
        if (l.IsComplete(m_soundTrack.m_state[0].m_p18))
        {
            if (!m_dict->FindTyped(track, (VIResourceType)14, idx))
                return -1;

            m_sound->ShareSound(idx);
            m_sound->Sound(idx)->AsXm()->SetRepeat(1);
            m_soundTrack.m_state[0].m_p24 = idx;
            l.Release(m_soundTrack.m_state[0].m_p18, *this);
            m_soundTrack.m_state[0].m_p18 = -1;
        }
    }
    else if (m_soundTrack.m_state[0].m_p24 == -1)
    {
        if (m_dict->FindTyped(track, (VIResourceType)14, idx))
        {
            m_sound->ShareSound(idx);
            m_sound->Sound(idx)->AsXm()->SetRepeat(1);
            m_soundTrack.m_state[0].m_p24 = idx;
        }
        else if (l.IsResourceAvailable(file, track))
        {
            m_soundTrack.m_state[0].m_p18 = l.Load(file, track);
            if (m_soundTrack.m_state[0].m_p18 < 0)
                return -1;
        }
    }

    for (j = 0; j < 2; j++)
    {
        if (m_soundTrack.m_state[j].m_p1C == -1)
            continue;

        if (m_soundTrack.m_state[j].m_p00 < 3)
        if (m_soundTrack.m_state[j].m_p00 > 0)
        {
            float t = (float)(m_time - m_soundTrack.m_state[j].m_p08)
                      / (float)m_soundTrack.m_p10;

            t = VIMIN(1.0f, VIMAX(0.0f, t));

            v = 0.0f;
            if (m_soundTrack.m_state[j].m_p00 != 1)
                v = m_soundTrack.m_p04;

            v -= m_soundTrack.m_state[j].m_p10;
            v *= t;
            v += m_soundTrack.m_state[j].m_p10;
            m_soundTrack.m_state[j].m_p14 = v;

            if (t == 1.0f)
            {
                if (m_soundTrack.m_state[j].m_p00 == 1)
                    m_soundTrack.m_state[j].m_p00 = 3;
                else if (m_soundTrack.m_state[j].m_p00 == 2)
                    m_soundTrack.m_state[j].m_p00 = 0;
            }
        }

        VIAtmosphere &atm = m_atmosphere;
        float q = 1.0f - atm.m_intensity;
        v = m_soundTrack.m_state[j].m_p14;
        v *= q;
        v *= q;
        v *= q;
        v *= m_soundTrack.m_p30;

        if (env & 0xF)
            v = 0.0f;

        m_sound->SetVolume(m_soundTrack.m_state[j].m_p1C, v);
    }

    for (j = 0; j < 2; j++)
    {
        if (m_soundTrack.m_state[j].m_p00 == 3)
        {
            if (m_soundTrack.m_state[j].m_p1C != -1)
                m_sound->Stop(m_soundTrack.m_state[j].m_p1C);

            if (m_soundTrack.m_state[j].m_p18 != -1)
                l.Release(m_soundTrack.m_state[j].m_p18, *this);

            if (m_soundTrack.m_state[j].m_p24 != -1)
                m_sound->ReleaseSound(m_soundTrack.m_state[j].m_p24, *m_dict);

            m_soundTrack.m_state[j].Clear();
        }
    }

    for (j = 0; j < 2; j++)
    {
        if (m_soundTrack.m_state[j].m_p1C != -1 || m_soundTrack.m_state[j].m_p24 == -1)
            continue;

        idx = m_soundTrack.m_state[j].m_p24;

        if (!m_sound->Sound(idx)->AsXm()->IsCached())
            continue;

        m_soundTrack.m_state[j].m_p1C = m_sound->Play(idx, 0, VISOUNDCLASS_SFX);

        if (m_soundTrack.m_p00)
        {
            m_soundTrack.m_state[j].m_p14 = 1.0f;
            m_soundTrack.m_p00 = 0;
        }
        else
            m_soundTrack.m_state[j].m_p14 = 0.0f;

        m_soundTrack.m_state[j].m_p00 = 2;
        m_soundTrack.m_state[j].m_p08 = m_time;
        m_soundTrack.m_state[j].m_p10 = m_soundTrack.m_state[j].m_p14;

        VIAtmosphere &atm = m_atmosphere;
        float q = 1.0f - atm.m_intensity;
        v = m_soundTrack.m_state[j].m_p10 * q;
        v *= q;
        v *= q;
        v *= m_soundTrack.m_p30;

        if (env & 0xF)
            v = 0.0f;

        m_sound->SetVolume(m_soundTrack.m_state[j].m_p1C, v);
    }

    if (env & 0xF)
    {
        if (m_soundTrack.m_p24 == -1 && m_soundTrack.m_p20 != -1)
        {
            m_soundTrack.m_p24 = m_sound->Play(m_soundTrack.m_p20, 0, VISOUNDCLASS_SFX);
            m_sound->SetVolume(m_soundTrack.m_p24, 0.7f);
        }
    }
    else if (m_soundTrack.m_p24 != -1)
    {
        m_sound->Stop(m_soundTrack.m_p24);
        m_soundTrack.m_p24 = -1;
    }

    return 0;
}

// ------------------------------------------------------------------ debug --

// A census of what the scene holds, printed, and the total byte count of the
// variable-sized allocations returned.  The sprite census counts by type; the
// sizes are element sizes times counts plus each object's own data block.
int VIScene::DebugResources()
{
    int counts[15];
    int i;
    int total;
    int n;
    int size;

    for (i = 14; i >= 0; i--)
        counts[i] = 0;

    for (i = m_sprites.Begin(); i != m_sprites.End(); i = m_sprites.Next(i))
        counts[GetSpriteType(i)]++;

    printf("Sprites: %d\n", m_sprites.Size());
    printf(" Simple: %d\n", counts[1]);
    printf(" Skin: %d\n", counts[3]);
    printf(" HSprite: %d\n", counts[4]);
    printf(" CSprite: %d\n", counts[5]);
    printf(" Name: %d\n", counts[6]);
    printf(" LOD: %d\n", counts[7]);
    printf(" PLight: %d\n", counts[8]);
    printf(" SndSprite: %d\n", counts[9]);
    printf(" Group: %d\n", counts[11]);
    printf(" StreamAudio: %d\n", counts[12]);
    printf(" Flora: %d\n", counts[13]);
    printf(" EffectVolume: %d\n", counts[14]);

    printf("Actors: %d\n", m_actors.Size());
    printf("Actors Size: %d\n", m_actors.Size() * 160);
    printf("Actor Occups: %d\n", m_occups.Size());
    printf("Actor Occups Size: %d\n", m_occups.Size() * 24);

    size = 0;
    for (i = m_anims.Begin(); i != m_anims.End(); i = m_anims.Next(i))
        size = size + 56 + m_anims.Value(i)->m_dataSize;

    printf("Animations: %d\n", m_anims.Size());
    total = size;
    printf("Animations Size: %d\n", total);

    n = 0;
    size = 0;
    for (i = 0; i < m_world.m_zones.m_size; i++)
    {
        n += m_world.m_zones[i].m_rooms.m_size;

        for (int r = 0; r < m_world.m_zones[i].m_rooms.m_size; r++)
            size += m_world.m_zones[i].m_rooms[r].m_dataSize + 120;
    }

    printf("Rooms: %d\n", n);
    total += size;
    printf("Rooms Size: %d\n", size);

    n = m_world.m_tree.m_nodes.m_size;
    size = m_world.m_tree.m_nodes.m_size * 24;
    n += m_world.m_tree.m_leaves.m_size;
    size += m_world.m_tree.m_leaves.m_size * 4;

    for (i = 0; i < m_world.m_zones.m_size; i++)
    {
        n += m_world.m_zones.m_data[i].m_tree.m_nodes.m_size;
        n += m_world.m_zones.m_data[i].m_tree.m_leaves.m_size;
        size += m_world.m_zones.m_data[i].m_tree.m_nodes.m_size * 24;
        size += m_world.m_zones.m_data[i].m_tree.m_leaves.m_size * 8;
    }

    printf("Scene Tree Nodes: %d\n", n);
    total += size;
    printf("Scene Tree Nodes Size: %d\n", size);

    size = 0;
    n = m_lightings.Size();
    for (i = m_lightings.Begin(); i != m_lightings.End(); i = m_lightings.Next(i))
    {
        size += 16;
        size += m_lightings.Value(i)->m_buffers.m_size * 4;
    }

    printf("Static Lighting Objects: %d\n", n);
    total += size;
    printf("Static Lighting Objects Size: %d\n", size);
    printf("Total Scene Allocations: %d\n", total);

    return total;
}

// The actor's bounding box in the editor: eight corner ticks a fifth of the
// box's smallest side long, or (draw style non-zero) the twelve edges.
int VIScene::RasterActorBBox(VICamera &cam, int i)
{
    VIColor32F c;
    VIVect3    n;
    VIBBox     bb;

    bb = m_sprites.Value(((VIActor *)i)->m_p60)->m_bbox;

    m_raster->SetSolidFillUIMaterial();
    c.Set(1.0f, 1.0f, 1.0f, 1.0f);
    n.Set(0.0f, 0.0f, 0.0f);

    if (m_bboxDrawStyle == VISCENEBBOX_NONE)
    {
        VIVect3 a;
        VIVect3 b;
        VIVect3 d = bb.max;

        d -= bb.min;
        float   len = VIMIN(d.z, VIMIN(d.y, d.x));

        len *= 0.4f;

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        a.Set(bb.min.x, bb.min.y, bb.min.z);
        b = a;
        b.x += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.y += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.z += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        m_raster->EndLines();

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        a.Set(bb.max.x, bb.min.y, bb.min.z);
        b = a;
        b.x -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.y += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.z += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        m_raster->EndLines();

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        a.Set(bb.min.x, bb.max.y, bb.min.z);
        b = a;
        b.x += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.y -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.z += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        m_raster->EndLines();

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        a.Set(bb.min.x, bb.min.y, bb.max.z);
        b = a;
        b.x += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.y += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.z -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        m_raster->EndLines();

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        a.Set(bb.max.x, bb.max.y, bb.max.z);
        b = a;
        b.x -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.y -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.z -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        m_raster->EndLines();

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        a.Set(bb.min.x, bb.max.y, bb.max.z);
        b = a;
        b.x += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.y -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.z -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        m_raster->EndLines();

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        a.Set(bb.max.x, bb.min.y, bb.max.z);
        b = a;
        b.x -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.y += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.z -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        m_raster->EndLines();

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        a.Set(bb.max.x, bb.max.y, bb.min.z);
        b = a;
        b.x -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.y -= len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        b = a;
        b.z += len;
        m_raster->Vertex(a);
        m_raster->Vertex(b);
        m_raster->EndLines();
    }
    else
    {
        VIVect3 v[8];

        bb.Store(v);

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        m_raster->Vertex(v[0]);
        m_raster->Vertex(v[1]);
        m_raster->Vertex(v[1]);
        m_raster->Vertex(v[2]);
        m_raster->Vertex(v[2]);
        m_raster->Vertex(v[3]);
        m_raster->Vertex(v[3]);
        m_raster->Vertex(v[0]);
        m_raster->EndLines();

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        m_raster->Vertex(v[0]);
        m_raster->Vertex(v[4]);
        m_raster->Vertex(v[1]);
        m_raster->Vertex(v[5]);
        m_raster->Vertex(v[2]);
        m_raster->Vertex(v[6]);
        m_raster->Vertex(v[3]);
        m_raster->Vertex(v[7]);
        m_raster->EndLines();

        m_raster->BeginLines();
        m_raster->Color(c);
        m_raster->Normal(n);
        m_raster->Vertex(v[4]);
        m_raster->Vertex(v[5]);
        m_raster->Vertex(v[5]);
        m_raster->Vertex(v[6]);
        m_raster->Vertex(v[6]);
        m_raster->Vertex(v[7]);
        m_raster->Vertex(v[7]);
        m_raster->Vertex(v[4]);
        m_raster->EndLines();
    }

    return 0;
}

// -------------------------------------------------------------- shadows --

// A character's blob shadow.  Six nodes of the character's skeleton -- two of
// them pulled halfway to the root -- are taken into the actor's own frame and
// bounded; that box, inflated by 0.2 in x and z, is taken back into the world
// and stretched from 1 below to 0.5 above the actor's feet, and every scene
// triangle facing up inside it is drawn again with the shadow material, the
// box mapped onto UV and the alpha fading over one unit of height.
int VIScene::RenderShadow(int actor)
{
    VIMatrix44    toWorld;
    VIMatrix44    toLocal;
    VIMatrix44    node;
    VIVect3       v;
    VIActorFilter filt;
    int           nodes[6] = { 5, 6, 4, 7, 0, 1 };

    if (m_p1880 == -1)
        return -1;

    VIVect3    pts[6];
    VIBBox     bb;
    VICSprite *sprite = (VICSprite *)GetActorSpritePtr(actor);

    if (!sprite)
        return -1;

    GetActorTransform(actor, toWorld);
    toLocal.Invert(toWorld);

    for (int k = 0; k < 6; k++)
    {
        sprite->CalcSlotWorldTransform((VICSpriteNodeID)nodes[k], actor, *this, node);
        v.x = node.m[3].x;
        v.y = node.m[3].y;
        v.z = node.m[3].z;
        toLocal.Mul(v, pts[k]);

        if (k >= 4)
            pts[k] *= 0.5f;
    }

    bb.Init(6, pts);

    VIVect3 e;

    e.Set(0.2f, 0.0f, 0.2f);
    bb.min -= e;
    e.Set(0.2f, 0.0f, 0.2f);

    VIBBox box;

    bb.max += e;
    bb.Transform(toWorld, box);

    v = GetActorLocation(actor);
    box.max.y = v.y + 0.5f;
    box.min.y = v.y - 1.0f;
    e.Set(0.0f, 1.0f, 0.0f);

    m_queryTris.Reset(box, e, 80.0f);
    QueryIntersection(box, filt, m_queryTris);

    VIVect2 uv[3];
    VIVect3 p0;
    VIVect3 p1;
    VIVect3 p2;

    m_raster->SetMaterial(m_p1880);
    m_raster->SetZBias(15);

    for (int t = 0; t < m_queryTris.m_numTris; t++)
    {
        p0 = m_queryTris.m_tris[t].m_v0;
        p0.y += 0.04f;
        p1 = m_queryTris.m_tris[t].m_v1;
        p1.y += 0.04f;
        p2 = m_queryTris.m_tris[t].m_v2;
        p2.y += 0.04f;

        uv[0].x = (p0.x * toLocal.m[0].x + p0.y * toLocal.m[1].x
                   + p0.z * toLocal.m[2].x + toLocal.m[3].x - bb.min.x)
                  * (1.0f / (bb.max.x - bb.min.x));
        uv[0].y = (p0.x * toLocal.m[0].z + p0.y * toLocal.m[1].z
                   + p0.z * toLocal.m[2].z + toLocal.m[3].z - bb.min.z)
                  * (1.0f / (bb.max.z - bb.min.z));
        e.x = v.y - p0.y;
        e.x = VIMIN(VIMAX(0.0f, e.x), 1.0f);

        uv[1].x = (p1.x * toLocal.m[0].x + p1.y * toLocal.m[1].x
                   + p1.z * toLocal.m[2].x + toLocal.m[3].x - bb.min.x)
                  * (1.0f / (bb.max.x - bb.min.x));
        uv[1].y = (p1.x * toLocal.m[0].z + p1.y * toLocal.m[1].z
                   + p1.z * toLocal.m[2].z + toLocal.m[3].z - bb.min.z)
                  * (1.0f / (bb.max.z - bb.min.z));
        e.y = v.y - p1.y;
        e.y = VIMIN(VIMAX(0.0f, e.y), 1.0f);

        uv[2].x = (p2.x * toLocal.m[0].x + p2.y * toLocal.m[1].x
                   + p2.z * toLocal.m[2].x + toLocal.m[3].x - bb.min.x)
                  * (1.0f / (bb.max.x - bb.min.x));
        uv[2].y = (p2.x * toLocal.m[0].z + p2.y * toLocal.m[1].z
                   + p2.z * toLocal.m[2].z + toLocal.m[3].z - bb.min.z)
                  * (1.0f / (bb.max.z - bb.min.z));
        e.z = v.y - p2.y;
        e.z = VIMIN(VIMAX(0.0f, e.z), 1.0f);

        m_raster->BeginTriFan();
        m_raster->Color(VIColor32F(1.0f, 1.0f, 1.0f, (1.0f - e.x) * 0.65f));
        m_raster->UV(uv[0]);
        m_raster->Vertex(p0);
        m_raster->Color(VIColor32F(1.0f, 1.0f, 1.0f, (1.0f - e.y) * 0.65f));
        m_raster->UV(uv[1]);
        m_raster->Vertex(p1);
        m_raster->Color(VIColor32F(1.0f, 1.0f, 1.0f, (1.0f - e.z) * 0.65f));
        m_raster->UV(uv[2]);
        m_raster->Vertex(p2);
        m_raster->EndTriFan();
    }

    m_raster->SetZBias(0);
    return 0;
}

// A colour table entry from a byte colour: each channel over 255.  Inline, and
// the destination is a REFERENCE, so the first store folds into the table's
// base and the rest go through the reference -- which is also why the second
// channel's byte is still known and the last two are re-read.
static inline void SetColorFrom(VIColor32F &d, const VIColor32 &c)
{
    d.r = c.r * (1.0f / 255.0f);
    d.g = c.g * (1.0f / 255.0f);
    d.b = c.b * (1.0f / 255.0f);
    d.a = c.a * (1.0f / 255.0f);
}

extern "C" double fabs(double);
extern "C" float  sinf(float);
extern "C" float  cosf(float);

// The 24-hour table.  Ambient (m_a[0]) and sun (m_c[0]) colours are written
// out per hour; the sky (m_d[0]) starts as the sun colour, the overcast
// ambient (m_a[1]) is the ambient's red at 90%, the overcast sun (m_c[1]) is
// black and the overcast sky (m_d[1]) a grey peaking at noon; the sun turns
// 15 degrees an hour about z.  The clock starts at 10:00.
int VIScene::InitTimeOfDay()
{
    int h;

    {
    VIColor32 c;
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[0].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[1].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[2].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[3].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[4].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[5].m_a[0], c);
    c.Set(85, 85, 85, 0);
    SetColorFrom(m_tod[6].m_a[0], c);
    c.Set(85, 85, 85, 0);
    SetColorFrom(m_tod[7].m_a[0], c);
    c.Set(95, 95, 95, 0);
    SetColorFrom(m_tod[8].m_a[0], c);
    c.Set(100, 100, 100, 0);
    SetColorFrom(m_tod[9].m_a[0], c);
    c.Set(100, 100, 100, 0);
    SetColorFrom(m_tod[10].m_a[0], c);
    c.Set(102, 102, 102, 0);
    SetColorFrom(m_tod[11].m_a[0], c);
    c.Set(102, 102, 102, 0);
    SetColorFrom(m_tod[12].m_a[0], c);
    c.Set(100, 100, 100, 0);
    SetColorFrom(m_tod[13].m_a[0], c);
    c.Set(100, 100, 100, 0);
    SetColorFrom(m_tod[14].m_a[0], c);
    c.Set(100, 100, 100, 0);
    SetColorFrom(m_tod[15].m_a[0], c);
    c.Set(95, 95, 95, 0);
    SetColorFrom(m_tod[16].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[17].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[18].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[19].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[20].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[21].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[22].m_a[0], c);
    c.Set(75, 75, 100, 0);
    SetColorFrom(m_tod[23].m_a[0], c);
    c.Set(70, 0, 190, 0);
    SetColorFrom(m_tod[0].m_c[0], c);
    c.Set(80, 0, 220, 0);
    SetColorFrom(m_tod[1].m_c[0], c);
    c.Set(85, 0, 240, 0);
    SetColorFrom(m_tod[2].m_c[0], c);
    c.Set(90, 0, 255, 0);
    SetColorFrom(m_tod[3].m_c[0], c);
    c.Set(120, 55, 240, 0);
    SetColorFrom(m_tod[4].m_c[0], c);
    c.Set(170, 55, 240, 0);
    SetColorFrom(m_tod[5].m_c[0], c);
    c.Set(255, 175, 55, 0);
    SetColorFrom(m_tod[6].m_c[0], c);
    c.Set(255, 210, 100, 0);
    SetColorFrom(m_tod[7].m_c[0], c);
    c.Set(255, 248, 160, 0);
    SetColorFrom(m_tod[8].m_c[0], c);
    c.Set(255, 250, 200, 0);
    SetColorFrom(m_tod[9].m_c[0], c);
    c.Set(255, 250, 210, 0);
    SetColorFrom(m_tod[10].m_c[0], c);
    c.Set(255, 250, 230, 0);
    SetColorFrom(m_tod[11].m_c[0], c);
    c.Set(255, 250, 240, 0);
    SetColorFrom(m_tod[12].m_c[0], c);
    c.Set(255, 250, 230, 0);
    SetColorFrom(m_tod[13].m_c[0], c);
    c.Set(255, 250, 210, 0);
    SetColorFrom(m_tod[14].m_c[0], c);
    c.Set(255, 250, 200, 0);
    SetColorFrom(m_tod[15].m_c[0], c);
    c.Set(255, 248, 160, 0);
    SetColorFrom(m_tod[16].m_c[0], c);
    c.Set(255, 210, 100, 0);
    SetColorFrom(m_tod[17].m_c[0], c);
    c.Set(255, 175, 55, 0);
    SetColorFrom(m_tod[18].m_c[0], c);
    c.Set(170, 55, 240, 0);
    SetColorFrom(m_tod[19].m_c[0], c);
    c.Set(120, 55, 240, 0);
    SetColorFrom(m_tod[20].m_c[0], c);
    c.Set(90, 0, 255, 0);
    SetColorFrom(m_tod[21].m_c[0], c);
    c.Set(85, 0, 240, 0);
    SetColorFrom(m_tod[22].m_c[0], c);
    c.Set(80, 0, 220, 0);
    SetColorFrom(m_tod[23].m_c[0], c);

    }

    for (h = 0; h < 24; h++)
        m_tod[h].m_d[0] = m_tod[h].m_c[0];

    for (h = 0; h < 24; h++)
    {
        m_tod[h].m_a[1] = m_tod[h].m_a[0];
        m_tod[h].m_a[1] *= 0.9f;
        m_tod[h].m_a[1].g = m_tod[h].m_a[1].b = m_tod[h].m_a[1].r;

        float f = 1.0f - (float)fabs((float)(h - 12) / 12.0f);

        f *= 0.7f;
        f = VIMAX(f, 0.2f);
        m_tod[h].m_c[1] = VIColor32F(0.0f, 0.0f, 0.0f, 0.0f);
        m_tod[h].m_d[1] = VIColor32F(f, f, f, 0.0f);
    }

    for (h = 0; h < 24; h++)
    {
        float a = (float)h * 15.0f * 0.0174532924f;
        float sn = sinf(a);
        float cs = cosf(a);

        m_tod[h].m_b.Set(-sn, cs, 0.0f);
    }

    m_p1FE0 = 10.0f;
    return 0;
}
