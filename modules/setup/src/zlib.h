/* zlib.h -- the part of zlib 1.1.4 SETUP calls, shaped as the PS2 build has it.
   uLong is 32 bits here: z_stream is 56 bytes (inflateInit_'s third argument),
   total_out sits at +0x14, and crc32 hands back a value nobody truncates.  The
   library itself is imported, not part of SETUP. */
#ifndef ZLIB_H
#define ZLIB_H
#ifdef __cplusplus
extern "C" {
#endif

#define ZLIB_VERSION "1.1.4"

typedef unsigned char  Byte;
typedef unsigned int   uInt;
typedef unsigned int   uLong;
typedef Byte           Bytef;
typedef char           charf;
typedef int            intf;
typedef uInt           uIntf;
typedef uLong          uLongf;
typedef void          *voidpf;
typedef void          *voidp;

typedef voidpf (*alloc_func)(voidpf opaque, uInt items, uInt size);
typedef void   (*free_func)(voidpf opaque, voidpf address);

struct internal_state;

typedef struct z_stream_s {
    Bytef    *next_in;
    uInt     avail_in;
    uLong    total_in;
    Bytef    *next_out;
    uInt     avail_out;
    uLong    total_out;
    char     *msg;
    struct internal_state *state;
    alloc_func zalloc;
    free_func  zfree;
    voidpf     opaque;
    int     data_type;
    uLong   adler;
    uLong   reserved;
} z_stream;

typedef z_stream *z_streamp;

#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)

#define Z_DEFLATED   8

typedef voidp gzFile;

int inflate(z_streamp strm, int flush);
int inflateEnd(z_streamp strm);
int inflateInit_(z_streamp strm, const char *version, int stream_size);
#define inflateInit(strm) inflateInit_((strm), ZLIB_VERSION, sizeof(z_stream))

gzFile gzopen(const char *path, const char *mode);
int gzwrite(gzFile file, const voidp buf, unsigned len);
int gzclose(gzFile file);
const char *gzerror(gzFile file, int *errnum);

uLong crc32(uLong crc, const Bytef *buf, uInt len);

#ifdef __cplusplus
}
#endif
#endif
