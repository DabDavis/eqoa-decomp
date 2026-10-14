// VIBBox / VISphere -- SUPPORT 0x003EE220 .. 0x003FA678.
//
// Style fact this class establishes: the author writes vector work as
// ASSIGN-THEN-MODIFY-IN-PLACE, one statement per step --
//
//     c.x = min.x;  c.x = c.x + max.x;  c.x = c.x * 0.5f;
//
// which is visible in the object code as three separate stores to the same
// slot, because a store through `this` may alias the const reference the next
// statement reads.  A single expression collapses them.
#include "vi.h"

int VIBBox::Init(int n, VIVect3 *p)
{
    for (int i = 0; i < n; i++)
        Bound(i == 0, p[i]);
    return 0;
}
// An inline FUNCTION, not the VIMIN macro: its return value is a pseudo of
// its own, which is what gives v.x the lower FP register (phase 61).
static inline float BBMin(float a, float b) { return (a > b) ? b : a; }


// The `else` branch is inline and the `if` body at the branch target, so the
// source's `if` holds the six-store initialisation (Phase 7's rule).
//
// `VIVect3 *mx = &max;` is load-bearing: the target reads max.y / max.z off a
// materialised `a0+12` and folds only the +0 access back to `12(a0)`.  Every
// spelling of `max.y` reaches 16(a0) directly instead.
//
// The min side goes through BBMin, an inline function: with the VIMIN macro
// v.x and the result swap FP registers (and the shared delay-slot load the
// target has cannot form).
void VIBBox::Bound(int first, const VIVect3 &v)
{
    if (!first)
    {
        min.x = BBMin(min.x, v.x);
        min.y = BBMin(min.y, v.y);
        min.z = BBMin(min.z, v.z);
        VIVect3 *mx = &max;
        mx->x = (v.x < mx->x) ? mx->x : v.x;
        mx->y = (v.y < mx->y) ? mx->y : v.y;
        mx->z = (v.z < mx->z) ? mx->z : v.z;
    }
    else
    {
        min = v;
        max = v;
    }
}

void VIBBox::Union(const VIBBox &b)
{
    min.x = (b.min.x < min.x) ? b.min.x : min.x;
    min.y = (b.min.y < min.y) ? b.min.y : min.y;
    min.z = (b.min.z < min.z) ? b.min.z : min.z;
    max.x = (b.max.x < max.x) ? max.x : b.max.x;
    max.y = (b.max.y < max.y) ? max.y : b.max.y;
    max.z = (b.max.z < max.z) ? max.z : b.max.z;
}

void VIBBox::Union(const VIBBox &a, const VIBBox &b)
{
    min.x = (b.min.x < a.min.x) ? b.min.x : a.min.x;
    min.y = (b.min.y < a.min.y) ? b.min.y : a.min.y;
    min.z = (b.min.z < a.min.z) ? b.min.z : a.min.z;
    max.x = (b.max.x < a.max.x) ? a.max.x : b.max.x;
    max.y = (b.max.y < a.max.y) ? a.max.y : b.max.y;
    max.z = (b.max.z < a.max.z) ? a.max.z : b.max.z;
}

int VIBBox::Intersect(const VIBBox &a, const VIBBox &b)
{
    min.x = (b.min.x < a.min.x) ? a.min.x : b.min.x;
    min.y = (b.min.y < a.min.y) ? a.min.y : b.min.y;
    min.z = (b.min.z < a.min.z) ? a.min.z : b.min.z;
    max.x = (b.max.x < a.max.x) ? b.max.x : a.max.x;
    max.y = (b.max.y < a.max.y) ? b.max.y : a.max.y;
    max.z = (b.max.z < a.max.z) ? b.max.z : a.max.z;

    if (max.x < min.x) return 0;
    if (max.y < min.y) return 0;
    if (max.z < min.z) return 0;
    return 1;
}

