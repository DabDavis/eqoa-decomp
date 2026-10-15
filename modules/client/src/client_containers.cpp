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

// 0x00bf41a8..0x00bf5360: 17 members, 24-byte nodes; the key compares UNSIGNED.
struct VIRec4_BF41A8 { int m_data[1]; };
template int VIMap<unsigned int, VIRec4_BF41A8>::Begin() const;  // 0x00bf41a8
template int VIMap<unsigned int, VIRec4_BF41A8>::End() const;  // 0x00bf4200
template int VIMap<unsigned int, VIRec4_BF41A8>::Next(int) const;  // 0x00bf4208
template int VIMap<unsigned int, VIRec4_BF41A8>::DestroyPool();  // 0x00bf42c0
template VIMap<unsigned int, VIRec4_BF41A8>::~VIMap();  // 0x00bf4318
template int VIMap<unsigned int, VIRec4_BF41A8>::Clear();  // 0x00bf45f8
template int VIMap<unsigned int, VIRec4_BF41A8>::LinkFreeChain(int);  // 0x00bf4668
template int VIMap<unsigned int, VIRec4_BF41A8>::Allocate();  // 0x00bf4710
template int VIMap<unsigned int, VIRec4_BF41A8>::Init(int);  // 0x00bf4868
template int VIMap<unsigned int, VIRec4_BF41A8>::Find(const unsigned int &) const;  // 0x00bf48c0
template int VIMap<unsigned int, VIRec4_BF41A8>::LeftRotate(int);  // 0x00bf4930
template int VIMap<unsigned int, VIRec4_BF41A8>::RightRotate(int);  // 0x00bf49e8
template int VIMap<unsigned int, VIRec4_BF41A8>::EraseFixup(int);  // 0x00bf4aa0
template int VIMap<unsigned int, VIRec4_BF41A8>::FreeNode(int);  // 0x00bf4e58
template int VIMap<unsigned int, VIRec4_BF41A8>::Erase(int);  // 0x00bf4e98
template int VIMap<unsigned int, VIRec4_BF41A8>::AddNode(int, int);  // 0x00bf5268
template int VIMap<unsigned int, VIRec4_BF41A8>::Insert(const unsigned int &, const VIRec4_BF41A8 &);  // 0x00bf5360

// 0x00cc0c88..0x00cc1800: 11 members, 28-byte nodes; nothing here compares keys, so `int` is a guess.
struct VIRec8_CC0C88 { int m_data[2]; };
template int VIMap<int, VIRec8_CC0C88>::Begin() const;  // 0x00cc0c88
template int VIMap<int, VIRec8_CC0C88>::End() const;  // 0x00cc0ce0
template int VIMap<int, VIRec8_CC0C88>::Next(int) const;  // 0x00cc0ce8
template int VIMap<int, VIRec8_CC0C88>::DestroyPool();  // 0x00cc0da0
template VIMap<int, VIRec8_CC0C88>::~VIMap();  // 0x00cc0df8
template int VIMap<int, VIRec8_CC0C88>::Clear();  // 0x00cc0ec8
template int VIMap<int, VIRec8_CC0C88>::LeftRotate(int);  // 0x00cc1298
template int VIMap<int, VIRec8_CC0C88>::RightRotate(int);  // 0x00cc1350
template int VIMap<int, VIRec8_CC0C88>::EraseFixup(int);  // 0x00cc1408
template int VIMap<int, VIRec8_CC0C88>::FreeNode(int);  // 0x00cc17c0
template int VIMap<int, VIRec8_CC0C88>::Erase(int);  // 0x00cc1800
