// VIMaterial -- SUPPORT 0x003FC170 .. 0x003FD7E8.
//
// The render state for one surface, and the second property bag in the corpus
// after VIParticleAttributes.  Its layout reads off its own accessors: every
// layer setter multiplies the index by 144 and adds it to `this`, so the layer
// array starts at +0x30 with a 144-byte stride, and almost every setter closes
// with `m_dirty = 1` at +0x20.
#include "vi.h"

// `this` advanced by whole layers -- see the note on VIMaterialLayer.  Written
// `&m_layers[i]` the array base folds onto `this` and every read-modify-write
// accessor comes out two instructions different.
inline static VIMaterialLayerView *Layer(VIMaterial *m, int i)
{
    return (VIMaterialLayerView *)((char *)m + i * 144);
}

inline static const VIMaterialLayerView *Layer(const VIMaterial *m, int i)
{
    return (const VIMaterialLayerView *)((const char *)m + i * 144);
}

VIMaterial::VIMaterial()
{
    m_numLayers = 0;
    m_flags = 19;
    m_emissive.Set(0, 0, 0, 0);
    m_dirty = 1;
    m_handle = -1;
    m_time = 0;
}

int VIMaterial::Init(int layers)
{
    m_flags = 19;
    m_numLayers = layers;
    m_emissive.Set(0, 0, 0, 0);
    m_dirty = 1;
    for (int i = 0; i < layers; i++)
    {
        VIMaterialLayerView *l = Layer(this, i);
        l->m_texture = -1;
        l->m_flags = 1;
        l->m_color.r = 255;
        l->m_color.g = 255;
        l->m_color.b = 255;
        l->m_color.a = 255;
        l->m_blendMode = 0;
        l->m_uvTransform.Identity();
        l->m_uvOffset.x = 0.0f;
        l->m_uvOffset.y = 0.0f;
        l->m_uvRate.x = 0.0f;
        l->m_uvRate.y = 0.0f;
        l->m_pass = -1;
        l->m_lodBias = 0.8f;
    }
    return 0;
}

int VIMaterial::InitWireframe()
{
    VIMaterialLayerView *l = Layer(this, 0);
    m_numLayers = 1;
    m_flags = 19;
    m_emissive.Set(0, 0, 0, 0);
    m_dirty = 1;
    l->m_texture = -1;
    l->m_blendMode = 0;
    l->m_flags = 33;
    l->m_color.r = 255;
    l->m_color.g = 255;
    l->m_color.b = 255;
    l->m_color.a = 255;
    l->m_uvTransform.Identity();
    l->m_pass = -1;
    l->m_lodBias = 0.8f;
    // uvOffset.x LAST: the final store of a constant is hoisted to the front
    // of the run, which is where the target has it.
    l->m_uvOffset.y = 0.0f;
    l->m_uvRate.x = 0.0f;
    l->m_uvRate.y = 0.0f;
    l->m_uvOffset.x = 0.0f;
    return 0;
}

int VIMaterial::InitSolidFill()
{
    VIMaterialLayerView *l = Layer(this, 0);
    m_numLayers = 1;
    m_flags = 19;
    m_emissive.Set(0, 0, 0, 0);
    m_dirty = 1;
    l->m_texture = -1;
    l->m_blendMode = 0;
    l->m_flags = 1;
    l->m_color.r = 255;
    l->m_color.g = 255;
    l->m_color.b = 255;
    l->m_color.a = 255;
    l->m_uvTransform.Identity();
    l->m_pass = -1;
    l->m_lodBias = 0.8f;
    // uvOffset.x LAST: the final store of a constant is hoisted to the front
    // of the run, which is where the target has it.
    l->m_uvOffset.y = 0.0f;
    l->m_uvRate.x = 0.0f;
    l->m_uvRate.y = 0.0f;
    l->m_uvOffset.x = 0.0f;
    return 0;
}

int VIMaterial::SetZWrite(int on)
{
    if (on)
        m_flags |= 2;
    else
        m_flags &= ~2u;
    m_dirty = 1;
    return 0;
}

int VIMaterial::SetZTest(int on)
{
    if (on)
        m_flags |= 1;
    else
        m_flags &= ~1u;
    m_dirty = 1;
    return 0;
}

int VIMaterial::SetDither(int on)
{
    if (on)
        m_flags |= 0x10;
    else
        m_flags &= ~0x10u;
    return 0;
}

int VIMaterial::SetTessellate(int on)
{
    if (on)
        m_flags |= 0x20;
    else
        m_flags &= ~0x20u;
    return 0;
}

