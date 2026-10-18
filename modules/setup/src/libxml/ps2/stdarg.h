#ifndef _STDARG_H
#define _STDARG_H
/* SUPPORT's settled EABI va_start (vistring.cpp, uassert.cpp). */
#include "viport.h"
typedef VI_VA_LIST va_list;
#define va_start(ap, last) \
    VI_VA_START(ap, last)
#define va_arg(ap, T)  VI_VA_ARG(ap, T)
#define va_end(ap)
#endif
