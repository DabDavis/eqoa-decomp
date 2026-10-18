// viport.h -- the switches between the PS2 build and the native port.
//
// The PS2 build is the matching build: ee-gcc 2.95, scored byte for byte by
// eqcheck.py.  The port build (port/Makefile) compiles the same tree for the
// host with -DVI_PORT=1.  Every macro here expands, in the PS2 build, to
// exactly the tokens the source held before it was introduced, so a guard can
// never move an instruction; the PS2 scorer is the gate for that.
//
// Edits that use these are made ON THE LINE they replace: an assert's
// __LINE__ is an immediate, so a .cpp file may not gain or lose a line.
// This header is C as well as C++ (SETUP's libxml includes it).
#ifndef VIPORT_H
#define VIPORT_H

#ifndef VI_PORT
#define VI_PORT 0
#endif

#if VI_PORT

// Layout checks (CHK/CL_WE_CHK, `typedef char n[(c) || VI_PORT ? 1 : -1]`)
// hold only on the EE: `long` is 8 bytes there, `sizeof(bool)` 4, and gcc 2.x
// puts the vptr after a class's own data.  The host's layouts differ and the
// game code does not depend on them.

// The EE's EABI va_start (phase 10) reads gcc 2.95's CUM.arg_words and steps
// eight-byte slots; the host uses its own.
#define VI_VA_LIST            __builtin_va_list
#define VI_VA_START(ap, last) __builtin_va_start(ap, last)
#define VI_VA_ARG(ap, T)      __builtin_va_arg(ap, T)
// ... except a type narrower than int: VI_vsnprintf<T>'s `%c` reads its
// character type, char or unsigned short, which on the EE is the low bytes of
// an eight-byte slot and on the host undefined (gcc emits a trap).  Read the
// int it was promoted to.  The tree's only narrow va_arg is that template's.
#ifdef __cplusplus
extern "C++" {
template <class T> struct vi_va_promote { typedef T type; };
template <> struct vi_va_promote<char> { typedef int type; };
template <> struct vi_va_promote<unsigned short> { typedef int type; };
template <class T> inline T vi_va_arg(__builtin_va_list &ap)
{
    return (T)__builtin_va_arg(ap, typename vi_va_promote<T>::type);
}
}
#undef VI_VA_ARG
#define VI_VA_ARG(ap, T)      vi_va_arg<T>(ap)
#endif

// A 128-bit quadword (`mode(TI)`).  i386 has none; the code only copies them.
#define VI_QWORD_TYPEDEF(name) \
    typedef struct { unsigned int w[4]; } __attribute__((aligned(16))) name

// The 16 KB scratchpad at 0x70000000, where VIRaster keeps the per-frame
// matrices, lights and fog (phase 56).  A static buffer on the host.
#ifdef __cplusplus
extern "C" char vi_spr[0x4000];
#else
extern char vi_spr[0x4000];
#endif
#define VI_SPR(a)  ((void *)(vi_spr + ((a) - 0x70000000)))
#define VI_SPRI(a) ((int)(vi_spr + ((a) - 0x70000000)))

// Declarations gcc 2.95 supplied implicitly (cc1plus knew memcpy, strlen,
// strcpy as builtins).  The tree's explicit prototypes are matching levers --
// `int strlen` and `unsigned strlen` schedule differently -- so they are not
// normalised; a TU that relied on the implicit one names it host-only.
#define VI_HOST_ONLY(x...) x

// The screen as text (port/platform/screentext.c): VIRFont::Cache hands every
// string it lays out here, the hook re-tools/screentext.py puts on the console.
#ifdef __cplusplus
extern "C" {
#endif
void vi_port_text8(const char *s, int max);
void vi_port_text16(const unsigned short *s, int max);
#ifdef __cplusplus
}
#endif

// A `bool` MEMBER whose EE size (4 bytes) the layout depends on: an int here.
// Only for members the field sweep (port/tools/layoutcheck.py) shows moving.
#define VI_BOOL4 int

// An EE-only statement with no host meaning: the `sync; ei` an interrupt
// handler or alarm callback ends with.
#define VI_EE_ASM(s) ((void)0)

// newlib's mallinfo() as the EE would answer it (port/platform/newlib.c): the
// free space of ONE fixed arena.  glibc's own answer is the free space inside
// a heap that grows on demand -- always small -- and VIScene::Stream reads
// "under 2,560 KB free" as memory pressure and pulls the streaming radius in
// for good, past the fog's near plane (everything near fogged solid).
#define VI_MALLINFO vi_ee_mallinfo

#else  // the PS2 build: the original tokens

#define VI_VA_LIST            char *
#define VI_VA_START(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)
#define VI_VA_ARG(ap, T)      (*(T *)(((ap) += 8) - 8))
#define VI_QWORD_TYPEDEF(name) \
    typedef unsigned int name __attribute__((mode(TI)))
#define VI_SPR(a)  a
#define VI_SPRI(a) a
#define VI_HOST_ONLY(x...)
#define VI_BOOL4 bool
#define VI_EE_ASM(s) __asm__ __volatile__ (s)
#define VI_MALLINFO mallinfo

#endif

#endif // VIPORT_H
