// VIBRect / VIRect -- SUPPORT 0x003EF750 .. 0x003EFE60 and 0x003F9EB0 ..
// 0x003FA3B0.  The float and integer twins of VIBBox in 2D, and they are
// written from the same text: every shape VIBBox established reappears here,
// including one that is a shipped BUG in both (see Store).
#include "vi.h"

// An inline FUNCTION, not the VIMIN macro: its return value is a pseudo of
// its own, which is what gives v.x the lower FP register.
static inline float BRMin(float a, float b) { return (a > b) ? b : a; }

void VIBRect::Bound(int first, const VIVect2 &v)
{
    if (!first)
    {
        min.x = BRMin(min.x, v.x);
        min.y = BRMin(min.y, v.y);
        VIVect2 *mx = &max;
        mx->x = (v.x < mx->x) ? mx->x : v.x;
        mx->y = (v.y < mx->y) ? mx->y : v.y;
    }
    else
    {
        min = v;
        max = v;
    }
}

void VIBRect::Union(const VIBRect &b)
{
    min.x = (b.min.x < min.x) ? b.min.x : min.x;
    min.y = (b.min.y < min.y) ? b.min.y : min.y;
    max.x = (b.max.x < max.x) ? max.x : b.max.x;
    max.y = (b.max.y < max.y) ? max.y : b.max.y;
}

void VIBRect::Union(const VIBRect &a, const VIBRect &b)
{
    min.x = (b.min.x < a.min.x) ? b.min.x : a.min.x;
    min.y = (b.min.y < a.min.y) ? b.min.y : a.min.y;
    max.x = (b.max.x < a.max.x) ? a.max.x : b.max.x;
    max.y = (b.max.y < a.max.y) ? a.max.y : b.max.y;
}

int VIBRect::Intersect(const VIBRect &a, const VIBRect &b)
{
    min.x = (b.min.x < a.min.x) ? a.min.x : b.min.x;
    min.y = (b.min.y < a.min.y) ? a.min.y : b.min.y;
    max.x = (b.max.x < a.max.x) ? b.max.x : a.max.x;
    max.y = (b.max.y < a.max.y) ? b.max.y : a.max.y;

    if (max.x < min.x) return 0;
    if (max.y < min.y) return 0;
    return 1;
}

int VIBRect::Contains(const VIVect2 &v) const
{
    if (v.x >= min.x && v.y >= min.y && v.x <= max.x && v.y <= max.y)
        return 1;
    return 0;
}

int VIBRect::Intersects(const VIBRect &b) const
{
    if (min.x > b.max.x || max.x < b.min.x ||
        min.y > b.max.y || max.y < b.min.y)
        return 0;
    return 1;
}

// A SHIPPED BUG, and the same one in VIRect::Store: the y of the last two
// corners is min.y, so the "rectangle" is degenerate.  Neither function ever
// reads +12 (max.y) at all.  It matches byte for byte written this way.
int VIBRect::Store(VIVect2 *p) const
{
    p[0].x = min.x;  p[0].y = min.y;
    p[1].x = max.x;  p[1].y = min.y;
    p[2].x = max.x;  p[2].y = min.y;
    p[3].x = min.x;  p[3].y = min.y;
    return 0;
}

VIVect2 VIBRect::Size()
{
    return VIVect2(max.x - min.x, max.y - min.y);
}

VIVect2 VIBRect::Center() const
{
    return VIVect2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
}

float VIBRect::DistanceSquared(const VIVect2 &v) const
{
    VIVect2 d;

    d.x = (d.x < min.x - v.x) ? min.x - v.x : d.x;
    d.y = (d.y < min.y - v.y) ? min.y - v.y : d.y;
    d.x = (d.x < v.x - max.x) ? v.x - max.x : d.x;
    d.y = (d.y < v.y - max.y) ? v.y - max.y : d.y;

    return d.x * d.x + d.y * d.y;
}

// Component-wise helpers taking REFERENCES, the shape of VIVect2's own
// operators: the first access through each folds into the base, the rest go
// through the reference's own pointer.
static inline void BRAdd(VIVect2 &a, const VIVect2 &b) { a.x = a.x + b.x; a.y = a.y + b.y; }
static inline void BRSub(VIVect2 &a, const VIVect2 &b) { a.x = a.x - b.x; a.y = a.y - b.y; }
static inline void BRMul(VIVect2 &a, float k) { a.x = a.x * k; a.y = a.y * k; }
static inline void BRMul(VIVect2 &a, const VIVect2 &k) { a.x = a.x * k.x; a.y = a.y * k.y; }

void VIBRect::Scale(float s)
{
    VIVect2 c;
    c = min;
    BRAdd(c, max);
    BRSub(min, c);
    BRMul(min, s);
    BRAdd(min, c);
    BRSub(max, c);
    BRMul(max, s);
    BRAdd(max, c);
}

void VIBRect::Scale(const VIVect2 &s)
{
    VIVect2 c;
    c = min;
    BRAdd(c, max);
    BRSub(min, c);
    BRMul(min, s);
    BRAdd(min, c);
    BRSub(max, c);
    BRMul(max, s);
    BRAdd(max, c);
}

// ------------------------------------------------------------------ VIRect --

// The min/max are inline FUNCTIONS, both written on `a > b`: VIRect's
// Union/Intersect match only with these, not with the VIMIN/VIMAX macros.
static inline int IRMax(int a, int b) { return (a > b) ? a : b; }
static inline int IRMin(int a, int b) { return (a > b) ? b : a; }

