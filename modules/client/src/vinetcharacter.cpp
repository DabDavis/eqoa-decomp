// The TU between VINet.cpp and VINetItem.cpp (CLIENT 0x00CBA850 .. 0x00CBA9D8, no
// template copies): the 0x000D dump's player block, and the stream operators of the
// 132-byte record VIClientPart3FC10's list holds.  The file name is ours (the original
// sorts between VINet.cpp and VINetItem.cpp).
#include "game.h"

// 0x00CBA850: Msg000D's player block (VIClient+0xC2DC).  +0x3C is not on the wire.
void VIPlayer::Read(VIMemStream &s)
{
    s.ReadSigned(m_id);
    s.ReadString(m_name, sizeof(m_name));
    s.ReadSigned(m_class);
    s.ReadSigned(m_race);
    s.ReadSigned(m_level);
    s.ReadSigned(m_exp);
    s.ReadSigned(m_p2C);
    s.Read(&m_p30, 1, 1);
    s.ReadSigned(m_p34);
    s.ReadSigned(m_p38);
    s.ReadSigned(m_p40);
    s.ReadSigned(m_p44);
    s.ReadSigned(m_world);
    s.Read(&m_pos, 4, 3);
    s.Read(&m_hpr, 4, 3);
}

// 0x00CBA950
VIMemStream &operator<<(VIMemStream &s, const VIRec132_BEB670 &r)
{
    s.WriteString(r.m_name);
    s.WriteSigned(r.m_value);
    return s;
}

// 0x00CBA990
VIMemStream &operator>>(VIMemStream &s, VIRec132_BEB670 &r)
{
    s.ReadString(r.m_name, sizeof(r.m_name));
    s.ReadSigned(r.m_value);
    return s;
}
