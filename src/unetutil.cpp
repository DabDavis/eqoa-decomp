// The two comparators the DRDP containers are handed.  They live between unet
// and usocket in the map, so they are that region's TU, not a file of their own.
#include "drdp.h"

// The 16-bit twin, and it sits FIRST in the map: same shape, one cast.  A
// sequence number wraps at 65536, so the difference has to be narrowed to
// `int16` before its sign is read.
int uns16_cmp_wrap(const uns16 *a, const uns16 *b)
{
    int diff = (int16)(*a - *b);

    if (diff > 0)
        diff = 1;
    else if (diff < 0)
        diff = -1;
    return diff;
}

// A subtract-and-look-at-the-sign compare: it orders a WRAPPING millisecond
// clock, which a plain `<` on the two values could not.
int uns32_cmp_wrap(const uns32 *a, const uns32 *b)
{
    int diff = *a - *b;

    if (diff > 0)
        diff = 1;
    else if (diff < 0)
        diff = -1;
    return diff;
}

int uns64_cmp(const uns64 *a, const uns64 *b)
{
    int result;

    if (*a > *b)
        return 1;
    result = 0;
    if (*a < *b)
        result = -1;
    return result;
}
