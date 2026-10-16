// vicsclient.cpp (name ours) -- STATION 0x002DDC48 .. 0x002DF918, CLIENT
// 0x00BE7120 .. 0x00BE8DF0 (byte-identical): the customer-service protocol.
// Requests go out on message 0x0AA1 as a sub-op byte and its fields; replies
// come back on 0x07B0, a sub-op byte and a status byte first.  The TU ends with
// its template copies: the item header's VIArray<VIItemStat>, then the three
// record vectors and VIMemStreamVector.h's reader for two of them.
#include "viclienti.h"
#include "vicsclient.h"

// 0x002DDC48
VICSClient::VICSClient(VIClientI *owner)
    : m_owner(owner), m_state(0), m_x08(0), m_count(-1)
{
}

// 0x002DDC88
VICSClient::~VICSClient()
{
}

// 0x002DDCD8: the reply's sub-op picks the handler, which checks it was waited for.
void VICSClient::Recv07B0(VIMemStream &s)
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
    case 0:
        RecvSubmitRequest(s);
        break;
    case 1:
        RecvSubmitReply(s);
        break;
    case 4:
        RecvDelete(s);
        break;
    case 5:
        m_x08 = 1;
        break;
    case 6:
        RecvCategories(s);
        break;
    }
}

// 0x002DDDC0: after a reconnect, ask again for what was being waited for.
void VICSClient::OnConnect()
{
    switch (m_state)
    {
    case 1:
        RequestInbox();
        break;
    case 2:
        RequestMessage(m_message.m_id);
        break;
    case 4:
        RemoveEntry(m_message.m_id);
    case 3:
    case 5:
        m_state = 0;
        break;
    }
}

// 0x002DDE40
void VICSClient::Reset()
{
    m_state = 0;
    m_count = -1;
}

// 0x002DDE50
void VICSClient::RequestInbox()
{
    m_state = 1;
    VIMemStream s;
    unsigned short op = 0x0AA1;
    s.Write(&op, 2, 1);
    s.WriteByte(2);
    VINetSend(s, m_owner->GetConn(251));
}

// 0x002DDF28
void VICSClient::RequestCategories()
{
    m_state = 6;
    VIMemStream s;
    unsigned short op = 0x0AA1;
    s.Write(&op, 2, 1);
    s.WriteByte(6);
    VINetSend(s, m_owner->GetConn(251));
}

// 0x002DE000
void VICSClient::RequestMessage(int id)
{
    int i = Find(id);
    m_message = m_headers[i];
    {
        VIMemStream s;
        unsigned short op = 0x0AA1;
        s.Write(&op, 2, 1);
        s.WriteByte(3);
        VINetSend(s << id, m_owner->GetConn(251));
    }
    m_state = 2;
}

// 0x002DE110
void VICSClient::DeleteMessage(int id)
{
    int i = Find(id);
    m_message = m_headers[i];
    {
        VIMemStream s;
        unsigned short op = 0x0AA1;
        s.Write(&op, 2, 1);
        s.WriteByte(4);
        VINetSend(s << id, m_owner->GetConn(251));
    }
    m_state = 4;
}

// 0x002DE220
void VICSClient::SubmitRequest(int category, int subcategory, const unsigned short *text)
{
    m_state = 3;
    VIMemStream s;
    unsigned short op = 0x0AA1;
    s.Write(&op, 2, 1);
    s.WriteByte(0);
    VINetSend(s << category << subcategory << text, m_owner->GetConn(251));
}

// 0x002DE340
void VICSClient::SubmitReply(int id, const unsigned short *text)
{
    m_state = 5;
    m_message = m_headers[Find(id)];
    VIMemStream s;
    unsigned short op = 0x0AA1;
    s.Write(&op, 2, 1);
    s.WriteByte(1);
    VINetSend(s << id << text, m_owner->GetConn(251));
}

// 0x002DE468
int VICSClient::GetX08()
{
    return m_x08;
}

// 0x002DE470
int VICSClient::GetCount()
{
    return m_count;
}

// 0x002DE478
VICSMessageHeader *VICSClient::GetHeader(int i)
{
    return &m_headers[i];
}

// 0x002DE490
int VICSClient::GetState()
{
    return m_state;
}

