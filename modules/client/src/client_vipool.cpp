// CLIENT's own copies of SUPPORT's VIPool (src/vipool_t.h), one per instantiation the
// image still calls -- found by eqtmpl.py scan (phase 62).  gcc 2.95 emitted a template
// instantiation in every TU that used it (no COMDAT); eqcheck counts the dead copies apart.
//
// The type arguments are PLACEHOLDERS: VIRec<size>_<seed> where the members measure the
// size (a struct of that size and alignment), VIElem_<seed> where none of them depends
// on it (any type reproduces those members), VIPtr_<seed> * for a pointer.  Rename one
// when the real type is known, and move it to the TU that uses it.

#include "game.h"
#include "vipool_t.h"

// 0x00bbb848..0x00bbb858: 2 members; pointer argument.
struct VIPtr_BBB848;
template int VIPool<VIPtr_BBB848 *>::End() const;  // 0x00bbb848
template int VIPool<VIPtr_BBB848 *>::Begin() const;  // 0x00bbb858

// 0x00bbb9e8..0x00bbd918: 11 members; size measured.
struct VIRec24_BBCE10 { int m_data[6]; };
template int VIPool<VIRec24_BBCE10>::End() const;  // 0x00bbb9e8
template int VIPool<VIRec24_BBCE10>::Begin() const;  // 0x00bbb9f8
template int VIPool<VIRec24_BBCE10>::Next(int) const;  // 0x00bbba28
template int VIPool<VIRec24_BBCE10>::DestroyPool();  // 0x00bbba60
template VIPool<VIRec24_BBCE10>::~VIPool();  // 0x00bbbab8
template int VIPool<VIRec24_BBCE10>::Clear();  // 0x00bbcda8
template int VIPool<VIRec24_BBCE10>::LinkFreeChain(int);  // 0x00bbce10
template int VIPool<VIRec24_BBCE10>::Allocate();  // 0x00bbcea8
template int VIPool<VIRec24_BBCE10>::Add();  // 0x00bbcfd0
template int VIPool<VIRec24_BBCE10>::Add(VIRec24_BBCE10 const &);  // 0x00bbd7f0
template int VIPool<VIRec24_BBCE10>::Erase(int);  // 0x00bbd918

// 0x00bdf100..0x00be3110: 9 members; size measured.
template int VIPool<VIRec32_BE2DE8>::End() const;  // 0x00bdf100
template int VIPool<VIRec32_BE2DE8>::Begin() const;  // 0x00bdf110
template int VIPool<VIRec32_BE2DE8>::Next(int) const;  // 0x00bdf140
template int VIPool<VIRec32_BE2DE8>::DestroyPool();  // 0x00bdf188
template VIPool<VIRec32_BE2DE8>::~VIPool();  // 0x00bdf1e0
template int VIPool<VIRec32_BE2DE8>::Size() const;  // 0x00bdf868
template VIPool<VIRec32_BE2DE8>::VIPool();  // 0x00bdf0d8
template int VIPool<VIRec32_BE2DE8>::LinkFreeChain(int);  // 0x00be2de8
template int VIPool<VIRec32_BE2DE8>::Allocate();  // 0x00be2e90
template int VIPool<VIRec32_BE2DE8>::Add(VIRec32_BE2DE8 const &);  // 0x00be2fd0
template int VIPool<VIRec32_BE2DE8>::Erase(int);  // 0x00be3110

// 0x00bf4040..0x00bf45a8: 6 members; pointer argument.
struct VIPtr_BF45A8;
template int VIPool<VIPtr_BF45A8 *>::End() const;  // 0x00bf4040
template int VIPool<VIPtr_BF45A8 *>::Begin() const;  // 0x00bf4050
template int VIPool<VIPtr_BF45A8 *>::DestroyPool();  // 0x00bf40c8
template VIPool<VIPtr_BF45A8 *>::~VIPool();  // 0x00bf4120
template int VIPool<VIPtr_BF45A8 *>::Clear();  // 0x00bf4380
template int VIPool<VIPtr_BF45A8 *>::Init(int);  // 0x00bf45a8

// 0x00bf9338..0x00bf9718: 6 members; pointer argument.
struct VIPtr_BF9718;
template int VIPool<VIPtr_BF9718 *>::End() const;  // 0x00bf9338
template int VIPool<VIPtr_BF9718 *>::Begin() const;  // 0x00bf9348
template int VIPool<VIPtr_BF9718 *>::DestroyPool();  // 0x00bf93c0
template VIPool<VIPtr_BF9718 *>::~VIPool();  // 0x00bf9418
template int VIPool<VIPtr_BF9718 *>::Clear();  // 0x00bf9480
template int VIPool<VIPtr_BF9718 *>::Init(int);  // 0x00bf9718

