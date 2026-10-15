// CLIENT's own copies of SUPPORT's VISet (src/viset_t.h), one per instantiation the
// image still calls -- found by eqtmpl.py scan (phase 62).  gcc 2.95 emitted a template
// instantiation in every TU that used it (no COMDAT); eqcheck counts the dead copies apart.
//
// The type arguments are PLACEHOLDERS: VIRec<size>_<seed> where the members measure the
// size (a struct of that size and alignment), VIElem_<seed> where none of them depends
// on it (any type reproduces those members), VIPtr_<seed> * for a pointer.  Rename one
// when the real type is known, and move it to the TU that uses it.

#include "game.h"
#include "viset_t.h"

// 0x00cb9680..0x00cb9c50: 10 members; size measured.

template int VISet<int>::Begin() const;  // 0x00cb9680
template int VISet<int>::End() const;  // 0x00cb96d8
template int VISet<int>::Next(int) const;  // 0x00cb96e0
template int VISet<int>::DestroyPool();  // 0x00cb9798
template int VISet<int>::LinkFreeChain(int);  // 0x00cb97f0
template int VISet<int>::Allocate();  // 0x00cb9898
template int VISet<int>::AddNode(int, int);  // 0x00cb99e8
template int VISet<int>::LeftRotate(int);  // 0x00cb9ae0
template int VISet<int>::RightRotate(int);  // 0x00cb9b98
template int VISet<int>::Insert(int const &);  // 0x00cb9c50
