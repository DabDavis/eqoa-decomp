// VIRaster -- SUPPORT 0x003FF2E8 .. 0x0040C758, phase 56.  The renderer.
//
// One 19,540-byte object that owns every GS resource the engine has: five
// pools (prim buffers, colour buffers, surfaces, material palettes, fonts), a
// list of materials with an LRU over the resident ones, the DMA double buffer
// every draw call appends to, and the immediate-mode vertex path.
//
// The per-frame uniform state is NOT in the object.  GetModelView returns a
// bare 0x70000000 and every light and fog setter writes an absolute
// scratchpad address, so the matrices, the lights and the fog parameters all
// live in SPR where VU1's unpack can reach them -- VIRasterSPR in vi.h is
// that block, addressed by cast.
//
// The surface cache is a FILE-SCOPE object, not a member: Init hands
// VIEESurfCache::Init a `lui`/`addiu` pair, not `this + N`.
#include "vi.h"

extern VIEESurfCache g_surfCache;

// The DMA/GS floor.  None of these bodies is in this TU and the image is
// un-relocated, so only the ARGUMENT SHAPE is being asserted.
extern "C" int   sceGsResetGraph(int mode, int inter, int omode, int ffmd);
extern "C" int   sceGsSyncV(int mode);
extern "C" int   sceGsSyncPath(int mode, unsigned short timeout);
extern "C" void *sceDmaGetChan(int ch);
extern int   VIEEDMASync(int wait);
extern int   VIEEDMAStart(int ch);
extern int   VIEEDMASend(void *chan, int tag);
extern int   VIEEDMAKick(int ch, _sceDmaTag *tag);
extern "C" void sceVu0CopyMatrix(VIMatrix44 *d, const VIMatrix44 &s);
extern "C" void VIVu0MulMatrix(void *d, const void *a, const void *b);
extern "C" void VIVu0InverseMatrix(void *d, const void *m);
extern "C" void VIVu0ApplyMatrix(void *m, const void *v, void *d);
extern "C" void VIVu0Normalize(void *v);
extern "C" void VIVu0UnitMatrix(void *m);
extern "C" void VIVu0CopyMatrix(void *d, const void *s);
extern int VIEEMicroDump(int a, int b);
extern "C" void *memcpy(void *, const void *, unsigned int);

// The DMAC handler the raster installs on channel 1 (VIF1).  The handler body
// is not in this TU; the id it is registered under is a file-scope int here,
// which is what RemoveDMACHandler tests against -1.
extern "C" int AddDmacHandler(int ch, int (*fn)(int), int next);
extern "C" int RemoveDmacHandler(int ch, int id);
extern "C" int EnableDmac(int ch);
extern "C" int DisableDmac(int ch);
extern "C" int VIEERasterDMACHandler(int ch);

// The billboard microprogram.  Only the DIFFERENCE of the two labels is used
// -- `(VIBbRender - VIBbInit) >> 3` is the MSCALF entry point in VU
// instructions -- so they are declared as arrays of the 8-byte word the
// subtraction scales by.
extern "C" long VIBbInit[];
extern "C" long VIDMARasterMicrocode[];
extern "C" long VIDMABillboardMicrocode[];
extern "C" long VIBbRender[];

int VIEERasterDMACHandlerID;


int VIRaster::RInitOffset;
int VIRaster::RMaterialOffset;
int VIRaster::RStripOffset;
int VIRaster::RStrip2Offset;
int VIRaster::RStrip3Offset;
int VIRaster::RStrip4Offset;
int VIRaster::RStrip5Offset;
int VIRaster::RStrip6Offset;
int VIRaster::RFanOffset;
int VIRaster::ObjectBlendsBuffer;
int VIRaster::PreTransBuffer;
int VIRaster::PrimBuffer1;
int VIRaster::PrimBuffer2;
int VIRaster::ColorMatrix;
long VIRaster::DBuffer[128];

// ---------------------------------------------------------------- resources --

VIPrimBuffer *VIRaster::PrimBuffer(int i)
{
    return m_primBuffers.Value(i);
}

int VIRaster::SharePrimBuffer(int i)
{
    VIPrimBuffer *p = m_primBuffers.Value(i);
    p->m_userData++;
    return 0;
}

VIColorBuffer *VIRaster::ColorBuffer(int i)
{
    return m_colorBuffers.Value(i);
}

int VIRaster::ShareColorBuffer(int i)
{
    VIColorBuffer *p = m_colorBuffers.Value(i);
    p->m_userData++;
    return 0;
}

VISurface *VIRaster::Surface(int i)
{
    return m_surfaces.Value(i);
}

int VIRaster::ShareSurface(int i)
{
    VISurface *p = m_surfaces.Value(i);
    p->m_userData++;
    return 0;
}

VIMaterial *VIRaster::Material(int i)
{
    return (VIMaterial *)i;
}

int VIRaster::ShareMaterial(int i)
{
    VIMaterial *m = (VIMaterial *)i;
    m->m_userData++;
    return 0;
}

int VIRaster::MaterialHasDepthSort(int i)
{
    VIMaterial *m = (VIMaterial *)i;
    return m->m_layers[0].m_blendMode == 0;
}

int VIRaster::MaterialPal(int pal)
{
    return (int)m_materialPals.Value(pal);
}

int VIRaster::ShareMaterialPal(int pal)
{
    VIMaterialPal *p = m_materialPals.Value(pal);
    p->m_refCount++;
    return 0;
}

int VIRaster::GetMaterialPalSize(int pal)
{
    VIMaterialPal *p = m_materialPals.Value(pal);
    return p->m_mats.m_size;
}

VIMaterial *VIRaster::GetMaterialPalEntry(int pal, int i)
{
    VIMaterialPal *p = m_materialPals.Value(pal);
    return Material(p->m_mats.m_data[i]);
}

VIRFont *VIRaster::Font(int i)
{
    return m_fonts.Value(i);
}

int VIRaster::ShareFont(int i)
{
    VIRFont *f = m_fonts.Value(i);
    f->m_userData++;
    return 0;
}

// ------------------------------------------------------------------- fonts --

int VIRaster::GetPointSize(int font)
{
    VIRFont *f = m_fonts.Value(font);
    return f->m_charHeight;
}

int VIRaster::GetWidth(int font, char c)
{
    VIRFont *f = m_fonts.Value(font);
    return f->GetWidth(c) + 1;
}

int VIRaster::GetWidth(int font, unsigned short c)
{
    VIRFont *f = m_fonts.Value(font);
    return f->GetWidth(c) + 1;
}

int VIRaster::GetStringWidth(int font, const char *s)
{
    VIRFont *f = m_fonts.Value(font);
    return f->GetStringWidth(s, 1);
}

int VIRaster::GetStringWidth(int font, const unsigned short *s)
{
    VIRFont *f = m_fonts.Value(font);
    return f->GetStringWidth(s, 1);
}

// ------------------------------------------------------------------- state --

