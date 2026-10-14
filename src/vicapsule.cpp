// VICapsule / VICylinder -- SUPPORT 0x003EFE60 .. 0x003F0178, one TU between
// VIBRect and VIMatrix44.
//
// Two distinct types with a byte-identical layout, and the front half of the
// two bodies is instruction for instruction the same: reject the point unless
// it is within `radius` of the INFINITE axis, then decide the ends.  The axial
// coordinate is computed TWICE by different routes and the two are not
// interchangeable -- the perpendicular rejection uses t = dot(p - base, dir),
// and the cap test recomputes it as dot(dir, p) - dot(base, dir), a different
// rounding of the same quantity.  go-eqoa-vi/vicoll measured that against the
// console; folding them together does not agree with it.
#include "vi.h"

// The vector helpers, inline and BY REFERENCE, and the bodies hand them the
// MEMBERS (claude-notes/decomp-alias-rule-inline-ref-params-20261014.md): the
// inlined `m_dir` is a fresh pseudo with no pointer flag, so a load of dir.y or
// dir.z through it has no known base and every store to the stack `d` kills
// it -- the guest re-reads dir.y/dir.z after each one, while dir.x (folded to
// 12(a0)), m_base (off `this`) and p (the incoming, flagged argument register)
// stay in registers.  A named `VIVect3 *dir` folds all of it: 66/72 and 116/125.
static inline float Dot(const VIVect3 &a, const VIVect3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline void SubScaled(VIVect3 &a, const VIVect3 &b, float k)
{
    a.x = a.x - b.x * k;
    a.y = a.y - b.y * k;
    a.z = a.z - b.z * k;
}

static inline void AddScaled(VIVect3 &a, const VIVect3 &b, float k)
{
    a.x = a.x + b.x * k;
    a.y = a.y + b.y * k;
    a.z = a.z + b.z * k;
}

// The cylinder: inside the tube AND between the two end planes, both tests
// inclusive, so a point exactly on an end plane is inside.
int VICylinder::PointInSolid(const VIVect3 &p)
{
    VIVect3  zero;
    VIVect3  d;
    float    t;
    float    s;
    int      r = 0;

    d = p;
    d -= m_base;
    t = Dot(d, m_dir);
    SubScaled(d, m_dir, t);
    if (Dot(d, d) <= m_radius * m_radius)
    {
        s = Dot(m_dir, p) - Dot(m_base, m_dir);
        if (zero.x <= s && s <= m_length)
            r = 1;
    }
    return r;
}

// The capsule: the cylinder test, then a sphere at each end.  The perpendicular
// gate covers the caps too -- a point inside an end sphere is within `radius`
// of a centre ON the axis, so its distance to the axis cannot exceed the
// radius -- which is why the two sphere tests can be plain distance checks.
int VICapsule::PointInSolid(const VIVect3 &p)
{
    VIVect3  zero;
    VIVect3  d;
    float    t;
    float    s;
    float    r2;
    int      r = 0;

    d = p;
    d -= m_base;
    t = Dot(d, m_dir);
    SubScaled(d, m_dir, t);
    if (Dot(d, d) <= m_radius * m_radius)
    {
        s = Dot(m_dir, p) - Dot(m_base, m_dir);
        if (zero.x <= s && s <= m_length)
            r = 1;
        else
        {
            r2 = m_radius * m_radius;
            d = p;
            d -= m_base;
            if (Dot(d, d) <= r2)
                r = 1;
            else
            {
                // `d = m_base` and then the scaled add: the copy-in stores of
                // d.y and d.z SURVIVE, because the dir.y/dir.z loads between
                // them and the final stores may alias d (flow cannot call them
                // dead); d.x's has nothing in between and is deleted.  And
                // `d = p; d -= m_base` rather than three subtractions is what
                // orders the x component first, here and in the head.
                d = m_base;
                AddScaled(d, m_dir, m_length);
                d -= p;
                if (Dot(d, d) <= r2)
                    r = 1;
            }
        }
    }
    return r;
}
