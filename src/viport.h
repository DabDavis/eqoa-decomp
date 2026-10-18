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

// An address in the EE's uncached mirror (KSEG 0x20000000 + RAM), which the
// IOP-streaming file path reads its DMA'd sector buffer through.  The host's
// heap has no mirror: the address itself.
#define VI_UNCACHED(a) (a)

// `long long` is a 128-bit quadword on the EE (lq/sq): SETUP's RunPatcher copies
// a whole 16-byte sceDNAS2Status through one.  abi.h's `long` cannot take a
// second `long`, so the host copies a 16-byte struct.
typedef struct { unsigned int w[4]; } vi_quad;
#define VI_QUAD vi_quad

// SETUP's Http_get::http_uncompress inflates a gzip body's raw deflate stream
// by poking zlib 1.1.4's private inflate state: mode (its first word) = 7,
// BLOCKS, right after inflateInit.  The host's zlib keeps a different state
// (the mode is not first, and BLOCKS is not 7); its public way to the same
// thing is inflateReset2 with negative window bits.
struct z_stream_s;
#ifdef __cplusplus
extern "C" int inflateReset2(struct z_stream_s *strm, int windowBits);
#else
int inflateReset2(struct z_stream_s *strm, int windowBits);
#endif
#define VI_ZLIB_RAW(s) inflateReset2(&(s), -15)

// A DMA tag whose data is a prim buffer's or a colour buffer's: built once and
// the same bytes every frame until the buffer is released or rebuilt.  The
// port (platform/vif1.c) keeps what such data does to VIF1 and VU1 and does
// not re-send it; the tag says so with PCE (bits 26-27, which the EE's DMAC
// ignores on channel 1 with PCE off in D_CTRL, and the port never reads).
// vi_port_static_dma_changed(): a buffer was released or is being rebuilt.
#define VI_DMA_STATIC(id) ((id) | 4)
// A surface's upload (VIEESurfCache::ConstructDMAPacket's palette and level
// REFs): the surface's own bytes, the same until it is locked, unlocked or
// freed, which VISurface reports with vi_port_surface_changed(data, size).
#define VI_DMA_STATIC_TEX(id) ((id) | 8)
// The DrawPrimBuffer hook (the port's modules/hook_port.cpp): UploadMatrices
// and UploadPerMaterial report the uniform qwords they built, and each prim
// buffer material run reports itself after AddMaterial, which may add a
// marker tag to the chain.
#ifdef __cplusplus
class VIRaster;
class VIPrimBuffer;
struct _sceDmaTag;
void vi_port_hook_run(VIRaster *r, VIPrimBuffer *pb, int run, int mat, _sceDmaTag **tag);
extern "C" void vi_port_hook_uniforms(const void *qw, int first, int n);
#endif
#ifdef __cplusplus
extern "C" void vi_port_surface_changed(const void *p, int size);
#else
void vi_port_surface_changed(const void *p, int size);
#endif

// VIClient::Process's frame cap: it spins until this many VITimer ticks
// (983.04 Hz, so 20 is 20.3 ms) have passed since the frame began.  The port
// can lower it (VI_FRAME_TICKS, platform/kernel.c) to run the game uncapped.
#ifdef __cplusplus
extern "C" int vi_port_frame_ticks(int ticks);
#else
int vi_port_frame_ticks(int ticks);
#endif
#define VI_FRAME_TICKS(n) vi_port_frame_ticks(n)
#ifdef __cplusplus
extern "C" void vi_port_static_dma_changed(void);
#else
void vi_port_static_dma_changed(void);
#endif

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
#define VI_UNCACHED(a) (a | 0x20000000)
#define VI_QUAD long long
#define VI_ZLIB_RAW(s) *(int *)s.state = 7
#define VI_DMA_STATIC(id) id
#define VI_DMA_STATIC_TEX(id) id
#define VI_FRAME_TICKS(n) n

#endif

#endif // VIPORT_H
