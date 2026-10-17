// CLIENT's own copies of SUPPORT's VIVector (src/vivector_t.h), one per instantiation the
// image still calls -- found by eqtmpl.py scan (phase 62).  gcc 2.95 emitted a template
// instantiation in every TU that used it (no COMDAT); eqcheck counts the dead copies apart.
//
// The type arguments are PLACEHOLDERS: VIRec<size>_<seed> where the members measure the
// size (a struct of that size and alignment), VIElem_<seed> where none of them depends
// on it (any type reproduces those members), VIPtr_<seed> * for a pointer.  Rename one
// when the real type is known, and move it to the TU that uses it.

#include "game.h"
#include "vivector_t.h"

// 0x00bbbe38..0x00bbbfe0: VIVector<int> (the element was VIRec4_BBBF08 here; STATION's
// VIMemStreamVector.cpp shows it is int).  Clear, Init(int) and Add() at 0x00cba088..
// are VIMemStreamVector.cpp's own copies, in vimemstreamvector.cpp (shared with STATION).
template int VIVector<int>::DestroyPool();  // 0x00bbbe38
template VIVector<int>::~VIVector();  // 0x00bbbe88
template int VIVector<int>::Allocate();  // 0x00bbbf08
template int VIVector<int>::Add(const int &);  // 0x00bbbfe0

// 0x00be81c8..0x00be8c48 and 0x00bfce48..0x00bfce98: VIVector<VICSSubcategory>,
// <VICSMessageHeader> and <VICSCategory> (the placeholders VIRec136_BE8808,
// VIRec24_BE84C8 and VIElem_BE8C48 here before) -- vicsclient.cpp's and
// vicsrecords.cpp's own copies, in those files (shared with STATION).

// 0x00c03478..0x00c03818: VIVector<VIRec32_C035B0>, emitted by viclientpart38334.cpp (its
// stream reader instantiates them; lane cl-parts2).

// 0x00c0cca0..0x00c0cf30: 6 members; size measured.
// VITextLine is declared in cl_uitext.h (VITextList's line; was VIRec36_C0CDC0).
template int VIVector<VITextLine>::DestroyPool();  // 0x00c0cca0
template VIVector<VITextLine>::~VIVector();  // 0x00c0ccf0
template int VIVector<VITextLine>::Clear();  // 0x00c0cd58
template int VIVector<VITextLine>::Allocate();  // 0x00c0cdc0
template int VIVector<VITextLine>::Init(int);  // 0x00c0cee0
template int VIVector<VITextLine>::Add(const VITextLine &);  // 0x00c0cf30

// 0x00caa028..0x00cb2ec0: VICharacter.cpp's VIDbChar* rows (VIDbCharItem .. VIDbCharMasteryKey):
// vicharacter.cpp's own copies, in that file (the placeholders VIElem_CAA028 .. VIRec12_CB30D0
// that stood here are gone).
