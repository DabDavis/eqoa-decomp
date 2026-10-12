// VIEffectVolumeSprite -- SUPPORT 0x0042F798 .. 0x00430A90.  A VIEffectVolume
// in the scene graph.  It keeps a SECOND box, the one the volume was last built
// for, and re-seeds the volume when the sprite's own box has drifted more than a
// metre in any axis.  Bit 1 of the raster flags draws the effect itself, in the
// camera's space: the camera matrix is transposed, its translation rebuilt by
// hand, composed with the model-view and transposed again.
#include "vi.h"

extern "C" void sceVu0CopyMatrix(VIMatrix44 &d, const VIMatrix44 &s);

VIEffectVolumeSprite::VIEffectVolumeSprite()
{
    m_type = 14;
}

VIEffectVolumeSprite::~VIEffectVolumeSprite()
{
}

int VIEffectVolumeSprite::Init(const VIBBox &b)
{
    m_bbox = b;
    m_bsphere.Init(m_bbox);
    m_localBBox = b;
    m_volume.Init(b);
    return 0;
}

int VIEffectVolumeSprite::GetInstType(VIScene &sc) const
{
    return 1;
}

int VIEffectVolumeSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r,
                                 unsigned int flags)
{
    float d[3];
    VIVect3 a = m_bbox.Size();
    VIVect3 b = m_localBBox.Size();

    d[0] = a.x - b.x;
    d[1] = a.y - b.y;
    d[2] = a.z - b.z;
    if (d[0] > 1.0f || d[1] > 1.0f || d[2] > 1.0f ||
        d[0] < -1.0f || d[1] < -1.0f || d[2] < -1.0f)
    {
        m_localBBox = m_bbox;
        m_volume.OnResize(m_localBBox);
    }

    VIMatrix44 mv;

    if (sc.GetStreamAudioUISprite() != -1)
    {
        sceVu0CopyMatrix(mv, *r.GetModelView());
        r.SetModelView(mv);
        sc.RasterUISprite(cam, sc.GetStreamAudioUISprite(), flags);
    }

    if (flags & 2)
    {
        VIMatrix44 t;
        VIMatrix44 q;
        VIMatrix44 w;

        sceVu0CopyMatrix(t, cam.m_view);
        q.Transpose(t);
        {
            float x = -(q.m[0].w * q.m[0].x + q.m[1].w * q.m[1].x + q.m[2].w * q.m[2].x);
            float y = -(q.m[0].w * q.m[0].y + q.m[1].w * q.m[1].y + q.m[2].w * q.m[2].y);
            float z = -(q.m[0].w * q.m[0].z + q.m[1].w * q.m[1].z + q.m[2].w * q.m[2].z);
            q.m[0].w = q.m[1].w = q.m[2].w = 0.0f;
            q.m[3].x = x;
            q.m[3].y = y;
            q.m[3].z = z;
        }
        sceVu0CopyMatrix(t, *r.GetModelView());
        w.Mul(t, q);
        q.Transpose(w);
        {
            float x = -(q.m[0].w * q.m[0].x + q.m[1].w * q.m[1].x + q.m[2].w * q.m[2].x);
            float y = -(q.m[0].w * q.m[0].y + q.m[1].w * q.m[1].y + q.m[2].w * q.m[2].y);
            float z = -(q.m[0].w * q.m[0].z + q.m[1].w * q.m[1].z + q.m[2].w * q.m[2].z);
            q.m[0].w = q.m[1].w = q.m[2].w = 0.0f;
            q.m[3].x = x;
            q.m[3].y = y;
            q.m[3].z = z;
        }
        m_volume.Process(1.0f / 30.0f);
        m_volume.Render(cam, q);
    }
    return 1;
}

int VIEffectVolumeSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc,
                                  VICollide &c)
{
    return 0;
}

int VIEffectVolumeSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc,
                                  VICollide &c)
{
    return 0;
}

// The scene's STREAM AUDIO debug sprite, not one of its own: the effect volume
// picks (and draws) with the marker the streaming-audio volumes use.
int VIEffectVolumeSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc,
                               VIRaster &r)
{
    if (sc.GetStreamAudioUISprite() != -1)
    {
        VIMatrix44 m;
        sceVu0CopyMatrix(m, *r.GetModelView());
        r.SetModelView(m);
        return sc.PickSprite(seg, f, sc.GetStreamAudioUISprite());
    }
    return 0;
}

// m_density is written twice: SetType already carries it, and the second copy
// past SetFlags writes it again.
int VIEffectVolumeSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd,
                               VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VIEffectVolumeSprite *s = (VIEffectVolumeSprite *)sc.Sprite(i);
    s->Init(m_bbox);
    VIEffectVolume *dv = &s->m_volume;
    VIEffectVolume *sv = &m_volume;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
    dv->Init(m_bbox);
    dv->SetType(sv->Type(), sv->Density());
    dv->m_p34 = sv->m_p34;
    dv->m_p38 = sv->m_p38;
    dv->SetFlags(sv->Flags());
    dv->m_p30 = sv->m_p30;
    dv->m_p28 = sv->m_p28;
    dv->AttachSurface(sv->Surface());
    dv->AttachSprite(sv->Sprite());
    return i;
}

int VIEffectVolumeSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                                  VISoundDevice *sd, VICollide &c)
{
    m_volume.Free();
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }
    return 0;
}

VIEffectVolume *VIEffectVolumeSprite::EffectVolume()
{
    return &m_volume;
}
