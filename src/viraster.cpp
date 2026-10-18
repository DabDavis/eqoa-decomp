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


// The DMA/GS floor.  None of these bodies is in this TU and the image is
// un-relocated, so only the ARGUMENT SHAPE is being asserted.
extern "C" int   sceGsResetGraph(int mode, int inter, int omode, int ffmd);
extern "C" int   sceGsSyncV(int mode);
extern "C" int   sceGsSyncPath(int mode, unsigned short timeout);
extern "C" void *sceDmaGetChan(int ch);
extern int   VIEEDMASync(int wait);
extern "C" void FlushCache(int mode);
void VIEEDMASend(int chan, void *tag);
// (Phase 61: `VIEEDMAStart`/`VIEEDMAKick` were guesses -- Flush calls these two.)
// (Phase 61: `VIVu0MulMatrix`/`InverseMatrix`/`ApplyMatrix`/`Normalize` were
// guesses -- CalcMatrices calls VIMatrix44::Mul/InvertAffine/SubMatrixMul and
// VIVect3::Normalize, whose `int` result is what puts the test in v1.)
// (Phase 61: `VIVu0UnitMatrix`/`VIVu0CopyMatrix` were guesses -- CalcClipToScreen
// calls VIMatrix44::Identity, Store2D/Restore2D sceVu0CopyMatrix.)
int VIEEDebugVU1Memory(int a, int b);
extern "C" void *memcpy(void *, const void *, unsigned int);

// The DMAC handler the raster installs on channel 1 (VIF1).  The handler body
// is not in this TU; the id it is registered under is a file-scope int here,
// which is what RemoveDMACHandler tests against -1.
extern "C" int AddDmacHandler(int ch, int (*fn)(int), int next);
extern "C" int RemoveDmacHandler(int ch, int id);
extern "C" int EnableDmac(int ch);
extern "C" int DisableDmac(int ch);
int VIEERasterDMACHandler(int ch);
void VIEEDMASend(int chan, void *tag);

// The billboard microprogram.  Only the DIFFERENCE of the two labels is used
// -- `(VIBbRender - VIBbInit) >> 3` is the MSCALF entry point in VU
// instructions -- so they are declared as arrays of the 8-byte word the
// subtraction scales by.
extern "C" long VIBbInit[];
extern "C" long VIDMARasterMicrocode[];
extern "C" long VIDMABillboardMicrocode[];
extern "C" long VIBbRender[];

int VIEERasterDMACHandlerID;

// A DMA tag written through a helper: its int `qwc` parameter is what lets
// cse share one `li 1` between a tag's halfword count and a later int store.
static inline void RasterTag(_sceDmaTag **tag, int id, unsigned int addr,
                             int qwc)
{
    (*tag)->m_id = id;
    (*tag)->m_addr = addr;
    (*tag)->m_qwc = qwc;
}


// A DMA tag written through a helper: its int `qwc` parameter is what lets
// cse share one `li 1` between a tag's halfword count and a later int store.

// A float colour quantised into a VIColor32 through REFERENCES: stores through
// the inlined `c` pointer cannot be told apart from the loads off `f`, so the
// four conversions come out strictly one after another -- and the default
// constructor's stores to g, b and a survive with them.
static inline void RasterColor32(VIColor32 &c, const VIColor32F &f)
{
    c.r = (int)(f.r * 255.0f + 0.5f);
    c.g = (int)(f.g * 255.0f + 0.5f);
    c.b = (int)(f.b * 255.0f + 0.5f);
    c.a = (int)(f.a * 255.0f + 0.5f);
}



// The microprogram's entry points, as VU instruction indices.  The labels are
// absolute VU addresses, so a DIFFERENCE of two of them is not a link-time
// constant and every offset but the first is computed by the TU's static
// initialiser (phase 61).  ColorMatrix is initialised so that it, not the
// first .comm static, names `_GLOBAL_$I$`.
extern "C" long VIRInit[], VIRMaterial[], VIRStrip[], VIRStrip2[], VIRStrip3[],
                VIRStrip4[], VIRStrip5[], VIRStrip6[], VIRFan[];

int VIRaster::ColorMatrix = 15;
int VIRaster::RInitOffset = 0;
int VIRaster::RMaterialOffset = (VIRMaterial - VIRInit);
int VIRaster::RStripOffset = (VIRStrip - VIRInit);
int VIRaster::RStrip2Offset = (VIRStrip2 - VIRInit);
int VIRaster::RStrip3Offset = (VIRStrip3 - VIRInit);
int VIRaster::RStrip4Offset = (VIRStrip4 - VIRInit);
int VIRaster::RStrip5Offset = (VIRStrip5 - VIRInit);
int VIRaster::RStrip6Offset = (VIRStrip6 - VIRInit);
int VIRaster::RFanOffset = (VIRFan - VIRInit);
int VIRaster::ObjectBlendsBuffer = 612;
int VIRaster::PreTransBuffer = 932;
int VIRaster::PrimBuffer1 = 21;
int VIRaster::PrimBuffer2 = 314;
long VIRaster::DBuffer[128];

// File scope, constructed by the same static initialiser.
static VIEESurfCache g_surfCache;

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
    p->m_userData++;
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
    // Byte arithmetic on the data pointer: base-first `addu`, where every
    // int-typed subscript spelling canonicalises offset-first.
    return Material(*(int *)((char *)p->m_mats.m_data + (i << 2)));
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
    ((unsigned char *)VI_SPR(0x70000000))[i + 546] = on;
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
    ((unsigned char *)VI_SPR(0x70000000))[i + 546] = on;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::MaxPointLights()
{
    return 1;
}

int VIRaster::EnableAllPointLights(int on)
{
    unsigned char *p = (unsigned char *)VI_SPR(0x70000222);
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
    sceVu0CopyMatrix(VIRASTER_SPR->m_modelView, m);
    m_dirtyLights = 1;
    m_dirtyMatrices = 1;
    return 0;
}

int VIRaster::SetProjection(const VIMatrix44 &m)
{
    sceVu0CopyMatrix(VIRASTER_SPR->m_projection, m);
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
        FlushCache(0);
        VIEEDMASync(1);
        VIEEDMASend(1, m_dmaStart);
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
        CalcMatrices();
        UploadMatrices(&m_dmaTag);
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
        CalcMatrices();
        UploadMatrices(&m_dmaTag);
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
    m_viewportX = x1;
    m_viewportY = y1;
    // The sizes go to the members FIRST and the halves are read back off
    // them -- no `int w` local, which is what reordered the stores.
    m_viewportW = x2 - x1 + 1;
    m_viewportH = y2 - y1 + 1;
    float hw = (float)m_viewportW * 0.5f;
    float hh = (float)m_viewportH * 0.5f;
    m_viewportWF = hw;
    m_viewportHF = hh;
    m_invViewportW = 1.0f / hw;
    m_invViewportH = 1.0f / hh;
    m_aspectX = 2048.0f - (m_screenWF - ((float)x1 + hw));
    m_aspectY = 2048.0f - (m_screenHF - ((float)y1 + hh));
    CalcClipToScreen();
    SetGSScissoring();
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
    float *sprNear = (float *)VI_SPR(0x700001F8);
    float *sprFar = (float *)VI_SPR(0x700001FC);
    *sprNear = 0.0f;
    *sprFar = 255.0f;
    m_fogColor = c;
    *(float *)VI_SPR(0x700001F0) = (*sprNear - *sprFar) / (m_fogFar - m_fogNear);
    *(float *)VI_SPR(0x700001F4) =
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
    ((VIMatrix44 *)VI_SPR(0x70000080))->Mul(*(VIMatrix44 *)VI_SPR(0x70000000),
                                    *(VIMatrix44 *)VI_SPR(0x70000040));
    ((VIMatrix44 *)VI_SPR(0x70000140))->Mul(*(VIMatrix44 *)VI_SPR(0x70000080),
                                    *(VIMatrix44 *)VI_SPR(0x700000C0));
    ((VIMatrix44 *)VI_SPR(0x70000100))->InvertAffine(*(VIMatrix44 *)VI_SPR(0x70000000));
    ((VIMatrix44 *)VI_SPR(0x70000100))->SubMatrixMul(*(VIVect3 *)VI_SPR(0x700001A0),
                                             *(VIVect3 *)VI_SPR(0x70000180));
    ((VIVect3 *)VI_SPR(0x70000180))->Normalize();
    if (*(unsigned char *)VI_SPR(0x70000222) != 0)
        ((VIMatrix44 *)VI_SPR(0x70000100))->Mul(*(VIVect3 *)VI_SPR(0x700001C0),
                                        *(VIVect3 *)VI_SPR(0x70000190));
    return 0;
}

int VIRaster::CalcClipToScreen()
{
    ((VIMatrix44 *)VI_SPR(0x700000C0))->Identity();
    *(float *)VI_SPR(0x700000C0) = (float)m_viewportW * 0.5f;
    *(float *)VI_SPR(0x700000D4) = -(float)m_viewportH * 0.5f;
    *(float *)VI_SPR(0x700000F0) = 2048.0f - (float)m_screenW * 0.5f
                           + (float)m_viewportW * 0.5f + (float)m_viewportX;
    *(float *)VI_SPR(0x700000F4) = 2048.0f - (float)m_screenH * 0.5f
                           + (float)m_viewportH * 0.5f + (float)m_viewportY;
    *(float *)VI_SPR(0x700000E8) = -(m_p4570 - m_p456C) * 0.5f;
    *(float *)VI_SPR(0x700000F8) = (m_p4570 - m_p456C) * 0.5f + m_p456C + m_zBias;
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
    sceVu0CopyMatrix(m_m44B0, *(VIMatrix44 *)VI_SPR(0x70000000));
    sceVu0CopyMatrix(m_m44F0, *(VIMatrix44 *)VI_SPR(0x70000040));
    unsigned char *amb = (unsigned char *)VI_SPR(0x70000220);
    unsigned char *sun = (unsigned char *)VI_SPR(0x70000221);
    m_b4530 = *amb;
    m_b4531 = *sun;
    *amb = 0;
    *sun = 0;
    m_dirtyLights = 1;
    return 0;
}

int VIRaster::Restore2D()
{
    sceVu0CopyMatrix(*(VIMatrix44 *)VI_SPR(0x70000000), m_m44B0);
    sceVu0CopyMatrix(*(VIMatrix44 *)VI_SPR(0x70000040), m_m44F0);
    *(unsigned char *)VI_SPR(0x70000220) = m_b4530;
    *(unsigned char *)VI_SPR(0x70000221) = m_b4531;
    m_dirtyLights = 1;
    return 0;
}

// -------------------------------------------------------- the double buffers --

int VIRaster::InitSPRDoubleBuffer()
{
    m_p08 = VI_SPRI(0x70002000);
    m_p04 = VI_SPRI(0x70000000);
    m_p00 = VI_SPRI(0x70002000);
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
        delete [] m_p4C14;
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
    // A multiply, not a running offset: the strength-reduced giv is what puts
    // its `move s1,zero` in the loop preheader.
    for (int i = 0; i < n; i++)
    {
        VIRasterMaterialLRU e;
        e.m_a = (int)(m_p4C14 + i * 848);
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
        delete [] (char *)m_dmaBuffer0;
        m_dmaBuffer0 = 0;
    }
    if (m_dmaBuffer1)
    {
        delete [] (char *)m_dmaBuffer1;
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
        pb->m_userFlags = m_frame; VI_HOST_ONLY(vi_port_static_dma_changed();)
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
        Flush();
    if (m_sceneState == 1)
        m_sceneState = 2;
    else
        m_sceneState = 0;
    m_blitBlend = 0;
    m_blitSurface = -1;
    if (ReleaseDeferred() < 0)
        return -1;
    return 0;
}

int VIRaster::DebugEEMicro()
{
    VIEEDebugVU1Memory(18, 35);
    VIEEDebugVU1Memory(342, 359);
    VIEEDebugVU1Memory(100, 130);
    VIEEDebugVU1Memory(131, 134);
    VIEEDebugVU1Memory(146, 176);
    VIEEDebugVU1Memory(607, 637);
    VIEEDebugVU1Memory(470, 502);
    VIEEDebugVU1Memory(242, 281);
    VIEEDebugVU1Memory(292, 331);
    return 0;
}

int VIRaster::ReleaseMaterialPal(int pal, VIDictionary &d)
{
    VIMaterialPal *p = m_materialPals.Value(pal);
    p->m_userData--;
    if (p->m_userData <= 0)
    {
        for (int i = 0; i < p->m_mats.m_size; i++)
        {
            if (ReleaseMaterial(p->m_mats[i], d) < 0)
                return -1;
        }
        if (p->m_id != 0)
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
        // Re-read through byte arithmetic: base-first `addu`, as in
        // GetMaterialPalEntry.
        if (*(int *)((char *)p->m_mats.m_data + (i << 2)) < 0)
            return -1;
    }
    if (share)
        p->m_userData++;
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
        *e = CopyMaterial(*(int *)((char *)sm.m_data + (i << 2)), share);
        if (*(int *)((char *)p->m_mats.m_data + (i << 2)) < 0)
            return -1;
    }
    if (share)
        p->m_userData++;
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
        return DrawPrimBufferPackVU(prim, pal, 1.0f, flags);
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
    ((VIRasterDirLight *)VI_SPR(0x700001BC))[i] = l;
    m.SubMatrixMul(l.m_direction, ((VIRasterDirLight *)VI_SPR(0x700001BC))[i].m_direction);
    ((VIRasterDirLight *)VI_SPR(0x700001BC))[i].m_direction.Normalize();
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
    ((VIColor32F *)VI_SPR(0x700001E0))->r = 0.4f;
    ((VIColor32F *)VI_SPR(0x700001E0))->g = 0.4f;
    ((VIColor32F *)VI_SPR(0x700001E0))->b = 0.4f;
    *(unsigned char *)VI_SPR(0x70000220) = 1;
    *(float *)VI_SPR(0x700001EC) = 0.0f;
    // Set(), not three field assignments: the inlined `this` is what
    // materialises `addiu v1,sp,16` for the second vector.  `fill` is never
    // read -- a dead local, and a source fact.
    dir.Set(-0.577f, -0.577f, -0.577f);
    fill.Set(0.577f, 0.577f, 0.577f);
    m.SubMatrixMul(dir, VIRASTER_SPR->m_sun.m_direction);
    VIRASTER_SPR->m_sun.m_direction.Normalize();
    ((VIColor32F *)VI_SPR(0x700001AC))->r = 0.8f;
    ((VIColor32F *)VI_SPR(0x700001AC))->g = 0.8f;
    ((VIColor32F *)VI_SPR(0x700001AC))->b = 0.8f;
    *(unsigned char *)VI_SPR(0x70000221) = 1;
    *(float *)VI_SPR(0x7000020C) = 1.0f;
    *(float *)VI_SPR(0x700001B8) = 0.0f;
    *(float *)VI_SPR(0x70000200) = 0.0f;
    *(float *)VI_SPR(0x70000204) = 0.0f;
    *(float *)VI_SPR(0x70000208) = 0.0f;
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
        delete cb; VI_HOST_ONLY(vi_port_static_dma_changed();)
    }
    return 0;
}

