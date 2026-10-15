// VIMultiMap<K, V> -- SUPPORT 0x0046FAF8.. (float,int), 0x0047E198..
// (float,VIEmitterEntry) and the <unsigned,int> copy in the scene TU.
//
// VISet's tree with the key split out of the payload: the Node carries K then
// V after the four link words, and Insert's two placement news are what
// separate them.  Same source, same toolchain, no re-tuning -- see
// viset.cpp for the layout and for every lever used here.
#include "vi.h"

#include "vimultimap_t.h"


// VIMultiMap<unsigned int, int>
template VIMultiMap<unsigned int, int>::VIMultiMap();
template int VIMultiMap<unsigned int, int>::FreeNode(int);
template int VIMultiMap<unsigned int, int>::DestroyPool();
template int VIMultiMap<unsigned int, int>::LinkFreeChain(int);
template int VIMultiMap<unsigned int, int>::LeftRotate(int);
template int VIMultiMap<unsigned int, int>::RightRotate(int);
template int VIMultiMap<unsigned int, int>::AddNode(int, int);
template int VIMultiMap<unsigned int, int>::Clear();
template int VIMultiMap<unsigned int, int>::Allocate();
template int VIMultiMap<unsigned int, int>::Erase(int);
template int VIMultiMap<unsigned int, int>::Insert(const unsigned int &, const int &);
template int VIMultiMap<unsigned int, int>::EraseFixup(int);
template const unsigned int &VIMultiMap<unsigned int, int>::Key(int) const;
template int VIMultiMap<unsigned int, int>::LowerBound(const unsigned int &) const;
template VIMultiMap<unsigned int, int>::~VIMultiMap();

// VIMultiMap<float, int>
template int VIMultiMap<float, int>::FreeNode(int);
template int VIMultiMap<float, int>::DestroyPool();
template int VIMultiMap<float, int>::LinkFreeChain(int);
template int VIMultiMap<float, int>::LeftRotate(int);
template int VIMultiMap<float, int>::RightRotate(int);
template int VIMultiMap<float, int>::AddNode(int, int);
template int VIMultiMap<float, int>::Init(int);
template int VIMultiMap<float, int>::Clear();
template int VIMultiMap<float, int>::Allocate();
template int VIMultiMap<float, int>::Erase(int);
template int VIMultiMap<float, int>::Insert(const float &, const int &);
template int VIMultiMap<float, int>::EraseFixup(int);
template int VIMultiMap<float, int>::Size() const;
template int VIMultiMap<float, int>::Last() const;
template int VIMultiMap<float, int>::Prev(int) const;

// VIMultiMap<float, VIEmitterEntry>
template VIMultiMap<float, VIEmitterEntry>::VIMultiMap();
template int VIMultiMap<float, VIEmitterEntry>::FreeNode(int);
template int VIMultiMap<float, VIEmitterEntry>::DestroyPool();
template int VIMultiMap<float, VIEmitterEntry>::LinkFreeChain(int);
template int VIMultiMap<float, VIEmitterEntry>::LeftRotate(int);
template int VIMultiMap<float, VIEmitterEntry>::RightRotate(int);
template int VIMultiMap<float, VIEmitterEntry>::AddNode(int, int);
template int VIMultiMap<float, VIEmitterEntry>::Init(int);
template int VIMultiMap<float, VIEmitterEntry>::Clear();
template int VIMultiMap<float, VIEmitterEntry>::Allocate();
template int VIMultiMap<float, VIEmitterEntry>::Erase(int);
template int VIMultiMap<float, VIEmitterEntry>::Insert(const float &, const VIEmitterEntry &);
template int VIMultiMap<float, VIEmitterEntry>::EraseFixup(int);
template int VIMultiMap<float, VIEmitterEntry>::Size() const;
template VIMultiMap<float, VIEmitterEntry>::~VIMultiMap();
