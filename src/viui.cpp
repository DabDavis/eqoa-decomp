// ---------------------------------------------------------------------------
// viui.cpp -- SUPPORT 0x004A4B30 .. 0x004A8330.  Phase 42.
//
// The 2D UI drawing device.  See vi.h for the layout and for what the two
// static lookup tables are.
// ---------------------------------------------------------------------------
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);
unsigned int VIHashResourceID_Icon(const char *);
extern "C" int sprintf(char *, const char *, ...);
extern "C" int printf(const char *, ...);
extern int VIMemCardReadFile(const char *name, void *buf, int size);
VIGameOptions *VIGetGameOptions();

VIUI::VIUI()
{
    m_raster = 0;
    m_sound = 0;
    m_dict = 0;
}

VIUI::~VIUI()
{
}

void VIUI::Close()
{
    m_words.Unload();
    m_textures.Unload();
    m_sounds.Unload();
    m_raster = 0;
    m_sound = 0;
    m_dict = 0;
}

int VIUI::Init(VIRaster *r, VISoundDevice *s, VIDictionary *d)
{
    m_raster = r;
    m_sound = s;
    m_dict = d;
    if (ReloadWords() < 0)
        return -1;
    int e = m_textures.Load("data\\textures.txt");
    if (e != 0)
        return e;
    e = m_sounds.Load("data\\UISound.txt");
    if (e == 0)
        m_language = 0;
    return e;
}

void VIUI::SetLanguage(int l)
{
    m_language = l;
}

int VIUI::Begin2D()
{
    return m_raster->Begin2D();
}

int VIUI::End2D()
{
    return m_raster->End2D();
}

int VIUI::FillRectangle(const VIVect2I *p, const VIColor32F *c)
{
    m_raster->SetSolidFillUIMaterial();
    VIVect2I a(p[1].x, p[0].y);
    VIVect2I b(p[0].x, p[1].y);
    m_raster->BeginTriFan();
    m_raster->Color(*c);
    m_raster->Vertex(p[0]);
    m_raster->Vertex(a);
    m_raster->Vertex(p[1]);
    m_raster->Vertex(b);
    return m_raster->EndTriFan();
}

int VIUI::FrameFillRectangle(const VIVect2I *p)
{
    VIVect2I r[2];
    r[0] = p[0];
    r[1] = p[1];
    VIColor32F c1(1.0f, 1.0f, 1.0f, 1.0f);
    VIColor32F c2(0.0f, 0.0f, 0.0f, 1.0f);
    FillRectangle(r, &c1);
    r[0] += VIVect2I(2, 2);
    r[1] -= VIVect2I(2, 2);
    return FillRectangle(r, &c2);
}

// Phase 61.  Each strip's moved edge is computed from the COPY (`r[1].x =
// r[0].x + w`), so cse reuses the value the copy just stored; written from
// `p[...]` it reloads through a different address form (21 -> 8 words; the
// rest is the second strip's store order).
int VIUI::FrameRectangle(const VIVect2I *p, const VIColor32F *c, int w)
{
    VIVect2I r[2];
    r[0] = p[0];
    r[1] = p[1];
    r[1].x = r[0].x + w;
    FillRectangle(r, c);
    r[0] = p[0];
    r[1] = p[1];
    r[1].y = r[0].y + w;
    FillRectangle(r, c);
    r[0] = p[0];
    r[1] = p[1];
    r[0].x = r[1].x - w;
    FillRectangle(r, c);
    r[0] = p[0];
    r[1] = p[1];
    r[0].y = r[1].y - w;
    return FillRectangle(r, c);
}

int VIUI::Blit(int id, VIVect2I at, VIVect2I a, VIVect2I b,
               VIAlphaBlendMode m, float alpha, unsigned int flags)
{
    VIResourceType t;
    int i = -1;
    if (m_dict->Find(id, t, i) == 0)
        return -1;
    m_raster->Blit(i, flags, at, b, a, m, alpha);
    return 0;
}

int VIUI::Blit(int id, VIVect2I at, VIAlphaBlendMode m, float alpha,
               unsigned int flags)
{
    VIResourceType t;
    int i = -1;
    if (m_dict->Find(id, t, i) == 0)
        return -1;
    m_raster->Blit(i, flags, at, m, alpha);
    return 0;
}