int VIBBox::Contains(const VIVect3 &v) const
{
    if (v.x >= min.x && v.y >= min.y && v.z >= min.z &&
        v.x <= max.x && v.y <= max.y && v.z <= max.z)
        return 1;
    return 0;
}

int VIBBox::Intersects(const VIBBox &b) const
{
    if (min.x > b.max.x || max.x < b.min.x ||
        min.y > b.max.y || max.y < b.min.y ||
        min.z > b.max.z || max.z < b.min.z)
        return 0;
    return 1;
}

int VIBBox::Store(VIVect3 *p) const
{
    p[0].x = min.x;  p[0].y = min.y;  p[0].z = min.z;
    p[1].x = max.x;  p[1].y = min.y;  p[1].z = min.z;
    p[2].x = max.x;  p[2].y = min.y;  p[2].z = max.z;
    p[3].x = min.x;  p[3].y = min.y;  p[3].z = max.z;
    p[4].x = min.x;  p[4].y = max.y;  p[4].z = min.z;
    p[5].x = max.x;  p[5].y = max.y;  p[5].z = min.z;
    p[6].x = max.x;  p[6].y = max.y;  p[6].z = max.z;
    p[7].x = min.x;  p[7].y = max.y;  p[7].z = max.z;
    return 0;
}

int VIBBox::StoreOutward(VIPlane *p) const
{
    VIVect3 *n0 = &p[0].n;
    n0->x = -1.0f;  n0->y =  0.0f;  n0->z =  0.0f;  p[0].d =  min.x;
    VIVect3 *n1 = &p[1].n;
    n1->x =  1.0f;  n1->y =  0.0f;  n1->z =  0.0f;  p[1].d = -max.x;
    VIVect3 *n2 = &p[2].n;
    n2->x =  0.0f;  n2->y = -1.0f;  n2->z =  0.0f;  p[2].d =  min.y;
    VIVect3 *n3 = &p[3].n;
    n3->x =  0.0f;  n3->y =  1.0f;  n3->z =  0.0f;  p[3].d = -max.y;
    VIVect3 *n4 = &p[4].n;
    n4->x =  0.0f;  n4->y =  0.0f;  n4->z = -1.0f;  p[4].d =  min.z;
    VIVect3 *n5 = &p[5].n;
    n5->x =  0.0f;  n5->y =  0.0f;  n5->z =  1.0f;  p[5].d = -max.z;
    return 0;
}

int VIBBox::StoreInward(VIPlane *p) const
{
    VIVect3 *n0 = &p[0].n;
    n0->x =  1.0f;  n0->y =  0.0f;  n0->z =  0.0f;  p[0].d = -min.x;
    VIVect3 *n1 = &p[1].n;
    n1->x = -1.0f;  n1->y =  0.0f;  n1->z =  0.0f;  p[1].d =  max.x;
    VIVect3 *n2 = &p[2].n;
    n2->x =  0.0f;  n2->y =  1.0f;  n2->z =  0.0f;  p[2].d = -min.y;
    VIVect3 *n3 = &p[3].n;
    n3->x =  0.0f;  n3->y = -1.0f;  n3->z =  0.0f;  p[3].d =  max.y;
    VIVect3 *n4 = &p[4].n;
    n4->x =  0.0f;  n4->y =  0.0f;  n4->z =  1.0f;  p[4].d = -min.z;
    VIVect3 *n5 = &p[5].n;
    n5->x =  0.0f;  n5->y =  0.0f;  n5->z = -1.0f;  p[5].d =  max.z;
    return 0;
}

VIVect3 VIBBox::Size() const
{
    return VIVect3(max.x - min.x, max.y - min.y, max.z - min.z);
}

VIVect3 VIBBox::Center() const
{
    return VIVect3((min.x + max.x) * 0.5f,
                   (min.y + max.y) * 0.5f,
                   (min.z + max.z) * 0.5f);
}

