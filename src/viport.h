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
#ifdef __wasm__
// wasm32's list is a void * into the caller's argument block, each argument
// at its own alignment and at least 4 bytes.  The tree hands a list on as a
// char * (VI_vsnprintf's `ap`), which i386's char * list allowed, so in C++
// the list is a char * here too and is read by hand.
template <class T> inline T vi_va_arg(char *&ap)
{
    typedef typename vi_va_promote<T>::type P;
    const unsigned int a = __alignof__(P) > 4 ? __alignof__(P) : 4;
    P v;
    ap = (char *)(((unsigned int)ap + a - 1) & ~(a - 1));
    __builtin_memcpy(&v, ap, sizeof(P));
    ap += (sizeof(P) + 3) & ~3u;
    return (T)v;
}
#undef VI_VA_LIST
#undef VI_VA_START
#define VI_VA_LIST            char *
#define VI_VA_START(ap, last) \
    ({ __builtin_va_list vi_va_; __builtin_va_start(vi_va_, last); (ap) = (char *)vi_va_; })
#else
template <class T> inline T vi_va_arg(__builtin_va_list &ap)
{
    return (T)__builtin_va_arg(ap, typename vi_va_promote<T>::type);
}
#endif
}
#undef VI_VA_ARG
#define VI_VA_ARG(ap, T)      vi_va_arg<T>(ap)
#endif

// A function whose callers and definer declare different return types (the
// caller's TU wrote `void`, the definer returns int, or the reverse), which the
// PS2's linker never compared.  wasm-ld does, and turns such a direct call into
// a trap, so the host gives the CALLER's declaration the definer's type: every
// such caller ignores the value.  VI_RET(host, ps2).
#define VI_RET(h, p) h

// The placement new's size parameter (the `_t.h` container headers): the
// host's own size_t, which C++ requires there, and which is `unsigned long` on
// wasm32 -- unspellable in the tree, where abi.h makes `long` eight bytes.
#define VI_SIZE_T __typeof__(sizeof(0))

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
// Level 4: inside a hooked run that qualifies (vi_port_hook_begin), a texture
// download keeps all of its bookkeeping but adds no DMA (the GS thread has the
// surface's bytes); such a surface is OWED its upload, which the next
// download-free Cache of it outside a hooked run sends after all.
class VISurface;
void vi_port_hook_begin(VIRaster *r, VIPrimBuffer *pb, int mat);
int vi_port_hook_nodownload(VISurface *s);
int vi_port_hook_owed(VISurface *s);
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

// Widescreen (the port's modules/wide_port.cpp; VI_ASPECT=16:9).  The picture
// is shown W times wider than the 4:3 the game draws for (W = aspect / (4/3)),
// and the GS frame stays 640x448, stretched by W on the way out.  So every
// draw in the FULL-SCREEN viewport has its NDC x scaled by 1/W where VIRaster
// builds the matrices VU1 gets (CalcMatrices, and the billboards' own
// product): the world shows W times more at the sides (Hor+), and the 2D
// layer, in 640x448 game pixels, keeps its shape in the middle 4:3.  The
// full-screen camera's culling frustum widens with it (VICamera::Update, the
// camera whose aspect is 10/7), and an INSET viewport (the character preview,
// a text scroll, the loading gears) is squeezed about the screen centre as a
// whole: CalcClipToScreen, the scissor, the billboard offset.  What the game
// works out from its own projection (targeting, name plates, damage numbers)
// stays in 4:3 terms, and the 2D layer's squeeze puts it where the wide world
// drew it.  W = 1 changes nothing.  vi_port_wide() is W.
#ifdef __cplusplus
class VIMatrix44;
class VIVect2I;
extern "C" float vi_port_wide(void);
void vi_port_wide_cull(VIMatrix44 &inv, const VIMatrix44 &proj, float aspect);
void vi_port_wide_mvp(VIRaster *r, VIMatrix44 *m);
const VIMatrix44 &vi_port_wide_proj(VIRaster *r, const VIMatrix44 &p);
void vi_port_wide_clip(VIRaster *r);
void vi_port_wide_scissor(VIRaster *r, VIVect2I &a, VIVect2I &b);
void vi_port_wide_viewport(VIRaster *r);
// A screen-covering 2D quad's x edge (the fades), spread to the wide picture's
extern "C" int vi_port_wide_edge(int x, int screenW);
#define VI_WIDE_PROJ(p) vi_port_wide_proj(this, p)
#define VI_WIDE_EDGE(x, sw) vi_port_wide_edge(x, sw)
#endif

#else  // the PS2 build: the original tokens

#define VI_VA_LIST            char *
#define VI_VA_START(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)
#define VI_VA_ARG(ap, T)      (*(T *)(((ap) += 8) - 8))
#define VI_SIZE_T unsigned int
#define VI_RET(h, p) p
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
#define VI_WIDE_PROJ(p) p
#define VI_WIDE_EDGE(x, sw) x

#endif

#endif // VIPORT_H
