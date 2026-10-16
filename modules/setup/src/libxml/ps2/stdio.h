#ifndef _STDIO_H
#define _STDIO_H
#include <stddef.h>
#include <stdarg.h>
typedef struct __sFILE FILE;
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
#endif
