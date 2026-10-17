// viadmin.cpp (name ours) -- CLIENT 0x00BB4A80 .. 0x00BB7DA8, the first TU in CLIENT's
// link order: the auction query's stream operators, then VIClientPart382E8, the GM/debug
// command channel (VIClient+0x382E8).  Every command is one 0x0701 message: a sub-op
// byte and its fields.  No asserts, so no #line: the original file name is unknown.
// The TU ends with its template copies (VIMap<int, 4>, VIArray<8>) and VIItemList::Item.
#include "game.h"

// 0x00BB4A80
VIMemStream &operator<<(VIMemStream &s, const VIAuctionQuery &q)
{
    s.WriteByte(q.m_type);
    s.WriteString(q.m_name);
    s.WriteString(q.m_text);
    s.WriteU32(q.m_p9C);
    s.WriteU32(q.m_pA0);
    s.WriteSigned(q.m_pA4);
    s.WriteSigned(q.m_pA8);
    s.WriteSigned(q.m_pAC);
    s.WriteSigned(q.m_pB0);
    s.WriteSigned(q.m_pB4);
    s.WriteSigned(q.m_pB8);
    s << VIMemStreamFlag(q.m_pBC);
    return s;
}

// 0x00BB4B80
VIMemStream &operator>>(VIMemStream &s, VIAuctionQuery &q)
{
    s.Read(&q.m_type, 1, 1);
    s.ReadString(q.m_name, 24);
    s.ReadString(q.m_text, 128);
    s.Read(&q.m_p9C, 4, 1);
    s.Read(&q.m_pA0, 4, 1);
    s.ReadSigned(q.m_pA4);
    s.ReadSigned(q.m_pA8);
    s.ReadSigned(q.m_pAC);
    s.ReadSigned(q.m_pB0);
    s.ReadSigned(q.m_pB4);
    s.ReadSigned(q.m_pB8);
    q.m_pBC = s.ReadBool();
    return s;
}

// 0x00BB4C70
VIClientPart382E8::VIClientPart382E8(VIClient *client)
{
    m_client = client;
    m_loss = 0;
    m_latency = 0;
    m_jitter = 0;
    m_p0C = 0;
    m_p10 = 0;
    m_enabled = 0;
}

// 0x00BB4C98
VIClientPart382E8::~VIClientPart382E8()
{
}

// 0x00BB4CC0
void VIClientPart382E8::SendText13(const unsigned short *text)
{
    VIMemStream out;
    unsigned short op = 0x0701;
    out.Write(&op, 2, 1);
    out.WriteByte(13);
    out.WriteString(text);
    VINetSend(out, m_client->GetConn(251));
}

// 0x00BB4DA0
int VIClientPart382E8::IsCommand(unsigned int id)
{
    return (id & 0xFF000000) == 0xA1000000;
}

// 0x00BB4DB8
int VIClientPart382E8::IsCommand2(unsigned int id)
{
    return (id & 0xFF000000) == 0xA2000000;
}

