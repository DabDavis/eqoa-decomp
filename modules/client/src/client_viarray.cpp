// CLIENT's own copies of SUPPORT's VIArray<T> (src/viarray_t.h), one per instantiation the
// image still calls -- found by eqtmpl.py scan (phase 62).  gcc 2.95 emitted a template
// instantiation in every TU that used it (no COMDAT); eqcheck counts the dead copies apart.
//
// The element types are PLACEHOLDERS: VIRec<size>_<seed> where the members measure the
// size (a struct of that size and alignment), VIElem_<seed> where none of them depends
// on it (any type reproduces those members), VIPtr_<seed> * for a pointer element.
// Rename one when the real type is known, and move it to the TU that uses it.

#include "game.h"
#include "viarray_t.h"

// 0x00bb8790..0x00bb8920: 5 members; element size measured.
struct VIRec8_BB88A8 { int m_data[2]; };
template int VIArray<VIRec8_BB88A8>::DestroyPool();  // 0x00bb8790
template VIArray<VIRec8_BB88A8>::~VIArray();  // 0x00bb87e0
template int VIArray<VIRec8_BB88A8>::Clear();  // 0x00bb8848
template int VIArray<VIRec8_BB88A8>::Allocate();  // 0x00bb88a8
template int VIArray<VIRec8_BB88A8>::Init(int);  // 0x00bb8920

// 0x00bbb768..0x00c02d78: 6 members; element size measured.
struct VIRec8_BBC340 { int m_data[2]; };
template int VIArray<VIRec8_BBC340>::DestroyPool();  // 0x00bbb768
template int VIArray<VIRec8_BBC340>::Clear();  // 0x00bbc2e0
template int VIArray<VIRec8_BBC340>::Allocate();  // 0x00bbc340
template int VIArray<VIRec8_BBC340>::Init(const VIArray<VIRec8_BBC340> &);  // 0x00bbc3b8
template VIArray<VIRec8_BBC340>::~VIArray();  // 0x00bdee48
template int VIArray<VIRec8_BBC340>::Init(int);  // 0x00c02d78

// 0x00bbbc50..0x00bbc708: 6 members; element size measured.
struct VIRec4_BBC218 { int m_data[1]; };
template int VIArray<VIRec4_BBC218>::DestroyPool();  // 0x00bbbc50
template VIArray<VIRec4_BBC218>::~VIArray();  // 0x00bbbca0
template int VIArray<VIRec4_BBC218>::Clear();  // 0x00bbc1b8
template int VIArray<VIRec4_BBC218>::Allocate();  // 0x00bbc218
template int VIArray<VIRec4_BBC218>::Init(int);  // 0x00bbc290
template int VIArray<VIRec4_BBC218>::Init(const VIArray<VIRec4_BBC218> &);  // 0x00bbc708

// 0x00bbbd18..0x00bbc880: 5 members; element size measured.
struct VIRec44_BBC808 { int m_data[11]; };
template int VIArray<VIRec44_BBC808>::DestroyPool();  // 0x00bbbd18
template VIArray<VIRec44_BBC808>::~VIArray();  // 0x00bbbd68
template int VIArray<VIRec44_BBC808>::Clear();  // 0x00bbc7a8
template int VIArray<VIRec44_BBC808>::Allocate();  // 0x00bbc808
template int VIArray<VIRec44_BBC808>::Init(int);  // 0x00bbc880

// 0x00bbc070..0x00bbc168: 2 members; element size not measurable from these members.
struct VIElem_BBC168 { int m_data[1]; };
template int VIArray<VIElem_BBC168>::Clear();  // 0x00bbc070
template int VIArray<VIElem_BBC168>::Init(int);  // 0x00bbc168

// 0x00bdf258..0x00bdf750: 5 members; element size measured.
struct VIRec436_BDF6D8 { int m_data[109]; };
template int VIArray<VIRec436_BDF6D8>::DestroyPool();  // 0x00bdf258
template VIArray<VIRec436_BDF6D8>::~VIArray();  // 0x00bdf2a8
template int VIArray<VIRec436_BDF6D8>::Clear();  // 0x00bdf678
template int VIArray<VIRec436_BDF6D8>::Allocate();  // 0x00bdf6d8
template int VIArray<VIRec436_BDF6D8>::Init(int);  // 0x00bdf750

