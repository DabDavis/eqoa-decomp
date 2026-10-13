// utime.cpp -- SUPPORT 0x004aeb70..0x004aeb78.  The library's own clock is not
// used on the console: the game hands `drdp_set_time` its time instead, and
// this answers zero.
#include "drdp.h"

uns32 utime_get_ms()
{
    return 0;
}
