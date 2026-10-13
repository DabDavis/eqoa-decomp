// VIHashRefID / VIHashResourceID -- SUPPORT 0x003EC1F8 .. 0x003EC278, two C
// functions between VIObjFile and VIString.  The same string hash twice over
// (h = h * 131 + c, signed char), one name per kind of key.
extern "C" unsigned int VIHashRefID(const char *s)
{
    unsigned int h = 0;
    while (*s)
        h = h * 131 + *s++;
    return h;
}

extern "C" unsigned int VIHashResourceID(const char *s)
{
    unsigned int h = 0;
    while (*s)
        h = h * 131 + *s++;
    return h;
}
