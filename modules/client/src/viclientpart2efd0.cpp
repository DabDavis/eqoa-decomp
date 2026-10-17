// The gram (in-game mail) client, VIClient+0x2EFD0 (game.h, VIClientPart2EFD0; vtable
// slot 7), CLIENT 0x00BF5628 .. 0x00BF62C0, then its template copies to 0x00BF6E98.
// VICSClient's twin, function for function (vicsclient.cpp): requests go out on
// message 0x0D41 as a sub-op byte and its fields, inside a 0x0D01 envelope carrying the
// same sub-op and the request as a blob; replies come back on 0x0D41.  The file name
// is ours.
#include "game.h"

// A value written through a REFERENCE: the named byte goes into the stack when the
// inline binds it, at the frame's end at that moment (+48 after the opcode's +32
// temporary), and its address is substituted straight into the call's argument, so
// it is never hoisted (RequestMessage's search loop would otherwise pull it to the top).
template <class T> static inline void WriteRef(VIMemStream &s, const T &v)
{
    s.Write(&v, sizeof(T), 1);
}


// 0x00BF5628
VIClientPart2EFD0::VIClientPart2EFD0(VIClient *owner)
    : m_owner(owner), m_state(0), m_x08(0), m_count(-1)
{
}

// 0x00BF5648
VIClientPart2EFD0::~VIClientPart2EFD0()
{
}

// 0x00BF5670: the reply's sub-op picks the handler, which checks it was waited for.
void VIClientPart2EFD0::Recv0D41(VIMemStream &s)
{
    unsigned char op;
    s.Read(&op, 1, 1);
    switch (op)
    {
    case 2:
        RecvInbox(s);
        break;
    case 3:
        RecvMessage(s);
        break;
    case 1:
        RecvSend(s);
        break;
    case 4:
        RecvDelete(s);
        break;
    case 5:
        m_x08 = 1;
        break;
    }
}

// 0x00BF5738: after a reconnect, ask again for what was being waited for.
void VIClientPart2EFD0::OnConnect()
{
    switch (m_state)
    {
    case 1:
        RequestInbox();
        break;
    case 2:
        RequestMessage(m_message.m_id);
        break;
    case 3:
    case 4:
        m_state = 0;
        break;
    }
}

// 0x00BF57A0
void VIClientPart2EFD0::Reset()
{
    m_state = 0;
    m_count = -1;
}

// 0x00BF57B0.  The request and the envelope each have a NAMED sub-op byte (two
// permanent slots, +48 and +96); the opcodes go through the inline WriteU16, whose
// parameter is a temporary both writes share (+32).
void VIClientPart2EFD0::RequestInbox()
{
    m_state = 1;
    VIMemStream s;
    s.WriteU16(0x0D41);
    unsigned char sub = 2;
    WriteRef(s, sub);
    VIMemStream e;
    e.WriteU16(0x0D01);
    unsigned char esub = 2;
    WriteRef(e, esub);
    VINetSend(e << VIMemStreamBlob(s), m_owner->GetConn(251));
}

// 0x00BF5920
void VIClientPart2EFD0::RequestMessage(int id)
{
    for (int i = 0; i < m_count; i++)
        if (m_headers[i].m_id == id)
        {
            m_message = m_headers[i];
            break;
        }
    VIMemStream s;
    s.WriteU16(0x0D41);
    unsigned char sub = 3;
    WriteRef(s, sub);
    {
        VIMemStream e;
        e.WriteU16(0x0D01);
        unsigned char esub = 3;
        WriteRef(e, esub);
        VINetSend(e << VIMemStreamBlob(s) << id, m_owner->GetConn(251));
    }
    m_state = 2;
}

