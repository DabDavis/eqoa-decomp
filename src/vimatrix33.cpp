#include "vi.h"

VIMatrix33::VIMatrix33()
{
    Zero();
}

// The three rows are assigned one at a time in the SOURCE.  gcc's synthesized
// array assignment is a LOOP (counter 2 down to -1, stride 12) and the target
// is straight-line, so `operator=` is user-written -- and the copy constructor
// spells the same three assignments rather than calling it, since the target
// inlines them where an out-of-line `operator=` would have been a `jal`.
VIMatrix33 &VIMatrix33::operator=(const VIMatrix33 &a)
{
    m[0] = a.m[0];
    m[1] = a.m[1];
    m[2] = a.m[2];
    return *this;
}

VIMatrix33::VIMatrix33(const VIMatrix33 &a)
{
    m[0] = a.m[0];
    m[1] = a.m[1];
    m[2] = a.m[2];
}

VIMatrix33::~VIMatrix33()
{
}

void VIMatrix33::Zero()
{
    float *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x;
    d0[0] = 0.0f; d0[1] = 0.0f; d0[2] = 0.0f;
    d1[0] = 0.0f; d1[1] = 0.0f; d1[2] = 0.0f;
    d2[0] = 0.0f; d2[1] = 0.0f; d2[2] = 0.0f;
}

void VIMatrix33::Identity()
{
    Zero();
    m[0].x = 1.0f;
    m[1].y = 1.0f;
    m[2].z = 1.0f;
}

void VIMatrix33::Translate(const VIVect2 &v)
{
    Identity();
    m[3].x = v.x;
    m[3].y = v.y;
}

void VIMatrix33::Scale(float s)
{
    Identity();
    m[0].x = s;
    m[1].y = s;
}

void VIMatrix33::Scale(const VIVect2 &v)
{
    Identity();
    m[0].x = v.x;
    m[1].y = v.y;
}

// sin and cos are computed BEFORE Identity(): the two jal's precede the third,
// and a call cannot be rescheduled across another.
void VIMatrix33::Rotate(float a)
{
    float s = sinf(a);
    float c = cosf(a);
    Identity();
    m[0].x =  c;
    m[0].y =  s;
    m[1].x = -s;
    m[1].y =  c;
}

void VIMatrix33::Store(float *p) const
{
    for (int i = 0; i < 3; i++)
    {
        *p++ = ((const float (*)[3])m)[i][0];
        *p++ = ((const float (*)[3])m)[i][1];
        *p++ = ((const float (*)[3])m)[i][2];
    }
}

int VIMatrix33::Debug()
{
    for (int i = 0; i < 3; i++)
        ;
    return 0;
}

void VIMatrix33::Transpose(const VIMatrix33 &a)
{
    float (*d)[3] = (float (*)[3])m;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            d[j][i] = ((const float (*)[3])a.m)[i][j];
}

void VIMatrix33::SwapRows(int i, int j)
{
    VIVect3 t;
    t = m[i];
    m[i] = m[j];
    m[j] = t;
}

void VIMatrix33::Add(const VIMatrix33 &a, float f)
{
    float *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x;
    const float *s0 = &a.m[0].x, *s1 = &a.m[1].x, *s2 = &a.m[2].x;
    d0[0] = s0[0] + f;
    d0[1] = s0[1] + f;
    d0[2] = s0[2] + f;
    d1[0] = s1[0] + f;
    d1[1] = s1[1] + f;
    d1[2] = s1[2] + f;
    d2[0] = s2[0] + f;
    d2[1] = s2[1] + f;
    d2[2] = s2[2] + f;
}

void VIMatrix33::Add(const VIMatrix33 &a, const VIMatrix33 &b)
{
    float *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x;
    const float *s0 = &a.m[0].x, *s1 = &a.m[1].x, *s2 = &a.m[2].x;
    const float *t0 = &b.m[0].x, *t1 = &b.m[1].x, *t2 = &b.m[2].x;
    d0[0] = s0[0] + t0[0];
    d0[1] = s0[1] + t0[1];
    d0[2] = s0[2] + t0[2];
    d1[0] = s1[0] + t1[0];
    d1[1] = s1[1] + t1[1];
    d1[2] = s1[2] + t1[2];
    d2[0] = s2[0] + t2[0];
    d2[1] = s2[1] + t2[1];
    d2[2] = s2[2] + t2[2];
}

