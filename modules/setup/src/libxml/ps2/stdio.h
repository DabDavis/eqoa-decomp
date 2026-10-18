#ifndef _STDIO_H
#define _STDIO_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdarg.h>
/* newlib's FILE, as far as ferror() reaches: _flags at +12, __SERR 0x40. */
struct __sFILE { unsigned char *_p; int _r; int _w; short _flags; short _file; };
typedef struct __sFILE FILE;
#define __SERR 0x0040
#define ferror(p) (((p)->_flags & __SERR) != 0)
struct _reent { int _errno; FILE *_stdin; FILE *_stdout; FILE *_stderr; };
extern struct _reent *_impure_ptr;
#define stdin  (_impure_ptr->_stdin)
#define stdout (_impure_ptr->_stdout)
#define stderr (_impure_ptr->_stderr)
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int sprintf(char *, const char *, ...);
int sscanf(const char *, const char *, ...);
int vfprintf(FILE *, const char *, va_list);
int vsprintf(char *, const char *, va_list);
int fputc(int, FILE *);
int fputs(const char *, FILE *);
int putchar(int);
int puts(const char *);
char *fgets(char *, int, FILE *);
int fgetc(FILE *);
int getc(FILE *);
int getchar(void);
int fflush(FILE *);
FILE *fopen(const char *, const char *);
FILE *fdopen(int, const char *);
int fclose(FILE *);
size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
int remove(const char *);
int rename(const char *, const char *);
void perror(const char *);
#if VI_PORT
/* the offset is a `long`: 8 bytes in the tree, 4 in the host's -m32 libc */
int vi_fseek(FILE *, long, int);
long vi_ftell(FILE *);
#define fseek vi_fseek
#define ftell vi_ftell
#endif
#ifdef __cplusplus
}
#endif
#endif
