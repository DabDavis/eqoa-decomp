// VIArray<T> -- SUPPORT 0x0040A790 .. 0x004A4B30, twenty-two instantiations.
//
// The engine's second template after VIPool, and by far its plainest
// container: two words, a heap block, and no growth path.  Init(n) sets the
// size and allocates; Clear() destroys and frees; there is no Add, no count
// separate from the capacity, and no way to resize without throwing the block
// away.  Every instantiation is the same code, so one source has to answer for
// 132 symbols.
//
//     +0x00 m_size    element count
//     +0x04 m_data    new T[m_size], null until Allocate()
//
// Three facts the object code hands over for free:
//
//  * `new T[n]` is what builds the elements -- Allocate()'s loop is gcc's own
//    inlined array-new constructor loop, reloading m_data and m_size every
//    iteration because a store through T * may alias them, and null-testing
//    the element address because gcc 2.95 guards a constructor call on the
//    allocation.  Reading the loop backwards gives the element's default
//    constructor: three zero stores for VIVect3, four for VIPlane, an `sd`
//    plus two words for VIHSpriteTrigger.
//
//  * DestroyPool() is Begin/End/Next over the same 0x7FFFFFFF sentinel VIPool
//    uses, calling the element destructor.  For a T that has none the body is
//    empty and the loop STILL SURVIVES -- gcc 2.95 has no dead-loop pass -- so
//    VIArray<int>::DestroyPool is a counter, a branch and nothing else.
//
//  * The element size is right there in the address arithmetic: `sll 0x4` for
//    VIPlane, `li 12` + `mult` for VIVect3, `sll 0x3` for unsigned long, which
//    is Phase 23's `long` is EIGHT bytes said a third way.
#include "vi.h"

#include "viarray_t.h"


// ------------------------------------------------------------ instantiations

#define VIARRAY_INSTANTIATE(T)                          \
    template VIArray<T>::VIArray();                     \
    template VIArray<T>::~VIArray();                    \
    template int VIArray<T>::DestroyPool();             \
    template int VIArray<T>::Clear();                   \
    template int VIArray<T>::Allocate();                \
    template int VIArray<T>::Init(int);

VIARRAY_INSTANTIATE(int)
VIARRAY_INSTANTIATE(char)
VIARRAY_INSTANTIATE(unsigned long)
VIARRAY_INSTANTIATE(VIVect3)
VIARRAY_INSTANTIATE(VIPlane)
VIARRAY_INSTANTIATE(VIClipEntry)
VIARRAY_INSTANTIATE(VIResourceElem)
VIARRAY_INSTANTIATE(VIZoneLeaf)
VIARRAY_INSTANTIATE(VIWorldLeaf)
VIARRAY_INSTANTIATE(VIRadialFloraSet)
VIARRAY_INSTANTIATE(VIRadialFloraDist)
VIARRAY_INSTANTIATE(VIZoneStaticTableElem)
VIARRAY_INSTANTIATE(VIZoneNode)
VIARRAY_INSTANTIATE(VIWorldNode)
VIARRAY_INSTANTIATE(VIHSpriteTrigger)
VIARRAY_INSTANTIATE(VIWorldRegion)
VIARRAY_INSTANTIATE(VIZoneRoom)
VIARRAY_INSTANTIATE(VIZone)
VIARRAY_INSTANTIATE(VICSpritePartEmitter)
VIARRAY_INSTANTIATE(VIRadialFloraModel)
VIARRAY_INSTANTIATE(VIRadialFloraInst)

template int VIArray<VIHSpriteTrigger>::Init(const VIArray<VIHSpriteTrigger> &);
