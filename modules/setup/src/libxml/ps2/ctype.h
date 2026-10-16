#ifndef _CTYPE_H
#define _CTYPE_H
#ifdef __cplusplus
extern "C" {
#endif
/* newlib's: the table is indexed at +1 (EOF), and toupper is the GNU
   statement-expression macro -- the image reads `_ctype_+0x1`. */
#define _U 01
#define _L 02
#define _N 04
#define _S 010
#define _P 020
#define _C 040
#define _X 0100
#define _B 0200
extern const char _ctype_[];
#define isalpha(c)  ((_ctype_+1)[(unsigned)(c)]&(_U|_L))
#define isupper(c)  ((_ctype_+1)[(unsigned)(c)]&_U)
#define islower(c)  ((_ctype_+1)[(unsigned)(c)]&_L)
#define isdigit(c)  ((_ctype_+1)[(unsigned)(c)]&_N)
#define isxdigit(c) ((_ctype_+1)[(unsigned)(c)]&(_X|_N))
#define isspace(c)  ((_ctype_+1)[(unsigned)(c)]&_S)
#define ispunct(c)  ((_ctype_+1)[(unsigned)(c)]&_P)
#define isalnum(c)  ((_ctype_+1)[(unsigned)(c)]&(_U|_L|_N))
#define isprint(c)  ((_ctype_+1)[(unsigned)(c)]&(_P|_U|_L|_N|_B))
#define isgraph(c)  ((_ctype_+1)[(unsigned)(c)]&(_P|_U|_L|_N))
#define iscntrl(c)  ((_ctype_+1)[(unsigned)(c)]&_C)
#define toupper(c) __extension__ ({ int __x = (c); islower(__x) ? (__x - 'a' + 'A') : __x; })
#define tolower(c) __extension__ ({ int __x = (c); isupper(__x) ? (__x - 'A' + 'a') : __x; })
#ifdef __cplusplus
}
#endif
#endif
