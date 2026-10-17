#line 1 "C:/code-frontiers-gm/game/VIClientPart2EFC8.cpp"
// VIClientPart2EFC8 (name provisional) -- CLIENT 0x00BED878 .. 0x00BED9CC, then this TU's
// template copies (0x00BED9D0 .. 0x00BEE590, verified duplicates).
#include "game.h"

// 0x00BED878
VIClientPart2EFC8::VIClientPart2EFC8(VIClient *c)
{
    m_client = c;
}

// 0x00BED888
VIClientPart2EFC8::~VIClientPart2EFC8()
{
}

// 0x00BED8B0: 0x0B01.
void VIClientPart2EFC8::Send(int x)
{
    VIMemStream out;
    unsigned short o = 2817;
    out.Write(&o, 2, 1);
    out.WriteByte(1), out.WriteByte(x);
    VINetSend(out, GetConn());
}

// 0x00BED990
VIClientConn VIClientPart2EFC8::GetConn()
{
    return m_client->GetConn(251);
}
