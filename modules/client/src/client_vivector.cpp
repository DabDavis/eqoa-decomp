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

// 0x00bbbe38..0x00cba218: 7 members; size measured.
struct VIRec4_BBBF08 { int m_data[1]; };
template int VIVector<VIRec4_BBBF08>::DestroyPool();  // 0x00bbbe38
template VIVector<VIRec4_BBBF08>::~VIVector();  // 0x00bbbe88
template int VIVector<VIRec4_BBBF08>::Allocate();  // 0x00bbbf08
template int VIVector<VIRec4_BBBF08>::Add(const VIRec4_BBBF08 &);  // 0x00bbbfe0
template int VIVector<VIRec4_BBBF08>::Clear();  // 0x00cba088
template int VIVector<VIRec4_BBBF08>::Init(int);  // 0x00cba1c8
template int VIVector<VIRec4_BBBF08>::Add();  // 0x00cba218

// 0x00be81c8..0x00bfce98: 7 members; size measured.
struct VIRec136_BE8808 { int m_data[34]; };
template int VIVector<VIRec136_BE8808>::DestroyPool();  // 0x00be81c8
template VIVector<VIRec136_BE8808>::~VIVector();  // 0x00be8218
template int VIVector<VIRec136_BE8808>::Clear();  // 0x00be87a0
template int VIVector<VIRec136_BE8808>::Allocate();  // 0x00be8808
template int VIVector<VIRec136_BE8808>::Init(const VIVector<VIRec136_BE8808> &);  // 0x00be8988
template int VIVector<VIRec136_BE8808>::Init(int);  // 0x00bfce48
template int VIVector<VIRec136_BE8808>::Add();  // 0x00bfce98

// 0x00be8390..0x00be8620: 6 members; size measured.
struct VIRec24_BE84C8 { int m_data[6]; };
template int VIVector<VIRec24_BE84C8>::DestroyPool();  // 0x00be8390
template VIVector<VIRec24_BE84C8>::~VIVector();  // 0x00be83e0
template int VIVector<VIRec24_BE84C8>::Clear();  // 0x00be8460
template int VIVector<VIRec24_BE84C8>::Allocate();  // 0x00be84c8
template int VIVector<VIRec24_BE84C8>::Init(int);  // 0x00be85d0
template int VIVector<VIRec24_BE84C8>::Add();  // 0x00be8620

// 0x00be8738..0x00be8c48: 2 members; size not measurable from these members.
struct VIElem_BE8C48 { int m_data[1]; };
template int VIVector<VIElem_BE8C48>::Clear();  // 0x00be8738
template int VIVector<VIElem_BE8C48>::Init(int);  // 0x00be8c48

// 0x00c03490..0x00c03818: 7 members; size measured.
struct VIRec32_C035B0 { int m_data[8]; };
template int VIVector<VIRec32_C035B0>::DestroyPool();  // 0x00c03490
template VIVector<VIRec32_C035B0>::~VIVector();  // 0x00c034e0
template int VIVector<VIRec32_C035B0>::Clear();  // 0x00c03548
template int VIVector<VIRec32_C035B0>::Allocate();  // 0x00c035b0
template int VIVector<VIRec32_C035B0>::Init(int);  // 0x00c036c0
template int VIVector<VIRec32_C035B0>::Add();  // 0x00c03710
template int VIVector<VIRec32_C035B0>::Add(const VIRec32_C035B0 &);  // 0x00c03818

// 0x00c0cca0..0x00c0cf30: 6 members; size measured.
struct VIRec36_C0CDC0 { int m_data[9]; };
template int VIVector<VIRec36_C0CDC0>::DestroyPool();  // 0x00c0cca0
template VIVector<VIRec36_C0CDC0>::~VIVector();  // 0x00c0ccf0
template int VIVector<VIRec36_C0CDC0>::Clear();  // 0x00c0cd58
template int VIVector<VIRec36_C0CDC0>::Allocate();  // 0x00c0cdc0
template int VIVector<VIRec36_C0CDC0>::Init(int);  // 0x00c0cee0
template int VIVector<VIRec36_C0CDC0>::Add(const VIRec36_C0CDC0 &);  // 0x00c0cf30

