// viguitogglebutton.cpp (name ours) -- STATION 0x002C8B00 .. 0x002C98B8: an object
// over a toggle model, drawn with an extra texture while on.  Then the
// item-header template copies, the implicit destructor and
// __tf17VIGUIToggleButton.
#include "station.h"

// 0x002C8B00.
VIGUIToggleButton::VIGUIToggleButton(const char *name, VIWndGenericRenderer *r,
                                     VIXMLParser *reader)
    : VIGUIObject(name, 2, r, reader)
{
    m_toggleTex = 0;
    m_model = new VIGUIToggleDataModel;
}

// 0x002C8B60.
VIGUIToggleDataModel *VIGUIToggleButton::GetModel()
{
    return (VIGUIToggleDataModel *)m_model;
}

// 0x002C8B68.
void VIGUIToggleButton::SetToggleImage(int id)
{
    m_toggleTex = id;
}

// 0x002C8B70.  The object at full alpha, then the toggle texture over it.
void VIGUIToggleButton::Draw(float alpha)
{
    if (!m_renderer)
        return;
    void *app = m_renderer->GetAppPtr();
    if (!app)
        return;
    VIUI *ui = ST_UI(app);
    VIGUIToggleDataModel *model = GetModel();
    if (!model)
        return;
    VIGUIObject::Draw(1.0f);
    int x = m_x + m_imageOffX;
    int y = m_y + m_imageOffY;
    if (model->GetValue() && m_toggleTex)
        ui->Blit((VIUITexId)m_toggleTex, VIVect2I(x, y));
}

// 0x002C8C38.  Enter or the pad's accept flips it.
int VIGUIToggleButton::OnKeyDown(int key)
{
    VIGUIToggleDataModel *model = GetModel();
    if (!model)
        return 0;
    if (key == 13 || key == 134)
        model->Toggle();
    return 0;
}
