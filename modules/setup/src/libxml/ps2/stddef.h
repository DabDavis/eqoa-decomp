/* Stub libc for compiling libxml2 2.3.6 the way MCERR/SETUP's build did:
   newlib-shaped declarations, nothing that generates code. */
#ifndef _STDDEF_H
#define _STDDEF_H
typedef unsigned int size_t;
typedef int ptrdiff_t;
#ifndef NULL
#define NULL 0
#endif
#define offsetof(t, m) ((size_t)&((t *)0)->m)
#endif