void VIMatrix33::Mul(const VIMatrix33 &a, float f)
{
    float *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x;
    const float *s0 = &a.m[0].x, *s1 = &a.m[1].x, *s2 = &a.m[2].x;
    d0[0] = s0[0] * f;
    d0[1] = s0[1] * f;
    d0[2] = s0[2] * f;
    d1[0] = s1[0] * f;
    d1[1] = s1[1] * f;
    d1[2] = s1[2] * f;
    d2[0] = s2[0] * f;
    d2[1] = s2[1] * f;
    d2[2] = s2[2] * f;
}

// Element k of a row, through the row taken by REFERENCE (Invert, Transpose).
static inline float &M33At(VIVect3 &v, int k) { return (&v.x)[k]; }

// Every element through M33At on its row: the three pointers (a load and a
// store pointer for m[i][j], one shared for m[j][i]) and both i*12 inside the
// guarded preheader come out exactly.  A `float (*)[3]` view folds them --
// 19 wrong words, open since phase 4.
void VIMatrix33::Transpose()
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = i; j < 3; j++)
        {
            float t = M33At(m[i], j);
            M33At(m[i], j) = M33At(m[j], i);
            M33At(m[j], i) = t;
        }
    }
}

// No Identity() here -- the six fields it writes are the only ones touched, so
// the third column is left alone.  And the translation goes into ROW 2, which
// is what makes Translate()'s row 3 a genuine out-of-bounds bug rather than a
// different convention.
void VIMatrix33::Transform(float r, float s, const VIVect2 &t)
{
    float sn = sinf(r);
    float cs = cosf(r);
    m[0].x = cs * s;
    m[0].y = sn * s;
    m[1].x = -sn * s;
    m[1].y = cs * s;
    m[2].x = t.x;
    m[2].y = t.y;
}

void VIMatrix33::Transform(float r, const VIVect2 &s, const VIVect2 &t)
{
    float sn = sinf(r);
    float cs = cosf(r);
    m[0].x = cs * s.x;
    m[0].y = sn * s.y;
    m[1].x = -sn * s.x;
    m[1].y = cs * s.y;
    m[2].x = t.x;
    m[2].y = t.y;
}

// The vector products are row operations through VIVect3 REFERENCES: one
// pointer per row and the product of each row with its component, in the
// shape of VIVect3's own compound operators (phase 61).
static inline void M3MulSet(VIVect3 &o, const VIVect3 &r, float f)
{
    o.x = r.x * f;
    o.y = r.y * f;
    o.z = r.z * f;
}

static inline void M3MulAdd(VIVect3 &o, const VIVect3 &r, float f)
{
    o.x += r.x * f;
    o.y += r.y * f;
    o.z += r.z * f;
}

VIVect3 operator*(const VIVect3 &v, const VIMatrix33 &m)
{
    VIVect3 r;
    M3MulSet(r, m.m[0], v.x);
    M3MulAdd(r, m.m[1], v.y);
    M3MulAdd(r, m.m[2], v.z);
    return r;
}

VIVect3 operator*(const VIVect2 &v, const VIMatrix33 &m)
{
    VIVect3 r;
    M3MulSet(r, m.m[0], v.x);
    M3MulAdd(r, m.m[1], v.y);
    r += m.m[2];
    return r;
}

