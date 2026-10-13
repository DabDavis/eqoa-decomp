// VIVect2 / VIVect3 / VIVect4 -- SUPPORT 0x003FA678 .. 0x003FB878, one TU that
// also holds the double-precision VIVect3D / VIVect4D siblings.
//
// The square root everywhere here is gcc's builtin expansion of `sqrtf`: the
// EE's `sqrt.s` inline, a `c.eq.s $f0,$f0` NaN test on the RESULT, and a call
// to the library only when that fails (which is where errno would be set).
// Nothing about it is hand-written, and it is the reason the engine's two
// square roots round differently -- see go-eqoa-vi/vimath/acos.go.
#include "vi.h"

float VIVect2::Magnitude() const
{
    return sqrtf(x * x + y * y);
}

int VIVect2::Normalize()
{
    float d = x * x + y * y;
    if (d != 0.0f)
    {
        float r = 1.0f / sqrtf(d);
        x = x * r;
        y = y * r;
        return 1;
    }
    return 0;
}

float VIVect2::NormalizeMagn()
{
    float d = x * x + y * y;
    if (d != 0.0f)
    {
        d = sqrtf(d);
        float r = 1.0f / d;
        x = x * r;
        y = y * r;
    }
    return d;
}

// NormalizeMagn has ONE exit and returns the local `d`, reassigned from the
// length: the zero case falls through to the shared epilogue with d still 0.0
// (`mov.s $f0,$f12`), so it is one variable and not `return 0.0f;` early.
// Distance and DistanceSquared build a real VIVect2 difference on the stack --
// the two `swc1` into the frame are dead stores gcc keeps because the local is
// a struct, and they are how we know it is not written as two scalars.
float VIVect2::Distance(const VIVect2 &v) const
{
    VIVect2 d;
    d.x = x - v.x;
    d.y = y - v.y;
    return sqrtf(d.x * d.x + d.y * d.y);
}

float VIVect2::DistanceSquared(const VIVect2 &v) const
{
    VIVect2 d;
    d.x = x - v.x;
    d.y = y - v.y;
    return d.x * d.x + d.y * d.y;
}

// Each component goes through the software double->float conversion, so this is
// three calls, not a block move.
VIVect3 &VIVect3::operator=(const VIVect3D &v)
{
    x = (float)v.x;
    y = (float)v.y;
    z = (float)v.z;
    return *this;
}

float VIVect3::Magnitude() const
{
    return sqrtf((x * x + y * y) + z * z);
}

int VIVect3::Normalize()
{
    float d = (x * x + y * y) + z * z;
    if (d != 0.0f)
    {
        float r = 1.0f / sqrtf(d);
        x = x * r;
        y = y * r;
        z = z * r;
        return 1;
    }
    return 0;
}

float VIVect3::NormalizeMagn()
{
    float d = (x * x + y * y) + z * z;
    if (d != 0.0f)
    {
        d = sqrtf(d);
        float r = 1.0f / d;
        x = x * r;
        y = y * r;
        z = z * r;
    }
    return d;
}

float VIVect3::Distance(const VIVect3 &v) const
{
    VIVect3 d = *this;
    d -= v;
    return sqrtf((d.x * d.x + d.y * d.y) + d.z * d.z);
}

// cosf(p) is called TWICE -- gcc cannot CSE across a call it does not know is
// pure -- and the roll argument is never read.
void VIVect3::HPR(float h, float p, float r)
{
    x = sinf(h) * cosf(p);
    y = -sinf(p);
    z = cosf(h) * cosf(p);
}

// The reader.  Roll is STORED as zero, not computed: a direction vector does
// not carry one, so HPR and this do not round-trip through the third angle.
void VIVect3::HPR(float *h, float *p, float *r) const
{
    *h = atan2f(x, z);
    *p = -atan2f(y, sqrtf(x * x + z * z));
    *r = 0.0f;
}

VIVect4 &VIVect4::operator=(const VIVect4D &v)
{
    x = (float)v.x;
    y = (float)v.y;
    z = (float)v.z;
    w = (float)v.w;
    return *this;
}

float VIVect4::Magnitude() const
{
    return sqrtf(((x * x + y * y) + z * z) + w * w);
}

// The only VU0 in the group: the scale-and-store is a broadcast multiply, so
// the reciprocal crosses from an FPR to a VU integer register through $t0.
int VIVect4::Normalize()
{
    float d = ((x * x + y * y) + z * z) + w * w;
    if (d != 0.0f)
    {
        float r = 1.0f / sqrtf(d);
        __asm__ __volatile__(
            "lqc2 $vf4, 0x0(%0)\n"
            "mfc1 $t0, %1\n"
            "qmtc2 $t0, $vf5\n"
            "vmulx.xyzw $vf6, $vf4, $vf5x\n"
            "sqc2 $vf6, 0x0(%0)\n"
            : : "r"(this), "f"(r) : "$8");
        return 1;
    }
    return 0;
}

float VIVect4::NormalizeMagn()
{
    float d = ((x * x + y * y) + z * z) + w * w;
    if (d != 0.0f)
    {
        d = sqrtf(d);
        float r = 1.0f / d;
        x = x * r;
        y = y * r;
        z = z * r;
        w = w * r;
    }
    return d;
}