int VIRaster::ReleaseDeferred()
{
    // Both free lists hold objects whose owner has let go but whose GS packets
    // may still be in flight: nothing is destroyed until the frame counter has
    // passed the stamp Release* wrote by three.  ONE iterator for both walks:
    // a second variable swaps the first loop's list and iterator registers.
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
    i = m_primBufferFree.Begin();
    while (i != m_primBufferFree.End())
    {
        int next = m_primBufferFree.Next(i);
        VIPrimBuffer *pb = *(VIPrimBuffer **)i;
        if (m_frame >= pb->m_userFlags + 3)
        {
            delete pb;
            if (m_primBufferFree.Erase(i) < 0)
                return -1;
        }
        i = next;
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

// The VIF1 DMAC handler: a packet parked while the channel was busy is kicked
// from interrupt context, then ExitHandler.
void *pVIEERasterPendingDMA = 0;

int VIEERasterDMACHandler(int ch)
{
    if (ch == 1)
    {
        if (pVIEERasterPendingDMA)
            VIEEDMASend(1, pVIEERasterPendingDMA);
        pVIEERasterPendingDMA = 0;
    }
    VI_EE_ASM("sync.l\n\tei\n");
    return 0;
}

int VIRaster::Begin2D()
{
    VIMatrix44 m;
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    Store2D();
    m.Identity();
    sceVu0CopyMatrix(*(VIMatrix44 *)VI_SPR(0x70000000), m);
    m.m[3].z = -0.9999989867f;
    sceVu0CopyMatrix(*(VIMatrix44 *)VI_SPR(0x70000040), m);
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
    // The billboards start one tag past m_dmaTag, and that address is taken
    // FIRST: the last billboard's flag word then addresses off the tag
    // pointer as the add's addend with no LOG_LINK back to its load, so
    // r5900_madd_profitable_p lets combine fuse the multiply and the add into
    // a madd, split after reload into a mult into a scratch (a0) and an addu.
    char *p = (char *)(m_dmaTag + 1);
    ((unsigned int *)((char *)m_dmaTag + m_numBillboards * 48))[3] |= 0x100;
    unsigned int *q;
    unsigned int mscalf = (VIBbRender - VIBbInit) | 0x15000000;
    char *e = p + m_numBillboards * 48;
    ((unsigned int *)e)[8] = 0;
    q = (unsigned int *)(e + 32);
    q[3] = mscalf;
    q[2] = 0;
    q[1] = 0;
    q = (unsigned int *)(e + 48);
    int qwc = ((char *)q - p) >> 4;
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
    int bytes = (qwc << 4) + 16;
    m_dmaTag->m_qwc = qwc;
    char *p = (char *)m_dmaTag + 16;
    m_dmaTag = (_sceDmaTag *)((char *)m_dmaTag + bytes);
    int i = 0;
    while (blocks)
    {
        // The unpack address is read BEFORE the VIFcode stores, and the
        // source is a multiply of the block index: loop.c's giv for it is
        // what initialises the source pointer in the preheader.
        // FLUSHE named first: loop.c hoists the invariants in the order the
        // loop body first mentions them.
        unsigned int flushe = 0x10000000;
        unsigned int unpack = (ObjectBlendsBuffer + i * 4) | 0x6C800000;
        ((unsigned int *)p)[0] = 0;
        ((unsigned int *)p)[1] = 0;
        ((unsigned int *)p)[2] = flushe;
        ((unsigned int *)p)[3] = unpack;
        p += 16;
        // A `void *` VARIABLE is what makes this a CALL: handed a char* or an
        // inline (void *) cast, gcc expands a constant-size memcpy into a
        // block-move loop, and the target has `li a2,2048` and a `jal`.
        void *dst = p;
        memcpy(dst, (char *)m + i * 64, 2048);
        i += 32;
        blocks--;
        p += 2048;
    }
    int rem = n - i;
    if (rem != 0)
    {
        unsigned int unpack =
            (ObjectBlendsBuffer + i * 4) | (rem << 18) | 0x6C000000;
        ((unsigned int *)p)[0] = 0;
        ((unsigned int *)p)[1] = 0;
        ((unsigned int *)p)[2] = 0x10000000;
        ((unsigned int *)p)[3] = unpack;
        void *dst = p + 16;
        memcpy(dst, (void *)(i * 64 + (int)m), rem * 64);
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
    char *base = (char *)m_dmaTag;
    m_dmaTag = (_sceDmaTag *)(base + 64);
    // Through RasterColor32's references: the conversions serialise and the
    // constructor's {0,0,0,255} survives.
    RasterColor32(c, m_fogColor);
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
    RasterTag(tag, 16, 0, 1);
    char *base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 32);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    ((unsigned int *)p)[3] = 0x13000000;
    ((unsigned int *)p)[2] = 0;
    ((unsigned int *)p)[1] = 0;
    RasterTag(tag, 80, (unsigned int)VIDMARasterMicrocode, 0);
    *tag = *tag + 1;
    RasterTag(tag, 16, 0, 1);
    char *base2 = (char *)*tag;
    *tag = (_sceDmaTag *)(base2 + 32);
    ((unsigned int *)base2)[4] = 0;
    // Both buffer bases read ONCE, ahead of the stores: PrimBuffer1 is a
    // single load feeding both VIFcodes.
    int pb1 = PrimBuffer1;
    int pb2 = PrimBuffer2;
    char *p2 = base2 + 16;
    ((unsigned int *)p2)[1] = 0;
    ((unsigned int *)p2)[2] = pb1 | 0x03000000;
    ((unsigned int *)p2)[3] = (pb2 - pb1) | 0x02000000;
    m_p4C4C = 1;
    m_dirtyLights = 1;
    m_dirtyPreTrans = 1;
    return 0;
}

int VIRaster::UploadBillboardMicro(_sceDmaTag **tag)
{
    RasterTag(tag, 16, 0, 1);
    char *base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 32);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    ((unsigned int *)p)[3] = 0x13000000;
    ((unsigned int *)p)[2] = 0;
    ((unsigned int *)p)[1] = 0;
    RasterTag(tag, 80, (unsigned int)VIDMABillboardMicrocode, 0);
    *tag = *tag + 1;
    RasterTag(tag, 16, 0, 1);
    char *base2 = (char *)*tag;
    *tag = (_sceDmaTag *)(base2 + 32);
    ((unsigned int *)base2)[4] = 0;
    char *p2 = base2 + 16;
    ((unsigned int *)p2)[1] = 0;
    ((unsigned int *)p2)[2] = 0x0300021A;
    ((unsigned int *)p2)[3] = 0x020000F0;
    m_p4C4C = 2;
    m_dirtyMatrices = 1;
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
    c.r = *(unsigned char *)VI_SPR(0x70000224) * (1.0f / 255.0f);
    c.g = *(unsigned char *)VI_SPR(0x70000225) * (1.0f / 255.0f);
    c.b = *(unsigned char *)VI_SPR(0x70000226) * (1.0f / 255.0f);
    c.a = *(unsigned char *)VI_SPR(0x70000227) * (1.0f / 255.0f);
    (*tag)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = 3;
    char *base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 64);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    // ONE pointer for the colour quadword and the MSCAL quadword after it:
    // the second `addiu a1,a2,48` reuses the first's register.
    char *q = base + 32;
    unsigned int unpack = (ColorMatrix + 2) | 0x6C010000;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[2] = 0;
    ((unsigned int *)p)[3] = unpack;
    ((float *)q)[0] = c.r + *(float *)VI_SPR(0x700001E0);
    ((float *)q)[1] = c.g + *(float *)VI_SPR(0x700001E4);
    ((float *)q)[2] = c.b + *(float *)VI_SPR(0x700001E8);
    unsigned int mscal = RMaterialOffset | 0x14000000;
    ((float *)q)[3] = c.a + *(float *)VI_SPR(0x700001EC);
    q = base + 48;
    ((unsigned int *)base)[12] = 0;
    ((unsigned int *)q)[3] = mscal;
    ((unsigned int *)q)[2] = 0;
    ((unsigned int *)q)[1] = 0;
    return 0;
}

// The baked-colour dispatcher: the second argument is a COLOUR BUFFER (phase
// 61, off DrawPrimBufferPackVUNC's own body).  The comparison tree is decided
// by the whole case set -- {0, 2, 4..5}, with 4 and 5 sharing a label, which
// is what leaves the range test's dead `slti v0,v1,4` in an annulled slot
// once every -1 arm has cross-jumped into the default.
int VIRaster::DrawPrimBuffer(int prim, int cbi, int pal, unsigned int flags)
{
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    switch (pb->m_type)
    {
    case VIPrimBufferTypeVUNC:
        return -1;
    case VIPrimBufferTypePackVUNC:
        return DrawPrimBufferPackVUNC(prim, cbi, pal, flags);
    case VIPrimBufferTypePackVUNCG:
    case VIPrimBufferTypePackVUNBW:
        return -1;
    }
    return -1;
}

// ---------------------------------------------------------- phase 61 --------

extern "C" void FlushCache(int mode);
extern "C" void sceGsSetDefDBuffDc(void *db, int psm, int w, int h, int ztest,
                                   int zpsm, int clear);
extern "C" int  sceGsSwapDBuffDc(void *db, int field);

// The constructor is the member roll-call -- the vertex attributes, the
// tessellator, the fog colour, the two saved matrices, the clear colour, the
// pre-translation block and the nine pools -- and then the body.
VIRaster::VIRaster()
{
    m_p00 = 0;
    m_p04 = 0;
    m_p08 = 0;
    m_vtx = 0;
    m_vtxBase = 0;
    m_p4C14 = 0;
    m_dmaBuffer0 = 0;
    m_dmaBuffer1 = 0;
    m_dmaTag = 0;
    m_dmaStart = 0;
    m_dmaFlushes = 1;
    m_curMaterial = -1;
    m_solidFillMaterial = -1;
    m_solidFillUIMaterial = -1;
    m_solidFillUIAVGMaterial = -1;
    m_wireframeMaterial = -1;
    m_portalMaterial = -1;
    m_blitMaterial = -1;
    m_p4C4C = 0;
}

VIRaster::~VIRaster()
{
    Close();
}

int VIRaster::Init(void *p)
{
    return Init(p, 640, 448, 0);
}

// The double-buffered GS environment: sceGsSetDefDBuffDc fills DBuffer, and
// the frame, Z and display base pointers are then patched in place -- both
// draw environments of buffer 0 at FBP 0, buffer 1 at 70, Z at 140 -- along
// with the clear colour and DTHE.
int VIRaster::Init(void *p, int w, int h, int first)
{
    VIColor32 c;
    m_firstFrame = first;
    RasterColor32(c, m_clearColor);
    if (w != 640 || h != 448)
        return -1;
    m_frame = 0;
    if (!m_firstFrame)
        sceGsResetGraph(0, 1, 2, 0);
    sceGsSetDefDBuffDc(DBuffer, 10, 640, 448, 3, 49, 1);
    // Statement order from a sweep (phase 61 wave 8: 4,041 orders, 58 -> 46),
    // not proven original.  What it fixes: sched1 prefers, among stores ready
    // together, the one whose register DIES there -- the LAST store in source
    // carries the DBuffer base's death and the last `= 1` store the constant's,
    // and each jumps the queue.  OPEN (46): the target's stores come out in
    // plain source order with [12] last in the FlushCache delay slot, which
    // needs no store to win that tie; no statement order gives it.
    DBuffer[76] = (DBuffer[76] & ~0x1FFL) | 140;
    DBuffer[48] = c.r | ((long)c.g << 8) | ((long)c.b << 16) |
                  ((long)c.a << 24) | ((long)0x3F800000 << 32);
    DBuffer[94] = c.r | ((long)c.g << 8) | ((long)c.b << 16) |
                  ((long)c.a << 24) | ((long)0x3F800000 << 32);
    DBuffer[28] &= ~0x1FFL;
    DBuffer[58] = (DBuffer[58] & ~0x1FFL) | 70;
    DBuffer[74] = (DBuffer[74] & ~0x1FFL) | 70;
    DBuffer[14] = (DBuffer[14] & ~0x1FFL) | 140;
    DBuffer[30] = (DBuffer[30] & ~0x1FFL) | 140;
    DBuffer[60] = (DBuffer[60] & ~0x1FFL) | 140;
    DBuffer[24] = 1;
    DBuffer[40] = 1;
    DBuffer[70] = 1;
    DBuffer[86] = 1;
    DBuffer[12] &= ~0x1FFL;
    ((int *)DBuffer)[4] = (((int *)DBuffer)[4] & ~0x1FF) | 70;
    FlushCache(0);
    m_p456C = 16.0f;
    m_p4570 = 8388608.0f;
    InitSPRDoubleBuffer();
    m_screenW = w;
    m_screenH = h;
    if (Init() < 0)
        return -1;
    return 0;
}

