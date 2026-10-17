#line 1 "C:/code-frontiers-gm/game/VIShardInfo.cpp"
// VIShardInfo.cpp -- CLIENT 0x00BFC048 .. 0x00BFC3E8, then its template copies to
// 0x00BFC890: the race camera and race info lookups, and the server-select (shard)
// list (game.h, VIShardInfo; VIClient+0x38320).  Its asserts sit on lines 166 and 175,
// so the file is padded to put them there.
#include "game.h"
#include "vimemstreamarray.h"

// 0x00BFC048: the LAST entry for the race (0 when out of range).
VIRaceCamera *GetRaceCamera(int race, int expansion)
{
    if ((unsigned int)race >= 21)
        race = 0;
    VIRaceCamera *found = 0;
    VIRaceCamera *table = (expansion & 2) ? gRaceCamerasEx : gRaceCameras;
    for (int i = 0; i < 21; i++)
        if (table[i].m_race == race)
            found = &table[i];
    return found;
}

// 0x00BFC0A8
VIRaceInfo *GetRaceInfo(int race, int expansion)
{
    if ((unsigned int)race >= 21)
        return gRaceInfo;
    VIRaceInfo *table = (expansion & 2) ? gRaceInfoEx : gRaceInfo;
    return &table[race];
}

// 0x00BFC0F0
VIShardInfo::VIShardInfo()
{
    m_region = -1;
}

// 0x00BFC128
int VIShardInfo::GetCount()
{
    return m_ServerSelectList.m_size;
}

// 0x00BFC130
int VIShardInfo::IsValid(int i)
{
    return m_ServerSelectList[i].m_id != -1;
}

// 0x00BFC150
VIElem_BFC798 *VIShardInfo::GetEntry(int i)
{
    return &m_ServerSelectList.m_data[i];
}

// 0x00BFC168
int VIShardInfo::FirstValid()
{
    for (int i = 0; i < m_ServerSelectList.m_size; i++)
        if (m_ServerSelectList[i].m_x40 && IsValid(i))
            return i;
    return -1;
}

// 0x00BFC1F0
long VIShardInfo::GetId(int i)
{
    return m_ServerSelectList[i].m_id;
}

// 0x00BFC208
void VIShardInfo::Read(VIMemStream &s)
{
    s >> m_shards;
    Filter();
}

// 0x00BFC240
void VIShardInfo::Clear()
{
    m_shards.Clear();
    m_ServerSelectList.Clear();
}

// 0x00BFC270
void VIShardInfo::SetRegion(int region)
{
    m_region = region;
    Filter();
}

// 0x00BFC290: the shards of the current region, in their order.





































































void VIShardInfo::Filter()
{
    int Match = 0;
    for (int i = 0; i < m_shards.m_size; i++)
        if (m_shards[i].m_region == m_region)
            Match++;
    VIASSERT(!m_ServerSelectList.Init( Match ));
    int j = 0;
    for (int i = 0; i < m_shards.m_size; i++)
    {
        if (m_shards[i].m_region == m_region)
        {
            m_ServerSelectList[j++] = m_shards[i];
        }
    }
    VIASSERT(j == Match);
}
