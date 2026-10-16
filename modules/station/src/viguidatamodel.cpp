// viguidatamodel.cpp (name ours) -- STATION 0x002BD010 .. 0x002BD178: the value a
// GUI object edits, by name.  The vtable, __tf and the inline destructor follow.
#include "station.h"

// 0x002BD010.
VIGUIDataModel::VIGUIDataModel(int type)
{
    SetType(type);
}

// 0x002BD048.
VIGUIDataModel::VIGUIDataModel(const char *name, int type)
{
    SetName(name);
    SetType(type);
}

// 0x002BD098.
void VIGUIDataModel::Init()
{
    memset(m_name, 0, 32);
    SetType(0);
}

// 0x002BD0D0.
void VIGUIDataModel::SetName(const char *name)
{
    strncpy(m_name, name, 31);
}

// 0x002BD0F0.
char *VIGUIDataModel::GetName()
{
    return m_name;
}

// 0x002BD0F8.
void VIGUIDataModel::SetType(int type)
{
    m_type = type;
}

// 0x002BD100.
int VIGUIDataModel::GetType()
{
    return m_type;
}