int VIRaster::SetClearColor(const VIColor32F &c)
{
    VIColor32 q;
    m_clearColor = c;
    // A named reference, which is what materialises &m_clearColor, and the
    // RGBAQ patches as addresses taken: the second is then cse'd off the
    // first, `addiu t4,DBuffer+384` + `sd 368(t4)`.
    const VIColor32F &cc = m_clearColor;
    q.r = (int)(cc.r * 255.0f + 0.5f);
    q.g = (int)(cc.g * 255.0f + 0.5f);
    q.b = (int)(cc.b * 255.0f + 0.5f);
    q.a = (int)(cc.a * 255.0f + 0.5f);
    *(long *)&DBuffer[48] = q.r | ((long)q.g << 8) | ((long)q.b << 16) |
                  ((long)q.a << 24) | ((long)0x3F800000 << 32);
    *(long *)&DBuffer[94] = q.r | ((long)q.g << 8) | ((long)q.b << 16) |
                  ((long)q.a << 24) | ((long)0x3F800000 << 32);
    FlushCache(0);
    return 0;
}

int VIRaster::Init()
{
    VIMatrix44 m;
    if (g_surfCache.Init(0x230000, 0x400000) < 0)
        return -1;
    m_sceneState = 0;
    if (InitMaterialLRU(950) < 0)
        return -1;
    if (InitDMADoubleBuffer() < 0)
        return -1;
    UploadRasterMicro(&m_dmaTag);
    InitGSRegisters(&m_dmaTag);
    m.Identity();
    SetDefaultLights(m);
    VIColor32F fog(1.0f, 1.0f, 1.0f, 1.0f);
    SetFog(1, 250.0f, 1000.0f, fog);
    m_viewportX = 0;
    m_viewportY = 0;
    m_viewportW = m_screenW;
    m_viewportH = m_screenH;
    m_viewportWF = (float)m_viewportW * 0.5f;
    m_viewportHF = (float)m_viewportH * 0.5f;
    m_invViewportW = 1.0f / m_viewportWF;
    m_invViewportH = 1.0f / m_viewportHF;
    m_screenWF = m_viewportWF;
    m_screenHF = m_viewportHF;
    m_invScreenW = m_invViewportW;
    m_invScreenH = m_invViewportH;
    m_aspectX = 2048.0f;
    m_aspectY = 2048.0f;
    m_numBillboards = 0;
    m_override = 0;
    m_wireframe = 0;
    m_solidFill = 0;
    m_multiTexture = 1;
    m_time = 0;
    m_primType = 0;
    m_dirtyLights = 1;
    m_dirtyMatrices = 1;
    m_dirtyPreTrans = 1;
    *(float *)VI_SPR(0x7000022C) = 0.15f;
    *(float *)VI_SPR(0x70000228) = 1.0f;
    *(int *)VI_SPR(0x70000210) = 0;
    *(int *)VI_SPR(0x70000214) = 0;
    *(int *)VI_SPR(0x70000218) = 0;
    *(int *)VI_SPR(0x7000021C) = 0;
    ((VIMatrix44 *)VI_SPR(0x70000000))->Identity();
    ((VIMatrix44 *)VI_SPR(0x70000040))->Identity();
    CalcClipToScreen();
    CalcMatrices();
    UploadMatrices(&m_dmaTag);
    if (m_primBuffers.Init(0) < 0)
        return -1;
    if (m_primBufferFree.Init() < 0)
        return -1;
    if (m_surfaces.Init(0) < 0)
        return -1;
    if (m_surfaceFree.Init() < 0)
        return -1;
    if (m_materials.Init() < 0)
        return -1;
    if (m_materialPals.Init(0) < 0)
        return -1;
    if (CreateDefaultResources() < 0)
        return -1;
    if (SetSolidFillMaterial() < 0)
        return -1;
    m_tessellate = 1;
    m_tess.Init();
    m_zBias = 0.0f;
    SetClipAndLight(1);
    Flush();
    if (!m_firstFrame)
    {
        for (int i = 0; i < 3; i++)
        {
            sceGsSyncV(0);
            sceGsSwapDBuffDc(DBuffer, m_frame++);
            *(volatile long *)0x12000000 = 32544;
        }
    }
    return 0;
}

int VIRaster::Clear()
{
    m_blitBlend = 0;
    m_solidFillMaterial = -1;
    m_solidFillUIMaterial = -1;
    m_solidFillUIAVGMaterial = -1;
    m_wireframeMaterial = -1;
    m_portalMaterial = -1;
    m_blitMaterial = -1;
    m_blitSurface = -1;
    if (ClearDMADoubleBuffer() < 0)
        return -1;
    if (ClearMaterialLRU() < 0)
        return -1;
    int i;
    for (i = m_colorBuffers.Begin(); i != m_colorBuffers.End();
         i = m_colorBuffers.Next(i))
        delete m_colorBuffers.Value(i);
    if (m_colorBuffers.Clear() < 0)
        return -1;
    for (i = m_primBuffers.Begin(); i != m_primBuffers.End();
         i = m_primBuffers.Next(i))
        delete m_primBuffers.Value(i);
    if (m_primBuffers.Clear() < 0)
        return -1;
    for (i = m_primBufferFree.Begin(); i != m_primBufferFree.End();
         i = m_primBufferFree.Next(i))
        delete *(VIPrimBuffer **)i;
    if (m_primBufferFree.Clear() < 0)
        return -1;
    for (i = m_surfaces.Begin(); i != m_surfaces.End();
         i = m_surfaces.Next(i))
        delete m_surfaces.Value(i);
    if (m_surfaces.Clear() < 0)
        return -1;
    for (i = m_surfaceFree.Begin(); i != m_surfaceFree.End();
         i = m_surfaceFree.Next(i))
        delete *(VISurface **)i;
    if (m_surfaceFree.Clear() < 0)
        return -1;
    if (m_materials.Clear() < 0)
        return -1;
    for (i = m_materialPals.Begin(); i != m_materialPals.End();
         i = m_materialPals.Next(i))
        delete m_materialPals.Value(i);
    if (m_materialPals.Clear() < 0)
        return -1;
    for (i = m_fonts.Begin(); i != m_fonts.End(); i = m_fonts.Next(i))
        delete m_fonts.Value(i);
    if (m_fonts.Clear() < 0)
        return -1;
    if (g_surfCache.Clear() < 0)
        return -1;
    return 0;
}

int VIRaster::BeginScene()
{
    if (m_firstFrame)
    {
        m_firstFrame = 0;
        sceGsResetGraph(0, 1, 2, 0);
        for (int i = 0; i < 3; i++)
        {
            sceGsSyncV(0);
            sceGsSwapDBuffDc(DBuffer, m_frame++);
            *(volatile long *)0x12000000 = 32544;
        }
    }
    g_surfCache.m_numDownloads = 0;
    SetGSScissoring();
    return 0;
}

int VIRaster::Flip()
{
    sceGsSyncV(0);
    sceGsSwapDBuffDc(DBuffer, m_frame);
    // Through an int copy: read straight off the member, fold narrows the
    // load to `lbu`; the target loads the whole word.  OPEN (13): the target
    // re-masks `odd & 1` at both tests (`andi v0,t1,1`).  Combine drops that
    // mask from reg_nonzero_bits, which is exact for a one-set pseudo; any
    // two-set spelling (`int odd = ~frame; odd &= 1;`) keeps it, but then the
    // NOT and the AND share odd's register where the target has v1 and t1 (16).
    int frame = m_frame;
    unsigned char odd = ~frame & 1;
    long fb0 = (int)(DBuffer[12] & 0x1FF);
    long fb1 = (int)(DBuffer[58] & 0x1FF);
    *(volatile long *)0x12000000 = 32547;
    *(volatile long *)0x12000090 = ((odd & 1) ? fb1 : fb0) | 0x51400;
    *(volatile long *)0x120000A0 = 0x001BF9FF0183227CL;
    *(volatile long *)0x12000070 = ((odd & 1) ? fb1 : fb0) | 0x51400;
    *(volatile long *)0x12000080 = 0x001BF9FF0183327DL;
    *(volatile long *)0x120000D0 = 0;
    sceGsSyncPath(0, 0);
    m_frame++;
    return 0;
}

// VIMaterialPass as its implicit operator= sees it: the A+D block is an array
// of FOUR quadwords, copied element by element with `lq`/`sq`, and +0x4C is
// alignment padding that no copy touches.
VI_QWORD_TYPEDEF(VIRasterQW);
class VIRasterPassImage
{
public:
    int          m_texture;
    unsigned int m_flags;
    VIColor32    m_color;
    int          m_blendMode;
    VIMatrix33   m_uvTransform;
    float        m_lodBias;
    VIVect2      m_uvRate;
    VIVect2      m_uvOffset;
    int          m_stamp;
    VIRasterQW   m_reg[4];
};

int VIRaster::CopyMaterial(int i, int share)
{
    VIMaterial *s = (VIMaterial *)i;
    VIMaterial *d = (VIMaterial *)m_materials.Add();
    if ((int)d < 0)
        return -1;
    if (share)
        d->m_userData++;
    d->m_numLayers = s->m_numLayers;
    d->m_flags = s->m_flags;
    *(long *)&d->m_time = *(long *)&s->m_time;
    d->m_dirty = s->m_dirty;
    int k;
    for (k = 0; k < d->m_numLayers; k++)
        *(VIRasterPassImage *)&d->m_layers[k] =
            *(VIRasterPassImage *)&s->m_layers[k];
    for (k = 0; k < d->m_numLayers; k++)
    {
        if (d->m_layers[k].m_texture != -1)
            ShareSurface(d->m_layers[k].m_texture);
    }
    return (int)d;
}

// The material's GS packet lives in an LRU slot: a dirty, animated or
// never-built material takes the least recently used slot (evicting its owner)
// and rebuilds; a resident one is only moved to the back.  Either way the
// slot is then REFerenced from the chain, and the emissive colour, when it
// changed, goes out with the per-material uniforms.
int VIRaster::AddMaterial(int i, _sceDmaTag **tag)
{
    VIMaterial *m = (VIMaterial *)i;
    int k;
    if (m_override)
        return 0;
    for (k = 0; k < m->m_numLayers; k++)
    {
        if (m->m_layers[k].m_texture != -1)
        {
            VISurface &surf = *m_surfaces.Value(m->m_layers[k].m_texture);
            if (g_surfCache.Cache(surf, *m, m->m_layers[k], *this, tag, 0) < 0)
                return -1;
        }
    }
    if (m->m_flags & 0x10)
        SetGSDitherMatrix(1, tag);
    else
        SetGSDitherMatrix(0, tag);
    if (m->m_dirty || (m->m_flags & 4) || m->m_handle == -1)
    {
        if (m->m_handle != -1)
        {
            m_materialLRU.Value(m->m_handle).m_c = -1;
            m->m_handle = -1;
        }
        k = m_materialLRU.Begin();
        if (m_materialLRU.Value(k).m_c != -1)
        {
            ((VIMaterial *)m_materialLRU.Value(k).m_c)->m_handle = -1;
            m_materialLRU.Value(k).m_c = -1;
        }
        m->m_handle = m_materialLRU.MoveToLast(k);
        m_materialLRU.Value(m->m_handle).m_c = (int)m;
        VIRasterMaterialLRU &e = m_materialLRU.Value(m->m_handle);
        e.m_b = m->ConstructDMAPacket(*this, (VIQWord *)e.m_a);
        m_materialLRU.Value(m->m_handle).m_d = m_dmaFlushes;
        m_p4C18++;
    }
    else
    {
        m->m_handle = m_materialLRU.MoveToLast(m->m_handle);
        if (m_materialLRU.Value(m->m_handle).m_d != m_dmaFlushes)
        {
            m_materialLRU.Value(m->m_handle).m_d = m_dmaFlushes;
            m_p4C18++;
        }
    }
    k = m->m_handle;
    int qwc = m_materialLRU.Value(k).m_b;
    int addr = m_materialLRU.Value(k).m_a;
    (*tag)->m_id = 48;
    (*tag)->m_addr = addr;
    (*tag)->m_qwc = qwc;
    *tag = *tag + 1;
    if (*(int *)VI_SPR(0x70000224) != *(int *)&m->m_emissive)
    {
        *(VIColor32 *)VI_SPR(0x70000224) = m->m_emissive;
        UploadPerMaterial(tag);
    }
    m_curMaterial = (int)m;
    return 0;
}

// ----------------------------------------------------------------- strings --