// Mul accumulates into `this` DIRECTLY -- there is no temporary matrix, which
// is why rows 1 and 2 reload their partial sums from memory: the stores to row
// 0 may alias `a` or `b`.  (go-eqoa-vi/vimath's Matrix33.Mul uses a temp, so it
// differs from the console whenever the destination is also an operand.)
//
// `a`'s element is hoisted into a scalar and `b`'s row into a pointer -- `a` is
// read through plain offsets off a1 with no base materialised at all, which is
// what one load per element looks like.
void VIMatrix33::Mul(const VIMatrix33 &a, const VIMatrix33 &b)
{
    float *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x;
    const float *t0 = &b.m[0].x, *t1 = &b.m[1].x, *t2 = &b.m[2].x;
    float f00 = a.m[0].x;
    d0[0] = t0[0] * f00;
    d0[1] = t0[1] * f00;
    d0[2] = t0[2] * f00;
    float f01 = a.m[0].y;
    d0[0] += t1[0] * f01;
    d0[1] += t1[1] * f01;
    d0[2] += t1[2] * f01;
    float f02 = a.m[0].z;
    d0[0] += t2[0] * f02;
    d0[1] += t2[1] * f02;
    d0[2] += t2[2] * f02;
    float f10 = a.m[1].x;
    d1[0] = t0[0] * f10;
    d1[1] = t0[1] * f10;
    d1[2] = t0[2] * f10;
    float f11 = a.m[1].y;
    d1[0] += t1[0] * f11;
    d1[1] += t1[1] * f11;
    d1[2] += t1[2] * f11;
    float f12 = a.m[1].z;
    d1[0] += t2[0] * f12;
    d1[1] += t2[1] * f12;
    d1[2] += t2[2] * f12;
    float f20 = a.m[2].x;
    d2[0] = t0[0] * f20;
    d2[1] = t0[1] * f20;
    d2[2] = t0[2] * f20;
    float f21 = a.m[2].y;
    d2[0] += t1[0] * f21;
    d2[1] += t1[1] * f21;
    d2[2] += t1[2] * f21;
    float f22 = a.m[2].z;
    d2[0] += t2[0] * f22;
    d2[1] += t2[1] * f22;
    d2[2] += t2[2] * f22;
}

// Gauss-Jordan with partial pivoting on a stack COPY of `a`, with `this` seeded
// to the identity and swapped/eliminated in lockstep -- the same algorithm as
// VIMatrix44::Invert, and the one go-eqoa-vi/vimath/invert.go documents.
// SwapRows is called UNCONDITIONALLY: there is no `row != i` guard in the
// object code, only the two jals.
// Every element is reached off its ROW (`(&row.x)[k]`), never through one
// `float (*)[3]` view: the view is a pseudo copy of sp held in a callee-saved
// register, and folds s[i][i] to i*16 where the target adds i*4 to the row.
// The pivot search reads through an inline taking the row by reference, so
// the |t| read and the `p` read are two pointers, as in the target.  67 wrong
// words (108 through the view); the rest is the scale step's two mults per
// row and the 12-constant registers.  The scale goes through a reference per
// row, and the elimination factor REUSES `q` -- the target keeps both in $f8
// (64 -> 57).
int VIMatrix33::Invert(const VIMatrix33 &a)
{
    VIMatrix33 w(a);
    Identity();
    for (int i = 0; i < 3; i++)
    {
        float mag = fabsf(M33At(w.m[i], i));
        float p = M33At(w.m[i], i);
        int row = i;
        for (int k = i + 1; k < 3; k++)
        {
            float t = fabsf(M33At(w.m[k], i));
            if (mag < t)
            {
                p = M33At(w.m[k], i);
                mag = t;
                row = k;
            }
        }
        if (p == 0.0f)
            return -1;
        w.SwapRows(i, row);
        SwapRows(i, row);
        float q = 1.0f / p;
        VIVect3 &wi = w.m[i];
        wi.x *= q;
        wi.y *= q;
        wi.z *= q;
        VIVect3 &mi = m[i];
        mi.x *= q;
        mi.y *= q;
        mi.z *= q;
        for (int j = 0; j < 3; j++)
        {
            if (j != i)
            {
                q = -(&w.m[j].x)[i];
                (&w.m[j].x)[0] += (&w.m[i].x)[0] * q;
                (&w.m[j].x)[1] += (&w.m[i].x)[1] * q;
                (&w.m[j].x)[2] += (&w.m[i].x)[2] * q;
                (&m[j].x)[0] += (&m[i].x)[0] * q;
                (&m[j].x)[1] += (&m[i].x)[1] * q;
                (&m[j].x)[2] += (&m[i].x)[2] * q;
            }
        }
    }
    return 0;
}
