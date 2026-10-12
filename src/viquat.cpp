// VIQuat -- SUPPORT 0x003F87D8 .. 0x003F9060, the third matched translation
// unit.  Semantics are not in question: go-eqoa-vi/vimath's Quat is already
// bit-exact against the console for every one of these, so a mismatch here is
// codegen and nothing else.
#include "vi.h"

// The hidden-return-pointer form: a0 is the result slot, a1 is `this`, a2 is
// the argument.  Same product as Mul(a, b) with a = *this, b = the argument --
// the cross terms carry the sign of b x a, not a x b.
VIQuat VIQuat::operator*(const VIQuat &b) const
{
    return VIQuat(((b.y * z - b.z * y) + b.w * x) + b.x * w,
                  ((b.z * x - b.x * z) + b.w * y) + b.y * w,
                  ((b.x * y - b.y * x) + b.w * z) + b.z * w,
                  ((b.w * w - b.x * x) - b.y * y) - b.z * z);
}

void VIQuat::RotateX(float a)
{
    x = sinf(a * 0.5f);
    y = 0.0f;
    z = 0.0f;
    w = cosf(a * 0.5f);
}

void VIQuat::RotateY(float a)
{
    x = 0.0f;
    y = sinf(a * 0.5f);
    z = 0.0f;
    w = cosf(a * 0.5f);
}

void VIQuat::RotateZ(float a)
{
    x = 0.0f;
    y = 0.0f;
    z = sinf(a * 0.5f);
    w = cosf(a * 0.5f);
}

// InvertUnit and Conjugate are byte-identical out-of-line bodies -- neither
// calls the other.
void VIQuat::InvertUnit(const VIQuat &a)
{
    x = -a.x;
    y = -a.y;
    z = -a.z;
    w = a.w;
}

void VIQuat::Invert(const VIQuat &a)
{
    x = -a.x;
    y = -a.y;
    z = -a.z;
    w = a.w;
    Normalize();
}

void VIQuat::Conjugate(const VIQuat &a)
{
    x = -a.x;
    y = -a.y;
    z = -a.z;
    w = a.w;
}

int VIQuat::Normalize()
{
    float d = ((x * x + y * y) + z * z) + w * w;
    if (d != 0.0f)
    {
        float r = 1.0f / sqrtf(d);
        x = x * r;
        y = y * r;
        z = z * r;
        w = w * r;
        return 1;
    }
    return 0;
}

// Each component is stored the moment it is computed -- `this` may alias `a`
// or `b`, so nothing is CSE'd across the four statements and every element of
// both sources is reloaded per component.
void VIQuat::Mul(const VIQuat &a, const VIQuat &b)
{
    x = ((b.y * a.z - b.z * a.y) + b.w * a.x) + b.x * a.w;
    y = ((b.z * a.x - b.x * a.z) + b.w * a.y) + b.y * a.w;
    z = ((b.x * a.y - b.y * a.x) + b.w * a.z) + b.z * a.w;
    w = ((b.w * a.w - b.x * a.x) - b.y * a.y) - b.z * a.z;
}

// Four VIQuat locals -- their sixteen `sw zero` are one run of default ctors at
// the top of the block -- and the classic sandwich q^-1 * (v,0) * q.  The pure
// vector's w is written 0.0f a SECOND time after x/y/z, in the delay slot of
// the first Mul.
void VIQuat::UnitRotate(const VIVect3 &v, VIVect3 &out) const
{
    VIQuat inv, t, r, p;

    inv.InvertUnit(*this);
    p.x = v.x;
    p.y = v.y;
    p.z = v.z;
    p.w = 0.0f;
    t.Mul(inv, p);
    r.Mul(t, *this);
    out.x = r.x;
    out.y = r.y;
    out.z = r.z;
}

// Euler (heading, pitch, roll) -> quaternion.  Each angle is halved AT THE CALL
// -- `v.x` is reloaded before both of its two calls, so there is no hoisted
// half-angle local.  The two negative terms are `(-cx)*sy` and `(-sx)*sy`,
// formed with neg.s on the leading factor: gcc keeps them distinct from the
// CSE'd `cx*sy` / `sx*sy` because it may not fold a negation through a product.
void VIQuat::ConvertFrom(const VIVect3 &v)
{
    float sx = sinf(v.x * 0.5f);
    float cx = cosf(v.x * 0.5f);
    float sy = sinf(v.y * 0.5f);
    float cy = cosf(v.y * 0.5f);
    float sz = sinf(v.z * 0.5f);
    float cz = cosf(v.z * 0.5f);

    x = (sx * cy) * sz + (cx * sy) * cz;
    y = (-cx * sy) * sz + (sx * cy) * cz;
    z = (-sx * sy) * cz + (cx * cy) * sz;
    w = (cx * cy) * cz + (sx * sy) * sz;
}

// Slerp returns VOID -- nothing writes v0 on any path.  The negative-dot half
// is a full textual duplicate, not `negate b and recurse`: it negates the DOT,
// takes the same two branches, and negates only the weight applied to b, so the
// endpoint quaternion is never itself flipped.
void VIQuat::Slerp(const VIQuat &a, const VIQuat &b, float t)
{
    float wa, wb;
    float dot = ((a.x * b.x + a.y * b.y) + a.z * b.z) + a.w * b.w;

    if (0.0f < dot)
    {
        if (1.0f - dot > 1.0e-4f)
        {
            float theta = acosf(dot);
            float inv = 1.0f / sinf(theta);
            wa = sinf((1.0f - t) * theta) * inv;
            wb = sinf(t * theta) * inv;
        }
        else
        {
            wa = 1.0f - t;
            wb = t;
        }
    }
    else
    {
        dot = -dot;
        if (1.0f - dot > 1.0e-4f)
        {
            float theta = acosf(dot);
            float inv = 1.0f / sinf(theta);
            wa = sinf((1.0f - t) * theta) * inv;
            wb = -sinf(t * theta) * inv;
        }
        else
        {
            wa = 1.0f - t;
            wb = -t;
        }
    }

    x = a.x * wa + b.x * wb;
    y = a.y * wa + b.y * wb;
    z = a.z * wa + b.z * wb;
    w = a.w * wa + b.w * wb;
}

// The quaternion's own euler conversion goes the long way round: a stack
// VIMatrix44 (its VIVect4 array ctor is the 4x4-word zero loop), Zero() inlined
// as VU0, then UnitRotate and ConvertToEulerXYZ.  The trailing call with
// a1 = 2 is the GNU 2.x destructor ABI's __in_chrg argument.
void VIQuat::ConvertTo(VIVect3 &a) const
{
    VIMatrix44 m;
    m.UnitRotate(*this);
    m.ConvertToEulerXYZ(a);
}
