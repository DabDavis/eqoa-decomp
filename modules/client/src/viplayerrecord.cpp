// The record TU between VIFileStream.cpp and VIJournal.cpp (0x00CB7510 .. 0x00CB7C30):
// the 40-byte player record (game.h, VIPlayerRecord) and its streams, twelve keyed
// records' compare pairs, a 48-byte record's streams and five "first valid entry"
// accessors.  Everything but VIPlayerRecord's reader and constructor is unreferenced in
// the image.  The file name is provisional (the original falls between VIFileStream.cpp
// and VIJournal.cpp); the record names are ours (cl_tail.h).
#include "game.h"
VI_HOST_ONLY(extern "C" void *memcpy(void *, const void *, unsigned int);)  // gcc 2.95: implicit
// 0x00CB7510 .. 0x00CB7688: the compare pairs, `m_key < k` then `m_key > k`.
bool VIRecKey_CB7510::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB7510::operator>(const int &k) const { return m_key > k; }
bool VIRecKey_CB7530::operator>(const int &k) const { return m_key > k; }
bool VIRecKey_CB7530::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB7550::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB7550::operator>(const int &k) const { return m_key > k; }
bool VIRecKey_CB7570::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB7570::operator>(const int &k) const { return m_key > k; }
bool VIRecKey_CB7590::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB7590::operator>(const int &k) const { return m_key > k; }
bool VIRecKey_CB75B0::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB75B0::operator>(const int &k) const { return m_key > k; }
bool VIRecKey_CB75D0::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB75D0::operator>(const int &k) const { return 0; }
bool VIRecKey_CB75E8::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB75E8::operator>(const int &k) const { return m_key > k; }
bool VIRecKey_CB7608::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB7608::operator>(const int &k) const { return m_key > k; }
bool VIRecKey_CB7628::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB7628::operator>(const int &k) const { return m_key > k; }
bool VIRecKey_CB7648::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB7648::operator>(const int &k) const { return m_key > k; }
bool VIRecKey_CB7668::operator<(const int &k) const { return m_key < k; }
bool VIRecKey_CB7668::operator>(const int &k) const { return m_key > k; }

// 0x00CB7688
VIMemStream &operator<<(VIMemStream &s, const VIRec48_CB7688 &r)
{
    s.WriteString(r.m_name);
    s.WriteSigned(r.m_p20);
    s.WriteSigned(r.m_p24);
    s.WriteU32(r.m_p28);
    s.WriteU32(r.m_p2C);
    return s;
}

// 0x00CB7710
VIMemStream &operator>>(VIMemStream &s, VIRec48_CB7688 &r)
{
    s.ReadString(r.m_name, sizeof(r.m_name));
    s.ReadSigned(r.m_p20);
    s.ReadSigned(r.m_p24);
    s.Read(&r.m_p28, 4, 1);
    s.Read(&r.m_p2C, 4, 1);
    return s;
}

// 0x00CB7788
VIPlayerRecord::VIPlayerRecord()
{
    m_id = -1;
    m_class = -1;
    m_race = -1;
    m_level = -1;
    memset(m_name, 0, sizeof(m_name));
}

// 0x00CB77D0
VIPlayerRecord::VIPlayerRecord(const VIRec52_CB77F8 &r)
{
    *this = r;
}

// 0x00CB77F8
VIPlayerRecord &VIPlayerRecord::operator=(const VIRec52_CB77F8 &r)
{
    m_id = r.m_id;
    m_class = r.m_class;
    m_race = r.m_race;
    m_level = r.m_level;
    memcpy(m_name, r.m_name, sizeof(m_name));
    return *this;
}

// 0x00CB7850
VIMemStream &operator>>(VIMemStream &s, VIPlayerRecord &r)
{
    s >> r.m_id;
    s.ReadString(r.m_name, sizeof(r.m_name));
    s >> r.m_class;
    s >> r.m_race;
    s >> r.m_level;
    return s;
}

// 0x00CB78E8
VIMemStream &operator<<(VIMemStream &s, const VIPlayerRecord &r)
{
    s << r.m_id;
    s.WriteString(r.m_name);
    s << r.m_class;
    s << r.m_race;
    s << r.m_level;
    return s;
}

static unsigned short sNoName[8];    // 0x00CF2488: what the accessors return without a text
static unsigned short sNoDesc[8];    // 0x00CF2498
static unsigned short sNoName2[8];   // 0x00CF24A8
static unsigned short sNoName3[8];   // 0x00CF24B8
static unsigned short sNoDesc3[8];   // 0x00CF24C8

// 0x00CB7980
const unsigned short *VIItemDef::Name()
{
    for (int i = m_texts.Begin(); i != m_texts.End(); i = m_texts.Next(i))
        if (!m_texts[i].m_language)
            return m_texts[i].m_name;
    return sNoName;
}

// 0x00CB7A18
const unsigned short *VIItemDef::Desc()
{
    for (int i = m_texts.Begin(); i != m_texts.End(); i = m_texts.Next(i))
        if (!m_texts[i].m_language)
            return m_texts[i].m_desc;
    return sNoDesc;
}

// 0x00CB7AB0
const unsigned short *VIRecDef_CB7AB0::Name()
{
    for (int i = 0; i < m_texts.m_size; i++)
        if (!m_texts[i].m_language)
            return m_texts[i].m_name;
    return sNoName2;
}

// 0x00CB7B00
const unsigned short *VIRecDef_CB7B00::Name()
{
    for (int i = m_texts.Begin(); i != m_texts.End(); i = m_texts.Next(i))
        if (!m_texts[i].m_language)
            return m_texts[i].m_name;
    return sNoName3;
}

// 0x00CB7B98
const unsigned short *VIRecDef_CB7B00::Desc()
{
    for (int i = m_texts.Begin(); i != m_texts.End(); i = m_texts.Next(i))
        if (!m_texts[i].m_language)
            return m_texts[i].m_desc;
    return sNoDesc3;
}