float VIBBox::DistanceSquared(const VIVect3 &v) const
{
    VIVect3 d;

    d.x = (d.x < min.x - v.x) ? min.x - v.x : d.x;
    d.y = (d.y < min.y - v.y) ? min.y - v.y : d.y;
    d.z = (d.z < min.z - v.z) ? min.z - v.z : d.z;
    d.x = (d.x < v.x - max.x) ? v.x - max.x : d.x;
    d.y = (d.y < v.y - max.y) ? v.y - max.y : d.y;
    d.z = (d.z < v.z - max.z) ? v.z - max.z : d.z;

    return d.x * d.x + d.y * d.y + d.z * d.z;
}

// Both Scales (and the VIBRect/VIRect twins) are VIVect3's own compound
// operators on min and max: each operator's reference gives `max` a pointer of
// its own and re-reads `c` off the stack, which is the divergence the old
// field-by-field spelling showed (phase 61 closed all six on that).
//
// The pivot is `min + max`, NOT the centre -- there is no 0.5f anywhere in
// either Scale, where Center() and VISphere::Init both materialise 0x3F000000.
// So a box away from the origin is MOVED by Scale, not just resized.  A shipped
// bug, on the same footing as VIMatrix33::Translate writing one row past the
// matrix: it matches byte for byte written this way.
void VIBBox::Scale(float s)
{
    VIVect3 c;
    c = min;
    c += max;
    min -= c;
    min *= s;
    min += c;
    max -= c;
    max *= s;
    max += c;
}

// VIVect3 has no component-wise `*=`; this is its shape.
static inline void BBMul(VIVect3 &a, const VIVect3 &k)
{
    a.x = a.x * k.x;
    a.y = a.y * k.y;
    a.z = a.z * k.z;
}

void VIBBox::Scale(const VIVect3 &s)
{
    VIVect3 c;
    c = min;
    c += max;
    min -= c;
    BBMul(min, s);
    min += c;
    max -= c;
    BBMul(max, s);
    max += c;
}

// 0x003EE930.  A segment against the box: trivially out if both ends are
// past any one face, otherwise clipped face by face -- each slab plane cuts
// the segment where it crosses inside [0,1], replacing whichever end was
// outside -- and out the moment both ends are past the face being cut.
int VIBBox::Intersects(const VIVect3 &a, const VIVect3 &b) const
{
    VIVect3 p0;
    VIVect3 p1;
    VIVect3 hit;
    VIPlane pl;
    float t;
    int i;

    if ((a.x > max.x && b.x > max.x) || (a.y > max.y && b.y > max.y) ||
        (a.z > max.z && b.z > max.z) || (a.x < min.x && b.x < min.x) ||
        (a.y < min.y && b.y < min.y) || (a.z < min.z && b.z < min.z))
        return 0;

    p0 = a;
    p1 = b;

    // The normal and the far end through pointer variables: a different base
    // from `sp`, so the loop keeps them as givs of their own instead of
    // folding them onto p0's.
    VIVect3 *q1 = &p1;
    float *n = &pl.n.x;

    for (i = 0; i < 3; i++)
    {
        pl.n.Zero();
        n[i] = -1.0f;
        pl.d = (&min.x)[i];
        if (pl.LineIntersection(p0, *q1, t, hit) && 0.0f <= t && t <= 1.0f)
        {
            if ((&p0.x)[i] < (&min.x)[i])
                p0 = hit;
            else
                p1 = hit;
        }
        if ((&p0.x)[i] < (&min.x)[i] && (&q1->x)[i] < (&min.x)[i])
            return 0;
    }

    // n is set AGAIN for the max faces: the second set is a copy the loop
    // keeps (`move s4,s6`), and LineIntersection's `this` is taken off it.
    const float *mx = &max.x;
    n = &pl.n.x;
    for (i = 0; i < 3; i++)
    {
        pl.n.Zero();
        n[i] = 1.0f;
        pl.d = -mx[i];
        if (pl.LineIntersection(p0, *q1, t, hit) && 0.0f <= t && t <= 1.0f)
        {
            if ((&p0.x)[i] > mx[i])
                p0 = hit;
            else
                p1 = hit;
        }
        if ((&p0.x)[i] > mx[i] && (&q1->x)[i] > mx[i])
            return 0;
    }

    return 1;
}

