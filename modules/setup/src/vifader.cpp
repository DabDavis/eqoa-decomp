// vifader.cpp (name ours) -- SETUP 0x0056B088 .. 0x0056B3A0.  The screen fade.
#include "setup.h"

// 0x0056B088.
VIFader::VIFader()
{
    m_material = -1;
}

// 0x0056B098.
VIFader::~VIFader()
{
    Free();
}

// 0x0056B0E0.  The dictionary argument is never stored, so Free hands
// ReleaseMaterial whatever +0x04 already held (the 2002 beta's Init is 16
// bytes longer).
int VIFader::Init(VIRaster *r, VIDictionary *d, float alpha)
{
    m_target = 0.0f;
    m_rate = 0.0f;
    m_alpha = alpha;
    m_raster = r;
    m_material = r->CreateMaterial(1);
    if (m_material < 0)
        return -1;
    VIMaterial *m = m_raster->Material(m_material);
    m->InitSolidFill();
    m->SetLayerBlendMode(0, (VIAlphaBlendMode)3);
    m_time = VITimer::GetTime();
    return 0;
}

// 0x0056B170.
void VIFader::Free()
{
    if (m_raster && m_material >= 0)
        m_raster->ReleaseMaterial(m_material, *m_dict);
    m_raster = 0;
    m_material = -1;
}

// 0x0056B1C0.
void VIFader::Begin(float target, float seconds)
{
    if (seconds == 0.0f)
    {
        m_target = target;
        m_alpha = target;
        m_rate = 0.0f;
    }
    else
    {
        m_target = target;
        m_rate = (target - m_alpha) / seconds;
    }
}

// 0x0056B200.
void VIFader::End()
{
    m_alpha = m_target;
    m_rate = 0.0f;
}

// 0x0056B210.  A black quad over the whole screen, one pixel past each edge.
void VIFader::Update()
{
    long now = VITimer::GetTime();
    float dt = (float)(now - m_time) * 0.001f;
    m_time = now;
    m_alpha += m_rate * dt;
    if (m_rate > 0.0f)
        m_alpha = m_alpha < m_target ? m_alpha : m_target;
    else
        m_alpha = m_alpha < m_target ? m_target : m_alpha;
    if (m_alpha > 0.0f)
    {
        m_raster->SetMaterial(m_material);
        m_raster->Begin2D();
        m_raster->BeginTriStrip();
        VIColor32F c(0.0f, 0.0f, 0.0f, m_alpha);
        m_raster->Color(c);
        VIVect2I v(0, m_raster->m_screenH);
        m_raster->Vertex(v);
        v.Set(0, -1);
        m_raster->Vertex(v);
        v.Set(m_raster->m_screenW + 1, m_raster->m_screenH);
        m_raster->Vertex(v);
        v.Set(m_raster->m_screenW + 1, -1);
        m_raster->Vertex(v);
        m_raster->EndTriStrip();
        m_raster->End2D();
    }
}