// Phase 61.  The five atlas blitters build both rectangles with the two-int
// constructor out of the entry's fields: both loads come before both stores
// (and the second pair y-first after scheduling), where the copy constructor
// gives load-store-load-store (3 to 8 words each).
int VIUI::Blit(VIUITexId id, VIVect2I at)
{
    char name[64];
    const VIUITexEntry *e = GetUITexture(id);
    if (e == 0)
        return -1;
    VIVect2I a(e->m_uv0.x, e->m_uv0.y);
    VIVect2I b(e->m_uv1.x, e->m_uv1.y);
    name[0] = 0;
    VIString::str16to8(name, 64,
        (const unsigned short *)m_textures.GetString(e->m_row, m_language));
    return Blit((int)VIHashResourceID_Icon(name), at, b, a,
                e->m_mode, e->m_alpha, e->m_flags);
}

int VIUI::Blit(VIUITexId id, VIVect2I at, VIVect2I size)
{
    char name[64];
    const VIUITexEntry *e = GetUITexture(id);
    if (e == 0)
        return -1;
    VIVect2I b(0, 0);
    VIVect2I a(e->m_uv0.x, e->m_uv0.y);
    int x = size.x;
    if (x == -1)
        x = e->m_uv1.x;
    b.x = x;
    int y = size.y;
    if (y == -1)
        y = e->m_uv1.y;
    b.y = y;
    name[0] = 0;
    VIString::str16to8(name, 64,
        (const unsigned short *)m_textures.GetString(e->m_row, m_language));
    return Blit((int)VIHashResourceID_Icon(name), at, b, a,
                e->m_mode, e->m_alpha, e->m_flags);
}

int VIUI::BlitEx(VIUITexId id, VIVect2I at, VIAlphaBlendMode m, float alpha,
                 unsigned int flags)
{
    char name[64];
    const VIUITexEntry *e = GetUITexture(id);
    if (e == 0)
        return -1;
    VIVect2I a(e->m_uv0.x, e->m_uv0.y);
    VIVect2I b(e->m_uv1.x, e->m_uv1.y);
    name[0] = 0;
    VIString::str16to8(name, 64,
        (const unsigned short *)m_textures.GetString(e->m_row, m_language));
    return Blit((int)VIHashResourceID_Icon(name), at, b, a, m, alpha, flags);
}

int VIUI::BlitEx(VIUITexId id, VIVect2I at, VIVect2I size, VIAlphaBlendMode m,
                 float alpha, unsigned int flags)
{
    char name[64];
    const VIUITexEntry *e = GetUITexture(id);
    if (e == 0)
        return -1;
    VIVect2I b(0, 0);
    VIVect2I a(e->m_uv0.x, e->m_uv0.y);
    int x = size.x;
    if (x == -1)
        x = e->m_uv1.x;
    b.x = x;
    int y = size.y;
    if (y == -1)
        y = e->m_uv1.y;
    b.y = y;
    name[0] = 0;
    VIString::str16to8(name, 64,
        (const unsigned short *)m_textures.GetString(e->m_row, m_language));
    return Blit((int)VIHashResourceID_Icon(name), at, b, a, m, alpha, flags);
}

int VIUI::StretchBlitEx(VIUITexId id, VIVect2I at, VIVect2I size,
                        VIAlphaBlendMode m, float alpha)
{
    char name[64];
    const VIUITexEntry *e = GetUITexture(id);
    if (e == 0)
        return -1;
    VIVect2I a(e->m_uv0.x, e->m_uv0.y);
    VIVect2I b(e->m_uv1.x, e->m_uv1.y);
    name[0] = 0;
    VIString::str16to8(name, 64,
        (const unsigned short *)m_textures.GetString(e->m_row, m_language));
    VIResourceType t;
    int i = -1;
    if (m_dict->Find(VIHashResourceID_Icon(name), t, i) == 0)
        return -1;
    return m_raster->StretchBlit(i, 0, at, size, a, b, m, alpha);
}

