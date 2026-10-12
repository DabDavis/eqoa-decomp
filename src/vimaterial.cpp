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
    m_time = 0.0;
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
        l->m_uvTransform.Identity();
        l->m_blendMode = 0;
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
    l->m_uvOffset.x = 0.0f;
    l->m_uvOffset.y = 0.0f;
    l->m_uvRate.x = 0.0f;
    l->m_uvRate.y = 0.0f;
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
    l->m_uvOffset.x = 0.0f;
    l->m_uvOffset.y = 0.0f;
    l->m_uvRate.x = 0.0f;
    l->m_uvRate.y = 0.0f;
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
    Layer(this, i)->m_uvRate = r;
    if (r.x != 0.0f || r.y != 0.0f)
        m_flags |= 4;
    m_dirty = 1;
    return 0;
}