void VIRect::Bound(int first, const VIVect2I &v)
{
    if (!first)
    {
        min.x = (v.x < min.x) ? v.x : min.x;
        min.y = (v.y < min.y) ? v.y : min.y;
        VIVect2I *mx = &max;
        mx->x = (v.x < mx->x) ? mx->x : v.x;
        mx->y = (v.y < mx->y) ? mx->y : v.y;
    }
    else
    {
        min = v;
        max = v;
    }
}

void VIRect::Union(const VIRect &b)
{
    min.x = IRMin(min.x, b.min.x);
    min.y = IRMin(min.y, b.min.y);
    max.x = IRMax(max.x, b.max.x);
    max.y = IRMax(max.y, b.max.y);
}

void VIRect::Union(const VIRect &a, const VIRect &b)
{
    min.x = IRMin(a.min.x, b.min.x);
    min.y = IRMin(a.min.y, b.min.y);
    max.x = IRMax(a.max.x, b.max.x);
    max.y = IRMax(a.max.y, b.max.y);
}

int VIRect::Intersect(const VIRect &a, const VIRect &b)
{
    min.x = IRMax(a.min.x, b.min.x);
    min.y = IRMax(a.min.y, b.min.y);
    max.x = IRMin(a.max.x, b.max.x);
    max.y = IRMin(a.max.y, b.max.y);

    if (max.x < min.x) return 0;
    if (max.y < min.y) return 0;
    return 1;
}

int VIRect::Contains(const VIVect2I &v) const
{
    if (v.x >= min.x && v.y >= min.y && v.x <= max.x && v.y <= max.y)
        return 1;
    return 0;
}

int VIRect::Intersects(const VIRect &b) const
{
    if (min.x > b.max.x || max.x < b.min.x ||
        min.y > b.max.y || max.y < b.min.y)
        return 0;
    return 1;
}

int VIRect::Store(VIVect2I *p) const
{
    p[0].x = min.x;  p[0].y = min.y;
    p[1].x = max.x;  p[1].y = min.y;
    p[2].x = max.x;  p[2].y = min.y;
    p[3].x = min.x;  p[3].y = min.y;
    return 0;
}

// The integer rectangle is INCLUSIVE: both extents are +1, and Area is their
// product through the R5900's three-operand `mult`.
VIVect2I VIRect::Size() const
{
    return VIVect2I(max.x - min.x + 1, max.y - min.y + 1);
}

int VIRect::Area()
{
    return (max.x - min.x + 1) * (max.y - min.y + 1);
}

// A shift, not a divide: `/ 2` on a signed int needs the round-toward-zero
// correction and the target has a bare `sra`.
VIVect2I VIRect::Center() const
{
    return VIVect2I((min.x + max.x) >> 1, (min.y + max.y) >> 1);
}


// The first pair through IRMax: 2 wrong words where the ternary is 4 (the
// target compares `x < 1` -- LE against a LITERAL 0 -- not reached yet).

int VIRect::DistanceSquared(const VIVect2I &v) const
{
    VIVect2I d;

    d.x = IRMax(d.x, min.x - v.x);
    d.y = IRMax(d.y, min.y - v.y);
    d.x = (d.x < v.x - max.x) ? v.x - max.x : d.x;
    d.y = (d.y < v.y - max.y) ? v.y - max.y : d.y;

    return d.x * d.x + d.y * d.y;
}

// The integer twins of the BR* helpers.
static inline void IRAdd(VIVect2I &a, const VIVect2I &b) { a.x = a.x + b.x; a.y = a.y + b.y; }
static inline void IRSub(VIVect2I &a, const VIVect2I &b) { a.x = a.x - b.x; a.y = a.y - b.y; }
static inline void IRMul(VIVect2I &a, int k) { a.x = a.x * k; a.y = a.y * k; }
static inline void IRMul(VIVect2I &a, const VIVect2I &k) { a.x = a.x * k.x; a.y = a.y * k.y; }

void VIRect::Scale(int s)
{
    VIVect2I c;
    c = min;
    IRAdd(c, max);
    IRSub(min, c);
    IRMul(min, s);
    IRAdd(min, c);
    IRSub(max, c);
    IRMul(max, s);
    IRAdd(max, c);
}

void VIRect::Scale(const VIVect2I &s)
{
    VIVect2I c;
    c = min;
    IRAdd(c, max);
    IRSub(min, c);
    IRMul(min, s);
    IRAdd(min, c);
    IRSub(max, c);
    IRMul(max, s);
    IRAdd(max, c);
}

// The same degenerate-corner bug as Store: the y of every corner is min.y,
// and +12 (max.y) is never read.
int VIBRect::Transform(const VIMatrix33 &m, VIVect2 *p) const
{
    VIVect3 v;

    v.x = min.x;  v.y = min.y;  v.z = 1.0f;
    v = v * m;
    p[0].x = v.x;  p[0].y = v.y;

    v.x = max.x;  v.y = min.y;  v.z = 1.0f;
    v = v * m;
    p[1].x = v.x;  p[1].y = v.y;

    v.x = max.x;  v.y = min.y;  v.z = 1.0f;
    v = v * m;
    p[2].x = v.x;  p[2].y = v.y;

    v.x = min.x;  v.y = min.y;  v.z = 1.0f;
    v = v * m;
    p[3].x = v.x;  p[3].y = v.y;

    return 0;
}
