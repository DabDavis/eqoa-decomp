// ---------------------------------------------------------------------------
// viui.cpp -- SUPPORT 0x004A4B30 .. 0x004A8330.  Phase 42.
//
// The 2D UI drawing device.  See vi.h for the layout and for what the two
// static lookup tables are.
// ---------------------------------------------------------------------------
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);
extern "C" unsigned int VIHashResourceID_Icon(const char *);

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

int VIUI::FrameRectangle(const VIVect2I *p, const VIColor32F *c, int w)
{
    VIVect2I r[2];
    r[0] = p[0];
    r[1] = p[1];
    r[1].x = p[0].x + w;
    FillRectangle(r, c);
    r[0] = p[0];
    r[1] = p[1];
    r[1].y = p[0].y + w;
    FillRectangle(r, c);
    r[0] = p[0];
    r[1] = p[1];
    r[0].x = p[1].x - w;
    FillRectangle(r, c);
    r[0] = p[0];
    r[1] = p[1];
    r[0].y = p[1].y - w;
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

int VIUI::Blit(VIUITexId id, VIVect2I at)
{
    char name[64];
    const VIUITexEntry *e = GetUITexture(id);
    if (e == 0)
        return -1;
    VIVect2I a = e->m_uv0;
    VIVect2I b = e->m_uv1;
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
    VIVect2I a = e->m_uv0;
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
    VIVect2I a = e->m_uv0;
    VIVect2I b = e->m_uv1;
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
    VIVect2I a = e->m_uv0;
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
    VIVect2I a = e->m_uv0;
    VIVect2I b = e->m_uv1;
    name[0] = 0;
    VIString::str16to8(name, 64,
        (const unsigned short *)m_textures.GetString(e->m_row, m_language));
    VIResourceType t;
    int i = -1;
    if (m_dict->Find(VIHashResourceID_Icon(name), t, i) == 0)
        return -1;
    return m_raster->StretchBlit(i, 0, at, size, a, b, m, alpha);
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
    VIColor32F save = m_font.m_color;
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
    VIColor32F save = m_font.m_color;
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
    VIColor32F save = m_font.m_color;
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
    VIColor32F save = m_font.m_color;
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
    VIColor32F save = m_font.m_color;
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
    VIColor32F save = m_font.m_color;
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

int VIUI::SoundPlay(VIUISoundId id)
{
    char name[64];
    VIString::str16to8(name, 64,
        (const unsigned short *)m_sounds.GetString(id, 0));
    const VIUISoundEntry *e = GetUISound(id);
    if (e == 0)
        return -1;
    return SoundPlay(name, e->m_volume);
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

const VIUISoundEntry *GetUISound(VIUISoundId id)
{
    static VIUISoundEntry sSounds[17];
    if ((unsigned int)id < 17)
        return &sSounds[id];
    return &sSounds[0];
}
