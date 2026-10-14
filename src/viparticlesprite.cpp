// VIParticleSprite -- SUPPORT 0x00454E38 .. 0x004564E0.
#include "vi.h"


VIParticleSprite::VIParticleSprite()
{
    m_type = 10;
    m_defIndex = -1;
    m_unk430 = 0;
}

VIParticleSprite::~VIParticleSprite()
{
}

int VIParticleSprite::Create(int def, VIScene &sc)
{
    VIParticleSystem *sys = sc.ParticleSystem();

    if (sys == 0)
        return -1;

    if (m_emitter.Create(sys, sys->ParticleDefinition(def)) < 0)
        return -1;

    m_bbox = m_emitter.StaticBounds();
    m_bsphere.Init(m_bbox);
    m_defIndex = def;
    sys->ShareParticleDefinition(def);
    return 0;
}

int VIParticleSprite::GetInstType(VIScene &sc) const
{
    return 1;
}

int VIParticleSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VIParticleSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VIParticleSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);

    if (i < 0)
        return -1;

    VIParticleSprite *d = (VIParticleSprite *)sc.Sprite(i);

    d->m_bbox = m_bbox;
    d->m_bsphere = m_bsphere;
    d->m_type = m_type;
    d->m_offDist = m_offDist;
    d->m_unk430 = m_unk430;

    if (m_defIndex != -1)
    {
        d->m_defIndex = m_defIndex;

        VIParticleEmitter *e = &m_emitter;

        d->m_emitter.Create(e->m_system, e->m_definition);
        d->m_emitter.SetLocation(m_emitter.Location());
        d->m_emitter.SetLocation(m_emitter.Location());
        d->m_emitter.SetDirection(m_emitter.Direction());
        d->m_emitter.SetSpace(m_emitter.Space());

        VIParticleSystem *sys = sc.ParticleSystem();

        if (sys != 0)
            sys->ShareParticleDefinition(m_defIndex);
    }

    return i;
}

// The emitter lives in the sprite's frame: the camera's view is inverted by
// hand (transpose, translation rebuilt) and composed with the model-view to
// find where the sprite sits.  A free emitter (m_unk430 == 0) is placed and
// aimed there; a local one is pinned at the origin, aimed down +z and given
// the whole transform as its space.  Past 100 m it is purged rather than
// updated.  The particle marker is drawn as in Pick.
int VIParticleSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    const VIVect3 &cp = cam.m_position;
    VIVect3 p(cp.x, cp.y, cp.z);
    VIVect3 d;
    VIMatrix44 a;
    VIMatrix44 b;
    VIMatrix44 c;

    a = cam.m_view;
    b.Transpose(a);
    {
        float x = -(b.m[0].w * b.m[0].x + b.m[1].w * b.m[1].x + b.m[2].w * b.m[2].x);
        float y = -(b.m[0].w * b.m[0].y + b.m[1].w * b.m[1].y + b.m[2].w * b.m[2].y);
        float z = -(b.m[0].w * b.m[0].z + b.m[1].w * b.m[1].z + b.m[2].w * b.m[2].z);
        b.m[0].w = b.m[1].w = b.m[2].w = 0.0f;
        b.m[3].x = x;
        b.m[3].y = y;
        b.m[3].z = z;
    }
    a = *r.GetModelView();
    c.Mul(a, b);
    d.x = c.m[3].x;
    d.y = c.m[3].y;
    d.z = c.m[3].z;
    if (m_unk430 == 0)
        m_emitter.SetLocation(d);
    else
        m_emitter.SetLocation(VIVect3(0.0f, 0.0f, 0.0f));
    if (m_unk430 == 0)
    {
        VIMatrix33 m;

        m.m[0].x = c.m[0].x;
        m.m[0].y = c.m[0].y;
        m.m[0].z = c.m[0].z;
        m.m[1].x = c.m[1].x;
        m.m[1].y = c.m[1].y;
        m.m[1].z = c.m[1].z;
        m.m[2].x = c.m[2].x;
        m.m[2].y = c.m[2].y;
        m.m[2].z = c.m[2].z;
        m_emitter.SetDirection(m);
    }
    else
    {
        m_emitter.SetDirection(VIVect3(0.0f, 0.0f, 1.0f));
        m_emitter.SetSpace(c);
    }
    p -= d;
    if (100.0f < p.Magnitude())
        m_emitter.Purge();
    else
        m_emitter.Update();

    if (sc.GetParticleUISprite() != -1)
    {
        VIMatrix44 m;
        VIVect3 s;

        m = *r.GetModelView();
        s.x = 1.0f / sqrtf(m.m[0].x * m.m[0].x + m.m[0].y * m.m[0].y + m.m[0].z * m.m[0].z);
        s.y = 1.0f / sqrtf(m.m[1].x * m.m[1].x + m.m[1].y * m.m[1].y + m.m[1].z * m.m[1].z);
        s.z = 1.0f / sqrtf(m.m[2].x * m.m[2].x + m.m[2].y * m.m[2].y + m.m[2].z * m.m[2].z);
        if (s.x < 0.25f)
        {
            ((float *)&m)[0] = ((float *)&m)[0] * s.x * 4.0f;
            ((float *)&m)[1] = ((float *)&m)[1] * s.x * 4.0f;
            ((float *)&m)[2] = ((float *)&m)[2] * s.x * 4.0f;
        }
        if (s.y < 0.25f)
        {
            ((float *)&m)[4] = ((float *)&m)[4] * s.y * 4.0f;
            ((float *)&m)[5] = ((float *)&m)[5] * s.y * 4.0f;
            ((float *)&m)[6] = ((float *)&m)[6] * s.y * 4.0f;
        }
        if (s.z < 0.25f)
        {
            ((float *)&m)[8] = ((float *)&m)[8] * s.z * 4.0f;
            ((float *)&m)[9] = ((float *)&m)[9] * s.z * 4.0f;
            ((float *)&m)[10] = ((float *)&m)[10] * s.z * 4.0f;
        }
        r.SetModelView(m);
        sc.RasterUISprite(cam, sc.GetParticleUISprite(), flags);
    }
    return 0;
}

