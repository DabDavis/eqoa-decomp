// delta.cpp (name ours) -- SETUP 0x0057CEB0 .. 0x0057D938: the patcher's delta
// coder.  MakeDeltaData indexes the old data every `step` bytes by its first 16
// bytes, sorts the index, and for each position in the new data takes the
// longest match among the entries with the same 16-byte key.
#include "patcher.h"

static inline void PutInt(char *p, int v)
{
    p[0] = v >> 24;
    p[1] = v >> 16;
    p[2] = v >> 8;
    p[3] = v;
}

static inline int GetInt(char *p)
{
    unsigned char *u = (unsigned char *)p;
    return (u[0] << 24) | (u[1] << 16) | (u[2] << 8) | u[3];
}

static void SortIndex(char **index, int count);

// 0x0057CEB0.
char *MakeDeltaData(char *from, int fromSize, char *to, int toSize, int *outSize, int step)
{
    if (step <= 0)
        step = 1;
    int n = fromSize - 16;
    if (n < 0)
        n = 0;
    int count = n / step;
    char **index = 0;
    if (count > 2)
    {
        index = (char **)malloc(count * sizeof(char *));
        char **ip = index;
        char *f = from;
        for (int i = 0; i < count; i++)
        {
            *ip++ = f;
            f += step;
        }
        SortIndex(index, count);
    }
    char *out = (char *)malloc(toSize + 1024);
    int remaining = toSize;
    char *cur = to;
    char *p = out;
    PutInt(p, 1);
    p += 4;
    PutInt(p, toSize);
    p += 4;
    char *lit = p;
    while (remaining > 0)
    {
        int best = 0;
        char *bestPtr = 0;
        if (index && remaining > 16 && fromSize > 16)
        {
            char **end = index + count;
            for (char **e = (char **)DeltaIBSearch(cur, index, count, sizeof(char *));
                 e < end && !memcmp(*e, cur, 16); e++)
            {
                int max = remaining - 16;
                char *a = *e + 16;
                char *b = cur + 16;
                int avail = fromSize - (a - from);
                if (avail < max)
                    max = avail;
                char *limit = a + max;
                while (*a == *b && a < limit)
                {
                    a++;
                    b++;
                }
                int len = b - cur;
                if (best < len)
                {
                    best = len;
                    bestPtr = *e;
                    if (best >= 512)
                        break;
                }
            }
        }
        if (best >= 16)
        {
            int len = p - lit;
            if (len > 0)
                PutInt(lit, 4 - len);
            PutInt(p, best);
            p += 4;
            PutInt(p, bestPtr - from);
            p += 4;
            lit = p;
            cur += best;
            remaining -= best;
        }
        else
        {
            if (p == lit)
                p += 4;
            int k = remaining > 1 ? 1 : remaining;
            memcpy(p, cur, k);
            p += k;
            cur += k;
            remaining -= k;
        }
    }
    int len = p - lit;
    if (len > 0)
        PutInt(lit, 4 - len);
    if (index)
        free(index);
    *outSize = p - out;
    return out;
}

// 0x0057D218.
char *UnmakeDeltaData(char *from, int fromSize, char *delta, int deltaSize, int *outSize)
{
    char *end = delta + deltaSize;
    delta += 4;
    *outSize = GetInt(delta);
    delta += 4;
    char *out = (char *)malloc(*outSize);
    char *q = out;
    while (delta < end)
    {
        int n = GetInt(delta);
        delta += 4;
        if (n < 0)
        {
            memcpy(q, delta, -n);
            q -= n;
            delta -= n;
        }
        else
        {
            int off = GetInt(delta);
            delta += 4;
            memcpy(q, from + off, n);
            q += n;
        }
    }
    return out;
}