// One triangle fan per glyph: VIRFont::Cache resolves the string into
// m_glyphs (character indices, made resident), and each character's cell is
// drawn as a quad with the slot's four precomputed UVs.
int VIRaster::DrawString(int font, const VIVect2I &at, const VIColor32F &c,
                         const char *s)
{
    VIVect2I p0;
    VIVect2I p1;
    VIRFont *f = m_fonts.Value(font);
    int n = f->Cache(*this, s, 256, m_glyphs);
    if (n < 0)
        return -1;
    if (AddMaterial(f->m_material, &m_dmaTag) < 0)
        return -1;
    p0.x = at.x;
    p0.y = at.y;
    p1.x = at.x;
    p1.y = at.y;
    p1.y += f->m_charHeight;
    for (int i = 0; i < n; i++)
    {
        BeginTriFan();
        int slot = f->Char(m_glyphs[i])->m_slot;
        // Named in the loop, so loop.c hoists &p1 into a callee-saved
        // register -- which is what spills `c` to the stack.
        VIVect2I *pp1 = &p1;
        Color(c);
        int off = slot * (int)sizeof(VIRFontSlot);
        p1.x = p0.x + f->Char(m_glyphs[i])->m_width;
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[0]);
        Vertex(p0);
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[1]);
        Vertex(VIVect2I(p1.x, p0.y));
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[2]);
        Vertex(*pp1);
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[3]);
        Vertex(VIVect2I(p0.x, p1.y));
        EndTriFan();
        p0.x = p1.x + 1;
    }
    return 0;
}

int VIRaster::DrawString(int font, const VIVect2I &at, const VIColor32F &c,
                         const unsigned short *s)
{
    VIVect2I p0;
    VIVect2I p1;
    VIRFont *f = m_fonts.Value(font);
    int n = f->Cache(*this, s, 256, m_glyphs);
    if (n < 0)
        return -1;
    if (AddMaterial(f->m_material, &m_dmaTag) < 0)
        return -1;
    p0.x = at.x;
    p0.y = at.y;
    p1.x = at.x;
    p1.y = at.y;
    p1.y += f->m_charHeight;
    for (int i = 0; i < n; i++)
    {
        BeginTriFan();
        int slot = f->Char(m_glyphs[i])->m_slot;
        // Named in the loop, so loop.c hoists &p1 into a callee-saved
        // register -- which is what spills `c` to the stack.
        VIVect2I *pp1 = &p1;
        Color(c);
        int off = slot * (int)sizeof(VIRFontSlot);
        p1.x = p0.x + f->Char(m_glyphs[i])->m_width;
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[0]);
        Vertex(p0);
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[1]);
        Vertex(VIVect2I(p1.x, p0.y));
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[2]);
        Vertex(*pp1);
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[3]);
        Vertex(VIVect2I(p0.x, p1.y));
        EndTriFan();
        p0.x = p1.x + 1;
    }
    return 0;
}

// `mode` places `at` on the string: 0 and 3 centre it both ways, 1 only
// vertically, 2 only horizontally.  The width is the glyph run's own, one
// pixel of spacing per character.
int VIRaster::DrawCenteredString(int font, const VIVect2I &at,
                                 const VIColor32F &c, const char *s,
                                 unsigned int mode)
{
    VIVect2I p0;
    VIVect2I p1;
    int w;
    int i;
    // One glyph variable for BOTH loops: set twice, it is a global pseudo,
    // so local-alloc never ties it to the madd's sum and the width loop's
    // three locals sort into v0/v1/a0.  And `mode` is NOT reused for the
    // slot offset: with its few refs `w` outranks it in global-alloc and
    // takes its s0 preference.
    int glyph;
    VIRFont *f = m_fonts.Value(font);
    int n = f->Cache(*this, s, 256, m_glyphs);
    if (n < 0)
        return -1;
    if (AddMaterial(f->m_material, &m_dmaTag) < 0)
        return -1;
    w = 0;
    for (i = 0; i < n; i++)
    {
        glyph = m_glyphs[i];
        w += f->Char(glyph)->m_width + 1;
    }
    p0.x = at.x;
    p0.y = at.y;
    switch (mode)
    {
    case 1:
        p0.y -= f->m_charHeight >> 1;
        break;
    case 2:
        p0.x -= w >> 1;
        break;
    case 0:
    case 3:
        p0.x -= w >> 1;
        p0.y -= f->m_charHeight >> 1;
        break;
    default:
        return -1;
    }
    p1.x = p0.x;
    p1.y = p0.y;
    p1.y += f->m_charHeight;
    for (i = 0; i < n; i++)
    {
        BeginTriFan();
        glyph = m_glyphs[i];
        int slot = f->Char(glyph)->m_slot;
        // Named in the loop, so loop.c hoists &p1 into a callee-saved
        // register -- which is what spills `c` to the stack.
        VIVect2I *pp1 = &p1;
        Color(c);
        int off = slot * (int)sizeof(VIRFontSlot);
        p1.x = p0.x + f->Char(m_glyphs[i])->m_width;
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[0]);
        Vertex(p0);
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[1]);
        Vertex(VIVect2I(p1.x, p0.y));
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[2]);
        Vertex(*pp1);
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[3]);
        Vertex(VIVect2I(p0.x, p1.y));
        EndTriFan();
        p0.x = p1.x + 1;
    }
    return 0;
}

int VIRaster::DrawCenteredString(int font, const VIVect2I &at,
                                 const VIColor32F &c, const unsigned short *s,
                                 unsigned int mode)
{
    VIVect2I p0;
    VIVect2I p1;
    VIRFont *f = m_fonts.Value(font);
    int n = f->Cache(*this, s, 256, m_glyphs);
    if (n < 0)
        return -1;
    if (AddMaterial(f->m_material, &m_dmaTag) < 0)
        return -1;
    int w = 0;
    int i;
    int glyph;
    for (i = 0; i < n; i++)
    {
        glyph = m_glyphs[i];
        w += f->Char(glyph)->m_width + 1;
    }
    p0.x = at.x;
    p0.y = at.y;
    switch (mode)
    {
    case 1:
        p0.y -= f->m_charHeight >> 1;
        break;
    case 2:
        p0.x -= w >> 1;
        break;
    case 0:
    case 3:
        p0.x -= w >> 1;
        p0.y -= f->m_charHeight >> 1;
        break;
    default:
        return -1;
    }
    p1.x = p0.x;
    p1.y = p0.y;
    p1.y += f->m_charHeight;
    for (i = 0; i < n; i++)
    {
        BeginTriFan();
        glyph = m_glyphs[i];
        int slot = f->Char(glyph)->m_slot;
        // Named in the loop, so loop.c hoists &p1 into a callee-saved
        // register -- which is what spills `c` to the stack.
        VIVect2I *pp1 = &p1;
        Color(c);
        int off = slot * (int)sizeof(VIRFontSlot);
        p1.x = p0.x + f->Char(m_glyphs[i])->m_width;
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[0]);
        Vertex(p0);
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[1]);
        Vertex(VIVect2I(p1.x, p0.y));
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[2]);
        Vertex(*pp1);
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[3]);
        Vertex(VIVect2I(p0.x, p1.y));
        EndTriFan();
        p0.x = p1.x + 1;
    }
    return 0;
}

// A name plate: the anchor goes through the SPR model-view and projection to
// a screen point, and the string is drawn centred on it in 2D, at the
// anchor's own clip-space depth.
int VIRaster::DrawCenteredString(int font, const VIVect3 &at,
                                 const VIColor32F &c, const char *s)
{
    VIVect2I p0;
    VIVect2I p1;
    // f before q and v: its load is scheduled ahead of their zeroing stores.
    VIRFont *f = m_fonts.Value(font);
    VIVect2I q;
    VIVect4 v;
    int n = f->Cache(*this, s, 256, m_glyphs);
    if (n < 0)
        return -1;
    if (AddMaterial(f->m_material, &m_dmaTag) < 0)
        return -1;
    v.Set(at * *(VIMatrix44 *)VI_SPR(0x70000000));
    v.Set(v * *(VIMatrix44 *)VI_SPR(0x70000040));
    float inv = 1.0f / v.w;
    v.x *= inv;
    v.y *= inv;
    v.z *= inv;
    v.w *= inv;
    float hw = (float)(m_screenW >> 1);
    float hh = (float)(m_screenH >> 1);
    q.x = (int)(v.x * hw + hw);
    q.y = (int)(-v.y * hh + hh);
    Store2D();
    ((VIMatrix44 *)VI_SPR(0x70000000))->Identity();
    ((VIMatrix44 *)VI_SPR(0x70000040))->Identity();
    m_dirtyLights = 1;
    int w = 0;
    int i;
    int glyph;
    for (i = 0; i < n; i++)
    {
        glyph = m_glyphs[i];
        w += f->Char(glyph)->m_width + 1;
    }
    p0.x = q.x;
    p0.y = q.y;
    p0.x -= w >> 1;
    p0.y -= f->m_charHeight >> 1;
    p1.x = p0.x;
    p1.y = p0.y;
    p1.y += f->m_charHeight;
    for (i = 0; i < n; i++)
    {
        BeginTriFan();
        glyph = m_glyphs[i];
        int slot = f->Char(glyph)->m_slot;
        Color(c);
        int off = slot * (int)sizeof(VIRFontSlot);
        p1.x = p0.x + f->Char(m_glyphs[i])->m_width;
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[0]);
        Vertex(VIVect3(((float)p0.x - m_viewportWF + 0.5f) * m_invViewportW,
                       ((float)p0.y - m_viewportHF + 0.5f) * -m_invViewportH,
                       v.z));
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[1]);
        Vertex(VIVect3(((float)p1.x - m_viewportWF + 0.5f) * m_invViewportW,
                       ((float)p0.y - m_viewportHF + 0.5f) * -m_invViewportH,
                       v.z));
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[2]);
        Vertex(VIVect3(((float)p1.x - m_viewportWF + 0.5f) * m_invViewportW,
                       ((float)p1.y - m_viewportHF + 0.5f) * -m_invViewportH,
                       v.z));
        UV(((VIRFontSlot *)(f->m_slots + off))->m_uv[3]);
        Vertex(VIVect3(((float)p0.x - m_viewportWF + 0.5f) * m_invViewportW,
                       ((float)p1.y - m_viewportHF + 0.5f) * -m_invViewportH,
                       v.z));
        EndTriFan();
        p0.x = p1.x + 1;
    }
    Restore2D();
    return 0;
}

int VIRaster::CalcCenteredStringScreenRect(int font, const VIVect3 &at,
                                           const char *s, VIRect &r)
{
    VIVect4 v;
    VIVect2I q;
    VIRFont *f = m_fonts.Value(font);
    v.Set(at * *(VIMatrix44 *)VI_SPR(0x70000000));
    v.Set(v * *(VIMatrix44 *)VI_SPR(0x70000040));
    float inv = 1.0f / v.w;
    v.x *= inv;
    v.y *= inv;
    v.z *= inv;
    v.w *= inv;
    float hw = (float)(m_screenW >> 1);
    float hh = (float)(m_screenH >> 1);
    q.x = (int)(v.x * hw + hw);
    q.y = (int)(-v.y * hh + hh);
    int w = f->GetStringWidth(s, 1);
    r.min = q;
    r.min.x -= w >> 1;
    r.min.y -= f->m_charHeight >> 1;
    // max is a COPY of min: through operator='s reference parameter cse does
    // not forward the stores, so both += re-read r.max (reload_cse turns the
    // .y read into the target's `move v1,a1`) -- and those reads are the refs
    // that put r in s0 ahead of this and &v.
    r.max = r.min;
    r.max.x += w;
    r.max.y += f->m_charHeight;
    return 0;
}

// --------------------------------------------------------- default state --

int VIRaster::CreateDefaultResources()
{
    VIColor32 c;
    m_solidFillMaterial = CreateMaterial(1);
    if (m_solidFillMaterial < 0)
        return -1;
    Material(m_solidFillMaterial)->InitSolidFill();
    m_solidFillUIMaterial = CreateMaterial(1);
    if (m_solidFillUIMaterial < 0)
        return -1;
    Material(m_solidFillUIMaterial)->InitSolidFill();
    m_solidFillUIAVGMaterial = CreateMaterial(1);
    if (m_solidFillUIAVGMaterial < 0)
        return -1;
    VIMaterial *m = Material(m_solidFillUIAVGMaterial);
    m->InitSolidFill();
    m->SetLayerBlendMode(0, VIAlphaBlendMode3);
    m_wireframeMaterial = CreateMaterial(1);
    if (m_wireframeMaterial < 0)
        return -1;
    m = Material(m_wireframeMaterial);
    m->InitSolidFill();
    m->SetLayerFillType(0, VIMaterialFillTypeSolid);
    m_portalMaterial = CreateMaterial(1);
    if (m_portalMaterial < 0)
        return -1;
    c.Set(255, 255, 255, 200);
    m = Material(m_portalMaterial);
    m->InitSolidFill();
    m->SetLayerBlendMode(0, VIAlphaBlendMode3);
    m->SetLayerColor(0, c);
    m_blitMaterial = CreateMaterial(1);
    if (m_blitMaterial < 0)
        return -1;
    c.Set(255, 255, 255, 255);
    m = Material(m_blitMaterial);
    m->InitSolidFill();
    m->SetLayerColor(0, c);
    m->SetLayerMagFilter(0, 0);
    m->SetZTest(0);
    m->SetZWrite(0);
    return 0;
}