// 0x00BB4DD0
void VIClientPart382E8::Show(int id, int x)
{
    switch (id)
    {
    case 0xA10B0000:
    {
        int v = x >= 0 ? x : 0;
        if (v > 50)
            v = 50;
        m_loss = v;
        SendSimulation();
        break;
    }
    case 0xA10C0000:
    {
        int v = x >= 0 ? x : 0;
        if (v > 8192)
            v = 8192;
        m_latency = v;
        SendSimulation();
        break;
    }
    case 0xA10D0000:
    {
        int v = x >= 0 ? x : 0;
        if (v > 8192)
            v = 8192;
        m_loss = v;
        SendSimulation();
        break;
    }
    case 0xA1010000:
        Send16(m_client->m_target);
        break;
    case 0xA1020000:
        Send16(m_client->m_selfId);
        break;
    case 0xA1030000:
        Send17(m_client->m_target);
        break;
    case 0xA1040000:
        Send17(m_client->m_selfId);
        break;
    case 0xA1050000:
        Send14();
        break;
    case 0xA10A0000:
        Toggle19();
        break;
    case 0xA1090000:
        Send15();
        break;
    case 0xA10E0000:
        Send01();
        break;
    case 0xA10F0000:
        m_client->OpenPanel27();
        break;
    case 0xA1100000:
        Send02(m_client->m_target, x);
        break;
    case 0xA1110000:
        m_client->ShowNameClear(0xA1110000, 656);
        break;
    case 0xA1250000:
        m_client->ShowNameClear(0xA1250000, 656);
        break;
    case 0xA1220000:
        m_client->ShowNameClear(0xA1220000, 741);
        break;
    case 0xA1120000:
        m_client->ShowNameClear(0xA1120000, 657);
        break;
    case 0xA1130000:
        Send05();
        break;
    case 0xA1140000:
        m_client->ShowNameClear(0xA1140000, 660);
        break;
    case 0xA1150000:
    case 0xA1160000:
    case 0xA1230000:
    case 0xA1240000:
        m_client->ShowNameClear(id, 661);
        break;
    case 0xA1170000:
        m_client->ShowTargetPlayerName(0xA1170000, 689);
        break;
    case 0xA1180000:
        m_client->ShowTargetPlayerName(0xA1180000, 690);
        break;
    case 0xA1340000:
        m_client->ShowTargetPlayerName(0xA1340000, 1468);
        break;
    case 0xA1190000:
        m_client->ShowTargetPlayerName(0xA1190000, 691);
        break;
    case 0xA11A0000:
        m_client->ShowTargetPlayerName(0xA11A0000, 692);
        break;
    case 0xA1200000:
        m_client->ShowTargetPlayerName(0xA1200000, 729);
        break;
    case 0xA1210000:
        m_client->ShowTargetPlayerName(0xA1210000, 728);
        break;
    case 0xA1080000:
        m_client->ShowNameClear(0xA1080000, 155);
        break;
    case 0xA1260000:
        Send32();
        break;
    case 0xA1270000:
        m_client->ShowNameClear(0xA1270000, 1030);
        break;
    case 0xA1280000:
        Send34(m_client->m_target);
        break;
    case 0xA1290000:
        Send37();
        break;
    case 0xA1300000:
        Send40(m_client->m_target);
        break;
    case 0xA1310000:
        Send38(m_client->m_target);
        break;
    case 0xA1320000:
        Send39(m_client->m_target);
        break;
    case 0xA1330000:
        m_client->ShowTargetPlayerName(0xA1330000, 1038);
        break;
    case 0xA1350000:
        Send52(m_client->m_target);
        break;
    }
}

// 0x00BB5400
void VIClientPart382E8::DoText(int id, const unsigned short *text)
{
    if (!text)
        return;
    int len = VIString::strlen16(text);
    char *buf = (char *)__builtin_alloca(len + 1);
    VIString::str16to8(buf, len + 1, text);
    switch (id)
    {
    case 0xA1080000:
        SendText13(text);
        break;
    case 0xA1170000:
        Send09(buf);
        break;
    case 0xA1180000:
        Send10(buf);
        break;
    case 0xA1340000:
        Send51(buf);
        break;
    case 0xA1190000:
        Send11(buf);
        break;
    case 0xA11A0000:
        Send12(buf);
        break;
    case 0xA1200000:
        Send20(buf);
        break;
    case 0xA1210000:
        Send21(buf);
        break;
    case 0xA1110000:
    {
        int n = atoi(buf);
        if (n)
            Send03(n);
        else
            m_client->ShowMsg(816);
        break;
    }
    case 0xA1111000:
        Send53(buf);
        break;
    case 0xA1360000:
        Send54(buf);
        break;
    case 0xA1370000:
        Send55(buf);
        break;
    case 0xA1250000:
    {
        int n = atoi(buf);
        if (n)
            Send25(m_client->m_target, n);
        else
            m_client->ShowMsg(816);
        break;
    }
    case 0xA1220000:
    {
        int n = atoi(buf);
        if (n)
            Send22(n);
        else
            m_client->ShowMsg(817);
        break;
    }
    case 0xA1120000:
        Send04(buf);
        break;
    case 0xA1140000:
        Send06(buf);
        break;
    case 0xA1150000:
    {
        int n = atoi(buf);
        if (n)
            Send07(n);
        else
            m_client->ShowMsg(818);
        break;
    }
    case 0xA1160000:
    {
        int n = atoi(buf);
        if (n)
            Send08(n);
        else
            m_client->ShowMsg(818);
        break;
    }
    case 0xA1230000:
    {
        int n = atoi(buf);
        if (n)
            Send23(n);
        else
            m_client->ShowMsg(818);
        break;
    }
    case 0xA1240000:
    {
        int n = atoi(buf);
        if (n)
            Send24(n);
        else
            m_client->ShowMsg(818);
        break;
    }
    case 0xA1270000:
        if (strlen(buf) > 12)
            m_client->ShowMsg(1031);
        else
            Send33(m_client->m_target, buf);
        break;
    case 0xA1330000:
        Send41(buf);
        break;
    }
}

