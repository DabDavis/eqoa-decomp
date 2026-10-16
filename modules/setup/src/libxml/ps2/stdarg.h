#ifndef _STDARG_H
#define _STDARG_H
/* SUPPORT's settled EABI va_start (vistring.cpp, uassert.cpp). */
typedef char *va_list;
#define va_start(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)
#define va_arg(ap, T)  (*(T *)(((ap) += 8) - 8))
#define va_end(ap)
#endif