int VIMaterial::SetEmissiveColor(const VIColor32 &c)
{
    m_emissive = c;
    m_dirty = 1;
    return 0;
}

int VIMaterial::SetLayerFillType(int i, VIMaterialFillType t)
{
    if (t == VIMaterialFillTypeSolid)
        Layer(this, i)->m_flags |= 0x20;
    else
        Layer(this, i)->m_flags &= ~0x20u;
    m_dirty = 1;
    return 0;
}

int VIMaterial::GetLayerTexture(int i)
{
    return Layer(this, i)->m_texture;
}

int VIMaterial::SetLayerColor(int i, const VIColor32 &c)
{
    Layer(this, i)->m_color = c;
    m_dirty = 1;
    return 0;
}

VIColor32 VIMaterial::GetLayerColor(int i) const
{
    return Layer(this, i)->m_color;
}

int VIMaterial::SetLayerModulate(int i, int on)
{
    if (on)
        Layer(this, i)->m_flags |= 1;
    else
        Layer(this, i)->m_flags &= ~1u;
    m_dirty = 1;
    return 0;
}

int VIMaterial::SetLayerWrapMode(int i, VIMaterialWrapMode m)
{
    switch (m)
    {
    case VIMaterialWrapModeRepeat:
        Layer(this, i)->m_flags &= ~6u;
        break;
    case VIMaterialWrapModeClamp:
        Layer(this, i)->m_flags = (Layer(this, i)->m_flags & ~6u) | 2;
        break;
    case VIMaterialWrapModeRegion:
        Layer(this, i)->m_flags = (Layer(this, i)->m_flags & ~6u) | 4;
        break;
    case VIMaterialWrapModeMirror:
        Layer(this, i)->m_flags |= 6;
        break;
    default:
        return -1;
    }
    m_dirty = 1;
    return 0;
}

int VIMaterial::GetLayerWrapMode(int i)
{
    switch (Layer(this, i)->m_flags & 6)
    {
    case 0:
        return VIMaterialWrapModeRepeat;
    case 2:
        return VIMaterialWrapModeClamp;
    case 4:
        return VIMaterialWrapModeRegion;
    case 6:
        return VIMaterialWrapModeMirror;
    }
    return 0;
}

int VIMaterial::SetLayerBlendMode(int i, VIAlphaBlendMode m)
{
    Layer(this, i)->m_blendMode = m;
    m_dirty = 1;
    return 0;
}

int VIMaterial::GetLayerBlendMode(int i)
{
    return Layer(this, i)->m_blendMode;
}

int VIMaterial::SetLayerTextureLODBias(int i, float b)
{
    Layer(this, i)->m_lodBias = b;
    m_dirty = 1;
    return 0;
}

int VIMaterial::SetLayerMagFilter(int i, int on)
{
    if (on)
        Layer(this, i)->m_flags &= ~8u;
    else
        Layer(this, i)->m_flags |= 8;
    m_dirty = 1;
    return 0;
}

// A shipped bug, matched as written: the `set` arm ORs in the COMPLEMENT of the
// bit it just cleared, so asking for UV set 1 turns on every flag except the
// one meant.  Written `|= 0x10` the function is two instructions different.
int VIMaterial::SetLayerUVSet(int i, int set)
{
    Layer(this, i)->m_flags &= ~0x10u;
    if (set)
        Layer(this, i)->m_flags |= ~0x10u;
    m_dirty = 1;
    return 0;
}

int VIMaterial::SetLayerUVTransform(int i, const VIMatrix33 &m)
{
    Layer(this, i)->m_uvTransform = m;
    m_dirty = 1;
    return 0;
}

int VIMaterial::SetLayerUVRate(int i, const VIVect2 &r)
{
    // The real member array: its +48 rides in the pointer, so op='s `this`
    // is materialised (`addiu a1,a1,104`) instead of folding into the stores.
    m_layers[i].m_uvRate = r;
    if (r.x != 0.0f || r.y != 0.0f)
        m_flags |= 4;
    m_dirty = 1;
    return 0;
}