// 0x00caa028..0x00caa078: 2 members; size not measurable from these members.
struct VIElem_CAA028 { int m_data[1]; };
template int VIVector<VIElem_CAA028>::DestroyPool();  // 0x00caa028
template VIVector<VIElem_CAA028>::~VIVector();  // 0x00caa078

// 0x00caa0e0..0x00caa130: 2 members; size not measurable from these members.
struct VIElem_CAA0E0 { int m_data[1]; };
template int VIVector<VIElem_CAA0E0>::DestroyPool();  // 0x00caa0e0
template VIVector<VIElem_CAA0E0>::~VIVector();  // 0x00caa130

// 0x00caa968..0x00caa9b8: 2 members; size not measurable from these members.
struct VIElem_CAA968 { int m_data[1]; };
template int VIVector<VIElem_CAA968>::DestroyPool();  // 0x00caa968
template VIVector<VIElem_CAA968>::~VIVector();  // 0x00caa9b8

// 0x00caaa20..0x00caaa70: 2 members; size not measurable from these members.
struct VIElem_CAAA20 { int m_data[1]; };
template int VIVector<VIElem_CAAA20>::DestroyPool();  // 0x00caaa20
template VIVector<VIElem_CAAA20>::~VIVector();  // 0x00caaa70

// 0x00cab268..0x00cab670: 4 members; size measured.
struct VIRec12_CAB580 { int m_data[3]; };
template int VIVector<VIRec12_CAB580>::DestroyPool();  // 0x00cab268
template VIVector<VIRec12_CAB580>::~VIVector();  // 0x00cab2b8
template int VIVector<VIRec12_CAB580>::Allocate();  // 0x00cab580
template int VIVector<VIRec12_CAB580>::Add(const VIRec12_CAB580 &);  // 0x00cab670

// 0x00cab320..0x00cab370: 2 members; size not measurable from these members.
struct VIElem_CAB320 { int m_data[1]; };
template int VIVector<VIElem_CAB320>::DestroyPool();  // 0x00cab320
template VIVector<VIElem_CAB320>::~VIVector();  // 0x00cab370

// 0x00cabb38..0x00cabf40: 4 members; size measured.
struct VIRec12_CABE50 { int m_data[3]; };
template int VIVector<VIRec12_CABE50>::DestroyPool();  // 0x00cabb38
template VIVector<VIRec12_CABE50>::~VIVector();  // 0x00cabb88
template int VIVector<VIRec12_CABE50>::Allocate();  // 0x00cabe50
template int VIVector<VIRec12_CABE50>::Add(const VIRec12_CABE50 &);  // 0x00cabf40

// 0x00cabbf0..0x00cabc40: 2 members; size not measurable from these members.
struct VIElem_CABBF0 { int m_data[1]; };
template int VIVector<VIElem_CABBF0>::DestroyPool();  // 0x00cabbf0
template VIVector<VIElem_CABBF0>::~VIVector();  // 0x00cabc40

// 0x00cac408..0x00cac810: 4 members; size measured.
struct VIRec12_CAC720 { int m_data[3]; };
template int VIVector<VIRec12_CAC720>::DestroyPool();  // 0x00cac408
template VIVector<VIRec12_CAC720>::~VIVector();  // 0x00cac458
template int VIVector<VIRec12_CAC720>::Allocate();  // 0x00cac720
template int VIVector<VIRec12_CAC720>::Add(const VIRec12_CAC720 &);  // 0x00cac810

// 0x00cac4c0..0x00cac510: 2 members; size not measurable from these members.
struct VIElem_CAC4C0 { int m_data[1]; };
template int VIVector<VIElem_CAC4C0>::DestroyPool();  // 0x00cac4c0
template VIVector<VIElem_CAC4C0>::~VIVector();  // 0x00cac510

// 0x00caccd8..0x00cacd28: 2 members; size not measurable from these members.
struct VIElem_CACCD8 { int m_data[1]; };
template int VIVector<VIElem_CACCD8>::DestroyPool();  // 0x00caccd8
template VIVector<VIElem_CACCD8>::~VIVector();  // 0x00cacd28

