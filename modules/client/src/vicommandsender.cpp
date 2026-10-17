// VIClientPart2EFCC (game.h): CLIENT 0x00CBBDD8 .. 0x00CBCBD8, the TU after the
// server-select entry's.  Two senders of message 0x0C01; the slash-command parser
// (func_00BCFE78) hands one the rest of a command.  File name ours.
#include "game.h"

// 0x00CBBDD8
VIClientPart2EFCC::VIClientPart2EFCC(VIClientI *client)
{
    m_client = client;
}

// 0x00CBBDE8
VIClientPart2EFCC::~VIClientPart2EFCC()
{
}

// 0x00CBBE10
void VIClientPart2EFCC::SendText(const unsigned short *text)
{
    VIMemStream out;
    unsigned short op = 0x0C01;
    out.Write(&op, 2, 1);
    out.WriteByte(2);
    out.WriteString(text);
    VINetSend(out, GetConn());
}

// 0x00CBBEE0
void VIClientPart2EFCC::SendFlag(int b)
{
    VIMemStream out;
    unsigned short op = 0x0C01;
    out.Write(&op, 2, 1);
    out.WriteByte(1);
    unsigned char c = b;
    out.Write(&c, 1, 1);
    VINetSend(out, GetConn());
}

// 0x00CBBFC0
VIClientConn VIClientPart2EFCC::GetConn()
{
    return m_client->GetConn(251);
}