// Phase 61.  Four edges tiled between four corners.  `br` is `size + at` --
// the operand order survives into the two addu's -- and the top and left
// edges start at `tl + VIVect2I(dx, 0)`, an operator+ whose operand gets its
// own stack temp beside the result.
int VIUI::BlitFrame(VIVect2I at, VIVect2I size, VIUIFrameParam fp,
                    VIAlphaBlendMode m, float alpha, unsigned char left)
{
    VIVect2I tl;
    VIVect2I br;
    VIVect2I u;
    VIVect2I v;
    int w = size.x;
    if (w <= 0 || size.y <= 0)
        return 0;
    tl = at;
    br = size + at;
    const VIUITexEntry *e0 = GetUITexture(fp.m_tl.m_id);
    const VIUITexEntry *e1 = GetUITexture(fp.m_tr.m_id);
    const VIUITexEntry *e2 = GetUITexture(fp.m_br.m_id);
    const VIUITexEntry *e3 = GetUITexture(fp.m_bl.m_id);
    if (e0 == 0 || e1 == 0 || e2 == 0 || e3 == 0)
        return -1;

    BlitHorz(tl + VIVect2I(left ? e0->m_uv1.x : 0, 0), fp.m_top.m_id,
             left ? w - e0->m_uv1.x - e1->m_uv1.x : w - e1->m_uv1.x,
             m, alpha, fp.m_top.m_flags);
    BlitHorz(VIVect2I(tl.x + (left ? e3->m_uv1.x : 0), br.y - e3->m_uv1.y),
             fp.m_bottom.m_id,
             left ? size.x - e3->m_uv1.x - e2->m_uv1.x : size.x - e2->m_uv1.x,
             m, alpha, fp.m_bottom.m_flags);
    if (left)
        BlitVert(tl + VIVect2I(0, e0->m_uv1.y), fp.m_left.m_id,
                 size.y - e0->m_uv1.y - e3->m_uv1.y, m, alpha,
                 fp.m_left.m_flags);
    BlitVert(VIVect2I(br.x - e1->m_uv1.x, tl.y + e1->m_uv1.y), fp.m_right.m_id,
             size.y - e1->m_uv1.y - e2->m_uv1.y, m, alpha, fp.m_right.m_flags);

    if (left)
        BlitEx(fp.m_tl.m_id, tl, m, alpha, fp.m_tl.m_flags);
    BlitEx(fp.m_tr.m_id, VIVect2I(br.x - e1->m_uv1.x, tl.y), m, alpha,
           fp.m_tr.m_flags);
    if (left)
        BlitEx(fp.m_bl.m_id, VIVect2I(tl.x, br.y - e3->m_uv1.y), m, alpha,
               fp.m_bl.m_flags);
    BlitEx(fp.m_br.m_id, VIVect2I(br.x - e2->m_uv1.x, br.y - e2->m_uv1.y), m,
           alpha, fp.m_br.m_flags);
    return 0;
}

// Phase 61.  The plain frame, then rows of studs evenly spaced along each
// edge: nx along the top and bottom, ny down the sides.
int VIUI::BlitStuddedFrame(VIVect2I at, VIVect2I size, VIUIFrameParam fp,
                           VIUIFrameParam studs, int nx, int ny,
                           VIAlphaBlendMode m, float alpha, unsigned char left)
{
    VIVect2I p;
    const VIUITexEntry *e;
    int step;
    int i;

    BlitFrame(at, size, fp, m, alpha, left);

    p = at;
    e = GetUITexture(studs.m_top.m_id);
    if (e == 0)
        return -1;
    step = size.x / (nx + 1) - e->m_uv1.x / 2;
    for (i = nx; i > 0; i--)
    {
        p.x = p.x + step;
        BlitEx(studs.m_top.m_id, p, m, alpha, studs.m_top.m_flags);
    }

    p = at;
    e = GetUITexture(studs.m_bottom.m_id);
    if (e == 0)
        return -1;
    p.y = p.y + (size.y - e->m_uv1.y);
    step = size.x / (nx + 1) - e->m_uv1.x / 2;
    for (i = nx; i > 0; i--)
    {
        p.x = p.x + step;
        BlitEx(studs.m_bottom.m_id, p, m, alpha, studs.m_bottom.m_flags);
    }

    if (left)
    {
        p = at;
        e = GetUITexture(studs.m_left.m_id);
        if (e == 0)
            return -1;
        step = size.y / (ny + 1) - e->m_uv1.y / 2;
        for (i = ny; i > 0; i--)
        {
            p.y = p.y + step;
            BlitEx(studs.m_left.m_id, p, m, alpha, studs.m_left.m_flags);
        }
    }

    p = at;
    e = GetUITexture(studs.m_right.m_id);
    if (e == 0)
        return -1;
    p.x = p.x + (size.x - e->m_uv1.x);
    step = size.y / (ny + 1) - e->m_uv1.y / 2;
    for (i = ny; i > 0; i--)
    {
        p.y = p.y + step;
        BlitEx(studs.m_right.m_id, p, m, alpha, studs.m_right.m_flags);
    }
    return 0;
}

