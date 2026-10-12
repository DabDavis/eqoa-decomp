// VIString -- SUPPORT 0x003EC278 .. 0x003ED5E0.
//
// The first non-mathematical class in this tree: pointers, chars, `new[]` /
// `delete[]`, varargs and alloca, on the same toolchain everything else
// matched with, with no re-tuning.
//
// The style facts this file establishes:
//
//  1. A LOOP WHOSE CONDITION IS ALSO READ IN THE BODY is written as an
//     explicit guard around a do/while -- `if (*a) { do { ... } while (*a); }`
//     -- not as a plain `while`.  A `while` leaves the exit test at the top of
//     the loop, where gcc reuses its load for the body; the rotated form emits
//     two loads and a `bnezl` whose annulled delay slot carries the body's
//     first instruction.  strcmp16, strcasecmp16 and strstr16's OUTER loop all
//     turn on this.
//  2. A COPY LOOP WALKS THE PARAMETER and keeps the base in a local
//     (`char *r = d;` ... `return r;`), never the other way round.  It decides
//     which register holds which, and five functions here were "right
//     instructions, wrong registers" until it was applied.
//  3. `if (cond) return 1; return 0;` -- Phase 9's shape, again.
#include "vi.h"

extern "C" int   strlen(const char *);
extern "C" char *strcpy(char *, const char *);
extern "C" void *memcpy(void *, const void *, int);
extern "C" int   strcmp(const char *, const char *);
extern "C" char *_strlwr(char *);
extern "C" char *_strupr(char *);
extern "C" int   _strcasecmp(const char *, const char *);
extern "C" int   _strncasecmp(const char *, const char *, int);
extern "C" int   _vsnprintf(char *, int, const char *, char *);
extern "C" int   _vsnprintf16(unsigned short *, int, const unsigned short *, char *);

// gcc 2.95's own va-mips.h, reconstructed: SETUP_INCOMING_VARARGS for the EABI
// (config/mips/abi64.h) puts the register save block BELOW
// virtual_incoming_args, so the first unnamed argument is at
// next_arg - (8 - CUM.arg_words) * 8.  Bare __builtin_next_arg lands 40 bytes
// too high -- past the whole save area -- and the three snprintf wrappers are
// one instruction wrong.  __builtin_args_info(2) is CUM.arg_words and folds at
// compile time, so va_start stays a single addiu.
typedef char *va_list;
#define va_start(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)

unsigned short VIString::ConversionTable1252[32];
static int ConversionTable1252Count;

VIString::VIString()
{
    m_len = 0;
    m_size = 0;
    m_str = 0;
}

VIString::VIString(const VIString &s)
{
    m_len = 0;
    m_size = 0;
    m_str = 0;
    Init(s);
}

VIString::VIString(const char *p)
{
    m_len = 0;
    m_size = 0;
    m_str = 0;
    Init(p);
}

VIString::~VIString()
{
    Deallocate();
}

int VIString::Init(int n)
{
    m_size = n + 1;
    m_len = 0;
    if (Allocate() < 0)
        return -1;
    return 0;
}

int VIString::Init(const char *p)
{
    if (p)
    {
        m_len = strlen(p);
        m_size = m_len + 1;
        if (Allocate() < 0)
            return -1;
        strcpy(m_str, p);
    }
    return 0;
}

int VIString::Init(const char *p, int n)
{
    if (p)
    {
        m_len = n;
        m_size = n + 1;
        if (Allocate() < 0)
            return -1;
        memcpy(m_str, p, n);
        m_str[n] = 0;
    }
    return 0;
}

int VIString::Init(const VIString &s)
{
    m_len = 0;
    m_size = s.m_size;
    if (Allocate() < 0)
        return -1;
    if (s.m_str)
    {
        strcpy(m_str, s.m_str);
        m_len = s.m_len;
    }
    return 0;
}

