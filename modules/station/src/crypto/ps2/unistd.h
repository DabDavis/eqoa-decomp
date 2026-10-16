/* unistd.h -- the one header libdes's des_locl.h asks for that SETUP's PS2 libc
 * set (src/libxml/ps2/) lacks.  Nothing STATION links from libdes calls into it
 * (getpid is rand_key.c's, which is not linked). */
#ifndef _UNISTD_H
#define _UNISTD_H
#endif