// Phase 61.  The fill is tiled inside the corner insets, then the frame goes
// over it.
int VIUI::BlitFillFrame(VIVect2I at, VIVect2I size, VIUIFrameParam fp,
                        VIUITexId fill, VIAlphaBlendMode m, float alpha,
                        unsigned char left)
{
    const VIUITexEntry *e0 = GetUITexture(fp.m_tl.m_id);
    const VIUITexEntry *e1 = GetUITexture(fp.m_br.m_id);
    const VIUITexEntry *e2 = GetUITexture(fill);
    if (e0 == 0 || e1 == 0 || e2 == 0)
        return -1;
    VIVect2I p;
    VIVect2I sz;
    int w = size.x;
    int h = size.y;
    p.Set(at.x + e0->m_uv1.x, at.y + e0->m_uv1.y);
    sz.Set(w - e0->m_uv1.x - e1->m_uv1.x, h - e0->m_uv1.y - e1->m_uv1.y);
    BlitRect(fill, p, sz, m, alpha);
    return BlitFrame(at, VIVect2I(w, h), fp, m, alpha, left);
}

int VIUI::BlitFillStuddedFrame(VIVect2I at, VIVect2I size, VIUIFrameParam fp,
                               VIUIFrameParam studs, VIUITexId fill, int nx,
                               int ny, VIAlphaBlendMode m, float alpha,
                               unsigned char left)
{
    const VIUITexEntry *e0 = GetUITexture(fp.m_tl.m_id);
    const VIUITexEntry *e1 = GetUITexture(fp.m_br.m_id);
    const VIUITexEntry *e2 = GetUITexture(fill);
    if (e0 == 0 || e1 == 0 || e2 == 0)
        return -1;
    VIVect2I p;
    VIVect2I sz;
    int w = size.x;
    int h = size.y;
    p.Set(at.x + e0->m_uv1.x, at.y + e0->m_uv1.y);
    sz.Set(w - e0->m_uv1.x - e1->m_uv1.x, h - e0->m_uv1.y - e1->m_uv1.y);
    BlitRect(fill, p, sz, m, alpha);
    return BlitStuddedFrame(at, VIVect2I(w, h), fp, studs, nx, ny, m, alpha,
                            left);
}

int VIUI::BlitHorz(VIVect2I at, VIUITexId id, int w, VIAlphaBlendMode m,
                   float alpha, unsigned int flags)
{
    if (w <= 0)
        return 0;
    VIVect2I p = at;
    const VIUITexEntry *e = GetUITexture(id);
    if (e == 0)
        return -1;
    if (e->m_row == 1)
    {
        while (p.x + e->m_uv1.x < at.x + w)
        {
            BlitEx(id, p, m, alpha, flags);
            p.x = p.x + e->m_uv1.x;
        }
        BlitEx(id, p, VIVect2I(at.x + w - p.x, e->m_uv1.y), m, alpha, flags);
    }
    else
    {
        VIVect2I sz(w, e->m_uv1.y);
        BlitEx(id, p, sz, m, alpha, flags);
    }
    return 0;
}