// 0x00cacd90..0x00cacde0: 2 members; size not measurable from these members.
struct VIElem_CACD90 { int m_data[1]; };
template int VIVector<VIElem_CACD90>::DestroyPool();  // 0x00cacd90
template VIVector<VIElem_CACD90>::~VIVector();  // 0x00cacde0

// 0x00cad5d8..0x00cad9e0: 4 members; size measured.
struct VIRec12_CAD8F0 { int m_data[3]; };
template int VIVector<VIRec12_CAD8F0>::DestroyPool();  // 0x00cad5d8
template VIVector<VIRec12_CAD8F0>::~VIVector();  // 0x00cad628
template int VIVector<VIRec12_CAD8F0>::Allocate();  // 0x00cad8f0
template int VIVector<VIRec12_CAD8F0>::Add(const VIRec12_CAD8F0 &);  // 0x00cad9e0

// 0x00cad690..0x00cad6e0: 2 members; size not measurable from these members.
struct VIElem_CAD690 { int m_data[1]; };
template int VIVector<VIElem_CAD690>::DestroyPool();  // 0x00cad690
template VIVector<VIElem_CAD690>::~VIVector();  // 0x00cad6e0

// 0x00cadea8..0x00cadef8: 2 members; size not measurable from these members.
struct VIElem_CADEA8 { int m_data[1]; };
template int VIVector<VIElem_CADEA8>::DestroyPool();  // 0x00cadea8
template VIVector<VIElem_CADEA8>::~VIVector();  // 0x00cadef8

// 0x00cadf60..0x00cadfb0: 2 members; size not measurable from these members.
struct VIElem_CADF60 { int m_data[1]; };
template int VIVector<VIElem_CADF60>::DestroyPool();  // 0x00cadf60
template VIVector<VIElem_CADF60>::~VIVector();  // 0x00cadfb0

// 0x00cae7d8..0x00cae828: 2 members; size not measurable from these members.
struct VIElem_CAE7D8 { int m_data[1]; };
template int VIVector<VIElem_CAE7D8>::DestroyPool();  // 0x00cae7d8
template VIVector<VIElem_CAE7D8>::~VIVector();  // 0x00cae828

// 0x00caeda8..0x00caedf8: 2 members; size not measurable from these members.
struct VIElem_CAEDA8 { int m_data[1]; };
template int VIVector<VIElem_CAEDA8>::DestroyPool();  // 0x00caeda8
template VIVector<VIElem_CAEDA8>::~VIVector();  // 0x00caedf8

// 0x00caee60..0x00caf038: 4 members; size measured.
struct VIRec12_CAEF48 { int m_data[3]; };
template int VIVector<VIRec12_CAEF48>::DestroyPool();  // 0x00caee60
template VIVector<VIRec12_CAEF48>::~VIVector();  // 0x00caeeb0
template int VIVector<VIRec12_CAEF48>::Allocate();  // 0x00caef48
template int VIVector<VIRec12_CAEF48>::Add(const VIRec12_CAEF48 &);  // 0x00caf038

// 0x00caf7d8..0x00caf828: 2 members; size not measurable from these members.
struct VIElem_CAF7D8 { int m_data[1]; };
template int VIVector<VIElem_CAF7D8>::DestroyPool();  // 0x00caf7d8
template VIVector<VIElem_CAF7D8>::~VIVector();  // 0x00caf828

// 0x00caf890..0x00caf8e0: 2 members; size not measurable from these members.
struct VIElem_CAF890 { int m_data[1]; };
template int VIVector<VIElem_CAF890>::DestroyPool();  // 0x00caf890
template VIVector<VIElem_CAF890>::~VIVector();  // 0x00caf8e0

// 0x00cb00c8..0x00cb04d0: 4 members; size measured.
struct VIRec12_CB03E0 { int m_data[3]; };
template int VIVector<VIRec12_CB03E0>::DestroyPool();  // 0x00cb00c8
template VIVector<VIRec12_CB03E0>::~VIVector();  // 0x00cb0118
template int VIVector<VIRec12_CB03E0>::Allocate();  // 0x00cb03e0
template int VIVector<VIRec12_CB03E0>::Add(const VIRec12_CB03E0 &);  // 0x00cb04d0

