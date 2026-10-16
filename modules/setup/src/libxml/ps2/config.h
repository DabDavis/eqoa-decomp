/* config.h -- the PS2 build's libxml2 2.3.6 configuration, as SETUP's image
 * shows it.  Every line below is read off the image; nothing is fitted.  The
 * long form is claude-notes/decomp-setup-libxml-20261016.md. */

/* Headers that only gate #includes. */
#define HAVE_CTYPE_H 1
#define HAVE_STRING_H 1
#define HAVE_STDARG_H 1
#define HAVE_MATH_H 1
#define HAVE_TIME_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_ERRNO_H 1

/* HAVE_STDLIB_H: undefined.  xmlInitMemory is five instructions, with no
 * getenv("XML_MEM_BREAKPOINT").  malloc comes from <malloc.h> instead. */
#define HAVE_MALLOC_H 1

/* HAVE_FLOAT_H: undefined.  xmlXPathFormatNumber's buffer offsets and bounds
 * come from xpath.c's own DBL_DIG 16 fallback, not newlib's 15. */

/* HAVE_ZLIB_H: undefined.  xmlIO has no xmlGzfile* callbacks, xmlSaveFile does
 * no compression, and the module does not import gzread. */

/* isnan is imported; isinf is not, so xpath.c compiles its own (HUGE_VAL is
 * newlib's __infinity[0].d -- see math.h). */
#define HAVE_ISNAN 1

#define HAVE_PRINTF 1
#define HAVE_FPRINTF 1
#define HAVE_SPRINTF 1
#define HAVE_SSCANF 1
#define HAVE_VFPRINTF 1
#define HAVE_VSPRINTF 1
#define HAVE_STRDUP 1
#define HAVE_LOCALTIME 1

/* The PS2 libc has no snprintf: every one in the image is a sprintf with the
 * size argument gone (xlinkIsLink, the valid.c qnames, xmlShellPwd), and only
 * sprintf/vsprintf are imported. */
#define snprintf(buf, size, fmt, args...) sprintf(buf, fmt , ## args)
#define vsnprintf(buf, size, fmt, ap) vsprintf(buf, fmt, ap)

/* No unistd on the PS2: xmlFdRead/xmlFdWrite are `return 0` and xmlFdClose
 * is empty in the image. */
#define read(fd, buf, len) 0
#define write(fd, buf, len) 0
#define close(fd)
