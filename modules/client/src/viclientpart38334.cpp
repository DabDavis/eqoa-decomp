// The who list client, VIClient+0x38334 (game.h, VIClientPart38334; vtable slot 11),
// CLIENT 0x00C02FB0 .. 0x00C03A10: requests on 0x0E01, replies on 0x0E02, and the 32-byte
// row's stream operators.  The file name is ours.
#include "game.h"

// A value written through a REFERENCE (see viclientpart2efd0.cpp).
template <class T> static inline void WriteRef(VIMemStream &s, const T &v)
{
    s.Write(&v, sizeof(T), 1);
}

// 0x00C02FB0
VIClientPart38334::VIClientPart38334(VIClient *owner)
    : m_owner(owner), m_serial(0)
{
    m_pending = 0;
}

// 0x00C02FE8
VIClientPart38334::~VIClientPart38334()
{
}

// 0x00C03038
void VIClientPart38334::Open()
{
    Close();
    {
        VIMemStream s;
        s.WriteU16(0x0E01);
        unsigned char sub = 1;
        WriteRef(s, sub);
        VINetSend(s << VIMemStreamInt(m_serial), m_owner->GetConn(251));
    }
    m_pending = 1;
}

// 0x00C03128
VIVector<VIRec32_C035B0> *VIClientPart38334::GetList()
{
    return &m_list;
}

// 0x00C03130
void VIClientPart38334::Close()
{
    m_list.Clear();
    m_pending = 0;
    m_serial++;
}

// 0x00C03168
void VIClientPart38334::Recv0E02(VIMemStream &s)
{
    unsigned char op;
    s.Read(&op, 1, 1);
    if (op == 1)
        Recv01(s);
}

// 0x00C031C8: a reply to an older request is dropped.
void VIClientPart38334::Recv01(VIMemStream &s)
{
    VIVector<VIRec32_C035B0> list;
    int serial;
    s >> VIMemStreamInt(serial);
    if (serial != m_serial)
        return;
    s >> list;
    for (int i = 0; i < list.m_size; i++)
        m_list.Add(list.m_data[i]);
    m_pending = 0;
}

// 0x00C038E0
VIMemStream &operator<<(VIMemStream &s, const VIRec32_C035B0 &r)
{
    s << VIMemStreamByte(r.m_x00);
    s << VIMemStreamByte(r.m_x01);
    s << VIMemStreamByte(r.m_x02);
    s.WriteString(r.m_name);
    s << VIMemStreamRaw(r.m_x1C);
    return s;
}

// 0x00C03988
VIMemStream &operator>>(VIMemStream &s, VIRec32_C035B0 &r)
{
    s.Read(&r.m_x00, 1, 1);
    s.Read(&r.m_x01, 1, 1);
    s.Read(&r.m_x02, 1, 1);
    s.ReadString(r.m_name, sizeof(r.m_name));
    s.Read(&r.m_x1C, 4, 1);
    return s;
}

// 0x00C03770, VIMemStreamVector.h:71 (the header carries the #line).
#include "vimemstreamvector.h"
template VIMemStream &operator>>(VIMemStream &, VIVector<VIRec32_C035B0> &);
