#ifndef _SYS_STAT_H
#define _SYS_STAT_H
#ifdef __cplusplus
extern "C" {
#endif
struct stat { int st_dev; int st_ino; int st_mode; int st_nlink; int st_uid; int st_gid; int st_rdev; long st_size; };
int stat(const char *, struct stat *);
#define S_ISDIR(m) (((m) & 0170000) == 0040000)
#ifdef __cplusplus
}
#endif
#endif
