// VIQuatD -- SUPPORT 0x003F9060 .. 0x003F9EB0.  The same thirteen functions as
// VIQuat, one register class down: every arithmetic operator is a libgcc call
// (__muldf3 / __adddf3 / __subdf3) and `sqrt` is a call too, so there is no
// inline sqrt.s and no NaN retest anywhere in this file.
#include "vi.h"

VIQuatD VIQuatD::operator*(const VIQuatD &b) const
{
    return VIQuatD(((b.y * z - b.z * y) + b.w * x) + b.x * w,
                   ((b.z * x - b.x * z) + b.w * y) + b.y * w,
                   ((b.x * y - b.y * x) + b.w * z) + b.z * w,
                   ((b.w * w - b.x * x) - b.y * y) - b.z * z);
}

void VIQuatD::RotateX(double a)
{
    x = sin(a * 0.5);
    y = 0.0;
    z = 0.0;
    w = cos(a * 0.5);
}

void VIQuatD::RotateY(double a)
{
    x = 0.0;
    y = sin(a * 0.5);
    z = 0.0;
    w = cos(a * 0.5);
}

void VIQuatD::RotateZ(double a)
{
    x = 0.0;
    y = 0.0;
    z = sin(a * 0.5);
    w = cos(a * 0.5);
}

void VIQuatD::InvertUnit(const VIQuatD &a)
{
    x = -a.x;
    y = -a.y;
    z = -a.z;
    w = a.w;
}

void VIQuatD::Invert(const VIQuatD &a)
{
    x = -a.x;
    y = -a.y;
    z = -a.z;
    w = a.w;
    Normalize();
}

void VIQuatD::Conjugate(const VIQuatD &a)
{
    x = -a.x;
    y = -a.y;
    z = -a.z;
    w = a.w;
}

int VIQuatD::Normalize()
{
    double d = ((x * x + y * y) + z * z) + w * w;
    if (d != 0.0)
    {
        double r = 1.0 / sqrt(d);
        x = x * r;
        y = y * r;
        z = z * r;
        w = w * r;
        return 1;
    }
    return 0;
}

void VIQuatD::Mul(const VIQuatD &a, const VIQuatD &b)
{
    x = ((b.y * a.z - b.z * a.y) + b.w * a.x) + b.x * a.w;
    y = ((b.z * a.x - b.x * a.z) + b.w * a.y) + b.y * a.w;
    z = ((b.x * a.y - b.y * a.x) + b.w * a.z) + b.z * a.w;
    w = ((b.w * a.w - b.x * a.x) - b.y * a.y) - b.z * a.z;
}

void VIQuatD::UnitRotate(const VIVect3D &v, VIVect3D &out) const
{
    VIQuatD inv, t, r, p;

    inv.InvertUnit(*this);
    p.x = v.x;
    p.y = v.y;
    p.z = v.z;
    p.w = 0.0;
    t.Mul(inv, p);
    r.Mul(t, *this);
    out.x = r.x;
    out.y = r.y;
    out.z = r.z;
}

void VIQuatD::ConvertFrom(const VIVect3D &v)
{
    double sx = sin(v.x * 0.5);
    double cx = cos(v.x * 0.5);
    double sy = sin(v.y * 0.5);
    double cy = cos(v.y * 0.5);
    double sz = sin(v.z * 0.5);
    double cz = cos(v.z * 0.5);

    x = (sx * cy) * sz + (cx * sy) * cz;
    y = (-cx * sy) * sz + (sx * cy) * cz;
    z = (-sx * sy) * cz + (cx * cy) * sz;
    w = (cx * cy) * cz + (sx * sy) * sz;
}

void VIQuatD::Slerp(const VIQuatD &a, const VIQuatD &b, double t)
{
    double wa, wb;
    double dot = ((a.x * b.x + a.y * b.y) + a.z * b.z) + a.w * b.w;

    if (0.0 < dot)
    {
        if (1.0 - dot > 1.0e-4)
        {
            double theta = acos(dot);
            double inv = 1.0 / sin(theta);
            wa = sin((1.0 - t) * theta) * inv;
            wb = sin(t * theta) * inv;
        }
        else
        {
            wa = 1.0 - t;
            wb = t;
        }
    }
    else
    {
        dot = -dot;
        if (1.0 - dot > 1.0e-4)
        {
            double theta = acos(dot);
            double inv = 1.0 / sin(theta);
            wa = sin((1.0 - t) * theta) * inv;
            wb = -sin(t * theta) * inv;
        }
        else
        {
            wa = 1.0 - t;
            wb = -t;
        }
    }

    x = a.x * wa + b.x * wb;
    y = a.y * wa + b.y * wb;
    z = a.z * wa + b.z * wb;
    w = a.w * wa + b.w * wb;
}

void VIQuatD::ConvertTo(VIVect3D &a) const
{
    // No Zero() here, unlike the float sibling: VIMatrix44D has a real
    // constructor, and one destructor call closes the block.
    VIMatrix44D m;
    m.UnitRotate(*this);
    m.ConvertToEulerXYZ(a);
}