// 0x00BF5B08: the row goes at once; the server is told after.
void VIClientPart2EFD0::DeleteMessage(int id)
{
    int i;
    for (i = 0; i < m_count; i++)
        if (m_headers[i].m_id == id)
            break;
    for (m_count--; i < m_count; i++)
        m_headers[i] = m_headers[i + 1];
    VIMemStream s;
    s.WriteU16(0x0D41);
    unsigned char sub = 4;
    WriteRef(s, sub);
    {
        VIMemStream e;
        e.WriteU16(0x0D01);
        unsigned char esub = 4;
        WriteRef(e, esub);
        VINetSend(e << VIMemStreamBlob(s) << id, m_owner->GetConn(251));
    }
    m_state = 4;
}

// 0x00BF5DE8
void VIClientPart2EFD0::SendGram(const char *to, const unsigned short *subject,
                                 const unsigned short *text)
{
    m_state = 3;
    VIMemStream s;
    s.WriteU16(0x0D41);
    unsigned char sub = 1;
    WriteRef(s, sub);
    VIMemStream e;
    e.WriteU16(0x0D01);
    unsigned char esub = 1;
    WriteRef(e, esub);
    VINetSend(e << VIMemStreamBlob(s) << to << subject << text, m_owner->GetConn(251));
}

// 0x00BF5F98
int VIClientPart2EFD0::GetState()
{
    return m_state;
}

// 0x00BF5FA0
int VIClientPart2EFD0::GetX08()
{
    return m_x08;
}

// 0x00BF5FA8
int VIClientPart2EFD0::GetCount()
{
    return m_count;
}

// 0x00BF5FB0
VIGramHeader *VIClientPart2EFD0::GetHeader(int i)
{
    return &m_headers[i];
}

// 0x00BF5FC8
VIGramMessage *VIClientPart2EFD0::GetMessage()
{
    return &m_message;
}

// 0x00BF5FD0: at most the first 100 rows are kept, but the count is the list's.
void VIClientPart2EFD0::RecvInbox(VIMemStream &s)
{
    if (m_state != 1)
        return;
    unsigned char status;
    s.Read(&status, 1, 1);
    if (status != 2)
    {
        m_state = -6;
        return;
    }
    m_x08 = 0;
    m_state = 0;
    s >> m_count;
    for (int i = 0; i < m_count && i < 100; i++)
        s >> m_headers[i];
}

// 0x00BF60A8
void VIClientPart2EFD0::RecvMessage(VIMemStream &s)
{
    if (m_state != 2)
        return;
    unsigned char status;
    s.Read(&status, 1, 1);
    if (status != 3)
    {
        m_state = -7;
        return;
    }
    m_state = 0;
    int id;
    s >> id;
    if (id != m_message.m_id)
        return;
    for (int i = 0; i < m_count; i++)
        if (m_headers[i].m_id == id)
            m_headers[i].m_read = 1;
    s.ReadString(m_message.m_text, sizeof(m_message.m_text));
}

// 0x00BF6180
void VIClientPart2EFD0::RecvSend(VIMemStream &s)
{
    if (m_state != 3)
        return;
    unsigned char status;
    s.Read(&status, 1, 1);
    switch (status)
    {
    case 1:
    {
        VIUI &ui = m_owner->m_ui;
        m_owner->ShowText((const unsigned short *)ui.m_words.GetString(839, ui.m_language));
        m_state = 0;
        break;
    }
    case 9:
        m_state = -1;
        break;
    case 10:
        m_state = -2;
        break;
    case 11:
        m_state = -3;
        break;
    case 12:
        m_state = -4;
        break;
    default:
        m_state = -5;
        break;
    }
}

// 0x00BF6240
void VIClientPart2EFD0::RecvDelete(VIMemStream &s)
{
    if (m_state != 4)
        return;
    unsigned char status;
    s.Read(&status, 1, 1);
    if (status == 13)
    {
        VIUI &ui = m_owner->m_ui;
        m_owner->ShowText((const unsigned short *)ui.m_words.GetString(840, ui.m_language));
        m_state = 0;
    }
    else
        m_state = -8;
}
