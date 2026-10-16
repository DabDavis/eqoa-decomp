// viguistringdatamodel.cpp (name ours) -- STATION 0x002BD178 .. 0x002BD428: a
// string value with an optional length cap.  VIGUIDataModel's __tf and
// destructor are emitted here again, then this class's __tf.
#include "station.h"

// 0x002BD178.
VIGUIStringDataModel::VIGUIStringDataModel()
    : VIGUIDataModel(1)
{
    Reset();
}

// 0x002BD1B8.
VIGUIStringDataModel::VIGUIStringDataModel(const char *name)
    : VIGUIDataModel(name, 1)
{
    Reset();
}

// 0x002BD1F8.
void VIGUIStringDataModel::Reset()
{
    m_string = 0;
    m_maxLen = -1;
}

// 0x002BD208.
VIGUIStringDataModel::~VIGUIStringDataModel()
{
    if (m_string)
        free(m_string);
    m_maxLen = -1;
}

// 0x002BD278.
void VIGUIStringDataModel::SetString(const char *s)
{
    if (s)
    {
        int n = strlen(s);
        if (m_maxLen != -1 && m_maxLen < n)
            n = m_maxLen;
        if (m_string)
        {
            free(m_string);
            m_string = 0;
        }
        m_string = (char *)malloc(n + 1);
        memset(m_string, 0, n + 1);
        if (m_string)
            strncpy(m_string, s, n);
    }
}

// 0x002BD328.
const char *VIGUIStringDataModel::GetString()
{
    return m_string;
}

// 0x002BD330.
void VIGUIStringDataModel::SetMaxLength(int n)
{
    m_maxLen = n;
}

// 0x002BD338.
int VIGUIStringDataModel::GetMaxLength()
{
    return m_maxLen;
}
