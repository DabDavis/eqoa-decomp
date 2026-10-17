#line 1 "C:/code-frontiers-gm/game/VIClientPart06D50.cpp"
// VIClientPart06D50 (name provisional) -- CLIENT 0x00BF2ED8 .. 0x00BF3644: the icon cache at
// VIClient+0x6D50, then this TU's template copies (0x00BF3648 .. 0x00BF5360): its own
// VIPool<VIIconSlot> and VIMap<unsigned int, int>, the canonical copies (they were
// client_vipool.cpp's VIPool<VIPtr_BF45A8 *> and client_containers.cpp's
// VIMap<unsigned int, VIRec4_BF41A8>, placeholders of the wrong size and type).
#include "game.h"

// 0x00BF2ED8
VIIconSlot::VIIconSlot()
{
    m_id = 0;
    m_surface = -1;
    m_load = -1;
}

// 0x00BF2EF0
VIClientPart06D50::VIClientPart06D50()
{
    m_loader = 0;
    m_raster = 0;
    m_dict = 0;
    m_scene = 0;
}

// 0x00BF2F38
VIClientPart06D50::~VIClientPart06D50()
{
    Clear();
}

// 0x00BF2F98
int VIClientPart06D50::Init(int n, VIScene &sc, VILoader &ld)
{
    if (Clear() < 0)
        return -1;
    m_raster = sc.m_raster;
    m_dict = sc.m_dict;
    m_loader = &ld;
    m_scene = &sc;
    m_n = n;
    if (m_slots.Init(n) < 0)
        return -1;
    if (m_index.Init(m_n) < 0)
        return -1;
    return 0;
}

// 0x00BF3038
int VIClientPart06D50::Clear()
{
    for (int i = m_slots.Begin(); i != m_slots.End(); i = m_slots.Next(i))
        if (Unload(m_slots.Value(i)) < 0)
            return -1;
    if (m_slots.Clear() < 0)
        return -1;
    if (m_index.Clear() < 0)
        return -1;
    m_loader = 0;
    m_raster = 0;
    m_dict = 0;
    m_scene = 0;
    return 0;
}

// 0x00BF3108
int VIClientPart06D50::Update()
{
    for (int i = m_slots.Begin(); i != m_slots.End(); i = m_slots.Next(i))
        if (m_slots.Value(i).m_surface == -1 && Resolve(m_slots.Value(i)) < 0)
            return -1;
    return 0;
}

// 0x00BF31B8
void VIClientPart06D50::Preload(int icon)
{
    int state, surface;
    Get(icon, state, surface);
}

// 0x00BF31D8
int VIClientPart06D50::Get(unsigned int id, int &state, int &surface)
{
    surface = -1;
    state = 1;
    int i = m_index.Find(id);
    if (i != m_index.End())
    {
        int k = m_index.Value(i);
        if (m_slots.Value(k).m_surface != -1)
        {
            surface = m_slots.Value(k).m_surface;
            state = 0;
            if (Touch(k) < 0)
                return -1;
        }
    }
    else if (m_loader->IsResourceAvailable(3, id))
    {
        if (Load(id) < 0)
            return -1;
    }
    else
    {
        surface = -1;
        state = 2;
    }
    return 0;
}

// 0x00BF32E8
int VIClientPart06D50::Load(unsigned int id)
{
    VIIconSlot s;
    int k;
    if (m_slots.Size() == m_n)
    {
        k = m_slots.Begin();
        int i = m_index.Find(m_slots.Value(k).m_id);
        if (i == m_index.End())
            return -1;
        if (m_index.Erase(i) < 0)
            return -1;
        if (Unload(m_slots.Value(k)) < 0)
            return -1;
        if (m_slots.Erase(k) < 0)
            return -1;
    }
    s.m_id = id;
    if ((k = m_slots.Add(s)) < 0)
        return -1;
    if (m_index.Insert(id, k) < 0)
        return -1;
    return 0;
}

// 0x00BF3400
int VIClientPart06D50::Touch(int slot)
{
    VIIconSlot s;
    s = m_slots.Value(slot);
    int i = m_index.Find(s.m_id);
    if (i == m_index.End())
        return -1;
    if (m_slots.Erase(slot) < 0)
        return -1;
    int k = m_slots.Add(s);
    if (k < 0)
        return -1;
    m_index.Value(i) = k;
    return 0;
}

