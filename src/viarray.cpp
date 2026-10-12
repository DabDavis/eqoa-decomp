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

// Placement new.  The allocation is a RAW char block -- `new T[n]` would carry
// gcc's own vec-init loop, which the target does not have -- and the elements
// are constructed one at a time by the loop below, whose null test is written
// out: placement new does not generate one.
inline void *operator new(unsigned int, void *p)
{
    return p;
}

// The element types, modelled only as far as VIArray needs them: the SIZE,
// which the address arithmetic states outright, and whether the default
// constructor and destructor emit anything, which the two loops state.  None
// of the nine below has a destructor -- every one of their DestroyPool bodies
// is empty -- and only the last five construct.
// VIResourceElem is a real class in vi.h since Phase 41 -- VILoader's resource
// directory is an array of them -- and it is the same 16 POD bytes this stub
// was.
// (moved to vi.h -- phase 51)

// 24 bytes, and the constructor is exactly VIPlane's four zero stores: the
// split plane of a BSP node, with the two child indices left uninitialised.
// (moved to vi.h -- phase 51)


// One class big enough to have its own out-of-line constructor and destructor,
// and nothing else VIArray can see: the element loop is a bare `jal` on the
// element address, and DestroyPool's is the same `jal` with `__in_chrg` = 2.
// Its body is not attempted here, so it is declared and left undefined -- the
// harness assembles, it does not link.  (VIZoneRoom used to be the second, a
// 120-byte stub measured off this very stride; phase 34 wrote the real class
// into vi.h and it came out 120 bytes, so the stub is gone.)
// (moved to vi.h -- phase 51)

// 1056 bytes with a 992-byte sub-object at +16 whose constructor and
// destructor are out of line: VIParticleEmitter is a MEMBER, not a base --
// a base would sit at offset 0 -- and the implicit destructor gcc inlines is
// why DestroyPool's `jal` lands on `_$_17VIParticleEmitter` and not on a
// destructor of this class's own.  Its constructor IS this class's, inline,
// with the last store (the `sd` at +1040) hoisted to the front.
//
// Phase 28 wrote the real class out of its own constructor and it is 992 bytes
// exactly, so the stub that stood here is gone and vi.h's declaration is used.
// Phase 53 wrote VICSprite, which owns the array, so the ELEMENT is in vi.h too.

// (moved to vi.h -- phase 53)

// Two more that construct their members TWICE -- Phase 13's VIBBox double-zero
// and Phase 19's VISphere, here as a member: VIVect3's own zeroing constructor
// runs first and VISphere's body zeroes the same three words again, plus the
// radius.  VIRadialFloraInst does it three deep: VIVect4[4]'s constructors,
// then VIMatrix44's VU0 `vsub`+four `sqc2` over the same 64 bytes, then the
// sphere -- and its implicit destructor inlines to `~VIMatrix44`, which is why
// its DestroyPool has a `jal` where VIRadialFloraModel's has nothing.
// (moved to vi.h -- phase 51)

class VIRadialFloraInst
{
public:
    VIMatrix44 m_world;         // +0x00
    VISphere   m_sphere;        // +0x40
    int        m_a, m_b, m_c, m_d;  // +0x50
};

// One byte, and the constructor is one `sb`.
// (moved to vi.h -- phase 51)


template <class T> VIArray<T>::VIArray()
{
    m_size = 0;
    m_data = 0;
}

template <class T> VIArray<T>::~VIArray()
{
    if (m_data)
    {
        DestroyPool();
        delete [] (char *)m_data;
    }
}

template <class T> int VIArray<T>::DestroyPool()
{
    int i = Begin();
    while (i != End())
    {
        int j = i;
        i = Next(i);
        m_data[j].~T();
    }
    return 0;
}

template <class T> int VIArray<T>::Clear()
{
    if (m_data)
    {
        if (DestroyPool() < 0)
            return -1;
        delete [] (char *)m_data;
    }
    m_size = 0;
    m_data = 0;
    return 0;
}

template <class T> int VIArray<T>::Allocate()
{
    if (m_size == 0)
        return 0;
    m_data = (T *)new char[m_size * sizeof(T)];
    if (m_data == 0)
        return -1;
    for (int i = 0; i < m_size; i++)
    {
        T *p = (T *)(i * sizeof(T) + (unsigned int)m_data);
        if (p)
            new (p) T();
    }
    return 0;
}

template <class T> int VIArray<T>::Init(int n)
{
    if (Clear() < 0)
        return -1;
    m_size = n;
    if (Allocate() < 0)
        return -1;
    return 0;
}

// The copy form, instantiated for VIHSpriteTrigger alone.  The element copy is
// the implicit `operator=` -- two `ld`/`sd` pairs for a 16-byte class -- and
// the loop bound is re-read from the SOURCE array every iteration, so the
// guard is `a.m_size` and not the size just stored.
template <class T> int VIArray<T>::Init(const VIArray<T> &a)
{
    if (Clear() < 0)
        return -1;
    m_size = a.m_size;
    if (Allocate() < 0)
        return -1;
    for (int i = 0; i < a.m_size; i++)
    {
        T *p = (T *)(i * sizeof(T) + (unsigned int)m_data);
        if (p)
            *p = a.m_data[i];
    }
    return 0;
}


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