int VIUI::BlitVert(VIVect2I at, VIUITexId id, int h, VIAlphaBlendMode m,
                   float alpha, unsigned int flags)
{
    if (h <= 0)
        return 0;
    VIVect2I p = at;
    const VIUITexEntry *e = GetUITexture(id);
    if (e == 0)
        return -1;
    if (e->m_row == 1)
    {
        while (p.y + e->m_uv1.y < at.y + h)
        {
            BlitEx(id, p, m, alpha, flags);
            p.y = p.y + e->m_uv1.y;
        }
        BlitEx(id, p, VIVect2I(e->m_uv1.x, at.y + h - p.y), m, alpha, flags);
    }
    else
    {
        VIVect2I sz(e->m_uv1.x, h);
        BlitEx(id, p, sz, m, alpha, flags);
    }
    return 0;
}

int VIUI::BlitRect(VIUITexId id, VIVect2I at, VIVect2I size,
                   VIAlphaBlendMode m, float alpha)
{
    VIVect2I d;
    const VIUITexEntry *e = GetUITexture(id);
    if (e == 0)
        return -1;
    VIVect2I p = at;
    if (e->m_row == 1)
    {
        while (p.x + e->m_uv1.x < at.x + size.x)
        {
            while (p.y + e->m_uv1.y < at.y + size.y)
            {
                BlitEx(id, p, m, alpha, 0);
                p.y = p.y + e->m_uv1.y;
            }
            p.x = p.x + e->m_uv1.x;
            d.y = at.y + size.y - p.y;
            p.y = at.y;
        }
        d.x = at.x + size.x - p.x;
        while (p.y + e->m_uv1.y < at.y + size.y)
        {
            BlitEx(id, p, VIVect2I(d.x, e->m_uv1.y), m, alpha, 0);
            p.y = p.y + e->m_uv1.y;
        }
        p.x = at.x;
        while (p.x + e->m_uv1.x < at.x + size.x)
        {
            BlitEx(id, p, VIVect2I(e->m_uv1.x, d.y), m, alpha, 0);
            p.x = p.x + e->m_uv1.x;
        }
        BlitEx(id, p, d, m, alpha, 0);
    }
    else
    {
        BlitEx((VIUITexId)e->m_x00, at, size, m, alpha, 0);
    }
    return 0;
}

// Phase 61.  The end cap is drawn twice, the second time with blit flag 1
// (mirrored), so one texture serves both ends.  Both text-position vectors are
// set through VIVect2I::Set: the stores go through `this`, cse forgets them,
// and the offset is read back in both arms (33 wrong words to 0).
int VIUI::BlitBar(const unsigned short *s, VIColor32F c, VIVect2I at,
                  VIUITexId end, VIUITexId mid, int w, VIAlphaBlendMode m,
                  float alpha, unsigned int just, int shadow)
{
    VIVect2I off;
    VIVect2I tp;
    VIVect2I p = at;
    const VIUITexEntry *e1 = GetUITexture(end);
    const VIUITexEntry *e2 = GetUITexture(mid);
    if (e1 == 0 || e2 == 0)
        return -1;
    BlitEx(end, p, m, alpha, 0);
    p.x = p.x + e1->m_uv1.x;
    if (e2->m_row == 1)
    {
        BlitHorz(p, mid, w - e1->m_uv1.x * 2, m, alpha, 0);
    }
    else
    {
        VIVect2I sz(w - e1->m_uv1.x * 2, e2->m_uv1.y);
        BlitEx(mid, p, sz, m, alpha, 0);
    }
    p.x = at.x + w - e1->m_uv1.x;
    BlitEx(end, p, m, alpha, 1);
    off.Set(0, 3);
    if (just == 1)
        tp.Set(at.x + e1->m_uv1.x - off.x, at.y + e2->m_uv1.y / 2 - off.y);
    else
        tp.Set(at.x + w / 2 - off.x, at.y + e2->m_uv1.y / 2 - off.y);
    DrawCenteredStringEx(s, tp.x, tp.y, c, just, shadow);
    return 0;
}

int VIUI::BlitBar(const char *s, VIColor32F c, VIVect2I at,
                  VIUITexId end, VIUITexId mid, int w, VIAlphaBlendMode m,
                  float alpha, unsigned int just, int shadow)
{
    unsigned short t[256];
    VIString::str8to16(t, 512, s);
    return BlitBar(t, c, at, end, mid, w, m, alpha, just, shadow);
}