int VIString::Init(const VIString &s, int nStart, int nEnd)
{
    if (nEnd < nStart)
        return -1;
    if (s.m_len - 1 < nEnd)
        return -1;
    m_len = 0;
    m_size = (nEnd - nStart) + 2;
    if (Allocate() < 0)
        return -1;
    if (s.m_str)
    {
        m_len = m_size - 1;
        memcpy(m_str, s.m_str + nStart, m_len);
        m_str[m_len] = 0;
    }
    return 0;
}

int VIString::Clear()
{
    return Deallocate();
}

// operator= RETURNS BY VALUE.  a0 is the caller's sret slot, a1 is `this` and
// a2 the argument, and the tail is a copy constructor into a0 followed by the
// temporary's destructor -- none of which a `VIString &` return would emit.
VIString VIString::operator=(const VIString &s)
{
    Init(s);
    return *this;
}

int VIString::operator<(const VIString &s) const
{
    if (!m_str)
        return 1;
    if (!s.m_str)
        return 0;
    if (strcmp(m_str, s.m_str) < 0)
        return 1;
    return 0;
}

int VIString::Replace(char cFrom, char cTo)
{
    for (int i = 0; i < m_len; i++)
        if (m_str[i] == cFrom)
            m_str[i] = cTo;
    return 0;
}

int VIString::Concat(const VIString &s)
{
    int n = m_len + s.m_len + 1;
    if (m_size < n)
    {
        m_size = n;
        if (Allocate() < 0)
            return -1;
    }
    if (s.m_str)
    {
        strcpy(m_str + m_len, s.m_str);
        m_len = m_len + s.m_len;
    }
    return 0;
}

int VIString::Concat(const char *p)
{
    int l = strlen(p);
    int n = m_len + l + 1;
    if (m_size < n)
    {
        m_size = n;
        if (Allocate() < 0)
            return -1;
    }
    if (p)
    {
        strcpy(m_str + m_len, p);
        m_len = m_len + l;
    }
    return 0;
}

int VIString::Concat(char c)
{
    int n = m_len + 2;
    if (m_size < n)
    {
        m_size = n;
        if (Allocate() < 0)
            return -1;
    }
    m_str[m_len] = c;
    m_len = m_len + 1;
    m_str[m_len] = 0;
    return 0;
}

VIString VIString::operator+(const VIString &s)
{
    VIString t;
    t.Init(*this);
    t.Concat(s);
    return t;
}

VIString VIString::operator+(const char *p)
{
    VIString t;
    t.Init(*this);
    t.Concat(p);
    return t;
}

VIString VIString::operator+(char c)
{
    VIString t;
    t.Init(*this);
    t.Concat(c);
    return t;
}

// The second `if (m_str)` in the object code is `delete []`'s own null test:
// gcc 2.95 emits `if (p) operator delete(p)` and cannot fold it away here
// because `memcpy` intervenes and m_str is reloaded through `this`.  In
// Deallocate, where nothing intervenes, the same `delete []` folds to one test
// -- which is how both are known to be `delete []` and not `free`.
int VIString::Allocate()
{
    if (m_size == 0)
        return 0;
    char *p = new char[m_size];
    if (!p)
        return -1;
    p[0] = 0;
    if (m_str)
    {
        memcpy(p, m_str, m_len);
        p[m_len] = 0;
        delete [] m_str;
    }
    m_str = p;
    return 0;
}

int VIString::Deallocate()
{
    if (m_str)
    {
        delete [] m_str;
        m_len = 0;
        m_size = 0;
        m_str = 0;
    }
    return 0;
}

int VIString::vsnprintf(char *d, int n, const char *f, char *ap)
{
    return _vsnprintf(d, n, f, ap);
}