// 0x003FC4A0.  Grow by initialising the new layers (no LOD bias and no UV
// rate -- only what Init's loop sets first), shrink by releasing the textures
// of the ones dropped.  The release walk subscripts the real array: its
// pointer carries the +48, where the init walk's displacements do.
int VIMaterial::SetNumLayers(int n, VIRaster &r, VIDictionary &d)
{
    if (n >= 4)
        return -1;
    // ONE index for both walks, and the count read once into its own
    // variable: either alone leaves the loop counter in another register.
    int i;
    int cur = m_numLayers;
    if (cur < n)
    {
        for (i = cur; i < n; i++)
        {
            VIMaterialLayerView *l = Layer(this, i);
            l->m_texture = -1;
            l->m_flags = 1;
            l->m_color.r = 255;
            l->m_color.g = 255;
            l->m_color.b = 255;
            l->m_color.a = 255;
            l->m_blendMode = 0;
            l->m_uvTransform.Identity();
            l->m_uvOffset.x = 0.0f;
            l->m_uvOffset.y = 0.0f;
        }
    }
    else if (n < cur)
    {
        for (i = n; i < m_numLayers; i++)
            if (m_layers[i].m_texture != -1)
                r.ReleaseSurface(m_layers[i].m_texture, d);
    }
    m_numLayers = n;
    m_dirty = 1;
    return 0;
}

// 0x003FC738.  Release the old surface, take a reference on the new one.
int VIMaterial::SetLayerTexture(int i, int tex, VIRaster &r, VIDictionary &d)
{
    VIMaterialLayerView *l = Layer(this, i);
    if (l->m_texture != -1)
        r.ReleaseSurface(l->m_texture, d);
    l->m_texture = tex;
    if (tex != -1)
        r.ShareSurface(tex);
    m_dirty = 1;
    return 0;
}

// 0x003FCB80.  Every layer field but the texture by value, then the texture
// through SetLayerTexture so its reference count follows.
int VIMaterial::CopyLayer(int src, int dst, VIRaster &r, VIDictionary &d)
{
    if (src != dst)
    {
        VIMaterialLayerView *s = Layer(this, src);
        VIMaterialLayerView *t = Layer(this, dst);
        t->m_flags = s->m_flags;
        t->m_color = s->m_color;
        t->m_blendMode = s->m_blendMode;
        m_layers[dst].m_uvTransform = m_layers[src].m_uvTransform;
        t->m_uvRate = s->m_uvRate;
        t->m_uvOffset = s->m_uvOffset;
        t->m_lodBias = s->m_lodBias;
        SetLayerTexture(dst, s->m_texture, r, d);
        m_dirty = 1;
    }
    return 0;
}

// 0x003FCC70.  Scroll every textured layer's UV offset by its rate over the
// time since the last call, and wrap each component back into (-1, 1).
int VIMaterial::Process(long time)
{
    if (m_time != time)
    {
        float dt = (float)(int)(time - m_time) * 0.001f;
        for (int i = 0; i < m_numLayers; i++)
        {
            if (Layer(this, i)->m_texture < 0)
                continue;
            VIVect2 &uv = Layer(this, i)->m_uvOffset;
            uv.x += Layer(this, i)->m_uvRate.x * dt;
            uv.y += Layer(this, i)->m_uvRate.y * dt;
            if (1.0f < uv.x || uv.x < -1.0f)
                uv.x -= (float)(int)uv.x;
            if (1.0f < m_layers[i].m_uvOffset.y ||
                m_layers[i].m_uvOffset.y < -1.0f)
                m_layers[i].m_uvOffset.y -= (float)(int)m_layers[i].m_uvOffset.y;
        }
        m_time = time;
    }
    return 0;
}

// A byte colour to a unit float one.  Through a reference: the colour's +0
// folds onto the layer base and +1..+3 come off a register of their own.
static inline void ToFloat(VIColor32F &f, const VIColor32 &c)
{
    f.r = (float)c.r * (1.0f / 255.0f);
    f.g = (float)c.g * (1.0f / 255.0f);
    f.b = (float)c.b * (1.0f / 255.0f);
    f.a = (float)c.a * (1.0f / 255.0f);
}

