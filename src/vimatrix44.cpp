#include "vi.h"

// NOT YET BYTE-EXACT, but every part is accounted for: the zero loop IS the
// implicit construction of the `VIVect4 m[4]` member (gcc's array-ctor loop,
// counter down to -1 with `bne`, not `bgez`) and comes out instruction for
// instruction, and the copy IS a 64-byte block move, which needs the class to
// be 16-byte aligned or it becomes ldl/ldr.  What differs is one register
// choice: the target keeps `this` in v0 across the loop and walks v1, this
// build walks v0 and rematerialises `this` from a0 at the end.  Adding a
// self-assignment guard (`if (this != &a)`) reproduces the target's allocation
// exactly -- at the cost of the two instructions the guard itself costs -- so
// the mechanism is "something keeps `this` live across the loop", and fifteen
// spellings that should have done it all coalesce the same way.
VIMatrix44::VIMatrix44(const VIMatrix44 &a)
{
    *this = a;
}

void VIMatrix44::Scale(float s)
{
    Identity();
    m[0].x = s;
    m[1].y = s;
    m[2].z = s;
}

void VIMatrix44::Transpose(const VIMatrix44 &a)
{
    float (*d)[4] = (float (*)[4])m;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            d[j][i] = ((const float (*)[4])a.m)[i][j];
}

void VIMatrix44::Scale(const VIVect3 &v)
{
    Identity();
    m[0].x = v.x;
    m[1].y = v.y;
    m[2].z = v.z;
}

void VIMatrix44::Translate(const VIVect3 &v)
{
    Identity();
    m[3].x = v.x;
    m[3].y = v.y;
    m[3].z = v.z;
}

void VIMatrix44::Store(float *p) const
{
    for (int i = 0; i < 4; i++)
    {
        *p++ = ((const float (*)[4])m)[i][0];
        *p++ = ((const float (*)[4])m)[i][1];
        *p++ = ((const float (*)[4])m)[i][2];
        *p++ = ((const float (*)[4])m)[i][3];
    }
}

void VIMatrix44::StoreTranspose(float *p) const
{
    *p++ = m[0].x;
    *p++ = m[1].x;
    *p++ = m[2].x;
    *p++ = m[3].x;
    *p++ = m[0].y;
    *p++ = m[1].y;
    *p++ = m[2].y;
    *p++ = m[3].y;
    *p++ = m[0].z;
    *p++ = m[1].z;
    *p++ = m[2].z;
    *p++ = m[3].z;
    *p++ = m[0].w;
    *p++ = m[1].w;
    *p++ = m[2].w;
    *p++ = m[3].w;
}

void VIMatrix44::Transpose()
{
    float (*d)[4] = (float (*)[4])m;
    for (int i = 0; i < 4; i++)
        for (int j = i; j < 4; j++)
        {
            float t = d[i][j];
            d[i][j] = d[j][i];
            d[j][i] = t;
        }
}

void VIMatrix44::RotateX(float a)
{
    float s = sinf(a);
    float c = cosf(a);
    Identity();
    m[1].y = c;
    m[1].z = s;
    m[2].y = -s;
    m[2].z = c;
}

void VIMatrix44::RotateY(float a)
{
    float s = sinf(a);
    float c = cosf(a);
    Identity();
    m[0].x = c;
    m[0].z = -s;
    m[2].x = s;
    m[2].z = c;
}

void VIMatrix44::RotateZ(float a)
{
    float s = sinf(a);
    float c = cosf(a);
    Identity();
    m[0].x = c;
    m[0].y = s;
    m[1].x = -s;
    m[1].y = c;
}

void VIMatrix44::ConvertTo(VIVect3 &a, VIVect3 &b, VIVect3 &c) const
{
    VIQuat q;
    ConvertTo(q);
    q.ConvertTo(a);
}

void VIMatrix44::ConvertTo(VIVect3 &a, float &b, VIVect3 &c) const
{
    VIQuat q;
    ConvertTo(q);
    q.ConvertTo(a);
}

VIMatrix44::~VIMatrix44()
{
}