int VIBBox::CullIntersect(const VISphere &s) const
{
    if (s.r < min.x - s.c.x || s.r < min.y - s.c.y || s.r < min.z - s.c.z ||
        s.r < s.c.x - max.x || s.r < s.c.y - max.y || s.r < s.c.z - max.z)
        return 1;
    return 0;
}

int VIBBox::Intersects(const VISphere &s) const
{
    VIVect3 d;

    d.x = (d.x < min.x - s.c.x) ? min.x - s.c.x : d.x;
    d.y = (d.y < min.y - s.c.y) ? min.y - s.c.y : d.y;
    d.z = (d.z < min.z - s.c.z) ? min.z - s.c.z : d.z;
    d.x = (d.x < s.c.x - max.x) ? s.c.x - max.x : d.x;
    d.y = (d.y < s.c.y - max.y) ? s.c.y - max.y : d.y;
    d.z = (d.z < s.c.z - max.z) ? s.c.z - max.z : d.z;

    if (d.x * d.x + d.y * d.y + d.z * d.z < s.r * s.r)
        return 1;
    return 0;
}

// VIVect3's compound operators again: `c = b.min; c += b.max; c *= 0.5f;`.
// The old field-by-field spelling lost d's zeroing stores and two of the
// `d = b.max` copies to forwarding; the operators' references keep them.
int VISphere::Init(const VIBBox &b)
{
    VIVect3 d;
    c = b.min;
    c += b.max;
    c *= 0.5f;
    d = b.max;
    d -= c;
    r = d.Magnitude();
    return 0;
}

int VISphere::PointInSolid(const VIVect3 &v) const
{
    VIVect3 d;

    d = v;
    d.x = d.x - c.x;
    d.y = d.y - c.y;
    d.z = d.z - c.z;

    if (!(d.x * d.x + d.y * d.y + d.z * d.z <= r * r))
        return 0;
    return 1;
}

int VIBBox::Transform(const VIMatrix44 &m, VIBBox &b) const
{
    VIVect3 p, q;

    p.x = min.x;  p.y = min.y;  p.z = min.z;
    m.Mul(p, q);
    b.Bound(1, q);

    p.x = max.x;  p.y = min.y;  p.z = min.z;
    m.Mul(p, q);
    b.Bound(0, q);

    p.x = max.x;  p.y = min.y;  p.z = max.z;
    m.Mul(p, q);
    b.Bound(0, q);

    p.x = min.x;  p.y = min.y;  p.z = max.z;
    m.Mul(p, q);
    b.Bound(0, q);

    p.x = min.x;  p.y = max.y;  p.z = min.z;
    m.Mul(p, q);
    b.Bound(0, q);

    p.x = max.x;  p.y = max.y;  p.z = min.z;
    m.Mul(p, q);
    b.Bound(0, q);

    p.x = max.x;  p.y = max.y;  p.z = max.z;
    m.Mul(p, q);
    b.Bound(0, q);

    p.x = min.x;  p.y = max.y;  p.z = max.z;
    m.Mul(p, q);
    b.Bound(0, q);

    return 0;
}