// 0x003FCF38.  One textured layer: seventeen quadwords, a count word and a
// GIFtag of eight A+D pairs (TEST_1, ALPHA_1, CLAMP_1, the four texture
// registers VIEESurfCache::Set wrote into the pass, ZBUF_1), then the three
// primitive GIFtags the microcode reuses, the layer colour and the UV
// transform with the scroll offset folded into its translation row.
int VIMaterial::ConstructTextPass(int i, VIRaster &r, VIQWord *q)
{
    VIColor32F col;

    if ((Layer(this, i)->m_flags & 6) == 0)
        q->m_w[0] = 1;
    else
        q->m_w[0] = 3;
    q->m_w[1] = 0;
    q->m_w[2] = 0;
    q->m_w[3] = 0;
    q++;

    q->m_d[0] = 0x1000000000008008;
    q->m_d[1] = 14;
    q++;

    long test;
    long testz;
    if (Layer(this, i)->m_blendMode)
    {
        test = 0x33001;
        testz = 0x53001;
    }
    else
    {
        test = 0x3080B;
        testz = 0x5080B;
    }
    if (m_flags & 1)
        test = testz;
    q->m_d[0] = test;
    q->m_d[1] = 71;
    q++;

    long alpha;
    switch (Layer(this, i)->m_blendMode)
    {
    case 1:
        alpha = 0x8000000048;
        break;
    case 2:
        alpha = 0x8000000042;
        break;
    case 3:
        alpha = 0x8000000044;
        break;
    case 4:
        alpha = 0x8000000089;
        break;
    case 0:
    default:
        alpha = 0x8000000064;
        break;
    }
    q->m_d[0] = alpha;
    q->m_d[1] = 66;
    q++;

    long clamp = (int)Layer(this, i)->m_flags;
    clamp = (clamp >> 1) & 1;
    if (Layer(this, i)->m_flags & 4)
        clamp |= 4;
    q->m_d[0] = clamp;
    q->m_d[1] = 8;
    q++;

    VIMaterialLayerView *p = Layer(this, i);
    q->m_d[0] = p->m_reg[0];
    q->m_d[1] = p->m_reg[1];
    q++;
    q->m_d[0] = p->m_reg[2];
    q->m_d[1] = p->m_reg[3];
    q++;
    q->m_d[0] = p->m_reg[4];
    q->m_d[1] = p->m_reg[5];
    q++;
    q->m_d[0] = p->m_reg[6];
    q->m_d[1] = p->m_reg[7];
    q++;

    long z = r.GetZBufferReg();
    long psm = z & 0x0F000000;
    long zbuf = (z & 0x1FF) | psm;
    if ((m_flags & 2) == 0)
        zbuf |= 0x100000000;
    q->m_d[0] = zbuf;
    q->m_d[1] = 78;
    q++;

    long fog = 0;
    if (r.m_fogEnabled)
        fog = p->m_blendMode != 4;
    long abe = p->m_blendMode != 0;
    fog <<= 5;
    abe <<= 6;
    q->m_d[0] = (fog | (abe | 28)) << 47 | 0x3000400000008003;
    q->m_d[1] = 1042;
    q++;
    q->m_d[0] = (fog | (abe | 29)) << 47 | 0x3000400000008003;
    q->m_d[1] = 1042;
    q++;
    q->m_d[0] = (fog | (abe | 10)) << 47 | 0x2000400000008003;
    q->m_d[1] = 65;
    q++;

    int blend = p->m_blendMode;
    ToFloat(col, p->m_color);
    col.r *= 128.0f;
    col.g *= 128.0f;
    col.b *= 128.0f;
    col.a *= 128.0f;
    if (blend)
        col.a *= 0.5f;
    q->m_f[0] = col.r;
    q->m_f[1] = col.g;
    q->m_f[2] = col.b;
    q->m_f[3] = col.a;
    q++;

    q->m_f[0] = p->m_uvTransform.m[0].x;
    q->m_f[1] = p->m_uvTransform.m[0].y;
    q->m_f[2] = p->m_uvTransform.m[0].z;
    q++;
    q->m_f[0] = p->m_uvTransform.m[1].x;
    q->m_f[1] = p->m_uvTransform.m[1].y;
    q->m_f[2] = p->m_uvTransform.m[1].z;
    q++;
    q->m_f[0] = p->m_uvTransform.m[2].x + p->m_uvOffset.x;
    q->m_f[1] = p->m_uvTransform.m[2].y + p->m_uvOffset.y;
    q->m_f[2] = p->m_uvTransform.m[2].z;
    return 0;
}