// The per-frame uniform block VU1's raster microprogram reads: the three
// matrices CalcMatrices left in SPR, then the lighting -- sun direction (with
// y and z exchanged), point light and its reciprocal range, the clip flag
// and the two SPR constants, sun, point and ambient colours each zeroed when
// disabled, the fog and the static lighting -- and an MSCAL of the init
// entry point to latch it.
int VIRaster::UploadMatrices(_sceDmaTag **tag)
{
    char *p = (char *)*tag;
    ((_sceDmaTag *)p)->m_id = 16;
    (*tag)->m_addr = 0;
    (*tag)->m_qwc = 23;
    char *base = (char *)*tag;
    *tag = (_sceDmaTag *)(base + 384);
    ((unsigned int *)base)[4] = 0;
    p = base + 16;
    ((unsigned int *)p)[2] = 0x13000000;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[3] = 0x6C150000;
    sceVu0CopyMatrix(*(VIMatrix44 *)(base + 32), *(VIMatrix44 *)VI_SPR(0x70000140));
    sceVu0CopyMatrix(*(VIMatrix44 *)(base + 96), *(VIMatrix44 *)VI_SPR(0x70000080));
    sceVu0CopyMatrix(*(VIMatrix44 *)(base + 160), *(VIMatrix44 *)VI_SPR(0x700000C0));
    p = base + 224;
    ((float *)p)[0] = *(float *)VI_SPR(0x70000180);
    ((float *)p)[1] = -*(float *)VI_SPR(0x70000188);
    ((float *)p)[2] = *(float *)VI_SPR(0x70000184);
    ((float *)p)[3] = 0.0f;
    p = base + 240;
    ((float *)p)[0] = *(float *)VI_SPR(0x70000190);
    ((float *)p)[1] = *(float *)VI_SPR(0x70000194);
    ((float *)p)[2] = *(float *)VI_SPR(0x70000198);
    float range = 1.0f;
    if (*(unsigned char *)VI_SPR(0x70000222))
        range = 1.0f / *(float *)VI_SPR(0x700001CC);
    ((float *)p)[3] = range;
    p = base + 256;
    ((int *)p)[0] = m_p45C8;
    ((float *)p)[1] = *(float *)VI_SPR(0x7000022C);
    ((float *)p)[2] = 0.0f;
    ((float *)p)[3] = *(float *)VI_SPR(0x70000228);
    p = base + 272;
    if (*(unsigned char *)VI_SPR(0x70000221))
    {
        ((float *)p)[0] = *(float *)VI_SPR(0x700001AC);
        ((float *)p)[1] = *(float *)VI_SPR(0x700001B0);
        ((float *)p)[2] = *(float *)VI_SPR(0x700001B4);
        ((float *)p)[3] = *(float *)VI_SPR(0x700001B8);
    }
    else
    {
        ((float *)p)[0] = 0.0f;
        ((float *)p)[1] = 0.0f;
        ((float *)p)[2] = 0.0f;
        ((float *)p)[3] = 0.0f;
    }
    p += 16;
    if (*(unsigned char *)VI_SPR(0x70000222))
    {
        VIRasterPointLight *l = (VIRasterPointLight *)VI_SPR(0x700001C0);
        ((float *)p)[0] = l->m_color.r;
        ((float *)p)[1] = l->m_color.g;
        ((float *)p)[2] = l->m_color.b;
        ((float *)p)[3] = 0.0f;
    }
    else
    {
        ((float *)p)[0] = 0.0f;
        ((float *)p)[1] = 0.0f;
        ((float *)p)[2] = 0.0f;
        ((float *)p)[3] = 0.0f;
    }
    p += 16;
    if (*(unsigned char *)VI_SPR(0x70000220))
    {
        ((float *)p)[0] = *(float *)VI_SPR(0x700001E0);
        ((float *)p)[1] = *(float *)VI_SPR(0x700001E4);
        ((float *)p)[2] = *(float *)VI_SPR(0x700001E8);
        ((float *)p)[3] = *(float *)VI_SPR(0x700001EC);
    }
    else
    {
        ((float *)p)[0] = 0.0f;
        ((float *)p)[1] = 0.0f;
        ((float *)p)[2] = 0.0f;
        ((float *)p)[3] = 0.0f;
    }
    p += 16;
    // Through a STRUCT member.  `((float *)p)[0]` folds to `*(float *)p`, and
    // expr.c sets MEM_IN_STRUCT_P on an INDIRECT_REF only when its address is
    // a PLUS_EXPR, so true_dependence finds the RInitOffset load (a scalar at a
    // fixed lo_sum address) in conflict and holds it two cycles behind this
    // store.  An aggregate member at a varying address cannot alias it.
    ((VIQWord *)p)->m_f[0] = *(float *)VI_SPR(0x70000210);
    unsigned int mscal = RInitOffset | 0x14000000;
    ((float *)p)[1] = *(float *)VI_SPR(0x70000214);
    ((float *)p)[2] = *(float *)VI_SPR(0x70000218);
    ((float *)p)[3] = *(float *)VI_SPR(0x7000021C);
    p += 16;
    ((float *)p)[0] = *(float *)VI_SPR(0x700001F0);
    ((float *)p)[1] = *(float *)VI_SPR(0x700001F4);
    ((float *)p)[2] = *(float *)VI_SPR(0x700001F8);
    ((float *)p)[3] = *(float *)VI_SPR(0x700001FC);
    p += 16;
    ((float *)p)[0] = *(float *)VI_SPR(0x70000200);
    ((float *)p)[1] = *(float *)VI_SPR(0x70000204);
    ((float *)p)[2] = *(float *)VI_SPR(0x70000208);
    ((float *)p)[3] = *(float *)VI_SPR(0x7000020C);
    p += 16;
    ((unsigned int *)p)[3] = mscal;
    ((unsigned int *)p)[1] = 0;
    ((unsigned int *)p)[2] = 0;
    ((unsigned int *)p)[0] = 0;
    m_dirtyLights = 0;
    *(unsigned char *)VI_SPR(0x70000224) = 0;
    *(unsigned char *)VI_SPR(0x70000225) = 0;
    *(unsigned char *)VI_SPR(0x70000226) = 0;
    *(unsigned char *)VI_SPR(0x70000227) = 0;
    return 0;
}

// DIMX, its own packet, and only when the dither state actually changes: the
// standard 4x4 ordered matrix when on, a flat one when off.
int VIRaster::SetGSDitherMatrix(int on, _sceDmaTag **tag)
{
    if (m_p459C != on)
    {
        (*tag)->m_id = 16;
        (*tag)->m_addr = 0;
        (*tag)->m_qwc = 3;
        char *base = (char *)*tag;
        *tag = (_sceDmaTag *)(base + 64);
        ((unsigned int *)base)[4] = 0;
        char *p = base + 16;
        ((unsigned int *)p)[2] = 0x10000000;
        ((unsigned int *)p)[1] = 0;
        ((unsigned int *)p)[3] = 0x50000002;
        ((long *)base)[4] = 0x1000000000008001;
        ((long *)base)[5] = 14;
        p = base + 48;
        if (on == 1)
            ((long *)p)[0] = 0x6071243571603524;
        else
            ((long *)p)[0] = 0x2031312020313120;
        ((long *)p)[1] = 68;
        m_p459C = on;
    }
    return 0;
}

// SCISSOR_1 and SCISSOR_2 to the viewport, one A+D packet.
int VIRaster::SetGSScissoring()
{
    VIVect2I a;
    VIVect2I b;
    a.x = m_viewportX;
    a.y = m_viewportY;
    b.x = m_viewportX + m_viewportW - 1;
    b.y = m_viewportY + m_viewportH - 1;
    m_dmaTag->m_id = 16;
    m_dmaTag->m_addr = 0;
    m_dmaTag->m_qwc = 4;
    char *base = (char *)m_dmaTag;
    m_dmaTag = (_sceDmaTag *)(base + 80);
    ((unsigned int *)base)[4] = 0;
    char *p = base + 16;
    ((unsigned int *)p)[2] = 0x10000000;
    ((unsigned int *)p)[3] = 0x50000003;
    ((unsigned int *)p)[1] = 0;
    ((long *)base)[4] = 0x1000000000008002;
    ((long *)base)[5] = 14;
    ((long *)base)[6] = (long)a.x | ((long)b.x << 16) | ((long)a.y << 32) |
                        ((long)b.y << 48);
    ((long *)base)[7] = 64;
    ((long *)base)[8] = (long)a.x | ((long)b.x << 16) | ((long)a.y << 32) |
                        ((long)b.y << 48);
    ((long *)base)[9] = 65;
    return 0;
}

// ------------------------------------------------------------- billboards --

// Opens a batch of at most 80 billboards and hands the caller the run of
// VIBillboard slots to fill in place.  When the matrices are dirty the whole
// billboard microprogram's constant block goes out first: the model-view, the
// projection taken through to screen space, the model-view-projection, a GIF
// tag template, the 28 corner UVs of the seven sprite-sheet layouts, a white
// and the screen offset with the alpha scale.
int VIRaster::BeginBillboards(VIBillboard **bb, int count)
{
    // A LOCAL, not the parameter clamped in place: only the copy gives the
    // `move`, then `movz` on the clamp constant.
    int n = count;
    if (n > 80)
        n = 80;
    char *p;
    VIMaterial *m = Material(m_curMaterial);
    if (n > 0)
    {
        if (m_p4C4C != 2)
            UploadBillboardMicro(&m_dmaTag);
        if (m_dirtyMatrices)
        {
            CalcMatrices();
            VIMatrix44 mat;
            p = (char *)m_dmaTag;
            p += 16;
            ((unsigned int *)p)[2] = 0x13000000;
            ((unsigned int *)p)[3] = 0x6C0C0000;
            ((unsigned int *)p)[0] = 0;
            ((unsigned int *)p)[1] = 0;
            p += 16;
            void *dst = p;
            memcpy(dst, (void *)VI_SPR(0x70000000), 64);
            p += 64;
            mat.Mul(*(VIMatrix44 *)VI_SPR(0x70000040), *(VIMatrix44 *)VI_SPR(0x700000C0));
            ((VIRasterQW *)p)[0] = ((VIRasterQW *)&mat)[0];
            ((VIRasterQW *)p)[1] = ((VIRasterQW *)&mat)[1];
            ((VIRasterQW *)p)[2] = ((VIRasterQW *)&mat)[2];
            ((VIRasterQW *)p)[3] = ((VIRasterQW *)&mat)[3];
            p += 64;
            dst = p;
            memcpy(dst, (void *)VI_SPR(0x70000080), 64);
            p += 64;
            ((unsigned int *)p)[0] = 0;
            ((unsigned int *)p)[1] = 0;
            ((unsigned int *)p)[3] = 0x6C040010;
            ((unsigned int *)p)[2] = 0;
            p += 16;
            ((long *)p)[0] = 0xB000000000008000;
            ((long *)p)[1] = 0x0000044242424421;
            p += 16;
            ((long *)p)[0] = 0x202A400000008001;
            ((long *)p)[1] = 66;
            p += 16;
            ((unsigned int *)p)[2] = 0x3F800000;
            ((unsigned int *)p)[0] = 0;
            ((unsigned int *)p)[1] = 0;
            ((unsigned int *)p)[3] = 0;
            p += 16;
            ((unsigned int *)p)[0] = 8192;
            ((unsigned int *)p)[1] = 8192;
            ((unsigned int *)p)[2] = 8192;
            ((unsigned int *)p)[3] = 0;
            p += 16;
            ((unsigned int *)p)[0] = 0;
            ((unsigned int *)p)[1] = 0;
            ((unsigned int *)p)[3] = 0x6C1C0020;
            ((unsigned int *)p)[2] = 0;
            p += 16;
    ((long *)p)[0] = 0x3F80000000000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F8000003F800000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F800000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F00000000000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F0000003F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F0000003F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F0000003F800000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F800000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F80000000000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F00000000000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F8000003F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F0000003F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F8000003F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F0000003F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F8000003F800000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F0000003F800000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F80000000000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F8000003F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F8000003F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F000000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F8000003F800000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
    ((long *)p)[0] = 0x3F800000;
    ((long *)p)[1] = 0x3F800000;
    p += 16;
            ((unsigned int *)p)[3] = 0x6C02000C;
            ((unsigned int *)p)[0] = 0;
            ((unsigned int *)p)[1] = 0;
            ((unsigned int *)p)[2] = 0;
            p += 16;
            ((float *)p)[0] = 1.0f;
            ((float *)p)[1] = 1.0f;
            ((float *)p)[2] = 1.0f;
            ((float *)p)[3] = 1.0f;
            p += 16;
            if (m->m_layers[0].m_blendMode == 0)
            {
                ((float *)p)[0] = m_aspectX;
                ((float *)p)[1] = m_aspectY;
                ((float *)p)[2] = 128.0f;
                ((float *)p)[3] = 128.0f;
                // The step in both arms: cross-jumped with the stores after
                // scheduling, which is what orders the join's quadword.
                p += 16;
            }
            else
            {
                ((float *)p)[0] = m_aspectX;
                ((float *)p)[1] = m_aspectY;
                ((float *)p)[2] = 128.0f;
                ((float *)p)[3] = 64.0f;
                p += 16;
            }
            ((unsigned int *)p)[0] = 0;
            ((unsigned int *)p)[3] = 0x14000000;
            ((unsigned int *)p)[1] = 0;
            ((unsigned int *)p)[2] = 0;
            int qwc = (p - (char *)m_dmaTag) >> 4;
            m_dmaTag->m_id = 16;
            m_dmaTag->m_addr = 0;
            m_dmaTag->m_qwc = qwc;
            int bytes = (qwc << 4) + 16;
            m_dmaTag = (_sceDmaTag *)((char *)m_dmaTag + bytes);
            FlushDMABufferIfFull();
            m_dirtyMatrices = 0;
        }
        p = (char *)m_dmaTag;
        p += 16;
        ((unsigned int *)p)[3] = ((n * 3 + 1) << 16) | 0x6C008000;
        ((unsigned int *)p)[2] = 0x13000000;
        ((unsigned int *)p)[0] = 0;
        ((unsigned int *)p)[1] = 0;
        p += 16;
        ((unsigned int *)p)[0] = n;
        ((unsigned int *)p)[1] = 0;
        ((unsigned int *)p)[2] = 0;
        ((unsigned int *)p)[3] = 0;
        p += 16;
        *bb = (VIBillboard *)p;
    }
    else
    {
        *bb = 0;
        n = 0;
    }
    m_numBillboards = n;
    return n;
}

