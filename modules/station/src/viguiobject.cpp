// viguiobject.cpp (name ours) -- STATION 0x002C10B8 .. 0x002C2610: the base of every
// page element -- position, size, the select effect, the image or tiled frame
// behind it.  Then the item-header template copies, __tf11VIGUIObject and the
// three inline virtuals.
#include "station.h"

// 0x003500D8: the solid-fill material FillRect makes on first use.
static int sSolidMaterial = -1;

// 0x003500E0 / 0x00350120: the tilesets, by m_tileset.  Only Parchment.
static VIUIFrameParam sTilesetFrame[1] =
{
    { { (VIUITexId)44, 0 }, { (VIUITexId)45, 0 }, { (VIUITexId)46, 0 }, { (VIUITexId)52, 0 },
      { (VIUITexId)50, 0 }, { (VIUITexId)49, 0 }, { (VIUITexId)48, 0 }, { (VIUITexId)47, 0 } },
};
static VIUITexId sTilesetFill[1] = { (VIUITexId)35 };

// 0x002C10B8.
VIGUIObject::VIGUIObject(const char *name, int type, VIWndGenericRenderer *r,
                         VIXMLParser *reader)
{
    Init();
    SetName(name);
    SetType(type);
    SetRenderer(r);
    SetReader(reader);
}

// 0x002C1160.  The order of the five -1s decides which two registers hold them:
// cse lets a short take a wider register's -1, but not the other way round.
void VIGUIObject::Init()
{
    memset(m_name, 0, 31);
    m_type = 0;
    m_renderer = 0;
    m_model = 0;
    m_reader = 0;
    m_x = 0;
    m_y = 0;
    m_normalX = 0;
    m_scrollTarget = 0;
    m_focusX = 0;
    m_effect = 0;
    m_depth = 0;
    m_selected = 0;
    m_texture = 0;
    m_focusTexture = 0;
    m_capTexture = 0;
    m_image = 0;
    m_focusImage = 0;
    m_tile = 0;
    m_imageOffX = 0;
    m_imageOffY = 0;
    m_buttonX = -1;
    m_buttonY = -1;
    m_tileset = -1;
    m_width = -1;
    m_height = -1;
    m_alpha = 1.0f;
    m_alphaTarget = 1.0f;
    m_normalAlpha = 1.0f;
    m_focusAlpha = 1.0f;
    m_alphaStep = 0.01f;
    memset(&m_bgColor, 0, 16);
}

// 0x002C1230.
VIGUIObject::~VIGUIObject()
{
    memset(m_name, 0, 32);
    m_type = 0;
    m_renderer = 0;
    if (m_model)
    {
        delete m_model;
        m_model = 0;
    }
    m_x = 0;
    m_y = 0;
    m_width = 0;
    m_height = 0;
}

// 0x002C12C0.  The blend mode is never read: the material is made blending.
void VIGUIObject::FillRect(const VIVect2I *r, const VIColor32F &c, VIAlphaBlendMode m)
{
    void *app = m_renderer->GetAppPtr();
    VIRaster *raster;
    if (!app || !(raster = ST_RASTER(app)))
        return;
    if (sSolidMaterial == -1)
    {
        if ((sSolidMaterial = raster->CreateMaterial(1)) < 0)
            return;
        VIMaterial *mat = raster->Material(sSolidMaterial);
        mat->InitSolidFill();
        mat->SetLayerBlendMode(0, VIAlphaBlendMode3);
    }
    if (sSolidMaterial == -1)
        return;
    raster->SetMaterial(sSolidMaterial);
    VIVect2I tr(r[1].x, r[0].y);
    VIVect2I bl(r[0].x, r[1].y);
    raster->BeginTriFan();
    raster->Color(c);
    raster->Vertex(r[0]);
    raster->Vertex(tr);
    raster->Vertex(r[1]);
    raster->Vertex(bl);
    raster->EndTriFan();
}

// 0x002C13F0.
void VIGUIObject::SetType(int type)
{
    m_type = type;
}

// 0x002C13F8.
int VIGUIObject::GetType()
{
    return m_type;
}

// 0x002C1400.
void VIGUIObject::SetRenderer(VIWndGenericRenderer *r)
{
    m_renderer = r;
}

// 0x002C1408.
void VIGUIObject::SetReader(VIXMLParser *reader)
{
    m_reader = reader;
}

// 0x002C1410.
void VIGUIObject::SetName(const char *name)
{
    if (name)
        strncpy(m_name, name, 31);
}

// 0x002C1430.
void VIGUIObject::SetAlphaStep(float f)
{
    m_alphaStep = f;
}

// 0x002C1438.
const char *VIGUIObject::GetName()
{
    return m_name;
}

// 0x002C1440.
void VIGUIObject::SetXTarget(short x)
{
    m_focusX = x;
}

// 0x002C1448.
void VIGUIObject::SetSelectEffect(int e)
{
    m_effect = e;
}

// 0x002C1450.  The scroll effect comes back to where the object was put.
void VIGUIObject::SetPos(short x, short y)
{
    m_x = x;
    m_y = y;
    m_normalX = x;
}

// 0x002C1468.
short VIGUIObject::GetX()
{
    return m_x;
}

// 0x002C1470.
short VIGUIObject::GetY()
{
    return m_y;
}

// 0x002C1478.
void VIGUIObject::SetBGColor(VIColor32F c)
{
    m_bgColor = c;
}

// 0x002C14C8.
void VIGUIObject::SetTileset(int t)
{
    m_tileset = t;
}

// 0x002C14D0.
void VIGUIObject::SetTileMode(int t)
{
    m_tile = t;
}

// 0x002C14D8.
void VIGUIObject::SetSize(short w, short h)
{
    m_width = w;
    m_height = h;
}

