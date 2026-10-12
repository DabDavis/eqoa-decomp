// VIParticleDefinition -- SUPPORT 0x004565A0 .. 0x0045690C.
#include "vi.h"

VIParticleDefinition::VIParticleDefinition()
{
    m_blendMode = BLENDMODE_0;
    m_zWrite = 0;
    m_zTest = 1;
    m_texture = -1;
    m_material = -1;
    m_texConfig = 1;
    m_raster = 0;
}

VIParticleDefinition::~VIParticleDefinition()
{
}

int VIParticleDefinition::Create(VIRaster &r)
{
    m_material = r.CreateMaterial(0);

    if (m_material == -1)
        return -1;

    VIMaterial *mat = r.Material(m_material);

    mat->Init(1);
    mat->SetLayerFillType(0, VIMaterialFillTypeTexture);
    mat->SetLayerModulate(0, 1);
    mat->SetDither(0);

    m_raster = &r;

    SetBlendMode(m_blendMode);
    SetZWrite(m_zWrite);
    SetZTest(m_zTest);
    return 0;
}

int VIParticleDefinition::Destroy(VIRaster &r, VIDictionary &d)
{
    if (m_material != -1)
    {
        VIMaterial *mat = r.Material(m_material);

        mat->SetLayerTexture(0, -1, r, d);
        r.ReleaseMaterial(m_material, d);
    }

    VIParticleMotif *m;

    while ((m = FirstMotif()) != 0)
        DestroyMotif(m);

    return 0;
}

void VIParticleDefinition::SetBlendMode(VIParticleDefinitionEx::BLENDMODE m)
{
    m_blendMode = m;

    VIMaterial *mat = m_raster->Material(m_material);

    switch (m_blendMode)
    {
    case BLENDMODE_0:
        mat->SetLayerBlendMode(0, VIAlphaBlendModeNone);
        break;
    case BLENDMODE_1:
        mat->SetLayerBlendMode(0, VIAlphaBlendMode1);
        break;
    case BLENDMODE_2:
        mat->SetLayerBlendMode(0, VIAlphaBlendMode2);
        break;
    case BLENDMODE_3:
        mat->SetLayerBlendMode(0, VIAlphaBlendMode3);
        break;
    case BLENDMODE_4:
        mat->SetLayerBlendMode(0, VIAlphaBlendMode4);
        break;
    }
}

void VIParticleDefinition::SetZWrite(bool on)
{
    m_zWrite = on;
    m_raster->Material(m_material)->SetZWrite(m_zWrite != 0);
}

void VIParticleDefinition::SetZTest(bool on)
{
    m_zTest = on;
    m_raster->Material(m_material)->SetZTest(m_zTest != 0);
}

int VIParticleDefinition::Texture() const
{
    return m_texture;
}

void VIParticleDefinition::SetTexture(int tex, VIRaster &r, VIDictionary &d)
{
    m_texture = tex;
    m_raster->Material(m_material)->SetLayerTexture(0, m_texture, r, d);
}

void VIParticleDefinition::SetTextureConfiguration(int c)
{
    m_texConfig = c;
}
