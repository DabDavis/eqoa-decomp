#ifndef _STDLIB_H
#define _STDLIB_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);
int atoi(const char *);
long atol(const char *);
double atof(const char *);
double strtod(const char *, char **);
long strtol(const char *, char **, int);
unsigned long strtoul(const char *, char **, int);
char *getenv(const char *);
void exit(int);
void abort(void);
int abs(int);
int rand(void);
#if VI_PORT
/* The native port (viport.h): `long` is the EE's 8 bytes in the tree and 4
 * in the host's libc, whose strtol saturates at 0x7FFFFFFF where newlib's
 * returns the whole value -- and atoi is (int)strtol in both, so a manifest
 * CRC above 0x7FFFFFFF read as 0x7FFFFFFF.  These are the EE's. */
int vi_atoi(const char *);
long vi_atol(const char *);
long vi_strtol(const char *, char **, int);
unsigned long vi_strtoul(const char *, char **, int);
#define atoi vi_atoi
#define atol vi_atol
#define strtol vi_strtol
#define strtoul vi_strtoul
#endif
#ifdef __cplusplus
}
#endif
#endif