// 0x002C14E8.
void VIGUIObject::SetButtonPos(short x, short y)
{
    m_buttonX = x;
    m_buttonY = y;
}

// 0x002C14F8.
short VIGUIObject::GetButtonX()
{
    return m_buttonX;
}

// 0x002C1500.
short VIGUIObject::GetButtonY()
{
    return m_buttonY;
}

// 0x002C1508.
void VIGUIObject::SetImageOffset(short x, short y)
{
    m_imageOffX = x;
    m_imageOffY = y;
}

// 0x002C1518.
short VIGUIObject::GetImageOffsetY()
{
    return m_imageOffY;
}

// 0x002C1520.  The same field again.
short VIGUIObject::GetImageOffsetY2()
{
    return m_imageOffY;
}

// 0x002C1528.
void VIGUIObject::SetDepth(unsigned char d)
{
    m_depth = d;
}

// 0x002C1530.
unsigned char VIGUIObject::GetDepth()
{
    return m_depth;
}

// 0x002C1538.
void VIGUIObject::SetTexture(int id)
{
    m_texture = id;
}

// 0x002C1540.
void VIGUIObject::SetFocusTexture(int id)
{
    m_focusTexture = id;
}

// 0x002C1548.
void VIGUIObject::SetCapTexture(int id)
{
    m_capTexture = id;
}

// 0x002C1550.
void VIGUIObject::SetImage(unsigned int hash)
{
    m_image = hash;
}

// 0x002C1558.
void VIGUIObject::SetFocusImage(int id)
{
    m_focusImage = id;
}

// 0x002C1560.
void VIGUIObject::SetPulse(float f)
{
    m_alphaTarget = f;
}

// 0x002C1568.  The fade effect: toward the focused or the normal alpha.
void VIGUIObject::UpdateAlpha()
{
    switch (m_effect)
    {
    case 2:
        m_alphaTarget = m_selected ? m_focusAlpha : m_normalAlpha;
    case 3:
        if (m_alpha != m_alphaTarget)
        {
            if (m_alpha < m_alphaTarget)
                m_alpha += m_alphaStep;
            else
                m_alpha -= m_alphaStep;
        }
        break;
    }
}

// 0x002C15E0.  The scroll effect: a pixel a frame toward the focused or the
// normal x.
void VIGUIObject::UpdateScroll()
{
    switch (m_effect)
    {
    case 0:
        break;
    case 1:
        if (m_selected)
            m_scrollTarget = m_focusX;
        else
            m_scrollTarget = m_normalX;
        if (m_x != m_scrollTarget)
        {
            if (m_x < m_scrollTarget)
                m_x++;
            else
                m_x--;
        }
        break;
    }
}

// 0x002C1640.
void VIGUIObject::SetAlphaTarget(float f)
{
    m_focusAlpha = f;
}

// 0x002C1648.
void VIGUIObject::SetAlpha(float f)
{
    m_alpha = f;
    m_normalAlpha = f;
}

// 0x002C1658.  The texture (tiled as m_tile says), else the tileset frame, else
// the background colour, else the image.  `mode` is a local: cse turns each
// argument back into an `li`, but the m_tile == 3 test reuses its register,
// which is why that 3 is loaded at the top.
void VIGUIObject::Draw(float)
{
    void *app = m_renderer->GetAppPtr();
    if (!app)
        return;
    int tex = m_texture;
    if (m_selected && m_focusTexture)
        tex = m_focusTexture;
    UpdateAlpha();
    UpdateScroll();
    VIUI *ui = ST_UI(app);
    int x = m_x + m_imageOffX;
    int y = m_y + m_imageOffY;
    float alpha = m_alpha;
    VIAlphaBlendMode mode = VIAlphaBlendMode3;
    if (tex)
    {
        if (m_tile == 0)
            ui->BlitEx((VIUITexId)tex, VIVect2I(x, y), mode, alpha, 0);
        else if (m_tile == 1)
        {
            if (!m_capTexture)
                ui->BlitHorz(VIVect2I(x, y), (VIUITexId)tex, m_width, mode, alpha, 0);
            else
                ui->BlitBar("", VIColor32F(), VIVect2I(x, y), (VIUITexId)m_capTexture, (VIUITexId)tex, m_width,
                            mode, alpha, 0, 1);
        }
        else if (m_tile == 2)
            ui->BlitVert(VIVect2I(x, y), (VIUITexId)tex, m_height, mode, alpha, 0);
        else if (m_tile == 3)
            ui->BlitRect((VIUITexId)tex, VIVect2I(x, y), VIVect2I(m_width, m_height),
                         mode, alpha);
    }
    else if (m_tileset != -1)
    {
        if (m_width != -1 && m_height != -1)
            ui->BlitFillFrame(VIVect2I(x, y), VIVect2I(m_width, m_height),
                              sTilesetFrame[m_tileset], sTilesetFill[m_tileset],
                              mode, 1.0f, 1);
    }
    else if (m_bgColor.a > 0.001f)
    {
        VIVect2I r[2];
        r[0].Set(x, y);
        r[1] = VIVect2I(x + m_width, y + m_height);
        FillRect(r, m_bgColor, mode);
    }
    else
    {
        int image = m_image;
        if (m_selected && m_focusImage != -1)
            image = m_focusImage;
        ui->Blit(image, VIVect2I(x, y), mode, 1.0f, 0);
    }
}

// 0x002C19E0.
VIGUIDataModel *VIGUIObject::GetModel()
{
    return m_model;
}

// 0x002C19E8.
void VIGUIObject::SetSelected(int f)
{
    m_selected = f;
}

// 0x002C19F0.
int VIGUIObject::IsSelected()
{
    return m_selected;
}