int VIUI::BlitBar(unsigned short id, VIColor32F c, VIVect2I at,
                  VIUITexId end, VIUITexId mid, int w, VIAlphaBlendMode m,
                  float alpha, unsigned int just, int shadow)
{
    return BlitBar((const unsigned short *)m_words.GetString(id, m_language),
                   c, at, end, mid, w, m, alpha, just, shadow);
}

int VIUI::BlitBar(const unsigned short *s, VIColor32F c, VIVect2I at,
                  VIUITexId left, VIUITexId right, VIUITexId mid, int w,
                  VIAlphaBlendMode m, float alpha, unsigned int just, int shadow)
{
    VIVect2I off;
    VIVect2I tp;
    VIVect2I p = at;
    const VIUITexEntry *e1 = GetUITexture(left);
    const VIUITexEntry *e2 = GetUITexture(right);
    const VIUITexEntry *e3 = GetUITexture(mid);
    if (e1 == 0 || e2 == 0 || e3 == 0)
        return -1;
    BlitEx(left, p, m, alpha, 0);
    p.x = p.x + e1->m_uv1.x;
    if (e3->m_row == 1)
    {
        BlitHorz(p, mid, w - e1->m_uv1.x - e2->m_uv1.x, m, alpha, 0);
    }
    else
    {
        VIVect2I sz(w - e1->m_uv1.x - e2->m_uv1.x, e3->m_uv1.y);
        BlitEx(mid, p, sz, m, alpha, 0);
    }
    p.x = at.x + w - e2->m_uv1.x;
    BlitEx(right, p, m, alpha, 0);
    off.Set(0, 3);
    if (just == 1)
        tp.Set(at.x + e1->m_uv1.x - off.x, at.y + e3->m_uv1.y / 2 - off.y);
    else
        tp.Set(at.x + w / 2 - off.x, at.y + e3->m_uv1.y / 2 - off.y);
    DrawCenteredStringEx(s, tp.x, tp.y, c, just, shadow);
    return 0;
}

int VIUI::BlitBar(const char *s, VIColor32F c, VIVect2I at,
                  VIUITexId left, VIUITexId right, VIUITexId mid, int w,
                  VIAlphaBlendMode m, float alpha, unsigned int just, int shadow)
{
    unsigned short t[256];
    VIString::str8to16(t, 512, s);
    return BlitBar(t, c, at, left, right, mid, w, m, alpha, just, shadow);
}

int VIUI::BlitBar(unsigned short id, VIColor32F c, VIVect2I at,
                  VIUITexId left, VIUITexId right, VIUITexId mid, int w,
                  VIAlphaBlendMode m, float alpha, unsigned int just, int shadow)
{
    return BlitBar((const unsigned short *)m_words.GetString(id, m_language),
                   c, at, left, right, mid, w, m, alpha, just, shadow);
}

int VIUI::BlitVertBar(VIVect2I at, VIUITexId top, VIUITexId bot, VIUITexId mid,
                      int h, VIAlphaBlendMode m, float alpha)
{
    VIVect2I p = at;
    const VIUITexEntry *e1 = GetUITexture(top);
    const VIUITexEntry *e2 = GetUITexture(bot);
    const VIUITexEntry *e3 = GetUITexture(mid);
    if (e1 == 0 || e2 == 0 || e3 == 0)
        return -1;
    BlitEx(top, p, m, alpha, 0);
    p.y = p.y + e1->m_uv1.y;
    if (e3->m_row == 1)
    {
        BlitVert(p, mid, h - e1->m_uv1.y - e2->m_uv1.y, m, alpha, 0);
    }
    else
    {
        VIVect2I sz(e3->m_uv1.x, h - e1->m_uv1.y - e2->m_uv1.y);
        BlitEx((VIUITexId)e3->m_x00, p, sz, m, alpha, 0);
    }
    p.y = at.y + h - e2->m_uv1.y;
    BlitEx(bot, p, m, alpha, 0);
    return 0;
}

