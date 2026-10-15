// CLIENT's own copies of SUPPORT's VIList (src/vilist_t.h), one per instantiation the
// image still calls -- found by eqtmpl.py scan (phase 62).  gcc 2.95 emitted a template
// instantiation in every TU that used it (no COMDAT); eqcheck counts the dead copies apart.
//
// The type arguments are PLACEHOLDERS: VIRec<size>_<seed> where the members measure the
// size (a struct of that size and alignment), VIElem_<seed> where none of them depends
// on it (any type reproduces those members), VIPtr_<seed> * for a pointer.  Rename one
// when the real type is known, and move it to the TU that uses it.

#include "game.h"
#include "vilist_t.h"

// 0x00c35c38..0x00c35c98: 2 members; pointer argument.
struct VIPtr_C35C38;
template int VIList<VIPtr_C35C38 *>::Clear();  // 0x00c35c38
template VIList<VIPtr_C35C38 *>::~VIList();  // 0x00c35c98

// 0x00c35e00..0x00c35eb0: 4 members; pointer argument.
struct VIPtr_C35E00;
template int VIList<VIPtr_C35E00 *>::End() const;  // 0x00c35e00
template int VIList<VIPtr_C35E00 *>::Begin() const;  // 0x00c35e58
template int VIList<VIPtr_C35E00 *>::Next(int) const;  // 0x00c35e88
template int VIList<VIPtr_C35E00 *>::Erase(int);  // 0x00c35eb0