// 0x003FD380.  A layer with no texture: the same seventeen quadwords with the
// count word zero, TEST_1/ALPHA_1/ZBUF_1 only, the texture block skipped, the
// three untextured primitive tags (alpha-blended when the layer blends) and
// the colour scaled to 255 rather than 128.
int VIMaterial::ConstructSolidFillPass(int i, VIRaster &r, VIQWord *q)
{
    VIColor32F col;

    q->m_w[0] = 0;
    q->m_w[1] = 0;
    q->m_w[2] = 0;
    q->m_w[3] = 0;
    q++;

    q->m_d[0] = 0x1000000000008003;
    q->m_d[1] = 14;
    q++;

    long test = 0x30000;
    if (m_flags & 1)
        test = 0x50000;
    q->m_d[0] = test;
    q->m_d[1] = 71;
    q++;

    long alpha;
    switch (Layer(this, i)->m_blendMode)
    {
    case 1:
        alpha = 0x8000000048;
        break;
    case 2:
        alpha = 0x8000000042;
        break;
    case 3:
        alpha = 0x8000000044;
        break;
    case 4:
        alpha = 0x8000000089;
        break;
    case 0:
    default:
        alpha = 0x8000000064;
        break;
    }
    q->m_d[0] = alpha;
    q->m_d[1] = 66;
    q++;

    long z = r.GetZBufferReg();
    long psm = z & 0x0F000000;
    long zbuf = (z & 0x1FF) | psm;
    if ((m_flags & 2) == 0)
        zbuf |= 0x100000000;
    q->m_d[0] = zbuf;
    q->m_d[1] = 78;
    q += 6;

    VIMaterialLayerView *p = Layer(this, i);
    q->m_d[0] = p->m_blendMode ? 0x2026400000008003 : 0x2006400000008003;
    q->m_d[1] = 65;
    q++;
    q->m_d[0] = p->m_blendMode ? 0x2026C00000008003 : 0x2006C00000008003;
    q->m_d[1] = 65;
    q++;
    q->m_d[0] = p->m_blendMode ? 0x2025400000008003 : 0x2005400000008003;
    q->m_d[1] = 65;
    q++;

    int blend = p->m_blendMode;
    ToFloat(col, p->m_color);
    col.r *= 255.0f;
    col.g *= 255.0f;
    col.b *= 255.0f;
    col.a *= 255.0f;
    if (blend)
        col.a *= 0.5f;
    q->m_f[0] = col.r;
    q->m_f[1] = col.g;
    q->m_f[2] = col.b;
    q->m_f[3] = col.a;
    return 0;
}

// 0x003FD658.  Wireframe: everything constant but the colour.  The raster
// is not read at all.
int VIMaterial::ConstructWirePass(int i, VIRaster &r, VIQWord *q)
{
    VIColor32F col;

    q->m_w[0] = 2;
    q->m_w[1] = 0;
    q->m_w[2] = 0;
    q->m_w[3] = 0;
    q++;

    q->m_d[0] = 0x1000000000008002;
    q->m_d[1] = 14;
    q++;
    q->m_d[0] = 0x50000;
    q->m_d[1] = 71;
    q++;
    q->m_d[0] = 0x8000000064;
    q->m_d[1] = 66;
    q += 7;

    q->m_d[0] = 0x2006400000008003;
    q->m_d[1] = 65;
    q++;
    q->m_d[0] = 0x2006C00000008003;
    q->m_d[1] = 65;
    q++;
    q->m_d[0] = 0x2005400000008003;
    q->m_d[1] = 65;
    q++;

    ToFloat(col, m_layers[i].m_color);
    col.r *= 128.0f;
    col.g *= 128.0f;
    col.b *= 128.0f;
    col.a *= 128.0f;
    q->m_f[0] = col.r;
    q->m_f[1] = col.g;
    q->m_f[2] = col.b;
    q->m_f[3] = col.a;
    return 0;
}

// 0x003FCDB8.  The whole material: a FLUSHE and an UNPACK of 17 quadwords per
// layer plus one, the layer count, then each layer's pass.  Layer flag 0x20
// selects the WIREFRAME pass -- the bit SetLayerFillType sets for its value 1
// and InitWireframe sets in its 33 -- and a layer without a texture the solid
// one.  Only the first layer is sent unless the raster does multitexture.
int VIMaterial::ConstructDMAPacket(VIRaster &r, VIQWord *q)
{
    VIColor32F unused;
    int n = 1;
    if (r.m_multiTexture)
        n = m_numLayers;
    if (m_flags & 4)
        Process(r.GetTime());

    q->m_w[0] = 0;
    q->m_w[1] = 0;
    q->m_w[2] = 0x10000000;
    q->m_w[3] = ((n * 17 + 1) << 16) | 0x6C0001B3;
    q++;
    q->m_w[0] = n;
    q->m_w[1] = 0;
    q->m_w[2] = 0;
    q->m_w[3] = 0;
    q++;

    int count = 2;
    for (int i = 0; i < n; i++)
    {
        if (Layer(this, i)->m_flags & 0x20)
            ConstructWirePass(i, r, q);
        else if (Layer(this, i)->m_texture != -1)
            ConstructTextPass(i, r, q);
        else
            ConstructSolidFillPass(i, r, q);
        q += 17;
        count += 17;
    }
    m_dirty = 0;
    return count;
}