// 0x002DE498
VICSMessage *VICSClient::GetMessage()
{
    return &m_message;
}

// 0x002DE4A0
VIVector<VICSCategory> *VICSClient::GetCategories()
{
    return &m_categories;
}

// 0x002DE4A8
VICSMessage &VICSMessage::operator=(const VICSMessageHeader &h)
{
    VICSMessageHeader::operator=(h);
    m_text[0] = 0;
    return *this;
}

// 0x002DE4E8: the server's status byte as an error code.  The cases are
// listed in the order their bodies are laid out, which is not numeric.  255
// is a case of its own (its test is in the tree) that returns what the
// default does; as `case 255: break;` it drops out of the tree.
int VICSClient::StatusToError(unsigned char status)
{
    switch (status)
    {
    case 0:
        return 0;
    case 1:
        return -1;
    case 2:
        return -2;
    case 3:
        return -3;
    case 4:
        return -4;
    case 5:
        return -5;
    case 6:
        return -6;
    case 7:
        return -7;
    case 8:
        return -8;
    case 9:
        return -9;
    case 10:
        return -10;
    case 11:
        return -11;
    case 15:
        return -12;
    case 16:
        return -13;
    case 17:
        return -14;
    case 18:
        return -15;
    case 19:
        return -16;
    case 12:
        return -17;
    case 13:
        return -18;
    case 14:
        return -19;
    case 255:
        return -128;
    }
    return -128;
}

// 0x002DE6C0: at most the first 100 entries are kept, but the count is the list's.
void VICSClient::RecvInbox(VIMemStream &s)
{
    if (m_state != 1)
        return;
    unsigned char status;
    s.Read(&status, 1, 1);
    m_state = StatusToError(status);
    if (m_state)
        return;
    VIVector<VICSMessageHeader> list;
    int x;
    s.Read(&x, 4, 1);
    s >> list;
    for (int i = 0; i < list.m_size && i < 100; i++)
    {
        const VICSMessageHeader &h = list.m_data[i];
        m_headers[i] = h;
    }
    m_x08 = 0;
    m_count = list.m_size;
}

// 0x002DE7E0
void VICSClient::RecvCategories(VIMemStream &s)
{
    if (m_state != 6)
        return;
    unsigned char status;
    s.Read(&status, 1, 1);
    m_state = StatusToError(status);
    if (m_state)
        return;
    s >> m_categories;
}

// 0x002DE850
void VICSClient::RecvMessage(VIMemStream &s)
{
    if (m_state != 2)
        return;
    unsigned char status;
    s.Read(&status, 1, 1);
    m_state = StatusToError(status);
    if (m_state)
        return;
    s.ReadString(m_message.m_text, sizeof(m_message.m_text));
    m_headers[Find(m_message.m_id)].m_unread = 0;
}

// 0x002DE8E0
void VICSClient::RecvSubmitRequest(VIMemStream &s)
{
    if (m_state != 3)
        return;
    unsigned char status;
    s.Read(&status, 1, 1);
    m_state = StatusToError(status);
}

// 0x002DE930: a reply sent, the message is fetched again.
void VICSClient::RecvSubmitReply(VIMemStream &s)
{
    if (m_state != 5)
        return;
    unsigned char status;
    s.Read(&status, 1, 1);
    m_state = StatusToError(status);
    if (m_state)
        return;
    RequestMessage(m_message.m_id);
}

// 0x002DE990
void VICSClient::RecvDelete(VIMemStream &s)
{
    if (m_state != 4)
        return;
    unsigned char status;
    int id;
    s.Read(&status, 1, 1);
    s.ReadSigned(id);
    m_state = StatusToError(status);
    if (m_state)
        return;
    RemoveEntry(id);
}

// 0x002DEA08
void VICSClient::RemoveEntry(int id)
{
    int i = Find(id);
    for (m_count--; i < m_count; i++)
        m_headers[i] = m_headers[i + 1];
}

// 0x002DEAA0: m_count when it is not listed.
int VICSClient::Find(int id)
{
    int i;
    for (i = 0; i < m_count; i++)
        if (m_headers[i].m_id == id)
            break;
    return i;
}

// 0x002DEAE8 .. 0x002DF918: the template copies (implicit instantiations).
