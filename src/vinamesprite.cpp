// VINameSprite -- SUPPORT 0x00453AB8 .. 0x00454E38.  The name plate, and the
// only sprite here that does not draw when asked: ::Raster ENQUEUES into a
// 24-entry file-scope table and ::RasterDeferred, a STATIC called once at the
// end of the frame, sorts that table by view depth, drops every plate whose
// screen rectangle overlaps one already kept, and draws the survivors.
#include "vi.h"

extern "C" void  sceVu0CopyMatrix(VIMatrix44 &d, const VIMatrix44 &s);
extern "C" char *strncpy(char *d, const char *s, unsigned int n);
extern "C" char *strcpy(char *d, const char *s);
extern "C" void  qsort(void *base, unsigned int n, unsigned int size,
                       int (*cmp)(const void *, const void *));

int         VINameSprite::NumNameDefers = 0;
VINameDefer VINameSprite::NameDefers[24];

// Ascending by the queued depth.  Three-way, and the equal case is tested
// rather than folded into the greater one.
int VINameDeferSort(const void *a, const void *b)
{
    if (((const VINameDefer *)a)->m_z < ((const VINameDefer *)b)->m_z)
        return -1;
    if (((const VINameDefer *)a)->m_z == ((const VINameDefer *)b)->m_z)
        return 0;
    return 1;
}

VINameSprite::VINameSprite()
{
    Clear();
}

VINameSprite::~VINameSprite()
{
    Clear();
}

int VINameSprite::Init(const char *name, int font, const VIColor32F &c, VIRaster *r)
{
    strncpy(m_name, name, 31);
    m_name[31] = 0;
    m_color = c;
    m_font = font;
    if (r)
    {
        if (r->ShareFont(font) < 0)
            return -1;
    }
    return 0;
}

// 22.0f is the off distance a name plate stops being queued at, and it is
// compared against the SQUARED length of the view-space anchor -- so the
// number the player sees is not this one.
int VINameSprite::Clear()
{
    m_offDist = 22.0f;
    m_type = 6;
    m_font = -1;
    m_name[0] = 0;
    m_color.Set(1.0f, 1.0f, 1.0f, 1.0f);
    return 0;
}

int VINameSprite::SetName(const char *name)
{
    strncpy(m_name, name, 31);
    m_name[31] = 0;
    return 0;
}

int VINameSprite::SetColor(const VIColor32F &c)
{
    m_color = c;
    return 0;
}

int VINameSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    VIVect4 p;
    VIVect3 at;
    VIVect2 unused;

    if (m_name[0] == 0)
        return 0;
    if (!(flags & 1))
        return 0;

    p.x = 0.0f;
    p.y = 0.6f;
    p.z = 0.0f;
    p.w = 1.0f;
    p.Set(p * *r.GetModelView());
    if (m_offDist * m_offDist <= p.x * p.x + p.y * p.y + p.z * p.z + p.w * p.w)
        return 0;

    at.Set(0.0f, 0.6f, 0.0f);
    if (NumNameDefers < 24)
    {
        NameDefers[NumNameDefers].m_z = p.z;
        NameDefers[NumNameDefers].m_color = m_color;
        NameDefers[NumNameDefers].m_font = m_font;
        strcpy(NameDefers[NumNameDefers].m_name, m_name);
        sceVu0CopyMatrix(NameDefers[NumNameDefers].m_mat, *r.GetModelView());
        r.CalcCenteredStringScreenRect(m_font, at, m_name,
                                       NameDefers[NumNameDefers].m_rect);
        NumNameDefers++;
    }
    return 0;
}

int VINameSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VINameSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VINameSprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    return 0;
}

int VINameSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int i = sc.CreateSprite((VISpriteType)m_type, n);
    if (i < 0)
        return -1;
    VINameSprite *s = (VINameSprite *)sc.Sprite(i);
    s->m_bbox = m_bbox;
    s->m_bsphere = m_bsphere;
    s->m_type = m_type;
    s->m_offDist = m_offDist;
    s->m_font = m_font;
    VIString::strcpy8(s->m_name, 32, m_name);
    s->m_color = m_color;
    if (r)
    {
        if (r->ShareFont(m_font) < 0)
            return -1;
    }
    return i;
}

int VINameSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                          VISoundDevice *sd, VICollide &c)
{
    if (m_id)
    {
        if (d.Remove(*this) < 0)
            return -1;
    }
    if (r)
    {
        if (r->ReleaseFont(m_font, d) < 0)
            return -1;
    }
    return 0;
}

// The frame's one deferred pass.  Sort back to front, compact in place keeping
// only the plates whose screen rectangle clears every plate already kept, then
// draw what survived under each plate's own queued model-view.
int VINameSprite::RasterDeferred(VIRaster &r)
{
    VIVect3 at;
    int i, j, keep;

    qsort(NameDefers, NumNameDefers, sizeof(VINameDefer), VINameDeferSort);

    keep = 0;
    for (i = 0; i < NumNameDefers; i++)
    {
        for (j = 0; j < keep; j++)
        {
            if (NameDefers[i].m_rect.Intersects(NameDefers[j].m_rect))
                break;
        }
        if (j == keep)
        {
            VINameDefer *d = &NameDefers[keep];
            VINameDefer *s = &NameDefers[i];
            d->m_z = s->m_z;
            d->m_font = s->m_font;
            char *dn = d->m_name;
            const char *sn = s->m_name;
            int k = 32;
            while (k--)
                *dn++ = *sn++;
            d->m_color = s->m_color;
            d->m_rect = s->m_rect;
            sceVu0CopyMatrix(d->m_mat, s->m_mat);
            keep++;
        }
    }

    at.x = 0.0f;
    at.y = 0.6f;
    at.z = 0.0f;
    for (i = 0; i < keep; i++)
    {
        r.SetModelView(NameDefers[i].m_mat);
        r.DrawCenteredString(NameDefers[i].m_font, at, NameDefers[i].m_color,
                             NameDefers[i].m_name);
    }
    NumNameDefers = 0;
    return 0;
}