// The compound subtract VIVect4 / VIVect3D / VIVect4D do not declare, in
// VIVect3::operator-='s shape: a REFERENCE to the destination.
static inline void V4Sub(VIVect4 &a, const VIVect4 &b)
{
    a.x = a.x - b.x;
    a.y = a.y - b.y;
    a.z = a.z - b.z;
    a.w = a.w - b.w;
}

float VIVect4::Distance(const VIVect4 &v) const
{
    VIVect4 d;
    d.Set(*this);
    V4Sub(d, v);
    return sqrtf(((d.x * d.x + d.y * d.y) + d.z * d.z) + d.w * d.w);
}

float VIVect4::DistanceSquared(const VIVect4 &v) const
{
    VIVect4 d;
    d.Set(*this);
    V4Sub(d, v);
    return ((d.x * d.x + d.y * d.y) + d.z * d.z) + d.w * d.w;
}

// ---- the double-precision siblings -------------------------------------
// Same source shapes, one register class down: `sqrt` is a libgcc CALL here,
// not the EE's inline `sqrt.s`, so there is no NaN-retest branch at all.


static inline void D3SubV(VIVect3D &a, const VIVect3D &b)
{
    a.x = a.x - b.x;
    a.y = a.y - b.y;
    a.z = a.z - b.z;
}

static inline double D3DotV(const VIVect3D &a, const VIVect3D &b)
{
    return (a.x * b.x + a.y * b.y) + a.z * b.z;
}

static inline double D4DotV(const VIVect4D &a, const VIVect4D &b)
{
    return ((a.x * b.x + a.y * b.y) + a.z * b.z) + a.w * b.w;
}

static inline void D4SubV(VIVect4D &a, const VIVect4D &b)
{
    a.x = a.x - b.x;
    a.y = a.y - b.y;
    a.z = a.z - b.z;
    a.w = a.w - b.w;
}

double VIVect3D::Magnitude() const
{
    return sqrt((x * x + y * y) + z * z);
}

int VIVect3D::Normalize()
{
    double d = (x * x + y * y) + z * z;
    if (d != 0.0)
    {
        double r = 1.0 / sqrt(d);
        x = x * r;
        y = y * r;
        z = z * r;
        return 1;
    }
    return 0;
}

double VIVect3D::NormalizeMagn()
{
    double d = (x * x + y * y) + z * z;
    if (d != 0.0)
    {
        d = sqrt(d);
        double r = 1.0 / d;
        x = x * r;
        y = y * r;
        z = z * r;
    }
    return d;
}

// COPY THEN SUBTRACT IN PLACE, through REFERENCES: `d = *this`, then a
// compound subtract and a dot product that take `d` by reference (D3SubV /
// D3DotV above -- VIVect3D declares no operators of its own).  Phase 8 read
// the residue of the field-wise spelling as a compiler difference (cse
// forwarding the stores past the soft-double libcalls); phase 61 showed it is
// the source: through the references d's slots are re-read after each call,
// exactly as the target does, and all six float/double Distance functions are
// byte-exact this way.
double VIVect3D::Distance(const VIVect3D &v) const
{
    VIVect3D d = *this;
    D3SubV(d, v);
    return sqrt(D3DotV(d, d));
}

// Copy then subtract in place -- see VIVect3D::Distance above.
double VIVect3D::DistanceSquared(const VIVect3D &v) const
{
    VIVect3D d = *this;
    D3SubV(d, v);
    return D3DotV(d, d);
}

void VIVect3D::HPR(double h, double p, double r)
{
    x = sin(h) * cos(p);
    y = -sin(p);
    z = cos(h) * cos(p);
}

void VIVect3D::HPR(double *h, double *p, double *r) const
{
    *h = atan2(x, z);
    *p = -atan2(y, sqrt(x * x + z * z));
    *r = 0.0;
}

double VIVect4D::Magnitude() const
{
    return sqrt(((x * x + y * y) + z * z) + w * w);
}

int VIVect4D::Normalize()
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

double VIVect4D::NormalizeMagn()
{
    double d = ((x * x + y * y) + z * z) + w * w;
    if (d != 0.0)
    {
        d = sqrt(d);
        double r = 1.0 / d;
        x = x * r;
        y = y * r;
        z = z * r;
        w = w * r;
    }
    return d;
}

// The locals here are NOT `VIVect3D`/`VIVect4D`.  That class has a zeroing default
// constructor -- VIMatrix44D::SwapRows' temp proves it, +8/+16/+24 stored zero
// with the +0 store dead -- and gcc does not eliminate those stores even when
// every member is overwritten immediately, so a `VIVect4D d;` opens these two
// with four `sd zero` the target does not have.
// Copy then subtract in place -- see VIVect3D::Distance above.
double VIVect4D::Distance(const VIVect4D &v) const
{
    VIVect4D d = *this;
    D4SubV(d, v);
    return sqrt(D4DotV(d, d));
}

// Copy then subtract in place -- see VIVect3D::Distance above.
double VIVect4D::DistanceSquared(const VIVect4D &v) const
{
    VIVect4D d = *this;
    D4SubV(d, v);
    return D4DotV(d, d);
}
