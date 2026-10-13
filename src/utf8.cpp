// utf8.cpp -- SUPPORT 0x0049DA40 .. 0x0049E618.  Phase 61.
//
// The UTF-8 string helpers: free functions, no class, between VISound's TU and
// VIBattleMusic's.  Only one- to three-byte sequences convert (UCS-2 out); the
// size table still answers 4, 5 and 6 for the longer lead bytes.
#include "vi.h"

extern "C" unsigned int strlen(const char *);
extern "C" char *strstr(const char *, const char *);
extern "C" int   strcmp(const char *, const char *);
extern "C" int   strncmp(const char *, const char *, unsigned int);
extern "C" void *memcpy(void *, const void *, unsigned int);

// Indexed by bits 2..5 of a byte with the top bit set.  A continuation byte
// (10xxxxxx) reads as a two-byte lead -- bit 6 is never looked at.
static char s_utf8Size[16] = { 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 6 };

int UTF8_charSize(const char *s)
{
    unsigned char c = *s;

    if (!(c & 0x80))
        return 1;
    return s_utf8Size[(c & 0x3C) >> 2];
}

// Every invalid form returns 0 and a one-byte character its size; the multibyte
// checks are NESTED under `n != 1` with one `return n` at the bottom.
int UTF8_charValid(char *s)
{
    int n = UTF8_charSize(s);

    if (n == 0)
        return 0;
    if (n != 1)
    {
        unsigned char c = *s;

        if ((c & 0xC0) != 0xC0)
            return 0;
        if ((c >> (7 - n)) & 1)
            return 0;
        for (int i = 1; i < n; i++)
            if ((s[i] & 0xC0) != 0x80)
                return 0;
    }
    return n;
}

int UTF8_valid(char *s)
{
    while (*s)
    {
        int n = UTF8_charValid(s);

        if (n == 0)
            return 0;
        s += n;
    }
    return 1;
}

int UTF8_size(char *s)
{
    return strlen(s);
}

int UTF8_strlen(const char *s)
{
    int n = 0;

    while (*s)
    {
        n++;
        s += UTF8_charSize(s);
    }
    return n;
}

// Shipped: the null test reads s[0] every pass, not the character at `i`, so
// only `max` ends the walk once the string has a first byte.
int UTF8_strlen(const char *s, int max)
{
    int n = 0;
    int i = 0;

    while (*s && i < max)
    {
        n++;
        i += UTF8_charSize(s + i);
    }
    return n;
}

char *UTF8_strchr(char *s, char *c)
{
    return strstr(s, c);
}

// Shipped: the continuation-byte skip multiplies where it means to mask --
// `*p * 0xC0 == 0x80` has no solution, so the inner loop never runs and every
// byte position is tried.
char *UTF8_strrchr(char *s, char *c)
{
    char *p = s;
    int len = UTF8_size(c);

    while (*p)
        p++;

    while (p != s)
    {
        p--;
        while (*p * 0xC0 == 0x80)
            p--;
        if (strncmp(p, c, len) == 0)
            return p;
    }
    return 0;
}

int UTF8_charCopy(char *dst, char *src)
{
    int n = UTF8_charSize(src);
    int k = n;

    while (k--)
        *dst++ = *src++;
    return n;
}

char *UTF8_characterAt(char *s, int idx)
{
    while (idx && *s)
    {
        idx--;
        s += UTF8_charSize(s);
    }
    if (idx == 0)
        return s;
    return 0;
}

// Copies at most n-1 bytes and never splits a sequence: when the string does
// not fit, the cut backs up over continuation bytes first.
char *UTF8_strncpy(char *dst, char *src, int n)
{
    int len = strlen(src);

    if (len < n)
        memcpy(dst, src, len + 1);
    else
    {
        char *p = src + (n - 1);

        while ((*p & 0xC0) == 0x80)
            p--;
        int k = p - src;
        memcpy(dst, src, k);
        dst[k] = 0;
    }
    return dst;
}