// -------------------------------------------------------- the tessellator --

VIRasterTess::VIRasterTess()
{
}

// ------------------------------------------------------ prim-buffer draws --

// Each material run of the buffer is REFerenced straight out of the prim
// buffer's own vertex block, behind its material's GS packet.  A material that
// depth-sorts is drawn on pass bit 0 and one that does not on bit 1, and the
// return value says whether any of the second kind was seen.
int VIRaster::DrawPrimBufferVUNC(int prim, int pal, unsigned int flags)
{
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    int r = 0;
    FlushDMABufferIfFull();
    if (m_dirtyLights)
    {
        CalcMatrices();
        UploadMatrices(&m_dmaTag);
    }
    for (int i = 0; i < pb->m_numMaterials; i++)
    {
        int mat = pb->m_materials[i].m_matIdx;
        mat = m_materialPals.Value(pal)->m_mats[mat];
        int ds = MaterialHasDepthSort(mat);
        if (!ds)
            r = 1;
        if (ds ? (flags & 1) : (flags & 2))
        {
            AddMaterial(mat, &m_dmaTag);
            int addr = (pb->m_materials[i].m_firstVert << 4) + (int)pb->m_verts;
            int qwc = pb->m_materials[i].m_numVerts;
            m_dmaTag->m_id = VI_DMA_STATIC(48);
            m_dmaTag->m_addr = addr;
            m_dmaTag->m_qwc = qwc;
            m_dmaTag++;
        }
    }
    FlushDMABufferIfFull();
    return r;
}

// The packed forms put the buffer's four unpack scales in SPR first.  With
// the point light on, the raster's clip-and-light flag says whether the light
// sphere -- taken into object space through the inverse model-view, whose
// first row's length is the scale -- reaches the buffer's box at all.
int VIRaster::DrawPrimBufferPackVUNC(int prim, int pal, unsigned int flags)
{
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    int r = 0;
    FlushDMABufferIfFull();
    ((VIVect4 *)VI_SPR(0x70000210))->Set(pb->m_scale);
    if (m_dirtyLights)
        CalcMatrices();
    m_p45C8 = 0;
    if (*(unsigned char *)VI_SPR(0x70000222))
    {
        VISphere s;
        float x = *(float *)VI_SPR(0x70000100);
        float y = *(float *)VI_SPR(0x70000104);
        float z = *(float *)VI_SPR(0x70000108);
        float len = sqrtf(x * x + y * y + z * z);
        s.r = *(float *)VI_SPR(0x700001CC) * (1.0f / len);
        s.c.x = *(float *)VI_SPR(0x70000190);
        s.c.y = *(float *)VI_SPR(0x70000194);
        s.c.z = *(float *)VI_SPR(0x70000198);
        if (pb->m_bbox.Intersects(s))
            m_p45C8 = 1;
    }
    UploadMatrices(&m_dmaTag);
    for (int i = 0; i < pb->m_numMaterials; i++)
    {
        int mat = pb->m_materials[i].m_matIdx;
        mat = m_materialPals.Value(pal)->m_mats[mat];
        int ds = MaterialHasDepthSort(mat);
        if (!ds)
            r = 1;
        if (ds ? (flags & 1) : (flags & 2))
        {
            AddMaterial(mat, &m_dmaTag);
            int addr = (pb->m_materials[i].m_firstVert << 4) + (int)pb->m_verts;
            int qwc = pb->m_materials[i].m_numVerts;
            m_dmaTag->m_id = VI_DMA_STATIC(48);
            m_dmaTag->m_addr = addr;
            m_dmaTag->m_qwc = qwc;
            m_dmaTag++;
        }
    }
    FlushDMABufferIfFull();
    return r;
}

int VIRaster::DrawPrimBufferPackVUNCG(int prim, int pal, unsigned int flags)
{
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    int r = 0;
    FlushDMABufferIfFull();
    ((VIVect4 *)VI_SPR(0x70000210))->Set(pb->m_scale);
    if (m_dirtyLights)
        CalcMatrices();
    UploadMatrices(&m_dmaTag);
    if (m_dirtyPreTrans)
        UploadPreTrans(&m_dmaTag);
    for (int i = 0; i < pb->m_numMaterials; i++)
    {
        int mat = pb->m_materials[i].m_matIdx;
        mat = m_materialPals.Value(pal)->m_mats[mat];
        int ds = MaterialHasDepthSort(mat);
        if (!ds)
            r = 1;
        if (ds ? (flags & 1) : (flags & 2))
        {
            AddMaterial(mat, &m_dmaTag);
            int addr = (pb->m_materials[i].m_firstVert << 4) + (int)pb->m_verts;
            int qwc = pb->m_materials[i].m_numVerts;
            m_dmaTag->m_id = VI_DMA_STATIC(48);
            m_dmaTag->m_addr = addr;
            m_dmaTag->m_qwc = qwc;
            m_dmaTag++;
        }
    }
    FlushDMABufferIfFull();
    if (DrawPrimBufferTessPackVUNCG(prim, pal, flags))
        r = 1;
    return r;
}

