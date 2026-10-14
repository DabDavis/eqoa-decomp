// VIMatrix44D -- SUPPORT 0x003F3300 .. 0x003F7AB8.  The double sibling of
// VIMatrix44, and the more informative of the two: the vector unit is
// single-precision only, so every routine the float class hides behind a VU0
// macro-mode block -- Mul, both Inverts, BlendAccum, the vector products -- is
// ordinary C++ here.
#include "vi.h"

// The zero loop at the top is the implicit construction of `VIVect4D m[4]`
// (counter 3 down to -1, stride 32), so the body is the single call after it.
// Which call it is cannot be read off the image -- the target is relocated to
// zero -- but Identity() is what VIMatrix33's constructor does, and Zero()
// would be redundant with the member construction that just ran.
VIMatrix44D::VIMatrix44D()
{
    Zero();
}

// Same three-part shape as VIMatrix33: a user operator= assigning the rows one
// at a time (the synthesized one is a LOOP), and a copy constructor that spells
// the same assignments rather than calling it.
VIMatrix44D::~VIMatrix44D()
{
}

VIMatrix44D &VIMatrix44D::operator=(const VIMatrix44D &a)
{
    m[0] = a.m[0];
    m[1] = a.m[1];
    m[2] = a.m[2];
    m[3] = a.m[3];
    return *this;
}

VIMatrix44D::VIMatrix44D(const VIMatrix44D &a)
{
    m[0] = a.m[0];
    m[1] = a.m[1];
    m[2] = a.m[2];
    m[3] = a.m[3];
}

VIMatrix44D &VIMatrix44D::operator=(const VIMatrix44 &a)
{
    m[0] = a.m[0];
    m[1] = a.m[1];
    m[2] = a.m[2];
    m[3] = a.m[3];
    return *this;
}

// The float direction: four calls to VIVect4::operator=(const VIVect4D &),
// unrolled in the source, stride 16 on the destination and 32 on the source.
VIMatrix44 &VIMatrix44::operator=(const VIMatrix44D &a)
{
    m[0] = a.m[0];
    m[1] = a.m[1];
    m[2] = a.m[2];
    m[3] = a.m[3];
    return *this;
}

void VIMatrix44D::Zero()
{
    double *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x, *d3 = &m[3].x;
    d0[0] = 0.0; d0[1] = 0.0; d0[2] = 0.0; d0[3] = 0.0;
    d1[0] = 0.0; d1[1] = 0.0; d1[2] = 0.0; d1[3] = 0.0;
    d2[0] = 0.0; d2[1] = 0.0; d2[2] = 0.0; d2[3] = 0.0;
    d3[0] = 0.0; d3[1] = 0.0; d3[2] = 0.0; d3[3] = 0.0;
}

void VIMatrix44D::Identity()
{
    Zero();
    m[0].x = 1.0;
    m[1].y = 1.0;
    m[2].z = 1.0;
    m[3].w = 1.0;
}

void VIMatrix44D::Scale(double s)
{
    Identity();
    m[0].x = s;
    m[1].y = s;
    m[2].z = s;
}

void VIMatrix44D::Scale(const VIVect3D &v)
{
    Identity();
    m[0].x = v.x;
    m[1].y = v.y;
    m[2].z = v.z;
}

void VIMatrix44D::Translate(const VIVect3D &v)
{
    Identity();
    m[3].x = v.x;
    m[3].y = v.y;
    m[3].z = v.z;
}

void VIMatrix44D::Store(double *p) const
{
    for (int i = 0; i < 4; i++)
    {
        *p++ = ((const double (*)[4])m)[i][0];
        *p++ = ((const double (*)[4])m)[i][1];
        *p++ = ((const double (*)[4])m)[i][2];
        *p++ = ((const double (*)[4])m)[i][3];
    }
}

void VIMatrix44D::Transpose(const VIMatrix44D &a)
{
    double (*d)[4] = (double (*)[4])m;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            d[j][i] = ((const double (*)[4])a.m)[i][j];
}

void VIMatrix44D::Transpose()
{
    double (*d)[4] = (double (*)[4])m;
    for (int i = 0; i < 4; i++)
        for (int j = i; j < 4; j++)
        {
            double t = d[i][j];
            d[i][j] = d[j][i];
            d[j][i] = t;
        }
}

void VIMatrix44D::SwapRows(int i, int j)
{
    VIVect4D t;

    t = m[i];
    m[i] = m[j];
    m[j] = t;
}

void VIMatrix44D::RotateX(double a)
{
    double s = sin(a);
    double c = cos(a);
    Identity();
    m[1].y = c;
    m[1].z = s;
    m[2].y = -s;
    m[2].z = c;
}

void VIMatrix44D::RotateY(double a)
{
    double s = sin(a);
    double c = cos(a);
    Identity();
    m[0].x = c;
    m[0].z = -s;
    m[2].x = s;
    m[2].z = c;
}