// OPEN, two words: the target sets up the alloca frame pointer (`move s8,sp`)
// in the delay slot of the strlen call and materialises that call's argument
// before it; this build does the opposite.  Eleven spellings -- declaration
// order, `char *` vs `unsigned short *`, extern `alloca` vs __builtin_alloca,
// a named return temporary, the size arithmetic -- are byte-identical, so it is
// not the source.  Same instruction-scheduling family as the 23 carried since
// Phase 5.
int VIString::vsnprintf16(unsigned short *d, int n, const char *f, char *ap)
{
    int size = (strlen(f) + 1) * 2;
    unsigned short *t = (unsigned short *)__builtin_alloca(size);
    str8to16(t, size, f);
    return vsnprintf16(d, n, t, ap);
}

int VIString::vsnprintf16(unsigned short *d, int n, const unsigned short *f, char *ap)
{
    return _vsnprintf16(d, n, f, ap);
}

int VIString::snprintf(char *d, int n, const char *f, ...)
{
    va_list ap;
    va_start(ap, f);
    return vsnprintf(d, n, f, ap);
}

int VIString::snprintf16(unsigned short *d, int n, const char *f, ...)
{
    va_list ap;
    va_start(ap, f);
    return vsnprintf16(d, n, f, ap);
}

int VIString::snprintf16(unsigned short *d, int n, const unsigned short *f, ...)
{
    va_list ap;
    va_start(ap, f);
    return vsnprintf16(d, n, f, ap);
}

char *VIString::strlwr(char *p)
{
    return _strlwr(p);
}

char *VIString::strupr(char *p)
{
    return _strupr(p);
}

void VIString::strlwr16(unsigned short *p)
{
    unsigned short c;
    while ((c = *p) != 0)
    {
        if ((unsigned int)(c - 65) < 26)
            *p = c - 'A' + 'a';
        p++;
    }
}

// `c - 'a' + 'A'` and not `c - 32`: gcc folds the pair into the UNSIGNED 16-bit
// constant 65504, which does not fit a signed addiu, so it comes out as a
// hoisted `li a1,0xffe0` plus `addu`.  Spelled `- 32` it is one `addiu`.
void VIString::strupr16(unsigned short *p)
{
    unsigned short c;
    while ((c = *p) != 0)
    {
        if ((unsigned int)(c - 97) < 26)
            *p = c - 'a' + 'A';
        p++;
    }
}

int VIString::strcasecmp(const char *a, const char *b)
{
    return _strcasecmp(a, b);
}

int VIString::strcasecmp16(const unsigned short *a, const unsigned short *b)
{
    int r = 0;
    if (*a || *b)
    {
        do
        {
            unsigned short ca = *a;
            unsigned short cb = *b;
            if ((unsigned int)(ca - 65) < 26)
                ca = ca + 32;
            if ((unsigned int)(cb - 65) < 26)
                cb = cb + 32;
            if (cb < ca)
            {
                r = 1;
                break;
            }
            if (ca < cb)
            {
                r = -1;
                break;
            }
            a++;
            b++;
        }
        while (*a || *b);
    }
    return r;
}

int VIString::strncasecmp(const char *a, const char *b, int n)
{
    return _strncasecmp(a, b, n);
}

int VIString::strlen16(const unsigned short *p)
{
    int n = 0;
    while (*p)
    {
        p++;
        n++;
    }
    return n;
}

unsigned short *VIString::strdup16(const unsigned short *p)
{
    int size = (strlen16(p) + 1) * 2;
    unsigned short *d = (unsigned short *)new char[size];
    if (d)
        memcpy(d, p, size);
    return d;
}

char *VIString::strcpy8(char *d, unsigned int size, const char *s)
{
    char *r = d;
    while (*s && (unsigned int)(d - r + 1) < size)
    {
        *d = *s;
        s++;
        d++;
    }
    *d = 0;
    return r;
}

unsigned short *VIString::strcpy16(unsigned short *d, unsigned int size, const unsigned short *s)
{
    unsigned short *r = d;
    while (*s && (unsigned int)((d - r + 1) * 2) < size)
    {
        *d = *s;
        s++;
        d++;
    }
    *d = 0;
    return r;
}

