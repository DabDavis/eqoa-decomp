// viarray_t.h -- VIArray's template definitions, moved VERBATIM out of viarray.cpp so that
// CLIENT's own copies of the template (modules/client/src) compile from the same source.
// viarray.cpp includes it where the block stood, so the token stream it compiles is
// unchanged; the placement new is guarded because a CLIENT TU includes several of these.
#ifndef VI_VIARRAY_T_H
#define VI_VIARRAY_T_H

// Placement new, DECLARED `throw()`.  The allocation is a RAW char block --
// `new T[n]` would carry gcc's own vec-init loop, which the target does not
// have -- and the elements are constructed one at a time by the loop below.
// The null test on each element is the COMPILER'S: gcc 2.95 tests a nothrow
// operator new's result before running the constructor.  Written out by hand
// instead (`T *p = (T *)(i * sizeof(T) + (unsigned)m_data); if (p) ...`) all
// but the one-byte VIWorldRegion matched; the compiler's own test puts the
// scaled index FIRST in the sum and, for a one-byte element, m_data first.
#ifndef VI_PLACEMENT_NEW
#define VI_PLACEMENT_NEW
inline void *operator new(unsigned int, void *p) throw()
{
    return p;
}
#endif

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

// (moved to vi.h -- phase 61)

// One byte, and the constructor is one `sb`.
// (moved to vi.h -- phase 51)


template <class T> VIArray<T>::VIArray()
{
    m_size = 0;
    m_data = 0;
}

template <class T> VIArray<T>::VIArray(const VIArray<T> &a)
{
    m_size = 0;
    m_data = 0;
    Init(a);
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
        new (&m_data[i]) T();
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

#endif // VI_VIARRAY_T_H
