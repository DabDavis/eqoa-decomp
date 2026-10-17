// CLIENT's own copies of SUPPORT's VIMap whose key and value types are not known yet.
// gcc 2.95 emitted a template instantiation in EVERY TU that used it (there is no
// COMDAT); the linker kept every copy and resolved each call to one of them.  These
// are the referenced copies, found by a walk (phase 62): seeded at a live member, each
// callee named off the compiled candidate's own relocations, and every function
// accepted only byte-exact with each of its calls landing on the member it names.
// The definitions are SUPPORT's own, unchanged (src/vimap_t.h).
//
// The value types are PLACEHOLDERS, sized from the node stride: VIRec<size>_<seed>.
// Rename one when its type is known, and move its instantiation to the TU that uses
// it.  Where no member compares keys, the key's signedness cannot be measured either.
#include "game.h"
#include "vimap_t.h"


// 0x00bb7db0..0x00bb8718: 13 members, 24-byte nodes; the key compares signed.
struct VIRec4_BB7DB0 { int m_data[1]; };
template int VIMap<int, VIRec4_BB7DB0>::Begin() const;  // 0x00bb7db0
template int VIMap<int, VIRec4_BB7DB0>::End() const;  // 0x00bb7e08
template int VIMap<int, VIRec4_BB7DB0>::Next(int) const;  // 0x00bb7e10
template int VIMap<int, VIRec4_BB7DB0>::DestroyPool();  // 0x00bb7ec8
template int VIMap<int, VIRec4_BB7DB0>::Clear();  // 0x00bb7f20
template int VIMap<int, VIRec4_BB7DB0>::LinkFreeChain(int);  // 0x00bb7f90
template int VIMap<int, VIRec4_BB7DB0>::Allocate();  // 0x00bb8038
template int VIMap<int, VIRec4_BB7DB0>::Init(int);  // 0x00bb8190
template int VIMap<int, VIRec4_BB7DB0>::AddNode(int, int);  // 0x00bb81e8
template int VIMap<int, VIRec4_BB7DB0>::LeftRotate(int);  // 0x00bb82e0
template int VIMap<int, VIRec4_BB7DB0>::RightRotate(int);  // 0x00bb8398
template int VIMap<int, VIRec4_BB7DB0>::Insert(const int &, const VIRec4_BB7DB0 &);  // 0x00bb8450
template int VIMap<int, VIRec4_BB7DB0>::Find(const int &) const;  // 0x00bb8718

// 0x00bbdd10..0x00bbde80: 5 members, 60-byte nodes; nothing here compares keys, so `int` is a guess.
struct VIRec40_BBDD10 { int m_data[10]; };
template int VIMap<int, VIRec40_BBDD10>::Begin() const;  // 0x00bbdd10
template int VIMap<int, VIRec40_BBDD10>::End() const;  // 0x00bbdd68
template int VIMap<int, VIRec40_BBDD10>::Next(int) const;  // 0x00bbdd70
template int VIMap<int, VIRec40_BBDD10>::DestroyPool();  // 0x00bbde28
template VIMap<int, VIRec40_BBDD10>::~VIMap();  // 0x00bbde80

// 0x00bf41a8..0x00bf5360: was VIMap<unsigned int, VIRec4_BF41A8>; the value is the icon cache's
// slot index, an int: VIMap<unsigned int, int>, instantiated in viclientpart06d50.cpp.

// 0x00cc0c68..0x00cc1cd8: VIMap<VICharResKey, int> (the KEY is the 8-byte record: Insert
// compares it) -- in viswing.cpp, the TU that uses it.