int VIUI::BlitStuddedHorz(VIVect2I at, VIUITexId bar, VIUITexId stud, int w,
                          int n, VIAlphaBlendMode m, float alpha,
                          unsigned int flags)
{
    VIVect2I p = at;
    if (w <= 0)
        return 0;
    BlitHorz(p, bar, w, m, alpha, flags);
    const VIUITexEntry *e = GetUITexture(stud);
    if (e == 0)
        return -1;
    int step = w / (n + 1) - e->m_uv1.x / 2;
    if (n <= 0)
        return 0;
    int i = n;
    do
    {
        p.x = p.x + step;
        BlitEx(stud, p, m, alpha, flags);
        i--;
    } while (i);
    return 0;
}

int VIUI::BlitAbilityIcon(VIVect2I at, int idx, VIAlphaBlendMode m, float alpha)
{
    VIVect2I p = at;
    const VIUITexEntry *l = GetUITexture((VIUITexId)77);
    if (l == 0)
        return -1;
    BlitEx((VIUITexId)77, p, m, alpha, 0);
    VIUITexId id = (VIUITexId)79;
    if (idx == 0)
        id = (VIUITexId)80;
    const VIUITexEntry *e = GetUITexture(id);
    if (e == 0)
        return -1;
    p.x = p.x + l->m_uv1.x;
    BlitEx(id, p, m, alpha, 0);
    p.x = p.x + e->m_uv1.x;
    BlitEx((VIUITexId)78, p, m, alpha, 0);
    return 0;
}

int VIUI::DrawStringEx(const char *s, int x, int y, VIColor32F c, int shadow)
{
    VIColor32F save = m_font.GetTextColor();
    if (shadow)
    {
        VIColor32F sh(0.0f, 0.0f, 0.0f, c.a);
        m_font.SetTextColor(sh);
        m_font.DrawString(m_raster, m_dict, s, x + 2, y + 2);
    }
    m_font.SetTextColor(c);
    m_font.DrawString(m_raster, m_dict, s, x, y);
    m_font.SetTextColor(save);
    return 0;
}

int VIUI::DrawStringEx(const unsigned short *s, int x, int y, VIColor32F c,
                       int shadow)
{
    VIColor32F save = m_font.GetTextColor();
    if (shadow)
    {
        VIColor32F sh(0.0f, 0.0f, 0.0f, c.a);
        m_font.SetTextColor(sh);
        m_font.DrawString(m_raster, m_dict, s, x + 2, y + 2);
    }
    m_font.SetTextColor(c);
    m_font.DrawString(m_raster, m_dict, s, x, y);
    m_font.SetTextColor(save);
    return 0;
}

int VIUI::DrawStringEx(unsigned short id, int x, int y, VIColor32F c,
                       int shadow)
{
    VIColor32F save = m_font.GetTextColor();
    if (shadow)
    {
        VIColor32F sh(0.0f, 0.0f, 0.0f, c.a);
        int sx = x + 2;
        int sy = y + 2;
        m_font.SetTextColor(sh);
        m_font.DrawString(m_raster, m_dict,
            (const unsigned short *)m_words.GetString(id, m_language), sx, sy);
    }
    m_font.SetTextColor(c);
    m_font.DrawString(m_raster, m_dict,
        (const unsigned short *)m_words.GetString(id, m_language), x, y);
    m_font.SetTextColor(save);
    return 0;
}

int VIUI::DrawCenteredStringEx(const char *s, int x, int y, VIColor32F c,
                               unsigned int w, int shadow)
{
    VIColor32F save = m_font.GetTextColor();
    if (shadow)
    {
        VIColor32F sh(0.0f, 0.0f, 0.0f, c.a);
        m_font.SetTextColor(sh);
        m_font.DrawCenteredString(m_raster, m_dict, s, x + 2, y + 2, w);
    }
    m_font.SetTextColor(c);
    m_font.DrawCenteredString(m_raster, m_dict, s, x, y, w);
    m_font.SetTextColor(save);
    return 0;
}

