// VIPlane / VIPlaneD -- SUPPORT 0x003F7F00 .. 0x003F87D8.  Matched as a PAIR:
// the double sibling is where a hoisted temporary stops being invisible.
#include "vi.h"

VIPlane &VIPlane::operator=(const VIPlaneD &p)
{
    n = p.n;
    d = (float)p.d;
    return *this;
}

VIVect3 VIPlane::Project(const VIVect3 &v) const
{
    VIVect3 r = n;
    float t = -(((v.x * n.x + v.y * n.y) + v.z * n.z) + d);
    r.x = r.x * t;
    r.y = r.y * t;
    r.z = r.z * t;
    r.x = r.x + v.x;
    r.y = r.y + v.y;
    r.z = r.z + v.z;
    return r;
}

int VIPlane::RayIntersection(const VIVect3 &o, const VIVect3 &dir, float &t, VIVect3 &out) const
{
    t = ((o.x * n.x + o.y * n.y) + o.z * n.z) + d;

    float den = (dir.x * n.x + dir.y * n.y) + dir.z * n.z;
    if (den != 0.0f)
    {
        t = -t / den;
        out = dir;
        float s = t;
        out.x = out.x * s;
        out.y = out.y * s;
        out.z = out.z * s;
        out.x = out.x + o.x;
        out.y = out.y + o.y;
        out.z = out.z + o.z;
        return 1;
    }

    t = 0.0f;
    out.x = 0.0f;
    out.y = 0.0f;
    out.z = 0.0f;
    return 0;
}

int VIPlane::LineIntersection(const VIVect3 &a, const VIVect3 &b, float &t, VIVect3 &out) const
{
    float da = ((a.x * n.x + a.y * n.y) + a.z * n.z) + d;
    float db = ((b.x * n.x + b.y * n.y) + b.z * n.z) + d;

    if (da != db)
    {
        t = da / (da - db);
        out = b;
        out.x = out.x - a.x;
        out.y = out.y - a.y;
        out.z = out.z - a.z;
        float s = t;
        out.x = out.x * s;
        out.y = out.y * s;
        out.z = out.z * s;
        out.x = out.x + a.x;
        out.y = out.y + a.y;
        out.z = out.z + a.z;
        return 1;
    }

    t = 0.0f;
    out.x = 0.0f;
    out.y = 0.0f;
    out.z = 0.0f;
    return 0;
}

int VIPlane::operator()(const VIVect3 &a, const VIVect3 &b, const VIVect3 &c)
{
    VIVect3 u, v, w;

    u.x = b.x - a.x;
    u.y = b.y - a.y;
    u.z = b.z - a.z;
    v.x = c.x - b.x;
    v.y = c.y - b.y;
    v.z = c.z - b.z;

    w.x = v.y * u.z - v.z * u.y;
    w.y = -v.x * u.z + v.z * u.x;
    w.z = v.x * u.y - v.y * u.x;

    n.x = w.x;
    n.y = w.y;
    n.z = w.z;

    if (n.Normalize())
    {
        d = -((n.x * a.x + n.y * a.y) + n.z * a.z);
        return 1;
    }

    d = 0.0f;
    return 0;
}

VIVect3D VIPlaneD::Project(const VIVect3D &v) const
{
    VIVect3D r = n;
    double t = -(((v.x * n.x + v.y * n.y) + v.z * n.z) + d);
    r.x = r.x * t;
    r.y = r.y * t;
    r.z = r.z * t;
    r.x = r.x + v.x;
    r.y = r.y + v.y;
    r.z = r.z + v.z;
    return r;
}

int VIPlaneD::RayIntersection(const VIVect3D &o, const VIVect3D &dir, double &t, VIVect3D &out) const
{
    t = ((o.x * n.x + o.y * n.y) + o.z * n.z) + d;

    double den = (dir.x * n.x + dir.y * n.y) + dir.z * n.z;
    if (den != 0.0)
    {
        t = -t / den;
        out = dir;
        double s = t;
        out.x = out.x * s;
        out.y = out.y * s;
        out.z = out.z * s;
        out.x = out.x + o.x;
        out.y = out.y + o.y;
        out.z = out.z + o.z;
        return 1;
    }

    t = 0.0;
    out.x = 0.0;
    out.y = 0.0;
    out.z = 0.0;
    return 0;
}

int VIPlaneD::operator()(const VIVect3D &a, const VIVect3D &b, const VIVect3D &c)
{
    VIVect3D u = b;
    VIVect3D v;
    double w[3];

    u.x = u.x - a.x;
    u.y = u.y - a.y;
    u.z = u.z - a.z;
    v = c;
    v.x = v.x - b.x;
    v.y = v.y - b.y;
    v.z = v.z - b.z;

    w[0] = v.y * u.z - v.z * u.y;
    w[1] = -v.x * u.z + v.z * u.x;
    w[2] = v.x * u.y - v.y * u.x;

    n.x = w[0];
    n.y = w[1];
    n.z = w[2];

    if (n.Normalize())
    {
        d = -((n.x * a.x + n.y * a.y) + n.z * a.z);
        return 1;
    }

    d = 0.0;
    return 0;
}
