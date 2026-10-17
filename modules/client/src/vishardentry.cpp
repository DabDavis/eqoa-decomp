// The server-select entry (cl_tail.h, VIElem_BFC798): CLIENT 0x00CBBA98 .. 0x00CBBDD8,
// the TU after VIRandomMT's.  Its constructor is what VIArray<VIElem_BFC798>::Allocate
// builds each element with.  File name ours.
#include "game.h"

// 0x00CBBA98
VIElem_BFC798::VIElem_BFC798()
{
    m_x40 = 0;
    m_name[0] = 0;
    m_id = (unsigned long)-1;
    m_region = -1;
}

// 0x00CBBAB8
VIMemStream &operator<<(VIMemStream &s, const VIElem_BFC798 &e)
{
    s.WriteString(e.m_name);
    s.WriteBool(e.m_x40);
    s.WriteU64(e.m_id);
    s.WriteByte(e.m_region);
    return s;
}

// 0x00CBBB40
VIMemStream &operator>>(VIMemStream &s, VIElem_BFC798 &e)
{
    s.ReadString(e.m_name, sizeof(e.m_name));
    s >> e.m_x40;
    s.Read(&e.m_id, 8, 1);
    {
        signed char c;
        s.Read(&c, 1, 1);
        e.m_region = c;
    }
    return s;
}

const char *drdp_format_endpoint1(unsigned long id);   // SUPPORT (drdp.h; drdpapi.h lacks it)

// 0x00CBBBC8
const char *VIElem_BFC798::Address()
{
    return drdp_format_endpoint1(m_id);
}