// 0 when the two characters are the same.  Unrolled to the six bytes the size
// table can answer, and ORing every byte's difference into the result.
int UTF8_chrcmp(char *a, char *b)
{
    int r;

    if (!(*a & 0x80))
        r = *a != *b;
    else
    {
        int n = UTF8_charSize(a);

        r = n != UTF8_charSize(b);
        r |= *a++ != *b++;
        if (n > 1)
            r |= *a++ != *b++;
        if (n > 2)
            r |= *a++ != *b++;
        if (n > 3)
            r |= *a++ != *b++;
        if (n > 4)
            r |= *a++ != *b++;
        if (n > 5)
            r |= *a != *b;
    }
    return r;
}

// Case-insensitive only in the ASCII range, by masking bit 5 -- which also
// equates '@' with '`', '[' with '{' and so on.
int UTF8_chricmp(char *a, char *b)
{
    int r;

    if (!(*a & 0x80))
        r = (*a & 0xDF) != (*b & 0xDF);
    else
    {
        int n = UTF8_charSize(a);

        r = n != UTF8_charSize(b);
        r |= *a++ != *b++;
        if (n > 1)
            r |= *a++ != *b++;
        if (n > 2)
            r |= *a++ != *b++;
        if (n > 3)
            r |= *a++ != *b++;
        if (n > 4)
            r |= *a++ != *b++;
        if (n > 5)
            r |= *a != *b;
    }
    return r;
}

int UTF8_strcmp(char *a, char *b)
{
    return strcmp(a, b);
}

int UTF8_stricmp(char *a, char *b)
{
    int r = 0;

    while (!r)
    {
        r = UTF8_chricmp(a, b);
        if (!*a || !*b)
        {
            r |= *a != *b;
            break;
        }
        a += UTF8_charSize(a);
        b += UTF8_charSize(b);
    }
    return r;
}

int UTF8_strncmp(char *a, char *b, int n)
{
    int r = 0;

    for (int i = 0; i < n && !r; i++)
    {
        r = UTF8_chrcmp(a, b);
        if (!*a || !*b)
            break;
        a += UTF8_charSize(a);
        b += UTF8_charSize(b);
    }
    return r;
}

int UTF8_strnicmp(char *a, char *b, int n)
{
    int r = 0;

    for (int i = 0; i < n && !r; i++)
    {
        r = UTF8_chricmp(a, b);
        if (!*a || !*b)
            break;
        a += UTF8_charSize(a);
        b += UTF8_charSize(b);
    }
    return r;
}

int UTF8_convertCharToUTF16(const char *s, unsigned short *out)
{
    int n = UTF8_charSize(s);

    if (n == 1)
        *out = s[0];
    else if (n == 2)
        *out = ((s[0] & 0x1F) << 6) + (s[1] & 0x3F);
    else if (n == 3)
        *out = ((s[0] & 0x0F) << 12) + ((s[1] & 0x3F) << 6) + (s[2] & 0x3F);
    return n;
}

int UTF8_convertToUTF16(const char *s, unsigned short *out, int max)
{
    int n = 0;

    while (*s && n < max)
    {
        s += UTF8_convertCharToUTF16(s, out);
        out++;
        n++;
    }
    *out = 0;
    return n;
}

// Shipped: 0x80 itself takes the one-byte path (`<= 0x80`, not `< 0x80`).
int UTF16_convertCharToUTF8(unsigned short c, char *out, int max)
{
    int n = 0;

    if (c <= 0x80)
    {
        if (max > 0)
        {
            out[0] = c;
            n = 1;
        }
    }
    else if (c < 0x800)
    {
        if (max >= 2)
        {
            n = 2;
            out[0] = 0xC0 + ((c & 0x7C0) >> 6);
            // The cast is the target's `addiu -128` where the three-byte arm
            // has `+128`: the add is done in char, so 0x80 folds to -128.
            out[1] = (char)(c & 0x3F) + 0x80;
        }
    }
    else if (max >= 3)
    {
        out[0] = 0xE0 + (c >> 12);
        out[1] = 0x80 + ((c & 0xFC0) >> 6);
        out[2] = 0x80 + (c & 0x3F);
        n = 3;
    }
    return n;
}

int UTF16_convertToUTF8(const unsigned short *s, char *out, int max)
{
    while (*s && max)
    {
        int n = UTF16_convertCharToUTF8(*s, out, max);

        if (n == 0)
            break;
        out += n;
        max -= n;
        s++;
    }
    *out = 0;
    return 0;
}
