#line 1 "C:/code-frontiers-gm/core/VIString.cpp"
// VIString -- SUPPORT 0x003EC278 .. 0x003EDE70; the last 0x890 bytes are the
// two VI_vsnprintf instantiations, emitted at the end of the TU (phase 61).
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
extern "C" char *strncpy(char *, const char *, int); extern "C" char *strcat(char *, const char *);
extern "C" char *strlwr(char *); extern "C" char *strupr(char *); extern "C" void *malloc(int);
extern "C" int   strcasecmp(const char *, const char *);
extern "C" int   strncasecmp(const char *, const char *, int);
extern "C" int   sprintf(char *, const char *, ...);
extern "C" void  _VIAssert(const char *expr, const char *file, int line);
extern "C" void  _VIAssertf(int cond, const char *fmt, ...);
extern "C" const char *__VIAssertf_file;
extern "C" int   __VIAssertf_line;

#define VIASSERT(x)  if (!(x)) _VIAssert(#x, __FILE__, __LINE__);
#define VIASSERTF(x, fmt, args...) \
    { __VIAssertf_file = __FILE__; __VIAssertf_line = __LINE__; _VIAssertf(x, fmt, ## args); }

// The printf engine is ONE template over the character type, instantiated for
// char and unsigned short; gcc 2.95 emits both at the end of the TU, which is
// where the image has them.  Defined at the bottom of this file.
template <class T> void VI_vsnprintf(T *dst, int dstCount, const T *fmt, char *ap);

// Every vararg occupies one 8-byte slot on this EABI; the target reads it back
// with `addiu ap,ap,8` and a load at -8 and nothing else.
#define va_arg(ap, T)  (*(T *)(((ap) += 8) - 8))

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
        strncpy(m_str, p, n);
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
        strncpy(m_str, s.m_str + nStart, m_len);
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
        strcat(m_str + m_len, s.m_str);
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
        strncpy(p, m_str, m_len);
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

// No `return`: the snprintf family hands back whatever the template left in v0
// -- the address of the terminating NUL.  Shipped.  Declared VOID, or the first
// pseudo after the call in viglobalee's _VIAssertf/_VIAlert cannot take $v0.
void VIString::vsnprintf(char *d, int n, const char *f, char *ap)
{
    VI_vsnprintf(d, n, f, ap);
}

// Phase 61.  The character narrowing VI_vsnprintf runs every format character
// through, as two explicit specialisations emitted in place.
template <class T> char charTto8(T c);

template <> char charTto8(unsigned short c)
{
    return VIString::char16to8(c);
}

template <> char charTto8(char c)
{
    return c;
}

// The four appenders VI_vsnprintf's %s / %S and every numeric conversion end
// in: one per (destination, source) character pair, each writing while there
// is room for the terminator and substituting "(null)" for a null string.  The
// two that read a WIDE source copy a wide "(null)" into a block-scope array
// and point at it after the block has ended, and they also write '?' wherever
// the narrowed character does not read back equal -- which for the
// unsigned short -> unsigned short copy can never fire.
static void AppendString(char *dst, int &pos, int dstCount, const char *s)
{
    if (s == 0)
        s = "(null)";
    if (*s)
    {
        const char *p = s;
        do
        {
            if (pos + 1 < dstCount)
                dst[pos++] = *p;
            p++;
        }
        while (*p);
    }
}

static void AppendString(unsigned short *dst, int &pos, int dstCount, const char *s)
{
    if (s == 0)
        s = "(null)";
    if (*s)
    {
        const char *p = s;
        do
        {
            if (pos + 1 < dstCount)
                dst[pos++] = VIString::char8to16(*p);
            p++;
        }
        while (*p);
    }
}

static void AppendString(char *dst, int &pos, int dstCount, const unsigned short *s)
{
    if (s == 0)
    {
        unsigned short null16[] = { '(', 'n', 'u', 'l', 'l', ')', 0 };
        s = null16;
    }
    if (*s)
    {
        const unsigned short *p = s;
        do
        {
            if (pos + 1 < dstCount)
            {
                dst[pos] = VIString::char16to8(*p);
                if (dst[pos] != *p)
                    dst[pos] = '?';
                pos++;
            }
            p++;
        }
        while (*p);
    }
}

static void AppendString(unsigned short *dst, int &pos, int dstCount, const unsigned short *s)
{
    if (s == 0)
    {
        unsigned short null16[] = { '(', 'n', 'u', 'l', 'l', ')', 0 };
        s = null16;
    }
    if (*s)
    {
        const unsigned short *p = s;
        do
        {
            if (pos + 1 < dstCount)
            {
                dst[pos] = *p;
                if (dst[pos] != *p)
                    dst[pos] = '?';
                pos++;
            }
            p++;
        }
        while (*p);
    }
}

// A result variable and a `break`: the early `return 1` moves the exit out of
// line and the loop's own test changes shape.
int StringIncludes(const char *s, char c)
{
    int r = 0;
    while (*s)
        if (*s++ == c)
        {
            r = 1;
            break;
        }
    return r;
}

// ---------------------------------------------------------------------------
// VI_vsnprintf<T> -- the printf engine under every VIString formatter.
//
// Its two asserts carry the original's line numbers, 663 and 792 of
// core/VIString.cpp, as immediates: the blank runs below are FILLER that put
// them there, not a claim about what the original had in between.
// ---------------------------------------------------------------------------


























































































































































































































template <class T>
void VI_vsnprintf(T *dst, int DstCount, const T *fmt, char *ap)
{
    int pos = 0;
    int i = 0;

    DstCount /= sizeof(T);
    VIASSERT(DstCount > 0);

    while (pos + 1 < DstCount && fmt[i])
    {
        if (fmt[i] == '%')
        {
            char conv[] = "cCdiIouxXeEfgGnpsSh%";
            char spec[64];
            char tmp[64];
            unsigned int n = 0;
            char c;

            for (;;)
            {
                c = charTto8(fmt[i++]);
                spec[n++] = c;
                if (n > 1 && StringIncludes(conv, c))
                    break;
                if (n + 1 == sizeof(spec) || c == 0)
                    goto done;
            }
            spec[n] = 0;

            switch (c)
            {
            case 'S':
                AppendString(dst, pos, DstCount, va_arg(ap, const unsigned short *));
                break;

            case 's':
                AppendString(dst, pos, DstCount, va_arg(ap, const char *));
                break;

            // A hex dump: pointer, then byte count; a space every four bytes.
            case 'h':
            {
                const unsigned char *p = va_arg(ap, const unsigned char *);
                int len = va_arg(ap, int);

                for (int k = 0; k < len; k++)
                {
                    char hex[] = "0123456789ABCDEF";

                    if (k != 0 && (k & 3) == 0)
                    {
                        dst[pos++] = ' ';
                        if (pos + 1 == DstCount)
                            break;
                    }
                    dst[pos++] = hex[p[k] >> 4];
                    if (pos + 1 == DstCount)
                        break;
                    dst[pos++] = hex[p[k] & 15];
                    if (pos + 1 == DstCount)
                        break;
                }
                break;
            }

            case 'c':
            case 'C':
                if (pos + 1 < DstCount)
                    dst[pos++] = va_arg(ap, T);
                break;

            case '%':
                if (pos + 1 < DstCount)
                    dst[pos++] = '%';
                break;

            case 'I':
                sprintf(tmp, "%lli", va_arg(ap, long));
                AppendString(dst, pos, DstCount, tmp);
                break;

            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
                sprintf(tmp, spec, va_arg(ap, double));
                AppendString(dst, pos, DstCount, tmp);
                break;

            // No `break`: %n stores the count and then ALSO consumes the next
            // argument and prints it as an integer.  Shipped.
            case 'n':
                *va_arg(ap, int *) = pos;

            case 'd':
            case 'i':
            case 'o':
            case 'p':
            case 'u':
            case 'x':
            case 'X':
                sprintf(tmp, spec, va_arg(ap, int));
                AppendString(dst, pos, DstCount, tmp);
                break;

            default:




























                VIASSERTF(0, "VIString::vsnprintf16 unimplemented format '%s'", spec);
                break;
            }
        }
        else
            dst[pos++] = fmt[i++];
    }

done:
    dst[pos] = 0;
}

// Phase 61, wave 6: NO `return` -- like the overload below, it falls off the
// end with the callee's result still in v0.  `return f()` puts a no-op
// `v0 = v0` between the call and the final USE, so sched1's
// add_branch_dependences stops at it; without it the trailing call is made to
// depend on every insn in the block, those LOG_LINKS survive into sched2, and
// `move a0,s0` out-ranks the prologue's `move s8,sp` by one dependent (the
// target puts s8 in the strlen delay slot).
int VIString::vsnprintf16(unsigned short *d, int n, const char *f, char *ap)
{
    int size = (strlen(f) + 1) * 2;
    unsigned short *t = (unsigned short *)__builtin_alloca(size);
    str8to16(t, size, f);
    vsnprintf16(d, n, t, ap);
}

int VIString::vsnprintf16(unsigned short *d, int n, const unsigned short *f, char *ap)
{
    VI_vsnprintf(d, n, f, ap);
}

// Phase 61, wave 3: VOID (no caller reads a result, and the tail call leaves
// vsnprintf's v0 either way).  The return type is visible only at the
// callers: VITimeOfDay::GetLocalTime's schedule needs the call not to set v0.
void VIString::snprintf(char *d, int n, const char *f, ...)
{
    va_list ap;
    va_start(ap, f);
    vsnprintf(d, n, f, ap);
}

// VOID, as snprintf above: the type shows only at the callers, which need the
// call not to set v0 -- three SETUP screens (VIWndController::OnDrawSelf,
// VIWndEula::OnKeyDown, VIWndOptions::OnDrawSelf) and eight CLIENT functions
// (PetCommand, InviteEntity, Msg0057, Msg0770, Msg0771, QuestStage,
// RequestTrade, ConnectStatusString).
void VIString::snprintf16(unsigned short *d, int n, const char *f, ...)
{
    va_list ap;
    va_start(ap, f);
    vsnprintf16(d, n, f, ap);
}

void VIString::snprintf16(unsigned short *d, int n, const unsigned short *f, ...)
{
    va_list ap;
    va_start(ap, f);
    vsnprintf16(d, n, f, ap);
}

char *VIString::strlwr(char *p)
{
    return ::strlwr(p);
}

void VIString::strupr(char *p)
{
    ::strupr(p);
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
    return ::strcasecmp(a, b);
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
    return ::strncasecmp(a, b, n);
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
    unsigned short *d = (unsigned short *)malloc(size);
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