// 0x00bdf508..0x00bfc798: 4 members; element size not measurable from these members.
struct VIElem_BFC798 { int m_data[1]; };
template int VIArray<VIElem_BFC798>::DestroyPool();  // 0x00bdf508
template VIArray<VIElem_BFC798>::~VIArray();  // 0x00bdf558
template int VIArray<VIElem_BFC798>::Clear();  // 0x00bfc6a0
template int VIArray<VIElem_BFC798>::Init(int);  // 0x00bfc798

// 0x00bdf5c0..0x00beb6e8: 5 members; element size measured.
struct VIRec132_BEB670 { int m_data[33]; };
template int VIArray<VIRec132_BEB670>::DestroyPool();  // 0x00bdf5c0
template VIArray<VIRec132_BEB670>::~VIArray();  // 0x00bdf610
template int VIArray<VIRec132_BEB670>::Clear();  // 0x00beb610
template int VIArray<VIRec132_BEB670>::Allocate();  // 0x00beb670
template int VIArray<VIRec132_BEB670>::Init(int);  // 0x00beb6e8

// 0x00bdf7a0..0x00bdf7f0: 2 members; element size not measurable from these members.
struct VIElem_BDF7A0 { int m_data[1]; };
template int VIArray<VIElem_BDF7A0>::DestroyPool();  // 0x00bdf7a0
template VIArray<VIElem_BDF7A0>::~VIArray();  // 0x00bdf7f0

// 0x00be07a0..0x00be3208: 6 members; element size measured.
struct VIRec144_BE08C8 { int m_data[36]; };
template int VIArray<VIRec144_BE08C8>::DestroyPool();  // 0x00be07a0
template VIArray<VIRec144_BE08C8>::~VIArray();  // 0x00be07f0
template int VIArray<VIRec144_BE08C8>::Clear();  // 0x00be0868
template int VIArray<VIRec144_BE08C8>::Allocate();  // 0x00be08c8
template int VIArray<VIRec144_BE08C8>::Init(const VIArray<VIRec144_BE08C8> &);  // 0x00be0940
template int VIArray<VIRec144_BE08C8>::Init(int);  // 0x00be3208

// 0x00be1c18..0x00be1d10: 2 members; element size not measurable from these members.
struct VIElem_BE1D10 { int m_data[1]; };
template int VIArray<VIElem_BE1D10>::Clear();  // 0x00be1c18
template int VIArray<VIElem_BE1D10>::Init(int);  // 0x00be1d10

// 0x00be1d60..0x00be3320: 5 members; element size measured.
struct VIRec388_BE1E10 { int m_data[97]; };
template int VIArray<VIRec388_BE1E10>::DestroyPool();  // 0x00be1d60
template int VIArray<VIRec388_BE1E10>::Clear();  // 0x00be1db0
template int VIArray<VIRec388_BE1E10>::Allocate();  // 0x00be1e10
template int VIArray<VIRec388_BE1E10>::Init(int);  // 0x00be1e88
template VIArray<VIRec388_BE1E10>::~VIArray();  // 0x00be3320

// 0x00bf90c8..0x00bf9a08: 5 members; element size measured.
struct VIRec8_BF9990 { int m_data[2]; };
template int VIArray<VIRec8_BF9990>::DestroyPool();  // 0x00bf90c8
template VIArray<VIRec8_BF9990>::~VIArray();  // 0x00bf9118
template int VIArray<VIRec8_BF9990>::Clear();  // 0x00bf9930
template int VIArray<VIRec8_BF9990>::Allocate();  // 0x00bf9990
template int VIArray<VIRec8_BF9990>::Init(int);  // 0x00bf9a08

// 0x00bf9190..0x00bf9b30: 5 members; element size measured.
struct VIRec130_BF9AB8_A2 { short m_data[65]; };
template int VIArray<VIRec130_BF9AB8_A2>::DestroyPool();  // 0x00bf9190
template VIArray<VIRec130_BF9AB8_A2>::~VIArray();  // 0x00bf91e0
template int VIArray<VIRec130_BF9AB8_A2>::Clear();  // 0x00bf9a58
template int VIArray<VIRec130_BF9AB8_A2>::Allocate();  // 0x00bf9ab8
template int VIArray<VIRec130_BF9AB8_A2>::Init(int);  // 0x00bf9b30