// 0x00BB5830
void VIClientPart382E8::Do2(unsigned int id, int x)
{
    if (id == 0xA2010000)
    {
        VIMemStream out;
        unsigned short op = 0x0702;
        out.Write(&op, 2, 1);
        out.WriteByte(100);
        unsigned char b = x;
        out.Write(&b, 1, 1);
        VINetSend(out, m_client->GetConn(251));
    }
}

// 0x00BB5928
void VIClientPart382E8::ApplySimulation(connection_t *c)
{
    connection_set_simulated(c, m_loss, m_latency, m_jitter);
}

// 0x00BB5958
int VIClientPart382E8::GetP0C()
{
    return m_p0C;
}

// 0x00BB5960
void VIClientPart382E8::Reset()
{
    m_p0C = 0;
}

#define SEND_STRING(name, sub)                                  \
void VIClientPart382E8::name(const char *s)                     \
{                                                               \
    VIMemStream out;                                            \
    unsigned short op = 0x0701;                                 \
    out.Write(&op, 2, 1);                                       \
    out.WriteByte(sub);                                         \
    out.WriteString(s);                                         \
    VINetSend(out, m_client->GetConn(251));                     \
}
#define SEND_NONE(name, sub)                                    \
void VIClientPart382E8::name()                                  \
{                                                               \
    VIMemStream out;                                            \
    unsigned short op = 0x0701;                                 \
    out.Write(&op, 2, 1);                                       \
    out.WriteByte(sub);                                         \
    VINetSend(out, m_client->GetConn(251));                     \
}
#define SEND_INT(name, sub)                                     \
void VIClientPart382E8::name(int a)                             \
{                                                               \
    VIMemStream out;                                            \
    unsigned short op = 0x0701;                                 \
    out.Write(&op, 2, 1);                                       \
    out.WriteByte(sub);                                         \
    out.WriteU32(a);                                            \
    VINetSend(out, m_client->GetConn(251));                     \
}
#define SEND_INT2(name, sub)                                    \
void VIClientPart382E8::name(int a, int b)                      \
{                                                               \
    VIMemStream out;                                            \
    unsigned short op = 0x0701;                                 \
    out.Write(&op, 2, 1);                                       \
    out.WriteByte(sub);                                         \
    out.WriteU32(a);                                            \
    out.WriteU32(b);                                            \
    VINetSend(out, m_client->GetConn(251));                     \
}

SEND_STRING(Send04, 4)       // 0x00BB5968
SEND_STRING(Send06, 6)       // 0x00BB5A48
SEND_STRING(Send53, 53)      // 0x00BB5B28
SEND_STRING(Send54, 54)      // 0x00BB5C08
SEND_STRING(Send55, 55)      // 0x00BB5CE8

// 0x00BB5DC8
int VIClientPart382E8::IsEnabled()
{
    return m_enabled;
}

// 0x00BB5DD0
void VIClientPart382E8::Recv0700(VIMemStream &s)
{
    unsigned char op;
    s.Read(&op, 1, 1);
    switch (op)
    {
    case 35:
        RecvEnabled(s);
        break;
    case 50:
        RecvPrompt(s);
        break;
    }
}

