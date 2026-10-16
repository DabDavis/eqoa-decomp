#ifndef _ERRNO_H
#define _ERRNO_H
#ifdef __cplusplus
extern "C" {
#endif
/* newlib's reentrant errno: Http_get::http_get_byte calls __errno(). */
int *__errno(void);
#define errno (*__errno())
#ifdef __cplusplus
}
#endif
#endif