// 0x00bf9258..0x00bf9c58: 5 members; element size measured.
struct VIRec128_BF9BE0 { int m_data[32]; };
template int VIArray<VIRec128_BF9BE0>::DestroyPool();  // 0x00bf9258
template VIArray<VIRec128_BF9BE0>::~VIArray();  // 0x00bf92a8
template int VIArray<VIRec128_BF9BE0>::Clear();  // 0x00bf9b80
template int VIArray<VIRec128_BF9BE0>::Allocate();  // 0x00bf9be0
template int VIArray<VIRec128_BF9BE0>::Init(int);  // 0x00bf9c58

// 0x00c01710..0x00c8e248: 5 members; element size measured.
struct VIRec144_C017C0 { int m_data[36]; };
template int VIArray<VIRec144_C017C0>::DestroyPool();  // 0x00c01710
template int VIArray<VIRec144_C017C0>::Clear();  // 0x00c01760
template int VIArray<VIRec144_C017C0>::Allocate();  // 0x00c017c0
template int VIArray<VIRec144_C017C0>::Init(int);  // 0x00c01838
template VIArray<VIRec144_C017C0>::~VIArray();  // 0x00c8e248

// 0x00c027e8..0x00c02dc8: 6 members; element size measured.
struct VIRec130_C029C8_A2 { short m_data[65]; };
template int VIArray<VIRec130_C029C8_A2>::DestroyPool();  // 0x00c027e8
template VIArray<VIRec130_C029C8_A2>::~VIArray();  // 0x00c02838
template int VIArray<VIRec130_C029C8_A2>::Clear();  // 0x00c02968
template int VIArray<VIRec130_C029C8_A2>::Allocate();  // 0x00c029c8
template int VIArray<VIRec130_C029C8_A2>::Init(const VIArray<VIRec130_C029C8_A2> &);  // 0x00c02a40
template int VIArray<VIRec130_C029C8_A2>::Init(int);  // 0x00c02dc8

// 0x00c028b0..0x00c02e18: 6 members; element size measured.
struct VIRec130_C02BD0_A2 { short m_data[65]; };
template int VIArray<VIRec130_C02BD0_A2>::DestroyPool();  // 0x00c028b0
template VIArray<VIRec130_C02BD0_A2>::~VIArray();  // 0x00c02900
template int VIArray<VIRec130_C02BD0_A2>::Clear();  // 0x00c02b70
template int VIArray<VIRec130_C02BD0_A2>::Allocate();  // 0x00c02bd0
template int VIArray<VIRec130_C02BD0_A2>::Init(const VIArray<VIRec130_C02BD0_A2> &);  // 0x00c02c48
template int VIArray<VIRec130_C02BD0_A2>::Init(int);  // 0x00c02e18

// 0x00c0c1a8..0x00c0c348: 5 members; element size measured.
struct VIRec1_C0C2D0_A1 { char m_data[1]; };
template int VIArray<VIRec1_C0C2D0_A1>::DestroyPool();  // 0x00c0c1a8
template VIArray<VIRec1_C0C2D0_A1>::~VIArray();  // 0x00c0c1f8
template int VIArray<VIRec1_C0C2D0_A1>::Clear();  // 0x00c0c270
template int VIArray<VIRec1_C0C2D0_A1>::Allocate();  // 0x00c0c2d0
template int VIArray<VIRec1_C0C2D0_A1>::Init(int);  // 0x00c0c348

// 0x00c26c20..0x00c26db0: 5 members; element size measured.
struct VIRec8_C26D38 { int m_data[2]; };
template int VIArray<VIRec8_C26D38>::DestroyPool();  // 0x00c26c20
template VIArray<VIRec8_C26D38>::~VIArray();  // 0x00c26c70
template int VIArray<VIRec8_C26D38>::Clear();  // 0x00c26cd8
template int VIArray<VIRec8_C26D38>::Allocate();  // 0x00c26d38
template int VIArray<VIRec8_C26D38>::Init(int);  // 0x00c26db0

