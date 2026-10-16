// viguitoggledatamodel.cpp (name ours) -- STATION 0x002BD428 .. 0x002BD5C0: an
// on/off value.  The implicit destructor and this class's __tf follow, then
// VIGUIDataModel's __tf and destructor again.
#include "station.h"

// 0x002BD428.
VIGUIToggleDataModel::VIGUIToggleDataModel()
    : VIGUIDataModel(2)
{
    Reset();
}

// 0x002BD468.
VIGUIToggleDataModel::VIGUIToggleDataModel(const char *name)
    : VIGUIDataModel(name, 2)
{
    Reset();
}

// 0x002BD4A8.
void VIGUIToggleDataModel::Reset()
{
    m_value = 0;
}

// 0x002BD4B0.
void VIGUIToggleDataModel::SetValue(int s)
{
    m_value = s;
}

// 0x002BD4B8.
int VIGUIToggleDataModel::GetValue()
{
    return m_value;
}

// 0x002BD4C0.
void VIGUIToggleDataModel::Toggle()
{
    m_value ^= 1;
}