// 0x00BB5E50: the fields are ONE chain of const-reference temporaries (each its own
// 16-byte BLKmode slot); the opcode's is freed by the time GetConn's result needs one.
void VIClientPart382E8::SendSimulation()
{
    VIMemStream out;
    out << VIMemStreamU16(0x0701) << VIMemStreamU8(18) << VIMemStreamU8(m_loss)
        << VIMemStreamU16(m_latency) << VIMemStreamU16(m_jitter);
    VINetSend(out, m_client->GetConn(251));
}

// 0x00BB5F78
void VIClientPart382E8::Send16(unsigned int id)
{
    if (id == 0xFFFFFFFF)
        return;
    VIMemStream out;
    unsigned short op = 0x0701;
    out.Write(&op, 2, 1);
    out.WriteByte(16);
    out.WriteU32(id);
    VINetSend(out, m_client->GetConn(251));
}

// 0x00BB6070
void VIClientPart382E8::Send17(unsigned int id)
{
    if (id == 0xFFFFFFFF)
        return;
    VIMemStream out;
    unsigned short op = 0x0701;
    out.Write(&op, 2, 1);
    out.WriteByte(17);
    out.WriteU32(id);
    VINetSend(out, m_client->GetConn(251));
}

SEND_NONE(Send14, 14)        // 0x00BB6168

// 0x00BB6238
void VIClientPart382E8::Toggle19()
{
    m_p0C ^= 1;
    VIMemStream out;
    unsigned short op = 0x0701;
    out.Write(&op, 2, 1);
    out.WriteByte(19);
    VINetSend(out, m_client->GetConn(251));
}

SEND_NONE(Send15, 15)        // 0x00BB6318
SEND_NONE(Send01, 1)         // 0x00BB63E8
SEND_INT2(Send02, 2)         // 0x00BB64B8
SEND_INT(Send03, 3)          // 0x00BB65C8
SEND_INT2(Send25, 25)        // 0x00BB66B8
SEND_INT(Send22, 22)         // 0x00BB67C8
SEND_NONE(Send05, 5)         // 0x00BB68B8
SEND_INT(Send07, 7)          // 0x00BB6988
SEND_INT(Send08, 8)          // 0x00BB6A78
SEND_INT(Send23, 23)         // 0x00BB6B68
SEND_INT(Send24, 24)         // 0x00BB6C58
SEND_STRING(Send09, 9)       // 0x00BB6D48
SEND_STRING(Send10, 10)      // 0x00BB6E28
SEND_STRING(Send51, 51)      // 0x00BB6F08
SEND_STRING(Send11, 11)      // 0x00BB6FE8
SEND_STRING(Send12, 12)      // 0x00BB70C8
SEND_STRING(Send20, 20)      // 0x00BB71A8
SEND_STRING(Send21, 21)      // 0x00BB7288
SEND_NONE(Send32, 32)        // 0x00BB7368

// 0x00BB7438
void VIClientPart382E8::Send33(int a, const char *s)
{
    VIMemStream out;
    unsigned short op = 0x0701;
    out.Write(&op, 2, 1);
    out.WriteByte(33);
    out.WriteU32(a);
    out.WriteString(s);
    VINetSend(out, m_client->GetConn(251));
}

SEND_INT(Send34, 34)         // 0x00BB7540
SEND_NONE(Send37, 37)        // 0x00BB7630
SEND_INT(Send40, 40)         // 0x00BB7700
SEND_INT(Send38, 38)         // 0x00BB77F0
SEND_INT(Send39, 39)         // 0x00BB78E0
SEND_STRING(Send41, 41)      // 0x00BB79D0
SEND_INT(Send52, 52)         // 0x00BB7AB0
SEND_NONE(Send0701a, 48)     // 0x00BB7BA0
SEND_NONE(Send0701b, 49)     // 0x00BB7C70

// 0x00BB7D40
void VIClientPart382E8::RecvEnabled(VIMemStream &s)
{
    s >> m_enabled;
}

// 0x00BB7D70
void VIClientPart382E8::RecvPrompt(VIMemStream &s)
{
    unsigned short name[32];
    s.ReadString(name, 64);
    m_client->PromptWithName(name);
}