void VIMatrix44D::RotateZ(double a)
{
    double s = sin(a);
    double c = cos(a);
    Identity();
    m[0].x = c;
    m[0].y = s;
    m[1].x = -s;
    m[1].y = c;
}

void VIMatrix44D::Add(const VIMatrix44D &a, double f)
{
    double *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x, *d3 = &m[3].x;
    const double *s0 = &a.m[0].x, *s1 = &a.m[1].x, *s2 = &a.m[2].x, *s3 = &a.m[3].x;
    d0[0] = s0[0] + f;
    d0[1] = s0[1] + f;
    d0[2] = s0[2] + f;
    d0[3] = s0[3] + f;
    d1[0] = s1[0] + f;
    d1[1] = s1[1] + f;
    d1[2] = s1[2] + f;
    d1[3] = s1[3] + f;
    d2[0] = s2[0] + f;
    d2[1] = s2[1] + f;
    d2[2] = s2[2] + f;
    d2[3] = s2[3] + f;
    d3[0] = s3[0] + f;
    d3[1] = s3[1] + f;
    d3[2] = s3[2] + f;
    d3[3] = s3[3] + f;
}

void VIMatrix44D::Add(const VIMatrix44D &a, const VIMatrix44D &b)
{
    double *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x, *d3 = &m[3].x;
    const double *s0 = &a.m[0].x, *s1 = &a.m[1].x, *s2 = &a.m[2].x, *s3 = &a.m[3].x;
    const double *t0 = &b.m[0].x, *t1 = &b.m[1].x, *t2 = &b.m[2].x, *t3 = &b.m[3].x;
    d0[0] = s0[0] + t0[0];
    d0[1] = s0[1] + t0[1];
    d0[2] = s0[2] + t0[2];
    d0[3] = s0[3] + t0[3];
    d1[0] = s1[0] + t1[0];
    d1[1] = s1[1] + t1[1];
    d1[2] = s1[2] + t1[2];
    d1[3] = s1[3] + t1[3];
    d2[0] = s2[0] + t2[0];
    d2[1] = s2[1] + t2[1];
    d2[2] = s2[2] + t2[2];
    d2[3] = s2[3] + t2[3];
    d3[0] = s3[0] + t3[0];
    d3[1] = s3[1] + t3[1];
    d3[2] = s3[2] + t3[2];
    d3[3] = s3[3] + t3[3];
}

void VIMatrix44D::Mul(const VIMatrix44D &a, double f)
{
    double *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x, *d3 = &m[3].x;
    const double *s0 = &a.m[0].x, *s1 = &a.m[1].x, *s2 = &a.m[2].x, *s3 = &a.m[3].x;
    d0[0] = s0[0] * f;
    d0[1] = s0[1] * f;
    d0[2] = s0[2] * f;
    d0[3] = s0[3] * f;
    d1[0] = s1[0] * f;
    d1[1] = s1[1] * f;
    d1[2] = s1[2] * f;
    d1[3] = s1[3] * f;
    d2[0] = s2[0] * f;
    d2[1] = s2[1] * f;
    d2[2] = s2[2] * f;
    d2[3] = s2[3] * f;
    d3[0] = s3[0] * f;
    d3[1] = s3[1] * f;
    d3[2] = s3[2] * f;
    d3[3] = s3[3] * f;
}