// 0x00c2d6a8..0x00c2d838: 5 members; element size measured.
struct VIRec16_C2D7C0 { int m_data[4]; };
template int VIArray<VIRec16_C2D7C0>::DestroyPool();  // 0x00c2d6a8
template VIArray<VIRec16_C2D7C0>::~VIArray();  // 0x00c2d6f8
template int VIArray<VIRec16_C2D7C0>::Clear();  // 0x00c2d760
template int VIArray<VIRec16_C2D7C0>::Allocate();  // 0x00c2d7c0
template int VIArray<VIRec16_C2D7C0>::Init(int);  // 0x00c2d838

// 0x00c6a7a0..0x00c6a930: 5 members; element size measured.
struct VIRec24_C6A8B8 { int m_data[6]; };
template int VIArray<VIRec24_C6A8B8>::DestroyPool();  // 0x00c6a7a0
template VIArray<VIRec24_C6A8B8>::~VIArray();  // 0x00c6a7f0
template int VIArray<VIRec24_C6A8B8>::Clear();  // 0x00c6a858
template int VIArray<VIRec24_C6A8B8>::Allocate();  // 0x00c6a8b8
template int VIArray<VIRec24_C6A8B8>::Init(int);  // 0x00c6a930

// 0x00c976a8..0x00c97900: 5 members; element size measured.
struct VIRec32_C97888 { int m_data[8]; };
template int VIArray<VIRec32_C97888>::DestroyPool();  // 0x00c976a8
template VIArray<VIRec32_C97888>::~VIArray();  // 0x00c976f8
template int VIArray<VIRec32_C97888>::Clear();  // 0x00c97828
template int VIArray<VIRec32_C97888>::Allocate();  // 0x00c97888
template int VIArray<VIRec32_C97888>::Init(int);  // 0x00c97900

// 0x00c97770..0x00c97a28: 5 members; element size measured.
struct VIRec12_C979B0 { int m_data[3]; };
template int VIArray<VIRec12_C979B0>::DestroyPool();  // 0x00c97770
template VIArray<VIRec12_C979B0>::~VIArray();  // 0x00c977c0
template int VIArray<VIRec12_C979B0>::Clear();  // 0x00c97950
template int VIArray<VIRec12_C979B0>::Allocate();  // 0x00c979b0
template int VIArray<VIRec12_C979B0>::Init(int);  // 0x00c97a28

// 0x00c97b40..0x00c97ce0: 5 members; element size measured.
struct VIRec2_C97C68_A2 { short m_data[1]; };
template int VIArray<VIRec2_C97C68_A2>::DestroyPool();  // 0x00c97b40
template VIArray<VIRec2_C97C68_A2>::~VIArray();  // 0x00c97b90
template int VIArray<VIRec2_C97C68_A2>::Clear();  // 0x00c97c08
template int VIArray<VIRec2_C97C68_A2>::Allocate();  // 0x00c97c68
template int VIArray<VIRec2_C97C68_A2>::Init(int);  // 0x00c97ce0

// 0x00ca9b90..0x00ca9fb0: 4 members; element size not measurable from these members.
struct VIElem_CA9CC8 { int m_data[1]; };
template int VIArray<VIElem_CA9CC8>::DestroyPool();  // 0x00ca9b90
template int VIArray<VIElem_CA9CC8>::Clear();  // 0x00ca9be0
template int VIArray<VIElem_CA9CC8>::Init(int);  // 0x00ca9cc8
template VIArray<VIElem_CA9CC8>::~VIArray();  // 0x00ca9fb0

// 0x00ca9dc0..0x00ca9f38: 5 members; element size measured.
struct VIRec8_CA9E70 { int m_data[2]; };
template int VIArray<VIRec8_CA9E70>::DestroyPool();  // 0x00ca9dc0
template int VIArray<VIRec8_CA9E70>::Clear();  // 0x00ca9e10
template int VIArray<VIRec8_CA9E70>::Allocate();  // 0x00ca9e70
template int VIArray<VIRec8_CA9E70>::Init(int);  // 0x00ca9ee8
template VIArray<VIRec8_CA9E70>::~VIArray();  // 0x00ca9f38

