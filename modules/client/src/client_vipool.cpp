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

// 0x00bbb820..0x00bbd6e0: VIPool<VIAuctionItem>, VIBankControl's lots (eqtmpl.py had it as a
// pointer pool from the two size-blind members; the 800-byte node is the 792-byte lot)

template VIPool<VIAuctionItem>::VIPool();  // 0x00bbb820
template int VIPool<VIAuctionItem>::End() const;  // 0x00bbb848
template int VIPool<VIAuctionItem>::Begin() const;  // 0x00bbb858
template int VIPool<VIAuctionItem>::Next(int) const;  // 0x00bbb888
template int VIPool<VIAuctionItem>::DestroyPool();  // 0x00bbb8d0
template VIPool<VIAuctionItem>::~VIPool();  // 0x00bbb958
template int VIPool<VIAuctionItem>::Clear();  // 0x00bbc960
template int VIPool<VIAuctionItem>::LinkFreeChain(int);  // 0x00bbc9c8
template int VIPool<VIAuctionItem>::Allocate();  // 0x00bbca70
template int VIPool<VIAuctionItem>::Add();  // 0x00bbcc08
template int VIPool<VIAuctionItem>::Add(VIAuctionItem const &);  // 0x00bbd560
template int VIPool<VIAuctionItem>::Erase(int);  // 0x00bbd6e0

// 0x00bbb9e8..0x00bbd918: 11 members; size measured.
// VIDbCharBid is declared in cl_char.h; VIBankControl holds a VIPool of them.
template VIPool<VIDbCharBid>::VIPool();  // 0x00bbb9c0
template int VIPool<VIDbCharBid>::End() const;  // 0x00bbb9e8
template int VIPool<VIDbCharBid>::Begin() const;  // 0x00bbb9f8
template int VIPool<VIDbCharBid>::Next(int) const;  // 0x00bbba28
template int VIPool<VIDbCharBid>::DestroyPool();  // 0x00bbba60
template VIPool<VIDbCharBid>::~VIPool();  // 0x00bbbab8
template int VIPool<VIDbCharBid>::Clear();  // 0x00bbcda8
template int VIPool<VIDbCharBid>::LinkFreeChain(int);  // 0x00bbce10
template int VIPool<VIDbCharBid>::Allocate();  // 0x00bbcea8
template int VIPool<VIDbCharBid>::Add();  // 0x00bbcfd0
template int VIPool<VIDbCharBid>::Add(VIDbCharBid const &);  // 0x00bbd7f0
template int VIPool<VIDbCharBid>::Erase(int);  // 0x00bbd918

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

// 0x00bf4040..0x00bf5150: was VIPool<VIPtr_BF45A8 *>; the element is VIIconSlot (12 bytes),
// instantiated in viclientpart06d50.cpp.

// 0x00bf9338..0x00bf9930: VIPool<VICMSkill> (was VIPool<VIPtr_BF9718 *>: the node is
// 160 bytes), instantiated in viclientpart4ec58.cpp, the TU that emitted it.

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
// (element type declared in game.h with VIClientPart38300)
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
template VIPool<VIRec36_BFBCB8>::VIPool();  // 0x00bfb8f0
template int VIPool<VIRec36_BFBCB8>::Size() const;  // 0x00bfbec8

// 0x00c00f58..0x00c02770: VIPool<VINetItem> (was VIPool<VIPtr_C00F58 *>: a 772-byte node) and
// 0x00c010f8..0x00c01b48: VIPool<VIRec144_C017C0> (was VIPool<VIPtr_C01508 *>: 152), both
// instantiated in viclientpart3834c.cpp, the TU that emitted them.

// 0x00cb9110..0x00cb9410: VIPool<int>, VIMemStream ops' own copies -- in vimemstreamops.cpp (shared with STATION).

// 0x00cc0af8..0x00cc2018: VIPool<VICharRes> -- in viswing.cpp, the TU that uses it.