// 0x00cb0180..0x00cb01d0: 2 members; size not measurable from these members.
struct VIElem_CB0180 { int m_data[1]; };
template int VIVector<VIElem_CB0180>::DestroyPool();  // 0x00cb0180
template VIVector<VIElem_CB0180>::~VIVector();  // 0x00cb01d0

// 0x00cb0998..0x00cb09e8: 2 members; size not measurable from these members.
struct VIElem_CB0998 { int m_data[1]; };
template int VIVector<VIElem_CB0998>::DestroyPool();  // 0x00cb0998
template VIVector<VIElem_CB0998>::~VIVector();  // 0x00cb09e8

// 0x00cb0a50..0x00cb0aa0: 2 members; size not measurable from these members.
struct VIElem_CB0A50 { int m_data[1]; };
template int VIVector<VIElem_CB0A50>::DestroyPool();  // 0x00cb0a50
template VIVector<VIElem_CB0A50>::~VIVector();  // 0x00cb0aa0

// 0x00cb12d8..0x00cb1328: 2 members; size not measurable from these members.
struct VIElem_CB12D8 { int m_data[1]; };
template int VIVector<VIElem_CB12D8>::DestroyPool();  // 0x00cb12d8
template VIVector<VIElem_CB12D8>::~VIVector();  // 0x00cb1328

// 0x00cb1390..0x00cb13e0: 2 members; size not measurable from these members.
struct VIElem_CB1390 { int m_data[1]; };
template int VIVector<VIElem_CB1390>::DestroyPool();  // 0x00cb1390
template VIVector<VIElem_CB1390>::~VIVector();  // 0x00cb13e0

// 0x00cb1bd8..0x00cb1c28: 2 members; size not measurable from these members.
struct VIElem_CB1BD8 { int m_data[1]; };
template int VIVector<VIElem_CB1BD8>::DestroyPool();  // 0x00cb1bd8
template VIVector<VIElem_CB1BD8>::~VIVector();  // 0x00cb1c28

// 0x00cb1c90..0x00cb1ce0: 2 members; size not measurable from these members.
struct VIElem_CB1C90 { int m_data[1]; };
template int VIVector<VIElem_CB1C90>::DestroyPool();  // 0x00cb1c90
template VIVector<VIElem_CB1C90>::~VIVector();  // 0x00cb1ce0

// 0x00cb24c8..0x00cb2518: 2 members; size not measurable from these members.
struct VIElem_CB24C8 { int m_data[1]; };
template int VIVector<VIElem_CB24C8>::DestroyPool();  // 0x00cb24c8
template VIVector<VIElem_CB24C8>::~VIVector();  // 0x00cb2518

// 0x00cb2580..0x00cb25d0: 2 members; size not measurable from these members.
struct VIElem_CB2580 { int m_data[1]; };
template int VIVector<VIElem_CB2580>::DestroyPool();  // 0x00cb2580
template VIVector<VIElem_CB2580>::~VIVector();  // 0x00cb25d0

// 0x00cb2db8..0x00cb31c0: 4 members; size measured.
struct VIRec12_CB30D0 { int m_data[3]; };
template int VIVector<VIRec12_CB30D0>::DestroyPool();  // 0x00cb2db8
template VIVector<VIRec12_CB30D0>::~VIVector();  // 0x00cb2e08
template int VIVector<VIRec12_CB30D0>::Allocate();  // 0x00cb30d0
template int VIVector<VIRec12_CB30D0>::Add(const VIRec12_CB30D0 &);  // 0x00cb31c0

// 0x00cb2e70..0x00cb2ec0: 2 members; size not measurable from these members.
struct VIElem_CB2E70 { int m_data[1]; };
template int VIVector<VIElem_CB2E70>::DestroyPool();  // 0x00cb2e70
template VIVector<VIElem_CB2E70>::~VIVector();  // 0x00cb2ec0
