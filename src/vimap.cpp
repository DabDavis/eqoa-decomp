// VIMap<K, V> -- SUPPORT 0x003E4498.. for VIMap<unsigned,VIDictionary::VIDictEntry>.
//
// VIMultiMap with a Find, and VIDictionary's base class.  Four instantiations,
// the same source as viset.cpp and vimultimap.cpp with no re-tuning.
#include "vi.h"

#include "vimap_t.h"




#define VIMAP_COMMON(K, V, KV)                              \
    template int VIMap<K, V >::FreeNode(int);               \
    template int VIMap<K, V >::DestroyPool();               \
    template int VIMap<K, V >::LinkFreeChain(int);          \
    template int VIMap<K, V >::LeftRotate(int);             \
    template int VIMap<K, V >::RightRotate(int);            \
    template int VIMap<K, V >::AddNode(int, int);           \
    template int VIMap<K, V >::Clear();                     \
    template int VIMap<K, V >::Allocate();                  \
    template int VIMap<K, V >::Erase(int);                  \
    template int VIMap<K, V >::EraseFixup(int);             \
    template int VIMap<K, V >::Begin() const;               \
    template int VIMap<K, V >::End() const;                 \
    template int VIMap<K, V >::Next(int) const;             \
    template int VIMap<K, V >::Insert(const K &, const V &);

VIMAP_COMMON(unsigned int, VIDictionary::VIDictEntry, x)
template VIMap<unsigned int, VIDictionary::VIDictEntry>::VIMap();
template int VIMap<unsigned int, VIDictionary::VIDictEntry>::Init(int);
template int VIMap<unsigned int, VIDictionary::VIDictEntry>::Size() const;
template int VIMap<unsigned int, VIDictionary::VIDictEntry>::Find(const unsigned int &) const;

VIMAP_COMMON(unsigned int, int, x)
template VIMap<unsigned int, int>::VIMap();
template int VIMap<unsigned int, int>::Find(const unsigned int &) const;
template VIMap<unsigned int, int>::~VIMap();

VIMAP_COMMON(int, VIEESurfCacheInfo, x)
template VIMap<int, VIEESurfCacheInfo>::VIMap();
template int VIMap<int, VIEESurfCacheInfo>::Init(int);
template int VIMap<int, VIEESurfCacheInfo>::Size() const;
template const int &VIMap<int, VIEESurfCacheInfo>::Key(int) const;
template VIMap<int, VIEESurfCacheInfo>::~VIMap();

template VIMap<int, int>::VIMap();
template int VIMap<int, int>::LinkFreeChain(int);
template int VIMap<int, int>::LeftRotate(int);
template int VIMap<int, int>::RightRotate(int);
template int VIMap<int, int>::AddNode(int, int);
template int VIMap<int, int>::Clear();
template int VIMap<int, int>::Allocate();
template int VIMap<int, int>::DestroyPool();
template int VIMap<int, int>::Begin() const;
template int VIMap<int, int>::End() const;
template int VIMap<int, int>::Next(int) const;
template int VIMap<int, int>::Init(int);
template int VIMap<int, int>::Insert(const int &, const int &);
template int VIMap<int, int>::Find(const int &) const;
template VIMap<int, int>::~VIMap();


// ------------------------------------------------------------ VIDictionary --
// Nothing but a name for VIMap<unsigned, VIDictEntry>: every member forwards,
// and the two Finds are the only ones that read the node.
VIDictionary::VIDictionary()
{
}

// m_map's own destructor runs after this (`li a1,2`); the Clear is the body.
VIDictionary::~VIDictionary()
{
    Clear();
}

int VIDictionary::Init(int n)
{
    if (m_map.Init(n) < 0)
        return -1;
    return 0;
}

int VIDictionary::Size()
{
    return m_map.Size();
}

int VIDictionary::Clear()
{
    return m_map.Clear();
}

int VIDictionary::Add(VIResource &r, unsigned int id, VIResourceType type, int i)
{
    r.m_id = id;

    VIDictEntry e;
    e.m_index = i;
    e.m_type = type;

    if (m_map.Insert(id, e) < 0)
        return -1;
    return 0;
}

int VIDictionary::Remove(VIResource &r)
{
    int i = m_map.Find(r.m_id);
    if (i == m_map.End())
        return -1;
    if (m_map.Erase(i) < 0)
        return -1;
    return 0;
}

int VIDictionary::Find(unsigned int id, VIResourceType &type, int &i)
{
    int n = m_map.Find(id);
    if (n == m_map.End())
        return 0;
    // Through the inline Value() accessor: it hands back `m_nodes + n` as
    // (plus nodes offset), where `m_nodes[n]` written out folds the other way
    // round and reuses the offset register for the second address.
    type = m_map.Value(n).m_type;
    i = m_map.Value(n).m_index;
    return 1;
}

int VIDictionary::FindTyped(unsigned int id, VIResourceType type, int &i)
{
    int n = m_map.Find(id);
    if (n == m_map.End())
        return 0;

    // Phase 7's if-direction, and the entry is named through a POINTER: the
    // target reads the index as `4(a0)` off &m_value, not as `24(v1)` off the
    // node.
    VIDictEntry *e = &m_map.m_nodes[n].m_value;
    if (type == e->m_type)
    {
        i = e->m_index;
        return 1;
    }
    return 0;
}