// The same no-temp accumulation VIMatrix33::Mul uses: `this` IS the
// destination, so rows 1..3 reload their partial sums because the stores may
// alias `a` or `b`.
void VIMatrix44D::Mul(const VIMatrix44D &a, const VIMatrix44D &b)
{
    double *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x, *d3 = &m[3].x;
    const double *t0 = &b.m[0].x, *t1 = &b.m[1].x, *t2 = &b.m[2].x, *t3 = &b.m[3].x;
    double f00 = a.m[0].x;
    d0[0] = t0[0] * f00;
    d0[1] = t0[1] * f00;
    d0[2] = t0[2] * f00;
    d0[3] = t0[3] * f00;
    double f01 = a.m[0].y;
    d0[0] += t1[0] * f01;
    d0[1] += t1[1] * f01;
    d0[2] += t1[2] * f01;
    d0[3] += t1[3] * f01;
    double f02 = a.m[0].z;
    d0[0] += t2[0] * f02;
    d0[1] += t2[1] * f02;
    d0[2] += t2[2] * f02;
    d0[3] += t2[3] * f02;
    double f03 = a.m[0].w;
    d0[0] += t3[0] * f03;
    d0[1] += t3[1] * f03;
    d0[2] += t3[2] * f03;
    d0[3] += t3[3] * f03;
    double f10 = a.m[1].x;
    d1[0] = t0[0] * f10;
    d1[1] = t0[1] * f10;
    d1[2] = t0[2] * f10;
    d1[3] = t0[3] * f10;
    double f11 = a.m[1].y;
    d1[0] += t1[0] * f11;
    d1[1] += t1[1] * f11;
    d1[2] += t1[2] * f11;
    d1[3] += t1[3] * f11;
    double f12 = a.m[1].z;
    d1[0] += t2[0] * f12;
    d1[1] += t2[1] * f12;
    d1[2] += t2[2] * f12;
    d1[3] += t2[3] * f12;
    double f13 = a.m[1].w;
    d1[0] += t3[0] * f13;
    d1[1] += t3[1] * f13;
    d1[2] += t3[2] * f13;
    d1[3] += t3[3] * f13;
    double f20 = a.m[2].x;
    d2[0] = t0[0] * f20;
    d2[1] = t0[1] * f20;
    d2[2] = t0[2] * f20;
    d2[3] = t0[3] * f20;
    double f21 = a.m[2].y;
    d2[0] += t1[0] * f21;
    d2[1] += t1[1] * f21;
    d2[2] += t1[2] * f21;
    d2[3] += t1[3] * f21;
    double f22 = a.m[2].z;
    d2[0] += t2[0] * f22;
    d2[1] += t2[1] * f22;
    d2[2] += t2[2] * f22;
    d2[3] += t2[3] * f22;
    double f23 = a.m[2].w;
    d2[0] += t3[0] * f23;
    d2[1] += t3[1] * f23;
    d2[2] += t3[2] * f23;
    d2[3] += t3[3] * f23;
    double f30 = a.m[3].x;
    d3[0] = t0[0] * f30;
    d3[1] = t0[1] * f30;
    d3[2] = t0[2] * f30;
    d3[3] = t0[3] * f30;
    double f31 = a.m[3].y;
    d3[0] += t1[0] * f31;
    d3[1] += t1[1] * f31;
    d3[2] += t1[2] * f31;
    d3[3] += t1[3] * f31;
    double f32 = a.m[3].z;
    d3[0] += t2[0] * f32;
    d3[1] += t2[1] * f32;
    d3[2] += t2[2] * f32;
    d3[3] += t2[3] * f32;
    double f33 = a.m[3].w;
    d3[0] += t3[0] * f33;
    d3[1] += t3[1] * f33;
    d3[2] += t3[2] * f33;
    d3[3] += t3[3] * f33;
}

// Row-vector convention, and the same shape as VIMatrix33::Mul: `m`'s rows are
// pointers (their FIRST element still folds into an offset off `this`) and the
// vector component is hoisted into a fresh local per row.  Without that local
// `v.x` is reloaded for all four terms -- `out` and `v` are both VIVect4D
// references and may alias, so the store to out.x kills the cached v.x.
void VIMatrix44D::Mul(const VIVect4D &v, VIVect4D &out) const
{
    const double *s0 = &m[0].x, *s1 = &m[1].x, *s2 = &m[2].x, *s3 = &m[3].x;
    double f0 = v.x;
    out.x = s0[0] * f0;
    out.y = s0[1] * f0;
    out.z = s0[2] * f0;
    out.w = s0[3] * f0;
    double f1 = v.y;
    out.x += s1[0] * f1;
    out.y += s1[1] * f1;
    out.z += s1[2] * f1;
    out.w += s1[3] * f1;
    double f2 = v.z;
    out.x += s2[0] * f2;
    out.y += s2[1] * f2;
    out.z += s2[2] * f2;
    out.w += s2[3] * f2;
    double f3 = v.w;
    out.x += s3[0] * f3;
    out.y += s3[1] * f3;
    out.z += s3[2] * f3;
    out.w += s3[3] * f3;
}

// The point form: w is 1.0, so the translation row is added rather than scaled.
void VIMatrix44D::Mul(const VIVect3D &v, VIVect4D &out) const
{
    const double *s0 = &m[0].x, *s1 = &m[1].x, *s2 = &m[2].x, *s3 = &m[3].x;
    double f0 = v.x;
    out.x = s0[0] * f0;
    out.y = s0[1] * f0;
    out.z = s0[2] * f0;
    out.w = s0[3] * f0;
    double f1 = v.y;
    out.x += s1[0] * f1;
    out.y += s1[1] * f1;
    out.z += s1[2] * f1;
    out.w += s1[3] * f1;
    double f2 = v.z;
    out.x += s2[0] * f2;
    out.y += s2[1] * f2;
    out.z += s2[2] * f2;
    out.w += s2[3] * f2;
    out.x += s3[0];
    out.y += s3[1];
    out.z += s3[2];
    out.w += s3[3];
}

void VIMatrix44D::SubMatrixMul(const VIVect3D &v, VIVect3D &out) const
{
    out.x = m[0].x * v.x;
    out.y = m[0].y * v.x;
    out.z = m[0].z * v.x;
    out.x += m[1].x * v.y;
    out.y += m[1].y * v.y;
    out.z += m[1].z * v.y;
    out.x += m[2].x * v.z;
    out.y += m[2].y * v.z;
    out.z += m[2].z * v.z;
}

