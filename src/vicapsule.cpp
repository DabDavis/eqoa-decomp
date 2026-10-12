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

// The cylinder: inside the tube AND between the two end planes, both tests
// inclusive, so a point exactly on an end plane is inside.
int VICylinder::PointInSolid(const VIVect3 &p)
{
    // Phase 9's lever: the direction is named through a POINTER, so its .y and
    // .z come off that register and its .x folds back onto the object base.
    VIVect3 *dir = &m_dir;
    VIVect3  zero;
    VIVect3  d;
    float    t;
    float    s;

    d.x = p.x - m_base.x;
    d.y = p.y - m_base.y;
    d.z = p.z - m_base.z;
    t = d.x * dir->x + d.y * dir->y + d.z * dir->z;
    d.x = d.x - dir->x * t;
    d.y = d.y - dir->y * t;
    d.z = d.z - dir->z * t;
    if (d.x * d.x + d.y * d.y + d.z * d.z <= m_radius * m_radius)
    {
        s = dir->x * p.x + dir->y * p.y + dir->z * p.z
          - (m_base.x * dir->x + m_base.y * dir->y + m_base.z * dir->z);
        if (zero.x <= s && s <= m_length)
            return 1;
    }
    return 0;
}

// The capsule: the cylinder test, then a sphere at each end.  The perpendicular
// gate covers the caps too -- a point inside an end sphere is within `radius`
// of a centre ON the axis, so its distance to the axis cannot exceed the
// radius -- which is why the two sphere tests can be plain distance checks.
int VICapsule::PointInSolid(const VIVect3 &p)
{
    VIVect3 *dir = &m_dir;
    VIVect3  zero;
    VIVect3  d;
    float    t;
    float    s;
    float    r2;
    int      r = 0;

    d.x = p.x - m_base.x;
    d.y = p.y - m_base.y;
    d.z = p.z - m_base.z;
    t = d.x * dir->x + d.y * dir->y + d.z * dir->z;
    d.x = d.x - dir->x * t;
    d.y = d.y - dir->y * t;
    d.z = d.z - dir->z * t;
    if (d.x * d.x + d.y * d.y + d.z * d.z <= m_radius * m_radius)
    {
        s = dir->x * p.x + dir->y * p.y + dir->z * p.z
          - (m_base.x * dir->x + m_base.y * dir->y + m_base.z * dir->z);
        if (zero.x <= s && s <= m_length)
            return 1;

        r2 = m_radius * m_radius;
        d.x = p.x - m_base.x;
        d.y = p.y - m_base.y;
        d.z = p.z - m_base.z;
        if (d.x * d.x + d.y * d.y + d.z * d.z <= r2)
        {
            r = 1;
        }
        else
        {
            d.x = m_base.x + dir->x * m_length;
            d.y = m_base.y + dir->y * m_length;
            d.z = m_base.z + dir->z * m_length;
            d.x = d.x - p.x;
            d.y = d.y - p.y;
            d.z = d.z - p.z;
            if (d.x * d.x + d.y * d.y + d.z * d.z <= r2)
                r = 1;
        }
    }
    return r;
}
