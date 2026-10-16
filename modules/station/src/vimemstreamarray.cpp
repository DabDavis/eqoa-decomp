#line 1 "C:/code-frontiers-gm/game/VIMemStreamArray.cpp"
// VIMemStreamArray.cpp -- STATION 0x002E36E0 .. 0x002E3908, CLIENT 0x00CB8B40 ..
// 0x00CB8D68 (from 0x00CB8AB8 with the writer): an int array on the stream.  VIMemStreamArray.h's template
// does the same for every other element type (its assert is on line 71); an int
// has no operator>> of its own, so it is read zigzag, through the inline
// VIMemStreamInt wrapper.  The writer is first.  The VIArray<int> members it
// uses follow, as this TU's own template copies.
#include "vimemstreamx.h"
#include "viarray_t.h"


// 0x002E3658 (CLIENT 0x00CB8AB8): the writer, the count then each element
// zigzag; a const element goes through VIMemStreamInt's {&v, 0}.
VIMemStream &operator<<(VIMemStream &s, const VIArray<int> &a)
{
    s.WriteSigned(a.m_size);
    for (int i = 0; i < a.m_size; i++)
    {
        const int &v = a.m_data[i];
        s << v;
    }
    return s;
}





































// 0x002E36E0: the assert is on line 66.
VIMemStream &operator>>(VIMemStream &s, VIArray<int> &a)
{
    int n;
    s.ReadSigned(n);
    int result = a.Init(n);
    VIASSERT(result == 0);
    for (int i = 0; i < a.m_size; i++)
        s >> a[i];
    return s;
}