int VIUI::DrawCenteredStringEx(const unsigned short *s, int x, int y,
                               VIColor32F c, unsigned int w, int shadow)
{
    VIColor32F save = m_font.GetTextColor();
    if (shadow)
    {
        VIColor32F sh(0.0f, 0.0f, 0.0f, c.a);
        m_font.SetTextColor(sh);
        m_font.DrawCenteredString(m_raster, m_dict, s, x + 2, y + 2, w);
    }
    m_font.SetTextColor(c);
    m_font.DrawCenteredString(m_raster, m_dict, s, x, y, w);
    m_font.SetTextColor(save);
    return 0;
}

int VIUI::DrawCenteredStringEx(unsigned short id, int x, int y, VIColor32F c,
                               unsigned int w, int shadow)
{
    VIColor32F save = m_font.GetTextColor();
    if (shadow)
    {
        VIColor32F sh(0.0f, 0.0f, 0.0f, c.a);
        int sx = x + 2;
        int sy = y + 2;
        m_font.SetTextColor(sh);
        m_font.DrawCenteredString(m_raster, m_dict,
            (const unsigned short *)m_words.GetString(id, m_language),
            sx, sy, w);
    }
    m_font.SetTextColor(c);
    m_font.DrawCenteredString(m_raster, m_dict,
        (const unsigned short *)m_words.GetString(id, m_language), x, y, w);
    m_font.SetTextColor(save);
    return 0;
}

// Phase 61.  The success test goes in the `if` -- `if (e != 0) return ...;
// return -1;` -- which puts the call out of line behind a `bnezl`; the early
// `return -1` form lays it out in line (5 words).
int VIUI::SoundPlay(VIUISoundId id)
{
    char name[64];
    VIString::str16to8(name, 64,
        (const unsigned short *)m_sounds.GetString(id, 0));
    const VIUISoundEntry *e = GetUISound(id);
    if (e != 0)
        return SoundPlay(name, e->m_volume);
    return -1;
}

int VIUI::SoundPlay(const char *name, float vol)
{
    VIResourceType t;
    int i;
    if (m_dict->Find(VIHashResourceID(name), t, i) != 0)
    {
        int h = m_sound->Play(i, 127, VISOUNDCLASS_SFX);
        if (h >= 0)
            m_sound->SetVolume(h, vol);
    }
    return 0;
}

// Phase 61.  WORDS.TXT for the current language, in order of preference: the
// WORDS_<lang>.TXT entry of WORDS.WAD on the memory card, the same entry of
// data2\words.wad on the disc, WORDS.TXT on the card, data\words.txt on the
// disc.  The card's WAD goes through a 256 KB scratch buffer.

int VIUI::ReloadWords()
{
    char name[32];
    char *data = 0;
    int size = 0;

    m_words.Unload();
    m_wordsFromCard = 0;
    sprintf(name, "WORDS_%s.TXT",
            VILanguage::GetName((VILanguageID)(char)VIGetGameOptions()->m_region));

    char *buf = new char[0x40000];
    int n = VIMemCardReadFile("WORDS.WAD", buf, 0x40000);

    if (n > 0)
    {
        VIWadFile wad;

        if (wad.Init(buf, n) >= 0)
        {
            size = wad.GetSize(name);
            if (size > 0)
            {
                data = new char[size];
                wad.Load(name, data, size);
                wad.Free();
            }
        }
    }
    delete [] buf;

    if (data == 0)
    {
        VIWadFile wad;

        if (wad.Init("data2\\words.wad") >= 0)
        {
            size = wad.GetSize(name);
            if (size > 0)
            {
                data = new char[size];
                wad.Load(name, data, size);
                wad.Free();
            }
        }
    }

    if (data)
    {
        n = m_words.LoadFromMemory(data, size);
        delete [] data;
        if (n == 0)
            return 0;
    }

    if (m_words.LoadFromMemCard("WORDS.TXT") == 0)
    {
        m_wordsFromCard = 1;
        printf("WORDS.TXT read from memory card.\n");
        return 0;
    }
    if (m_words.Load("data\\words.txt") < 0)
        return -1;
    printf("WORDS.TXT read from disc.\n");
    return 0;
}


const VIUISoundEntry *GetUISound(VIUISoundId id)
{
    static VIUISoundEntry sSounds[17];
    if ((unsigned int)id < 17)
        return &sSounds[id];
    return &sSounds[0];
}
