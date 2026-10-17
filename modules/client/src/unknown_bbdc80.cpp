// A class nothing references -- no call, no vtable, no data word reaches 0x00BBDC80 or
// 0x00BBDCA8 -- holding a VIMap<int, 40-byte value> and nothing else: its ctor and dtor
// out of line, then the map's template copies (its ctor 0x00BBDCF0, Begin .. ~VIMap).
// The file name, the class name and the value type are placeholders (VIRec<size>_<addr>,
// as client_containers.cpp names the value).
#include "game.h"
#include "vimap_t.h"

struct VIRec40_BBDD10 { int m_data[10]; };

class VIRec20_BBDC80
{
public:
    VIRec20_BBDC80();
    ~VIRec20_BBDC80();
    VIMap<int, VIRec40_BBDD10> m_map;
};

VIRec20_BBDC80::VIRec20_BBDC80()
{
}

VIRec20_BBDC80::~VIRec20_BBDC80()
{
}