// 0x00cb3e78..0x00cb3fa0: 4 members; element size measured.
struct VIRec40_CB3F28 { int m_data[10]; };
template int VIArray<VIRec40_CB3F28>::DestroyPool();  // 0x00cb3e78
template int VIArray<VIRec40_CB3F28>::Clear();  // 0x00cb3ec8
template int VIArray<VIRec40_CB3F28>::Allocate();  // 0x00cb3f28
template int VIArray<VIRec40_CB3F28>::Init(int);  // 0x00cb3fa0

// 0x00cb4098..0x00cb41c0: 4 members; element size measured.
struct VIRec24_CB4148 { int m_data[6]; };
template int VIArray<VIRec24_CB4148>::DestroyPool();  // 0x00cb4098
template int VIArray<VIRec24_CB4148>::Clear();  // 0x00cb40e8
template int VIArray<VIRec24_CB4148>::Allocate();  // 0x00cb4148
template int VIArray<VIRec24_CB4148>::Init(int);  // 0x00cb41c0

// 0x00cb42b8..0x00cb43e0: 4 members; element size measured.
struct VIRec12_CB4368 { int m_data[3]; };
template int VIArray<VIRec12_CB4368>::DestroyPool();  // 0x00cb42b8
template int VIArray<VIRec12_CB4368>::Clear();  // 0x00cb4308
template int VIArray<VIRec12_CB4368>::Allocate();  // 0x00cb4368
template int VIArray<VIRec12_CB4368>::Init(int);  // 0x00cb43e0

// 0x00cb44d8..0x00cb4600: 4 members; element size measured.
struct VIRec36_CB4588 { int m_data[9]; };
template int VIArray<VIRec36_CB4588>::DestroyPool();  // 0x00cb44d8
template int VIArray<VIRec36_CB4588>::Clear();  // 0x00cb4528
template int VIArray<VIRec36_CB4588>::Allocate();  // 0x00cb4588
template int VIArray<VIRec36_CB4588>::Init(int);  // 0x00cb4600

// 0x00cb46f8..0x00cb4820: 4 members; element size measured.
struct VIRec12_CB47A8 { int m_data[3]; };
template int VIArray<VIRec12_CB47A8>::DestroyPool();  // 0x00cb46f8
template int VIArray<VIRec12_CB47A8>::Clear();  // 0x00cb4748
template int VIArray<VIRec12_CB47A8>::Allocate();  // 0x00cb47a8
template int VIArray<VIRec12_CB47A8>::Init(int);  // 0x00cb4820

// 0x00cb4918..0x00cb4a40: 4 members; element size measured.
struct VIRec12_CB49C8 { int m_data[3]; };
template int VIArray<VIRec12_CB49C8>::DestroyPool();  // 0x00cb4918
template int VIArray<VIRec12_CB49C8>::Clear();  // 0x00cb4968
template int VIArray<VIRec12_CB49C8>::Allocate();  // 0x00cb49c8
template int VIArray<VIRec12_CB49C8>::Init(int);  // 0x00cb4a40

// 0x00cb4b38..0x00cb4c60: 4 members; element size measured.
struct VIRec12_CB4BE8 { int m_data[3]; };
template int VIArray<VIRec12_CB4BE8>::DestroyPool();  // 0x00cb4b38
template int VIArray<VIRec12_CB4BE8>::Clear();  // 0x00cb4b88
template int VIArray<VIRec12_CB4BE8>::Allocate();  // 0x00cb4be8
template int VIArray<VIRec12_CB4BE8>::Init(int);  // 0x00cb4c60

// 0x00cb4d58..0x00cb4e80: 4 members; element size measured.
struct VIRec24_CB4E08 { int m_data[6]; };
template int VIArray<VIRec24_CB4E08>::DestroyPool();  // 0x00cb4d58
template int VIArray<VIRec24_CB4E08>::Clear();  // 0x00cb4da8
template int VIArray<VIRec24_CB4E08>::Allocate();  // 0x00cb4e08
template int VIArray<VIRec24_CB4E08>::Init(int);  // 0x00cb4e80

// 0x00cb5010..0x00cb5100: 2 members; element size not measurable from these members.
struct VIElem_CB5100 { int m_data[1]; };
template int VIArray<VIElem_CB5100>::Clear();  // 0x00cb5010
template int VIArray<VIElem_CB5100>::Init(int);  // 0x00cb5100

