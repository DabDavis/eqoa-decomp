// The ignore list, VIClient+0x38300 (game.h, VIClientPart38300; vtable slot 9), CLIENT
// 0x00BFA450 .. 0x00BFAD18, then its template copies to 0x00BFC048.  The file name is
// ours.
#include "game.h"

extern "C" int   strcmp(const char *a, const char *b);
extern "C" char *strcpy(char *d, const char *s);

// 0x00BFA450
VIClientPart38300::VIClientPart38300(VIClient *owner)
{
    m_owner = owner;
    m_wnd = 0;
    m_list.Init(0);
}

// 0x00BFA4A8
VIClientPart38300::~VIClientPart38300()
{
    Reset();
}

// 0x00BFA4F8
int VIClientPart38300::Reset()
{
    m_list.Clear();
    return 0;
}

// 0x00BFA518
void VIClientPart38300::OpenPanel()
{
    if (m_wnd)
        m_wnd->OpenIgnoreList();
}

// 0x00BFA540
void VIClientPart38300::ClosePanel()
{
    if (m_wnd)
        m_wnd->CloseIgnoreList();
}

// 0x00BFA568
void VIClientPart38300::Send1001(int id)
{
    VIMemStream s;
    s.WriteU16(0x1001);
    VINetSend(s << VIMemStreamRaw(id), GetConn());
}

// 0x00BFA628
void VIClientPart38300::Add(const char *name)
{
    VIMemStream s;
    s.WriteU16(0x1000);
    VINetSend(s << name, GetConn());
}

// 0x00BFA6D8
void VIClientPart38300::RemoveAt(int i)
{
    VIMemStream s;
    s.WriteU16(0x1002);
    VINetSend(s << VIMemStreamRaw(m_list.Value(i).m_id), GetConn());
}

// 0x00BFA7A8
void VIClientPart38300::Remove(const char *name)
{
    for (int i = m_list.Begin(); i != m_list.End(); i = m_list.Next(i))
        if (VIString::strcasecmp(m_list.Value(i).m_name, name) == 0)
        {
            RemoveAt(i);
            return;
        }
}

// 0x00BFA850
void VIClientPart38300::Recv(unsigned short op, VIMemStream &s)
{
    char name[32];
    switch (op)
    {
    case 0x1003:
    {
        s.ReadString(name, 32);
        int id;
        s.Read(&id, 4, 1);
        int i = m_list.Add();
        VIString::strcpy8(m_list.Value(i).m_name, 32, name);
        m_list.Value(i).m_id = id;
        break;
    }
    case 0x1004:
    {
        int id;
        s.Read(&id, 4, 1);
        for (int i = m_list.Begin(); i != m_list.End(); i = m_list.Next(i))
            if (m_list.Value(i).m_id == id)
            {
                m_list.Erase(i);
                return;
            }
        break;
    }
    case 0x1005:
        ReadList(s);
        break;
    case 0x1006:
    {
        s.ReadString(name, 32);
        int id;
        s.Read(&id, 4, 1);
        for (int i = m_list.Begin(); i != m_list.End(); i = m_list.Next(i))
            if (m_list.Value(i).m_id == id)
            {
                strcpy(m_list.Value(i).m_name, name);
                return;
            }
        break;
    }
    }
}

// 0x00BFAA38
void VIClientPart38300::Recv2(VIMemStream &s)
{
    unsigned char a, b;
    s >> a >> b;
}

// 0x00BFAA80
VIClientConn VIClientPart38300::GetConn()
{
    return m_owner->GetConn(251);
}

// 0x00BFAAC0
void VIClientPart38300::ReadList(VIMemStream &s)
{
    int n;
    s.ReadSigned(n);
    for (int k = 0; k < n; k++)
    {
        char name[32];
        s.ReadString(name, 32);
        int id;
        s.Read(&id, 4, 1);
        int i = m_list.Add();
        VIString::strcpy8(m_list.Value(i).m_name, 32, name);
        m_list.Value(i).m_id = id;
    }
}

// 0x00BFAB90
void VIClientPart38300::GetIndices(VIArray<int> &a)
{
    a.Init(m_list.Size());
    int k = 0;
    for (int i = m_list.Begin(); i != m_list.End(); i = m_list.Next(i))
    {
        int &d = a.m_data[k++];
        d = i;
    }
}

// 0x00BFAC30
void VIClientPart38300::GetName(int i, char *dst)
{
    VIString::strcpy8(dst, 32, m_list.Value(i).m_name);
}

// 0x00BFAC60
int VIClientPart38300::Count()
{
    return m_list.Size();
}

// 0x00BFAC80
int VIClientPart38300::Find(const char *name)
{
    for (int i = m_list.Begin(); i != m_list.End(); i = m_list.Next(i))
        if (strcmp(m_list.Value(i).m_name, name) == 0)
            return i;
    return -1;
}
