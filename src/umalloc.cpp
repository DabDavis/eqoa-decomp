// umalloc.cpp -- SUPPORT 0x004bf970..0x004bf988.  The debug heap's three
// checks, all compiled to nothing on the console: the library calls plain
// malloc and free.
#include "drdp.h"

void umalloc_validate_heap()
{
}

void umalloc_heap_status()
{
}

void umalloc_leak_check()
{
}