// A particle sprite picks with the scene's particle marker, drawn in the
// sprite's own frame but never smaller than a quarter of a unit: any axis the
// model-view shrinks below that is scaled back up by four.  The rows are
// scaled through `((float *)&m)[k]`, not `m.m[i].x`: cse skips each `if` body,
// and a struct-member store in a skipped block invalidates ALL memory, so
// `s.z` is reloaded for the third test where the target keeps it in $f5.
int VIParticleSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    if (sc.GetParticleUISprite() != -1)
    {
        VIMatrix44 m;
        VIVect3 s;

        sceVu0CopyMatrix(m, *r.GetModelView());
        s.x = 1.0f / sqrtf(m.m[0].x * m.m[0].x + m.m[0].y * m.m[0].y + m.m[0].z * m.m[0].z);
        s.y = 1.0f / sqrtf(m.m[1].x * m.m[1].x + m.m[1].y * m.m[1].y + m.m[1].z * m.m[1].z);
        s.z = 1.0f / sqrtf(m.m[2].x * m.m[2].x + m.m[2].y * m.m[2].y + m.m[2].z * m.m[2].z);
        if (s.x < 0.25f)
        {
            ((float *)&m)[0] = ((float *)&m)[0] * s.x * 4.0f;
            ((float *)&m)[1] = ((float *)&m)[1] * s.x * 4.0f;
            ((float *)&m)[2] = ((float *)&m)[2] * s.x * 4.0f;
        }
        if (s.y < 0.25f)
        {
            ((float *)&m)[4] = ((float *)&m)[4] * s.y * 4.0f;
            ((float *)&m)[5] = ((float *)&m)[5] * s.y * 4.0f;
            ((float *)&m)[6] = ((float *)&m)[6] * s.y * 4.0f;
        }
        if (s.z < 0.25f)
        {
            ((float *)&m)[8] = ((float *)&m)[8] * s.z * 4.0f;
            ((float *)&m)[9] = ((float *)&m)[9] * s.z * 4.0f;
            ((float *)&m)[10] = ((float *)&m)[10] * s.z * 4.0f;
        }
        r.SetModelView(m);
        return sc.PickSprite(seg, f, sc.GetParticleUISprite());
    }
    return 0;
}

int VIParticleSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                              VISoundDevice *sd, VICollide &c)
{
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }

    VIParticleSystem *sys = sc.ParticleSystem();

    if (sys != 0 && m_defIndex != -1)
        sys->ReleaseParticleDefinition(m_defIndex, d);
    return 0;
}

VIParticleDefinition *VIParticleSprite::ParticleDefinition()
{
    return m_emitter.m_definition;
}

VIParticleEmitter *VIParticleSprite::ParticleEmitter()
{
    return &m_emitter;
}
