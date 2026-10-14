// VIEndian -- SUPPORT 0x003E8990 .. 0x003E8A28.  The byte-order probe and the
// swap every ESF/CSF reader runs a field through when VIEndian is not 1.
//
// The probe is an unnamed static at 0x003E8990 (`jr ra; li v0,1` -- the EE is
// little-endian, so the test folds), called from the TU's static initialiser.
#include "vi.h"

// UNSIGNED: the int initialiser then wraps the call in a conversion, so it is
// not a bare CALL_EXPR and gcc computes VIEndian's address BEFORE the call.
static unsigned int VIGetEndian()
{
    int x = 1;
    return *(char *)&x;
}

int VIEndian = VIGetEndian();

// Reverse `n` bytes in place.
void VIBufferSwap(int n, unsigned char *p)
{
    for (int i = 0; i < n >> 1; i++)
    {
        unsigned char t = p[i];
        p[i] = p[n - i - 1];
        p[n - i - 1] = t;
    }
}
