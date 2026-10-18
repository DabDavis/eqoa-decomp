// dpf.cpp -- SUPPORT 0x004be120..0x004be220, the library's debug print.
//
// Every trace in DRDP funnels through `vpf`, and on the console `vpf` is a bare
// `vprintf`.  `pf_mask` drops a line whose category bits are not in the mask
// `dpf_set_print_mask` installs; the mask starts at zero, so nothing printed
// through it ever reaches the TTY unless somebody asks.  `pf_data` is the hex
// dump with its body compiled out: the loop that walked the bytes survives and
// does nothing.
#include "drdp.h"

typedef VI_VA_LIST va_list;
#define va_start(ap, last) \
    VI_VA_START(ap, last)

extern "C" int vprintf(const char *, va_list);

static uns32 s_uiPrintMask;

void dpf_set_print_mask(uns32 uiMask)
{
    s_uiPrintMask = uiMask;
}

extern "C" void vpf(const char *pFormat, va_list args)
{
    vprintf(pFormat, args);
}

extern "C" void pf_mask(uns32 uiMask, const char *pFormat, ...)
{
    va_list args;

    if (uiMask & s_uiPrintMask)
    {
        va_start(args, pFormat);
        vpf(pFormat, args);
    }
}

extern "C" void pf(const char *pFormat, ...)
{
    va_list args;

    va_start(args, pFormat);
    vpf(pFormat, args);
}

extern "C" void pf_data(const void *pData, int iSize)
{
    int i;

    for (i = 0; i < iSize; i++)
        ;
}
