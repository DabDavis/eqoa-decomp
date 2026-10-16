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

// 0x00cb9668..0x00cb9c50: VISet<int>, VIMemStreamSet.cpp's own copies -- in vimemstreamset.cpp (shared with STATION).