void VIMatrix44::Add(const VIMatrix44 &a, float f)
{
    float *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x, *d3 = &m[3].x;
    const float *s0 = &a.m[0].x, *s1 = &a.m[1].x, *s2 = &a.m[2].x, *s3 = &a.m[3].x;
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

void VIMatrix44::Add(const VIMatrix44 &a, const VIMatrix44 &b)
{
    float *d0 = &m[0].x, *d1 = &m[1].x, *d2 = &m[2].x, *d3 = &m[3].x;
    const float *s0 = &a.m[0].x, *s1 = &a.m[1].x, *s2 = &a.m[2].x, *s3 = &a.m[3].x;
    const float *t0 = &b.m[0].x, *t1 = &b.m[1].x, *t2 = &b.m[2].x, *t3 = &b.m[3].x;
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

void VIMatrix44::UnitRotate(const VIQuat &q)
{
    float wx = (q.w + q.w) * q.x;
    float wy = (q.w + q.w) * q.y;
    float wz = (q.w + q.w) * q.z;
    float xx = (q.x + q.x) * q.x;
    float xy = (q.x + q.x) * q.y;
    float xz = (q.x + q.x) * q.z;
    float yy = (q.y + q.y) * q.y;
    float yz = (q.y + q.y) * q.z;
    float zz = (q.z + q.z) * q.z;

    m[0].x = 1.0f - (yy + zz);
    m[0].y = xy + wz;
    m[0].z = xz - wy;
    m[0].w = 0.0f;
    m[1].x = xy - wz;
    m[1].y = 1.0f - (xx + zz);
    m[1].z = yz + wx;
    m[1].w = 0.0f;
    m[2].x = xz + wy;
    m[2].y = yz - wx;
    m[2].z = 1.0f - (xx + yy);
    m[2].w = 0.0f;
    m[3].x = 0.0f;
    m[3].y = 0.0f;
    m[3].z = 0.0f;
    m[3].w = 1.0f;
}

void VIMatrix44::Rotate(const VIQuat &q)
{
    float s = 2.0f / (q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    float xs = q.x * s;
    float ys = q.y * s;
    float zs = q.z * s;
    float wx = q.w * xs;
    float wy = q.w * ys;
    float wz = q.w * zs;
    float xx = q.x * xs;
    float xy = q.x * ys;
    float xz = q.x * zs;
    float yy = q.y * ys;
    float yz = q.y * zs;
    float zz = q.z * zs;

    m[0].x = 1.0f - (yy + zz);
    m[0].y = xy + wz;
    m[0].z = xz - wy;
    m[0].w = 0.0f;
    m[1].x = xy - wz;
    m[1].y = 1.0f - (xx + zz);
    m[1].z = yz + wx;
    m[1].w = 0.0f;
    m[2].x = xz + wy;
    m[2].y = yz - wx;
    m[2].z = 1.0f - (xx + yy);
    m[2].w = 0.0f;
    m[3].x = 0.0f;
    m[3].y = 0.0f;
    m[3].z = 0.0f;
    m[3].w = 1.0f;
}

void VIMatrix44::ConvertToEulerXYZ(VIVect3 &v) const
{
    float cy = sqrtf(m[0].x * m[0].x + m[0].y * m[0].y);

    if (cy > 0.0f)
    {
        v.z = atan2f(m[1].z, m[2].z);
        v.y = atan2f(-m[0].z, cy);
        v.x = atan2f(m[0].y, m[0].x);
    }
    else
    {
        v.z = atan2f(-m[2].y, m[1].y);
        v.y = atan2f(-m[0].z, cy);
        v.x = 0.0f;
    }
}

void VIMatrix44::ConvertToEulerZXY(VIVect3 &v) const
{
    float cy = sqrtf(m[2].z * m[2].z + m[2].x * m[2].x);

    if (cy > 0.0f)
    {
        v.z = atan2f(m[0].y, m[1].y);
        v.y = atan2f(-m[2].y, cy);
        v.x = atan2f(m[2].x, m[2].z);
    }
    else
    {
        v.z = atan2f(-m[1].x, m[0].x);
        v.y = atan2f(-m[2].y, cy);
        v.x = 0.0f;
    }
}

void VIMatrix44::RotateHPR(const VIVect3 &v)
{
    float si = sinf(v.x);
    float ci = cosf(v.x);
    float sj = sinf(v.y);
    float cj = cosf(v.y);
    float sh = sinf(v.z);
    float ch = cosf(v.z);

    m[0].x = ch * ci + sh * sj * si;
    m[0].y = sh * cj;
    m[0].z = -ch * si + sh * sj * ci;
    m[0].w = 0.0f;
    m[1].x = -sh * ci + ch * sj * si;
    m[1].y = ch * cj;
    m[1].z = sh * si + ch * sj * ci;
    m[1].w = 0.0f;
    m[2].x = cj * si;
    m[2].y = -sj;
    m[2].z = cj * ci;
    m[2].w = 0.0f;
    m[3].x = 0.0f;
    m[3].y = 0.0f;
    m[3].z = 0.0f;
    m[3].w = 1.0f;
}

void VIMatrix44::SubMatrixBlendAccum(const VIVect3 &v, float w, VIVect3 &out) const
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

void VIMatrix44::Transform(const VIQuat &q, float scale, const VIVect3 &t)
{
    float wx = (q.w + q.w) * q.x;
    float wy = (q.w + q.w) * q.y;
    float wz = (q.w + q.w) * q.z;
    float xx = (q.x + q.x) * q.x;
    float xy = (q.x + q.x) * q.y;
    float xz = (q.x + q.x) * q.z;
    float yy = (q.y + q.y) * q.y;
    float yz = (q.y + q.y) * q.z;
    float zz = (q.z + q.z) * q.z;

    m[0].x = (1.0f - (yy + zz)) * scale;
    m[0].y = (xy + wz) * scale;
    m[0].z = (xz - wy) * scale;
    m[0].w = 0.0f;
    m[1].x = (xy - wz) * scale;
    m[1].y = (1.0f - (xx + zz)) * scale;
    m[1].z = (yz + wx) * scale;
    m[1].w = 0.0f;
    m[2].x = (xz + wy) * scale;
    m[2].y = (yz - wx) * scale;
    m[2].z = (1.0f - (xx + yy)) * scale;
    m[2].w = 0.0f;
    m[3].x = t.x;
    m[3].y = t.y;
    m[3].z = t.z;
    m[3].w = 1.0f;
}

void VIMatrix44::Transform(const VIQuat &q, const VIVect3 &s, const VIVect3 &t)
{
    float wx = (q.w + q.w) * q.x;
    float wy = (q.w + q.w) * q.y;
    float wz = (q.w + q.w) * q.z;
    float xx = (q.x + q.x) * q.x;
    float xy = (q.x + q.x) * q.y;
    float xz = (q.x + q.x) * q.z;
    float yy = (q.y + q.y) * q.y;
    float yz = (q.y + q.y) * q.z;
    float zz = (q.z + q.z) * q.z;

    m[0].x = (1.0f - (yy + zz)) * s.x;
    m[0].y = (xy + wz) * s.x;
    m[0].z = (xz - wy) * s.x;
    m[0].w = 0.0f;
    m[1].x = (xy - wz) * s.y;
    m[1].y = (1.0f - (xx + zz)) * s.y;
    m[1].z = (yz + wx) * s.y;
    m[1].w = 0.0f;
    m[2].x = (xz + wy) * s.z;
    m[2].y = (yz - wx) * s.z;
    m[2].z = (1.0f - (xx + yy)) * s.z;
    m[2].w = 0.0f;
    m[3].x = t.x;
    m[3].y = t.y;
    m[3].z = t.z;
    m[3].w = 1.0f;
}

void VIMatrix44::Transform(const VIVect3 &v, float scale, const VIVect3 &t)
{
    float si = sinf(v.x);
    float ci = cosf(v.x);
    float sj = sinf(v.y);
    float cj = cosf(v.y);
    float sh = sinf(v.z);
    float ch = cosf(v.z);

    m[0].x = (ch * ci + sh * sj * si) * scale;
    m[0].y = (sh * cj) * scale;
    m[0].z = (-ch * si + sh * sj * ci) * scale;
    m[0].w = 0.0f;
    m[1].x = (-sh * ci + ch * sj * si) * scale;
    m[1].y = (ch * cj) * scale;
    m[1].z = (sh * si + ch * sj * ci) * scale;
    m[1].w = 0.0f;
    m[2].x = (cj * si) * scale;
    m[2].y = (-sj) * scale;
    m[2].z = (cj * ci) * scale;
    m[2].w = 0.0f;
    m[3].x = t.x;
    m[3].y = t.y;
    m[3].z = t.z;
    m[3].w = 1.0f;
}

void VIMatrix44::Transform(const VIVect3 &v, const VIVect3 &s, const VIVect3 &t)
{
    float si = sinf(v.x);
    float ci = cosf(v.x);
    float sj = sinf(v.y);
    float cj = cosf(v.y);
    float sh = sinf(v.z);
    float ch = cosf(v.z);

    m[0].x = (ch * ci + sh * sj * si) * s.x;
    m[0].y = (sh * cj) * s.x;
    m[0].z = (-ch * si + sh * sj * ci) * s.x;
    m[0].w = 0.0f;
    m[1].x = (-sh * ci + ch * sj * si) * s.y;
    m[1].y = (ch * cj) * s.y;
    m[1].z = (sh * si + ch * sj * ci) * s.y;
    m[1].w = 0.0f;
    m[2].x = (cj * si) * s.z;
    m[2].y = (-sj) * s.z;
    m[2].z = (cj * ci) * s.z;
    m[2].w = 0.0f;
    m[3].x = t.x;
    m[3].y = t.y;
    m[3].z = t.z;
    m[3].w = 1.0f;
}

// Byte-exact apart from ONE word: the `addiu` carrying %lo(nxt initialiser),
// which holds the section offset 2248 in the linked image and 0 + an R_MIPS_LO16
// relocation in an unlinked object.  `eqd.py diff --norm j26,hi16lo16` confirms
// the rest.  The SPELLING below is not believed to be the original: the integer
// cast forces the row pointer's `addu` operands into the target's order
// (index first), and the references force the element addresses into theirs
// (base first).  Both are the mirror of what the natural spelling produces --
// see claude-notes/decomp-phase3-vimatrix44-20260822.md.
void VIMatrix44::ConvertTo(VIQuat &q) const
{
    int nxt[3] = { 1, 2, 0 };
    float s;
    float tr = m[0].x + m[1].y + m[2].z;

    if (tr > 0.0f)
    {
        s = sqrtf(tr + 1.0f);
        q.w = s * 0.5f;
        s = 0.5f / s;
        q.x = (m[1].z - m[2].y) * s;
        q.y = (m[2].x - m[0].z) * s;
        q.z = (m[0].y - m[1].x) * s;
    }
    else
    {
        float *qq = &q.x;
        int i = 0;
        if (m[1].y > m[0].x)
            i = 1;
        const float *mt = (const float *)(i * 16 + (unsigned)m);
        const float &mti = mt[i];
        if (m[2].z > mti)
            i = 2;
        int j = nxt[i];
        int k = nxt[j];
        const float *mi = (const float *)(i * 16 + (unsigned)m);
        const float *mj = (const float *)(j * 16 + (unsigned)m);
        const float *mk = (const float *)(k * 16 + (unsigned)m);
        const float &mii = mi[i];
        const float &mjj = mj[j];
        const float &mkk = mk[k];
        s = sqrtf((mii - (mjj + mkk)) + 1.0f);
        float &qi = qq[i];
        qi = s * 0.5f;
        if (s != 0.0f)
            s = 0.5f / s;
        const float &mik = mi[k];
        const float &mki = mk[i];
        const float &mij = mi[j];
        const float &mji = mj[i];
        const float &mjk = mj[k];
        const float &mkj = mk[j];
        float &qj = qq[j];
        float &qk = qq[k];
        qq[3] = (mjk - mkj) * s;
        qj = (mij + mji) * s;
        qk = (mik + mki) * s;
    }
}

void VIMatrix44::ConvertTo(VIQuat &q, float &scale, VIVect3 &t) const
{
    int nxt[3] = { 1, 2, 0 };
    float s;
    float inv = 1.0f;

    t.x = m[3].x;
    t.y = m[3].y;
    t.z = m[3].z;

    scale = m[0].x * m[0].x + m[0].y * m[0].y + m[0].z * m[0].z;
    if (scale > 0.0f)
    {
        scale = scale * (1.0f / sqrtf(scale));
        inv = 1.0f / scale;
    }

    float tr = (m[0].x + m[1].y + m[2].z) * inv;

    if (tr > 0.0f)
    {
        s = sqrtf(tr + 1.0f);
        q.w = s * 0.5f;
        s = 0.5f / s;
        q.x = (m[1].z - m[2].y) * s * inv;
        q.y = (m[2].x - m[0].z) * s * inv;
        q.z = (m[0].y - m[1].x) * s * inv;
    }
    else
    {
        float *qq = &q.x;
        int i = 0;
        if (m[1].y > m[0].x)
            i = 1;
        const float *mt = (const float *)(i * 16 + (unsigned)m);
        const float &mti = mt[i];
        if (m[2].z > mti)
            i = 2;
        int j = nxt[i];
        int k = nxt[j];
        const float *mi = (const float *)(i * 16 + (unsigned)m);
        const float *mj = (const float *)(j * 16 + (unsigned)m);
        const float *mk = (const float *)(k * 16 + (unsigned)m);
        const float &mii = mi[i];
        const float &mjj = mj[j];
        const float &mkk = mk[k];
        s = sqrtf((mii - (mjj + mkk)) * inv + 1.0f);
        float &qi = qq[i];
        qi = s * 0.5f;
        if (s != 0.0f)
            s = 0.5f / s;
        const float &mik = mi[k];
        const float &mki = mk[i];
        const float &mij = mi[j];
        const float &mji = mj[i];
        const float &mjk = mj[k];
        const float &mkj = mk[j];
        float &qj = qq[j];
        float &qk = qq[k];
        qq[3] = (mjk - mkj) * s * inv;
        qj = (mij + mji) * s * inv;
        qk = (mik + mki) * s * inv;
    }
}

void VIMatrix44::SwapRows(int i, int j)
{
    VIVect4 t;

    t.Set(m[i]);
    m[i].Set(m[j]);
    m[j].Set(t);
}

// NOT YET BYTE-EXACT: 71 of 245 words differ, but the length is exactly right
// and the whole scale loop and both branch bodies match.  The divergence is one
// register-allocation choice in the prologue -- the target materialises
// `&inv.x` into a temporary and copies it (`addiu v0,sp,16` + `move s7,v0`)
// where this build writes `addiu s7,sp,16`, and the extra live pseudo renames
// registers through the else branch.  Fifteen spellings of the local and its
// pointers all coalesce the same way.
void VIMatrix44::ConvertTo(VIQuat &q, VIVect3 &scale, VIVect3 &t) const
{
    int nxt[3] = { 1, 2, 0 };
    float s;
    VIVect3 inv;
    float *iv = &inv.x;
    float *ivp = iv;
    float *sc = &scale.x;

    iv[0] = 1.0f;
    iv[1] = 1.0f;
    iv[2] = 1.0f;

    t.x = m[3].x;
    t.y = m[3].y;
    t.z = m[3].z;

    const float *row = &m[0].x;
    for (int n = 0; n < 3; n++)
    {
        *sc = row[0] * row[0] + row[1] * row[1] + row[2] * row[2];
        if (*sc > 0.0f)
        {
            *sc = *sc * (1.0f / sqrtf(*sc));
            *ivp = 1.0f / *sc;
        }
        ivp++;
        row += 4;
        sc++;
    }

    float tr = m[0].x * inv.x + m[1].y * inv.y + m[2].z * inv.z;

    if (tr > 0.0f)
    {
        s = sqrtf(tr + 1.0f);
        q.w = s * 0.5f;
        s = 0.5f / s;
        q.x = (m[1].z * inv.y - m[2].y * inv.z) * s;
        q.y = (m[2].x * inv.z - m[0].z * inv.x) * s;
        q.z = (m[0].y * inv.x - m[1].x * inv.y) * s;
    }
    else
    {
        float *qq = &q.x;
        int i = 0;
        if (m[1].y > m[0].x)
            i = 1;
        const float *mt = (const float *)(i * 16 + (unsigned)m);
        const float &mti = mt[i];
        if (m[2].z > mti)
            i = 2;
        int j = nxt[i];
        int k = nxt[j];
        const float *mi = (const float *)(i * 16 + (unsigned)m);
        const float *mj = (const float *)(j * 16 + (unsigned)m);
        const float *mk = (const float *)(k * 16 + (unsigned)m);
        const float &ivi = iv[i];
        const float &ivj = iv[j];
        const float &ivk = iv[k];
        const float &mii = mi[i];
        const float &mjj = mj[j];
        const float &mkk = mk[k];
        s = sqrtf((mii * ivi - (mjj * ivj + mkk * ivk)) + 1.0f);
        float &qi = qq[i];
        qi = s * 0.5f;
        if (s != 0.0f)
            s = 0.5f / s;
        const float &mik = mi[k];
        const float &mki = mk[i];
        const float &mij = mi[j];
        const float &mji = mj[i];
        const float &mjk = mj[k];
        const float &mkj = mk[j];
        float &qj = qq[j];
        float &qk = qq[k];
        qq[3] = (mjk * ivj - mkj * ivk) * s;
        qj = (mij * ivi + mji * ivj) * s;
        qk = (mik * ivi + mki * ivk) * s;
    }
}

// Debug() is an EMPTY LOOP: whatever it printed was compiled out, and the four
// iterations plus the five nops the scheduler left behind are all that remain.
int VIMatrix44::Debug()
{
    for (int i = 0; i < 4; i++)
        ;
    return 0;
}