// 0x00BF34D0
int VIClientPart06D50::Resolve(VIIconSlot &s)
{
    if (s.m_surface == -1)
    {
        if (s.m_load != -1)
        {
            if (m_loader->IsComplete(s.m_load))
            {
                if (!m_dict->FindTyped(s.m_id, (VIResourceType)1, s.m_surface))
                    return -1;
                m_raster->ShareSurface(s.m_surface);
                m_loader->Release(s.m_load, *m_scene);
                s.m_load = -1;
            }
        }
        else if (m_dict->FindTyped(s.m_id, (VIResourceType)1, s.m_surface))
            m_raster->ShareSurface(s.m_surface);
        else
        {
            if (!m_loader->IsResourceAvailable(3, s.m_id))
                return -1;
            s.m_load = m_loader->Load(3, s.m_id);
            if (s.m_load < 0)
                return -1;
        }
    }
    return 0;
}

// 0x00BF35D8
int VIClientPart06D50::Unload(VIIconSlot &s)
{
    if (s.m_load != -1)
    {
        m_loader->Release(s.m_load, *m_scene);
        s.m_load = -1;
    }
    if (s.m_surface != -1)
    {
        m_raster->ReleaseSurface(s.m_surface, *m_dict);
        s.m_surface = -1;
    }
    return 0;
}

// The two containers' members, SUPPORT's definitions (src/vipool_t.h, src/vimap_t.h).
#include "vipool_t.h"
#include "vimap_t.h"
template VIPool<VIIconSlot>::VIPool();  // 0x00bf4018
template int VIPool<VIIconSlot>::End() const;  // 0x00bf4040
template int VIPool<VIIconSlot>::Begin() const;  // 0x00bf4050
template int VIPool<VIIconSlot>::Next(int) const;  // 0x00bf4080
template int VIPool<VIIconSlot>::DestroyPool();  // 0x00bf40c8
template VIPool<VIIconSlot>::~VIPool();  // 0x00bf4120
template int VIPool<VIIconSlot>::Clear();  // 0x00bf4380
template int VIPool<VIIconSlot>::LinkFreeChain(int);  // 0x00bf43e8
template int VIPool<VIIconSlot>::Allocate();  // 0x00bf4490
template int VIPool<VIIconSlot>::Init(int);  // 0x00bf45a8
template int VIPool<VIIconSlot>::Size() const;  // 0x00bf4928
template int VIPool<VIIconSlot>::Erase(int);  // 0x00bf5070
template int VIPool<VIIconSlot>::Add(const VIIconSlot &);  // 0x00bf5150
template VIMap<unsigned int, int>::VIMap();  // 0x00bf4188
template int VIMap<unsigned int, int>::Begin() const;  // 0x00bf41a8
template int VIMap<unsigned int, int>::End() const;  // 0x00bf4200
template int VIMap<unsigned int, int>::Next(int) const;  // 0x00bf4208
template int VIMap<unsigned int, int>::DestroyPool();  // 0x00bf42c0
template VIMap<unsigned int, int>::~VIMap();  // 0x00bf4318
template int VIMap<unsigned int, int>::Clear();  // 0x00bf45f8
template int VIMap<unsigned int, int>::LinkFreeChain(int);  // 0x00bf4668
template int VIMap<unsigned int, int>::Allocate();  // 0x00bf4710
template int VIMap<unsigned int, int>::Init(int);  // 0x00bf4868
template int VIMap<unsigned int, int>::Find(const unsigned int &) const;  // 0x00bf48c0
template int VIMap<unsigned int, int>::LeftRotate(int);  // 0x00bf4930
template int VIMap<unsigned int, int>::RightRotate(int);  // 0x00bf49e8
template int VIMap<unsigned int, int>::EraseFixup(int);  // 0x00bf4aa0
template int VIMap<unsigned int, int>::FreeNode(int);  // 0x00bf4e58
template int VIMap<unsigned int, int>::Erase(int);  // 0x00bf4e98
template int VIMap<unsigned int, int>::AddNode(int, int);  // 0x00bf5268
template int VIMap<unsigned int, int>::Insert(const unsigned int &, const int &);  // 0x00bf5360