int VIRaster::SetMultiTexture(int on)
{
    m_multiTexture = on;
    return 0;
}

int VIRaster::SetTime(long t)
{
    m_time = t;
    return 0;
}

long VIRaster::GetTime()
{
    return m_time;
}

void VIRaster::EnableTessellation(int on)
{
    m_tessellate = on;
}

int VIRaster::SetWireframe(int on)
{
    m_wireframe = on;
    SetOverrideMaterial();
    return 0;
}

int VIRaster::SetSolidFill(int on)
{
    m_solidFill = on;
    SetOverrideMaterial();
    return 0;
}

const VIColor32F *VIRaster::GetClearColor() const
{
    return &m_clearColor;
}

const VIMatrix44 *VIRaster::GetModelView() const
{
    return &VIRASTER_SPR->m_modelView;
}

const VIMatrix44 *VIRaster::GetProjection() const
{
    return &VIRASTER_SPR->m_projection;
}

const VIColor32F *VIRaster::GetStaticLighting() const
{
    return &VIRASTER_SPR->m_staticLighting;
}

int VIRaster::GetViewport(int &x, int &y, int &w, int &h) const
{
    x = m_viewportX;
    y = m_viewportY;
    w = m_viewportX + m_viewportW - 1;
    h = m_viewportY + m_viewportH - 1;
    return 0;
}

int VIRaster::SetScreenResolution(int w, int h)
{
    if (w != 640 || h != 448)
        return -1;
    return 0;
}

int VIRaster::BeginLines()
{
    return -1;
}

int VIRaster::EndLines()
{
    return -1;
}

int VIRaster::DrawBBox(const VIBBox &b, const VIColor32F &c)
{
    return -1;
}

// ------------------------------------------------------------------ lights --

