// vicsrecords.cpp (name ours) -- STATION 0x002EA680 .. 0x002EADA0, CLIENT
// 0x00BFC890 .. 0x00BFCFB0 (byte-identical): the customer-service records on the
// wire.  An inbox header's words go raw and its date through lane net's
// operators; a ticket or a line leads with a zigzag id.  The TU ends with
// VIVector<VICSSubcategory>'s copies and VIMemStreamVector.h's writer and reader for it.
#include "viclienti.h"
#include "vicsclient.h"

// 0x002EA680
VIMemStream &operator<<(VIMemStream &s, const VICSMessageHeader &h)
{
    s << VIMemStreamRaw(h.m_id) << h.m_date << VIMemStreamRaw(h.m_category)
      << VIMemStreamRaw(h.m_subcategory) << VIMemStreamBool(h.m_unread);
    return s;
}

// 0x002EA738
VIMemStream &operator>>(VIMemStream &s, VICSMessageHeader &h)
{
    s >> VIMemStreamRawRef(h.m_id) >> h.m_date >> VIMemStreamRawRef(h.m_category)
      >> VIMemStreamRawRef(h.m_subcategory) >> h.m_unread;
    return s;
}

// 0x002EA7D8
VIMemStream &operator<<(VIMemStream &s, const VICSCategory &c)
{
    s.WriteSigned(c.m_id);
    s << VIMemStreamBool(c.m_x04);
    s.WriteString(c.m_name);
    s << c.m_subs;
    return s;
}

// 0x002EA848
VIMemStream &operator>>(VIMemStream &s, VICSCategory &c)
{
    s.ReadSigned(c.m_id);
    s >> c.m_x04;
    s.ReadString(c.m_name, sizeof(c.m_name));
    s >> c.m_subs;
    return s;
}

// 0x002EA8B8
VIMemStream &operator<<(VIMemStream &s, const VICSSubcategory &c)
{
    s.WriteSigned(c.m_id);
    s << VIMemStreamBool(c.m_x04);
    s.WriteString(c.m_name);
    return s;
}

// 0x002EA918
VIMemStream &operator>>(VIMemStream &s, VICSSubcategory &c)
{
    s.ReadSigned(c.m_id);
    s >> c.m_x04;
    s.ReadString(c.m_name, sizeof(c.m_name));
    return s;
}

// 0x002EA980 .. 0x002EADA0: the template copies (implicit instantiations).