// BlendAccum and the two vector products are row operations on VIVect4D
// REFERENCES (D4MulSet / D4MulAdd / D4Add here -- VIVect4D declares no
// compound operators): each row is a pointer of its own, the vector component
// times the weight is ONE call per row, and the libcall's result is copied
// before the next row element loads.  Phase 8 read that last order as a
// scheduler tie-break; with the references it is byte-exact (phase 61).
static inline void D4MulSet(VIVect4D &o, const VIVect4D &r, double f)
{
    o.x = r.x * f;
    o.y = r.y * f;
    o.z = r.z * f;
    o.w = r.w * f;
}

static inline void D4Add(VIVect4D &o, const VIVect4D &r)
{
    o.x += r.x;
    o.y += r.y;
    o.z += r.z;
    o.w += r.w;
}

static inline void D4MulAdd(VIVect4D &o, const VIVect4D &r, double f)
{
    o.x += r.x * f;
    o.y += r.y * f;
    o.z += r.z * f;
    o.w += r.w * f;
}


void VIMatrix44D::BlendAccum(const VIVect4D &v, double w, VIVect4D &out) const
{
    D4MulAdd(out, m[0], v.x * w);
    D4MulAdd(out, m[1], v.y * w);
    D4MulAdd(out, m[2], v.z * w);
    D4MulAdd(out, m[3], v.w * w);
}

void VIMatrix44D::BlendAccum(const VIVect3D &v, double w, VIVect4D &out) const
{
    D4MulAdd(out, m[0], v.x * w);
    D4MulAdd(out, m[1], v.y * w);
    D4MulAdd(out, m[2], v.z * w);
    D4MulAdd(out, m[3], w);
}

void VIMatrix44D::SubMatrixBlendAccum(const VIVect3D &v, double w, VIVect3D &out) const
{
    out.x += m[0].x * v.x * w;
    out.y += m[0].y * v.x * w;
    out.z += m[0].z * v.x * w;
    out.x += m[1].x * v.y * w;
    out.y += m[1].y * v.y * w;
    out.z += m[1].z * v.y * w;
    out.x += m[2].x * v.z * w;
    out.y += m[2].y * v.z * w;
    out.z += m[2].z * v.z * w;
}

void VIMatrix44D::UnitRotate(const VIQuatD &q)
{
    double wx = (q.w + q.w) * q.x;
    double wy = (q.w + q.w) * q.y;
    double wz = (q.w + q.w) * q.z;
    double xx = (q.x + q.x) * q.x;
    double xy = (q.x + q.x) * q.y;
    double xz = (q.x + q.x) * q.z;
    double yy = (q.y + q.y) * q.y;
    double yz = (q.y + q.y) * q.z;
    double zz = (q.z + q.z) * q.z;

    m[0].x = 1.0 - (yy + zz);
    m[0].y = xy + wz;
    m[0].z = xz - wy;
    m[0].w = 0.0;
    m[1].x = xy - wz;
    m[1].y = 1.0 - (xx + zz);
    m[1].z = yz + wx;
    m[1].w = 0.0;
    m[2].x = xz + wy;
    m[2].y = yz - wx;
    m[2].z = 1.0 - (xx + yy);
    m[2].w = 0.0;
    m[3].x = 0.0;
    m[3].y = 0.0;
    m[3].z = 0.0;
    m[3].w = 1.0;
}

