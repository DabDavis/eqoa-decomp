// VISoundSprite -- SUPPORT 0x004739C8 .. 0x00474BD0.  A positional sound.  It
// draws (and picks) the scene's sound UI sprite at its own transform, with
// VIPointSprite::ClampScale written out INLINE in both Raster and Pick.
#include "vi.h"

extern "C" void  sceVu0CopyMatrix(VIMatrix44 &d, const VIMatrix44 &s);
extern "C" float sqrtf(float x);

VISoundSprite::VISoundSprite()
{
    m_type = 9;
    m_sound = m_channel = -1;
    m_flags = 0x10000000;
    m_attenuation = ATTENUATION_0;
    m_pan = true;
    m_volume = 1.0f;
}

VISoundSprite::~VISoundSprite()
{
}

int VISoundSprite::GetInstType(VIScene &sc) const
{
    return 1;
}

int VISoundSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    if (sc.GetSoundUISprite() == -1)
        return 0;

    VIMatrix44 m;
    VIVect3 s;

    sceVu0CopyMatrix(m, *r.GetModelView());

    s.x = 1.0f / sqrtf(m.m[0].x * m.m[0].x + m.m[0].y * m.m[0].y + m.m[0].z * m.m[0].z);
    s.y = 1.0f / sqrtf(m.m[1].x * m.m[1].x + m.m[1].y * m.m[1].y + m.m[1].z * m.m[1].z);
    s.z = 1.0f / sqrtf(m.m[2].x * m.m[2].x + m.m[2].y * m.m[2].y + m.m[2].z * m.m[2].z);

    if (s.x < 0.25f)
    {
        m.m[0].x = m.m[0].x * s.x * 4.0f;
        m.m[0].y = m.m[0].y * s.x * 4.0f;
        m.m[0].z = m.m[0].z * s.x * 4.0f;
    }
    if (s.y < 0.25f)
    {
        m.m[1].x = m.m[1].x * s.y * 4.0f;
        m.m[1].y = m.m[1].y * s.y * 4.0f;
        m.m[1].z = m.m[1].z * s.y * 4.0f;
    }
    if (s.z < 0.25f)
    {
        m.m[2].x = m.m[2].x * s.z * 4.0f;
        m.m[2].y = m.m[2].y * s.z * 4.0f;
        m.m[2].z = m.m[2].z * s.z * 4.0f;
    }

    r.SetModelView(m);
    sc.RasterUISprite(cam, sc.GetSoundUISprite(), flags);
    return 0;
}

int VISoundSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VISoundSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VISoundSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    if (sc.GetSoundUISprite() != -1)
    {
    VIMatrix44 m;
    VIVect3 s;

    sceVu0CopyMatrix(m, *r.GetModelView());

    s.x = 1.0f / sqrtf(m.m[0].x * m.m[0].x + m.m[0].y * m.m[0].y + m.m[0].z * m.m[0].z);
    s.y = 1.0f / sqrtf(m.m[1].x * m.m[1].x + m.m[1].y * m.m[1].y + m.m[1].z * m.m[1].z);
    s.z = 1.0f / sqrtf(m.m[2].x * m.m[2].x + m.m[2].y * m.m[2].y + m.m[2].z * m.m[2].z);

    if (s.x < 0.25f)
    {
        m.m[0].x = m.m[0].x * s.x * 4.0f;
        m.m[0].y = m.m[0].y * s.x * 4.0f;
        m.m[0].z = m.m[0].z * s.x * 4.0f;
    }
    if (s.y < 0.25f)
    {
        m.m[1].x = m.m[1].x * s.y * 4.0f;
        m.m[1].y = m.m[1].y * s.y * 4.0f;
        m.m[1].z = m.m[1].z * s.y * 4.0f;
    }
    if (s.z < 0.25f)
    {
        m.m[2].x = m.m[2].x * s.z * 4.0f;
        m.m[2].y = m.m[2].y * s.z * 4.0f;
        m.m[2].z = m.m[2].z * s.z * 4.0f;
    }

    r.SetModelView(m);
    return sc.PickSprite(seg, f, sc.GetSoundUISprite());
    }
    return 0;
}

int VISoundSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VISoundSprite *s = (VISoundSprite *)sc.Sprite(i);
    s->m_bbox = m_bbox;
    s->m_bsphere = m_bsphere;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
    s->m_sound = m_sound;
    s->m_volume = m_volume;
    if (sd)
        sd->ShareSound(m_sound);
    return i;
}

// The two device calls in the middle are made BEFORE `sd` is null-tested, so a
// release with no device and a live channel calls through null.
int VISoundSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                           VISoundDevice *sd, VICollide &c)
{
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }
    if (m_channel != -1)
    {
        if (sd->IsPlaying(m_channel))
        {
            sd->Stop(m_channel);
            m_channel = -1;
        }
    }
    if (sd)
        sd->ReleaseSound(m_sound, d);
    return 0;
}

VISoundSprite::ATTENUATIONMODE VISoundSprite::AttenuationMode() const
{
    return m_attenuation;
}

void VISoundSprite::SetAttenuationMode(ATTENUATIONMODE m)
{
    m_attenuation = m;
}

bool VISoundSprite::IsPanEnabled() const
{
    return m_pan;
}

void VISoundSprite::EnablePan(bool on)
{
    m_pan = on;
}

float VISoundSprite::Volume() const
{
    return m_volume;
}