unsigned short *VIString::strncpy16(unsigned short *d, unsigned int size, const unsigned short *s, int n)
{
    unsigned short *r = d;
    while (*s && (unsigned int)((d - r + 1) * 2) < size && n > 0)
    {
        *d = *s;
        s++;
        d++;
        n--;
    }
    *d = 0;
    return r;
}

char VIString::char16to8(unsigned short w)
{
    if (w < 256)
        return (char)w;
    for (int i = 0; i < ConversionTable1252Count; i++)
        if (w == ConversionTable1252[i])
            return (char)(i - 128);
    return '?';
}

unsigned short VIString::char8to16(char c)
{
    unsigned int u = (unsigned char)c;
    if (!((unsigned char)(u + 128) < 32))
        return (unsigned short)u;
    return ConversionTable1252[u - 128];
}

char *VIString::str16to8(char *d, unsigned int size, const unsigned short *s)
{
    char *r = d;
    while (*s && (unsigned int)(d - r + 1) < size)
    {
        *d = char16to8(*s);
        s++;
        d++;
    }
    *d = 0;
    return r;
}

// The loop variable is `unsigned char`, not `char`: the target loads the byte
// with `lbu`, tests THAT for the loop, and sign-extends it (sll/sra 24) only to
// pass it to char8to16, whose parameter is `char`.  A `char` local loads `lb`
// and needs a second `lbu` for the call.
unsigned short *VIString::str8to16(unsigned short *d, unsigned int size, const char *s)
{
    unsigned short *r = d;
    unsigned char c;
    while ((c = *s) != 0 && (unsigned int)((d - r + 1) * 2) < size)
    {
        *d = char8to16(c);
        s++;
        d++;
    }
    *d = 0;
    return r;
}

unsigned short *VIString::strcat16(unsigned short *dst, unsigned int size, const unsigned short *s)
{
    unsigned short *d = dst + strlen16(dst);
    while (*s && (unsigned int)((d - dst + 1) * 2) < size)
    {
        *d = *s;
        s++;
        d++;
    }
    *d = 0;
    return dst;
}

// SHIPPED INCONSISTENCY: the 8-bit overload of strcat16 widens the byte with a
// plain sign-extending assignment, where str8to16 -- the same loop, one line
// different -- runs every byte through char16to8's inverse.  There is no call
// in this loop at all, so 0x80..0xFF land as negative values instead of their
// CP1252 code points.
unsigned short *VIString::strcat16(unsigned short *dst, unsigned int size, const char *s)
{
    unsigned short *d = dst + strlen16(dst);
    while (*s && (unsigned int)((d - dst + 1) * 2) < size)
    {
        *d = *s;
        s++;
        d++;
    }
    *d = 0;
    return dst;
}

// The author's loop idiom, and it decides the shape of three functions here:
// a guard `if` around an explicit do/while, NOT a plain `while`.  A `while`
// leaves the exit test at the TOP, where gcc reuses its load for the body; the
// rotated form is two separate loads of *a and a `bnezl` whose annulled delay
// slot carries the body's first instruction, which is what the target holds.
int VIString::strcmp16(const unsigned short *a, const unsigned short *b)
{
    int r = 0;
    if (*a)
    {
        do
        {
            r = *a - *b;
            a++;
            b++;
            if (r)
                break;
        }
        while (*a);
    }
    return r;
}

const unsigned short *VIString::strstr16(const unsigned short *h, const unsigned short *n)
{
    if (!h || !n)
        return 0;
    if (*h)
    {
        do
        {
            const unsigned short *p = h;
            const unsigned short *q = n;
            while (*p == *q && *p)
            {
                q++;
                if (*q == 0)
                    return h;
                p++;
            }
            h++;
        }
        while (*h);
    }
    return 0;
}