int VIRaster::SetAmbient(const VIColor32F &c)
{
    VIRASTER_SPR->m_ambient = c;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::EnableAmbient(int on)
{
    VIRASTER_SPR->m_ambientOn = on;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::EnableSunLight(int on)
{
    VIRASTER_SPR->m_sunOn = on;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::EnableDirLight(int i, int on)
{
    ((unsigned char *)0x70000000)[i + 546] = on;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::EnableAllDirLights(int on)
{
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::MaxDirLights()
{
    return 0;
}

int VIRaster::EnablePointLight(int i, int on)
{
    ((unsigned char *)0x70000000)[i + 546] = on;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::MaxPointLights()
{
    return 1;
}

int VIRaster::EnableAllPointLights(int on)
{
    unsigned char *p = (unsigned char *)0x70000222;
    for (int i = 0; i >= 0; i--)
        *p-- = on;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::EnableAllLights(int on)
{
    EnableAmbient(on);
    EnableSunLight(on);
    EnableAllDirLights(on);
    EnableAllPointLights(on);
    return 0;
}

int VIRaster::SetStaticLighting(const VIColor32F &c)
{
    VIRASTER_SPR->m_staticLighting = c;
    return 0;
}

// ------------------------------------------------------------------ matrix --

void VIRaster::Frustum(VIMatrix44 &m, float l, float r, float b, float t,
                       float n, float f)
{
    m.m[3].w = 0.0f;
    m.m[2].w = 1.0f;
    m.m[0].y = 0.0f;
    m.m[0].z = 0.0f;
    m.m[0].w = 0.0f;
    m.m[1].x = 0.0f;
    m.m[1].z = 0.0f;
    m.m[1].w = 0.0f;
    m.m[3].x = 0.0f;
    m.m[3].y = 0.0f;
    m.m[0].x = (n + n) / (r - l);
    m.m[1].y = (n + n) / (b - t);
    m.m[2].x = (r + l) / (r - l);
    m.m[2].y = (b + t) / (b - t);
    m.m[2].z = (f + n) / (f - n);
    m.m[3].z = -2.0f * f * n / (f - n);
}

// ------------------------------------------------------------- lifecycle --

int VIRaster::Close()
{
    VIEEDMASync(1);
    if (Clear() < 0)
        return -1;
    return 0;
}

int VIRaster::Reset()
{
    VIEEDMASync(1);
    if (Clear() < 0)
        return -1;
    if (Init() < 0)
        return -1;
    return 0;
}

// ---------------------------------------------------------------- creates --

int VIRaster::CreatePrimBuffer(int share)
{
    VIPrimBuffer *pb = new VIPrimBuffer;
    if (pb == 0)
        return -1;
    int i = m_primBuffers.Add(pb);
    if (i < 0)
        return -1;
    if (share)
        pb->m_userData++;
    return i;
}

int VIRaster::CreateColorBuffer(int share)
{
    VIColorBuffer *cb = new VIColorBuffer;
    if (cb == 0)
        return -1;
    int i = m_colorBuffers.Add(cb);
    if (i < 0)
        return -1;
    if (share)
        cb->m_userData++;
    return i;
}

int VIRaster::CreateSurface(int share)
{
    VISurface *s = new VISurface;
    if (s == 0)
        return -1;
    int i = m_surfaces.Add(s);
    if (i < 0)
        return -1;
    if (share)
        s->m_userData++;
    s->m_index = i;
    return i;
}

int VIRaster::CreateFont(int share)
{
    VIRFont *f = new VIRFont;
    if (f == 0)
        return -1;
    int i = m_fonts.Add(f);
    if (i < 0)
        return -1;
    if (share)
        f->m_userData++;
    return i;
}

// ------------------------------------------------------------- materials --

int VIRaster::SetSolidFillMaterial()
{
    if (SetMaterial(m_solidFillMaterial) < 0)
        return -1;
    return 0;
}

int VIRaster::SetSolidFillUIMaterial()
{
    if (SetMaterial(m_solidFillUIMaterial) < 0)
        return -1;
    return 0;
}

int VIRaster::SetSolidFillUIAVGMaterial()
{
    if (SetMaterial(m_solidFillUIAVGMaterial) < 0)
        return -1;
    return 0;
}

int VIRaster::SetWireframeMaterial()
{
    if (SetMaterial(m_wireframeMaterial) < 0)
        return -1;
    return 0;
}

int VIRaster::SetPortalMaterial()
{
    if (SetMaterial(m_portalMaterial) < 0)
        return -1;
    return 0;
}

int VIRaster::SetOverrideMaterial()
{
    if (m_wireframe)
    {
        m_override = 0;
        SetMaterial(m_wireframeMaterial);
        m_override = 1;
    }
    else if (m_solidFill)
    {
        m_override = 0;
        SetMaterial(m_solidFillMaterial);
        m_override = 1;
    }
    else
    {
        m_override = 0;
    }
    return 0;
}

int VIRaster::SetMaterial(int i)
{
    if (AddMaterial(i, &m_dmaTag) < 0)
        return -1;
    FlushDMABufferIfFull();
    return 0;
}

int VIRaster::MaterialPalHasDepthSort(int pal)
{
    VIMaterialPal *p = m_materialPals.Value(pal);
    for (int i = 0; i < p->m_mats.m_size; i++)
    {
        if (MaterialHasDepthSort(p->m_mats[i]))
            return 1;
    }
    return 0;
}

// --------------------------------------------------------- immediate mode --

int VIRaster::UV(const VIVect2 &uv)
{
    m_uv.x = uv.x;
    m_uv.y = uv.y;
    return 0;
}

int VIRaster::Normal(const VIVect3 &n)
{
    m_normal = n;
    return 0;
}

int VIRaster::Color(const VIColor32F &c)
{
    m_color = c;
    return 0;
}

int VIRaster::Vertex(const VIVect3 &v)
{
    m_vtx->x = v.x;
    m_vtx->y = v.y;
    m_vtx->z = v.z;
    m_vtx->w = 1.0f;
    m_vtx++;
    m_vtx->x = m_uv.x;
    m_vtx->y = m_uv.y;
    m_vtx->z = 1.0f;
    m_vtx->w = 0.0f;
    m_vtx++;
    m_vtx->x = m_normal.x;
    m_vtx->y = m_normal.y;
    m_vtx->z = m_normal.z;
    m_vtx->w = 0.0f;
    m_vtx++;
    m_vtx->x = m_color.r;
    m_vtx->y = m_color.g;
    m_vtx->z = m_color.b;
    m_vtx->w = m_color.a;
    m_vtx++;
    m_numVerts++;
    return 0;
}

// ------------------------------------------------------------- the matrices --

int VIRaster::SetModelView(const VIMatrix44 &m)
{
    sceVu0CopyMatrix(&VIRASTER_SPR->m_modelView, m);
    m_dirtyLights = 1;
    m_dirtyMatrices = 1;
    return 0;
}

int VIRaster::SetProjection(const VIMatrix44 &m)
{
    sceVu0CopyMatrix(&VIRASTER_SPR->m_projection, m);
    m_dirtyLights = 1;
    m_dirtyMatrices = 1;
    return 0;
}

int VIRaster::SetPreTranslations(int n, VIVect3 *v, const VIVect3 &at)
{
    m_dirtyPreTrans = 1;
    m_preTransAt = at;
    memcpy(m_preTrans, v, n * (int)sizeof(VIVect3));
    return 0;
}

// ----------------------------------------------------------------- the DMA --

int VIRaster::Flush()
{
    if (m_dmaTag != m_dmaStart)
    {
        m_dmaTag->m_id = 112;
        m_dmaTag->m_addr = 0;
        m_dmaTag->m_qwc = 0;
        m_dmaTag++;
        VIEEDMAStart(0);
        VIEEDMASync(1);
        VIEEDMAKick(1, m_dmaStart);
        if (m_dmaStart == m_dmaBuffer0)
        {
            m_dmaTag = m_dmaBuffer1;
            m_dmaStart = m_dmaBuffer1;
        }
        else
        {
            m_dmaTag = m_dmaBuffer0;
            m_dmaStart = m_dmaBuffer0;
        }
        m_p4C18 = 0;
        m_dmaFlushes++;
    }
    return 0;
}

int VIRaster::FlushDMABufferIfFull()
{
    if (m_dmaTag != m_dmaStart)
    {
        if (m_dmaTag - m_dmaStart > 0x9200 || m_p4C18 >= 921)
        {
            if (m_sceneState == 0)
            {
                VIEEDMASync(1);
                Flip();
                m_frame--;
            }
            m_sceneState = 1;
            Flush();
        }
    }
    return 0;
}

// ------------------------------------------------------------ the vertices --

int VIRaster::BeginTriStrip()
{
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    if (m_dirtyLights)
    {
        SetFogRegisters();
        UploadPerMaterial(&m_dmaTag);
    }
    m_primType = 1;
    m_numVerts = 0;
    m_vtxBase = (VIVect4 *)(m_dmaTag + 1);
    m_vtx = (VIVect4 *)(m_dmaTag + 3);
    return 0;
}

int VIRaster::BeginTriFan()
{
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    if (m_dirtyLights)
    {
        SetFogRegisters();
        UploadPerMaterial(&m_dmaTag);
    }
    m_primType = 2;
    m_numVerts = 0;
    m_vtxBase = (VIVect4 *)(m_dmaTag + 1);
    m_vtx = (VIVect4 *)(m_dmaTag + 3);
    return 0;
}

int VIRaster::End2D()
{
    m_primType = 0;
    Restore2D();
    return 0;
}

int VIRaster::Vertex(const VIVect2 &v)
{
    m_vtx->x = (v.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx->y = (v.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx->z = 0.0f;
    m_vtx->w = 1.0f;
    m_vtx++;
    m_vtx->x = m_uv.x;
    m_vtx->y = m_uv.y;
    m_vtx->z = 1.0f;
    m_vtx->w = 0.0f;
    m_vtx++;
    m_vtx->x = m_normal.x;
    m_vtx->y = m_normal.y;
    m_vtx->z = m_normal.z;
    m_vtx->w = 0.0f;
    m_vtx++;
    m_vtx->x = m_color.r;
    m_vtx->y = m_color.g;
    m_vtx->z = m_color.b;
    m_vtx->w = m_color.a;
    m_vtx++;
    m_numVerts++;
    return 0;
}

int VIRaster::Vertex(const VIVect2I &v)
{
    m_vtx->x = ((float)v.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx->y = ((float)v.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx->z = 0.0f;
    m_vtx->w = 1.0f;
    m_vtx++;
    m_vtx->x = m_uv.x;
    m_vtx->y = m_uv.y;
    m_vtx->z = 1.0f;
    m_vtx->w = 0.0f;
    m_vtx++;
    m_vtx->x = m_normal.x;
    m_vtx->y = m_normal.y;
    m_vtx->z = m_normal.z;
    m_vtx->w = 0.0f;
    m_vtx++;
    m_vtx->x = m_color.r;
    m_vtx->y = m_color.g;
    m_vtx->z = m_color.b;
    m_vtx->w = m_color.a;
    m_vtx++;
    m_numVerts++;
    return 0;
}

void VIRaster::Orthographic(VIMatrix44 &m, float l, float r, float b, float t,
                            float n, float f)
{
    m.m[0].y = 0.0f;
    m.m[0].z = 0.0f;
    m.m[0].w = 0.0f;
    m.m[1].x = 0.0f;
    m.m[1].z = 0.0f;
    m.m[1].w = 0.0f;
    m.m[2].x = 0.0f;
    m.m[2].y = 0.0f;
    m.m[2].w = 0.0f;
    m.m[3].w = 1.0f;
    m.m[0].x = 2.0f / (r - l);
    m.m[1].y = 2.0f / (b - t);
    m.m[2].z = -2.0f / (f - n);
    m.m[3].x = -(r + l) / (r - l);
    m.m[3].y = -(b + t) / (b - t);
    m.m[3].z = -(f + n) / (f - n);
}

int VIRaster::Viewport(int x1, int y1, int x2, int y2)
{
    int w = x2 - x1 + 1;
    int h = y2 - y1 + 1;
    float hw = (float)w * 0.5f;
    float hh = (float)h * 0.5f;
    m_viewportX = x1;
    m_viewportY = y1;
    m_viewportW = w;
    m_viewportH = h;
    m_viewportWF = hw;
    m_viewportHF = hh;
    m_invViewportW = 1.0f / hw;
    m_invViewportH = 1.0f / hh;
    m_aspectX = 2048.0f - (m_screenWF - ((float)x1 + hw));
    m_aspectY = 2048.0f - (m_screenHF - ((float)y1 + hh));
    CalcClipToScreen();
    CalcMatrices();
    m_dirtyLights = 1;
    m_dirtyMatrices = 1;
    return 0;
}

int VIRaster::NumTextureSwaps()
{
    return g_surfCache.m_numDownloads;
}

int VIRaster::SetFog(int on, float near, float far, const VIColor32F &c)
{
    m_fogNear = near;
    m_fogFar = far;
    m_fogEnabled = on;
    float *sprNear = (float *)0x700001F8;
    float *sprFar = (float *)0x700001FC;
    *sprNear = 0.0f;
    *sprFar = 255.0f;
    m_fogColor = c;
    *(float *)0x700001F0 = (*sprNear - *sprFar) / (m_fogFar - m_fogNear);
    *(float *)0x700001F4 =
        (*sprFar * m_fogFar - *sprNear * m_fogNear) / (m_fogFar - m_fogNear);
    SetFogRegisters();
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::CreateMaterial(int share)
{
    VIMaterial *m = (VIMaterial *)m_materials.Add();
    if ((int)m < 0)
        return -1;
    if (share)
        m->m_userData++;
    *(long *)&m->m_time = m_time;
    return (int)m;
}

int VIRaster::ReleaseMaterial(int i, VIDictionary &d)
{
    VIMaterial *m = (VIMaterial *)i;
    m->m_userData--;
    if (m->m_userData <= 0)
    {
        if (m->m_handle != -1)
            m_materialLRU.Value(m->m_handle).m_c = -1;
        for (int k = 0; k < m->m_numLayers; k++)
        {
            if (m->m_layers[k].m_texture != -1)
                ReleaseSurface(m->m_layers[k].m_texture, d);
        }
        int r = m_materials.Erase(i);
        if (r < 0)
            return -1;
    }
    return 0;
}

int VIRaster::ReleaseFont(int i, VIDictionary &d)
{
    VIRFont *f = m_fonts.Value(i);
    f->m_userData--;
    if (f->m_userData <= 0)
    {
        if (f->m_id != 0)
        {
            if (d.Remove(*(VIResource *)f) < 0)
                return -1;
        }
        delete f;
        if (m_fonts.Erase(i) < 0)
            return -1;
    }
    return 0;
}

// ------------------------------------------------------------- the SPR path --

int VIRaster::CalcMatrices()
{
    VIVu0MulMatrix((void *)0x70000080, (void *)0x70000000, (void *)0x70000040);
    VIVu0MulMatrix((void *)0x70000140, (void *)0x70000080, (void *)0x700000C0);
    VIVu0InverseMatrix((void *)0x70000100, (void *)0x70000000);
    VIVu0ApplyMatrix((void *)0x70000100, (void *)0x700001A0, (void *)0x70000180);
    VIVu0Normalize((void *)0x70000180);
    if (*(unsigned char *)0x70000222 != 0)
        VIVu0ApplyMatrix((void *)0x70000100, (void *)0x700001C0, (void *)0x70000190);
    return 0;
}

int VIRaster::CalcClipToScreen()
{
    VIVu0UnitMatrix((void *)0x700000C0);
    *(float *)0x700000C0 = (float)m_viewportW * 0.5f;
    *(float *)0x700000D4 = -(float)m_viewportH * 0.5f;
    *(float *)0x700000F0 = 2048.0f - (float)m_screenW * 0.5f
                           + (float)m_viewportW * 0.5f + (float)m_viewportX;
    *(float *)0x700000F4 = 2048.0f - (float)m_screenH * 0.5f
                           + (float)m_viewportH * 0.5f + (float)m_viewportY;
    *(float *)0x700000E8 = -(m_p4570 - m_p456C) * 0.5f;
    *(float *)0x700000F8 = (m_p4570 - m_p456C) * 0.5f + m_p456C + m_zBias;
    return 0;
}

void VIRaster::SetZBias(int z)
{
    if (z > 0)
        m_zBias = (float)z * 166.0f;
    else
        m_zBias = 0.0f;
    CalcClipToScreen();
    m_dirtyLights = 1;
}

int VIRaster::Store2D()
{
    VIVu0CopyMatrix(&m_m44B0, (void *)0x70000000);
    VIVu0CopyMatrix(&m_m44F0, (void *)0x70000040);
    unsigned char *amb = (unsigned char *)0x70000220;
    unsigned char *sun = (unsigned char *)0x70000221;
    m_b4530 = *amb;
    m_b4531 = *sun;
    *amb = 0;
    *sun = 0;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::Restore2D()
{
    VIVu0CopyMatrix((void *)0x70000000, &m_m44B0);
    VIVu0CopyMatrix((void *)0x70000040, &m_m44F0);
    *(unsigned char *)0x70000220 = m_b4530;
    *(unsigned char *)0x70000221 = m_b4531;
    m_dirtyLights = 1;
    return 0;
}

// -------------------------------------------------------- the double buffers --

int VIRaster::InitSPRDoubleBuffer()
{
    m_p08 = 0x70002000;
    m_p04 = 0x70000000;
    m_p00 = 0x70002000;
    return 0;
}

int VIRaster::SwapSPRBuffer()
{
    if (m_p00 == m_p04)
        m_p00 = m_p08;
    else
        m_p00 = m_p04;
    return m_p00;
}

int VIRaster::SPRBuffer()
{
    return m_p00;
}

int VIRaster::ClearMaterialLRU()
{
    if (m_p4C14)
    {
        delete m_p4C14;
        m_p4C14 = 0;
    }
    if (m_materialLRU.Clear() < 0)
        return -1;
    m_p4C18 = 0;
    return 0;
}

int VIRaster::InitMaterialLRU(int n)
{
    if (ClearMaterialLRU() < 0)
        return -1;
    if (m_materialLRU.Init(n) < 0)
        return -1;
    int q = n * 53;
    m_p4C14 = new char[q * 16];
    if (m_p4C14 == 0)
        return -1;
    for (int off = 0, i = 0; i < n; i++, off += 848)
    {
        VIRasterMaterialLRU e;
        e.m_a = (int)(m_p4C14 + off);
        e.m_b = 0;
        e.m_c = -1;
        e.m_d = 0;
        if (m_materialLRU.Add(e) < 0)
            return -1;
    }
    return 0;
}

int VIRaster::ClearDMADoubleBuffer()
{
    if (m_dmaBuffer0)
    {
        delete (char *)m_dmaBuffer0;
        m_dmaBuffer0 = 0;
    }
    if (m_dmaBuffer1)
    {
        delete (char *)m_dmaBuffer1;
        m_dmaBuffer1 = 0;
    }
    return 0;
}

int VIRaster::InitDMADoubleBuffer()
{
    if (ClearDMADoubleBuffer() < 0)
        return -1;
    m_dmaBuffer0 = (_sceDmaTag *)new char[0x96000];
    if (m_dmaBuffer0 == 0)
        return -1;
    m_dmaBuffer1 = (_sceDmaTag *)new char[0x96000];
    if (m_dmaBuffer1 == 0)
        return -1;
    m_dmaTag = m_dmaBuffer0;
    m_dmaStart = m_dmaBuffer0;
    return 0;
}

void VIRaster::SetClipAndLight(int on)
{
    m_p45C8 = on;
}

// ------------------------------------------------------------- the releases --

int VIRaster::ReleasePrimBuffer(int i, VIDictionary &d)
{
    VIPrimBuffer *pb = m_primBuffers.Value(i);
    pb->m_userData--;
    if (pb->m_userData <= 0)
    {
        if (pb->m_id != 0)
        {
            if (d.Remove(*(VIResource *)pb) < 0)
                return -1;
        }
        pb->m_userFlags = m_frame;
        if (m_primBufferFree.Add(pb) < 0)
            return -1;
        if (m_primBuffers.Erase(i) < 0)
            return -1;
    }
    return 0;
}

int VIRaster::ReleaseSurface(int i, VIDictionary &d)
{
    VISurface *s = m_surfaces.Value(i);
    s->m_userData--;
    if (s->m_userData <= 0)
    {
        if (g_surfCache.Decache(*s) < 0)
            return -1;
        if (s->m_id != 0)
        {
            if (d.Remove(*(VIResource *)s) < 0)
                return -1;
        }
        s->m_pad24 = m_frame;
        if (m_surfaceFree.Add(s) < 0)
            return -1;
        if (m_surfaces.Erase(i) < 0)
            return -1;
    }
    return 0;
}

int VIRaster::EndScene()
{
    if (m_sceneState & 1)
        Flush();
    VIEEDMASync(1);
    if (m_sceneState == 2)
    {
        m_sceneState = 0;
        m_frame++;
    }
    else
    {
        Flip();
    }
    if (m_sceneState == 0)
        Flip();
    if (m_sceneState == 1)
        m_sceneState = 2;
    else
        m_sceneState = 0;
    m_blitBlend = 0;
    m_blitSurface = -1;
    if (BeginScene() < 0)
        return -1;
    return 0;
}

int VIRaster::DebugEEMicro()
{
    VIEEMicroDump(18, 35);
    VIEEMicroDump(342, 359);
    VIEEMicroDump(100, 130);
    VIEEMicroDump(131, 134);
    VIEEMicroDump(146, 176);
    VIEEMicroDump(607, 637);
    VIEEMicroDump(470, 502);
    VIEEMicroDump(242, 281);
    VIEEMicroDump(292, 331);
    return 0;
}

int VIRaster::ReleaseMaterialPal(int pal, VIDictionary &d)
{
    VIMaterialPal *p = m_materialPals.Value(pal);
    p->m_refCount--;
    if (p->m_refCount <= 0)
    {
        for (int i = 0; i < p->m_mats.m_size; i++)
        {
            if (ReleaseMaterial(p->m_mats[i], d) < 0)
                return -1;
        }
        if (p->m_p00 != 0)
        {
            if (d.Remove(*(VIResource *)p) < 0)
                return -1;
        }
        delete p;
        int r = m_materialPals.Erase(pal);
        if (r < 0)
            return -1;
    }
    return 0;
}

int VIRaster::CreateMaterialPal(int n, int share)
{
    VIMaterialPal *p = new VIMaterialPal;
    if (p == 0)
        return -1;
    if (p->m_mats.Init(n) < 0)
        return -1;
    int idx = m_materialPals.Add(p);
    if (idx < 0)
        return -1;
    for (int i = 0; i < n; i++)
    {
        int *e = &p->m_mats.m_data[i];
        *e = CreateMaterial(0);
        if (p->m_mats.m_data[i] < 0)
            return -1;
    }
    if (share)
        p->m_refCount++;
    return idx;
}

int VIRaster::CopyMaterialPal(int pal, int share)
{
    VIMaterialPal *p = new VIMaterialPal;
    if (p == 0)
        return -1;
    if (p->m_mats.Init(GetMaterialPalSize(pal)) < 0)
        return -1;
    int idx = m_materialPals.Add(p);
    if (idx < 0)
        return -1;
    VIArray<int> &sm = m_materialPals.Value(pal)->m_mats;
    for (int i = 0; i < GetMaterialPalSize(pal); i++)
    {
        int *e = &p->m_mats.m_data[i];
        *e = CopyMaterial(sm.m_data[i], share);
        if (p->m_mats.m_data[i] < 0)
            return -1;
    }
    if (share)
        p->m_refCount++;
    return idx;
}

int VIRaster::DrawPrimBuffer(int prim, int pal, unsigned int flags)
{
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    switch (pb->m_type)
    {
    case VIPrimBufferTypeVUNC:
        return DrawPrimBufferVUNC(prim, pal, flags);
    case VIPrimBufferTypePackVUNC:
        return DrawPrimBufferPackVUNC(prim, pal, flags);
    case VIPrimBufferTypePackVUNCG:
        return DrawPrimBufferPackVUNCG(prim, pal, flags);
    case VIPrimBufferTypePackVUNBW:
        return DrawPrimBufferPackVUNBW(prim, pal, flags);
    case VIPrimBufferTypePackVU:
        return DrawPrimBuffer(prim, pal, 1.0f, flags);
    }
    return -1;
}

int VIRaster::DrawPrimBuffer(int prim, int pal, float f, unsigned int flags)
{
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    if (pb->m_type == VIPrimBufferTypePackVU)
        return DrawPrimBufferPackVU(prim, pal, f, flags);
    return -1;
}

// ---------------------------------------------------------- phase 57 --------

int VIRaster::SetSunLight(const VIMatrix44 &m, const VIRasterDirLight &l)
{
    VIRASTER_SPR->m_sun.m_direction = l.m_direction;
    VIRASTER_SPR->m_sun.m_color = l.m_color;
    m.SubMatrixMul(l.m_direction, VIRASTER_SPR->m_sun.m_direction);
    VIRASTER_SPR->m_sun.m_direction.Normalize();
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::SetDirLight(int i, const VIMatrix44 &m, const VIRasterDirLight &l)
{
    // The dir-light array overlaps the point-light array, so it cannot be a
    // member of VIRasterSPR; it is addressed by cast.  The whole-object
    // assignment is what keeps TWO address pseudos alive -- written through a
    // named pointer or reference, cse merges the store address with the one
    // the two calls need and the second `mult` disappears.
    ((VIRasterDirLight *)0x700001BC)[i] = l;
    m.SubMatrixMul(l.m_direction, ((VIRasterDirLight *)0x700001BC)[i].m_direction);
    ((VIRasterDirLight *)0x700001BC)[i].m_direction.Normalize();
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::SetPointLight(int i, const VIMatrix44 &m,
                            const VIRasterPointLight &l)
{
    VIRasterPointLight *p = &VIRASTER_SPR->m_points[i];
    p->m_position = l.m_position;
    p->m_range = l.m_range;
    p->m_color = l.m_color;
    m.Mul(l.m_position, p->m_position);
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::SetDefaultLights(const VIMatrix44 &m)
{
    VIVect3 dir;
    VIVect3 fill;
    // Every SPR field here is its OWN absolute constant, not a member of one
    // VIRasterSPR at 0x70000000: written through the macro, cse hoists the
    // shared base into a callee-saved register and every store goes off it,
    // where the target gives the two colours a pointer each (three uses) and
    // leaves every single-use field to the assembler's `lui at` macro.
    ((VIColor32F *)0x700001E0)->r = 0.4f;
    ((VIColor32F *)0x700001E0)->g = 0.4f;
    ((VIColor32F *)0x700001E0)->b = 0.4f;
    *(unsigned char *)0x70000220 = 1;
    *(float *)0x700001EC = 0.0f;
    // Set(), not three field assignments: the inlined `this` is what
    // materialises `addiu v1,sp,16` for the second vector.  `fill` is never
    // read -- a dead local, and a source fact.
    dir.Set(-0.577f, -0.577f, -0.577f);
    fill.Set(0.577f, 0.577f, 0.577f);
    m.SubMatrixMul(dir, VIRASTER_SPR->m_sun.m_direction);
    VIRASTER_SPR->m_sun.m_direction.Normalize();
    ((VIColor32F *)0x700001AC)->r = 0.8f;
    ((VIColor32F *)0x700001AC)->g = 0.8f;
    ((VIColor32F *)0x700001AC)->b = 0.8f;
    *(unsigned char *)0x70000221 = 1;
    *(float *)0x7000020C = 1.0f;
    *(float *)0x700001B8 = 0.0f;
    *(float *)0x70000200 = 0.0f;
    *(float *)0x70000204 = 0.0f;
    *(float *)0x70000208 = 0.0f;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::ReleaseColorBuffer(int i, VIDictionary &d)
{
    VIColorBuffer *cb = m_colorBuffers.Value(i);
    cb->m_userData--;
    if (cb->m_userData <= 0)
    {
        if (m_colorBuffers.Erase(i) < 0)
            return -1;
        if (cb->m_primBuffer != -1)
        {
            ReleasePrimBuffer(cb->m_primBuffer, d);
            cb->m_primBuffer = -1;
        }
        delete cb;
    }
    return 0;
}

int VIRaster::ReleaseDeferred()
{
    // Both free lists hold objects whose owner has let go but whose GS packets
    // may still be in flight: nothing is destroyed until the frame counter has
    // passed the stamp Release* wrote by three.
    int i = m_surfaceFree.Begin();
    while (i != m_surfaceFree.End())
    {
        int next = m_surfaceFree.Next(i);
        VISurface *s = *(VISurface **)i;
        if (m_frame >= s->m_pad24 + 3)
        {
            delete s;
            if (m_surfaceFree.Erase(i) < 0)
                return -1;
        }
        i = next;
    }
    int j = m_primBufferFree.Begin();
    while (j != m_primBufferFree.End())
    {
        int next = m_primBufferFree.Next(j);
        VIPrimBuffer *pb = *(VIPrimBuffer **)j;
        if (m_frame >= pb->m_userFlags + 3)
        {
            delete pb;
            if (m_primBufferFree.Erase(j) < 0)
                return -1;
        }
        j = next;
    }
    return 0;
}

long VIRaster::GetZBufferReg() const
{
    return DBuffer[60];
}

int VIRaster::InitDMACHandler()
{
    volatile int *p = (volatile int *)0x1000E010;
    *p = 1;
    VIEERasterDMACHandlerID = AddDmacHandler(1, VIEERasterDMACHandler, 0);
    if (VIEERasterDMACHandlerID < 0)
        return -1;
    EnableDmac(1);
    return 0;
}

int VIRaster::RemoveDMACHandler()
{
    if (VIEERasterDMACHandlerID != -1)
    {
        DisableDmac(1);
        RemoveDmacHandler(1, VIEERasterDMACHandlerID);
        VIEERasterDMACHandlerID = -1;
    }
    return 0;
}

int VIRaster::Begin2D()
{
    VIMatrix44 m;
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    Store2D();
    m.Identity();
    sceVu0CopyMatrix((VIMatrix44 *)0x70000000, m);
    m.m[3].z = -0.9999989867f;
    sceVu0CopyMatrix((VIMatrix44 *)0x70000040, m);
    m_dirtyLights = 1;
    return 0;
}

// The two strip closers.  Each one shuts the UNPACK it opened in Begin*, backs
// the write cursor up over the whole batch to fill in the header qword the
// vertices were written after, and hands the DMA tag its qword count.  The
// microprogram address is the static the matching Begin set up: a VIFcode
// MSCAL (0x14) over RStripOffset / RFanOffset.  Every store through m_vtx
// reloads it -- the member could alias what is being written.
int VIRaster::EndTriStrip()
{
    unsigned int mscal = RStripOffset | 0x14000000;
    ((unsigned int *)m_vtx)[0] = 0;
    ((unsigned int *)m_vtx)[1] = 0;
    ((unsigned int *)m_vtx)[2] = 0;
    ((unsigned int *)m_vtx)[3] = mscal;
    VIVect4 *end = m_vtx + 1;
    m_vtx = m_vtxBase;
    int n = end - m_vtxBase;
    ((unsigned int *)m_vtx)[0] = 0;
    ((unsigned int *)m_vtx)[1] = 0;
    ((unsigned int *)m_vtx)[2] = 0;
    ((unsigned int *)m_vtx)[3] = ((n - 2) << 16) | 0x6C008000;
    m_vtx++;
    ((unsigned int *)m_vtx)[0] = m_numVerts;
    ((unsigned int *)m_vtx)[1] = 0;
    ((unsigned int *)m_vtx)[2] = 0;
    ((unsigned int *)m_vtx)[3] = 0;
    m_vtx++;
    m_dmaTag->m_id = 16;
    m_dmaTag->m_addr = 0;
    m_dmaTag->m_qwc = n;
    m_primType = 0;
    int bytes = (n << 4) + 16;
    m_dmaTag = (_sceDmaTag *)((char *)m_dmaTag + bytes);
    FlushDMABufferIfFull();
    return 0;
}

int VIRaster::EndTriFan()
{
    unsigned int mscal = RFanOffset | 0x14000000;
    ((unsigned int *)m_vtx)[0] = 0;
    ((unsigned int *)m_vtx)[1] = 0;
    ((unsigned int *)m_vtx)[2] = 0;
    ((unsigned int *)m_vtx)[3] = mscal;
    VIVect4 *end = m_vtx + 1;
    m_vtx = m_vtxBase;
    int n = end - m_vtxBase;
    ((unsigned int *)m_vtx)[0] = 0;
    ((unsigned int *)m_vtx)[1] = 0;
    ((unsigned int *)m_vtx)[2] = 0;
    ((unsigned int *)m_vtx)[3] = ((n - 2) << 16) | 0x6C008000;
    m_vtx++;
    ((unsigned int *)m_vtx)[0] = m_numVerts;
    ((unsigned int *)m_vtx)[1] = 0;
    ((unsigned int *)m_vtx)[2] = 0;
    ((unsigned int *)m_vtx)[3] = 0;
    m_vtx++;
    m_dmaTag->m_id = 16;
    m_dmaTag->m_addr = 0;
    m_dmaTag->m_qwc = n;
    m_primType = 0;
    int bytes = (n << 4) + 16;
    m_dmaTag = (_sceDmaTag *)((char *)m_dmaTag + bytes);
    FlushDMABufferIfFull();
    return 0;
}

int VIRaster::EndBillboards()
{
    if (m_numBillboards <= 0)
        return -1;
    char *p = (char *)m_dmaTag;
    unsigned int *last = (unsigned int *)(p + m_numBillboards * 48);
    last[3] |= 0x100;
    unsigned int mscalf = (VIBbRender - VIBbInit) | 0x15000000;
    p += 16;
    unsigned int *e = (unsigned int *)(p + m_numBillboards * 48);
    e[8] = 0;
    unsigned int *q = e + 8;
    q[3] = mscalf;
    q[2] = 0;
    q[1] = 0;
    char *end = (char *)(e + 12);
    int qwc = (end - p) >> 4;
    m_dmaTag->m_id = 16;
    int bytes = (qwc << 4) + 16;
    m_dmaTag->m_addr = 0;
    m_dmaTag->m_qwc = qwc;
    m_dmaTag = (_sceDmaTag *)((char *)m_dmaTag + bytes);
    FlushDMABufferIfFull();
    m_numBillboards = 0;
    return 0;
}

// The skinning palette: n VIMatrix44 unpacked to VU1 in blocks of 32, which is
// 128 quadwords and the most one UNPACK's NUM field will carry.  Each block
// opens with a VIFcode pair -- a FLUSHE and an UNPACK V4-32 whose address is
// ObjectBlendsBuffer stepped by four quadwords per matrix -- and the matrices
// themselves go in with one memcpy.
int VIRaster::SetObjectBlends(int n, VIMatrix44 *m)
{
    int blocks = n / 32;
    int qwc = (n << 2) + blocks + 1;
    m_dmaTag->m_id = 16;
    m_dmaTag->m_addr = 0;
    m_dmaTag->m_qwc = qwc;
    char *p = (char *)m_dmaTag + 16;
    m_dmaTag = (_sceDmaTag *)((char *)m_dmaTag + (qwc << 4) + 16);
    int i = 0;
    char *src = (char *)m;
    while (blocks)
    {
        ((unsigned int *)p)[0] = 0;
        ((unsigned int *)p)[1] = 0;
        ((unsigned int *)p)[2] = 0x10000000;
        ((unsigned int *)p)[3] = (ObjectBlendsBuffer + i * 4) | 0x6C800000;
        p += 16;
        // A `void *` VARIABLE is what makes this a CALL: handed a char* or an
        // inline (void *) cast, gcc expands a constant-size memcpy into a
        // block-move loop, and the target has `li a2,2048` and a `jal`.
        void *dst = p;
        memcpy(dst, src, 2048);
        src += 2048;
        i += 32;
        blocks--;
        p += 2048;
    }
    int rem = n - i;
    if (rem != 0)
    {
        ((unsigned int *)p)[0] = 0;
        ((unsigned int *)p)[1] = 0;
        ((unsigned int *)p)[2] = 0x10000000;
        ((unsigned int *)p)[3] =
            (ObjectBlendsBuffer + i * 4) | (rem << 18) | 0x6C000000;
        void *dst = p + 16;
        memcpy(dst, (char *)m + i * 64, rem * 64);
    }
    return 0;
}

// FOGCOL, shipped as its own DMA packet: a VIF DIRECT of two quadwords holding
// a GIF A+D write, with the colour quantised out of m_fogColor.  The local is
// a VIColor32 -- its constructor is the {0,0,0,255} run at the top, and all
// four bytes are overwritten before the pack, which only reads three of them.
int VIRaster::SetFogRegisters()
{
    VIColor32 c;
    m_dmaTag->m_id = 16;
    m_dmaTag->m_addr = 0;
    m_dmaTag->m_qwc = 3;
    // ONE reused float temp, not four independent expressions: spelled out,
    // gcc hoists all four loads of m_fogColor to the top of the block where
    // the target converts one component at a time.
    float t;
    t = m_fogColor.r * 255.0f + 0.5f;
    c.r = (int)t;
    t = m_fogColor.g * 255.0f + 0.5f;
    c.g = (int)t;
    t = m_fogColor.b * 255.0f + 0.5f;
    c.b = (int)t;
    t = m_fogColor.a * 255.0f + 0.5f;
    c.a = (int)t;
    char *base = (char *)m_dmaTag;
    m_dmaTag = (_sceDmaTag *)(base + 64);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[2] = 0x10000000;
    ((unsigned int *)p)[3] = 0x50000002;
    ((long *)base)[4] = 0x1000000000008001;
    ((long *)base)[5] = 14;
    ((long *)base)[6] = c.r | ((long)c.g << 8) | ((long)c.b << 16);
    ((long *)base)[7] = 61;
    return 0;
}

int VIRaster::BlitBegin(_sceDmaTag **tag)
{
    if (m_dirtyLights)
    {
        CalcMatrices();
        UploadMatrices(tag);
    }
    _sceDmaTag *t = *tag;
    m_primType = 2;
    m_numVerts = 0;
    m_vtxBase = (VIVect4 *)(t + 1);
    m_vtx = (VIVect4 *)(t + 3);
    return 0;
}

// The zone's 32 pre-translations, unpacked to VU1 as V4-32 with the w of each
// left at zero: 33 quadwords, one VIFcode pair and 32 positions.
int VIRaster::UploadPreTrans(_sceDmaTag **tag)
{
    (*tag)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = 33;
    char *base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 544);
    ((unsigned int *)base)[4] = 0;
    unsigned int unpack = PreTransBuffer | 0x6C200000;
    char *p = base + 16;
    ((unsigned int *)p)[2] = 0x10000000;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[3] = unpack;
    VIVect3 *s = m_preTrans;
    p = base + 32;
    for (int i = 31; i >= 0; i--)
    {
        ((VIVect4 *)p)->x = s->x;
        ((VIVect4 *)p)->y = s->y;
        ((VIVect4 *)p)->z = s->z;
        ((VIVect4 *)p)->w = 0.0f;
        s++;
        p += 16;
    }
    m_dirtyPreTrans = 0;
    return 0;
}

// The raster microprogram: an MPG upload wrapped in three DMA tags -- a VIF
// packet that stops VU1, a REF tag pointing straight at the microcode, and a
// second packet that names the two double-buffered prim-buffer bases.
int VIRaster::UploadRasterMicro(_sceDmaTag **tag)
{
    (*tag)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = 1;
    char *base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 32);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[2] = 0;
    ((unsigned int *)p)[3] = 0x13000000;
    (*tag)->m_id = 80;
    (*tag)->m_addr = (unsigned int)VIDMARasterMicrocode;
    (*tag)->m_qwc = 0;
    *tag = *tag + 1;
    (*tag)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = 1;
    base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 32);
    ((unsigned int *)base)[4] = 0;
    p = base + 16;
    ((unsigned int *)p)[2] = PrimBuffer1 | 0x03000000;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[3] = (PrimBuffer2 - PrimBuffer1) | 0x02000000;
    m_p4C4C = 1;
    m_dirtyPreTrans = 1;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::UploadBillboardMicro(_sceDmaTag **tag)
{
    (*tag)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = 1;
    char *base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 32);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[2] = 0;
    ((unsigned int *)p)[3] = 0x13000000;
    (*tag)->m_id = 80;
    (*tag)->m_addr = (unsigned int)VIDMABillboardMicrocode;
    (*tag)->m_qwc = 0;
    *tag = *tag + 1;
    (*tag)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = 1;
    base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 32);
    ((unsigned int *)base)[4] = 0;
    p = base + 16;
    ((unsigned int *)p)[2] = 0x0300021A;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[3] = 0x020000F0;
    m_dirtyMatrices = 1;
    m_p4C4C = 2;
    return 0;
}

// EndTriFan's twin for the blit path: the same strip closer, but the DMA tag
// it fills belongs to the caller and there is no flush.
int VIRaster::BlitEnd(_sceDmaTag **tag)
{
    unsigned int mscal = RFanOffset | 0x14000000;
    ((unsigned int *)m_vtx)[0] = 0;
    ((unsigned int *)m_vtx)[1] = 0;
    ((unsigned int *)m_vtx)[2] = 0;
    ((unsigned int *)m_vtx)[3] = mscal;
    VIVect4 *end = m_vtx + 1;
    m_vtx = m_vtxBase;
    int n = end - m_vtxBase;
    ((unsigned int *)m_vtx)[0] = 0;
    ((unsigned int *)m_vtx)[1] = 0;
    ((unsigned int *)m_vtx)[2] = 0;
    ((unsigned int *)m_vtx)[3] = ((n - 2) << 16) | 0x6C008000;
    m_vtx++;
    ((unsigned int *)m_vtx)[0] = m_numVerts;
    ((unsigned int *)m_vtx)[1] = 0;
    ((unsigned int *)m_vtx)[2] = 0;
    ((unsigned int *)m_vtx)[3] = 0;
    m_vtx++;
    int bytes = (n << 4) + 16;
    (*tag)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = n;
    *tag = (_sceDmaTag *)((char *)*tag + bytes);
    m_primType = 0;
    return 0;
}

// DTHE, its own packet: the same VIF DIRECT + GIF A+D shape as SetFogRegisters.
int VIRaster::SetGSDithering(int on)
{
    m_dmaTag->m_id = 16;
    m_dmaTag->m_addr = 0;
    m_dmaTag->m_qwc = 3;
    char *base = (char *)m_dmaTag;
    m_dmaTag = (_sceDmaTag *)(base + 64);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    ((unsigned int *)p)[3] = 0x50000002;
    ((unsigned int *)p)[2] = 0x10000000;
    ((unsigned int *)p)[1] = 0;
    ((long *)base)[4] = 0x1000000000008001;
    ((long *)base)[5] = 14;
    ((long *)base)[6] = on;
    ((long *)base)[7] = 69;
    return 0;
}

// COLCLAMP, TEST_1 and the dither matrix DIMX, three A+D writes in one packet.
int VIRaster::InitGSRegisters(_sceDmaTag **tag)
{
    (*tag)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = 5;
    char *base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 96);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    ((unsigned int *)p)[3] = 0x50000004;
    ((unsigned int *)p)[2] = 0x10000000;
    ((unsigned int *)p)[1] = 0;
    ((long *)base)[4] = 0x1000000000008003;
    ((long *)base)[5] = 14;
    ((long *)base)[6] = 1;
    ((long *)base)[7] = 70;
    ((long *)base)[8] = 0x30000;
    ((long *)base)[9] = 71;
    ((long *)base)[10] = 0x6071243571603524;
    ((long *)base)[11] = 68;
    m_p459C = 1;
    return 0;
}

// The per-material uniform block: the sun's tint, quantised into SPR by the
// lighting pass, added to the ambient and unpacked to VU1 ahead of an MSCAL.
int VIRaster::UploadPerMaterial(_sceDmaTag **tag)
{
    VIColor32F c;
    c.r = *(unsigned char *)0x70000224 * (1.0f / 255.0f);
    c.g = *(unsigned char *)0x70000225 * (1.0f / 255.0f);
    c.b = *(unsigned char *)0x70000226 * (1.0f / 255.0f);
    c.a = *(unsigned char *)0x70000227 * (1.0f / 255.0f);
    (*tag)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = 3;
    char *base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 64);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[2] = 0;
    ((unsigned int *)p)[3] = (ColorMatrix + 2) | 0x6C010000;
    *(float *)(base + 32) = c.r + *(float *)0x700001E0;
    float *d = (float *)(base + 32);
    d[1] = c.g + *(float *)0x700001E4;
    d[2] = c.b + *(float *)0x700001E8;
    d[3] = c.a + *(float *)0x700001EC;
    ((unsigned int *)base)[12] = 0;
    char *r = base + 48;
    ((unsigned int *)r)[3] = RMaterialOffset | 0x14000000;
    ((unsigned int *)r)[1] = 0;
    ((unsigned int *)r)[2] = 0;
    return 0;
}

int VIRaster::DrawPrimBuffer(int prim, int lod, int pal, unsigned int flags)
{
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    switch (pb->m_type)
    {
    case VIPrimBufferTypePackVUNC:
        return DrawPrimBufferPackVUNC(prim, lod, pal, flags);
    case VIPrimBufferTypeUnused3:
    case VIPrimBufferTypePackVUNCG:
    case VIPrimBufferTypePackVUNBW:
        return -1;
    }
    return -1;
}