// 0x00bf9f98..0x00bfa348: 10 members; pointer argument.
struct VIPtr_BFA1F0;
template int VIPool<VIPtr_BFA1F0 *>::End() const;  // 0x00bf9f98
template int VIPool<VIPtr_BFA1F0 *>::Begin() const;  // 0x00bf9fa8
template int VIPool<VIPtr_BFA1F0 *>::Next(int) const;  // 0x00bf9fd8
template int VIPool<VIPtr_BFA1F0 *>::DestroyPool();  // 0x00bfa020
template VIPool<VIPtr_BFA1F0 *>::~VIPool();  // 0x00bfa078
template int VIPool<VIPtr_BFA1F0 *>::Clear();  // 0x00bfa0e0
template int VIPool<VIPtr_BFA1F0 *>::LinkFreeChain(int);  // 0x00bfa148
template int VIPool<VIPtr_BFA1F0 *>::Allocate();  // 0x00bfa1f0
template int VIPool<VIPtr_BFA1F0 *>::Init(int);  // 0x00bfa2f8
template int VIPool<VIPtr_BFA1F0 *>::Add(VIPtr_BFA1F0 * const &);  // 0x00bfa348

// 0x00bfb918..0x00bfbde8: 11 members; size measured.
struct VIRec36_BFBCB8 { int m_data[9]; };
template int VIPool<VIRec36_BFBCB8>::End() const;  // 0x00bfb918
template int VIPool<VIRec36_BFBCB8>::Begin() const;  // 0x00bfb928
template int VIPool<VIRec36_BFBCB8>::Next(int) const;  // 0x00bfb958
template int VIPool<VIRec36_BFBCB8>::DestroyPool();  // 0x00bfb9a0
template VIPool<VIRec36_BFBCB8>::~VIPool();  // 0x00bfb9f8
template int VIPool<VIRec36_BFBCB8>::Clear();  // 0x00bfba60
template int VIPool<VIRec36_BFBCB8>::LinkFreeChain(int);  // 0x00bfbac8
template int VIPool<VIRec36_BFBCB8>::Allocate();  // 0x00bfbb70
template int VIPool<VIRec36_BFBCB8>::Init(int);  // 0x00bfbcb8
template int VIPool<VIRec36_BFBCB8>::Add();  // 0x00bfbd08
template int VIPool<VIRec36_BFBCB8>::Erase(int);  // 0x00bfbde8

// 0x00c00f58..0x00c00f68: 2 members; pointer argument.
struct VIPtr_C00F58;
template int VIPool<VIPtr_C00F58 *>::End() const;  // 0x00c00f58
template int VIPool<VIPtr_C00F58 *>::Begin() const;  // 0x00c00f68

// 0x00c010f8..0x00c01508: 6 members; pointer argument.
struct VIPtr_C01508;
template int VIPool<VIPtr_C01508 *>::End() const;  // 0x00c010f8
template int VIPool<VIPtr_C01508 *>::Begin() const;  // 0x00c01108
template int VIPool<VIPtr_C01508 *>::DestroyPool();  // 0x00c01180
template VIPool<VIPtr_C01508 *>::~VIPool();  // 0x00c011d8
template int VIPool<VIPtr_C01508 *>::Clear();  // 0x00c01240
template int VIPool<VIPtr_C01508 *>::Init(int);  // 0x00c01508

// 0x00cb9110..0x00cb9410: VIPool<int>, VIMemStream ops' own copies -- in vimemstreamops.cpp (shared with STATION).

// 0x00cc0b20..0x00cc2018: 9 members; size measured.
struct VIRec524_CC0F38 { int m_data[131]; };
template int VIPool<VIRec524_CC0F38>::End() const;  // 0x00cc0b20
template int VIPool<VIRec524_CC0F38>::Begin() const;  // 0x00cc0b30
template int VIPool<VIRec524_CC0F38>::Next(int) const;  // 0x00cc0b60
template int VIPool<VIRec524_CC0F38>::DestroyPool();  // 0x00cc0ba8
template VIPool<VIRec524_CC0F38>::~VIPool();  // 0x00cc0c00
template int VIPool<VIRec524_CC0F38>::Clear();  // 0x00cc0e60
template int VIPool<VIRec524_CC0F38>::LinkFreeChain(int);  // 0x00cc0f38
template int VIPool<VIRec524_CC0F38>::Allocate();  // 0x00cc0fe0
template int VIPool<VIRec524_CC0F38>::Erase(int);  // 0x00cc2018