// 0x00cb51f0..0x00cb5318: 4 members; element size measured.
struct VIRec32_CB52A0 { int m_data[8]; };
template int VIArray<VIRec32_CB52A0>::DestroyPool();  // 0x00cb51f0
template int VIArray<VIRec32_CB52A0>::Clear();  // 0x00cb5240
template int VIArray<VIRec32_CB52A0>::Allocate();  // 0x00cb52a0
template int VIArray<VIRec32_CB52A0>::Init(int);  // 0x00cb5318

// 0x00cb5408..0x00cb5530: 4 members; element size measured.
struct VIRec12_CB54B8 { int m_data[3]; };
template int VIArray<VIRec12_CB54B8>::DestroyPool();  // 0x00cb5408
template int VIArray<VIRec12_CB54B8>::Clear();  // 0x00cb5458
template int VIArray<VIRec12_CB54B8>::Allocate();  // 0x00cb54b8
template int VIArray<VIRec12_CB54B8>::Init(int);  // 0x00cb5530

// 0x00cb5628..0x00cb5750: 4 members; element size measured.
struct VIRec40_CB56D8 { int m_data[10]; };
template int VIArray<VIRec40_CB56D8>::DestroyPool();  // 0x00cb5628
template int VIArray<VIRec40_CB56D8>::Clear();  // 0x00cb5678
template int VIArray<VIRec40_CB56D8>::Allocate();  // 0x00cb56d8
template int VIArray<VIRec40_CB56D8>::Init(int);  // 0x00cb5750

// 0x00cb5848..0x00cb5970: 4 members; element size measured.
struct VIRec24_CB58F8 { int m_data[6]; };
template int VIArray<VIRec24_CB58F8>::DestroyPool();  // 0x00cb5848
template int VIArray<VIRec24_CB58F8>::Clear();  // 0x00cb5898
template int VIArray<VIRec24_CB58F8>::Allocate();  // 0x00cb58f8
template int VIArray<VIRec24_CB58F8>::Init(int);  // 0x00cb5970

// 0x00cb5a68..0x00cb5b90: 4 members; element size measured.
struct VIRec20_CB5B18 { int m_data[5]; };
template int VIArray<VIRec20_CB5B18>::DestroyPool();  // 0x00cb5a68
template int VIArray<VIRec20_CB5B18>::Clear();  // 0x00cb5ab8
template int VIArray<VIRec20_CB5B18>::Allocate();  // 0x00cb5b18
template int VIArray<VIRec20_CB5B18>::Init(int);  // 0x00cb5b90

// 0x00cb5c88..0x00cb5db0: 4 members; element size measured.
struct VIRec20_CB5D38 { int m_data[5]; };
template int VIArray<VIRec20_CB5D38>::DestroyPool();  // 0x00cb5c88
template int VIArray<VIRec20_CB5D38>::Clear();  // 0x00cb5cd8
template int VIArray<VIRec20_CB5D38>::Allocate();  // 0x00cb5d38
template int VIArray<VIRec20_CB5D38>::Init(int);  // 0x00cb5db0

// 0x00cb5ea8..0x00cb5fd0: 4 members; element size measured.
struct VIRec12_CB5F58 { int m_data[3]; };
template int VIArray<VIRec12_CB5F58>::DestroyPool();  // 0x00cb5ea8
template int VIArray<VIRec12_CB5F58>::Clear();  // 0x00cb5ef8
template int VIArray<VIRec12_CB5F58>::Allocate();  // 0x00cb5f58
template int VIArray<VIRec12_CB5F58>::Init(int);  // 0x00cb5fd0

// 0x00cbb288..0x00cbb2d8: 2 members; element size not measurable from these members.
struct VIElem_CBB288 { int m_data[1]; };
template int VIArray<VIElem_CBB288>::DestroyPool();  // 0x00cbb288
template VIArray<VIElem_CBB288>::~VIArray();  // 0x00cbb2d8

// 0x00cbb340..0x00cbb390: 2 members; element size not measurable from these members.
struct VIElem_CBB340 { int m_data[1]; };
template int VIArray<VIElem_CBB340>::DestroyPool();  // 0x00cbb340
template VIArray<VIElem_CBB340>::~VIArray();  // 0x00cbb390
