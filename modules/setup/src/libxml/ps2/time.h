#ifndef _TIME_H
#define _TIME_H
#ifdef __cplusplus
extern "C" {
#endif
typedef long time_t;
struct tm { int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst; };
time_t time(time_t *);
struct tm *localtime(const time_t *);
time_t mktime(struct tm *);
#if VI_PORT
/* time_t is a `long`: 8 bytes in the tree, 4 in the host's -m32 libc */
struct tm *vi_localtime(const time_t *);
time_t vi_mktime(struct tm *);
#define localtime vi_localtime
#define mktime vi_mktime
#endif
#ifdef __cplusplus
}
#endif
#endif
