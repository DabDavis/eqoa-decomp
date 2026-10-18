#line 1 "C:/code-frontiers-gm/game/VIWndNetStat.cpp"
// VIWndNetStat.cpp -- CLIENT 0x00C7EB10 .. 0x00C7FFC8.  The network meter: a texture of
// the last 64 sends and receives, and the ping.
#include "game.h"
VI_HOST_ONLY(extern "C" void *memcpy(void *, const void *, unsigned int);)  // gcc 2.95: implicit
// 0x00C7EB10.
VIWndNetStat::VIWndNetStat()
{
    m_mat = -1;
}

// 0x00C7EB50.
VIWndNetStat::~VIWndNetStat()
{
    if (m_mat != -1)
    {
        VIClient *app = (VIClient *)GetAppPtr();
        ReleaseMat(app->m_raster, app->m_dict, m_mat);
    }
}

// 0x00C7EBB8.  Row 0: eight ticks either side of the middle, `phase` pixels apart from
// the frame's; row 1: a dot per bit of each history; rows 2..6 copy row 1, row 7 row 0.
int VIWndNetStat::UpdateTexture(unsigned long sent, unsigned long recv, int phase)
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIRaster &r = app->m_raster;
    int stride;
    VIPixelStore ps;
    unsigned char *bits;
    int i;
    int res = -1;
    if (m_mat == res)
        return res;
    VISurface *surf = r.Surface(r.Material(m_mat)->GetLayerTexture(0));
    res = surf->LockMipLevel(0, stride, ps, &bits);
    if (res < 0)
        return res;
    memset(bits, 0, stride * 32);
    switch (ps)
    {
    case VIPixelStoreI4:
        for (i = 0; i < 8; i++)
        {
            int x = 64 - phase - i * 8;
            if (x & 1)
                bits[x >> 1] |= 0xF0;
            else
                bits[x >> 1] |= 0x0F;
            int x2 = 128 - x;
            if (x2 & 1)
                bits[x2 >> 1] |= 0xF0;
            else
                bits[x2 >> 1] |= 0x0F;
        }
        for (i = 0; i < 64; i++)
        {
            if (sent & (1L << (64 - i)))
            {
                if (i & 1)
                    bits[stride + (i >> 1)] |= 0xF0;
                else
                    bits[stride + (i >> 1)] |= 0x0F;
            }
            if (recv & (1L << i))
            {
                if (i & 1)
                    bits[stride + ((i + 64) >> 1)] |= 0xF0;
                else
                    bits[stride + ((i + 64) >> 1)] |= 0x0F;
            }
        }
        for (i = 2; i < 7; i++)
            memcpy(bits + i * stride, bits + stride, stride);
        memcpy(bits + 7 * stride, bits, stride);
        break;
    case VIPixelStore6:
    {
        VIColor32 white;
        white.a = 255;
        white.r = 255;
        white.g = 255;
        white.b = 255;
        VIColor32 *p = (VIColor32 *)bits;
        for (i = 0; i < 8; i++)
        {
            int x = 64 - phase - i * 8;
            p[x] = white;
            p[128 - x] = white;
        }
        for (i = 0; i < 64; i++)
        {
            if (sent & (1L << (64 - i)))
            {
                int o = stride + i * 4;
                *(VIColor32 *)(bits + o) = white;
            }
            if (recv & (1L << i))
            {
                int o = stride + (i + 64) * 4;
                *(VIColor32 *)(bits + o) = white;
            }
        }
        for (i = 2; i < 7; i++)
            memcpy(bits + i * stride, bits + stride, stride);
        memcpy(bits + 7 * stride, bits, stride);
        break;
    }
    }
    res = surf->UnlockMipLevel(0);
    if (res >= 0)
        res = 0;
    return res;
}

// 0x00C7EF88.  A white solid-fill material over a 128 x 32 4-bit surface whose palette is
// a grey ramp, 0 transparent.  (The target's flow is a goto chain to one exit; this nested
// form scores closer for now.)
int VIWndNetStat::CreateTexture()
{
    VIClient *app = (VIClient *)GetAppPtr();
    VIColor32 white(255, 255, 255, 255);
    int surface = -1;
    m_mat = -1;
    VIRaster &r = app->m_raster;
    VIDictionary &d = app->m_dict;
    int res;
    VIMaterial *mat;
    VISurface *surf;
    int stride;
    VIPixelStore ps;
    unsigned char *bits;
    int count;
    unsigned char *pal;
    m_mat = r.CreateMaterial(0);
    if (m_mat < 0)
    {
        res = -1;
        goto done;
    }
    mat = r.Material(m_mat);
    mat->InitSolidFill();
    mat->SetLayerColor(0, white);
    surface = r.CreateSurface(0);
    if (surface < 0)
    {
        res = -1;
        goto done;
    }
    surf = r.Surface(surface);
    mat->SetLayerTexture(0, surface, r, d);
    surface = -1;
    res = surf->Init(r, 128, 32, VIBitDepth4, 1);
    if (res < 0)
        goto done;
    res = surf->LockMipLevel(0, stride, ps, &bits);
    if (res < 0)
        goto done;
    res = surf->UnlockMipLevel(0);
    if (res < 0)
        goto done;
    if (ps != VIPixelStoreI4)
        goto ok;
    res = surf->LockPalette(count, ps, &pal);
    if (res < 0)
        goto done;
    for (int i = 0; i < count; i++)
    {
        pal[i * 4 + 0] = pal[i * 4 + 1] = pal[i * 4 + 2] = i * 17;
        pal[i * 4 + 3] = i * 15;
    }
    res = surf->UnlockPalette();
    if (res < 0)
        goto done;
ok:
    res = 0;
done:
    if (surface != -1)
        r.ReleaseSurface(surface, d);
    if (res)
    {
        ReleaseMat(r, d, m_mat);
        m_mat = -1;
    }
    return res;
}

// 0x00C7F1A8.  Histories older than two seconds are shifted on by the whole seconds.
void VIWndNetStat::SetData(long age, unsigned long sent, unsigned long recv,
                           unsigned short number, int ping)
{
    unsigned long r = recv;
    if (age > 200)
    {
        long n = age / 100 - 1;
        number += n;
        sent <<= n;
        r <<= n;
    }
    m_ping = ping;
    UpdateTexture(sent, r, number & 7);
}

// 0x00C7F248.  The meter in the bottom right corner, the ping above it.
void VIWndNetStat::OnDrawSelf()
{
    int right = 624;
    VIClient *app = (VIClient *)GetAppPtr();
    VIVect2I uv(0, 0);
    VIVect2I size(128, 8);
    VIVect2I pos(right - size.x, 424 - size.y);
    VIUI &ui = app->m_ui;
    ui.Begin2D();
    if (m_mat != -1)
        ui.Blit(m_mat, pos, size, uv, (VIAlphaBlendMode)0, 1.0f, 0);
    char s[32];
    sprintf(s, "%i", m_ping);
    pos.x = right - ui.m_font.GetStringWidth(ui.m_raster, ui.m_dict, s);
    pos.y -= ui.m_font.GetHeight(ui.m_raster, ui.m_dict);
    ui.m_font.DrawString(ui.m_raster, ui.m_dict, s, pos.x, pos.y);
    ui.End2D();
}