// 0x0057D350.  The old file is renamed aside and read back through a 256 KB
// buffer; the new one is written in its place.
int UnmakeDeltaDataFile(char *file, char *delta, int deltaSize)
{
    int ok = 0;
    remove("delta.hld");
    rename(file, "delta.hld");
    FILE *in = fopen("delta.hld", "rb");
    if (in)
    {
        FILE *out = fopen(file, "wb");
        if (out)
        {
            char *end = delta + deltaSize;
            char *buf = (char *)malloc(0x40000);
            char *p = delta + 8;
            while (p < end)
            {
                int n = GetInt(p);
                p += 4;
                if (n < 0)
                {
                    fwrite(p, 1, -n, out);
                    p -= n;
                }
                else
                {
                    int off = GetInt(p);
                    p += 4;
                    fseek(in, off, SEEK_SET);
                    int left = n;
                    while (left > 0)
                    {
                        int chunk = left > 0x40000 ? 0x40000 : left;
                        fread(buf, 1, chunk, in);
                        fwrite(buf, 1, chunk, out);
                        left -= chunk;
                    }
                }
            }
            fclose(out);
            ok = 1;
            free(buf);
        }
        fclose(in);
    }
    remove("delta.hld");
    return ok;
}

// The sort's cursors are file statics, not locals: each recursion leaves them
// for the next.
static char **gLo = 0;
static char **gHi = 0;
static char *gTemp;
static char *gPivot;

static void QSort(char **lo, char **hi);

// 0x0057D530.
static void SortIndex(char **index, int count)
{
    if (count > 1)
        QSort(index, &index[count - 1]);
}

// 0x0057D560.  By the first 16 bytes; a run already in order is left alone,
// and the pivot is the median of three once there are nine or more.
static void QSort(char **lo, char **hi)
{
    gHi = hi;
    gLo = lo;
    while (gLo < gHi && memcmp(gLo[0], gLo[1], 16) <= 0)
        gLo++;
    if (gLo == gHi)
        return;
    gLo = lo;
    int n = gHi - gLo;
    gPivot = gLo[n / 2];
    if (n >= 9)
    {
        if (memcmp(*gLo, gPivot, 16) < 0)
        {
            if (memcmp(*gLo, *gHi, 16) > 0)
                gPivot = *gLo;
            else if (memcmp(*gHi, gPivot, 16) < 0)
                gPivot = *gHi;
        }
        else
        {
            if (memcmp(*gLo, *gHi, 16) < 0)
                gPivot = *gLo;
            else if (memcmp(*gHi, gPivot, 16) > 0)
                gPivot = *gHi;
        }
    }
    while (gLo < gHi)
    {
        while (gLo < gHi && memcmp(*gLo, gPivot, 16) < 0)
            gLo++;
        while (gHi > gLo && memcmp(*gHi, gPivot, 16) > 0)
            gHi--;
        if (gLo != gHi)
        {
            gTemp = *gLo;
            *gLo = *gHi;
            *gHi = gTemp;
            gLo++;
            gHi--;
        }
    }
    if (gLo == gHi && memcmp(*gLo, gPivot, 16) <= 0)
        gLo++;
    char **mid = gLo;
    if (lo != mid - 1)
        QSort(lo, mid - 1);
    if (mid != hi)
        QSort(mid, hi);
}

// 0x0057D828.  The FIRST entry whose key equals `key`, or where it would go.
void *DeltaIBSearch(void *key, void *base, int count, int size)
{
    char *mid = 0;
    int cmp = 0;
    if (count == 0)
        return base;
    char *lo = (char *)base;
    char *hi = (char *)base + (count - 1) * size;
    while (lo <= hi)
    {
        mid = lo + (hi - lo) / size / 2 * size;
        cmp = memcmp(key, *(void **)mid, 16);
        if (cmp < 0)
        {
            if (mid == base)
                break;
            hi = mid - size;
        }
        else if (cmp > 0)
            lo = mid + size;
        else
        {
            for (;;)
            {
                if (mid == base)
                    return mid;
                mid -= size;
                if (memcmp(key, *(void **)mid, 16))
                    return mid + size;
            }
        }
    }
    mid += (cmp > 0) * size;
    return mid;
}
