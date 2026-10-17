#line 1 "C:/code-frontiers-gm/game/VIClientPart4F078.cpp"
// VIClientPart4F078 (name provisional) -- CLIENT 0x00BEA670 .. 0x00BEA968: the faction
// standings at VIClient+0x4F078, then this TU's template copies (0x00BEA970 .. 0x00BEB6E8,
// verified duplicates and VIArray<VIRec132_BEB670>'s canonical members) and VIMemStreamArray.h's
// reader for the list (0x00BEB738).
#include "game.h"
#include "vimemstreamarray.h"

// 0x00BEA670
VIClientPart4F078::VIClientPart4F078(VIClient *c)
    : m_client(c), m_seq(0), m_ready(0), m_p14(-1)
{
    m_wnd = 0;
}

// 0x00BEA6B8: 0x0AC4 (2756).
void VIClientPart4F078::Request(int x)
{
    Reset();
    VIMemStream out;
    unsigned short o = 2756;
    out.Write(&o, 2, 1);
    VINetSend(out << VIMemStreamByte(102) << m_seq << x, m_client->GetConn(251));
}

// 0x00BEA7C0
void VIClientPart4F078::Reset()
{
    m_p14 = -1;
    m_ready = 0;
    m_seq++;
    m_list.Init(0);
}

// 0x00BEA800
int VIClientPart4F078::IsReady()
{
    return m_ready;
}

// 0x00BEA808
int VIClientPart4F078::GetP10()
{
    return m_p10;
}

// 0x00BEA810
int VIClientPart4F078::GetP14()
{
    return m_p14;
}

// 0x00BEA818
VIArray<VIRec132_BEB670> *VIClientPart4F078::GetList()
{
    return &m_list;
}

// 0x00BEA820
void VIClientPart4F078::Recv07E2(VIMemStream &s)
{
    unsigned char type;
    s.Read(&type, 1, 1);
    if (type == 119)
        RecvList(s);
}

// 0x00BEA880
void VIClientPart4F078::RecvList(VIMemStream &s)
{
    int seq;
    s.ReadSigned(seq);
    if (seq == m_seq)
    {
        s >> m_p10;
        s >> m_p14;
        s >> m_list;
        m_ready = 1;
    }
}

// 0x00BEA910
int VIClientPart4F078::IsFactionId(unsigned int id)
{
    return (id & 0xFF000000) == 0xAA000000;
}

// 0x00BEA928
void VIClientPart4F078::Command(unsigned int id, int arg)
{
    if (id == 0xAA000001)
        m_wnd->OpenFaction();
}

// 0x00BEA950
void VIClientPart4F078::Show()
{
    m_wnd->CloseFaction();
}