void VIMatrix44D::Rotate(const VIQuatD &q)
{
    double s = 2.0 / (q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    double xs = q.x * s;
    double ys = q.y * s;
    double zs = q.z * s;
    double wx = q.w * xs;
    double wy = q.w * ys;
    double wz = q.w * zs;
    double xx = q.x * xs;
    double xy = q.x * ys;
    double xz = q.x * zs;
    double yy = q.y * ys;
    double yz = q.y * zs;
    double zz = q.z * zs;

    m[0].x = 1.0 - (yy + zz);
    m[0].y = xy + wz;
    m[0].z = xz - wy;
    m[0].w = 0.0;
    m[1].x = xy - wz;
    m[1].y = 1.0 - (xx + zz);
    m[1].z = yz + wx;
    m[1].w = 0.0;
    m[2].x = xz + wy;
    m[2].y = yz - wx;
    m[2].z = 1.0 - (xx + yy);
    m[2].w = 0.0;
    m[3].x = 0.0;
    m[3].y = 0.0;
    m[3].z = 0.0;
    m[3].w = 1.0;
}

void VIMatrix44D::ConvertToEulerXYZ(VIVect3D &v) const
{
    double cy = sqrt(m[0].x * m[0].x + m[0].y * m[0].y);

    if (cy > 0.0)
    {
        v.z = atan2(m[1].z, m[2].z);
        v.y = atan2(-m[0].z, cy);
        v.x = atan2(m[0].y, m[0].x);
    }
    else
    {
        v.z = atan2(-m[2].y, m[1].y);
        v.y = atan2(-m[0].z, cy);
        v.x = 0.0;
    }
}

void VIMatrix44D::ConvertToEulerZXY(VIVect3D &v) const
{
    double cy = sqrt(m[2].z * m[2].z + m[2].x * m[2].x);

    if (cy > 0.0)
    {
        v.z = atan2(m[0].y, m[1].y);
        v.y = atan2(-m[2].y, cy);
        v.x = atan2(m[2].x, m[2].z);
    }
    else
    {
        v.z = atan2(-m[1].x, m[0].x);
        v.y = atan2(-m[2].y, cy);
        v.x = 0.0;
    }
}

void VIMatrix44D::RotateHPR(const VIVect3D &v)
{
    double si = sin(v.x);
    double ci = cos(v.x);
    double sj = sin(v.y);
    double cj = cos(v.y);
    double sh = sin(v.z);
    double ch = cos(v.z);

    m[0].x = ch * ci + sh * sj * si;
    m[0].y = sh * cj;
    m[0].z = -ch * si + sh * sj * ci;
    m[0].w = 0.0;
    m[1].x = -sh * ci + ch * sj * si;
    m[1].y = ch * cj;
    m[1].z = sh * si + ch * sj * ci;
    m[1].w = 0.0;
    m[2].x = cj * si;
    m[2].y = -sj;
    m[2].z = cj * ci;
    m[2].w = 0.0;
    m[3].x = 0.0;
    m[3].y = 0.0;
    m[3].z = 0.0;
    m[3].w = 1.0;
}

void VIMatrix44D::Transform(const VIQuatD &q, double scale, const VIVect3D &t)
{
    double wx = (q.w + q.w) * q.x;
    double wy = (q.w + q.w) * q.y;
    double wz = (q.w + q.w) * q.z;
    double xx = (q.x + q.x) * q.x;
    double xy = (q.x + q.x) * q.y;
    double xz = (q.x + q.x) * q.z;
    double yy = (q.y + q.y) * q.y;
    double yz = (q.y + q.y) * q.z;
    double zz = (q.z + q.z) * q.z;

    m[0].x = (1.0 - (yy + zz)) * scale;
    m[0].y = (xy + wz) * scale;
    m[0].z = (xz - wy) * scale;
    m[0].w = 0.0;
    m[1].x = (xy - wz) * scale;
    m[1].y = (1.0 - (xx + zz)) * scale;
    m[1].z = (yz + wx) * scale;
    m[1].w = 0.0;
    m[2].x = (xz + wy) * scale;
    m[2].y = (yz - wx) * scale;
    m[2].z = (1.0 - (xx + yy)) * scale;
    m[2].w = 0.0;
    m[3].x = t.x;
    m[3].y = t.y;
    m[3].z = t.z;
    m[3].w = 1.0;
}

void VIMatrix44D::Transform(const VIQuatD &q, const VIVect3D &s, const VIVect3D &t)
{
    double wx = (q.w + q.w) * q.x;
    double wy = (q.w + q.w) * q.y;
    double wz = (q.w + q.w) * q.z;
    double xx = (q.x + q.x) * q.x;
    double xy = (q.x + q.x) * q.y;
    double xz = (q.x + q.x) * q.z;
    double yy = (q.y + q.y) * q.y;
    double yz = (q.y + q.y) * q.z;
    double zz = (q.z + q.z) * q.z;

    m[0].x = (1.0 - (yy + zz)) * s.x;
    m[0].y = (xy + wz) * s.x;
    m[0].z = (xz - wy) * s.x;
    m[0].w = 0.0;
    m[1].x = (xy - wz) * s.y;
    m[1].y = (1.0 - (xx + zz)) * s.y;
    m[1].z = (yz + wx) * s.y;
    m[1].w = 0.0;
    m[2].x = (xz + wy) * s.z;
    m[2].y = (yz - wx) * s.z;
    m[2].z = (1.0 - (xx + yy)) * s.z;
    m[2].w = 0.0;
    m[3].x = t.x;
    m[3].y = t.y;
    m[3].z = t.z;
    m[3].w = 1.0;
}

void VIMatrix44D::Transform(const VIVect3D &v, double scale, const VIVect3D &t)
{
    double si = sin(v.x);
    double ci = cos(v.x);
    double sj = sin(v.y);
    double cj = cos(v.y);
    double sh = sin(v.z);
    double ch = cos(v.z);

    m[0].x = (ch * ci + sh * sj * si) * scale;
    m[0].y = (sh * cj) * scale;
    m[0].z = (-ch * si + sh * sj * ci) * scale;
    m[0].w = 0.0;
    m[1].x = (-sh * ci + ch * sj * si) * scale;
    m[1].y = (ch * cj) * scale;
    m[1].z = (sh * si + ch * sj * ci) * scale;
    m[1].w = 0.0;
    m[2].x = (cj * si) * scale;
    m[2].y = (-sj) * scale;
    m[2].z = (cj * ci) * scale;
    m[2].w = 0.0;
    m[3].x = t.x;
    m[3].y = t.y;
    m[3].z = t.z;
    m[3].w = 1.0;
}

void VIMatrix44D::Transform(const VIVect3D &v, const VIVect3D &s, const VIVect3D &t)
{
    double si = sin(v.x);
    double ci = cos(v.x);
    double sj = sin(v.y);
    double cj = cos(v.y);
    double sh = sin(v.z);
    double ch = cos(v.z);

    m[0].x = (ch * ci + sh * sj * si) * s.x;
    m[0].y = (sh * cj) * s.x;
    m[0].z = (-ch * si + sh * sj * ci) * s.x;
    m[0].w = 0.0;
    m[1].x = (-sh * ci + ch * sj * si) * s.y;
    m[1].y = (ch * cj) * s.y;
    m[1].z = (sh * si + ch * sj * ci) * s.y;
    m[1].w = 0.0;
    m[2].x = (cj * si) * s.z;
    m[2].y = (-sj) * s.z;
    m[2].z = (cj * ci) * s.z;
    m[2].w = 0.0;
    m[3].x = t.x;
    m[3].y = t.y;
    m[3].z = t.z;
    m[3].w = 1.0;
}

void VIMatrix44D::ConvertTo(VIQuatD &q) const
{
    int nxt[3] = { 1, 2, 0 };
    double s;
    double tr = m[0].x + m[1].y + m[2].z;

    if (tr > 0.0)
    {
        s = sqrt(tr + 1.0);
        q.w = s * 0.5;
        s = 0.5 / s;
        q.x = (m[1].z - m[2].y) * s;
        q.y = (m[2].x - m[0].z) * s;
        q.z = (m[0].y - m[1].x) * s;
    }
    else
    {
        double *qq = &q.x;
        int i = 0;
        if (m[1].y > m[0].x)
            i = 1;
        const double *mt = (const double *)(i * 32 + (unsigned)m);
        const double &mti = mt[i];
        if (m[2].z > mti)
            i = 2;
        int j = nxt[i];
        int k = nxt[j];
        const double *mi = (const double *)(i * 32 + (unsigned)m);
        const double *mj = (const double *)(j * 32 + (unsigned)m);
        const double *mk = (const double *)(k * 32 + (unsigned)m);
        const double &mii = mi[i];
        const double &mjj = mj[j];
        const double &mkk = mk[k];
        s = sqrt((mii - (mjj + mkk)) + 1.0);
        double &qi = qq[i];
        qi = s * 0.5;
        if (s != 0.0)
            s = 0.5 / s;
        double &qj = qq[j];
        double &qk = qq[k];
        const double *pjk = mj + k, *pkj = mk + j;
        qq[3] = (*pjk - *pkj) * s;
        const double *pij = mi + j, *pji = mj + i;
        qj = (*pij + *pji) * s;
        const double *pik = mi + k, *pki = mk + i;
        qk = (*pik + *pki) * s;
    }
}

void VIMatrix44D::ConvertTo(VIQuatD &q, double &scale, VIVect3D &t) const
{
    int nxt[3] = { 1, 2, 0 };
    double s;
    double inv = 1.0;

    t.x = m[3].x;
    t.y = m[3].y;
    t.z = m[3].z;

    scale = m[0].x * m[0].x + m[0].y * m[0].y + m[0].z * m[0].z;
    if (scale > 0.0)
    {
        scale = scale * (1.0 / sqrt(scale));
        inv = 1.0 / scale;
    }

    double tr = (m[0].x + m[1].y + m[2].z) * inv;

    if (tr > 0.0)
    {
        s = sqrt(tr + 1.0);
        q.w = s * 0.5;
        s = 0.5 / s;
        q.x = (m[1].z - m[2].y) * s * inv;
        q.y = (m[2].x - m[0].z) * s * inv;
        q.z = (m[0].y - m[1].x) * s * inv;
    }
    else
    {
        double *qq = &q.x;
        int i = 0;
        if (m[1].y > m[0].x)
            i = 1;
        const double *mt = (const double *)(i * 32 + (unsigned)m);
        const double &mti = mt[i];
        if (m[2].z > mti)
            i = 2;
        int j = nxt[i];
        int k = nxt[j];
        const double *mi = (const double *)(i * 32 + (unsigned)m);
        const double *mj = (const double *)(j * 32 + (unsigned)m);
        const double *mk = (const double *)(k * 32 + (unsigned)m);
        const double &mii = mi[i];
        const double &mjj = mj[j];
        const double &mkk = mk[k];
        s = sqrt((mii - (mjj + mkk)) * inv + 1.0);
        double &qi = qq[i];
        qi = s * 0.5;
        if (s != 0.0)
            s = 0.5 / s;
        double &qj = qq[j];
        double &qk = qq[k];
        const double *pjk = mj + k, *pkj = mk + j;
        qq[3] = (*pjk - *pkj) * s * inv;
        const double *pij = mi + j, *pji = mj + i;
        qj = (*pij + *pji) * s * inv;
        const double *pik = mi + k, *pki = mk + i;
        qk = (*pik + *pki) * s * inv;
    }
}

void VIMatrix44D::ConvertTo(VIQuatD &q, VIVect3D &scale, VIVect3D &t) const
{
    int nxt[3] = { 1, 2, 0 };
    double s;
    VIVect3D inv;
    double *iv = &inv.x;
    double *ivp = iv;
    double *sc = &scale.x;

    iv[0] = 1.0;
    iv[1] = 1.0;
    iv[2] = 1.0;

    t.x = m[3].x;
    t.y = m[3].y;
    t.z = m[3].z;

    const double *row = &m[0].x;
    for (int n = 0; n < 3; n++)
    {
        *sc = row[0] * row[0] + row[1] * row[1] + row[2] * row[2];
        if (*sc > 0.0)
        {
            *sc = *sc * (1.0 / sqrt(*sc));
            *ivp = 1.0 / *sc;
        }
        ivp++;
        row += 4;
        sc++;
    }

    double tr = m[0].x * inv.x + m[1].y * inv.y + m[2].z * inv.z;

    if (tr > 0.0)
    {
        s = sqrt(tr + 1.0);
        q.w = s * 0.5;
        s = 0.5 / s;
        q.x = (m[1].z * inv.y - m[2].y * inv.z) * s;
        q.y = (m[2].x * inv.z - m[0].z * inv.x) * s;
        q.z = (m[0].y * inv.x - m[1].x * inv.y) * s;
    }
    else
    {
        double *qq = &q.x;
        int i = 0;
        if (m[1].y > m[0].x)
            i = 1;
        const double *mt = (const double *)(i * 32 + (unsigned)m);
        const double &mti = mt[i];
        if (m[2].z > mti)
            i = 2;
        int j = nxt[i];
        int k = nxt[j];
        const double *mi = (const double *)(i * 32 + (unsigned)m);
        const double *mj = (const double *)(j * 32 + (unsigned)m);
        const double *mk = (const double *)(k * 32 + (unsigned)m);
        const double &ivi = iv[i];
        const double &ivj = iv[j];
        const double &ivk = iv[k];
        const double &mii = mi[i];
        const double &mjj = mj[j];
        const double &mkk = mk[k];
        s = sqrt((mii * ivi - (mjj * ivj + mkk * ivk)) + 1.0);
        double &qi = qq[i];
        qi = s * 0.5;
        if (s != 0.0)
            s = 0.5 / s;
        double &qj = qq[j];
        double &qk = qq[k];
        const double *pjk = mj + k, *pkj = mk + j;
        qq[3] = (*pjk * ivj - *pkj * ivk) * s;
        const double *pij = mi + j, *pji = mj + i;
        qj = (*pij * ivi + *pji * ivj) * s;
        const double *pik = mi + k, *pki = mk + i;
        qk = (*pik * ivi + *pki * ivk) * s;
    }
}

// Gauss-Jordan with partial pivoting on a stack COPY of `a`, `this` seeded to
// the identity and swapped and eliminated in lockstep -- the same algorithm
// VIMatrix33::Invert spells at three, and the one the float 4x4 runs on VU0.
//
// VIMatrix33::Invert's spelling: every element off its ROW, the pivot read
// through an inline taking the row by reference, `p` read before `|p|`, and
// the swap test written `t > mag` (the libcall is __gtdf2(t, mag), not
// __ltdf2(mag, t)).  And two variables REUSED: the elimination factor is `q`
// again and the elimination loop counter is the pivot loop's `k` -- each
// pair shares a register in the target.  2 wrong words (one move scheduled
// past a load), from 152 with the `double (*)[4]` view and separate
// variables.
static inline double &M44DAt(VIVect4D &v, int k) { return (&v.x)[k]; }
int VIMatrix44D::Invert(const VIMatrix44D &a)
{
    VIMatrix44D w(a);
    Identity();
    for (int i = 0; i < 4; i++)
    {
        double p = M44DAt(w.m[i], i);
        int row = i;
        double mag = fabs(p);
        int k;
        for (k = i + 1; k < 4; k++)
        {
            double t = fabs(M44DAt(w.m[k], i));
            if (t > mag)
            {
                p = M44DAt(w.m[k], i);
                mag = t;
                row = k;
            }
        }
        if (p == 0.0)
            return -1;
        w.SwapRows(i, row);
        SwapRows(i, row);
        double q = 1.0 / p;
        (&w.m[i].x)[0] *= q;
        (&w.m[i].x)[1] *= q;
        (&w.m[i].x)[2] *= q;
        (&w.m[i].x)[3] *= q;
        (&m[i].x)[0] *= q;
        (&m[i].x)[1] *= q;
        (&m[i].x)[2] *= q;
        (&m[i].x)[3] *= q;
        for (k = 0; k < 4; k++)
        {
            if (k != i)
            {
                q = -(&w.m[k].x)[i];
                (&w.m[k].x)[0] += (&w.m[i].x)[0] * q;
                (&w.m[k].x)[1] += (&w.m[i].x)[1] * q;
                (&w.m[k].x)[2] += (&w.m[i].x)[2] * q;
                (&w.m[k].x)[3] += (&w.m[i].x)[3] * q;
                (&m[k].x)[0] += (&m[i].x)[0] * q;
                (&m[k].x)[1] += (&m[i].x)[1] * q;
                (&m[k].x)[2] += (&m[i].x)[2] * q;
                (&m[k].x)[3] += (&m[i].x)[3] * q;
            }
        }
    }
    return 0;
}

// The cofactor (adjugate) inverse of the 3x3 part with the translation carried
// through: it assumes the last COLUMN is (0,0,0,1) -- nothing reads
// a.m[0..2].w -- and writes the result's as literal 0/0/0/1.
//
// The determinant is the SIX TRIPLE PRODUCTS written out, left-associated, NOT
// `r0 . (r1 x r2)` over cofactors already formed the way the float VU0 sibling
// computes it.  The two round differently; go-eqoa-vi/vimath/invert.go
// documents the VU0 form and is describing a different computation.
//
// A zero determinant writes NOTHING -- the whole body is inside the guard, so
// `this` is left exactly as the caller had it.  And every negation is a
// SUBTRACTION FROM ZERO (__subdf3(0.0, x)): the EE has no double negate.
void VIMatrix44D::InvertAffine(const VIMatrix44D &a)
{
    double det = ((((a.m[0].x * a.m[1].y * a.m[2].z
                   + a.m[0].y * a.m[1].z * a.m[2].x)
                   + a.m[0].z * a.m[1].x * a.m[2].y)
                   - a.m[0].z * a.m[1].y * a.m[2].x)
                   - a.m[0].y * a.m[1].x * a.m[2].z)
                   - a.m[0].x * a.m[1].z * a.m[2].y;

    if (det != 0.0)
    {
        double q = 1.0 / det;

        m[0].x = (a.m[1].y * a.m[2].z - a.m[1].z * a.m[2].y) * q;
        m[1].x = -(a.m[1].x * a.m[2].z - a.m[1].z * a.m[2].x) * q;
        m[2].x = (a.m[1].x * a.m[2].y - a.m[1].y * a.m[2].x) * q;
        m[0].y = -(a.m[0].y * a.m[2].z - a.m[0].z * a.m[2].y) * q;
        m[1].y = (a.m[0].x * a.m[2].z - a.m[0].z * a.m[2].x) * q;
        m[2].y = -(a.m[0].x * a.m[2].y - a.m[0].y * a.m[2].x) * q;
        m[0].z = (a.m[0].y * a.m[1].z - a.m[0].z * a.m[1].y) * q;
        m[1].z = -(a.m[0].x * a.m[1].z - a.m[0].z * a.m[1].x) * q;
        m[2].z = (a.m[0].x * a.m[1].y - a.m[0].y * a.m[1].x) * q;

        m[3].x = -((m[0].x * a.m[3].x + m[1].x * a.m[3].y) + m[2].x * a.m[3].z);
        m[3].y = -((m[0].y * a.m[3].x + m[1].y * a.m[3].y) + m[2].y * a.m[3].z);
        m[3].z = -((m[0].z * a.m[3].x + m[1].z * a.m[3].y) + m[2].z * a.m[3].z);

        // A CHAINED assignment: the stores land 2, 1, 0 because `=` groups
        // right to left, and that order is two words of the match.
        m[0].w = m[1].w = m[2].w = 0.0;
        m[3].w = 1.0;
    }
}

int VIMatrix44D::Debug()
{
    for (int i = 0; i < 4; i++)
        ;
    return 0;
}

// The free products: the row-vector transform, returning all four components
// even from the VIVect3D form -- the point is widened with w = 1.0.
VIVect4D operator*(const VIVect4D &v, const VIMatrix44D &m)
{
    VIVect4D r;
    D4MulSet(r, m.m[0], v.x);
    D4MulAdd(r, m.m[1], v.y);
    D4MulAdd(r, m.m[2], v.z);
    D4MulAdd(r, m.m[3], v.w);
    return r;
}

VIVect4D operator*(const VIVect3D &v, const VIMatrix44D &m)
{
    VIVect4D r;
    D4MulSet(r, m.m[0], v.x);
    D4MulAdd(r, m.m[1], v.y);
    D4MulAdd(r, m.m[2], v.z);
    D4Add(r, m.m[3]);
    return r;
}