// The point light's second pass: every triangle of a tessellated material
// that the light's box reaches is cut by VIRasterTess into pieces small
// enough for per-vertex lighting to show the falloff, and each resulting
// strip goes out as its own unpacked packet, all four attribute rows per
// vertex, closed by an MSCAL of the strip program.
// The light's sphere is built round a by-value sum: the target's frame puts
// the sphere at sp+80 and the sum's temporary at sp+96, which is the order a
// temporary made while constructing a declared object gets its slot.
//
// Phase 61 wave 5 (336 -> 0), each shape read off a pass, not guessed:
// - BoxAround is an inline taking the box BY REFERENCE: `box` sits at sp+0,
//   and an inline's pointer argument that is the bare frame base is copied
//   into a pseudo (sp+K is substituted as a constant), so &box.max becomes
//   the same pseudo as the ctor's (s2) and the max stores through it
//   invalidate cse's memory -- the target's reloads of s.c.z and of max.
// - One scale load per vertex (TessPos's by-value float); b and c share the
//   `sc` pointer, so loop.c hoists it out of the k loop, while the j loop's
//   single use is folded back into an absolute `lui at` by combine.
// - Plain `for` loops: gcse's PRE hoists each latch `+1` into both arms,
//   which is the target's early `addiu sN,sN,1` + spill in every arm.
// - i, prims, k are declared together at the top: the PRE reaching registers
//   are made in gcse hash-bucket order ((13258 + N + K) mod S), which is the
//   spill-slot order 180/184/188/192, and `prims++` in the BODY (not the for
//   increment) puts prims+1 ahead of k+1 on the insert edge, so sched1 kills
//   prims before c is born and the two share s4.
// - The vertex walk keeps b and c and rotates a/b/c at the top of the j loop,
//   like v[]; a prim is a 32-byte header, numVerts * 32, a 16-byte tail
//   (the earlier `a = verts` spelling skipped 32 bytes too few per prim).
// - `strips` is re-read from the member inside the n loop (loop.c hoists it)
//   and the m loop's bound re-indexes it, which gcse turns into the target's
//   `move v1,t2` copy; m_start is read off m_tess directly.
// - Each qword's stores in index order: sched1 issues the store that kills
//   the walker (the last one) first, giving the target's [0],[3],[1],[2].
static inline VIVect3 operator+(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline void BoxAround(VIBBox &b, const VISphere &s)
{
    b.min = s.c;
    b.min -= s.r;
    b.max = s.c;
    b.max += s.r;
}

static inline void TessPos(VIVect3 &o, const VIVect3S16 &p, float s)
{
    o.x = (float)p.x * s;
    o.y = (float)p.y * s;
    o.z = (float)p.z * s;
}

int VIRaster::DrawPrimBufferTessPackVUNCG(int prim, int pal, unsigned int flags)
{
    int i, prims, k;
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    if (*(unsigned char *)VI_SPR(0x70000222) != 0 && m_tessellate != 0)
    {
        VIBBox box;
        VIVect3 v[3];
        ((VIMatrix44 *)VI_SPR(0x70000100))->Mul(*(VIVect3 *)VI_SPR(0x700001C0), v[0]);
        {
            VISphere s(v[0] + m_preTransAt, *(float *)VI_SPR(0x700001CC));
            BoxAround(box, s);
        }
        if (box.Intersects(pb->m_bbox))
        {
            VIBBox lbox;
            VIBBox tbox;
            VISphere ls(v[0], *(float *)VI_SPR(0x700001CC));
            BoxAround(lbox, ls);
            char *verts = pb->m_verts;
            prims = 0;
            for (i = 0; i < pb->m_numMaterials; i++)
            {
                int mat = pb->m_materials[i].m_matIdx;
                mat = m_materialPals.Value(pal)->m_mats[mat];
                int ds = MaterialHasDepthSort(mat);
                if ((((VIMaterial *)mat)->m_flags & 0x20) &&
                    (ds ? (flags & 1) : (flags & 2)))
                {
                    AddMaterial(mat, &m_dmaTag);
                    for (k = 0; k < pb->m_materials[i].m_numPrims; k++)
                    {
                        if (box.Intersects(pb->m_prims[prims].m_bbox))
                        {
                            float *sc = (float *)VI_SPR(0x70000210);
                            verts += 32;
                            VIPrimVertexPackVUNCG *b = (VIPrimVertexPackVUNCG *)verts;
                            verts += 32;
                            VIPrimVertexPackVUNCG *c = (VIPrimVertexPackVUNCG *)verts;
                            verts += 32;
                            TessPos(v[1], b->m_pos, *sc);
                            v[1] += m_preTrans[b->m_preTrans];
                            TessPos(v[2], c->m_pos, *sc);
                            v[2] += m_preTrans[c->m_preTrans];
                            for (int j = 2; j < pb->m_prims[prims].m_numVerts; j++)
                            {
                                VIPrimVertexPackVUNCG *a = b;
                                b = c;
                                c = (VIPrimVertexPackVUNCG *)verts;
                                verts += 32;
                                v[0] = v[1];
                                v[1] = v[2];
                                TessPos(v[2], c->m_pos, *(float *)VI_SPR(0x70000210));
                                v[2] += m_preTrans[c->m_preTrans];
                                tbox.Init(3, v);
                                if (lbox.Intersects(tbox))
                                {
                                    if (!(j & 1))
                                        TessTriangle(v[0], v[1], v[2], a, b, c);
                                    else
                                        TessTriangle(v[2], v[1], v[0], c, b, a);
                                    for (int n = 0; n < m_tess.m_numStrips; n++)
                                    {
                                        VIRasterTessStrip *strips = m_tess.m_strips;
                                        VIRasterTessStrip *st = &strips[n];
                                        int qwc = (st->m_count << 2) + 3;
                                        m_dmaTag->m_id = 16;
                                        m_dmaTag->m_addr = 0;
                                        m_dmaTag->m_qwc = qwc;
                                        int bytes = (qwc << 4) + 16;
                                        char *q = (char *)m_dmaTag;
                                        m_dmaTag = (_sceDmaTag *)(q + bytes);
                                        VIVect4 *o = (VIVect4 *)(q + 16);
                                        ((unsigned int *)o)[0] = 0;
                                        ((unsigned int *)o)[1] = 0;
                                        ((unsigned int *)o)[2] = 0;
                                        ((unsigned int *)o)[3] =
                                            (((st->m_count << 2) + 1) << 16) | 0x6C008000;
                                        o++;
                                        ((unsigned int *)o)[0] = st->m_count;
                                        ((unsigned int *)o)[1] = 0;
                                        ((unsigned int *)o)[2] = 0;
                                        ((unsigned int *)o)[3] = 0;
                                        o++;
                                        VITessVertex *t = &m_tess.m_out[m_tess.m_strips[n].m_start];
                                        for (int m = 0; m < strips[n].m_count; m++)
                                        {
                                            o->x = t->m_pos.x;
                                            o->y = t->m_pos.y;
                                            o->z = t->m_pos.z;
                                            o->w = 1.0f;
                                            o++;
                                            o->x = t->m_uv.x;
                                            o->y = t->m_uv.y;
                                            o->z = 1.0f;
                                            o->w = 0.0f;
                                            o++;
                                            o->x = t->m_normal.x;
                                            o->y = t->m_normal.y;
                                            o->z = t->m_normal.z;
                                            o->w = 0.0f;
                                            o++;
                                            o->x = t->m_color.r;
                                            o->y = t->m_color.g;
                                            o->z = t->m_color.b;
                                            o->w = t->m_color.a;
                                            o++;
                                            t++;
                                        }
                                        unsigned int mscal = RStrip5Offset | 0x14000000;
                                        ((unsigned int *)o)[0] = 0;
                                        ((unsigned int *)o)[1] = 0;
                                        ((unsigned int *)o)[2] = 0;
                                        ((unsigned int *)o)[3] = mscal;
                                        FlushDMABufferIfFull();
                                    }
                                }
                            }
                            verts += 16;
                        }
                        else
                        {
                            verts += (pb->m_prims[prims].m_numVerts << 5) + 48;
                        }
                        prims++;
                    }
                }
                else
                {
                    prims += pb->m_materials[i].m_numPrims;
                    verts += pb->m_materials[i].m_numVerts << 4;
                }
            }
        }
    }
    return 0;
}

static inline void TessNormal(VIVect3 &o, const VIVect3S8 &n, float s)
{
    o.x = (float)n.x * s;
    o.y = (float)n.y * s;
    o.z = (float)n.z * s;
}

// The UV goes through a helper like the normal and the colour: the scale is
// ONE by-value load, and through the unflagged reference the .x store is
// ordered before the .y load (the third vertex's late `lh 14`).
static inline void TessUV(VIVect2 &o, const VIVect2S16 &n, float s)
{
    o.x = (float)n.x * s;
    o.y = (float)n.y * s;
}

static inline void TessColor(VIColor32F &o, const VIVect4S8 &c, float s)
{
    o.r = (float)c.x * s;
    o.g = (float)c.y * s;
    o.b = (float)c.z * s;
    o.a = (float)c.w * s;
}

int VIRaster::TessTriangle(VIVect3 &a, VIVect3 &b, VIVect3 &c,
                           VIPrimVertexPackVUNCG *va, VIPrimVertexPackVUNCG *vb,
                           VIPrimVertexPackVUNCG *vc)
{
    VITessVertex t[3];
    float *uvScale = (float *)VI_SPR(0x70000214);
    float *nScale = (float *)VI_SPR(0x70000218);
    float *cScale = (float *)VI_SPR(0x7000021C);
    t[0].m_pos = a;
    TessUV(t[0].m_uv, va->m_uv, *uvScale);
    TessNormal(t[0].m_normal, va->m_normal, *nScale);
    TessColor(t[0].m_color, va->m_color, *cScale);
    t[0].m_pos.x += t[0].m_normal.x * 0.01f;
    t[0].m_pos.y += t[0].m_normal.y * 0.01f;
    t[0].m_pos.z += t[0].m_normal.z * 0.01f;
    t[1].m_pos = b;
    TessUV(t[1].m_uv, vb->m_uv, *uvScale);
    TessNormal(t[1].m_normal, vb->m_normal, *nScale);
    TessColor(t[1].m_color, vb->m_color, *cScale);
    t[1].m_pos.x += t[1].m_normal.x * 0.01f;
    t[1].m_pos.y += t[1].m_normal.y * 0.01f;
    t[1].m_pos.z += t[1].m_normal.z * 0.01f;
    t[2].m_pos = c;
    TessUV(t[2].m_uv, vc->m_uv, *uvScale);
    TessNormal(t[2].m_normal, vc->m_normal, *nScale);
    TessColor(t[2].m_color, vc->m_color, *cScale);
    t[2].m_pos.x += t[2].m_normal.x * 0.01f;
    t[2].m_pos.y += t[2].m_normal.y * 0.01f;
    t[2].m_pos.z += t[2].m_normal.z * 0.01f;
    return m_tess.Tessellate(3, t);
}

int VIRaster::DrawPrimBufferPackVUNBW(int prim, int pal, unsigned int flags)
{
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    int r = 0;
    FlushDMABufferIfFull();
    ((VIVect4 *)VI_SPR(0x70000210))->Set(pb->m_scale);
    if (m_dirtyLights)
        CalcMatrices();
    UploadMatrices(&m_dmaTag);
    for (int i = 0; i < pb->m_numMaterials; i++)
    {
        int mat = pb->m_materials[i].m_matIdx;
        mat = m_materialPals.Value(pal)->m_mats[mat];
        int ds = MaterialHasDepthSort(mat);
        if (!ds)
            r = 1;
        if (ds ? (flags & 1) : (flags & 2))
        {
            AddMaterial(mat, &m_dmaTag);
            int addr = (pb->m_materials[i].m_firstVert << 4) + (int)pb->m_verts;
            int qwc = pb->m_materials[i].m_numVerts;
            m_dmaTag->m_id = VI_DMA_STATIC(48);
            m_dmaTag->m_addr = addr;
            m_dmaTag->m_qwc = qwc;
            m_dmaTag++;
        }
    }
    FlushDMABufferIfFull();
    return r;
}

int VIRaster::DrawPrimBufferPackVU(int prim, int pal, float f, unsigned int flags)
{
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    int r = 0;
    FlushDMABufferIfFull();
    *(float *)VI_SPR(0x70000228) = f;
    ((VIVect4 *)VI_SPR(0x70000210))->Set(pb->m_scale);
    if (m_dirtyLights)
        CalcMatrices();
    UploadMatrices(&m_dmaTag);
    for (int i = 0; i < pb->m_numMaterials; i++)
    {
        int mat = pb->m_materials[i].m_matIdx;
        mat = m_materialPals.Value(pal)->m_mats[mat];
        int ds = MaterialHasDepthSort(mat);
        if (!ds)
            r = 1;
        if (ds ? (flags & 1) : (flags & 2))
        {
            AddMaterial(mat, &m_dmaTag);
            int addr = (pb->m_materials[i].m_firstVert << 4) + (int)pb->m_verts;
            int qwc = pb->m_materials[i].m_numVerts;
            m_dmaTag->m_id = VI_DMA_STATIC(48);
            m_dmaTag->m_addr = addr;
            m_dmaTag->m_qwc = qwc;
            m_dmaTag++;
        }
    }
    FlushDMABufferIfFull();
    return r;
}

// The baked-colour form: the colour buffer carries its own pre-built chain
// per material, and each one is CALLed (DMAtag id 5) rather than REFerenced.
int VIRaster::DrawPrimBufferPackVUNC(int prim, int cbi, int pal,
                                     unsigned int flags)
{
    VIPrimBuffer *pb = m_primBuffers.Value(prim);
    VIColorBuffer *cb = m_colorBuffers.Value(cbi);
    int r = 0;
    FlushDMABufferIfFull();
    if (prim != cb->m_primBuffer)
        return -1;
    ((VIVect4 *)VI_SPR(0x70000210))->Set(pb->m_scale);
    if (m_dirtyLights)
        CalcMatrices();
    m_p45C8 = 0;
    if (*(unsigned char *)VI_SPR(0x70000222))
    {
        VISphere s;
        float x = *(float *)VI_SPR(0x70000100);
        float y = *(float *)VI_SPR(0x70000104);
        float z = *(float *)VI_SPR(0x70000108);
        float len = sqrtf(x * x + y * y + z * z);
        s.r = *(float *)VI_SPR(0x700001CC) * (1.0f / len);
        s.c.x = *(float *)VI_SPR(0x70000190);
        s.c.y = *(float *)VI_SPR(0x70000194);
        s.c.z = *(float *)VI_SPR(0x70000198);
        if (pb->m_bbox.Intersects(s))
            m_p45C8 = 1;
    }
    UploadMatrices(&m_dmaTag);
    for (int i = 0; i < cb->m_pad10; i++)
    {
        int mat = ((int *)(cb->m_headers + i * 8))[0];
        mat = m_materialPals.Value(pal)->m_mats[mat];
        int ds = MaterialHasDepthSort(mat);
        if (!ds)
            r = 1;
        if (ds ? (flags & 1) : (flags & 2))
        {
            AddMaterial(mat, &m_dmaTag);
            int addr = (((int *)(cb->m_headers + i * 8))[1] << 4) + (int)cb->m_verts;
            m_dmaTag->m_id = VI_DMA_STATIC(80);
            m_dmaTag->m_addr = addr;
            m_dmaTag->m_qwc = 0;
            m_dmaTag++;
        }
    }
    FlushDMABufferIfFull();
    return r;
}

// ------------------------------------------------------------- statistics --

extern "C" int printf(const char *f, ...);

// Bytes held per resource pool -- each object's own data plus its fixed
// size -- printed pool by pool, with the DMA double buffer's two 600 KB halves
// and the grand total last.
int VIRaster::DebugResources()
{
    int i;
    int a = 0;
    printf("Surfaces: %d\n", m_surfaces.Size());
    for (i = m_surfaces.Begin(); i != m_surfaces.End(); i = m_surfaces.Next(i))
        a += m_surfaces.Value(i)->m_dataSize;
    a += m_surfaces.Size() * 316;
    printf("Surfaces Allocations: %d\n", a);
    int total = a;
    printf("Materials: %d\n", m_materials.Size());
    a = m_materials.Size() * 480;
    printf("Material Allocations: %d\n", a);
    total += a;
    a = 0;
    printf("Prim Buffers: %d\n", m_primBuffers.Size());
    for (i = m_primBuffers.Begin(); i != m_primBuffers.End();
         i = m_primBuffers.Next(i))
        a += m_primBuffers.Value(i)->m_dataSize;
    a += m_primBuffers.Size() * 112;
    printf("Prim Buffer Allocations: %d\n", a);
    total += a;
    a = 0;
    printf("Color Buffers: %d\n", m_colorBuffers.Size());
    for (i = m_colorBuffers.Begin(); i != m_colorBuffers.End();
         i = m_colorBuffers.Next(i))
        a += m_colorBuffers.Value(i)->m_dataSize;
    a += m_colorBuffers.Size() * 56;
    printf("Color Buffer Allocations: %d\n", a);
    total += a;
    a = 0;
    printf("Material Palettes: %d\n", m_materialPals.Size());
    for (i = m_materialPals.Begin(); i != m_materialPals.End();
         i = m_materialPals.Next(i))
        a += m_materialPals.Value(i)->m_mats.m_size * 4;
    a += m_materialPals.Size() * 16;
    printf("Material Palette Allocations: %d\n", a);
    total += a;
    printf("Fonts: %d\n", m_fonts.Size());
    i = m_fonts.Begin();
    a = m_fonts.Size() * 88;
    for (; i != m_fonts.End(); i = m_fonts.Next(i))
        a += m_fonts.Value(i)->m_dataSize;
    printf("Font Allocations: %d\n", a);
    total += a;
    printf("Material LRU Entries: %d\n", m_materialLRU.Size());
    a = m_materialLRU.Size() * 24;
    a += m_materialLRU.Size() * 848;
    printf("Material LRU Allocations: %d\n", a);
    total += a;
    a = 0x12C000;
    printf("DMA Double Buffer Allocations: %d\n", 0x12C000);
    total += a;
    printf("Total Raster Allocations: %d\n", total);
    return total;
}

// --------------------------------------------------------------- the blits --

// A surface drawn 1:1 at a screen position as a four-vertex fan through the
// blit material: the material takes the surface and the blend mode (and is
// marked dirty when either changed since the last blit), the quad is written
// straight into the DMA chain one attribute row at a time, and the low two
// flag bits mirror the UVs in x and/or y.  Bit 2 turns dithering off.
int VIRaster::Blit(int id, unsigned int flags, const VIVect2I &at,
                   VIAlphaBlendMode m, float alpha)
{
    VIMaterial *mat = (VIMaterial *)m_blitMaterial;
    VISurface *s = m_surfaces.Value(id);
    VIVect2I p;
    VIColor32F c;
    VIVect3 n;
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    mat->m_layers[0].m_texture = id;
    mat->m_layers[0].m_blendMode = m;
    if (flags & 4)
        mat->SetDither(0);
    else
        mat->SetDither(1);
    if (id != m_blitSurface || m != m_blitBlend)
    {
        mat->m_dirty = 1;
        m_blitSurface = id;
        m_blitBlend = m;
    }
    if (AddMaterial(m_blitMaterial, &m_dmaTag) < 0)
        return -1;
    if (BlitBegin(&m_dmaTag) < 0)
        return -1;
    c.Set(1.0f, 1.0f, 1.0f, alpha);
    n.Set(0.0f, 0.0f, 0.0f);
    p.x = at.x;
    p.y = at.y;
    m_vtx[0].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[0].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[0].z = 0.0f;
    m_vtx[0].w = 1.0f;
    m_vtx[2].x = n.x;
    m_vtx[2].y = n.y;
    m_vtx[2].z = n.z;
    m_vtx[2].w = 0.0f;
    m_vtx[3].x = c.r;
    m_vtx[3].y = c.g;
    m_vtx[3].z = c.b;
    m_vtx[3].w = c.a;
    m_numVerts++;
    p.x += s->m_width;
    m_vtx[4].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[4].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[4].z = 0.0f;
    m_vtx[4].w = 1.0f;
    m_vtx[6].x = n.x;
    m_vtx[6].y = n.y;
    m_vtx[6].z = n.z;
    m_vtx[6].w = 0.0f;
    m_vtx[7].x = c.r;
    m_vtx[7].y = c.g;
    m_vtx[7].z = c.b;
    m_vtx[7].w = c.a;
    m_numVerts++;
    p.y += s->m_height;
    m_vtx[8].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[8].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[8].z = 0.0f;
    m_vtx[8].w = 1.0f;
    m_vtx[10].x = n.x;
    m_vtx[10].y = n.y;
    m_vtx[10].z = n.z;
    m_vtx[10].w = 0.0f;
    m_vtx[11].x = c.r;
    m_vtx[11].y = c.g;
    m_vtx[11].z = c.b;
    m_vtx[11].w = c.a;
    m_numVerts++;
    p.x = at.x;
    m_vtx[12].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[12].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[12].z = 0.0f;
    m_vtx[12].w = 1.0f;
    m_vtx[14].x = n.x;
    m_vtx[14].y = n.y;
    m_vtx[14].z = n.z;
    m_vtx[14].w = 0.0f;
    m_vtx[15].x = c.r;
    m_vtx[15].y = c.g;
    m_vtx[15].z = c.b;
    m_vtx[15].w = c.a;
    m_numVerts++;
    switch (flags & 3)
    {
    case 0:
        m_vtx[1].x = 0.0f;
        m_vtx[1].y = 0.0f;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = 1.0f;
        m_vtx[5].y = 0.0f;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = 1.0f;
        m_vtx[9].y = 1.0f;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = 0.0f;
        m_vtx[13].y = 1.0f;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    case 1:
        m_vtx[1].x = 1.0f;
        m_vtx[1].y = 0.0f;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = 0.0f;
        m_vtx[5].y = 0.0f;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = 0.0f;
        m_vtx[9].y = 1.0f;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = 1.0f;
        m_vtx[13].y = 1.0f;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    case 2:
        m_vtx[1].x = 0.0f;
        m_vtx[1].y = 1.0f;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = 1.0f;
        m_vtx[5].y = 1.0f;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = 1.0f;
        m_vtx[9].y = 0.0f;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = 0.0f;
        m_vtx[13].y = 0.0f;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    case 3:
        m_vtx[1].x = 1.0f;
        m_vtx[1].y = 1.0f;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = 0.0f;
        m_vtx[5].y = 1.0f;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = 0.0f;
        m_vtx[9].y = 0.0f;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = 1.0f;
        m_vtx[13].y = 0.0f;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    }
    m_vtx += 16;
    if (BlitEnd(&m_dmaTag) < 0)
        return -1;
    mat->m_layers[0].m_texture = -1;
    FlushDMABufferIfFull();
    return 0;
}

// A sub-rectangle of the surface: `a` is its texel origin and `b` its size,
// the UVs are those texels over the surface's own size, and the quad is `b`
// big on screen.
int VIRaster::Blit(int id, unsigned int flags, const VIVect2I &at,
                   const VIVect2I &a, const VIVect2I &b, VIAlphaBlendMode m,
                   float alpha)
{
    VIMaterial *mat = (VIMaterial *)m_blitMaterial;
    VISurface *s = m_surfaces.Value(id);
    VIVect2I p;
    VIColor32F c;
    VIVect3 n;
    VIVect2 inv;
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    mat->m_layers[0].m_texture = id;
    mat->m_layers[0].m_blendMode = m;
    if (flags & 4)
        mat->SetDither(0);
    else
        mat->SetDither(1);
    if (id != m_blitSurface || m != m_blitBlend)
    {
        mat->m_dirty = 1;
        m_blitSurface = id;
        m_blitBlend = m;
    }
    if (AddMaterial(m_blitMaterial, &m_dmaTag) < 0)
        return -1;
    if (BlitBegin(&m_dmaTag) < 0)
        return -1;
    c.Set(1.0f, 1.0f, 1.0f, alpha);
    n.Set(0.0f, 0.0f, 0.0f);
    inv.x = 1.0f / (float)s->m_width;
    inv.y = 1.0f / (float)s->m_height;
    p.x = at.x;
    p.y = at.y;
    m_vtx[0].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[0].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[0].z = 0.0f;
    m_vtx[0].w = 1.0f;
    m_vtx[2].x = n.x;
    m_vtx[2].y = n.y;
    m_vtx[2].z = n.z;
    m_vtx[2].w = 0.0f;
    m_vtx[3].x = c.r;
    m_vtx[3].y = c.g;
    m_vtx[3].z = c.b;
    m_vtx[3].w = c.a;
    m_numVerts++;
    p.x += b.x;
    m_vtx[4].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[4].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[4].z = 0.0f;
    m_vtx[4].w = 1.0f;
    m_vtx[6].x = n.x;
    m_vtx[6].y = n.y;
    m_vtx[6].z = n.z;
    m_vtx[6].w = 0.0f;
    m_vtx[7].x = c.r;
    m_vtx[7].y = c.g;
    m_vtx[7].z = c.b;
    m_vtx[7].w = c.a;
    m_numVerts++;
    p.y += b.y;
    m_vtx[8].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[8].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[8].z = 0.0f;
    m_vtx[8].w = 1.0f;
    m_vtx[10].x = n.x;
    m_vtx[10].y = n.y;
    m_vtx[10].z = n.z;
    m_vtx[10].w = 0.0f;
    m_vtx[11].x = c.r;
    m_vtx[11].y = c.g;
    m_vtx[11].z = c.b;
    m_vtx[11].w = c.a;
    m_numVerts++;
    p.x = at.x;
    m_vtx[12].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[12].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[12].z = 0.0f;
    m_vtx[12].w = 1.0f;
    m_vtx[14].x = n.x;
    m_vtx[14].y = n.y;
    m_vtx[14].z = n.z;
    m_vtx[14].w = 0.0f;
    m_vtx[15].x = c.r;
    m_vtx[15].y = c.g;
    m_vtx[15].z = c.b;
    m_vtx[15].w = c.a;
    m_numVerts++;
    switch (flags & 3)
    {
    case 0:
        m_vtx[1].x = (float)a.x * inv.x;
        m_vtx[1].y = (float)a.y * inv.y;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = (float)(a.x + b.x) * inv.x;
        m_vtx[5].y = (float)a.y * inv.y;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = (float)(a.x + b.x) * inv.x;
        m_vtx[9].y = (float)(a.y + b.y) * inv.y;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = (float)a.x * inv.x;
        m_vtx[13].y = (float)(a.y + b.y) * inv.y;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    case 1:
        m_vtx[1].x = (float)(a.x + b.x) * inv.x;
        m_vtx[1].y = (float)a.y * inv.y;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = (float)a.x * inv.x;
        m_vtx[5].y = (float)a.y * inv.y;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = (float)a.x * inv.x;
        m_vtx[9].y = (float)(a.y + b.y) * inv.y;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = (float)(a.x + b.x) * inv.x;
        m_vtx[13].y = (float)(a.y + b.y) * inv.y;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    case 2:
        m_vtx[1].x = (float)a.x * inv.x;
        m_vtx[1].y = (float)(a.y + b.y) * inv.y;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = (float)(a.x + b.x) * inv.x;
        m_vtx[5].y = (float)(a.y + b.y) * inv.y;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = (float)(a.x + b.x) * inv.x;
        m_vtx[9].y = (float)a.y * inv.y;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = (float)a.x * inv.x;
        m_vtx[13].y = (float)a.y * inv.y;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    case 3:
        m_vtx[1].x = (float)(a.x + b.x) * inv.x;
        m_vtx[1].y = (float)(a.y + b.y) * inv.y;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = (float)a.x * inv.x;
        m_vtx[5].y = (float)(a.y + b.y) * inv.y;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = (float)a.x * inv.x;
        m_vtx[9].y = (float)a.y * inv.y;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = (float)(a.x + b.x) * inv.x;
        m_vtx[13].y = (float)a.y * inv.y;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    }
    m_vtx += 16;
    if (BlitEnd(&m_dmaTag) < 0)
        return -1;
    mat->m_layers[0].m_texture = -1;
    FlushDMABufferIfFull();
    return 0;
}

// The same, drawn `size` big on screen whatever the texel rectangle's size.
int VIRaster::StretchBlit(int id, unsigned int flags, const VIVect2I &at,
                          const VIVect2I &size, const VIVect2I &a,
                          const VIVect2I &b, VIAlphaBlendMode m, float alpha)
{
    VIMaterial *mat = (VIMaterial *)m_blitMaterial;
    VISurface *s = m_surfaces.Value(id);
    VIVect2I p;
    VIColor32F c;
    VIVect3 n;
    VIVect2 inv;
    if (m_p4C4C != 1)
        UploadRasterMicro(&m_dmaTag);
    mat->m_layers[0].m_texture = id;
    mat->m_layers[0].m_blendMode = m;
    if (flags & 4)
        mat->SetDither(0);
    else
        mat->SetDither(1);
    if (id != m_blitSurface || m != m_blitBlend)
    {
        mat->m_dirty = 1;
        m_blitSurface = id;
        m_blitBlend = m;
    }
    if (AddMaterial(m_blitMaterial, &m_dmaTag) < 0)
        return -1;
    if (BlitBegin(&m_dmaTag) < 0)
        return -1;
    c.Set(1.0f, 1.0f, 1.0f, alpha);
    n.Set(0.0f, 0.0f, 0.0f);
    inv.x = 1.0f / (float)s->m_width;
    inv.y = 1.0f / (float)s->m_height;
    p.x = at.x;
    p.y = at.y;
    m_vtx[0].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[0].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[0].z = 0.0f;
    m_vtx[0].w = 1.0f;
    m_vtx[2].x = n.x;
    m_vtx[2].y = n.y;
    m_vtx[2].z = n.z;
    m_vtx[2].w = 0.0f;
    m_vtx[3].x = c.r;
    m_vtx[3].y = c.g;
    m_vtx[3].z = c.b;
    m_vtx[3].w = c.a;
    m_numVerts++;
    p.x += size.x;
    m_vtx[4].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[4].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[4].z = 0.0f;
    m_vtx[4].w = 1.0f;
    m_vtx[6].x = n.x;
    m_vtx[6].y = n.y;
    m_vtx[6].z = n.z;
    m_vtx[6].w = 0.0f;
    m_vtx[7].x = c.r;
    m_vtx[7].y = c.g;
    m_vtx[7].z = c.b;
    m_vtx[7].w = c.a;
    m_numVerts++;
    p.y += size.y;
    m_vtx[8].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[8].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[8].z = 0.0f;
    m_vtx[8].w = 1.0f;
    m_vtx[10].x = n.x;
    m_vtx[10].y = n.y;
    m_vtx[10].z = n.z;
    m_vtx[10].w = 0.0f;
    m_vtx[11].x = c.r;
    m_vtx[11].y = c.g;
    m_vtx[11].z = c.b;
    m_vtx[11].w = c.a;
    m_numVerts++;
    p.x = at.x;
    m_vtx[12].x = ((float)p.x - m_viewportWF + 0.5f) * m_invViewportW;
    m_vtx[12].y = ((float)p.y - m_viewportHF + 0.5f) * -m_invViewportH;
    m_vtx[12].z = 0.0f;
    m_vtx[12].w = 1.0f;
    m_vtx[14].x = n.x;
    m_vtx[14].y = n.y;
    m_vtx[14].z = n.z;
    m_vtx[14].w = 0.0f;
    m_vtx[15].x = c.r;
    m_vtx[15].y = c.g;
    m_vtx[15].z = c.b;
    m_vtx[15].w = c.a;
    m_numVerts++;
    switch (flags & 3)
    {
    case 0:
        m_vtx[1].x = (float)a.x * inv.x;
        m_vtx[1].y = (float)a.y * inv.y;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = (float)(a.x + b.x) * inv.x;
        m_vtx[5].y = (float)a.y * inv.y;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = (float)(a.x + b.x) * inv.x;
        m_vtx[9].y = (float)(a.y + b.y) * inv.y;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = (float)a.x * inv.x;
        m_vtx[13].y = (float)(a.y + b.y) * inv.y;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    case 1:
        m_vtx[1].x = (float)(a.x + b.x) * inv.x;
        m_vtx[1].y = (float)a.y * inv.y;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = (float)a.x * inv.x;
        m_vtx[5].y = (float)a.y * inv.y;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = (float)a.x * inv.x;
        m_vtx[9].y = (float)(a.y + b.y) * inv.y;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = (float)(a.x + b.x) * inv.x;
        m_vtx[13].y = (float)(a.y + b.y) * inv.y;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    case 2:
        m_vtx[1].x = (float)a.x * inv.x;
        m_vtx[1].y = (float)(a.y + b.y) * inv.y;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = (float)(a.x + b.x) * inv.x;
        m_vtx[5].y = (float)(a.y + b.y) * inv.y;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = (float)(a.x + b.x) * inv.x;
        m_vtx[9].y = (float)a.y * inv.y;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = (float)a.x * inv.x;
        m_vtx[13].y = (float)a.y * inv.y;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    case 3:
        m_vtx[1].x = (float)(a.x + b.x) * inv.x;
        m_vtx[1].y = (float)(a.y + b.y) * inv.y;
        m_vtx[1].z = 1.0f;
        m_vtx[1].w = 0.0f;
        m_vtx[5].x = (float)a.x * inv.x;
        m_vtx[5].y = (float)(a.y + b.y) * inv.y;
        m_vtx[5].z = 1.0f;
        m_vtx[5].w = 0.0f;
        m_vtx[9].x = (float)a.x * inv.x;
        m_vtx[9].y = (float)a.y * inv.y;
        m_vtx[9].z = 1.0f;
        m_vtx[9].w = 0.0f;
        m_vtx[13].x = (float)(a.x + b.x) * inv.x;
        m_vtx[13].y = (float)a.y * inv.y;
        m_vtx[13].z = 1.0f;
        m_vtx[13].w = 0.0f;
        break;
    }
    m_vtx += 16;
    if (BlitEnd(&m_dmaTag) < 0)
        return -1;
    mat->m_layers[0].m_texture = -1;
    FlushDMABufferIfFull();
    return 0;
}