// The result is copied back into v FIELD BY FIELD, w included -- VIVect4 has no
// user operator=, so `v = v * m` would be a 16-byte block move where VIBRect's
// twin can write exactly that because VIVect3 does have one.  All four stores
// are dead; they are visible only in the LAST corner, where nothing overwrites
// v again and they survive past the epilogue's register restores.
int VIBBox::Transform(const VIMatrix44 &m, VIVect3 *p) const
{
    VIVect4 v;

    v.x = min.x;  v.y = min.y;  v.z = min.z;  v.w = 1.0f;
    {
        VIVect4 r = v * m;
        v.x = r.x;  v.y = r.y;  v.z = r.z;  v.w = r.w;
    }
    p[0].x = v.x;  p[0].y = v.y;  p[0].z = v.z;

    v.x = max.x;  v.y = min.y;  v.z = min.z;  v.w = 1.0f;
    {
        VIVect4 r = v * m;
        v.x = r.x;  v.y = r.y;  v.z = r.z;  v.w = r.w;
    }
    p[1].x = v.x;  p[1].y = v.y;  p[1].z = v.z;

    v.x = max.x;  v.y = min.y;  v.z = max.z;  v.w = 1.0f;
    {
        VIVect4 r = v * m;
        v.x = r.x;  v.y = r.y;  v.z = r.z;  v.w = r.w;
    }
    p[2].x = v.x;  p[2].y = v.y;  p[2].z = v.z;

    v.x = min.x;  v.y = min.y;  v.z = max.z;  v.w = 1.0f;
    {
        VIVect4 r = v * m;
        v.x = r.x;  v.y = r.y;  v.z = r.z;  v.w = r.w;
    }
    p[3].x = v.x;  p[3].y = v.y;  p[3].z = v.z;

    v.x = min.x;  v.y = max.y;  v.z = min.z;  v.w = 1.0f;
    {
        VIVect4 r = v * m;
        v.x = r.x;  v.y = r.y;  v.z = r.z;  v.w = r.w;
    }
    p[4].x = v.x;  p[4].y = v.y;  p[4].z = v.z;

    v.x = max.x;  v.y = max.y;  v.z = min.z;  v.w = 1.0f;
    {
        VIVect4 r = v * m;
        v.x = r.x;  v.y = r.y;  v.z = r.z;  v.w = r.w;
    }
    p[5].x = v.x;  p[5].y = v.y;  p[5].z = v.z;

    v.x = max.x;  v.y = max.y;  v.z = max.z;  v.w = 1.0f;
    {
        VIVect4 r = v * m;
        v.x = r.x;  v.y = r.y;  v.z = r.z;  v.w = r.w;
    }
    p[6].x = v.x;  p[6].y = v.y;  p[6].z = v.z;

    v.x = min.x;  v.y = max.y;  v.z = max.z;  v.w = 1.0f;
    {
        VIVect4 r = v * m;
        v.x = r.x;  v.y = r.y;  v.z = r.z;  v.w = r.w;
    }
    p[7].x = v.x;  p[7].y = v.y;  p[7].z = v.z;

    return 0;
}

// The circumscribing sphere of three points: the centroid, then the largest of
// the three squared distances, then one sqrtf.  0x3EAAAAAB is 1.0f/3.0f folded.
int VISphere::Init(const VIVect3 &a, const VIVect3 &b, const VIVect3 &v)
{
    VIVect3 d;
    float m, t, u;

    c = a;
    c.x = c.x + b.x;
    c.y = c.y + b.y;
    c.z = c.z + b.z;
    c.x = c.x + v.x;
    c.y = c.y + v.y;
    c.z = c.z + v.z;
    c.x = c.x * (1.0f / 3.0f);
    c.y = c.y * (1.0f / 3.0f);
    c.z = c.z * (1.0f / 3.0f);

    d.x = c.x - a.x;
    d.y = c.y - a.y;
    d.z = c.z - a.z;
    m = d.x * d.x + d.y * d.y + d.z * d.z;

    d.x = c.x - b.x;
    d.y = c.y - b.y;
    d.z = c.z - b.z;
    t = d.x * d.x + d.y * d.y + d.z * d.z;
    m = (m < t) ? t : m;

    // A FRESH local for the third squared distance: reusing `t` gives the right
    // instructions with the wrong FPRs (3 of 99), the same phenomenon as
    // VIMatrix33::Mul's per-term scalar.
    d.x = c.x - v.x;
    d.y = c.y - v.y;
    d.z = c.z - v.z;
    u = d.x * d.x + d.y * d.y + d.z * d.z;
    m = (m < u) ? u : m;

    r = sqrtf(m);
    return 0;
}
