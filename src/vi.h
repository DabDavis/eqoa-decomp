// EQOA SUPPORT engine headers, recovered by matching.
// Only what the matched translation units need; every declaration here has
// been earned by a byte-exact match, or is marked as a working guess.
#ifndef VI_H
#define VI_H

extern "C" float sinf(float);
extern "C" float acosf(float);
extern "C" float asinf(float);
// The EE has no double-precision FPU: every one of these is a libgcc call, and
// so is every double arithmetic operator in the VIVect3D / VIVect4D group.
extern "C" double sqrt(double);
extern "C" double sin(double);
extern "C" double cos(double);
extern "C" double atan2(double, double);
extern "C" double acos(double);
extern "C" double fabs(double);
extern "C" float sqrtf(float);
extern "C" float fmodf(float, float);
extern "C" float powf(float, float);
extern "C" float atan2f(float, float);
extern "C" float cosf(float);
extern "C" float fabsf(float);
extern "C" float fmodf(float, float);

// The clamp every VIWalkControl input method inlines is VIMIN(hi, VIMAX(lo, v))
// with these as MACROS, and the double evaluation is the evidence: the guest
// tests `v < lo` TWICE around a single `hi < v`, and where lo and hi are
// literals it folds the inner `hi < lo` away entirely.  A helper function, or
// either order of a min/max pair, gives two tests and not three.
#define VIMIN(a, b) ((a) < (b) ? (a) : (b))
#define VIMAX(a, b) ((a) > (b) ? (a) : (b))

class VIVect3D;
class VIVect4D;
class VIQuatD;
class VIMatrix44;
class VIMatrix44D;
class VIPlaneD;
class VIBBox;
class VISphere;
class VIVect2I;
class VIMatrix33;
class VIRaster;
class VIZone;
class VIWorld;
template <class T> class VIArray;
class VILoader;
class VISceneQuery;
class VIEmitterQuery;
class VISceneStreamVars;
class VIPrimBuffer;
class VIZoneRadialFlora;
class VIZoneRoom;
class VIZoneStaticTableElem;
class VICSprite;

class VIVect3
{
public:
    float x, y, z;

    // the local `inv` in ConvertTo(VIQuat&, VIVect3&, VIVect3&) zeroes +4 and
    // +8 before storing 1.0f into all three: a default constructor, with the
    // store at +0 dead-eliminated.
    VIVect3() { x = 0.0f; y = 0.0f; z = 0.0f; }

    // VIVect3 HAS a user assignment operator where VIVect4 must not: VIMatrix33's
    // synthesized `operator=` and copy constructor copy the nine elements one
    // float at a time (lwc1/swc1), where VIMatrix44's are a block move.  Without
    // this, gcc emits an unaligned ldl/ldr block move for `m[i] = m[j]`.
    VIVect3 &operator=(const VIVect3 &v) { x = v.x; y = v.y; z = v.z; return *this; }

    // And a user copy constructor: `return r;` out of the two operator*
    // overloads copies the local into the caller's slot ONE FLOAT AT A TIME.
    // The implicit copy constructor is an unaligned ldl/ldr block move.
    VIVect3(const VIVect3 &v) { x = v.x; y = v.y; z = v.z; }

    // ... and a three-float ctor, earned by VIBBox::Size()/Center(), which
    // construct their VIVect3 return value STRAIGHT into the caller's sret
    // slot -- three swc1 off a0 and `move v0,a0`, no frame at all.  A named
    // local would zero-initialise first and then copy out (Phase 5 fact 1).
    VIVect3(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }

    // VICollRay::Precalc materialises &m_delta and &m_dir into registers and
    // folds each vector's +0 back into the object base -- Phase 9's
    // pointer-variable lever, reached here through INLINE compound operators
    // rather than a named pointer.  Writing the three components out by hand
    // instead reads them all off `this` and lets gcc forward the subtraction
    // results straight into the next statement's stores.
    VIVect3 &operator-=(const VIVect3 &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    VIVect3 &operator+=(const VIVect3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
    VIVect3 &operator*=(float f) { x *= f; y *= f; z *= f; return *this; }

    // Phase 20.  VIPointSprite::Init inflates a bbox around a sphere by its
    // radius; written component by component the radius is re-read off `this`
    // every time and both vectors sit on ONE base register, where the target
    // gives each vector a base of its own and reads the radius through the
    // SPHERE's -- Phase 11's compound-operator lever with a scalar on the
    // right-hand side.
    VIVect3 &operator-=(float f) { x -= f; y -= f; z -= f; return *this; }
    VIVect3 &operator+=(float f) { x += f; y += f; z += f; return *this; }

    // VIBBox's constructor calls this on min and on max: writing the six
    // components out on `this` instead puts all six on ONE base register,
    // where the target gives min and max a base each.
    void Zero() { x = 0.0f; y = 0.0f; z = 0.0f; }
    // Phase 32.  VINameSprite::Raster sets its anchor through this rather than
    // by name: the inlined `this` becomes an address pseudo that stays live
    // across the two calls between the set and the use, which is what parks
    // &at in a callee-saved register the way the target does.
    void Set(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }

    VIVect3 &operator=(const VIVect3D &v);
    float Magnitude() const;
    int   Normalize();
    float NormalizeMagn();
    float Distance(const VIVect3 &v) const;
    void  HPR(float h, float p, float r);
    void  HPR(float *h, float *p, float *r) const;
};

class VIQuat
{
public:
    float x, y, z, w;

    // The zeroing default ctor is earned by VIMatrix44::ConvertTo(VIVect3&,
    // VIVect3&, VIVect3&) and its float sibling, whose local `VIQuat q;`
    // zeroes all four slots before ConvertTo(q) overwrites them.
    VIQuat() { x = 0.0f; y = 0.0f; z = 0.0f; w = 0.0f; }

    // ... and the four-float ctor by operator*, which returns a TEMPORARY:
    // `return VIQuat(a,b,c,d);` is constructed straight into the caller's slot
    // with no stack frame at all, where `VIQuat r; ...; return r;` builds it on
    // the stack and block-copies it out (ldl/ldr/sdl/sdr, 10 extra words).
    VIQuat(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }

    void ConvertTo(VIVect3 &a) const;
    void ConvertFrom(const VIVect3 &v);
    void RotateX(float a);
    void RotateY(float a);
    void RotateZ(float a);
    void Conjugate(const VIQuat &a);
    void InvertUnit(const VIQuat &a);
    void Invert(const VIQuat &a);
    int  Normalize();
    void Mul(const VIQuat &a, const VIQuat &b);
    void UnitRotate(const VIVect3 &v, VIVect3 &out) const;
    void Slerp(const VIQuat &a, const VIQuat &b, float t);
    VIQuat operator*(const VIQuat &b) const;
};

// aligned(16), and that is MEASURED, not assumed: VIPrimBuffer's four packing
// scales are one VIVect4 member and the class puts them at +0x30, four bytes
// past the end of the VIBBox above them.  Every offset in the class is short by
// exactly one word without this.  It is also why VIMatrix44 comes out 16-aligned
// -- the attribute on the matrix class is belt and braces, not the cause.
class __attribute__((aligned(16))) VIVect4
{
public:
    float x, y, z, w;

    // Two things are load-bearing.  The zeroing ctor: SwapRows' temp zeroes
    // +4/+8/+12 (the store at +0 is dead), and the copy constructor's whole
    // loop is this ctor run over `VIMatrix44::m`.  And `Set` rather than
    // `operator=`: the copy must go through a function taking a REFERENCE, or
    // gcc forwards the values in registers instead of round-tripping them
    // through the stack the way SwapRows does -- while an `operator=` here
    // would also make VIMatrix44's implicit assignment element-wise, and the
    // copy constructor's is a 64-byte block move.
    VIVect4() { x = 0.0f; y = 0.0f; z = 0.0f; w = 0.0f; }

    // ... and a four-float ctor, earned by VIAtmosphere::RecomputeLightning-
    // Patches, whose (0, 100, 0, 1) is built inside a loop: the two non-zero
    // constants are hoisted into callee-saved FP registers, which a run of
    // member assignments does not do.
    VIVect4(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }
    void Set(const VIVect4 &v) { x = v.x; y = v.y; z = v.z; w = v.w; }

    VIVect4 &operator=(const VIVect4D &v);
    float Magnitude() const;
    int   Normalize();
    float NormalizeMagn();
    float Distance(const VIVect4 &v) const;
    float DistanceSquared(const VIVect4 &v) const;
};

class VIMatrix44
{
public:
    // NOT `float m[4][4]`: the copy constructor's zero loop is the implicit
    // construction of THIS array (gcc's array-ctor loop, counter down to -1
    // with `bne`), and SwapRows is `t = m[i]; m[i] = m[j]; m[j] = t;` verbatim.
    VIVect4 m[4];

    // The default constructor ZEROES.  For twenty-one phases this was
    // indistinguishable from an empty ctor, because every VIMatrix44 in the
    // corpus was a LOCAL and `VIMatrix44 m; m.Zero();` emits the same words as
    // a zeroing ctor does on its own.  VICollide is the first class to hold one
    // as a MEMBER, and there the two separate: the target's ctor is
    // loop, Zero, loop, Zero -- the array ctor and the asm INTERLEAVED per
    // member, which only a constructor can produce.  A body statement would
    // put both Zero()s after all four member constructions.
    // The asm is written out here rather than `{ Zero(); }`: gcc 2.95 does not
    // re-inline an inline member called from another inline member, and the
    // call form emits `jal Zero__10VIMatrix44` where the target has the five
    // words in line.
    VIMatrix44()
    {
        __asm__ __volatile__(
            "vsub.xyzw $vf4, $vf0, $vf0\n"
            "sqc2 $vf4, 0x30(%0)\n"
            "sqc2 $vf4, 0x20(%0)\n"
            "sqc2 $vf4, 0x10(%0)\n"
            "sqc2 $vf4, 0x0(%0)\n"
            : : "r"(m));
    }

    // VU0 macro mode, and defined IN the class body: VIQuat::ConvertTo inlines
    // these five words (`sqc2 $vf4,48(a2)` ...) while SUPPORT also carries the
    // out-of-line copy at 0x003F01D0.
    void Zero()
    {
        __asm__ __volatile__(
            "vsub.xyzw $vf4, $vf0, $vf0\n"
            "sqc2 $vf4, 0x30(%0)\n"
            "sqc2 $vf4, 0x20(%0)\n"
            "sqc2 $vf4, 0x10(%0)\n"
            "sqc2 $vf4, 0x0(%0)\n"
            : : "r"(m));
    }
    VIMatrix44(const VIMatrix44 &a);
    VIMatrix44 &operator=(const VIMatrix44 &a);
    ~VIMatrix44();

    void Identity();
    void Scale(float s);
    void Scale(const VIVect3 &v);
    void Translate(const VIVect3 &v);
    void Transpose(const VIMatrix44 &a);
    void Transpose();
    void SwapRows(int i, int j);
    void RotateX(float a);
    void RotateY(float a);
    void RotateZ(float a);
    void ConvertTo(VIQuat &q) const;
    void ConvertTo(VIQuat &q, float &scale, VIVect3 &t) const;
    void ConvertTo(VIQuat &q, VIVect3 &scale, VIVect3 &t) const;
    void ConvertTo(VIVect3 &a, VIVect3 &b, VIVect3 &c) const;
    void ConvertTo(VIVect3 &a, float &b, VIVect3 &c) const;
    void Add(const VIMatrix44 &a, float f);
    void UnitRotate(const VIQuat &q);
    void RotateHPR(const VIVect3 &v);
    void Transform(const VIVect3 &v, float scale, const VIVect3 &t);
    void Transform(const VIVect3 &v, const VIVect3 &s, const VIVect3 &t);
    void Transform(const VIQuat &q, float scale, const VIVect3 &t);
    void Transform(const VIQuat &q, const VIVect3 &s, const VIVect3 &t);
    void Rotate(const VIQuat &q);
    int  Invert(const VIMatrix44 &a);
    void InvertAffine(const VIMatrix44 &a);
    void Add(const VIMatrix44 &a, const VIMatrix44 &b);
    void SubMatrixBlendAccum(const VIVect3 &v, float w, VIVect3 &out) const;
    // VIRaster::SetSunLight and SetDirLight run a light direction through the
    // 3x3 of a matrix; the double twin is written in vimatrix44d.cpp.
    void SubMatrixMul(const VIVect3 &v, VIVect3 &out) const;
    // VU0 macro-mode, never attempted -- declared because VIBBox::Transform
    // calls it once per corner.
    void Mul(const VIVect3 &v, VIVect3 &out) const;
    // VU0 macro-mode likewise; VICamera::CalcFrustum runs one NDC corner
    // through each per corner (the VIVect3 form drops the w it just divided
    // by).
    void Mul(const VIVect4 &v, VIVect4 &out) const;
    void Mul(const VIVect4 &v, VIVect3 &out) const;
    void Mul(const VIVect3 &v, VIVect4 &out) const;
    // The VU0 macro-mode 4x4 product; declared for VIGroupSprite, which
    // composes a member transform onto the caller's.
    void Mul(const VIMatrix44 &a, const VIMatrix44 &b);
    void ConvertToEulerXYZ(VIVect3 &v) const;
    void ConvertToEulerZXY(VIVect3 &v) const;
    void Store(float *p) const;
    void StoreTranspose(float *p) const;
    int  Debug();
    VIMatrix44 &operator=(const VIMatrix44D &a);
} __attribute__((aligned(16)));


class VIVect2
{
public:
    float x, y;

    // VIBRect::DistanceSquared's local `VIVect2 d;` stores zero into both stack
    // slots and then READS d.x back as the first comparand, so VIVect2 has the
    // same zeroing default constructor as VIVect3.  Its two-float form is
    // earned by VIBRect::Size()/Center(), which build the return value straight
    // into the caller's sret slot.
    VIVect2() { x = 0.0f; y = 0.0f; }
    VIVect2(float _x, float _y) { x = _x; y = _y; }
    void Set(float _x, float _y) { x = _x; y = _y; }

    // And a user assignment operator, for the same reason VIVect3 needs one:
    // without it `min = v` in VIBRect::Bound and `c = min` in Scale come out as
    // an unaligned ldl/ldr/sdl/sdr 8-byte block move.
    VIVect2 &operator=(const VIVect2 &v) { x = v.x; y = v.y; return *this; }

    float Magnitude() const;
    int   Normalize();
    float NormalizeMagn();
    float Distance(const VIVect2 &v) const;
    float DistanceSquared(const VIVect2 &v) const;
};

class VIVect3D
{
public:
    double x, y, z;

    // The mirror of VIVect3's: the `VIVect3D inv;` in VIMatrix44D::ConvertTo
    // (VIQuatD&, VIVect3D&, VIVect3D&) zeroes +8 and +16 before storing 1.0
    // into all three, exactly as the float sibling does.
    VIVect3D() { x = 0.0; y = 0.0; z = 0.0; }
    VIVect3D(const VIVect3D &v) { x = v.x; y = v.y; z = v.z; }
    VIVect3D &operator=(const VIVect3D &v)
    {
        x = v.x; y = v.y; z = v.z;
        return *this;
    }

    double Magnitude() const;
    int    Normalize();
    double NormalizeMagn();
    double Distance(const VIVect3D &v) const;
    double DistanceSquared(const VIVect3D &v) const;
    void   HPR(double h, double p, double r);
    void   HPR(double *h, double *p, double *r) const;
};

class VIVect4D
{
public:
    double x, y, z, w;

    // The zeroing default constructor is earned by VIMatrix44D::SwapRows,
    // whose temp zeroes +8/+16/+24 (the store at +0 is dead), and by the zero
    // loop at the top of BOTH VIMatrix44D constructors -- gcc's array-ctor
    // loop over `VIVect4D m[4]`, counter 3 down to -1, stride 32.  It is the
    // exact mirror of VIVect4's, and it is why VIVect4D::Distance cannot
    // declare a bare `VIVect4D d;` (see vivect.cpp).
    VIVect4D() { x = 0.0; y = 0.0; z = 0.0; w = 0.0; }
    VIVect4D(const VIVect4D &v) { x = v.x; y = v.y; z = v.z; w = v.w; }
    VIVect4D &operator=(const VIVect4D &v)
    {
        x = v.x; y = v.y; z = v.z; w = v.w;
        return *this;
    }
    // The widening direction, inline: VIMatrix44D::operator=(const VIMatrix44 &)
    // is 78 instructions of four inlined copies, not four calls.
    VIVect4D &operator=(const VIVect4 &v)
    {
        x = v.x; y = v.y; z = v.z; w = v.w;
        return *this;
    }

    double Magnitude() const;
    int    Normalize();
    double NormalizeMagn();
    double Distance(const VIVect4D &v) const;
    double DistanceSquared(const VIVect4D &v) const;
};

class VIMatrix33
{
public:
    // The default constructor's zero loop is 3 iterations of stride 12 with the
    // counter walking down to -1 -- the implicit construction of an array of
    // THREE zeroing VIVect3.  Same shape as VIMatrix44's `VIVect4 m[4]`.
    VIVect3 m[3];

    VIMatrix33();
    VIMatrix33(const VIMatrix33 &a);
    VIMatrix33 &operator=(const VIMatrix33 &a);
    ~VIMatrix33();

    void Identity();
    void Zero();
    void Translate(const VIVect2 &v);
    void Scale(float s);
    void Scale(const VIVect2 &v);
    void Rotate(float a);
    void Add(const VIMatrix33 &a, float f);
    void Add(const VIMatrix33 &a, const VIMatrix33 &b);
    void Mul(const VIMatrix33 &a, float f);
    void Mul(const VIMatrix33 &a, const VIMatrix33 &b);
    void Transpose();
    void Transpose(const VIMatrix33 &a);
    int  Invert(const VIMatrix33 &a);
    void Transform(float r, float s, const VIVect2 &t);
    void Transform(float r, const VIVect2 &s, const VIVect2 &t);
    void SwapRows(int i, int j);
    void Store(float *p) const;
    int  Debug();
};


// ---- the double-precision siblings --------------------------------------
// VIQuatD and VIMatrix44D are the same classes one register class down.  There
// is no VU0 anywhere in either -- the vector unit is single-precision only --
// so every function the float side hides behind a macro-mode block (Mul, the
// two Inverts, BlendAccum, the vector products) is plain C++ here, and readable.

class VIQuatD
{
public:
    double x, y, z, w;

    VIQuatD() { x = 0.0; y = 0.0; z = 0.0; w = 0.0; }
    VIQuatD(double _x, double _y, double _z, double _w) { x = _x; y = _y; z = _z; w = _w; }

    void ConvertTo(VIVect3D &a) const;
    void ConvertFrom(const VIVect3D &v);
    void RotateX(double a);
    void RotateY(double a);
    void RotateZ(double a);
    void Conjugate(const VIQuatD &a);
    void InvertUnit(const VIQuatD &a);
    void Invert(const VIQuatD &a);
    int  Normalize();
    void Mul(const VIQuatD &a, const VIQuatD &b);
    void UnitRotate(const VIVect3D &v, VIVect3D &out) const;
    void Slerp(const VIQuatD &a, const VIQuatD &b, double t);
    VIQuatD operator*(const VIQuatD &b) const;
};

class VIMatrix44D
{
public:
    VIVect4D m[4];

    VIMatrix44D();
    VIMatrix44D(const VIMatrix44D &a);
    // VIQuatD::ConvertTo's stack matrix is destroyed with a `jal` carrying
    // a1 = 2 -- the GNU 2.x __in_chrg argument -- so the class has a
    // user-declared destructor.  The out-of-line copy is NOT in the symbol
    // map (there is no `_$_11VIMatrix44D` entry), so nothing scores it.
    ~VIMatrix44D();
    VIMatrix44D &operator=(const VIMatrix44D &a);
    VIMatrix44D &operator=(const VIMatrix44 &a);

    void Identity();
    void Zero();
    void Translate(const VIVect3D &v);
    void Scale(double s);
    void Scale(const VIVect3D &v);
    void RotateX(double a);
    void RotateY(double a);
    void RotateZ(double a);
    void RotateHPR(const VIVect3D &v);
    void Rotate(const VIQuatD &q);
    void UnitRotate(const VIQuatD &q);
    void Transform(const VIVect3D &v, double scale, const VIVect3D &t);
    void Transform(const VIVect3D &v, const VIVect3D &s, const VIVect3D &t);
    void Transform(const VIQuatD &q, double scale, const VIVect3D &t);
    void Transform(const VIQuatD &q, const VIVect3D &s, const VIVect3D &t);
    void ConvertToEulerZXY(VIVect3D &v) const;
    void ConvertToEulerXYZ(VIVect3D &v) const;
    void ConvertTo(VIQuatD &q) const;
    void ConvertTo(VIQuatD &q, double &scale, VIVect3D &t) const;
    void ConvertTo(VIQuatD &q, VIVect3D &scale, VIVect3D &t) const;
    void Add(const VIMatrix44D &a, double f);
    void Add(const VIMatrix44D &a, const VIMatrix44D &b);
    void Mul(const VIMatrix44D &a, double f);
    void Mul(const VIMatrix44D &a, const VIMatrix44D &b);
    void Mul(const VIVect4D &v, VIVect4D &out) const;
    void Mul(const VIVect3D &v, VIVect4D &out) const;
    void SubMatrixMul(const VIVect3D &v, VIVect3D &out) const;
    void BlendAccum(const VIVect4D &v, double w, VIVect4D &out) const;
    void BlendAccum(const VIVect3D &v, double w, VIVect4D &out) const;
    void SubMatrixBlendAccum(const VIVect3D &v, double w, VIVect3D &out) const;
    int  Invert(const VIMatrix44D &a);
    void InvertAffine(const VIMatrix44D &a);
    void Transpose();
    void Transpose(const VIMatrix44D &a);
    void SwapRows(int i, int j);
    void Store(double *p) const;
    int  Debug();
};


// VIPlane / VIPlaneD -- SUPPORT 0x003F7F00 .. 0x003F87D8.  The normal is the
// FIRST member: `n.Normalize()` inside operator() compiles to a call with
// a0 = this, so &n == this.
class VIPlane
{
public:
    VIVect3 n;
    float   d;

    // A zeroing default constructor: VIArray<VIPlane>::Allocate's element loop
    // stores FOUR zeroes, where the implicit constructor would run VIVect3's
    // and leave `d` alone.  The last of the four is hoisted to the front, so
    // the emitted 12/0/4/8 reads back as n.x, n.y, n.z, d.
    VIPlane() { n.x = 0.0f; n.y = 0.0f; n.z = 0.0f; d = 0.0f; }

    // Phase 46.  Inline: no symbol of its own, and both triangle collides
    // read the POINT first (`mul.s f0, f9, f5` is start.x * n.x).
    float Distance(const VIVect3 &v) const
    { return v.x * n.x + v.y * n.y + v.z * n.z + d; }

    VIPlane &operator=(const VIPlaneD &p);
    VIVect3 Project(const VIVect3 &v) const;
    int RayIntersection(const VIVect3 &o, const VIVect3 &dir, float &t, VIVect3 &out) const;
    int LineIntersection(const VIVect3 &a, const VIVect3 &b, float &t, VIVect3 &out) const;
    int operator()(const VIVect3 &a, const VIVect3 &b, const VIVect3 &c);
};

class VIPlaneD
{
public:
    VIVect3D n;
    double   d;

    VIVect3D Project(const VIVect3D &v) const;
    int RayIntersection(const VIVect3D &o, const VIVect3D &dir, double &t, VIVect3D &out) const;
    int operator()(const VIVect3D &a, const VIVect3D &b, const VIVect3D &c);
};

// --------------------------------------------------------- VIVect2I / rect --
// The integer 2-vector VIRect is built from.  It has NO zeroing default
// constructor: VIRect::DistanceSquared's local is compared against a literal 0
// (`slti v0,v1,1`) where VIBRect's reads its own zeroed slot back.
class VIVect2I
{
public:
    int x, y;

    VIVect2I() { x = 0; y = 0; }
    VIVect2I(int _x, int _y) { x = _x; y = _y; }
    // Phase 42.  VIUI::Blit copies both atlas rectangles out of the entry and
    // then copies each one AGAIN into the by-value argument's temporary, and
    // every one of those six copies is a pair of `lw`/`sw` in the target where
    // the implicit copy constructor is an unaligned `ldl`/`ldr` block move.
    VIVect2I(const VIVect2I &v) { x = v.x; y = v.y; }
    VIVect2I &operator=(const VIVect2I &v) { x = v.x; y = v.y; return *this; }
    // Phase 42.  VIUI::FrameFillRectangle insets its rectangle by a temporary
    // built twice at the SAME stack slot -- `r[0] += VIVect2I(2,2)` -- which is
    // why the constant pair is stored, read, and stored again between the two.
    VIVect2I &operator+=(const VIVect2I &v) { x += v.x; y += v.y; return *this; }
    VIVect2I &operator-=(const VIVect2I &v) { x -= v.x; y -= v.y; return *this; }
};

// SUPPORT 0x003EF750 (VIBRect) / 0x003F9EB0 (VIRect).  min at +0, max at +8.
class VIBRect
{
public:
    VIVect2 min, max;

    void Scale(float s);
    void Scale(const VIVect2 &s);
    void Bound(int first, const VIVect2 &v);
    void Union(const VIBRect &b);
    void Union(const VIBRect &a, const VIBRect &b);
    int  Intersect(const VIBRect &a, const VIBRect &b);
    int  Contains(const VIVect2 &v) const;
    int  Intersects(const VIBRect &b) const;
    int  Store(VIVect2 *p) const;
    int  Transform(const VIMatrix33 &m, VIVect2 *p) const;
    VIVect2 Size();
    VIVect2 Center() const;
    float DistanceSquared(const VIVect2 &v) const;
};

class VIRect
{
public:
    VIVect2I min, max;

    void Scale(int s);
    void Scale(const VIVect2I &s);
    void Bound(int first, const VIVect2I &v);
    void Union(const VIRect &b);
    void Union(const VIRect &a, const VIRect &b);
    int  Intersect(const VIRect &a, const VIRect &b);
    int  Contains(const VIVect2I &v) const;
    int  Intersects(const VIRect &b) const;
    int  Store(VIVect2I *p) const;
    VIVect2I Size() const;
    int  Area();
    VIVect2I Center() const;
    int  DistanceSquared(const VIVect2I &v) const;
};

// ---------------------------------------------------------------- VIBBox --
// SUPPORT 0x003EE220 .. 0x003EF750.  min at +0, max at +12: Contains reads
// `min.x <= v.x` off +0 and `v.x <= max.x` off +12.
class VIBBox
{
public:
    VIVect3 min, max;

    // Both buffer constructors zero min and max TWICE, and both passes address
    // through &min / &max rather than off the outer object -- the second pass
    // is this body, inlined with `this` == &m_bbox.  The members' own zeroing
    // constructors are the first.
    VIBBox() { min.Zero(); max.Zero(); }

    // And a user COPY constructor, read off VIParticleEmitter's three by-value
    // bounds accessors: the caller's slot is ZEROED -- the members' own default
    // constructors -- before the six floats are copied into it, where a
    // synthesized copy constructor would go straight to VIVect3's copy
    // constructor and not zero anything.
    VIBBox(const VIBBox &b) { min = b.min; max = b.max; }

    int  Init(int n, VIVect3 *p);
    void Scale(float s);
    void Scale(const VIVect3 &v);
    void Bound(int first, const VIVect3 &v);
    void Union(const VIBBox &b);
    void Union(const VIBBox &a, const VIBBox &b);
    int  Intersect(const VIBBox &a, const VIBBox &b);
    int  Contains(const VIVect3 &v) const;
    int  Intersects(const VIBBox &b) const;
    int  Intersects(const VIVect3 &a, const VIVect3 &b) const;
    int  Intersects(const VISphere &s) const;
    int  CullIntersect(const VISphere &s) const;
    int  Store(VIVect3 *p) const;
    int  StoreOutward(VIPlane *p) const;
    int  StoreInward(VIPlane *p) const;
    int  Transform(const VIMatrix44 &m, VIVect3 *p) const;
    int  Transform(const VIMatrix44 &m, VIBBox &b) const;
    VIVect3 Size() const;
    VIVect3 Center() const;
    float DistanceSquared(const VIVect3 &v) const;
};

// --------------------------------------------------------------- VISphere --
// SUPPORT 0x003FA3B0.  centre at +0, radius at +12.
class VISphere
{
public:
    VIVect3 c;
    float   r;

    // VISprite's constructor zeroes the sphere TWICE, exactly as it does the
    // bounding box: the member's own VIVect3() first, then this body inlined.
    // Without it only one pass is emitted.
    VISphere() { c.Zero(); r = 0.0f; }

    int Init(const VIBBox &b);
    int Init(const VIVect3 &a, const VIVect3 &b, const VIVect3 &v);
    int PointInSolid(const VIVect3 &v) const;
};

// --------------------------------------------------------------- VIString --
// SUPPORT 0x003EC278 .. 0x003ED5E0.  A counted, heap-allocated 8-bit string
// with a 16-bit (UCS-2) helper suite hung off it as statics.
//
//   +0  m_len    the length in characters, NOT counting the terminator
//   +4  m_size   the allocation in bytes, m_len + 1 when it is tight
//   +8  m_str    the buffer
//
// Read off Allocate: it news m_size bytes, copies m_len of them and writes the
// terminator at [m_len].  Every member function reloads all three through
// `this` after a call, so none of them is cached in the source either.
class VIString
{
public:
    int   m_len;
    int   m_size;
    char *m_str;

    VIString();
    VIString(const VIString &s);
    VIString(const char *p);
    ~VIString();

    int Init(int n);
    int Init(const char *p);
    int Init(const char *p, int n);
    int Init(const VIString &s);
    int Init(const VIString &s, int nStart, int nEnd);
    int Clear();

    // operator= returns by VALUE -- the object code builds a copy into the
    // caller's sret slot (a0) and returns it, so `this` is a1 and the argument
    // a2.  A reference return would have no copy constructor call at all.
    VIString operator=(const VIString &s);
    int operator<(const VIString &s) const;

    int Replace(char cFrom, char cTo);
    int Concat(const VIString &s);
    int Concat(const char *p);
    int Concat(char c);

    VIString operator+(const VIString &s);
    VIString operator+(const char *p);
    VIString operator+(char c);

    int Allocate();
    int Deallocate();

    static int vsnprintf(char *d, int n, const char *f, char *ap);
    static int vsnprintf16(unsigned short *d, int n, const char *f, char *ap);
    static int vsnprintf16(unsigned short *d, int n, const unsigned short *f, char *ap);
    static int snprintf(char *d, int n, const char *f, ...);
    static int snprintf16(unsigned short *d, int n, const char *f, ...);
    static int snprintf16(unsigned short *d, int n, const unsigned short *f, ...);

    static char *strlwr(char *p);
    static char *strupr(char *p);
    static void  strlwr16(unsigned short *p);
    static void  strupr16(unsigned short *p);
    static int   strcasecmp(const char *a, const char *b);
    static int   strcasecmp16(const unsigned short *a, const unsigned short *b);
    static int   strncasecmp(const char *a, const char *b, int n);
    static int   strlen16(const unsigned short *p);
    static unsigned short *strdup16(const unsigned short *p);
    static char *strcpy8(char *d, unsigned int size, const char *s);
    static unsigned short *strcpy16(unsigned short *d, unsigned int size, const unsigned short *s);
    static unsigned short *strncpy16(unsigned short *d, unsigned int size, const unsigned short *s, int n);
    static char char16to8(unsigned short w);
    static unsigned short char8to16(char c);
    static char *str16to8(char *d, unsigned int size, const unsigned short *s);
    static unsigned short *str8to16(unsigned short *d, unsigned int size, const char *s);
    static unsigned short *strcat16(unsigned short *d, unsigned int size, const unsigned short *s);
    static unsigned short *strcat16(unsigned short *d, unsigned int size, const char *s);
    static int strcmp16(const unsigned short *a, const unsigned short *b);
    static const unsigned short *strstr16(const unsigned short *h, const unsigned short *n);

    static unsigned short ConversionTable1252[32];
};

VIVect4D operator*(const VIVect4D &v, const VIMatrix44D &m);
VIVect4D operator*(const VIVect3D &v, const VIMatrix44D &m);

VIVect3 operator*(const VIVect3 &v, const VIMatrix33 &m);
VIVect4 operator*(const VIVect4 &v, const VIMatrix44 &m);   // VU0, not attempted
VIVect4 operator*(const VIVect3 &v, const VIMatrix44 &m);   // VU0-free, not attempted
VIVect3 operator*(const VIVect2 &v, const VIMatrix33 &m);


// ---------------------------------------------------------------- VILNode / VILList
//
// The engine's intrusive doubly-linked list, and it is Amiga Exec's, field for
// field: the header holds three pointers, `m_head` starts life pointing at
// `m_tail` and `m_tailPred` at `m_head`, so both ends of a walk terminate on a
// null WITHOUT a null check -- Init() is `NewList()` transliterated.
class VILList;

class VILNode
{
public:
    VILNode      *m_next;   // +0   ln_Succ
    VILNode      *m_prev;   // +4   ln_Pred
    int           m_pri;    // +8   ln_Pri -- read by VILList::PriorityInsert only
    unsigned int  m_id;     // +C   hashed name

    VILNode();
    void     Remove();
    void     InsertAfter(VILNode *n);
    int      Index() const;
    VILList *List() const;
    void     SetName(const char *name);
};

class VILList
{
public:
    VILNode *m_head;        // +0   lh_Head
    VILNode *m_tail;        // +4   lh_Tail, always 0
    VILNode *m_tailPred;    // +8   lh_TailPred

    VILList();
    ~VILList();
    void     Init();
    void     AddHead(VILNode *n);
    void     AddTail(VILNode *n);
    VILNode *RemHead();
    VILNode *RemTail();
    int      Length() const;
    int      Index(VILNode *n) const;
    VILNode *Node(int i) const;
    VILNode *Find(const char *name) const;
    VILNode *Find(unsigned int id) const;
    void     Reverse();
    void     PriorityInsert(VILNode *n, int dir);
};


// ---------------------------------------------------------------- VICollRay
//
// 0x58 bytes.  The layout is `go-eqoa-vi/vicoll`'s, probed against the console:
// four VIVect3s, two floats, a SECOND copy of the direction, then the hit
// state.  The default constructor is out of line and empty -- everything it
// emits is VIVect3's zeroing ctor run over the six vector members, which is
// why 0x30/0x34/0x44/0x48 are the only words it leaves alone.
class VICollRay
{
public:
    VIVect3 m_start;        // +0x00
    VIVect3 m_end;          // +0x0C
    VIVect3 m_delta;        // +0x18
    VIVect3 m_dir;          // +0x24  unit
    float   m_length;       // +0x30
    float   m_invLength;    // +0x34
    VIVect3 m_dir2;         // +0x38  a genuine second copy of m_dir
    float   m_hitTime;      // +0x44
    int     m_hitIdx;       // +0x48  -1 == no hit
    VIVect3 m_hitNormal;    // +0x4C

    VICollRay();
    int      Init(const VIVect3 &start, const VIVect3 &end);
    int      Init(const VICollRay &src, const VIMatrix44 &mat);
    int      Skitter(VIVect3 &v);
    int      Precalc();
    VIVect3  GetHitPoint();
    VIVect3 &GetHitNormal();
    float    GetHitDistance();
    float    GetHitTime();
};


// ------------------------------------------------- VICollSphere / VIPickSegment
//
// VICollSphere IS-A VICollRay -- plain single inheritance, no vtable: the
// derived ctor is nothing but the base ctor call, and the radius pair sits
// straight after the base's 0x58 bytes.  Both Init and Precalc REPEAT the
// base's body rather than calling it (the base versions are out of line, so a
// call would show as a jal, and there is none).
class VICollSphere : public VICollRay
{
public:
    float m_radius;         // +0x58
    float m_invRadius;      // +0x5C

    VICollSphere();
    int Init(VIVect3 &start, VIVect3 &end, float radius);
    int Init(const VICollSphere &src, const VIMatrix44 &mat);
    int Precalc();
};

class VIColor32F
{
public:
    float r, g, b, a;

    VIColor32F() { r = 0.0f; g = 0.0f; b = 0.0f; a = 0.0f; }
    VIColor32F(float _r, float _g, float _b, float _a) { r = _r; g = _g; b = _b; a = _a; }
    void Set(float _r, float _g, float _b, float _a) { r = _r; g = _g; b = _b; a = _a; }
};

// Not a VICollRay: same first 0x38 bytes by coincidence of purpose, but the
// hit state is laid out differently (no second direction copy) and it carries
// a material id and a colour.
class VIPickSegment
{
public:
    VIVect3    m_start;         // +0x00
    VIVect3    m_end;           // +0x0C
    VIVect3    m_delta;         // +0x18
    VIVect3    m_dir;           // +0x24
    float      m_length;        // +0x30
    float      m_invLength;     // +0x34
    float      m_hitTime;       // +0x38
    int        m_hitMaterial;   // +0x3C  -1 == none
    VIVect3    m_hitNormal;     // +0x40
    VIColor32F m_hitColor;      // +0x4C

    VIPickSegment();
    int        Init(const VIVect3 &start, const VIVect3 &end);
    int        Init(const VIPickSegment &src, const VIMatrix44 &mat);
    int        Precalc();
    VIVect3    GetHitPoint();
    VIVect3    GetHitNormal();
    float      GetHitDistance();
    float      GetHitTime();
    int        GetHitMaterial();
    VIColor32F &GetHitColor();
};


// ---------------------------------------------------------------------------
// VIParticleAttributes -- Phase 12.  The whole layout is readable off the
// accessors: every setter ends `m_version++` (a change stamp at +0x2CC), and
// the const accessors are two-instruction leaves naming their own offset.
// operator= is the IMPLICIT one -- it copies member by member in declaration
// order and returns `this`, which is what settles the type of every field:
// +0x1C is a VIVect3 (three lwc1/swc1 off a materialised base, i.e.
// VIVect3::operator=), the gradient is a 32-entry array of VIColor32F (an
// ldl/ldr block move per entry, 32 iterations), and +0x2C8 is a FLOAT
// (lwc1/swc1) where its integer neighbours at +0x2C4 and +0x2CC are lw/sw.
// Total size 0x2D4 = 724, exactly the length the constructor hands to memset.
// ---------------------------------------------------------------------------
class VIParticleAttributes
{
public:
    float      m_friction;          // +0x000
    float      m_birthrate;         // +0x004
    float      m_birthrateVar;      // +0x008
    float      m_lifespan;          // +0x00C
    float      m_lifespanVar;       // +0x010
    float      m_velocity;          // +0x014
    float      m_velocityVar;       // +0x018
    VIVect3    m_unk1C;             // +0x01C  no accessor; zeroed by its ctor
    float      m_startSize;         // +0x028
    float      m_startSizeVar;      // +0x02C
    float      m_endSize;           // +0x030
    float      m_endSizeVar;        // +0x034
    VIColor32F m_gradient[32];      // +0x038
    VIColor32F m_startColorVar;     // +0x238
    VIColor32F m_endColorVar;       // +0x248
    float      m_inheritVelocity;   // +0x258
    float      m_deltaSpawn;        // +0x25C
    float      m_gradientRepeat;    // +0x260
    VIVect3    m_innerOffset;       // +0x264
    VIVect3    m_innerHprVar;       // +0x270
    VIVect3    m_outerOffset;       // +0x27C
    VIVect3    m_outerHprVar;       // +0x288
    VIVect3    m_nozzleAxis;        // +0x294
    VIVect3    m_nozzleHprVar;      // +0x2A0
    int        m_innerOffsetSet;    // +0x2AC
    int        m_innerHprVarSet;    // +0x2B0
    int        m_outerOffsetSet;    // +0x2B4
    int        m_outerHprVarSet;    // +0x2B8
    int        m_nozzleHprVarSet;   // +0x2BC
    VIVect3   *m_pattern;           // +0x2C0
    int        m_patternSize;       // +0x2C4
    float      m_unk2C8;            // +0x2C8  no accessor
    int        m_version;           // +0x2CC
    float      m_gravity;           // +0x2D0

    VIParticleAttributes();
    ~VIParticleAttributes();

    float Friction() const;
    void  SetFriction(float f);
    float Birthrate() const;
    void  SetBirthrate(float f);
    float BirthrateVar() const;
    void  SetBirthrateVar(float f);
    float Lifespan() const;
    void  SetLifespan(float f);
    float LifespanVar() const;
    void  SetLifespanVar(float f);
    float Velocity() const;
    void  SetVelocity(float f);
    float VelocityVar() const;
    void  SetVelocityVar(float f);
    float StartSize() const;
    void  SetStartSize(float f);
    float StartSizeVar() const;
    void  SetStartSizeVar(float f);
    float EndSize() const;
    void  SetEndSize(float f);
    float EndSizeVar() const;
    void  SetEndSizeVar(float f);
    float InheritVelocity() const;
    void  SetInheritVelocity(float f);
    float DeltaSpawn() const;
    void  SetDeltaSpawn(float f);

    const VIColor32F &StartColorVar() const;
    void  SetStartColorVar(const VIColor32F &c);
    const VIColor32F &EndColorVar() const;
    void  SetEndColorVar(const VIColor32F &c);
    const VIColor32F &GradientColor(int i) const;
    void  SetGradientColor(int i, const VIColor32F &c);

    float GradientRepeat() const;
    void  SetGradientRepeat(float f);

    const VIVect3 &InnerOffset() const;
    void  SetInnerOffset(const VIVect3 &v);
    const VIVect3 &InnerHprVar() const;
    void  SetInnerHprVar(const VIVect3 &v);
    const VIVect3 &OuterOffset() const;
    void  SetOuterOffset(const VIVect3 &v);
    const VIVect3 &OuterHprVar() const;
    void  SetOuterHprVar(const VIVect3 &v);
    const VIVect3 &NozzleAxis() const;
    void  SetNozzleAxis(const VIVect3 &v);
    const VIVect3 &NozzleHprVar() const;
    void  SetNozzleHprVar(const VIVect3 &v);

    int   GravityOn() const;
    void  SetGravityOn(int on);

    int   IsPatternSet() const;
    int   PatternSize() const;
    VIVect3 &Pattern(int i) const;
    int   SetPattern(VIVect3 *pts, int count);
    void  ClearPattern();
};


// ---------------------------------------------------------- VICollBuffer --
// SUPPORT 0x004887F8 .. 0x00489250.  The collision geometry container: a
// triangle-STRIP soup in three packings, laid out as three sections in ONE
// allocation.  `go-eqoa-vi/vicoll/collbuffer.go` is a console-checked Go port
// of the READER, so the layout below is measured, not inferred, and any
// mismatch here is codegen.
//
// The writer's cursors are the half the Go port does not model:
//   +0x00 m_id        +0x04 m_userData   (VIBuffer; Clear never touches them)
//   +0x08 m_type      +0x0C m_locked     +0x10 m_scale
//   +0x14 m_bbox
//   +0x2C/0x30 groups num/max   +0x34/0x38 prims   +0x3C/0x40 verts
//   +0x44/0x48/0x4C section bases -- byte OFFSETS until Init rebases them
//   +0x50 m_vertexGroup  +0x51 m_floraType
//   +0x54 m_dataSize  +0x58 m_data

// The two words every buffer class starts with.  VICollBuffer, VIPrimBuffer
// and VIColorBuffer all zero +0x00 and +0x04 in their constructors, always
// +0x04 FIRST (a chained assignment), and Clear() never touches either --
// VIColorBuffer's constructor is those two stores and nothing else.  They have
// to be initialised BEFORE the member sub-objects, which is what makes this a
// base rather than two statements in each body; a first data member with the
// same constructor is codegen-identical and the object code cannot separate
// the two.
class VIBuffer
{
public:
    int m_id;
    int m_userData;

    VIBuffer() { m_id = m_userData = 0; }
};

// There is no packing 2: Init silently rewrites a 2 to a 3 (0x004888D8
// `li v0,3`) and every dispatcher answers -1 for it.
enum VICollBufferType
{
    VICollBufferTypeV       = 0,
    VICollBufferTypePackV   = 1,
    VICollBufferTypePackVG  = 2,
    VICollBufferTypePackVGF = 3
};

// A packed vertex.  No methods of its own anywhere in the module, so the
// halfword-at-a-time copy in Vertex() is the implicit operator=.
class VIVect3S16
{
public:
    short x, y, z;

    // VICollBuffer::Vertex copies one halfword at a time; the implicit
    // operator= is an unaligned lwl/lwr block move.  Same lever as VIVect3.
    VIVect3S16 &operator=(const VIVect3S16 &v) { x = v.x; y = v.y; z = v.z; return *this; }
};

// One PackVGF vertex: the packed position, the vertex-group byte that selects
// a translation out of VICollide's table at +0x90, and the flora type.
class VICollVertVGF
{
public:
    VIVect3S16  v;
    signed char m_group;
    signed char m_flora;
};

// 0x20 bytes.  The two counts are what make a culled group cheap -- the
// walker SKIPS by adding them rather than iterating.
class VICollGroup
{
public:
    int    m_numPrims;
    int    m_numVerts;
    VIBBox m_bbox;
};

// 0x1C bytes.  Both halfwords are read with `lh` by the walkers, so both are
// signed.
class VICollPrim
{
public:
    short  m_matIdx;
    short  m_numVerts;
    VIBBox m_bbox;
};

// ---------------------------------------------------------------------------
// VIPrimBuffer -- the RENDER geometry container, sibling of VICollBuffer.
//
// Same skeleton -- a VIBuffer base, a VIBBox, three num/max cursor pairs and
// three section pointers rebased by m_data -- but what it fills is a GS
// display list: every "vertex" is one or more QUADWORDS carrying a GIFtag, so
// the vertex section is indexed by `<< 4` and the counts in CalcDataSize are
// in quadwords, not bytes.
//
// The nine type values are settled from the DISPATCHERS, not guessed:
// VIRaster::PickPrimBuffer's if-chain tests 0, 2, 4 and 5 in that order and
// calls PickPrimBufferVUNC / PackVUNC / PackVUNCG / PackVUNBW, and
// VIRaster::DrawPrimBuffer(int,int,float,uint) tests 8 alone and calls
// DrawPrimBufferPackVU.  The four unused values are named here for the shape
// of the enum only -- nothing in the module reads them.
enum VIPrimBufferType
{
    VIPrimBufferTypeVUNC        = 0,
    VIPrimBufferTypeUnused1     = 1,
    VIPrimBufferTypePackVUNC    = 2,
    VIPrimBufferTypeUnused3     = 3,
    VIPrimBufferTypePackVUNCG   = 4,
    VIPrimBufferTypePackVUNBW   = 5,
    VIPrimBufferTypeUnused6     = 6,
    VIPrimBufferTypeUnused7     = 7,
    VIPrimBufferTypePackVU      = 8
};

class VIVect2S16
{
public:
    short x, y;

    VIVect2S16 &operator=(const VIVect2S16 &v) { x = v.x; y = v.y; return *this; }
};

class VIVect3S8
{
public:
    signed char x, y, z;

    VIVect3S8 &operator=(const VIVect3S8 &v) { x = v.x; y = v.y; z = v.z; return *this; }
};

class VIVect4S8
{
public:
    signed char x, y, z, w;

    VIVect4S8 &operator=(const VIVect4S8 &v) { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }
};

class VIVect4U8
{
public:
    unsigned char x, y, z, w;

    VIVect4U8 &operator=(const VIVect4U8 &v) { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }
};

// Copied with an unaligned lwl/lwr word move in Color(), so no user
// operator= -- the same evidence that gave VIVect4 none.
class VIColor32
{
public:
    unsigned char r, g, b, a;

    // Opaque black by default: VIMaterial's constructor zeroes m_emissive
    // TWICE -- once with a = 255 and once with all four zero -- and the first
    // pass is this constructor, the second the body's own assignment.  Same
    // evidence shape as VIBBox's zeroing constructor in Phase 13.
    VIColor32() { r = 0; g = 0; b = 0; a = 255; }

    // VIRFont::CreateResources builds a white one as the argument to
    // SetLayerColor, and it is FOUR stores, not the default constructor's four
    // plus Set's four -- so the class has a four-byte form as well.
    VIColor32(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
    { r = _r; g = _g; b = _b; a = _a; }

    // An inline member, not four field assignments: VIMaterial's constructor
    // and all three Init forms materialise `&m_emissive` into a register and
    // fold its +0 back into the outer base -- Phase 9's pointer lever, reached
    // here through `this`.
    void Set(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
    { r = _r; g = _g; b = _b; a = _a; }
};

// 0x10 bytes.  BeginMaterial stamps the index and the first vertex, and
// EndMaterial closes the span; m_numPrims is bumped once per End().
class VIPrimMaterial
{
public:
    int m_matIdx;
    int m_numPrims;
    int m_firstVert;
    int m_numVerts;
};

// 0x1C bytes -- an int count where VICollPrim packs two shorts, because the
// GIFtag arithmetic reads it with `lw`.
class VIPrimPrim
{
public:
    int    m_numVerts;
    VIBBox m_bbox;
};

// Only the static members VIPrimBuffer reaches for.  They are VU1 microprogram
// entry offsets -- End() ORs one into a `MSCAL` VIFcode (0x14 << 24) -- and
// they are the reason End's four packed cases look byte-identical in the image
// and still do not cross-jump: the image is un-relocated, so the symbol each
// `lw` names is zeroed out, but at compile time they are five DIFFERENT
// symbols and the backward walk stops on the first insn.  Which offset goes
// with which format cannot be read back out of the image for that same reason;
// the ascending order here is the declaration order of the data symbols.
class VIDictionary;
class VIPickSegment;
class VIMaterial;
class VISurface;

enum VIAlphaBlendMode
{
    VIAlphaBlendModeNone = 0,
    VIAlphaBlendMode1    = 1,
    VIAlphaBlendMode2    = 2,
    VIAlphaBlendMode3    = 3,
    VIAlphaBlendMode4    = 4
};

// Phase 55, from VIAtmosphere's three billboard renderers: 48 bytes, and the
// raster hands the caller a run of them to fill.  Only the fields those three
// write are named; +0x18 and +0x1C are never touched here.
class VIBillboard
{
public:
    VIVect3    m_location;   // +0x00
    int        m_frame;      // +0x0C
    float      m_width;      // +0x10
    float      m_height;     // +0x14
    int        m_p18;        // +0x18
    int        m_p1C;        // +0x1C
    VIColor32F m_color;      // +0x20
};

// VIRaster -- the class body moved to the END of this header in phase 56:
// its members are VIPool / VIList / VIArray instantiations, and those
// templates are declared below it.  Only the forward declaration at the
// top of the file is needed here.

class VIDictionary;

// ---------------------------------------------------------------------------
// VIMaterial -- the render state for one surface, and the second property bag
// in the corpus after VIParticleAttributes.  Its layout reads off its own
// accessors: every layer setter multiplies the index by 144 and adds it to
// `this`, so the layer array starts at +0x30 with a 144-byte stride, and
// almost every setter closes with `m_dirty = 1` at +0x20.
enum VIMaterialFillType
{
    VIMaterialFillTypeTexture = 0,
    VIMaterialFillTypeSolid   = 1
};

enum VIMaterialWrapMode
{
    VIMaterialWrapModeRepeat = 0,
    VIMaterialWrapModeClamp  = 1,
    VIMaterialWrapModeRegion = 2,
    VIMaterialWrapModeMirror = 3
};

// The layer, twice.  `VIMaterialPass` is the real member type, and phase 40
// reads the NAME off VIEESurfCache::Set, whose third parameter mangles
// `R14VIMaterialPass` and whose stores land at +0x04, +0x34, +0x48 and
// +0x50..+0x8F -- exactly this class's flags, LOD bias, stamp and packet.  The
// constructor constructs an array of THREE of them, which is where the
// VIMatrix33 constructor call in its loop comes from.  `VIMaterialLayerView`
// is the same 144 bytes with the material's 48-byte header in front, and it is
// what the ACCESSORS address through.
//
// That second type is measured, not decoration.  Every layer accessor in the
// target computes `this + i * 144` and reads a field at a displacement of 48,
// 52, 56, 60, 64, 100 or 104 -- the array base is folded into the
// DISPLACEMENT, not into the pointer.  A subscript `m_layers[i].m_flags`
// cannot produce that: gcc folds the 48 and the field offset onto `this` and
// leaves the load at `0(reg)`, which is two instructions different in every
// read-modify-write accessor, and no spelling of the subscript moves it (four
// swept, plus a standalone toy).  Init() walks the layers the same way,
// stepping one register by 144 while using displacements 48..59.
class VIMaterialPass
{
public:
    int          m_texture;       // +0x00, -1 when the layer has none
    unsigned int m_flags;         // +0x04  1 modulate, 6 wrap, 8 point,
                                  //        0x10 uv set, 0x20 solid fill
    VIColor32    m_color;         // +0x08
    int          m_blendMode;     // +0x0C
    VIMatrix33   m_uvTransform;   // +0x10 .. 0x33
    float        m_lodBias;       // +0x34
    VIVect2      m_uvRate;        // +0x38
    VIVect2      m_uvOffset;      // +0x40
    // +0x48 is the CACHE STAMP: VIEESurfCache::Cache compares it against the
    // cached VIEESurfCacheInfo's own +0x50 and skips the whole upload when
    // they agree, then copies the info's value back into it.
    int          m_stamp;         // +0x48
    int          m_pad4C;         // +0x4C  the packet below is 16-aligned
    // Four GS A+D pairs, written by VIEESurfCache::Set: TEX0_1 (reg 6),
    // TEX1_1 (0x14), MIPTBP1_1 (0x34) and MIPTBP2_1 (0x36).
    long         m_reg[8];        // +0x50 .. +0x8F
};

class VIMaterialLayerView
{
public:
    char         m_header[48];
    int          m_texture;
    unsigned int m_flags;
    VIColor32    m_color;
    int          m_blendMode;
    VIMatrix33   m_uvTransform;
    float        m_lodBias;
    VIVect2      m_uvRate;
    VIVect2      m_uvOffset;
    int          m_pass;
    char         m_packet[20];
};

// VIBuffer, again: the constructor's first two stores are +0x04 then +0x00,
// which is the base's chained `m_id = m_userData = 0` -- the same signature
// VICollBuffer, VIPrimBuffer and VIColorBuffer carry.
// aligned(16): VIList<VIMaterial>'s node is 496 bytes with its two links at
// +480/+484, so the 488 the members alone account for is rounded UP -- the
// element's alignment is 16, not 8.  The attribute moves no member (480 is
// already a multiple of 16), which is why it was invisible in Phase 15.
class VIMaterial : public VIBuffer
{
public:
    int             m_numLayers;    // +0x08
    unsigned int    m_flags;        // +0x0C  1 ZTest, 2 ZWrite, 4 UV animated,
                                    //        0x10 dither, 0x20 tessellate
    VIColor32       m_emissive;     // +0x10
    int             m_pad14;        // +0x14
    double          m_time;         // +0x18  zeroed as one 64-bit `sd`
    int             m_dirty;        // +0x20
    int             m_handle;       // +0x24  = -1
    int             m_pad28;        // +0x28
    int             m_pad2C;        // +0x2C
    VIMaterialPass  m_layers[3];    // +0x30, stride 144; SetNumLayers caps at 4

    VIMaterial();

    int  Init(int layers);
    int  InitWireframe();
    int  InitSolidFill();
    int  SetZWrite(int on);
    int  SetZTest(int on);
    int  SetDither(int on);
    int  SetTessellate(int on);
    int  SetEmissiveColor(const VIColor32 &c);
    int  SetLayerFillType(int i, VIMaterialFillType t);
    int  GetLayerTexture(int i);
    int  SetLayerColor(int i, const VIColor32 &c);
    VIColor32 GetLayerColor(int i) const;
    int  SetLayerModulate(int i, int on);
    int  SetLayerWrapMode(int i, VIMaterialWrapMode m);
    int  GetLayerWrapMode(int i);
    int  SetLayerBlendMode(int i, VIAlphaBlendMode m);
    int  GetLayerBlendMode(int i);
    int  SetLayerUVTransform(int i, const VIMatrix33 &m);
    int  SetLayerUVRate(int i, const VIVect2 &r);
    int  SetLayerTextureLODBias(int i, float b);
    int  SetLayerMagFilter(int i, int on);
    int  SetLayerUVSet(int i, int set);
    int  SetLayerTexture(int i, int tex, VIRaster &r, VIDictionary &d);
} __attribute__((aligned(16)));

class VIPrimBuffer : public VIBuffer
{
public:
    int             m_type;
    int             m_locked;
    int             m_userFlags;
    VIBBox          m_bbox;
    // The four packing scales, and they are ONE object: the constructor zeroes
    // +0x30..+0x3C before it calls Clear(), which is a member's zeroing
    // constructor and nothing else.  x is the position scale, y the UV scale,
    // z the normal/weight scale and w the colour scale; SetPacking writes the
    // first three and Unlock the last.
    VIVect4         m_scale;
    int             m_numMaterials;
    int             m_maxMaterials;
    int             m_numPrims;
    int             m_maxPrims;
    int             m_numVerts;
    int             m_maxVerts;
    VIPrimMaterial *m_materials;
    VIPrimPrim     *m_prims;
    char           *m_verts;
    int             m_dataSize;
    char           *m_data;

    static VIVect2    PrimUV0;
    static VIVect2S16 PrimUV0S16;
    static VIVect2    PrimUV1;
    static VIVect2S16 PrimUV1S16;
    static VIVect3    PrimNormal;
    static VIVect3S8  PrimNormalS8;
    static VIColor32  PrimColor;
    static VIVect4U8  PrimBones;
    static VIVect4    PrimWeights;
    static VIVect4S8  PrimWeightsS8;
    static int        PrimVQStart;
    // unsigned: Vertex(VIVect3S16&)'s PackVUNCG arm reads it with `lhu`.  The
    // setter takes a signed `short` -- the mangled name says so -- but the
    // static it writes into is unsigned.
    static unsigned short PrimVertexGroup;

    VIPrimBuffer();
    ~VIPrimBuffer();

    int  Init(VIRaster &r, VIPrimBufferType t, int mats, int prims, int verts);
    int  Clear();
    int  GetNumVertices() const;
    int  GetNumTriangles() const;
    int  Lock();
    int  Unlock();
    int  SetPacking(int v, int uv, int n);
    int  BeginMaterial(int mat);
    int  EndMaterial();
    int  Begin(int prim);
    int  Vertex(VIVect3 &v);
    int  Vertex(VIVect3S16 &v);
    int  UV0(VIVect2 &v);
    int  UV0(VIVect2S16 &v);
    int  UV1(VIVect2 &v);
    int  UV1(VIVect2S16 &v);
    int  Normal(VIVect3 &v);
    int  Normal(VIVect3S8 &v);
    int  Color(VIColor32 &c);
    int  Bones(VIVect4U8 &b);
    int  Weights(VIVect4 &w);
    int  Weights(VIVect4S8 &w);
    int  VertexGroup(short g);
    int  End();
    int  CalcBVolumes(int n, VIVect3 *p);
    int  CalcDataSize(int mats, int prims, int verts);
    int  Allocate();
};

class VICollBuffer : public VIBuffer
{
public:
    int          m_type;
    int          m_locked;
    float        m_scale;
    VIBBox       m_bbox;
    int          m_numGroups;
    int          m_maxGroups;
    int          m_numPrims;
    int          m_maxPrims;
    int          m_numVerts;
    int          m_maxVerts;
    VICollGroup *m_groups;
    VICollPrim  *m_prims;
    char        *m_verts;
    signed char  m_vertexGroup;
    signed char  m_floraType;
    int          m_dataSize;
    char        *m_data;

    VICollBuffer();
    ~VICollBuffer();

    int  Init(VICollBufferType t, int groups, int prims, int verts);
    int  Clear();
    int  Lock();
    int  Unlock();
    int  SetPacking(int n);
    int  BeginPrimGroup();
    int  EndPrimGroup();
    int  Begin(int mat);
    int  Vertex(VIVect3 &v);
    int  Vertex(VIVect3S16 &v);
    int  VertexGroup(signed char g);
    int  FloraType(signed char f);
    int  End();
    int  CalcBVolumes(int n, VIVect3 *p);
    int  CalcDataSize();
    int  Allocate();
};

// ---------------------------------------------------------------- VISurface --
// SUPPORT 0x0040DE50 .. 0x0040E780.  The texture surface: pixels for up to ten
// mip levels plus an optional CLUT, carved out of ONE allocation, with a
// 64-byte VIF/GIF header written in front of every section so a level can be
// uploaded to the GS by handing the DMAC a pointer.
//
//   +0x00 m_id  +0x04 m_userData       (VIBuffer)
//   +0x08 m_width      +0x0C m_height
//   +0x10 m_pixelStore                 (default VIPixelStoreRGBA32)
//   +0x14 m_index      +0x18 m_cacheIndex   both -1, and Deallocate resets
//                                          both (phase 40 named them)
//   +0x1C m_lockLevel  (-1 unlocked; LockPalette parks m_numMipLevels here)
//   +0x20 m_dirty      +0x24 m_pad24
//   +0x28 m_paletteCount   +0x2C m_paletteOffset
//   +0x30 m_paletteSize    +0x34 m_paletteStore
//   +0x38 m_paletteHeader  -- a byte OFFSET until SetDMAHeaders rebases it
//   +0x3C m_paletteQWC     +0x40 m_numMipLevels
//   +0x44 m_levels[10], stride 24      +0x134 m_dataSize   +0x138 m_data
//
// The level count 10 is the ctor's own loop bound (`li a2,9`, ten stores at
// +0x54 descending by 24), and the stride is LockMipLevel's `li a1,24` mult.
// Phase 30 CORRECTS the mapping below.  Phase 16 read it off the ORDER of
// CalcMipLevelStride's case bodies, which is source order and says nothing
// about the values; the JUMP TABLE itself (0x004EA560, six words) says
//   0 -> w/2 rounded up   1 -> w   2 -> w*4   3 -> w*4   4 -> w*3   5 -> w*2
// so RGB24 is 4, not 3, and 3 is a second four-byte store rather than the
// default.  VIRFont's two switches are the same shape with a SEVENTH entry
// (`sltiu 7`), value 6, also four bytes.  Nothing in the corpus names 3 or 6.
// The instruction bytes are unaffected either way -- the table is data, and a
// wrong label set with the right block order and the right bound produces the
// same code -- which is why the error survived Phase 16 byte-exact.
// Phase 40 settles 3 against 4.  VIEESurfCache::CalcCacheInfo's own table
// (0x004EA590) maps 3 to GS PSMCT24 and REJECTS 4, while CalcMipLevelStride
// gives 3 a four-byte stride and 4 a three-byte one: 3 is 24-bit colour in a
// 32-bit slot (uploadable as PSMCT24), 4 is the PACKED 24-bit form, which has
// a stride but no upload path.
enum VIPixelStore
{
    VIPixelStoreI4     = 0,
    VIPixelStoreI8     = 1,
    VIPixelStoreRGBA32 = 2,
    VIPixelStore3      = 3,   // RGB in a 32-bit slot -> PSMCT24
    VIPixelStoreRGB24  = 4,   // packed 24-bit; the cache will not upload it
    VIPixelStoreRGBA16 = 5,
    VIPixelStore6      = 6
};

// Init's switch is bounded `sltiu 5`, so five values; only four have a body.
enum VIBitDepth
{
    VIBitDepthNone = 0,
    VIBitDepth32   = 1,
    VIBitDepth16   = 2,
    VIBitDepth8    = 3,
    VIBitDepth4    = 4
};

class VIMipLevel
{
public:
    int            m_width;     // +0x00
    int            m_height;    // +0x04
    int            m_pixels;    // +0x08  a byte OFFSET into m_data, and it
                                //        stays one: LockMipLevel adds m_data
    int            m_stride;    // +0x0C
    int            m_header;    // +0x10  a byte offset too, until SetDMAHeaders
                                //        rebases it onto m_data in place
    int            m_qwc;       // +0x14
};

// Allocate's call takes the byte count in a0 and returns the block in v0, and
// Deallocate's takes the block -- with the source's own null test and NO
// second one, which is what rules out `delete []` (Phase 10: delete emits its
// own check, and VIString::Deallocate shows it as the only test there).
void *VIMemAlloc(int n);
void  VIMemFree(void *p);

class VISurface : public VIBuffer
{
public:
    int            m_width;
    int            m_height;
    VIPixelStore   m_pixelStore;
    int            m_index;      // +0x14  its own slot in the raster's surface
                                 //        pool: VIEESurfCache keys its info
                                 //        map on it and hands it back to
                                 //        VIRaster::Surface
    int            m_cacheIndex; // +0x18  the VIEESurfCache info index, -1
                                 //        when the surface is not resident
    int            m_lockLevel;
    int            m_dirty;
    int            m_pad24;
    int            m_paletteCount;
    int            m_paletteOffset;
    int            m_paletteSize;
    VIPixelStore   m_paletteStore;
    int            m_paletteHeader;
    int            m_paletteQWC;
    int            m_numMipLevels;
    VIMipLevel     m_levels[10];
    int            m_dataSize;
    unsigned char *m_data;

    VISurface();
    // VIRaster::ReleaseDeferred does `delete s` and the target calls
    // `_$_9VISurface(s, 3)`, not `__builtin_delete` -- so the class HAS a
    // destructor, and the null check that goes with one.
    ~VISurface();

    int Init(VIRaster &r, int w, int h, VIBitDepth bd, int levels);
    int LockMipLevel(int level, int &stride, VIPixelStore &ps, unsigned char **p);
    int UnlockMipLevel(int level);
    int LockPalette(int &count, VIPixelStore &ps, unsigned char **p);
    int UnlockPalette();
    int Allocate();
    int Deallocate();
    int CalcSurfaceOffsets();
    int SetDMAHeaders();
    int CalcMipLevelStride(int w);
    int ReorderPalette();
};

// ---------------------------------------------------------------------------
// VISound / VIWave / VIXm -- the sound resources, and the first POLYMORPHIC
// classes in the corpus.  Everything before this either had no vtable at all
// or inherited without one (VICollSphere, Phase 11).
//
// THE VPTR GOES AT THE END OF THE CLASS'S OWN DATA in this compiler, not at
// offset 0: measured with the settled toolchain, `class N { virtual ~N(); int
// a,b,c,m; }` puts a,b,c,m at 0,4,8,12 and the vptr at 16.  So VISound's vptr
// at +12 says the class carries exactly 12 bytes of data ahead of it -- an
// 8-byte non-polymorphic VIResource base plus its own m_type -- and it is
// VISound, not VIResource, that introduces the vtable.
//
// The RTTI name strings in the image spell the chain out: "6VIWave",
// "7VISound", "10VIResource" (0x004EBF18) and "4VIXm", "7VISound",
// "10VIResource" (0x004EBF90).  __ti6VIWave and __ti7VISound are 16 bytes (the
// single-inheritance __rttiSi form, which carries a base pointer);
// __ti10VIResource is 8 (the __rttiUser form, no base).
//
// A vtable entry is 8 bytes -- `{short delta; short index; void (*pfn)();}` --
// and entry 0 is the header, so the Nth virtual declared sits at 8*N.  Every
// vtable in this family is 64 bytes, i.e. 7 virtual slots, which is what caps
// the set below: VIXm::SetRepeat is NOT virtual, or there would be eight.
// The slots were read off the call sites:
//   1  ~VISound        VISoundDevice::ReleaseSound
//   2  Volume          result feeds `mul.s`      (float)
//   3  SetVolume       $f12 = 1.0f in VIWave::Create
//   4  Pan             result stored with swc1   (float)
//   5  SetPan          $f12 = 0.0f in VIWave::Create
//   6  IsCached        `beqz v0` gate in VISoundDevice::Play
//   7  ChannelCount
// None of the six accessors has an out-of-line VISound body anywhere in the
// image, and none can be inline (an inline one would leave a copy in the TU
// that emits the vtable) -- so they are PURE virtual and VISound is abstract.

class VIWave;
class VIXm;
class VISoundDevice;

// +0 is the dictionary id: VIDictionary::Add(VIResource &r, unsigned id, ...)
// opens with `sw a2,0(a1)`.  Both derived constructors zero +0 and +4 with no
// call, so the constructor is inline and there are exactly two words.
class VIResource
{
public:
    unsigned int m_id;
    int          m_link;

    VIResource() { m_id = m_link = 0; }
};

class VISound : public VIResource
{
public:
    enum TYPE { WAVE, XM };

    TYPE m_type;

    virtual ~VISound();

    TYPE    Type() const;
    VIWave *AsWave();
    VIXm   *AsXm();

    virtual float Volume() const = 0;
    virtual void  SetVolume(float v) = 0;
    virtual float Pan() const = 0;
    virtual void  SetPan(float p) = 0;
    virtual int   IsCached() = 0;

    // NOT virtual, and the vtable's own size is what says so.  A gcc 2.x
    // vtable ends with a NULL entry, so B bytes hold B/8 - 2 virtuals; all
    // three of these vtables are 64 bytes, i.e. SIX.  The dispatch sites in
    // VISoundDevice confirm it -- they reach slots 1..6 and no further, and
    // the pfn is read at 8N+4 (the `lh` at 8N is the entry's delta), so
    // `lw v1,52(vptr)` in Play is virtual SIX, IsCached, the last one there is.
    int   ChannelCount() const;
};

// The sound-class volume table is two floats and Play refuses a class of 2 or
// more, so there are exactly two.  Class 1 is the music bus: StreamBgm scales
// the BGM by m_classVolume[1].
enum VISOUNDCLASS
{
    VISOUNDCLASS_SFX   = 0,
    VISOUNDCLASS_MUSIC = 1
};

// 56 bytes -- the constructor's array loop strides 0x38 and calls VILNode's
// constructor directly, so VIPlayback derives from VILNode and declares no
// constructor of its own.  m_channels is a 64-bit mask (46 SPU voices), read
// and written with ld/sd throughout.
class VIPlayback : public VILNode
{
public:
    unsigned long m_channels;   // +0x10
    int           m_priority;   // +0x18
    int           m_sound;      // +0x1C
    int           m_handle;     // +0x20  slot | (serial << 8)
    int           m_pitch;      // +0x24
    float         m_volume;     // +0x28
    float         m_pan;        // +0x2C
    int           m_frame;      // +0x30  VISoundDevice::m_frame when started
    VISOUNDCLASS  m_class;      // +0x34
};

// 24 bytes: the free-list node of both the SPU and the XM allocator.  Both
// pools are 128 of these in one `new VISpuBlock[128]` (3072 bytes, measured off
// the __builtin_vec_new argument), split between a list of free MEMORY and a
// list of free NODES.
class VISpuBlock : public VILNode
{
public:
    unsigned int m_addr;        // +0x10
    int          m_size;        // +0x14
};

// 0x117C+ bytes.  The EE half of the sound system: it owns the playback slots,
// the loaded VISounds, two first-fit allocators over IOP memory, one DMA
// transfer in flight, and the BGM state machine -- and it says nothing to the
// IOP directly.  Every command is appended to a packet (SoundCmd) that
// SoundFlush ships over SIF RPC, which is why the class has no sceSpu calls in
// it at all.
class VISoundDevice
{
public:
    unsigned long m_channels;            // +0x0000  voices in use
    VIPlayback    m_playbacks[46];       // +0x0008
    VISound      *m_sounds[128];         // +0x0A18
    int           m_frame;               // +0x0C18
    VIPlayback   *m_playbackSlots[46];   // +0x0C1C
    VILList       m_activePlaybacks;     // +0x0CD4
    VILList       m_freePlaybacks;       // +0x0CE0
    int           m_handleSeed;          // +0x0CEC  0x16062, as VISpellEffectSystem
    VILList       m_spuMem;              // +0x0CF0
    VILList       m_spuNodes;            // +0x0CFC
    VISpuBlock   *m_spuBlocks;           // +0x0D08
    VILList       m_xmMem;               // +0x0D0C
    VILList       m_xmNodes;             // +0x0D18
    VISpuBlock   *m_xmBlocks;            // +0x0D24
    int           m_iopBuf;              // +0x0D28  IOP bounce buffer
    unsigned int  m_xmBase;              // +0x0D2C
    int           m_xmSize;              // +0x0D30
    void         *m_xferSrc;             // +0x0D34
    int           m_xferRemain;          // +0x0D38
    int           m_xferSpuAddr;         // +0x0D3C  -1 for an XM module
    int           m_xferXmAddr;          // +0x0D40  -1 for a sample
    int           m_xferXmSpu;           // +0x0D44
    int           m_xferSampleSpu;       // +0x0D48
    int           m_xferLast;            // +0x0D4C
    int           m_xferDma;             // +0x0D50
    int           m_xferIdle;            // +0x0D54
    int          *m_xferFlag;            // +0x0D58
    void         *m_xferFree;            // +0x0D5C
    float         m_classVolume[2];      // +0x0D60
    int           m_stereo;              // +0x0D68
    char          m_bgmRequest[2][256];  // +0x0D6C
    char          m_bgmPlaying[256];     // +0x0F6C
    char          m_bgmPending[256];     // +0x106C
    int           m_bgmFadeDir;          // +0x116C  -1 out, 1 in, 0 idle
    float         m_bgmFade;             // +0x1170
    float         m_bgmVolume;           // +0x1174
    unsigned int  m_bgmAddr;             // +0x1178

    VISoundDevice();
    ~VISoundDevice();

    static void SetBgmVersion(int v);
    static int  BgmVersion();
    // NOT static: every call site passes `this` in a0.
    int         BgmBufferSize();

    int   Init();
    int   Reset();
    int   Close();
    int   Process();

    int   CreateSound(VISound::TYPE type, int share);
    VIXm *Sound(int i);
    int   ShareSound(int i);
    int   ReleaseSound(int i, VIDictionary &d);

    int   PlayBgm(int slot, const char *path);
    void  StopBgm(int slot);
    void  StopAllBgm();
    int   IsBgmPlaying(int slot);
    void  SetBgmVolume(int slot, float v);
    void  EnableBgmFading(int on);
    int   StreamBgm();

    int   Play(int sound, int priority, VISOUNDCLASS cls);
    int   IsPlaying(int handle);
    void  Stop(int handle);
    int   Pitch(int handle);
    void  SetPitch(int handle, int pitch);
    float Volume(int handle);
    void  SetVolume(int handle, float v);
    float Pan(int handle);
    void  SetPan(int handle, float p);
    int   Flush();
    void  SetSoundClassVolume(VISOUNDCLASS cls, float v);
    void  SetOutputMode(int stereo);

    int   CreateDevice();
    int   ReleaseDevice();

    unsigned long AllocateChannels(int count, int priority);
    void          DeallocateChannels(unsigned long mask);
    int           ChannelCount(unsigned long mask);
    void          StopChannels(unsigned long mask);
    int           FirstChannel(unsigned long mask);
    int           NextChannel(unsigned long mask, int channel);

    VIPlayback *NewPlayback();
    void        DeletePlayback(VIPlayback *p);
    VIPlayback *Playback(int handle);
    VIPlayback *FirstPlayback();
    VIPlayback *NextPlayback(VIPlayback *p);

    int          InitSpuMem();
    void         CloseSpuMem();
    unsigned int AllocSpuMem(int size);
    void         FreeSpuMem(unsigned int addr, int size);
    void         MergeFreeSpuMem();

    int          InitXmMem(unsigned int addr, int size);
    void         CloseXmMem();
    unsigned int AllocXmMem(int size);
    void         FreeXmMem(unsigned int addr, int size);
    void         MergeFreeXmMem();

    int   BeginAdpcmTransfer(void **src, unsigned int spuAddr, int size, int *flag);
    int   BeginXmTransfer(void **src, unsigned int xmAddr, unsigned int sampleAddr,
                          int size, int *flag);
    int   IdleTransfer();
    int   DebugResources();

    static int sBgmVersion;
};
class VIWave : public VISound
{
public:
    int            m_format;
    int            m_channelCount;
    int            m_sampleSize;
    int            m_sampleFrameCount;
    int            m_sampleRate;
    int            m_loop;
    int            m_cached;
    float          m_volume;
    float          m_pan;
    unsigned char *m_buffer;
    int            m_size;
    unsigned int   m_spuAddr;
    int            m_pad64;
    int            m_p68;
    int            m_pad72;
    int            m_p76;
    int            m_p80;
    int            m_p84;
    VISoundDevice *m_device;

    VIWave();
    virtual ~VIWave();

    int  Create(VISoundDevice &dev, int channels, int format, int frames, int loop);
    int  Destroy();
    int  Format() const;
    int  SampleSize() const;
    int  SampleFrameCount() const;
    int  LockBuffer(unsigned char **p);
    int  UnlockBuffer();
    int  SampleRate() const;
    void SetSampleRate(int rate);
    int  Allocate(VISoundDevice &dev);
    int  Deallocate();

    virtual float Volume() const;
    virtual void  SetVolume(float v);
    virtual float Pan() const;
    virtual void  SetPan(float p);
    virtual int   IsCached();

    int   ChannelCount() const;
};

// VIXm::ChannelCount reads a SHORT at +8 of whatever m_xm points at.
struct VIXmHeader
{
    int   m_pad0;
    int   m_pad4;
    short m_channelCount;
};

class VIXm : public VISound
{
public:
    VISoundDevice *m_device;
    unsigned int   m_xmAddr;
    unsigned int   m_spuAddr;
    int            m_xmCached;    // +28
    int            m_sampleCached; // +32
    VIXmHeader    *m_xm;
    int            m_spuSize;
    void          *m_sample;
    int            m_xmSize;
    int            m_repeat;
    float          m_volume;
    float          m_pan;

    VIXm();
    virtual ~VIXm();

    int  Create(VISoundDevice &dev, void *xm, int spuSize, void *sample, int xmSize);
    void Destroy();
    void SetRepeat(int r);

    virtual float Volume() const;
    virtual void  SetVolume(float v);
    virtual float Pan() const;
    virtual void  SetPan(float p);
    virtual int   IsCached();

    int   ChannelCount() const;
};


// ---------------------------------------------------------------------------
// VIWalkControl -- SUPPORT 0x00484930 .. 0x00487BB0.  Tier 2: the simulation
// layer that turns a stick sample into a position.
//
// THE LAYOUT IS MEASURED, not transcribed.  go-eqoa-vi/viwalk ports 24 of the
// 33 methods bit-exact against the console, so every field below is justified
// by a reader, and claude-notes/movement-system.md's table is wrong in four
// places (+0x08 is 0.5 not 0.02, +0x0C/+0x20 are 3.0 not 0.01, the pitch limit
// is +-1.5607964 and the locomotion state is at +0x100, not +0x118).
//
// The eighteen VIVect3 members are exactly the offsets the constructor zeroes
// (0x74 0x80 0x8C 0x98 0xB0 0xBC 0xC8 0xD4 0xE0 0xEC 0x10C 0x138 0x144 0x150
// 0x15C 0x168 0x174 0x194); every gap between them is a member of some other
// type, which is what makes the padding fields below evidence rather than
// guesses.
class VIScene;

class VIWalkControl
{
public:
    int     m_tickMS;          // +0x000
    int     m_tickAltMS;       // +0x004
    float   m_speedThreshold;  // +0x008
    float   m_walkSpeed;       // +0x00C
    float   m_runSpeed;        // +0x010
    float   m_parm14;          // +0x014
    float   m_turnRate;        // +0x018
    float   m_pitchRate;       // +0x01C
    float   m_swimSpeed;       // +0x020
    float   m_parm24;          // +0x024
    float   m_parm28;          // +0x028
    float   m_parm2C;          // +0x02C
    float   m_parm30;          // +0x030
    float   m_parm34;          // +0x034
    float   m_parm38;          // +0x038
    float   m_parm3C;          // +0x03C
    float   m_moveDamping;     // +0x040
    float   m_parm44;          // +0x044  the AIR drag
    float   m_swimDamping;     // +0x048
    float   m_turnDamping;     // +0x04C
    float   m_pitchDamping;    // +0x050
    float   m_parm54;          // +0x054
    float   m_moveDeadzone;    // +0x058
    float   m_runThreshold;    // +0x05C
    int     m_parm60;          // +0x060  gravity on/off
    int     m_parm64;          // +0x064
    float   m_minPitch;        // +0x068
    float   m_maxPitch;        // +0x06C
    float   m_autoPitchRate;   // +0x070

    VIVect3 m_forward;         // +0x074  heading only, no pitch
    VIVect3 m_right;           // +0x080
    VIVect3 m_facing;          // +0x08C  heading and pitch
    VIVect3 m_up;              // +0x098  NOT normalized

    float   m_pA4;             // +0x0A4  twelve bytes the ctor does not zero,
    float   m_pA8;             // +0x0A8  so three members that are not a
    float   m_pAC;             // +0x0AC  VIVect3

    VIVect3 m_position;        // +0x0B0
    VIVect3 m_angles;          // +0x0BC  (heading, pitch, roll)
    VIVect3 m_velocity;        // +0x0C8
    VIVect3 m_angularVel;      // +0x0D4
    VIVect3 m_force;           // +0x0E0
    VIVect3 m_torque;          // +0x0EC

    long    m_flags;           // +0x0F8  `ld` at four sites, so eight bytes
    int     m_locoState;       // +0x100
    float   m_field104;        // +0x104
    float   m_targetHeading;   // +0x108
    VIVect3 m_v10C;            // +0x10C
    int     m_parm118;         // +0x118
    float   m_walkAccel;       // +0x11C
    float   m_runAccel;        // +0x120
    float   m_turnAccel;       // +0x124
    float   m_pitchAccel;      // +0x128
    float   m_swimAccel;       // +0x12C
    int     m_p130;            // +0x130
    int     m_p134;            // +0x134
    VIVect3 m_prevPosition;    // +0x138
    // +0x144, and its .y IS the alternate pitch go-eqoa-vi/viwalk calls
    // PitchAlt (+0x148).  It is a VIVect3 and not three floats: the
    // constructor zeroes it through its own base register, which is what
    // VIVect3::Zero() gives and three plain members do not -- so the pitch
    // limit lives in the middle of a second angles triple.
    VIVect3 m_anglesAlt;       // +0x144
    VIVect3 m_v150;            // +0x150
    VIVect3 m_v15C;            // +0x15C
    VIVect3 m_v168;            // +0x168
    VIVect3 m_v174;            // +0x174
    int     m_flags2;          // +0x180  a SECOND flag word, same masks
    int     m_p184;            // +0x184
    int     m_moveMode;        // +0x188
    float   m_field18C;        // +0x18C
    // SetCameraMode stores VIWalkCamera's alternate heading here with swc1,
    // so it is a float, not the int Phase 18 left it as.
    float   m_p190;            // +0x190
    VIVect3 m_landscapeNormal; // +0x194

    // The two gravities, SUPPORT 0x004E31F8 and 0x004E3208.  Each is a
    // VIVect3 with x and z zero, so gravity is straight down in world space
    // and never a function of the character's orientation.  UNDER a liquid it
    // is -1.0 rather than -19.6 -- a twentieth of the pull -- and AT a liquid
    // surface there is no third vector at all: the whole addition is skipped,
    // so a swimmer at the surface is weightless rather than lightly weighted.
    // They are static MEMBERS, which is what _GLOBAL_$I$_13VIWalkControl$Gravity
    // is the constructor run for.
    static VIVect3 Gravity;
    static VIVect3 WGravity;

    VIWalkControl();
    ~VIWalkControl();

    int   ClearForces();
    int   InitParms();
    int   InitIntermediate();
    int   SetLocation(const VIVect3 &pos, const VIVect3 &angles, VIScene &s);
    int   Process(int flags, VIScene &s);
    int   UpdateDirections();
    int   Walk(float x, float y);
    int   Run(float x, float y);
    int   Move(float x, float y);
    int   SwimUp(float t);
    int   TurnLeft(float t);
    int   PitchDown(float t);
    int   SetTargetHeading(float h);
    int   SetPitchRange(float lo, float hi);
    int   AutoPitch(float dt);
    int   ViewRelativeMove(const VIVect3 &ref, float x, float y);
    int   ViewRelativeWalk(const VIVect3 &ref, float x, float y);
    int   UpdateLocomotion(float dt);
    int   UpdatePhysics(float dt);
    int   CalcPitchRollToLandscape(VIVect3 &v) const;
    int   IsUnderWater();
    int   IsUnderLiquid();
    int   IsAtLiquidSurface();
    int   IsInLava();
    int   IsInSlime();
    int   IsInMud();
};


// ---------------------------------------------------------------------------
// VISprite -- SUPPORT 0x0047D130 ..  The base of the scene's renderable leaf
// hierarchy, and the class that settles the polymorphic ABI at a real depth.
//
// TEN virtuals, and the vtable's own SIZE is what says so.  A gcc 2.x vtable
// ends with a NULL entry, so B bytes hold B/8 - 2 virtuals; _vt$8VISprite runs
// 0x004EBAE0..0x004EBB40 (the RTTI name string "8VISprite" starts exactly
// there), i.e. 96 bytes, i.e. ten.  The order is confirmed independently by
// the console: go-eqoa-vi/vicoll reads Collide(sphere) out of the dispatch
// table at sprite+0x3C entry 4 and Collide(ray) out of entry 5, and this
// declaration order puts them there.  Note the pfn is at 8N+4 -- the `lh` at
// 8N is the entry's this-delta -- so a call site's `lw v1,52(vptr)` is virtual
// SIX, not seven.
//
//   +0x00  VIResource   m_id / m_link
//   +0x08  VIBBox       m_bbox      GetBBox returns a0+8
//   +0x20  VISphere     m_bsphere   GetBSphere returns a0+32
//   +0x30  float        m_offDist   GetOffDist; 500.0f from the constructor
//   +0x34  int          m_type      GetType; go-eqoa-vi's VISpriteType
//   +0x38  int          m_actor     -1, and Copy does NOT carry it over
//   +0x3C  vptr
//
// The constructor zeroes the bounding volumes TWICE each -- the members' own
// constructors, then the sub-object constructor bodies inlined -- which is the
// same shape VICollBuffer and VIPrimBuffer showed, and the reason VISphere
// needs a zeroing default constructor of its own.
// ---------------------------------------------------------------------------
// VIEmitterQuery -- the collector VISprite::QueryEmitters feeds.  Its layout
// comes straight out of that one function: +0 is compared against the sprite's
// m_type, +4 bounds the map's size, +8 and +12 are copied verbatim into every
// entry, and the map itself starts at +16.
//
// The entry itself is just the two words: QueryEmitters' local sits at sp+16
// and only +0 and +4 are ever written, and the 64-byte matrix beside it at
// sp+32 is a separate local (as a member it scores worse, and its own
// destructor is called on sp+32 rather than on the entry).
// ------------------------------------------------------------------ VIPool --
// The three VALUE element types phase 34 added, each modelled from its pool's
// own two loops the way phase 24 modelled VIArray's: the node stride gives the
// size and the copy gives the alignment.  VIEEBucket and VIRasterMaterialLRU
// are 16 bytes copied with `ldl`/`ldr`, so four-byte aligned and POD;
// VILoaderTrans is 48 copied with `ld`/`sd`, so it carries a 64-bit field.
// Phase 40 named the fields: it is VIEESurfCache's VRAM run.
class VIEEBucket
{
public:
    int m_addr;   // +0x00
    int m_size;   // +0x04
    int m_psm;    // +0x08  a GS PSM, or 255 for a run never handed out
    int m_free;   // +0x0C  its handle in VIEESurfCache::m_freeMap, -1 if used
};

class VIRasterMaterialLRU
{
public:
    int m_a, m_b, m_c, m_d;
};

// Phase 41 named the fields.  The three the caller does not fill are what
// VILoader::Align computes: it rounds m_position DOWN and m_readSize UP to the
// file's own asynchronous alignment and carries the slack in m_offset, so the
// object lands somewhere inside a sector-aligned read.  44 bytes of fields,
// padded to 48 by the `unsigned long` at +0x10.
class VILoaderTrans
{
public:
    int           m_state;       // +0x00  0 queued, 1 reading, 2 read, 3 done
    int           m_refCount;    // +0x04
    unsigned int  m_id;          // +0x08  DictID, 0 for an anonymous load
    int           m_file;        // +0x0C  index into VILoader::m_files
    unsigned long m_position;    // +0x10  file offset, rounded DOWN by Align
    unsigned int  m_size;        // +0x18  the object's own size
    unsigned int  m_offset;      // +0x1C  its offset inside the buffer
    unsigned int  m_readSize;    // +0x20  the aligned read
    unsigned int  m_readOffset;  // +0x24  where in the buffer it lands
    int           m_zone;        // +0x28  -1 for a plain resource
};

// The engine's intrusive free-list pool.  Definitions and the nineteen
// explicit instantiations live in vipool.cpp; the declaration is here because
// VICollide has one as a member.
template <class T>
class VIPool
{
public:
    struct Node
    {
        T   m_data;
        int m_next;
        int m_prev;
    };

    int   m_size;       // +0x00
    int   m_capacity;   // +0x04
    Node *m_nodes;      // +0x08
    int   m_free;       // +0x0C
    int   m_first;      // +0x10
    int   m_last;       // +0x14

    VIPool();
    ~VIPool();

    // Inline in the class body, the VISet<T>::Value lever of Phase 27: the
    // motif walks in VIParticleDefinitionEx read the element with `this` still
    // on the POOL's own register, which a written-out m_nodes[i].m_data cannot
    // do -- it computes the address into the local and copies it to a0, where
    // the inline's own `this` pseudo is a COPY of the argument temp.
    T &Value(int i) const { return m_nodes[i].m_data; }

    int Size() const;
    int End() const;
    int Begin() const;
    int Next(int i) const;
    int Prev(int i) const;
    int Clear();
    int DestroyPool();
    int LinkFreeChain(int i);
    int Allocate();
    int Add();
    int Add(const T &v);
    int Insert(const T &v, int i);
    int MoveToLast(int i);
    int Erase(int i);
    int Init(int n);
    int Init(const VIPool<T> &a);
};


// ------------------------------------------------------------- VIMap<K,V> --
// VIMultiMap with a Find, and the base of VIDictionary.  Four instantiations.
template <class K, class V>
class VIMap
{
public:
    struct Node
    {
        int m_color;
        int m_parent;
        int m_left;
        int m_right;
        K   m_key;
        V   m_value;
    };

    int   m_size;       // +0x00
    int   m_capacity;   // +0x04
    Node *m_nodes;      // +0x08
    int   m_root;       // +0x0C
    int   m_free;       // +0x10

    VIMap();
    ~VIMap();

    // Inline in the class body, as VIMultiMap's is: no instantiation has a
    // symbol for it, and VIEESurfCache's LRU surgery keeps `this` on the
    // MAP's own register across a dozen node reads, which a written-out
    // m_nodes[i].m_value cannot give.
    V &Value(int i) const { return m_nodes[i].m_value; }

    int Size() const;
    int End() const;
    int Begin() const;
    int Next(int i) const;
    const K &Key(int i) const;
    int Find(const K &k) const;
    int DestroyPool();
    int LinkFreeChain(int i);
    int Allocate();
    int Clear();
    int Init(int n);
    int FreeNode(int i);
    int AddNode(int p, int side);
    int LeftRotate(int i);
    int RightRotate(int i);
    int Insert(const K &k, const V &v);
    int EraseFixup(int i);
    int Erase(int i);
};

// Phase 54 names the rest of it out of VIScene::Share/ReleaseResource, whose
// switch has a 26-entry jump table -- read out of the LIVE dump, because the
// table is a relocation and the un-relocated image holds zeroes.  Fourteen of
// the entries go to the sprite arm; 18 and 21 fall through to the default, so
// they are types the scene does not own.
enum VIResourceType
{
    VIRT_0,
    // VICSpriteCust::GetFaceTexture passes 1.
    VIRT_Surface = 1,
    VIRT_Material,
    VIRT_MaterialPal,
    VIRT_Sprite,
    VIRT_5,
    VIRT_Animation,
    VIRT_7,
    VIRT_RefMap,
    VIRT_9,
    VIRT_CollBuffer,
    VIRT_PrimBuffer,
    // VIFont's five lookups all pass 12 to VIDictionary::FindTyped.
    VIRT_Font = 12,
    VIRT_13,
    VIRT_Sound,
    VIRT_15,
    VIRT_16,
    VIRT_17,
    VIRT_18,
    VIRT_19,
    VIRT_20,
    VIRT_21,
    VIRT_StaticLighting,
    VIRT_23,
    VIRT_24,
    VIRT_25,
    VIRT_26
};

// -------------------------------------------------------- VIEESurfCache --
// The GS VRAM allocator, and the EE half of every texture upload.  Phase 40.
//
// A bucket is one run of video memory: address, byte size, the GS PIXEL
// STORAGE MODE it is currently formatted for, and its handle in the free map.
// m_psm is a real GS PSM -- 0 CT32, 1 CT24, 2 CT16, 19 T8, 20 T4 -- and 255
// means "virgin", a run that has never been handed out and so may be
// reformatted to anything.  That is why the free map is keyed on
// `(psm << 24) | size`: a LowerBound on (psm, size) finds the smallest block
// already in the right format, and a second LowerBound on (255, size) falls
// back to untouched memory.  Everything about the page arithmetic follows
// from the same rule -- a GS page is 8192 bytes and its pixel layout depends
// on the psm, so a run may only change format on page boundaries.
//
// 12 bytes: Deallocate, Allocate and FindFree touch +0, +4 and +8 and nothing
// else, and VIEESurfCacheInfo's second alloc sits 16 bytes after its first
// with a fourth word between them.
class VIEESurfAlloc
{
public:
    int m_bucket;   // +0x00  index into VIEESurfCache::m_buckets
    int m_psm;      // +0x04
    int m_size;     // +0x08  bytes
};

// 84 bytes, alignment 4: VIMap<int,VIEESurfCacheInfo>'s node is 104 with the
// key at +16 and the value at +20, and Insert copies it with gcc's
// runtime-alignment-checked block move rather than by pieces.  The key is the
// surface's own VISurface::m_index.
class VIEESurfCacheInfo
{
public:
    VIEESurfAlloc m_clut;         // +0x00
    int           m_clutAddr;     // +0x0C  GS address of the palette
    VIEESurfAlloc m_tex;          // +0x10
    int           m_numLevels;    // +0x1C  mip levels actually resident
    int           m_levelAddr[10];// +0x20  GS address of each level
    int           m_lruNext;      // +0x48
    int           m_lruPrev;      // +0x4C
    int           m_stamp;        // +0x50  copied into VIMaterialPass::m_stamp
};

// 160 bytes with a destructor, read off VIList<VIActor>: its node is 176 with
// the two links at +160/+164, and Clear calls `~VIActor(n, 2)` before the
// node's own `operator delete`.
//
// Phase 41 fills it in off the constructor, which is nothing but sub-object
// constructors and a run of constant stores, and off CalcObjectToWorld, which
// is the whole point of the class: `m_objectToWorld.Transform(m_hpr, m_scale,
// m_location)` -- the ROTATION is at +0x50 and the translation at +0x44, which
// only the argument registers say.  The VIMatrix44 and the VISphere are each zeroed TWICE (the
// member's own constructor, then the class body's) exactly as VISprite's are.
class VIActor
{
public:
    VIMatrix44 m_objectToWorld;  // +0x00
    int        m_p40;            // +0x40  0
    VIVect3    m_location;       // +0x44
    VIVect3    m_hpr;            // +0x50
    float      m_scale;          // +0x5C  NOT initialised
    int        m_p60;            // +0x60  -1
    int        m_p64;            // +0x64  0
    int        m_p68;            // +0x68  -1
    int        m_p6C;            // +0x6C  NOT initialised
    // Phase 54: a COLOUR.  CreateStaticProxyActor fills it from a VIColor32
    // scaled by 1/255 and VIScene::RasterActor hands it to
    // VIRaster::SetStaticLighting.
    VIColor32F m_p70;            // +0x70
    int        m_p80;            // +0x80  0
    int        m_p84;            // +0x84  0
    VISphere   m_bound;          // +0x88
    int        m_p98;            // +0x98  -1
    int        m_p9C;            // +0x9C  -1

    VIActor();
    ~VIActor();

    void CalcObjectToWorld();
};

// -------------------------------------------------------------- VIList<T> --
// The engine's SIXTH template: a heap-allocated doubly-linked list whose
// iterator is a Node POINTER carried in an `int`, so the interface matches
// VIPool's and VISet's while the storage does not.  Every node is its own
// `new`, so there is no capacity, no free chain and no Allocate.
//
//     +0x00 m_size   +0x04 m_head   +0x08 m_tail
//     Node { T m_data; Node *m_next; Node *m_prev; }
//
// End() is 0 -- Erase CALLS it rather than materialising a constant.
template <class T>
class VIList
{
public:
    struct Node
    {
        T     m_data;
        Node *m_next;
        Node *m_prev;
    };

    int   m_size;   // +0x00
    Node *m_head;   // +0x04
    Node *m_tail;   // +0x08

    VIList();
    ~VIList();

    int Size() const;
    int End() const;
    int Begin() const;
    int Next(int i) const;
    int Clear();
    int Init();
    int Add();
    int Add(const T &v);
    int Insert(const T &v, int i);
    int MoveToLast(int i);
    int Erase(int i);
};

// 80 bytes, alignment 16: VIMultiMap<float,VIEmitterEntry>'s node is 112 with
// the key at +16 and the value at +32, and Insert copies the value as two
// `sw`s plus a `jal` on (dst+16, src+16) -- the implicit copy constructor
// running VIMatrix44's over a member at +16.  The two-word guess this
// replaced could not account for any of that.
class VIEmitterEntry
{
public:
    int        m_a;
    int        m_b;
    VIMatrix44 m_matrix;   // +16: VIMatrix44 is aligned(16), and the eight
                           // bytes of padding are NOT copied by the implicit
                           // operator=, which is how they are known to be
                           // padding rather than members
};

// ------------------------------------------------------- VISet / VIMultiMap --
// The engine's FOURTH and FIFTH templates, and its only ordered containers: a
// red-black tree laid over VIPool's intrusive free list.  Nodes live in one
// `new char[]` block and are named by INDEX, never by pointer, so the whole
// tree survives a reallocation -- and index 0 is the NIL sentinel, which is
// why AddNode's first growth step is to TWO slots and not one.
//
//     +0x00 m_size       live element count, the sentinel included
//     +0x04 m_capacity   node slots allocated
//     +0x08 m_nodes      Node[]
//     +0x0C m_root       0 when the tree is empty
//     +0x10 m_free       head of the free chain, -1 when exhausted
//
//     Node { int m_color; int m_parent; int m_left; int m_right; <payload>; }
//
// m_color is 1 for RED and 0 for BLACK; a free node re-uses m_parent as the
// chain link and carries -2 in m_left, exactly as VIPool marks m_prev.
// End() is 0 here, NOT VIPool's 0x7FFFFFFF -- the sentinel index is the
// past-the-end iterator, so Begin() on an empty tree and End() agree by
// construction.
template <class T>
class VISet
{
public:
    struct Node
    {
        int m_color;
        int m_parent;
        int m_left;
        int m_right;
        T   m_data;
    };

    int   m_size;       // +0x00
    int   m_capacity;   // +0x04
    Node *m_nodes;      // +0x08
    int   m_root;       // +0x0C
    int   m_free;       // +0x10

    VISet();
    ~VISet();

    // Inline in the class body, like VIMultiMap's: VIWnd's destructor and
    // OnDraw read the element off the node array with `this` still on the
    // VISet's own register, which a written-out `m_nodes[i].m_data` cannot do.
    T &Value(int i) { return m_nodes[i].m_data; }

    int Size() const;
    int End() const;
    int Begin() const;
    int Next(int i) const;
    int Find(const T &v) const;
    int DestroyPool();
    int LinkFreeChain(int i);
    int Allocate();
    int Clear();
    int Init(int n);
    int FreeNode(int i);
    int AddNode(int p, int side);
    int LeftRotate(int i);
    int RightRotate(int i);
    int Insert(const T &v);
    int EraseFixup(int i);
    int Erase(int i);
};


// The same tree with the key split out of the payload.  Everything above
// applies; the Node carries K then V after the four link words.
template <class K, class V>
class VIMultiMap
{
public:
    struct Node
    {
        int m_color;
        int m_parent;
        int m_left;
        int m_right;
        K   m_key;
        V   m_value;
    };

    int   m_size;       // +0x00
    int   m_capacity;   // +0x04
    Node *m_nodes;      // +0x08
    int   m_root;       // +0x0C
    int   m_free;       // +0x10

    VIMultiMap();
    ~VIMultiMap();

    // Inline in the class body: no symbol exists for it in any instantiation,
    // and VISceneQuery::EndQuery needs `this` to be the SAME pseudo the three
    // walk calls take, which a `m_nodes[i].m_value` written out cannot give.
    V &Value(int i) { return m_nodes[i].m_value; }

    int Size() const;
    int End() const;
    int Begin() const;
    int Next(int i) const;
    // Phase 54, off VIScene::QueryProximalActor: the walk runs backwards too,
    // to drop the farthest entry once the query is full.
    int Prev(int i) const;
    int Last();
    int DestroyPool();
    int LinkFreeChain(int i);
    int Allocate();
    int Clear();
    int Init(int n);
    int FreeNode(int i);
    int AddNode(int p, int side);
    int LeftRotate(int i);
    int RightRotate(int i);
    int Insert(const K &k, const V &v);
    int EraseFixup(int i);
    int Erase(int i);
    // Phase 40, for VIEESurfCache::FindFree.  Key returns a REFERENCE (the
    // last instruction is an `addiu`, not a load) -- FindFree reads BYTE 3 of
    // what comes back, which no by-value int return could give it.
    const K &Key(int i) const;
    int LowerBound(const K &k) const;
};

// The 16-byte DMA tag the packet builder walks.  Only three fields are ever
// written -- the id BYTE (bits 24..31 of the tag word, so `sb` at +3), the
// address and the quadword count -- and every write re-reads `*tag`, because
// a store through the packet may alias the pointer itself.
// One GS A+D pair: the value and the register number it goes to.
struct VIGifAD
{
    long m_value;
    long m_reg;
};

// What VIEESurfCache::ConstructDMAPacket writes after each CNT tag: one VIF
// quadword (four VIFcodes, executed word 0 first, so DIRECT sits in word 3),
// a GIFtag, and three A+D pairs.  Read off the walking base gcc builds for
// m_ad -- the target addresses ad[0] at +32 off this struct and reaches ad[1]
// and ad[2] by adding 16.
struct VIEEXferPacket
{
    unsigned int m_vif[4];   // +0x00
    long         m_gif;      // +0x10
    long         m_regs;     // +0x18
    VIGifAD      m_ad[3];    // +0x20
};

struct _sceDmaTag
{
    unsigned short m_qwc;    // +0x00
    unsigned char  m_pad2;   // +0x02
    unsigned char  m_id;     // +0x03
    unsigned int   m_addr;   // +0x04
    unsigned int   m_pad8;
    unsigned int   m_padC;
};

// The cache itself, VIRaster's texture manager: three containers over one
// span of GS memory, plus an LRU over the resident surfaces so a miss can
// evict.  Init is handed the base and the end of the VRAM span it owns.
class VIEESurfCache
{
public:
    // The stamp counter Download hands out.  A running int, not a table: the
    // 1112-byte span the map derives for it is the gap to the next symbol.
    static int Key;

    int m_base;         // +0x00
    int m_end;          // +0x04
    int m_lruFirst;     // +0x08
    int m_lruLast;      // +0x0C
    int m_noDownloads;  // +0x10  EnableDownloads stores the COMPLEMENT
    int m_used;         // +0x14
    int m_size;         // +0x18  the page-aligned span Clear hands out
    int m_numDownloads; // +0x1C

    VIPool<VIEEBucket>                 m_buckets;   // +0x20
    VIMap<int, VIEESurfCacheInfo>      m_infos;     // +0x38
    VIMultiMap<unsigned int, int>      m_freeMap;   // +0x4C

    int Init(int base, int end);
    int Clear();
    int Cache(VISurface &s, VIMaterial &m, VIMaterialPass &p, VIRaster &r,
              _sceDmaTag **tag, int noLRU);
    int Decache(VISurface &s);
    int EnableDownloads(int on);
    int CalcCacheInfo(VISurface &s, VIEESurfCacheInfo &c);
    int Set(VISurface &s, VIEESurfCacheInfo &c, VIMaterialPass &p);
    int Log2(int n);
    int AddToFreeMap(int i);
    int RemoveFromFreeMap(int i);
    int FindLRU(VIEESurfAlloc &a, VIRaster &r);
    int FindFree(VIEESurfAlloc &a);
    int Allocate(VIEESurfAlloc &a);
    int Deallocate(VIEESurfAlloc &a);
    int MoveToLRUEnd(int i);
    int RemoveFromLRU(int i);
    int Download(VISurface &s, VIEESurfCacheInfo &c, _sceDmaTag **tag);
    int ConstructDMAPacket(VISurface &s, VIEESurfCacheInfo &c,
                           _sceDmaTag **tag);
    int DebugVerify();
};

// Read out of VIScene::CreateSprite's JUMP TABLE at 0x004EB380 and not out of
// the order its case blocks appear in -- the last two are transposed.  Indexed
// by `type - 1`, fourteen entries, and 0 is not a sprite type.
enum VISpriteType
{
    VISpriteTypeSimple      = 1,
    VISpriteTypePoint       = 2,
    VISpriteTypeSkin        = 3,
    VISpriteTypeH           = 4,
    VISpriteTypeC           = 5,
    VISpriteTypeName        = 6,
    VISpriteTypeLOD         = 7,
    VISpriteTypePointLight  = 8,
    VISpriteTypeSound       = 9,
    VISpriteTypeParticle    = 10,
    VISpriteTypeGroup       = 11,
    VISpriteTypeStreamAudio = 12,
    VISpriteTypeFlora       = 13,
    VISpriteTypeEffectVol   = 14
};

class VIEmitterQuery
{
public:
    int Init(int limit, VISpriteType type);
    int Clear();

    int m_type;
    int m_limit;
    int m_a;
    int m_b;
    VIMultiMap<float, VIEmitterEntry> m_map;
};

class VICamera;
class VISceneFilter;
class VIHSpriteAnim;
class VIQueryTriList;
class VISprite;
class VICollide;
class VICapsule;
class VICylinder;

// ---------------------------------------------------------------------------
// The particle system -- Phase 28.  SUPPORT 0x004565A0 .. 0x0045B640, in four
// translation units either side of VIParticleAttributes:
//
//   0x004565A0  VIParticleDefinition
//   0x00456910  VIParticleEmitter
//   0x00458780  VIParticleSystem  (+ VIPool<VIParticleDefinition *>)
//   0x004597C0  VIParticleAttributes                       (Phase 12)
//   0x0045A3D8  VIParticleMotif + VIParticleDefinitionEx
//                                (+ VIPool<VIParticleMotif *>)
//
// VIParticleMotif is a VIParticleAttributes with a name: `new VIParticleMotif`
// hands __builtin_new 760 = 0x2D4 + 32 + 4, and SetName is strcpy into +0x2D4
// followed by VIHashResourceID into +0x2F4.
// ---------------------------------------------------------------------------
class VIParticleMotif : public VIParticleAttributes
{
public:
    VIParticleMotif();
    ~VIParticleMotif();

    const char  *Name() const;
    void         SetName(const char *name);
    unsigned int NameHash() const;

    char         m_name[32];    // +0x2D4
    unsigned int m_nameHash;    // +0x2F4
};

// The half of a particle definition that carries no render state: the motif
// list, and the four flags the render half re-applies to its VIMaterial.  The
// motifs are a VIPool of POINTERS (node stride 12), each one `new`d.
class VIParticleDefinitionEx : public VIParticleAttributes
{
public:
    enum BLENDMODE
    {
        BLENDMODE_0 = 0,
        BLENDMODE_1 = 1,
        BLENDMODE_2 = 2,
        BLENDMODE_3 = 3,
        BLENDMODE_4 = 4
    };

    VIParticleDefinitionEx();
    ~VIParticleDefinitionEx();

    BLENDMODE BlendMode() const;
    void      SetBlendModeEx(BLENDMODE m);
    bool      ZWrite() const;
    void      SetZWriteEx(bool on);
    bool      ZTest() const;
    void      SetZTestEx(bool on);
    int       TextureConfiguration() const;
    void      SetTextureConfigurationEx(int c);

    VIParticleMotif *CreateMotif();
    void             DestroyMotif(VIParticleMotif *m);
    VIParticleMotif *FindMotif(const char *name) const;
    VIParticleMotif *FindMotif(unsigned int hash) const;
    VIParticleMotif *FirstMotif() const;
    VIParticleMotif *NextMotif(VIParticleMotif *m) const;
    int              MotifCount() const;
    void             BlendMotifs(VIParticleAttributes *out);
    void             DuplicateEx(VIParticleDefinitionEx *d) const;

    VIPool<VIParticleMotif *> m_motifs;      // +0x2D4
    BLENDMODE                 m_blendMode;   // +0x2EC
    bool                      m_zWrite;      // +0x2F0
    bool                      m_zTest;       // +0x2F4
    int                       m_texConfig;   // +0x2F8
};

// The render half: a VIMaterial of its own out of the raster, and the texture
// bound into its layer 0.  Every setter writes the flag AND re-applies it.
class VIParticleDefinition : public VIParticleDefinitionEx
{
public:
    VIParticleDefinition();
    ~VIParticleDefinition();

    int  Create(VIRaster &r);
    int  Destroy(VIRaster &r, VIDictionary &d);
    void SetBlendMode(VIParticleDefinitionEx::BLENDMODE m);
    void SetZWrite(bool on);
    void SetZTest(bool on);
    int  Texture() const;
    void SetTexture(int tex, VIRaster &r, VIDictionary &d);
    void SetTextureConfiguration(int c);

    // A VIResource MEMBER, not a base: VIParticleSystem::ReleaseParticleDefinition
    // hands `d + 0x2FC` to VIDictionary::Remove(VIResource &), and its refcount
    // is the SECOND word -- `(d + 764) + 4`.  Its inline zeroing constructor is
    // what puts the +0x300/+0x2FC pair, in that order, at the front of this
    // class's own constructor.
    VIResource m_resource;  // +0x2FC
    VIRaster  *m_raster;    // +0x304
    int        m_material;  // +0x308
    int        m_texture;   // +0x30C
};


// ---------------------------------------------------------------------------
// The particle, the emitter and the system -- Phase 28.
//
// VIParticleSystem::Init hands __builtin_vec_new `n * 104` and then runs gcc's
// own array-init loop over it: an out-of-line VILNode constructor plus fourteen
// inline `sw zero` at +0x20..+0x54.  So a particle is 104 bytes on a VILNode,
// with sixteen uninitialised bytes either side of the zeroed run.
// ---------------------------------------------------------------------------
class VIParticle : public VILNode
{
public:
    VIParticle()
    {
        m_f20 = 0;
        m_f24 = 0;
        m_f28 = 0;
        m_f2C = 0;
        m_f30 = 0;
        m_f34 = 0;
        m_f38 = 0;
        m_f3C = 0;
        m_f40 = 0;
        m_f44 = 0;
        m_f48 = 0;
        m_f4C = 0;
        m_f50 = 0;
        m_f54 = 0;
    }

    int m_pad10[4];   // +0x10
    int m_f20;        // +0x20
    int m_f24;
    int m_f28;
    int m_f2C;
    int m_f30;
    int m_f34;
    int m_f38;
    int m_f3C;
    int m_f40;
    int m_f44;
    int m_f48;
    int m_f4C;
    int m_f50;
    int m_f54;        // +0x54
    int m_pad58[4];   // +0x58, to 104
};

class VIParticleSystem;
class VIMotifBlender;

// The emitter's layout is read off its own constructor, which calls a member
// constructor for every sub-object in declaration order: VILNode, two VIVect3,
// a VIMatrix33 at +0x28, a VIMatrix44 at +0x60 (whose aligned(16) is what pads
// +0x58), two VILList, and the VIParticleAttributes at +0xBC -- 724 bytes, so
// +0x390 closes it exactly.
class VIParticleEmitter : public VILNode
{
public:
    VIParticleEmitter();
    ~VIParticleEmitter();

    int  Create(VIParticleSystem *s, VIParticleDefinition *d);
    int  Destroy();

    VIMatrix44 &Space() const;
    void  SetSpace(const VIMatrix44 &m);
    VIVect3 &Location() const;
    VIVect3 &LastLocation() const;
    void  SetLocation(const VIVect3 &v);
    VIMatrix33 &Direction() const;
    void  SetDirection(const VIVect3 &v);
    void  SetDirection(const VIMatrix33 &m);
    int   LinkMotifBlender(const char *name, VIMotifBlender &b);
    int   LinkMotifBlender(unsigned int hash, VIMotifBlender &b);
    void  UnlinkAllMotifBlenders();
    int   ParticleCount() const;
    void  SetAttractor(const VIVect3 &v, float f);
    void  ClearAttractor();
    int   Purge();
    int   Update();
    int   BlendMotifs();
    VIBBox StaticBounds() const;
    VIBBox DynamicBounds() const;
    VIBBox Bounds() const;
    int   RecomputeStaticBBox();

    VIVect3               m_location;     // +0x10
    VIVect3               m_lastLocation; // +0x1C
    VIMatrix33            m_direction;    // +0x28
    int                   m_unk4C;        // +0x4C
    VIParticleSystem     *m_system;       // +0x50
    VIParticleDefinition *m_definition;   // +0x54
    VIMatrix44            m_space;        // +0x60
    VILList               m_particles;    // +0xA0
    int                   m_numParticles; // +0xAC
    VILList               m_blenders;     // +0xB0
    VIParticleAttributes  m_attributes;   // +0xBC
    int                   m_blendVersion; // +0x390
    VIBBox                m_staticBounds; // +0x394
    VIBBox                m_dynBounds;    // +0x3AC
    VIVect3               m_attractor;    // +0x3C4
    float                 m_attractorK;   // +0x3D0
    int                   m_haveLocation; // +0x3D4
    int                   m_attractorOn;  // +0x3D8
};

// +0x00 the raster, +0x08 the 64-bit clock, +0x10 the smoothed frame delta and
// +0x14 the eight-deep history Render averages it out of, then the emitter and
// free-particle lists, the particle array and the definition pool.
class VIParticleSystem
{
public:
    VIParticleSystem();
    ~VIParticleSystem();

    int  Init(long time, VIRaster *r, int maxParticles);
    int  Clear(VIDictionary &d);
    void Purge();
    int  CreateParticleDefinition(int share);
    VIParticleDefinition *ParticleDefinition(int i);
    int  ShareParticleDefinition(int i);
    int  ReleaseParticleDefinition(int i, VIDictionary &d);
    int  ReleaseAllParticleDefinitions(VIDictionary &d);
    long Time() const;
    int  Render(long time, VIRaster &r);
    float DeltaTime() const;

    VIRaster   *m_raster;        // +0x00
    long        m_time;          // +0x08
    float       m_deltaTime;     // +0x10
    float       m_dtHistory[8];  // +0x14
    VILList     m_emitters;      // +0x34
    VILList     m_freeParticles; // +0x40
    int         m_maxParticles;  // +0x4C
    VIParticle *m_particles;     // +0x50
    VIPool<VIParticleDefinition *> m_definitions;  // +0x54
};



// The real class is at the BOTTOM of this header -- it needs VIArray, which is
// declared 1,700 lines below this point -- so VIScene carries its 976 bytes as
// a pad and the one caller (VIRadialFloraSystem::CalcZoneIndex) casts.  The
// size is not a guess: SetProfiles' last store is `+964`, inside the 22nd and
// last 40-byte terrain profile, and 96 + 22*40 is 976 exactly.
class VIWorld;
// VIScene is defined at the END of this header: it EMBEDS VIWorld, VIClipStack,
// VIParticleSystem, VIAtmosphere, VIQueryTriList, VISceneSoundTrack,
// VISpellEffectSystem and VIRadialFloraSystem by value, so every one of them
// has to be a complete type first.  Everything above reaches it through the
// forward declaration.


// ----------------------------------------------------------------- VICollide --
// SUPPORT 0x00489250 .. 0x0048DE08.  The collision world: a transform pair, a
// table of vertex-group origins for the PackVGF packing, and a pool of the
// VICollBuffers that placed objects are collided against.
//
//   +0x000 int                    m_count    a statistic; the ctor does NOT
//                                            initialise it, and 12 bytes of
//                                            padding follow it to align the
//                                            first VIMatrix44
//   +0x010 VIMatrix44             m_world    object -> world
//   +0x050 VIMatrix44             m_invWorld world -> object, DERIVED by
//                                            SetWorld and never set directly
//   +0x090 VIVect3[32]            m_preTranslations
//   +0x210 VIPool<VICollBuffer *> m_buffers  (its m_nodes is the 0x218 every
//                                            table-driven method reads)
//
// The layout is not inferred from the ctor alone: go-eqoa-vi/vicoll ports this
// class against the console and a this-offset census over all 42 methods says
// nothing outside these fields is ever touched.  Which half of the matrix pair
// is which is settled by GetWorld returning +0x10 and SetWorld running
// InvertAffine(+0x50, +0x10).
class VICollide
{
public:
    int                    m_count;
    VIMatrix44             m_world;
    VIMatrix44             m_invWorld;
    VIVect3                m_preTranslations[32];
    VIPool<VICollBuffer *> m_buffers;

    VICollide();

    int Init();
    int Close();
    int Reset();
    int Clear();

    int           CreateCollBuffer(int share);
    VICollBuffer *CollBuffer(int i);
    int           ShareCollBuffer(int i);
    int           ReleaseCollBuffer(int i);

    int SetWorld(const VIMatrix44 &m);
    const VIMatrix44 *GetWorld() const;
    int SetPreTranslations(int n, VIVect3 *v);

    int CollideObj(VICollSphere &s, int obj);
    int CollideObj(VICollSphere &s, VICapsule &c);
    int CollideObj(VICollRay &r, int obj);
    int QueryObj(const VIBBox &b, VIQueryTriList &l, int obj);

    // Phase 46 -- the collision core.  The per-primitive tests the buffer
    // walkers reach, in the target's own overload order.
    int Collide(VICollSphere &s, int idx, VIVect3 &v0, VIVect3 &v1, VIVect3 &v2);
    int Collide(VICollSphere &s, VIVect3 &v0, VIVect3 &v1);
    int Collide(VICollSphere &s, VIVect3 &v);
    int Collide(VICollRay &r, int idx, VIVect3 &v0, VIVect3 &v1, VIVect3 &v2);
    int Collide(VICollSphere &s, VISphere &v);
    int Collide(VICollSphere &s, VICapsule &c);
    int PointInTriangle(VIVect3 &p, VIPlane &pl, VIVect3 &v0, VIVect3 &v1,
                        VIVect3 &v2);
    int Collide(VICollRay &r, const VISphere &v);
    int CollideNoCaps(VICollRay &r, const VICylinder &c);

    // The three dispatchers.  Each is a switch on VICollBuffer::m_type, and
    // packing 2 never reaches one: VICollBuffer::Init rewrites a 2 to a 3.
    int Collide(VICollSphere &s, VICollBuffer &b);
    int Collide(VICollRay &r, VICollBuffer &b);
    int Query(const VIBBox &w, const VIBBox &o, VIQueryTriList &l,
              VICollBuffer &b);

    int CollideBufferV(VICollSphere &s, VICollBuffer &b);
    int CollideBufferPackV(VICollSphere &s, VICollBuffer &b);
    int CollideBufferPackVGF(VICollSphere &s, VICollBuffer &b);
    int CollideBufferV(VICollRay &r, VICollBuffer &b);
    int CollideBufferPackV(VICollRay &r, VICollBuffer &b);
    int CollideBufferPackVGF(VICollRay &r, VICollBuffer &b);

    int QueryBufferV(const VIBBox &w, const VIBBox &o, VIQueryTriList &l,
                     VICollBuffer &b);
    int QueryBufferPackV(const VIBBox &w, const VIBBox &o, VIQueryTriList &l,
                         VICollBuffer &b);
    int QueryBufferPackVGF(const VIBBox &w, const VIBBox &o, VIQueryTriList &l,
                           VICollBuffer &b);

    // None of the four reads `this`.
    int Cull(VICollSphere &s, VIBBox &b);
    int Cull(VICollSphere &s, VISphere &v);
    int Cull(VICollRay &r, VIBBox &b);
    int Cull(VICollRay &r, VISphere &v);

    int DebugResources();
};

// Nothing but a name for VIMap<unsigned, VIDictEntry>: the map is the FIRST
// and only member, so `a0` goes straight through at every forwarding call and
// the two Finds read the node at +20/+24 themselves.
class VIDictionary
{
public:
    class VIDictEntry
    {
    public:
        VIResourceType m_type;    // node +20
        int            m_index;   // node +24
    };

    VIMap<unsigned int, VIDictEntry> m_map;

    VIDictionary();

    int Size();
    int Clear();
    int Init(int n);
    int Add(VIResource &r, unsigned int id, VIResourceType type, int i);
    int Remove(VIResource &r);
    int Find(unsigned int id, VIResourceType &type, int &i);
    int FindTyped(unsigned int id, VIResourceType type, int &i);
};


// ---------------------------------------------------------------------------
// VIRFont / VIFont -- the glyph atlas and the drawing front end.
//
// VIRFont owns two things at once: the DECODED font (every glyph's pixels, in
// one `new unsigned char[]` block whose four sections are byte offsets until
// Init rebases them onto the block) and an LRU CACHE of the glyphs currently
// resident in a 256x256 VISurface.  Cache() is the whole point: a string comes
// in, every character is found, promoted to the tail of the doubly-linked slot
// chain, blitted into the surface if it was not there, and its slot index
// handed back.
//
// The pixel-format switch is bounded `sltiu 7` in both CalcDataSize and
// InitChars, which is what gives VIPixelStore its seventh value -- see the
// enum above, and the Phase 16 correction recorded there.
//
// The character table is sorted by code: FindChar is a binary search over
// m_numCharsLoaded, which EndChar bumps once per glyph as the parser fills the
// table in order.

class VIRFontChar
{
public:
    int            m_offset;       // +0x00  byte offset into m_data
    unsigned short m_bytesPerRow;  // +0x04
    unsigned short m_code;         // +0x06  stamped by BeginChar
    unsigned short m_slot;         // +0x08  0xFFFF == not resident
    unsigned char  m_width;        // +0x0A
};

// 56 bytes: three link words, the resident character, the cell's top-left in
// the surface, and the four corners' UVs precomputed on the way in.
class VIRFontSlot
{
public:
    int            m_offset;       // +0x00  byte offset into the locked level
    int            m_next;         // +0x04
    int            m_prev;         // +0x08
    unsigned short m_char;         // +0x0C  0xFFFF == free
    VIVect2I       m_pos;          // +0x10  the cell's top-left, and it is a
                                   //        sub-object: InitSlots writes both
                                   //        halves off ONE `addiu ...,16`
    VIVect2        m_uv[4];        // +0x18
};

class VIRFont : public VIBuffer
{
public:
    int          m_locked;             // +0x08
    int          m_surface;            // +0x0C  = -1
    int          m_material;           // +0x10  = -1
    int          m_charHeight;         // +0x14
    int          m_maxWidth;           // +0x18
    VIPixelStore m_pixelStore;         // +0x1C  = VIPixelStoreI8
    VIPixelStore m_palettePixelStore;  // +0x20  = VIPixelStoreRGBA32
    int          m_numCharsLoaded;     // +0x24  BeginChar/EndChar's cursor
    int          m_numChars;           // +0x28
    int          m_numSlots;           // +0x2C  what the surface really holds
    int          m_maxSlots;           // +0x30  the 256x256 estimate
    int          m_paletteCount;       // +0x34
    int          m_head;               // +0x38  = -1, the LRU end
    int          m_tail;               // +0x3C  = -1, the MRU end
    int          m_chars;              // +0x40  offset, then a pointer
    int          m_slots;              // +0x44
    int          m_palette;            // +0x48
    int          m_pixels;             // +0x4C
    int          m_dataSize;           // +0x50
    unsigned char *m_data;             // +0x54

    // Both element addresses come out OFFSET FIRST -- Phase 24's rule -- and
    // both are re-derived at every use, because a store through a register
    // base kills gcc 2.95's memory table (Phase 27) and the member has to be
    // read back.  So they are written as integer arithmetic, inline, and
    // called rather than cached in a local.
    VIRFontChar *Char(int i) const
    { return (VIRFontChar *)(i * (int)sizeof(VIRFontChar) + m_chars); }
    VIRFontSlot *Slot(int i) const
    { return (VIRFontSlot *)(i * (int)sizeof(VIRFontSlot) + m_slots); }

    VIRFont();
    ~VIRFont();

    int Init(VIRaster &r, VIDictionary &d, int numChars, unsigned char *widths,
             int height, VIBitDepth bd, int extra);
    int Clear();
    int IsPalettized();
    VIPixelStore PixelStore();
    VIPixelStore PalettePixelStore();
    int Lock(VIRaster &r);
    int Unlock(VIRaster &r);
    int Palette(int &count, VIPixelStore &ps, unsigned char **p);
    int BeginChar(unsigned short c, int &bytesPerRow, unsigned char **p);
    int EndChar();
    int CreateResources(VIRaster &r, VIDictionary &d, int numChars, int height,
                        VIBitDepth bd, int extra);
    int ReleaseResources(VIRaster &r, VIDictionary &d);
    int Cache(VIRaster &r, const char *s, int max, int *out);
    int Cache(VIRaster &r, const unsigned short *s, int max, int *out);
    int FindChar(unsigned short c);
    int GetWidth(char c);
    int GetWidth(unsigned short c);
    int GetStringWidth(const char *s, int spacing);
    int GetStringWidth(const unsigned short *s, int spacing);
    int InitSlots(VIRaster &r);
    int InitChars(unsigned char *widths);
    int ResetCache(VIRaster &r);
    int CalcMaxWidth(int n, unsigned char *widths);
    int CalcDataSize(unsigned char *widths);
    int Allocate();
};

// Five of the seven entry points do the same three things -- hash the name,
// look the font up in the dictionary, forward to VIRaster -- and each stores
// the hash into a static of its own before using it.  The class itself is
// nothing but the text colour: `this` is passed straight through as the
// `const VIColor32F &` argument of every VIRaster call.
class VIFont
{
public:
    VIColor32F m_color;

    static char sm_FontName[];

    VIFont();

    int  DrawString(VIRaster *r, VIDictionary *d, const unsigned short *s, int x, int y);
    int  DrawString(VIRaster *r, VIDictionary *d, const char *s, int x, int y);
    int  DrawCenteredString(VIRaster *r, VIDictionary *d, const unsigned short *s,
                            int x, int y, unsigned int w);
    int  DrawCenteredString(VIRaster *r, VIDictionary *d, const char *s,
                            int x, int y, unsigned int w);
    int  GetHeight(VIRaster *r, VIDictionary *d);
    int  GetStringWidth(VIRaster *r, VIDictionary *d, const char *s);
    int  GetStringWidth(VIRaster *r, VIDictionary *d, const unsigned short *s);
    int  GetCharacterWidth(VIRaster *r, VIDictionary *d, unsigned short c);
    int  FontLoaded(VIDictionary *d);
    void SetTextColor(const VIColor32F &c);
    void SetTextColor(const VIColor32 &c);
    static unsigned long SjisToJis(unsigned long c);
};

class VISprite : public VIResource
{
public:
    VIBBox   m_bbox;
    VISphere m_bsphere;
    float    m_offDist;
    int      m_type;
    int      m_actor;

    VISprite();
    virtual ~VISprite();

    virtual int GetInstType(VIScene &sc) const;
    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int QueryEmitters(VIScene &sc, const VIMatrix44 &m, VIEmitterQuery &q);
    virtual int QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                  const VIMatrix44 &m, VIScene &sc);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    int       GetType();
    VIBBox   *GetBBox();
    VISphere *GetBSphere();
    float     GetOffDist();
};

// ---------------------------------------------------------------------------
// VIParticleSprite -- SUPPORT 0x00454E38 .. 0x004564E0.  A VISprite with a
// whole VIParticleEmitter EMBEDDED at +0x50 (16-aligned, for the emitter's own
// VIMatrix44), so the class is 1076 bytes.  Its two Collide overrides are
// `return 0` stubs, exactly like VISkinSprite's and VIFloraSprite's.
// ---------------------------------------------------------------------------
class VIParticleSprite : public VISprite
{
public:
    VIParticleSprite();
    virtual ~VIParticleSprite();

    int Create(int def, VIScene &sc);

    virtual int GetInstType(VIScene &sc) const;
    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    VIParticleDefinition *ParticleDefinition();
    VIParticleEmitter    *ParticleEmitter();

    int               m_defIndex;   // +0x40
    VIParticleEmitter m_emitter;    // +0x50
    int               m_unk430;     // +0x430
};

// VISimpleSprite -- SUPPORT 0x00471D60 ..  One prim buffer, one material
// palette, one collision object, and no geometry of its own: every virtual is
// two or three lines that hand an index to VIRaster or VICollide.
//
//   +0x40  int m_primBuffer   GetPrimBuffer
//   +0x44  int m_objIdx       GetMaterialPal's neighbour; go-eqoa-vi's ObjIdx,
//                             the index into the collider's buffer pool
//   +0x48  int m_materialPal  GetMaterialPal
//
// It adds no virtuals -- its vtable is 96 bytes too.
class VISimpleSprite : public VISprite
{
public:
    int m_primBuffer;
    int m_objIdx;
    int m_materialPal;

    VISimpleSprite();
    virtual ~VISimpleSprite();

    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                  const VIMatrix44 &m, VIScene &sc);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    int GetPrimBuffer() const;
    int GetMaterialPal() const;
    int SetMaterialPal(int pal, VIDictionary &d, VIRaster &r);
    int SetMaterialPalDither(int dither, VIRaster &r);
};

// ---------------------------------------------------------------------------
// Phase 20 -- the sprite LEAVES.
//
// Every one of these derives from VISprite directly, and NOT ONE of them adds a
// virtual: the vtable data symbols in the map look like 128/136/144 bytes, but
// that is the gap-derived span swallowing the RTTI name string that follows.
// Read the bytes and all six are the same shape -- twelve entries, of which
// entries 0..11 are the header plus ten pfns, and the ASCII of "12VISkinSprite"
// / "13VIFloraSprite" / ... begins at byte 96 exactly.  So VISprite's ten
// virtuals are the whole dispatch surface of the hierarchy, and everything a
// leaf adds beyond an override is a plain member function.

// VISkinSprite -- SUPPORT 0x00472C68.  A VISimpleSprite whose collision object
// it does not have: both Collide overloads are `return 0` stubs, because a
// skinned mesh moves every frame and the collider holds nothing for it.
//
//   +0x40  unsigned m_flags   bit 0 is HIDDEN, not visible -- SetVisible(1)
//                             CLEARS it, and Raster draws when it is clear
//   +0x44  int      m_primBuffer
//   +0x48  int      m_materialPal   GetMaterialPal
class VISkinSprite : public VISprite
{
public:
    unsigned int m_flags;
    int          m_primBuffer;
    int          m_materialPal;

    VISkinSprite();
    virtual ~VISkinSprite();

    int SetVisible(int visible);

    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    int GetMaterialPal() const;
    int SetMaterialPal(int pal, VIRaster &r, VIDictionary &d);
};

// VIFloraSprite -- SUPPORT 0x00440480.  VISimpleSprite minus the collision
// object and minus QueryIntersection: the radial flora system spawns these by
// the hundred off a terrain patch and nothing may ever collide with one.
//
//   +0x40  int m_primBuffer    GetPrimBuffer
//   +0x44  int m_materialPal   GetMaterialPal
class VIFloraSprite : public VISprite
{
public:
    int m_primBuffer;
    int m_materialPal;

    VIFloraSprite();
    virtual ~VIFloraSprite();

    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    int GetPrimBuffer() const;
    int GetMaterialPal() const;
    int SetMaterialPal(int pal, VIDictionary &d, VIRaster &r);
    int SetMaterialPalDither(int dither, VIRaster &r);
};

// VILODSprite -- SUPPORT 0x00452940.  A distance-switched cover for up to five
// other sprites: each level carries a switch distance and a sprite index, and
// every virtual walks the table for the first level whose distance still covers
// the viewer, then forwards to that sprite THROUGH THE VTABLE.
//
//   +0x40  unsigned      m_flags       bit 0 HIDDEN, as VISkinSprite's
//   +0x44  int           m_numLevels   GetNumLevels
//   +0x48  VILODLevel[5] m_levels      Copy moves 40 bytes of it in one go
//
// The five is not a guess: Copy's ldl/ldr block move runs 72..111 inclusive.
struct VILODLevel
{
    float m_dist;
    int   m_sprite;
};

class VILODSprite : public VISprite
{
public:
    unsigned int m_flags;
    int          m_numLevels;
    VILODLevel   m_levels[5];

    VILODSprite();
    virtual ~VILODSprite();

    int SetVisible(int visible);
    int SetLevelSprite(int level, int sprite, VIScene &sc);

    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                  const VIMatrix44 &m, VIScene &sc);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    int GetNumLevels();
    int GetLevelSprite(int level);
};

// ---------------------------------------------------------------------------
// VISpellEffectDriver -- SUPPORT 0x0047A978 .. 0x0047B8A8.  The per-cast
// object `spell-event-driver.md` describes from the other end: it holds the
// caster, the target and up to eight secondary targets, and it owns a VILList
// of VISpellEventDriver, one per event list in the effect.
//
//   +0x00  VISpellEffect *m_effect
//   +0x04  VIScene       *m_scene
//   +0x08  int            m_caster           -1
//   +0x0C  int            m_target           -1
//   +0x10  int            m_numSecondary
//   +0x14  int            m_secondary[8]     -1
//   +0x38  long           m_triggerTime      Trigger's `sd`, so 64-bit
//   +0x40  int            m_arg0
//   +0x44  int            m_arg1
//   +0x48  VILList        m_drivers          member, not a base (its dtor
//                                            takes __in_chrg 2 -- phase 29)
//   +0x54  VIVibrateFn    m_vibrate
//   +0x58  void          *m_vibrateUser
//   +0x5C  VICameraEffect *m_cameraEffect
//   +0x60  int            m_playerActor
//   +0x64  int            m_cameraEffectOn
//
// The class has NO vtable: nothing in the constructor stores a vptr.
// ---------------------------------------------------------------------------
class VISpellEventList;
class VISpellEffectDriver;
class VICameraEffect;
class VISpellActiveEvent;
class VISpellEvent;
class VILightning;
class VICSprite;

// The VICSprite enums.  Phase 53 fixes their WIDTH, which phase 38 could not:
// every one of them is a plain integer in the object code, but the counts are
// asserted by the constructor's zeroing loops and by the clamps -- nine nodes,
// sixty-six animation ids, six attach slots, six armour slots, nineteen
// texture slots, nine armour sets and fifteen tints.
//
// The node ids that are NAMED are the two the class reaches directly:
// VICSprite::SetName attaches the plate at node 3 and SetHelm the helm at node
// 8.  `claude-notes/anim-slot-semantics.md` has the slot -> action table for
// the animation ids, verified visually on the humanoid skeleton (0 idle, 1
// walk, 3 run, 14 death, 17 battle stance, 22..29 and 40..43 the attack
// families, 37..39 casting); nothing here asserts it, so the enumerators are
// numbered rather than named.
enum VICSpriteNodeID
{
    VICSpriteNodeID0 = 0,
    VICSpriteNodeID1 = 1,
    VICSpriteNodeID2 = 2,
    VICSpriteNodeIDName = 3,
    VICSpriteNodeID4 = 4,
    VICSpriteNodeID5 = 5,
    VICSpriteNodeID6 = 6,
    VICSpriteNodeID7 = 7,
    VICSpriteNodeIDHelm = 8
};

enum VICSpriteAnimID
{
    VICSpriteAnimID0 = 0,
    VICSpriteAnimID1 = 1,
    VICSpriteAnimID2 = 2,
    VICSpriteAnimID3 = 3,
    VICSpriteAnimID4 = 4,
    VICSpriteAnimID5 = 5,
    VICSpriteAnimID6 = 6,
    VICSpriteAnimID7 = 7,
    VICSpriteAnimID8 = 8,
    VICSpriteAnimID9 = 9,
    VICSpriteAnimID10 = 10,
    VICSpriteAnimID11 = 11,
    VICSpriteAnimID12 = 12,
    VICSpriteAnimID13 = 13,
    VICSpriteAnimID14 = 14,
    VICSpriteAnimID15 = 15,
    VICSpriteAnimID16 = 16,
    VICSpriteAnimID17 = 17,
    VICSpriteAnimID18 = 18,
    VICSpriteAnimID19 = 19,
    VICSpriteAnimID20 = 20,
    VICSpriteAnimID21 = 21,
    VICSpriteAnimID22 = 22,
    VICSpriteAnimID23 = 23,
    VICSpriteAnimID24 = 24,
    VICSpriteAnimID25 = 25,
    VICSpriteAnimID26 = 26,
    VICSpriteAnimID27 = 27,
    VICSpriteAnimID28 = 28,
    VICSpriteAnimID29 = 29,
    VICSpriteAnimID30 = 30,
    VICSpriteAnimID31 = 31,
    VICSpriteAnimID32 = 32,
    VICSpriteAnimID33 = 33,
    VICSpriteAnimID34 = 34,
    VICSpriteAnimID35 = 35,
    VICSpriteAnimID36 = 36,
    VICSpriteAnimID37 = 37,
    VICSpriteAnimID38 = 38,
    VICSpriteAnimID39 = 39,
    VICSpriteAnimID40 = 40,
    VICSpriteAnimID41 = 41,
    VICSpriteAnimID42 = 42,
    VICSpriteAnimID43 = 43,
    VICSpriteAnimID44 = 44,
    VICSpriteAnimID45 = 45,
    VICSpriteAnimID46 = 46,
    VICSpriteAnimID47 = 47,
    VICSpriteAnimID48 = 48,
    VICSpriteAnimID49 = 49,
    VICSpriteAnimID50 = 50,
    VICSpriteAnimID51 = 51,
    VICSpriteAnimID52 = 52,
    VICSpriteAnimID53 = 53,
    VICSpriteAnimID54 = 54,
    VICSpriteAnimID55 = 55,
    VICSpriteAnimID56 = 56,
    VICSpriteAnimID57 = 57,
    VICSpriteAnimID58 = 58,
    VICSpriteAnimID59 = 59,
    VICSpriteAnimID60 = 60,
    VICSpriteAnimID61 = 61,
    VICSpriteAnimID62 = 62,
    VICSpriteAnimID63 = 63,
    VICSpriteAnimID64 = 64,
    VICSpriteAnimID65 = 65
};

// Six of them: 0 and 1 are the two weapon hands (SetAttackAction refuses
// anything else with a `sltiu 2`), 3 is the skin the LOD pair hangs off, 4 the
// name plate and 5 the helm.
enum VICSpriteAttachSlot
{
    VICSpriteAttachSlot0 = 0,
    VICSpriteAttachSlot1 = 1,
    VICSpriteAttachSlot2 = 2,
    VICSpriteAttachSlotSkin = 3,
    VICSpriteAttachSlotName = 4,
    VICSpriteAttachSlotHelm = 5
};

// SetArmorSlot takes one; slot 5 is the helm and is handed straight to
// SetHelm, the other five are texture swaps on the first five texture slots.
enum VICSpriteArmorSlot
{
    VICSpriteArmorSlot0 = 0,
    VICSpriteArmorSlot1 = 1,
    VICSpriteArmorSlot2 = 2,
    VICSpriteArmorSlot3 = 3,
    VICSpriteArmorSlot4 = 4,
    VICSpriteArmorSlotHelm = 5
};

enum VICSpriteSkelType
{
    VICSpriteSkelType0 = 0,
    VICSpriteSkelType1 = 1
};

enum VICSpriteSex
{
    VICSpriteSexMale = 0,
    VICSpriteSexFemale = 1
};

// The default in the constructor and in DetachItem is 11.
enum VICSpriteAttackType
{
    VICSpriteAttackType0 = 0,
    VICSpriteAttackType11 = 11
};

// SetName indexes _9VICSprite$NameColors with it, four bytes per entry.
enum VICSpriteNameColor
{
    VICSpriteNameColor0 = 0
};

// ---------------------------------------------------------------------------
// VISpellEventDriver -- SUPPORT 0x00475E68 .. 0x0047A978, Phase 38.  One event
// LIST while it is running: a cursor into the list, whatever that list has
// started that is still going, and the two animation handles it needs to wait
// on.  100 bytes -- AddSpellEventDriver hands __builtin_new exactly that -- and
// a VILNode, since AddTail takes it with no adjustment.
//
// The layout is read off the constructor, which stores every field, and the
// destructor, which says what each pointer POINTS AT: the deletes at +0x28 and
// +0x2C are `_$_11VILightning`, not `_$_18VISpellActiveEvent`, so the two words
// `spell-event-driver.md` calls "reserved" are the bolt pair the lightning
// event draws with.  +0x30 is a VISpellActiveEvent and is NOT deleted.
// ---------------------------------------------------------------------------
class VISpellEventDriver : public VILNode
{
public:
    VISpellEventDriver(VISpellEffectDriver *d, VISpellEventList *l, int actor,
                       VIScene *sc);
    ~VISpellEventDriver();

    VISpellActiveEvent *FindCreateEmitter(int id);
    VISpellActiveEvent *FindCreateSound(int id);
    VISpellActiveEvent *FindCreateSprite(int id);
    VISpellActiveEvent *FindSetEmitterMotif(int id, unsigned int motif);

    int NewWait(VISpellEvent *ev);
    int NewSetAction(VISpellEvent *ev);
    int NewPlayAttack(VISpellEvent *ev);
    int NewVibrateController(VISpellEvent *ev);
    int NewShakeCamera(VISpellEvent *ev);
    int NewCreateEmitter(VISpellEvent *ev);
    int NewSetEmitterMotif(VISpellEvent *ev);
    int NewFadeSprite(VISpellEvent *ev);
    int NewSetEmitterAttractor(VISpellEvent *ev);
    int NewDestroyEmitter(VISpellEvent *ev);
    int NewPlayTargetInstant(VISpellEvent *ev);
    int NewPlayTargetProjectile(VISpellEvent *ev);
    int NewCreateSound(VISpellEvent *ev);
    int NewDestroySound(VISpellEvent *ev);
    int NewCreateSprite(VISpellEvent *ev);
    int NewDestroySprite(VISpellEvent *ev);
    int NewPlayTargetLightning(VISpellEvent *ev);

    int ProcessCreateEmitter(VISpellActiveEvent *e, bool active);
    int ProcessCreateSound(VISpellActiveEvent *e, bool active);
    int ProcessCreateSprite(VISpellActiveEvent *e, bool active);
    int ProcessSetEmitterMotif(VISpellActiveEvent *e);
    int ProcessFadeSprite(VISpellActiveEvent *e);
    int ProcessLightningEvent();
    int ProcessProjectileEvent();
    int ProcessWaitEvent();
    int ProcessInactiveEvents();

    int GetNodeTransform(int actor, VICSpriteNodeID node, VIMatrix44 &m);
    int Process();
    int ParticleCount() const;

    VISpellEffectDriver *m_driver;      // +0x10
    VIScene             *m_scene;       // +0x14
    int                  m_actor;       // +0x18
    int                  m_index;       // +0x1C  cursor into the event array
    VISpellActiveEvent  *m_wait;        // +0x20
    VISpellActiveEvent  *m_projectile;  // +0x24
    VILightning         *m_bolt0;       // +0x28
    VILightning         *m_bolt1;       // +0x2C
    VISpellActiveEvent  *m_lightning;   // +0x30
    int                  m_action;      // +0x34  -1
    // ProcessWaitEvent indexes it (`sll 2` off the event's own field), so it
    // is an ARRAY of four and not four named words: "Spell FX 1".."Spell FX 4".
    int                  m_anim[4];     // +0x38
    VISpellEventList    *m_list;        // +0x48
    VILList              m_active;      // +0x4C
    VILList              m_inactive;    // +0x58
};

// ---------------------------------------------------------------------------
// Phase 37 -- the spell effect system, and the classes underneath it.
//
// A VISpellEvent is the 44-byte record the ESF carries (`Create` hands
// __builtin_vec_new exactly `n * 44`).  Only the two fields VISpellEventList
// itself reads are named: the type at +0 and the resource id at +8, which
// Destroy hands back to the particle system (type 3) or the scene (type 9).
// ---------------------------------------------------------------------------
class VISpellEvent
{
public:
    int   m_type;       // +0x00   1..17, the jump table in Process
    int   m_p04;
    int   m_id;         // +0x08   particle definition or sprite, -1 for none
    int   m_p0C;
    int   m_p10;
    float m_f14;
    float m_f18;
    float m_f1C;
    float m_f20;
    float m_f24;
    float m_f28;        // 44 bytes
};

class VISpellEventList
{
public:
    VISpellEventList();
    ~VISpellEventList();

    int  Create(int n, VIScene *sc);
    void Destroy();

    VIScene      *m_scene;   // +0x00
    int           m_count;   // +0x04
    VISpellEvent *m_events;  // +0x08
};

// 48 bytes -- CreateSpellEffect hands __builtin_new exactly that -- with the
// resource header as a BASE: ReleaseSpellEffect passes the effect itself to
// VIDictionary::Remove with no adjustment, where Phase 28's VIParticleDefinition
// is handed `d + 764`.  The three lists are three NAMED members and not an
// array of three: the constructor calls their constructors at +12/+24/+36 with
// three separate `jal`s, where a member array emits a loop (Phase 24).
class VISpellEffect : public VIResource
{
public:
    VISpellEffect();
    ~VISpellEffect();

    int Create(VIScene *sc);
    int Destroy();

    int GetLastImpactTime(VIScene &sc, unsigned int a, int b, int c, int d,
                          int *e, int *f);

    VIScene         *m_scene;         // +0x08
    VISpellEventList m_casterEvents;  // +0x0C
    VISpellEventList m_targetEvents;  // +0x18
    VISpellEventList m_areaEvents;    // +0x24
};

typedef void (*VIVibrateFn)(unsigned char a, unsigned char b, int c, void *user);

class VISpellEffectDriver
{
public:
    VISpellEffectDriver();
    ~VISpellEffectDriver();

    int  Create(VISpellEffect *e, VIScene *sc);
    void Destroy();
    int  SetCaster(int actor);
    int  SetTarget(int actor);
    int  AddSecondaryTarget(int actor);
    void SetVibrateCallback(VIVibrateFn fn, void *user);
    void SetCameraEffect(VICameraEffect *e);
    void SetPlayerActor(int actor);
    void NotifyActorWasDestroyed(int actor);
    int  Trigger(int a, int b);
    VISpellEventDriver *AddSpellEventDriver(VISpellEventList *l, int actor);
    int  Process();

    VISpellEffect  *m_effect;           // +0x00
    VIScene        *m_scene;            // +0x04
    int             m_caster;           // +0x08
    int             m_target;           // +0x0C
    int             m_numSecondary;     // +0x10
    int             m_secondary[8];     // +0x14
    int             m_pad;              // +0x34
    long            m_triggerTime;      // +0x38
    int             m_arg0;             // +0x40
    int             m_arg1;             // +0x44
    VILList         m_drivers;          // +0x48
    VIVibrateFn     m_vibrate;          // +0x54
    void           *m_vibrateUser;      // +0x58
    VICameraEffect *m_cameraEffect;     // +0x5C
    int             m_playerActor;      // +0x60
    int             m_cameraEffectOn;   // +0x64
};

// ---------------------------------------------------------------------------
// VIIconDefinition -- SUPPORT 0x003E9140 .. 0x003E96D8, its own TU.  It IS the
// PS2 memory card's `icon.sys`: 0x3C4 bytes of header laid out exactly as the
// BIOS reads it, plus the icon file's address and length behind it.  The
// offsets are not guessed -- the constructor writes 'PS2D' at +0, memsets 68
// bytes at +0xC0 and three 64-byte names at +0x104/+0x144/+0x184, and SetIcon
// puts the data pointer at +0x3C4.
//
// The two colour groups are INTEGER (the four background corners are built on
// the stack as {0,0,64,128}) and the light group is FLOAT.  Shipped: two of
// the three light colours are BLACK -- only light 1 and the ambient are set,
// both to 0.65 in all four channels.
// ---------------------------------------------------------------------------

// The background colour of a card icon: four INTEGER channels, with a zeroing
// default constructor (the icon definition's own constructor opens with a
// four-iteration loop over the four corners, sixteen bytes each) and a
// four-channel constructor whose temporary is block-copied onto the member.
class VIIconColor
{
public:
    int r, g, b, a;

    VIIconColor() { r = 0; g = 0; b = 0; a = 0; }
    VIIconColor(int _r, int _g, int _b, int _a) { r = _r; g = _g; b = _b; a = _a; }
};

// One entry of Ascii2Sjis's two tables: four bytes, of which the punctuation
// table uses only the first short.
class VISjisMap
{
public:
    unsigned short m_sjis;
    unsigned short m_first;
};

class VIIconDefinition
{
public:
    VIIconDefinition();
    ~VIIconDefinition();

    int  SetTitle(const char *title);
    void SetIcon(void *data, int size);
    unsigned short Ascii2Sjis(char c);
    void AsciiString2Sjis(char *src, unsigned short *dst);

    // One WORD, not four bytes: the constructor materialises 0x44325350 with
    // lui+ori and stores it with `sw`, where a 4-byte memcpy of the string
    // constant would load it out of .rodata.
    unsigned int   m_head;           // +0x000  'PS2D'
    unsigned short m_reserved;       // +0x004
    unsigned short m_nlOffset;       // +0x006  the line break, in HALFWORDS*2
    int            m_unknown;        // +0x008
    int            m_trans;          // +0x00C  80
    VIIconColor    m_bgColor[4];     // +0x010
    float          m_lightDir[3][4]; // +0x050
    float          m_lightColor[3][4];// +0x080
    float          m_ambient[4];     // +0x0B0
    unsigned short m_title[34];      // +0x0C0  68 bytes
    char           m_fnameView[64];  // +0x104
    char           m_fnameCopy[64];  // +0x144
    char           m_fnameDel[64];   // +0x184
    char           m_reserved2[512]; // +0x1C4
    void          *m_iconData;       // +0x3C4
    int            m_iconSize;       // +0x3C8
};

// ---------------------------------------------------------------------------
// VICSpriteCust -- SUPPORT 0x004283C8 .. 0x00429570.  Character customisation,
// and it is nothing but static tables and the lookups into them: armour, hair,
// robe, helm, face and tint.  `claude-notes/vicspritecust-tables.md` read the
// VALUES out of EE RAM years ago; what the object code adds is the SHAPES, and
// it corrects that note twice -- $Tints is FIFTEEN entries, not fourteen (the
// static constructor's loop runs `li v1,14` down to -1), and $Race's rows are
// [4][2], which is what makes `race*32 + face*8 + sex*4` one subscript chain.
//
// Every entry point is a static member: a0 carries the first argument, never a
// `this`.  The only static member with a constructor is $Tints, which is why
// gcc's initialiser is named _GLOBAL_$I$_13VICSpriteCust$Tints (phase 32) --
// VIColor32's opaque-black default constructor, fifteen times.
//
// Shipped: GetArmorSetTexture IGNORES ITS RACE.  `li a0,20` sits in the delay
// slot of the `armorSet == 0` test, so the race argument is destroyed before
// it is read, and the note above had that from the decompilation too.
// ---------------------------------------------------------------------------
enum VICSpriteRace
{
    VICSpriteRaceHuman = 0,
    VICSpriteRaceNone  = 9
};

// Nine of them: SetArmorSlot clamps `set >= 9` back to 0.
enum VICSpriteArmorSet
{
    VICSpriteArmorSetNone = 0
};

// Nineteen, one per entry of VICSprite::m_slots.  Which is which is read off
// the callers, not guessed: 0..4 are the armour/robe body slots (SetArmorSlot
// walks slots 0..4 and SetRobe the same five), 7 is the face (SetFace reaches
// m_slots[7] with the index folded to a constant) and 8..15 are the hair.
enum VICSpriteTextSlot
{
    VICSpriteTextSlot0 = 0,
    VICSpriteTextSlot1 = 1,
    VICSpriteTextSlot2 = 2,
    VICSpriteTextSlot3 = 3,
    VICSpriteTextSlot4 = 4,
    VICSpriteTextSlot5 = 5,
    VICSpriteTextSlot6 = 6,
    VICSpriteTextSlot7 = 7,
    VICSpriteTextSlot8 = 8,
    VICSpriteTextSlot9 = 9,
    VICSpriteTextSlot10 = 10,
    VICSpriteTextSlot11 = 11,
    VICSpriteTextSlot12 = 12,
    VICSpriteTextSlot13 = 13,
    VICSpriteTextSlot14 = 14,
    VICSpriteTextSlot15 = 15,
    VICSpriteTextSlot16 = 16,
    VICSpriteTextSlot17 = 17,
    VICSpriteTextSlot18 = 18
};

// Fifteen, and SetArmorSlot clamps `tint >= 15` back to 0 -- which is the same
// count VICSpriteCust::$Tints's static constructor runs (phase 35).
enum VICSpriteTint
{
    VICSpriteTintWhite = 0
};

class VICSpriteCust
{
public:
    static int GetArmorSetTexture(VICSpriteRace race, VICSpriteArmorSet set,
                                  VICSpriteTextSlot slot);
    static int GetHairTexture(int hair);
    static int GetFaceTexture(int face, int sex, VICSpriteRace race, VIDictionary &d);
    static int GetFaceResourceID(int face, int sex, VICSpriteRace race);
    static int GetRobeTexture(int robe, VICSpriteTextSlot slot);
    static int GetHelm(VICSpriteArmorSet set);
    static VIColor32 *GetTintColor(VICSpriteTint tint);

    static VIColor32 Tints[15];
    static int       Armor[9][5];
    static int       Race[11][4][2];
    static int       Hair[8];
    static int       Robe[4][2];
    static int       Helm[10];
    static int       HairNull;
};

// VIPointLight -- SUPPORT 0x0045B878 .. 0x0045C950, its own TU, and the LAST
// sprite leaf.  It is VIPointSprite's twin: Raster, Pick, Collide, Release and
// ClampScale are byte-identical to it, the only difference being that the
// scene sprite it forwards to is the LIGHT UI sprite rather than the point one
// (eqcalls.py reads GetLightUISprite out of the relocated dump; the call site
// is the same instruction).  Where VIPointSprite carries an int type at +0x40,
// this carries a whole VIColor32F, so the class is 0x50 bytes.
//
//   +0x40  VIColor32F m_color   SetColor / GetColor
//
// GetColor returns the colour BY VALUE (a0 is the sret slot) and SetColor
// returns int -- the same asymmetry VISprite's own accessors have.
class VIPointLight : public VISprite
{
public:
    VIColor32F m_color;

    VIPointLight();
    virtual ~VIPointLight();

    int Init(float r, const VIColor32F &c);

    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    int        ClampScale(VIMatrix44 &m);
    int        SetColor(VIColor32F &c);
    VIColor32F GetColor() const;
};

// VIPointSprite -- SUPPORT 0x0045C950.  A billboard for one of the scene's
// shared UI sprites (light / sound / particle / point / stream-audio), used to
// show an otherwise invisible object in the editor.  It owns no geometry at
// all: it clamps the model-view scale and hands the whole draw to the scene's
// point UI sprite.
//
//   +0x40  int m_pointType   GetPointType
enum VIPointType
{
    VIPointTypePoint = 0
};

class VIPointSprite : public VISprite
{
public:
    int m_pointType;

    VIPointSprite();
    virtual ~VIPointSprite();

    int Init(VIPointType type);

    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    int ClampScale(VIMatrix44 &m);
    int GetPointType();
};

// VIGroupSprite -- SUPPORT 0x00443250.  Four (sprite, location, scale, hpr)
// members drawn under one transform, and the only leaf that composes matrices:
// every virtual walks the members, builds the member transform, multiplies it
// onto whatever the caller had set, pushes that back and forwards.
//
//   +0x40  int              m_numMembers   GetNumMembers
//   +0x44  VIGroupMember[5] m_members      stride 32, Copy moves 160 bytes
//
// The stride is read off GetMemberSprite's `sll a1,a1,5`, and the FIVE off two
// independent counts: the constructor's array loop runs `li v1,4` down to -1,
// and Copy's block-move loop stops at `this + 228` = 0x44 + 5*32.  That loop
// zeroes m_loc and m_hpr and NOTHING ELSE, because those are the two VIVect3s
// and VIVect3 has a zeroing constructor -- m_sprite and m_scale are left.
struct VIGroupMember
{
    int     m_sprite;
    VIVect3 m_loc;
    float   m_scale;
    VIVect3 m_hpr;
};

class VIGroupSprite : public VISprite
{
public:
    int           m_numMembers;
    VIGroupMember m_members[5];

    VIGroupSprite();
    virtual ~VIGroupSprite();

    virtual int GetInstType(VIScene &sc) const;
    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int QueryEmitters(VIScene &sc, const VIMatrix44 &m, VIEmitterQuery &q);
    virtual int QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                  const VIMatrix44 &m, VIScene &sc);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    int CalcMemberTransform(int i, VIMatrix44 &m);
    int GetNumMembers();
    int GetMemberSprite(int i);
};

// --------------------------------------------------------------- VINameSprite --
// SUPPORT 0x00453AB8 .. 0x00454E38.  The floating name plate over a character,
// and the only sprite in the corpus that does not draw itself where it is asked
// to: Raster() only ENQUEUES, into a 24-entry file-scope table, and one static
// pass at the end of the frame sorts the queue back to front, throws away every
// plate whose screen rectangle overlaps one already kept, and draws what is
// left.  So a name is a two-phase sprite and the table is the phase boundary.
//
// The record is 144 bytes and its shape reads straight off Raster: the sort key
// is a float at +0, the font at +4, a 32-byte name at +8, the colour at +0x28,
// the screen rect at +0x38 (the eight bytes after it are the VIMatrix44's
// 16-alignment), and the model-view the plate was queued under at +0x50.
class VINameDefer
{
public:
    float      m_z;         // +0x00   view-space depth, the qsort key
    int        m_font;      // +0x04
    char       m_name[32];  // +0x08
    VIColor32F m_color;     // +0x28
    VIRect     m_rect;      // +0x38
    VIMatrix44 m_mat;       // +0x50
};

// A free function, not a static member: the map spells it
// `VINameDeferSort__FPCvT0` -- two `const void *`, the second a repeat.
int VINameDeferSort(const void *a, const void *b);

class VINameSprite : public VISprite
{
public:
    int        m_font;      // +0x40
    char       m_name[32];  // +0x44
    VIColor32F m_color;     // +0x64

    static int         NumNameDefers;
    static VINameDefer NameDefers[24];

    VINameSprite();
    virtual ~VINameSprite();

    int Init(const char *name, int font, const VIColor32F &c, VIRaster *r);
    int Clear();
    int SetName(const char *name);
    int SetColor(const VIColor32F &c);

    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    // `this` is never formed: a0 is the raster at every use, so this is a
    // STATIC member -- the frame's one deferred pass, not a per-plate call.
    static int RasterDeferred(VIRaster &r);
};

// -------------------------------------------------------------- VISoundSprite --
// SUPPORT 0x004739C8 .. 0x00474BD0.  A positional sound in the scene graph: it
// draws the scene's sound UI sprite where it stands (so it is visible only with
// the audio debug sprite loaded) and owns one VISoundDevice sound plus the
// channel it is playing on.  VIPointSprite::ClampScale is written out INLINE in
// both Raster and Pick rather than called -- the same nine multiplies twice.
class VISoundSprite : public VISprite
{
public:
    enum ATTENUATIONMODE
    {
        ATTENUATION_0 = 0,
        ATTENUATION_1 = 1,
        ATTENUATION_2 = 2
    };

    int             m_sound;        // +0x40
    int             m_channel;      // +0x44
    int             m_flags;        // +0x48  0x10000000
    ATTENUATIONMODE m_attenuation;  // +0x4C
    bool            m_pan;          // +0x50  four bytes: Phase 28
    float           m_volume;       // +0x54

    VISoundSprite();
    virtual ~VISoundSprite();

    virtual int GetInstType(VIScene &sc) const;
    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    ATTENUATIONMODE AttenuationMode() const;
    void            SetAttenuationMode(ATTENUATIONMODE m);
    bool            IsPanEnabled() const;
    void            EnablePan(bool on);
    float           Volume() const;
};

// --------------------------------------------------- VIStreamAudioSprite --
// SUPPORT 0x0047F090 .. 0x004812E0.  A streaming-audio volume: a box the
// listener can stand inside, a 128-byte relative pathname, and a priority that
// is turned into the query sort key by a THREE-case switch with a `return -1`
// default.  Unlike VISoundSprite it keeps ClampScale out of line, and it is the
// only leaf in this phase that overrides QueryEmitters.
class VIStreamAudioSprite : public VISprite
{
public:
    bool m_looping;         // +0x40   true
    int  m_priority;        // +0x44
    char m_pathname[128];   // +0x48

    VIStreamAudioSprite();
    virtual ~VIStreamAudioSprite();

    int Init(const VIBBox &b);

    virtual int GetInstType(VIScene &sc) const;
    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int QueryEmitters(VIScene &sc, const VIMatrix44 &m, VIEmitterQuery &q);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    bool        IsLooping() const;
    int         GetPriority() const;
    const char *GetRelativePathname() const;

    int ClampScale(VIMatrix44 &m);
};

// ------------------------------------------------------ VIEffectVolume --
// SUPPORT 0x00429570 .. 0x0042F798.  A box of weather: falling leaves, light
// shafts, ground fog, a dust storm or a flock of birds, chosen by m_type and
// sized by the box the sprite hands it.  The layout reads straight off the
// constructor -- a VIBBox whose members zero themselves twice, then a run of
// scalars -- and the five accessors name m_type, m_density, m_flags,
// m_surface and m_sprite exactly.
enum VIEffectVolumeType
{
    VIEFFECTVOLUME_NONE   = 0,
    VIEFFECTVOLUME_LEAVES = 1,
    VIEFFECTVOLUME_SHAFTS = 2,
    VIEFFECTVOLUME_FOG    = 3,
    VIEFFECTVOLUME_DUST   = 4,
    VIEFFECTVOLUME_FLOCK  = 5
};

class VIEffectVolume;

class VIBoid
{
public:
    int        m_sprite;    // +0x00
    VIVect3    m_hpr;       // +0x04
    int        m_p10;       // +0x10
    VIVect3    m_dir;       // +0x14
    VIVect3    m_pos;       // +0x20
    char       m_pad[0x60 - 0x2C];
    VIMatrix44 m_xform;     // +0x60
    int        m_pA0;       // +0xA0  -1
    float      m_pA4;       // +0xA4
    float      m_pA8;       // +0xA8
    char       m_tail[0xB0 - 0xAC];

    void Init();
    void Free(VIScene &sc);
    void SeekGoal(float dt, VIVect3 &goal);
    void Apply(float dt);
    void Render(VIScene &sc, VICamera &cam);
};

class VIEffectVolume
{
public:
    long               m_time;         // +0x00  `sd zero` -- eight bytes
    VIBBox             m_bbox;         // +0x08
    int                m_reseed;       // +0x20  1
    VIEffectVolumeType m_type;         // +0x24
    float              m_p28;          // +0x28  1.0f
    float              m_density;      // +0x2C  1.0f
    float              m_p30;          // +0x30  20.0f
    float              m_p34;          // +0x34
    float              m_p38;          // +0x38
    unsigned int       m_flags;        // +0x3C
    int                m_surface;      // +0x40  -1
    int                m_sprite;       // +0x44  -1
    int                m_dirty;        // +0x48  1
    int                m_material;     // +0x4C  -1
    int                m_numParticles; // +0x50
    void              *m_particles;    // +0x54
    int                m_p58;          // +0x58
    int                m_p5C;          // +0x5C
    VIVect3            m_wind;         // +0x60
    int                m_p6C;          // +0x6C

    static VIScene *mScene;

    VIEffectVolume();
    ~VIEffectVolume();

    static void GlobalFree();
    // Phase 54, off VIScene::Init: the scene hands itself to the effect-volume
    // system once, and the static mScene above is where it lands.
    static int  GlobalInit(VIScene *sc);

    int   Init(const VIBBox &b);
    void  Free();
    void  OnResize(const VIBBox &b);
    int   Process(float dt);
    int   Render(VICamera &cam, VIMatrix44 &m);

    VIEffectVolumeType Type() const;
    float              Density() const;
    void               SetType(VIEffectVolumeType t, float density);
    void               SetDensity(float d);
    unsigned int       Flags() const;
    void               SetFlags(unsigned int f);
    int                Surface() const;
    int                AttachSurface(int i);
    int                DetachSurface();
    int                Sprite() const;
    int                AttachSprite(int i);
    int                DetachSprite();
    void               DetachAllResources();

    int  ProcessFallingLeaves(float dt);
    int  ProcessLightShafts(float dt);
    int  ProcessGroundFog(float dt);
    int  ProcessDustStorm(float dt);
    int  ProcessFlock(float dt);

    int AllocParticles();
    void FreeParticles();
    int AllocFlock();
    void FreeFlock();
};

// ------------------------------------------------- VIEffectVolumeSprite --
// SUPPORT 0x0042F798 .. 0x00430A90.  A VIEffectVolume with a place in the
// scene graph.  It keeps a SECOND bounding box -- the one the volume was last
// built for -- and re-seeds the volume whenever the sprite's own box has drifted
// more than a metre in any axis.  Bit 1 of the raster flags is the one that
// actually draws the effect, in the camera's own space.
class VIEffectVolumeSprite : public VISprite
{
public:
    VIBBox         m_localBBox;   // +0x40   the box m_volume was built for
    VIEffectVolume m_volume;      // +0x58

    VIEffectVolumeSprite();
    virtual ~VIEffectVolumeSprite();

    int Init(const VIBBox &b);

    virtual int GetInstType(VIScene &sc) const;
    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    VIEffectVolume *EffectVolume();
};

// ---------------------------------------------------------------- VIFrustum --
// SUPPORT 0x00442CC8 .. 0x00443268.  Eight corner vertices and six planes, and
// the layout reads straight off the two accessors: `Vertex(i)` is
// `this + i*12` and `Plane(i)` is `this + 96 + i*16`, so the vertices are at
// +0x00 (8 x VIVect3 = 96 bytes) and the planes at +0x60 (6 x VIPlane = 96),
// size 0xC0.
class VIFrustum
{
public:
    VIVect3 m_verts[8];
    VIPlane m_planes[6];

    int Init();
    int CalcPlanes();
    int Cull(const VIVect3 &p, float r) const;
    int Cull(const VISphere &s) const;
    int Cull(const VIBBox &b, const VIMatrix44 &m) const;
    VIPlane *Plane(int i) const;
    void SetPlane(int i, const VIPlane &p);
    VIVect3 *Vertex(int i) const;
    void SetVertex(int i, const VIVect3 &v);
};

// -------------------------------------------------------------- VIColorBuffer --
// SUPPORT 0x003FB878 .. 0x003FC128.  The baked per-vertex lighting buffer:
// a GS display list built alongside a VIPrimBuffer, so its layout is read out
// of CalcDataSize, which lays three sections down in the one allocation and
// then Init rebases the three offsets onto m_data in place -- VISurface's
// m_header trick, at 32 bits.
class VIColorBuffer : public VIBuffer
{
public:
    int   m_locked;
    int   m_primBuffer;
    int   m_pad10;
    int   m_vertCursor;
    int   m_numVerts;
    int   m_pad1C;
    int   m_numTags;
    char *m_headers;
    char *m_verts;
    char *m_tags;
    int   m_dataSize;
    char *m_data;

    static VIPrimBuffer *pBuildSource;
    static unsigned int *pBuildDMATag;
    static int           BuildVQSource;
    static int           BuildI;
    static int           BuildM;
    static int           BuildMLeft;
    static int           BuildP;
    static int           BuildPLeft;

    VIColorBuffer();
    ~VIColorBuffer();

    int Init(int i, VIRaster &r);
    int Clear(VIRaster &r, VIDictionary &d);
    int Lock(VIRaster &r);
    int Unlock();
    int Color(VIColor32 &c);
    int Clear();
    int CalcDataSize(VIPrimBuffer &pb);
    int Allocate();
};


// ---------------------------------------------------------------- VIFile --
// SUPPORT 0x003E5E88 .. 0x003E9140 (with the Memory* half at 0x003E8A28).
//
// One class with four back ends behind one interface, selected per call by
// which member is non-null: a real CD/host file (m_fd >= 0, buffered, with a
// sector cache and an asynchronous sceCdRead path), a flat memory image
// (m_memory), a scatter-gather list of memory segments (m_segms), and nothing.
// Every dispatcher is the same three-way ladder, and Read/Write/Seek/Eof all
// spell it out again rather than sharing a helper.
//
//   +0x00 m_fd            sceOpen handle; -1 when this is not a real file
//   +0x04 m_flags         the open mode; bits 0/1/2 are the access modes and
//                         at least one must be set, 8 truncates, 0x10 seeks to
//                         the end, 0x40 asks for the 32 KB read-ahead buffer
//   +0x08 m_status        0 ok, 1 at end of data -- what Eof() returns
//   +0x10 m_position      the logical file position, 64-bit
//   +0x18 m_osPosition    where the OS handle actually is, so BufferFlush can
//                         skip the lseek when the write is already contiguous
//   +0x20 m_memSize       memory back end: the capacity
//   +0x28 m_memLength     memory back end: the valid length (grows on write)
//   +0x30 m_memory        memory back end: the base pointer
//   +0x34 m_numSegms      segment back end: how many
//   +0x38 m_segms         segment back end: the array (owned, delete[]d)
//   +0x3C m_segm          segment back end: the current index
//   +0x40 m_segmOffset    segment back end: the offset within it
//   +0x48 m_cdFile        the sceCdSearchFile result -- +0x4C is its size, and
//                         that is what Seek_End measures against
//   +0x6C m_cdMode        trycount 20, spindlctrl 1, datapattern 0
//   +0x70 m_cdPending     a read is in flight; +0x74..+0x80 are its arguments,
//                         kept so CDSync can reissue it after a disc error
//   +0x88 m_bufBase       file offset the read-ahead buffer starts at
//   +0x90 m_dirtyStart    the unwritten window inside the buffer
//   +0x98 m_dirtyEnd
//   +0xA0 m_bufPos        the read cursor inside the buffer
//   +0xA8 m_bufEof        this buffer runs to the end of the file
//   +0xB0 m_bufLength     valid bytes in the buffer
//   +0xB8 m_buffer        the allocation, +0xBC the 16-byte aligned pointer
//   size 0xC0
struct sceCdlFILE
{
    unsigned int  lsn;
    unsigned int  size;
    char          name[16];
    unsigned char date[8];
};

struct sceCdRMode
{
    unsigned char trycount;
    unsigned char spindlctrl;
    unsigned char datapattern;
    unsigned char pad;
};

class VIFile
{
public:
    // Seek_Cur is 0 and Seek_Set is 1: every Seek tests 1 first, then zero,
    // then 2, and anything else is -1.
    enum SeekOrigin { Seek_Cur = 0, Seek_Set = 1, Seek_End = 2 };

    // The caller's description of one memory segment.  16 bytes: the whole of
    // it is copied into the internal record with two ld/sd pairs, which is
    // what says m_addr sits at +8 of a 16-byte aligned POD rather than being
    // written out field by field.
    struct MemSegm
    {
        long  m_size;
        void      *m_addr;
    };

    // The internal record, 24 bytes -- MemSegm with the running file offset in
    // front of it.  `MemorySegmOpen` news m_numSegms * 24 and every walk of it
    // multiplies the index by a literal 24 held in a register.
    struct Segm
    {
        long m_start;
        MemSegm   m_segm;
    };

    int         m_fd;
    int         m_flags;
    int         m_status;
    long   m_position;
    long   m_osPosition;
    long   m_memSize;
    long   m_memLength;
    char       *m_memory;
    int         m_numSegms;
    Segm       *m_segms;
    int         m_segm;
    long   m_segmOffset;
    sceCdlFILE  m_cdFile;
    int         m_cdPad;
    sceCdRMode  m_cdMode;
    int         m_cdPending;
    unsigned    m_cdLsn;
    unsigned    m_cdSectors;
    void       *m_cdBuffer;
    sceCdRMode *m_cdReadMode;
    long   m_bufBase;
    long   m_dirtyStart;
    long   m_dirtyEnd;
    long   m_bufPos;
    int         m_bufEof;
    long   m_bufLength;
    char       *m_buffer;
    char       *m_bufferAligned;

    VIFile();
    ~VIFile();

    static void EnableIopStreaming(int b);
    static int  IsIopStreamingEnabled();

    int  Open(const char *path, int flags);
    int  Open(const void *p, long size, int flags, long len);
    int  Open(int nSegms, MemSegm *segms, int flags, long len);
    int  Close();

    int       Seek(long offset, SeekOrigin origin);
    long Tell();
    int       Flush();
    int       Eof();
    int       Read(void *dst, int n);
    int       Write(const void *src, int n);

    int BufferFlush();
    int BufferedRead(void *dst, int n);
    int BufferedReadCopy(void *dst, int n);
    int BufferedWrite(const void *src, int n);
    int BufferedSeek(long offset, SeekOrigin origin);
    int BufferClear();

    int AsynchAlignment();
    int ReadAsynch(long offset, void *dst, int n);
    int WriteAsynch(long offset, void *src, int n);
    int IsAsynchComplete();

    int CDDiskReady(int mode);
    int CDSearchFile(sceCdlFILE *f, const char *name);
    int CDSync(int mode);
    int CDRead(unsigned lsn, unsigned sectors, void *buf, sceCdRMode *mode);

    int MemoryOpen(const void *p, long size, int flags, long len);
    int MemorySeek(long offset, SeekOrigin origin);
    int MemoryRead(void *dst, int n);
    int MemoryWrite(const void *src, int n);

    int MemorySegmOpen(int nSegms, MemSegm *segms, int flags, long len);
    int MemorySegmSeek(long offset, SeekOrigin origin);
    int MemorySegmRead(void *dst, int n);
    int MemorySegmWrite(const void *src, int n);
    int MemorySegmCalcSegm();

    static int sIopStream;
};

// -------------------------------------------------------------- VIWadFile --
// SUPPORT 0x003EDE70 .. 0x003EE220.  A flat archive over a VIFile: a 12-byte
// header (magic, version, count) followed by `count` 16-byte directory entries
// and then the data, which starts 256 bytes in.  Lookup is a LINEAR walk over
// a case-folded string hash -- there is no ordering and no bucket table.
//
//   +0xC0 m_numEntries    read straight out of the header
//   +0xC4 m_entries       new WadEntry[m_numEntries]
class VIWadFile : public VIFile
{
public:
    struct WadEntry
    {
        unsigned int hash;
        int          offset;
        int          size;
        int          pad;
    };

    int       m_numEntries;
    WadEntry *m_entries;

    VIWadFile();

    int Init(const char *path);
    int Init(void *p, int size);
    void Free();
    int GetSize(const char *name);
    int Load(const char *name, void *dst, int size);
    int SharedInit();

    WadEntry *LookupEntry(const char *name);
    unsigned int HashStringLowercase(const char *name);

    static unsigned int WadFileID;
};

// ------------------------------------------------------------- VIObjFile --
// SUPPORT 0x003E9C80 .. 0x003EC1D8.  The ESF/CSF container: a 32-byte file
// header followed by a tree of 12-byte tagged objects, each of which may hold
// sub-objects.  Nothing here is polymorphic either -- it derives from VIFile
// and calls the base by name.
//
//   +0xC0  m_mode        the FileMode Open() was given
//   +0xC8  m_hdr         32 bytes, written verbatim at offset 0
//   +0xE8  m_level       index into m_dirs; -1 when no object is open
//   +0xF0  m_dirs[10]    the open-object stack, 32 bytes each
//   +0x230 the eight state flags
//
// sizeof is 592, measured off Append()'s stack frame, which holds a whole
// VIObjFile as a local.
class VIObjFile
{
public:
    enum FileMode { Mode_Read = 0, Mode_Update = 1, Mode_Create = 2 };

    // The 32-byte file header.  m_headerSize and m_dirOffset are `long`, i.e.
    // eight bytes -- WriteFileHeader hands them to VIBufferSwap as 8 and the
    // stores are `sd`.
    struct Header
    {
        unsigned int m_magic;        // +0x00  'OBJF'
        int          m_numObjects;   // +0x04
        unsigned int m_fileType;     // +0x08
        int          m_pad;          // +0x0C
        long         m_headerSize;   // +0x10  always 32
        long         m_dirOffset;    // +0x18  -1 when there is no directory
    };

    // The 12 bytes that go on the wire in front of every object.  Copy() reads
    // one into a stack local, which is what says it is a type of its own.
    struct Obj
    {
        unsigned short m_type;
        unsigned short m_version;
        unsigned int   m_size;
        unsigned int   m_numSubObjects;
    };

    // One level of the open-object stack.  32 bytes: the wire record, the file
    // offset its header was written at, and -- on the read side -- how many
    // bytes of it are left.
    struct Dir
    {
        Obj  m_obj;
        long m_offset;
        int  m_remain;
    };

    VIFile m_file;
    int    m_mode;
    Header m_hdr;
    int    m_level;
    Dir    m_dirs[10];
    int    m_open;
    int    m_write;
    int    m_read;
    int    m_writeOpen;
    int    m_readOpen;
    int    m_canWrite;
    int    m_canRead;
    int    m_dirty;

    VIObjFile();
    ~VIObjFile();

    int ResetState();

    int Open(const char *path, FileMode mode);
    int Open(void *p, long size, long len, FileMode mode);
    int Open(int nSegms, VIFile::MemSegm *segms, long len, FileMode mode);
    int PostOpen();
    int OpenObject(const void *p, unsigned int size);
    int AbortOpen();
    int Close();
    int Flush();

    int NumObjects();
    unsigned int FileType();
    int FileType(unsigned int t);
    int Append(const char *path);

    int NumSubObjects();
    unsigned int ObjectVersion();
    unsigned int ObjectType();
    unsigned int ObjectSize();

    int WriteFileHeader(int restore);
    int ReadFileHeader(int restore);

    int WriteBegin(unsigned short type, unsigned short version);
    int WriteBegin(unsigned short type, unsigned short version, unsigned long &offset);
    int WriteDirBegin(unsigned short type, unsigned short version, unsigned long &offset);
    int WriteEnd();
    int WriteEnd(unsigned int &size);

    int Write(char v);
    int Write(signed char v);
    int Write(unsigned char v);
    int Write(short v);
    int Write(unsigned short v);
    int Write(int v);
    int Write(unsigned int v);
    int Write(long v);
    int Write(unsigned long v);
    int Write(float v);
    int Write(double v);
    int Write(const unsigned char *p, int n);
    int WriteString(const char *s);

    int ReadBegin(unsigned short &type, unsigned short &version);
    int ReadBegin(unsigned short &type, unsigned short &version,
                  unsigned long &offset, unsigned int &size);
    int ReadBegin(unsigned long offset, unsigned short &type, unsigned short &version);
    int ReadDirBegin(unsigned short &type, unsigned short &version);
    int ReadEnd();

    int Read(char &v);
    int Read(signed char &v);
    int Read(unsigned char &v);
    int Read(short &v);
    int Read(unsigned short &v);
    int Read(int &v);
    int Read(unsigned int &v);
    int Read(long &v);
    int Read(unsigned long &v);
    int Read(float &v);
    int Read(double &v);
    int Read(unsigned char *p, int n);
    int ReadString(char *dst, unsigned short max);
    int ReadStringLength(unsigned short &len);
    int ReadStringBody(char *dst, unsigned short len);
    int ReadStringBody(char *dst, unsigned short len, unsigned short max);

    int Copy(VIObjFile &src, unsigned long &offset, unsigned int &size);
    int Copy(VIObjFile &src, unsigned long srcOffset, unsigned long &offset, unsigned int &size);

    static unsigned int ObjFileID;
};


// ----------------------------------------------------------------- VIArray --
// The engine's second template, and the plainest container in it: a size and a
// heap block, with no growth and no element count -- every slot is live from
// Init() to Clear().  Definitions and the explicit instantiations live in
// viarray.cpp; the declaration is here because VIClipStack has five.
//
//     +0x00 m_size    element count, set by Init() BEFORE Allocate()
//     +0x04 m_data    new T[m_size]
//
// Begin/End/Next are the VIPool walk again -- End() is 0x7FFFFFFF, Begin() is
// End() on an empty array -- and they are the reason DestroyPool survives as a
// loop even for T = int, where its body constructs nothing at all: gcc 2.95
// has no dead-loop elimination, so an empty body leaves the counter and the
// branch behind.
template <class T>
class VIArray
{
public:
    int m_size;     // +0x00
    T  *m_data;     // +0x04

    VIArray();
    ~VIArray();

    // Inline in the class body: no End/Begin/Next symbol exists for ANY of the
    // twenty-two instantiations, where VIPool's three are emitted out of line.
    // Phase 51.  An inline subscript, and it is NOT cosmetic: the world and
    // zone descents address the node array as `addiu <t>, this, 52` followed by
    // `lw <d>, 4(<t>)`, where `m_tree.m_nodes.m_data[node]` written out folds to
    // a single `lw <d>, 56(this)`.  An inline member's `this` is an address
    // pseudo (phase 32), and that pseudo is what the fourteen descents keep in
    // a callee-saved register across their recursive calls.
    T &operator[](int i) const { return m_data[i]; }
    // ... and the block itself.  `&a[0]` folds the pseudo away again (gcc
    // simplifies `&*(m_data + 0)` back to the member and then folds the base);
    // an accessor that names m_data does not, which is what the three
    // SetPreTranslations call sites need.
    T *Data() const { return m_data; }

    int End() const { return 0x7FFFFFFF; }
    int Begin() const { if (m_size == 0) return End(); return 0; }
    int Next(int i) const { if (i != m_size - 1) return i + 1; return End(); }

    int DestroyPool();
    int Clear();
    int Allocate();
    int Init(int n);
    int Init(const VIArray<T> &a);
};

// ---------------------------------------------------------------------------
// VIRadialFloraSystem -- SUPPORT 0x0045E000 .. 0x0045F7A0, its own TU.  The
// grass and flowers drawn around the camera: a 3x3 grid of SWATCHES, each one
// a zone tile's worth of procedurally placed instances, rebuilt as the player
// walks.  `claude-notes/radial-flora-system.md` has the semantics; the object
// code adds the shapes, and corrects that note once -- the swatch grid is
// NINE POINTERS AND A TENTH SLOT (the constructor's loop and both release
// loops run `li 8` down THROUGH zero, so ten iterations, +0x20..+0x44).
//
//   +0x00  VIScene   *m_scene
//   +0x04  VIRaster  *m_raster      scene+0x4BA0
//   +0x08  VICollide *m_collide     scene+0x4BA8
//   +0x0C  int        m_zone        -1
//   +0x10  int        m_row, m_row2, m_col, m_col2
//   +0x20  VIRadialFloraSwatch *m_swatches[9]
//   +0x44  int        m_numSorted
//   +0x48  VIRadialFloraSort m_sorted[700]
//   +0x2118 int       m_rebuild
// ---------------------------------------------------------------------------
class VIRadialFloraInst;

// 0x34 bytes, which is what CreateSwatch hands __builtin_new: a bounding box,
// the zone address it was built for, a seed, and the instance array whose
// destructor every release path calls at +0x2C.
class VIRadialFloraSwatch
{
public:
    VIVect3 m_min;      // +0x00
    VIVect3 m_max;      // +0x0C
    int     m_row;      // +0x18
    int     m_col;      // +0x1C
    int     m_subRow;   // +0x20
    int     m_subCol;   // +0x24
    int     m_seed;     // +0x28
    VIArray<VIRadialFloraInst> m_insts;   // +0x2C
};

// Six ints.  Only the first three are pinned by the functions in this phase:
// CalcZoneIndex hands +0x08 and +0x04 to VIScene::GetWorldGridCenter and takes
// the leaf back through +0x00.
class VIRadialFloraLoc
{
public:
    int m_zone;         // +0x00
    int m_row;          // +0x04
    int m_col;          // +0x08
    int m_subRow;       // +0x0C
    int m_subCol;       // +0x10
    int m_slot;         // +0x14
};

class VIRadialFloraSort
{
public:
    float m_dist;       // +0x00  the qsort key
    int   m_a;          // +0x04
    int   m_b;          // +0x08
};

class VIRadialFloraSystem
{
public:
    VIRadialFloraSystem();

    int  Init(VIScene &sc);
    int  Clear();
    // void, not int: nothing writes v0 on any path (phase 32).
    void Reset();
    int  ReleaseSwatch(int i);
    int  CalcZoneIndex(VIRadialFloraLoc &loc);

    static int QSortCompare(const void *a, const void *b);

    VIScene   *m_scene;                 // +0x00
    VIRaster  *m_raster;                // +0x04
    VICollide *m_collide;               // +0x08
    int        m_zone;                  // +0x0C
    int        m_row;                   // +0x10
    int        m_row2;                  // +0x14
    int        m_col;                   // +0x18
    int        m_col2;                  // +0x1C
    VIRadialFloraSwatch *m_swatches[9]; // +0x20
    int        m_numSorted;             // +0x44
    VIRadialFloraSort m_sorted[700];    // +0x48
    int        m_rebuild;               // +0x2118
};



// -------------------------------------------------------------- VIClipStack --
// The portal/frustum clip stack.  Its layout is readable straight off
// GetEntry(): the entry array is indexed by 16, the vertex array by 12 and the
// plane array by 16, and the three running totals sit one word IN FRONT of the
// VIArray they count -- so the class is three (count, array) pairs and two
// scratch vertex arrays.
class VIClipEntry
{
public:
    int m_numPlanes;    // +0x00
    int m_planeIndex;   // +0x04
    int m_numVerts;     // +0x08
    int m_vertIndex;    // +0x0C
};

class VIClipStack
{
public:
    int                  m_numEntries;  // +0x00
    VIArray<VIClipEntry> m_entries;     // +0x04   500
    int                  m_numVerts;    // +0x0C
    VIArray<VIVect3>     m_verts;       // +0x10  2000
    int                  m_numPlanes;   // +0x18
    VIArray<VIPlane>     m_planes;      // +0x1C  2000
    VIArray<VIVect3>     m_clipA;       // +0x24    32
    VIArray<VIVect3>     m_clipB;       // +0x2C    32

    int Init();
    int Clear();
    int Reset();
    int Pop();
    int GetEntry(int i, int &numVerts, VIVect3 **verts,
                 int &numPlanes, VIPlane **planes);
    int GetTop(int &numVerts, VIVect3 **verts,
               int &numPlanes, VIPlane **planes);
    int CullByTop(const VIBBox &b);
    int CullByTop(const VISphere &s);
    int PushTranslatedVolume(int numVerts, VIVect3 *verts,
                             int numPlanes, VIPlane *planes,
                             const VIVect3 &t);
    int ClipByTop(int numVerts, VIVect3 *verts, int &numOut, VIVect3 **out);
    int PushBBox(VIBBox &b);
    int PushFrustum(VICamera &cam);
    int PushPortal(VICamera &cam, int n, VIVect3 *p);
};

// ----------------------------------------------------------- VIHSpriteAnim --
// SUPPORT 0x0044D258 .. 0x0044D828.  One skeletal animation clip: a node table,
// a frame table and a trigger table, carved out of ONE allocation the way
// VICollBuffer and VISurface carve theirs -- the three pointers hold byte
// OFFSETS until Init() rebases them onto m_data.
//
//   +0x00 m_id  +0x04 m_userData        (VIBuffer)
//   +0x08 m_format      +0x0C m_numNodes
//   +0x10 m_numFrames   +0x14 m_numTriggers
//   +0x18 m_playRate    +0x1C m_playSpeed
//   +0x20 m_playbackType
//   +0x24 m_nodes       +0x28 m_frames      +0x2C m_triggers
//   +0x30 m_dataSize    +0x34 m_data
//
// The frame side is already settled from the console: `go-eqoa-vi/vianim`
// reads exactly these two frame formats out of a live game, with the same two
// dequantisation scales (1/32767 for the rotation, 1/512 for everything else).
enum VIHSpriteAnimFormat
{
    VIHSpriteAnimFormatFrame = 0,
    VIHSpriteAnimFormatPack  = 1
};

// The four values ProcessPlayTiming tests.  0 is anything that is not one of
// the other three, so it is the loop.
enum VIPlaybackType
{
    VIPlaybackTypeLoop    = 0,
    VIPlaybackTypeOnce    = 1,
    VIPlaybackTypeStopped = 2,
    VIPlaybackTypeStopEnd = 3
};

// 32 bytes.  SetNodeFrame copies it in three statements and that is what says
// where the members are: the quaternion goes as one unaligned 16-byte block
// move (VIQuat has no user operator=), the scale as one word, and the
// translation component by component through a pointer at +0x14 (VIVect3 does
// have one -- Phase 4).
class VIHSpriteFrame
{
public:
    VIQuat  m_rot;      // +0x00
    float   m_scale;    // +0x10
    VIVect3 m_trans;    // +0x14

    // A user operator=, and the tell is VIPool<VIHSpritePlayNode>::Allocate:
    // the implicit one copies m_rot with a 16-byte ldl/ldr block move (which
    // is what VIHSpriteAnim::SetNodeFrame's `d->m_rot = f.m_rot` still does,
    // by hand), where the target copies the rotation and the scale as five
    // floats off one pointer and the translation as three off another.
    VIHSpriteFrame &operator=(const VIHSpriteFrame &f)
    {
        m_rot.x = f.m_rot.x;
        m_rot.y = f.m_rot.y;
        m_rot.z = f.m_rot.z;
        m_rot.w = f.m_rot.w;
        m_scale = f.m_scale;
        m_trans = f.m_trans;
        return *this;
    }

    // The identity pose, and every word of it is written -- VIQuat's own
    // zeroing constructor has already run over x/y/z, and this writes them
    // again.  Read off VIHSpriteNode's constructor, where all three frames
    // show the same double pass.
    VIHSpriteFrame()
    {
        m_rot.x = 0.0f;
        m_rot.y = 0.0f;
        m_rot.z = 0.0f;
        m_rot.w = 1.0f;
        m_scale = 1.0f;
        // `Zero()`, not three component stores: the member call materialises
        // &m_trans and folds its +0 back onto the frame's base, which is
        // Phase 13's VIBBox lever.  Written out, all three land on the
        // rotation's base register -- 59 of 129 words in
        // VIVector<VIHSpriteNode>::Add, where this constructor is inlined.
        m_trans.Zero();
    }
};

// Four packed shorts.  The user operator= is what makes a copy four halfword
// moves instead of an unaligned block one -- VIVect3S16's lever (Phase 13) at
// one component more.
class VIVect4S16
{
public:
    short x, y, z, w;

    VIVect4S16 &operator=(const VIVect4S16 &v)
    { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }
};

// 16 bytes, and NOT the same shape as VIHSpriteFrame: the packed form does not
// separate the scale from the translation.  Both are quantised by the same
// 1/512, and the object code says so twice -- SetNodeFrame copies +0x08..+0x0E
// through ONE pointer at +8 (the implicit operator= calling VIVect4S16's), and
// GetNodeFrame walks the same four as a run.  `go-eqoa-vi/vianim` reads the
// fields out of a live game as scale, trans.x, trans.y, trans.z, in that order.
class VIHSpritePackFrame
{
public:
    VIVect4S16 m_rot;         // +0x00
    VIVect4S16 m_scaleTrans;  // +0x08  x = scale, yzw = translation
};

// 8 bytes, indexed by `sll 3` everywhere: the id the sprite's node table is
// matched against, and the index of this node's first frame.  Init fills the
// second field in and nothing else ever writes it.
class VIHSpriteAnimNode
{
public:
    int m_refID;        // +0x00
    int m_firstFrame;   // +0x04
};

// 8 bytes: a frame coordinate and a trigger id.  Same shape `go-eqoa-vi`'s
// AnimTrigger reads off the live animation at +0x2C.
class VIHSpriteKeyframe
{
public:
    float m_frame;      // +0x00
    int   m_id;         // +0x04
};

class VIHSpriteAnim : public VIBuffer
{
public:
    VIHSpriteAnimFormat m_format;        // +0x08
    int                 m_numNodes;      // +0x0C
    int                 m_numFrames;     // +0x10
    int                 m_numTriggers;   // +0x14
    float               m_playRate;      // +0x18
    float               m_playSpeed;     // +0x1C
    VIPlaybackType      m_playbackType;  // +0x20
    VIHSpriteAnimNode  *m_nodes;         // +0x24
    char               *m_frames;        // +0x28
    VIHSpriteKeyframe  *m_triggers;      // +0x2C
    int                 m_dataSize;      // +0x30
    char               *m_data;          // +0x34

    VIHSpriteAnim();
    ~VIHSpriteAnim();

    int   Init(VIHSpriteAnimFormat format, int numNodes, int numFrames,
               int numTriggers, float playRate, float playSpeed,
               VIPlaybackType type);
    int   Clear();
    int   GetNumFramesPerNode() const;
    float GetPlayRate() const;
    int   SetPlayRate(float r);
    float GetPlaySpeed() const;
    int   SetPlaySpeed(float s);
    VIPlaybackType GetPlayMode() const;
    int   SetPlaybackType(VIPlaybackType t);
    int   GetNodeRefID(int node);
    int   SetNodeRefID(int node, int id);
    int   SetNodeFrame(int node, int frame, VIHSpriteFrame &f);
    int   SetNodeFrame(int node, int frame, VIHSpritePackFrame &f);
    int   GetNodeFrame(int node, int frame, VIHSpriteFrame &f);
    int   SetKeyframe(int i, int id, float frame);
    int   CalcDataSize();
    int   Allocate();
};

// 16 bytes zeroed as `sd` at +8 plus two words -- Phase 23's `long` is EIGHT
// bytes, said by a constructor this time.  The store hoisted to the front is
// the LAST one, so the source order is +0, +4, +8.
class VIHSpriteTrigger
{
public:
    int  m_a;
    int  m_b;
    long m_c;

    VIHSpriteTrigger() { m_a = 0; m_b = 0; m_c = 0; }
};

// ---------------------------------------------------------------- VIVector --
// The engine's THIRD template and its only growable array: VIArray sizes once
// and throws the block away, VIPool is an intrusive free list, and this one
// doubles.
//
//   +0x00 m_size        the live element count
//   +0x04 m_capacity    the allocated element count
//   +0x08 m_data
//
// Allocate() is the growth step and it is written to be called on a vector
// that already holds elements: it takes a SECOND block, copy-constructs the
// live ones across, destroys and frees the old, and installs the new.
template <class T> class VIVector
{
public:
    int m_size;         // +0x00
    int m_capacity;     // +0x04
    T  *m_data;         // +0x08

    VIVector();
    ~VIVector();

    // Inline in the class body, as VIArray's are: no Begin/End/Next symbol
    // exists for any of the four instantiations.
    int End() const { return 0x7FFFFFFF; }
    int Begin() const { if (m_size == 0) return End(); return 0; }
    int Next(int i) const { if (i != m_size - 1) return i + 1; return End(); }

    int DestroyPool();
    int Clear();
    int Allocate();
    int Init(int n);
    int Init(const VIVector<T> &a);
    int Add();
    int Add(const T &v);
};

// 12 bytes, no constructor and no destructor: VIVector<VISceneDisplayElem>'s
// Allocate copies it with an unaligned eight-byte block move plus one word,
// and its DestroyPool body is empty.
class VISceneDisplayElem
{
public:
    // The qsort comparator: -1 / 0 / 1 over three keys in turn, with a sentinel
    // of -1 in the second word sorting FIRST.
    static int Compare(const void *a, const void *b);

    int m_a, m_b, m_c;
};

// 256 bytes, and its whole layout is its own constructor, inlined into
// VIVector<VIHSpriteNode>::Add(): three VIHSpriteFrames (each constructed
// twice -- the member zeroing constructors, then the identity body), two
// VIMatrix44s (each constructed twice as well -- VIVect4[4] then the VU0
// `vsub`+four `sqc2`, Phase 24's VIRadialFloraInst pattern), one VIVect3, and
// four ints written LAST.  The tree links agree with what VIHSprite::First,
// ::Next and ::Parent read: child at +4, sibling at +8, parent at +0.
class VIHSpriteNode
{
public:
    int            m_parent;      // +0x000  -1
    int            m_child;       // +0x004  -1
    int            m_sibling;     // +0x008  -1
    // The detail level the node survives to: ProcessPlayList animates it only
    // while the level it was called with is strictly LESS than this.
    int            m_lod;         // +0x00C   0
    // Phase 47 corrects the two frame names, which were the wrong way round:
    // SetNodeBindPose (0x00446058) and GetNodeBindPose write and read +0x7C,
    // +0x8C, +0x90 -- so the BIND pose is the second frame, not the first, and
    // `go-eqoa-vi/vianim`'s console-read map (NodeRot 0x10, NodeBindRot 0x7C)
    // is the one that agrees.  +0xA0 is not a second world matrix either: it is
    // the INVERSE of the node's world bind transform, which is what
    // SetNodeBindPose builds there with VIMatrix44::Invert and then multiplies
    // the child's world bind by to recover a local rest transform.
    VIHSpriteFrame m_frame;       // +0x010  the animated local transform
    VIMatrix44     m_world;       // +0x030  the animated output ("bone + 48")
    VIVect3        m_fixScale;    // +0x070
    VIHSpriteFrame m_bind;        // +0x07C
    VIMatrix44     m_invBindWorld;// +0x0A0
    VIHSpriteFrame m_rest;        // +0x0E0

    VIHSpriteNode()
    {
        m_parent = -1;
        m_child = -1;
        m_sibling = -1;
        m_lod = 0;
    }
};

// ---------------------------------------------------------------- VIHSprite --
// SUPPORT 0x00445AF0 ..  The skinned/hierarchical sprite: a node tree, a
// palette of blend matrices, a trigger table, and three pools -- attachments,
// plays, and the per-node entries a play owns.  Clear() names all six in
// order, and their offsets are read off the accessors below.
//
//   +0x40 VIVector<VIHSpriteNode>           m_nodes      (size +0x40, data +0x48)
//   +0x4C VIVector<VIMatrix44>              m_blend
//   +0x58 VIArray<VIHSpriteTrigger>         m_triggers   (size +0x58, data +0x5C)
//   +0x60 VIPool<VIHSpriteAttachment>       m_attach     (data +0x68)
//   +0x78 VIPool<VIHSpritePlay>             m_plays      (data +0x80)
//   +0x90 VIPool<VIHSpritePlayNode>         m_playNodes
//   +0xD0 float m_defaultScale   +0x180 int  +0x184 int   (Clear sets -1/1.0/-1)
//
// The three element sizes come from the pools' own Allocate: the attachment
// node is 16 bytes so the element is 8, the play node 104 so the play is 96 --
// which is what `go-eqoa-vi/vianim` measured off the console -- and the play
// node 52, so its element is 44.
class VIHSpriteAttachment
{
public:
    int m_sprite;   // +0x00
    int m_node;     // +0x04
};

// 96 bytes, and the field map is `go-eqoa-vi/vianim`'s -- read off a live
// game, and confirmed here by an alignment the decomp could not otherwise get:
// the three 64-bit clock fields give the class alignment 8, and without them
// gcc guards the 104-byte node copy in Allocate with a runtime `(src|dst)&7`
// test that the target does not have (48 of 100 words).
class VIHSpritePlay
{
public:
    int   m_animID;         // +0x00
    int   m_playbackType;   // +0x04
    int   m_interpolate;    // +0x08
    int   m_blendMode;      // +0x0C
    long  m_blendStart;     // +0x10
    long  m_blendDuration;  // +0x18
    float m_weight;         // +0x20
    // The rate the caller SETS; PrecalcPlayback derives the frames-per-ms at
    // +0x48 from it, which is the one vianim named Rate.
    float m_rate;           // +0x24
    float m_speed;          // +0x28
    int   m_frameA;         // +0x2C
    int   m_frameB;         // +0x30
    float m_timeAccum;      // +0x34
    float m_frameT;         // +0x38
    int   m_pad3c;          // +0x3C
    long  m_lastTime;       // +0x40
    float m_frameRate;      // +0x48
    float m_frameDuration;  // +0x4C
    int   m_numNodes;       // +0x50
    int   m_firstNode;      // +0x54
    int   m_pad58;          // +0x58
    int   m_pad5c;          // +0x5C
};


// 44 bytes, and its constructor -- inlined into VIPool<VIHSpritePlayNode>::Add
// -- is nothing but a VIHSpriteFrame's: three uninitialised ints and then the
// same double pass (VIQuat's and VIVect3's zeroing constructors, then the
// identity body) at +0x0C.
class VIHSpritePlayNode
{
public:
    int            m_a;      // +0x00
    int            m_b;      // +0x04
    int            m_c;      // +0x08
    VIHSpriteFrame m_frame;  // +0x0C
};

class VIHSpriteLOD
{
public:
    int   m_num;
    float m_dist[3];

    VIHSpriteLOD() { m_num = 0; }
};

class VIHSprite : public VISprite
{
public:
    VIVector<VIHSpriteNode>     m_nodes;      // +0x40
    VIVector<VIMatrix44>        m_blend;      // +0x4C
    VIArray<VIHSpriteTrigger>   m_triggers;   // +0x58
    VIPool<VIHSpriteAttachment> m_attach;     // +0x60
    VIPool<VIHSpritePlay>       m_plays;      // +0x78
    VIPool<VIHSpritePlayNode>   m_playNodes;  // +0x90

    int   Init();
    int   Clear();
    int   Size();
    int   End();
    int   Root();
    int   First(int i);
    int   Last(int i);
    int   Next(int i);
    int   Prev(int i);
    int   Parent(int i);
    int   IsLeaf(int i);

    int   FindTrigger(int id, int &out);
    int   HasTripped(int i);
    int   ResetTrigger(int i);

    int   GetNumAttach() const;
    int   GetFirstAttach() const;
    int   GetNextAttach(int i) const;
    int   GetPrevAttach(int i) const;
    int   GetLastAttach() const;
    int   GetAttachSprite(int i) const;
    int   GetAttachNode(int i) const;

    int   GetPlayAnimation(int i);
    int   SetPlaySpeed(int i, float s);
    float GetPlaySpeed(int i);
    int   SetPlayRate(int i, float r);
    float GetPlayRate(int i);
    int   IsPlaybackStopped(int i);
    int   PrecalcPlayback(int i);

    VIHSprite();
    virtual ~VIHSprite();

    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r);
    virtual int QueryEmitters(VIScene &sc, const VIMatrix44 &m, VIEmitterQuery &q);
    virtual int QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                  const VIMatrix44 &m, VIScene &sc);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    int   AddRoot();
    int   Add(int parent);

    int   SetNodeBindPose(int i, const VIQuat &q, float s, const VIVect3 &t);
    int   GetNodeBindPose(int i, VIQuat &q, float &s, VIVect3 &t);
    int   SetNodeFixScale(int i, const VIVect3 &v);
    int   GetNodeFixScale(int i, VIVect3 &v);
    int   CalcNodeTransform(int i, VIMatrix44 &m, VIScene &sc, VIMatrix44 &out);

    int   AttachSkin(int sprite, VIScene &sc);
    int   Attach(int sprite, int node, VIScene &sc);
    int   Detach(int i, VIScene &sc);

    int   AddPlay(int anim, VIScene &sc);
    int   AddPlay(int anim, VIPlaybackType type, VIScene &sc);
    int   AddPlay(int anim, VIPlaybackType type, float weight, VIScene &sc);
    int   AddPlayWithSync(int anim, int sync, VIScene &sc);
    int   AddInterpolatedPlay(int anim, VIPlaybackType type, float weight,
                              int ms, VIScene &sc);
    int   AddInterpolatedPlayWithSync(int anim, int sync, int ms, VIScene &sc);
    int   ErasePlay(int i, VIScene &sc);
    int   EraseAllPlay(VIScene &sc);
    int   StopPlayback(int i, VIScene &sc);
    int   StopPlaybackAtCycle(int i, VIScene &sc);
    int   FreezePlayback(int i, VIScene &sc);
    int   StopAllPlayback(VIScene &sc);
    int   IsBlendComplete(int i, VIScene &sc);
    int   SetPlayFrame(int i, float frame, VIScene &sc);
    int   SyncTiming(int a, int b, VIScene &sc);

    int   SetCenterTransform(VIVect3 &center, const VIVect3 &hpr, float s,
                             const VIVect3 &t, VIScene &sc);
    int   BlendToCenterTransform(VIVect3 &center, const VIVect3 &hpr, float s,
                                 const VIVect3 &t, int ms, VIScene &sc);
    int   IsCenterTransformBlending(VIScene &sc);
    int   IsCenterTransformBlendComplete(VIScene &sc);
    int   StopCenterTransformBlend(VIScene &sc);
    int   CalcTransAboutCenter(const VIMatrix44 &m, VIScene &sc, VIMatrix44 &out);
    int   UpdateCenterTransform(VIScene &sc);
    int   InterpTransAboutCenter(VIScene &sc, VIVect3 &center, VIHSpriteFrame &f);

    int   Process(int lod, VIScene &sc);
    int   ProcessPlayList(int lod, VIScene &sc);
    int   ProcessPlayTiming(int i, VIScene &sc);
    int   SetBindPose();
    int   ProcessHierarchy();
    int   ProcessHierarchy(int i, VIMatrix44 &m);
    int   ProcessUpHierarchy(int i);
    int   SetBlendMatrices(VIRaster &r);
    int   CalcLODLevelIndex(const VIMatrix44 &m);
    int   CalcLODLevelIndex(VIPickSegment &seg, VIRaster &r);

    int   GetDefaultAnim();
    float GetDefaultScale() const;

    // +0xA8 .. +0x188, read off the constructor (0x00445AF0), Clear
    // (0x00445DB0) and the center-transform group -- and cross-checked against
    // `go-eqoa-vi/vianim`, which measured the same offsets off a live game.
    //
    // The two clocks are DIFFERENT stamps for the same scene time:
    // ProcessPlayList short-circuits on m_playTime and Process on m_poseTime,
    // so a caller reaching ProcessPlayList directly still gets one update per
    // tick.  m_centerTime is the third, for the center-transform cache.
    // The LOD table, read by both CalcLODLevelIndex overloads (0x0044A440,
    // 0x0044A4E0): a count and three SQUARED-on-the-fly distances.
    //
    // It is a CLASS of its own, and two things say so.  Its constructor's one
    // `sw zero` lands between the last pool's constructor call and the
    // m_center array's inline loop, where a body statement could not go -- so
    // the count is constructed, in declaration order, by a member.  And
    // VIHSprite::Copy carries the whole thing across as two unaligned
    // eight-byte block moves, which is the implicit operator= of a class with
    // an array member: written field by field it would be a word and a loop.
    // Sixteen bytes, so the distance array is exactly three long.
    VIHSpriteLOD m_lod;          // +0x0A8
    long       m_playTime;       // +0x0B8  -1
    long       m_centerTime;     // +0x0C0  -1
    long       m_poseTime;       // +0x0C8  -1
    float      m_defaultScale;   // +0x0D0   1.0

    // An ARRAY of two, not two members: the constructor zeroes them in a LOOP
    // (0x00445B50, two turns of twelve bytes), which no pair of separate
    // VIVect3 constructors would give.  [1] is the live center and [0] the one
    // a blend started from.
    VIVect3        m_center[2];  // +0x0D4, +0x0E0

    // The same, at 32 bytes and two turns: [1] is the transform being blended
    // TO and [0] the one it started from.
    VIHSpriteFrame m_transform[2]; // +0x0EC, +0x10C

    int        m_blendMS;        // +0x12C   0 -- no blend in progress
    long       m_blendStart;     // +0x130
    VIMatrix44 m_centerMatrix;   // +0x140
    // Copy shares it with VIScene::ShareRefMap and Release lets it go with
    // ReleaseRefMap, which is what names it.
    int        m_refMap;         // +0x180  -1
    int        m_defaultAnim;    // +0x184  -1  (GetDefaultAnim)
};

class VITrailFx;

class VICapsule
{
public:
    // Phase 46.  Collide(VICollSphere &, VICapsule &) builds one on the stack
    // and the zeroing comes out TWICE for the two vectors and ONCE for the two
    // floats -- the members' own VIVect3() first, then this body inlined.
    VICapsule()
    { m_base.Zero(); m_dir.Zero(); m_length = 0.0f; m_radius = 0.0f; }

    int PointInSolid(const VIVect3 &p);

    VIVect3 m_base;    // +0x00
    VIVect3 m_dir;     // +0x0C  unit axis
    float   m_length;  // +0x18
    float   m_radius;  // +0x1C
};

class VICylinder
{
public:
    VICylinder()
    { m_base.Zero(); m_dir.Zero(); m_length = 0.0f; m_radius = 0.0f; }

    int PointInSolid(const VIVect3 &p);

    VIVect3 m_base;
    VIVect3 m_dir;
    float   m_length;
    float   m_radius;
};

// ---------------------------------------------------------------------------
// VICSprite -- the character sprite, sprite type 5, and the biggest object in
// the corpus at 4,624 bytes.  Phase 53.  It derives from VIHSprite (which
// derives from VISprite), so the downcast GetActorSpritePtr's result needs is
// free.
//
// Its own data starts at +0x190 and NOT at +0x188, where VIHSprite's last
// member ends: VIHSprite carries a VIMatrix44, so it is aligned(16) and its
// SIZE is rounded up.  Eight bytes of the base's tail padding are the reason
// nothing in the class ever touches +0x188.
//
// The layout is read off the constructor and Clear, which between them store
// every field, and then checked against a this-offset census over all fifty
// methods -- nothing outside these members is ever accessed.  The array
// LENGTHS come from the constructor's four zeroing loops (1, 9, 6 and 3
// turns), from Share/Release's `i < 66` and `i < 2` pairs, and from the
// texture-slot walks that stop at `this + 4364`.
// ---------------------------------------------------------------------------

// One animation slot.  Two of everything that plays, because a VICSprite runs
// an upper-body and a lower-body layer at once: Share and Release both walk
// `for (i = 0; i < 66; i++) for (j = 0; j < 2; j++)` over m_animation and
// m_sound, which is what fixes the pair as an ARRAY and not two members.
class VICSpriteAnimEntry
{
public:
    int   m_animation[2];   // +0x00  -1
    float m_weight;         // +0x08   1.0
    int   m_sync;           // +0x0C
    int   m_priority[2];    // +0x10   0
    int   m_sound[2];       // +0x18  -1
    float m_volume;         // +0x20   1.0
    float m_pitch;          // +0x24   1.0
    float m_particle;       // +0x28   phase 60: a FLOAT, from the play list
    float m_trigger;        // +0x2C   phase 60: a FLOAT, from the play list
    int   m_channel;        // +0x30   SetAnimSoundChannels writes only this

    VICSpriteAnimEntry()
    {
        m_animation[1] = -1;
        m_animation[0] = -1;
        m_weight = 1.0f;
        m_sync = 0;
        m_priority[1] = 0;
        m_priority[0] = 0;
        m_sound[1] = -1;
        m_sound[0] = -1;
        m_channel = 0;
        m_pitch = 1.0f;
        m_volume = 1.0f;
        m_trigger = 0.0f;
        m_particle = 0.0f;
    }
};

// One texture slot: a material palette entry, the layer inside it, and the
// texture InitTextSlots caches out of that layer so SetArmorSlot can put it
// back when the armour set is 0.  m_flags bit 0 is "the robe owns this slot",
// which is what makes SetRobe(0, ...) able to restore the armour underneath.
class VICSpriteTextSlotEntry
{
public:
    int m_flags;      // +0x00
    int m_palEntry;   // +0x04  -1
    int m_layer;      // +0x08  -1
    int m_texture;    // +0x0C  -1  the layer's ORIGINAL texture
    int m_armorSet;   // +0x10
    int m_tint;       // +0x14

    VICSpriteTextSlotEntry()
    {
        m_texture = -1;
        m_flags = 0;
        m_palEntry = -1;
        m_layer = -1;
        m_armorSet = 0;
        m_tint = 0;
    }
};

// One held item.  Three of them, one per weapon-bearing attach slot.  No
// constructor: the three are filled in by an ASCENDING loop in the body, where
// every member with one gets gcc's count-down-through--1 vec-init instead.
class VICSpriteItem
{
public:
    // The NODE the item hangs off, not the item: AttachItem hands it straight
    // to VIHSprite::Attach.  Nothing in this class ever writes it -- the ESF
    // parser's CSpriteASlotList does.
    int m_node;        // +0x00  -1
    int m_attackType;  // +0x04  11
    int m_projectile;  // +0x08  -1
};

// The hair triple.  A class and not three ints, because its three stores are
// emitted BETWEEN m_slots' vec-init and VICapsule's constructor -- gcc 2.95
// runs every member constructor, in declaration order, before a line of the
// body, so anything in that run is a member with a constructor of its own.
class VICSpriteHair
{
public:
    int m_hair;   // +0x00  0
    int m_a;      // +0x04  2
    int m_b;      // +0x08  1

    VICSpriteHair() { m_hair = 0; m_a = 2; m_b = 1; }
};

// The one sound a VICSprite has playing, by the same argument: its three
// stores and the eight-byte zero sit between VIColor32F's constructor and
// VIArray's.  Clear does not touch any of them, which is the other half of the
// evidence -- these four fields are initialised ONCE, at construction.
class VICSpriteSoundState
{
public:
    int   m_sound;    // +0x00  -1
    float m_volume;   // +0x04   0
    int   m_channel;  // +0x08  -1
    long  m_time;     // +0x10   0

    VICSpriteSoundState()
    {
        m_sound = -1;
        m_channel = -1;
        m_volume = 0.0f;
        m_time = 0;
    }
};

// One playing layer.  Two arrays of two: m_cur is what is playing now and
// m_prev what it is blending out of, and SetAnimation copies one over the
// other as a sixteen-byte block move.
class VICSpritePlay
{
public:
    int m_anim;   // +0x00   0
    int m_play;   // +0x04  -1
    int m_sync;   // +0x08   0
    int m_flags;  // +0x0C  -1
};

// One particle emitter bound to a node.  1,056 bytes: a sixteen-byte header
// (so the emitter's own VIMatrix44 stays 16-aligned), then the emitter, then
// the binding.  sizeof(VIParticleEmitter) is 992, which VIParticleSprite's
// +0x50 emitter and +0x430 tail already said.
class VICSpritePartEmitter
{
public:
    int               m_definition;   // +0x000  -1
    int               m_unk004;
    int               m_unk008;
    int               m_unk00C;
    VIParticleEmitter m_emitter;      // +0x010
    int               m_oneShot;      // +0x3F0
    int               m_flags;        // +0x3F4  bit 0: player only
    int               m_node;         // +0x3F8
    int               m_useLocation;  // +0x3FC
    int               m_unk400;
    int               m_unk404;
    int               m_life;         // +0x408
    long              m_start;        // +0x410
    char              m_tail[8];      // +0x418

    // Phase 24 measured this constructor off VIArray<T>::Init's inline
    // expansion, from a stub in viarray.cpp; phase 53 keeps it verbatim and
    // moves it here.  It leaves the twelve bytes at +0x004 alone.
    VICSpritePartEmitter()
    {
        m_definition = -1;
        m_oneShot = 0;
        m_flags = 0;
        m_node = -1;
        m_useLocation = 0;
        m_unk400 = 0;
        m_unk404 = 0;
        m_life = 0;
        m_start = 0;
    }
};

class VICSprite : public VIHSprite
{
public:
    VICSprite();
    virtual ~VICSprite();

    int  Init(VICSpriteSkelType skel, VICSpriteRace race, VICSpriteSex sex,
              float scale, int npc);
    int  Clear();

    int  GetSkeletonType();
    int  GetRace();
    int  GetSex();
    int  IsDead();

    int  SetIsPlayer(int on);
    int  SetSkinsVisible(int on, VIScene &sc);

    int  SetDefaults(VIScene &sc);
    int  SetDefaultEntries();
    int  SetAnimPriorities();
    int  SetAnimSoundChannels();
    int  InitTextSlots(VIRaster &r);
    int  CalcLODOffDist(VIScene &sc);

    int  SetAction(VICSpriteAnimID id, VIScene &sc);
    int  SetLocomotion(VICSpriteAnimID id, VIScene &sc);
    int  SetAttackAction(VICSpriteAttachSlot slot, VIScene &sc);
    int  SetAttackAltAction(VICSpriteAttachSlot slot, VIScene &sc);
    int  SetDeath(int a, int b, VIScene &sc);
    int  SetAnimation(VICSpriteAnimID id, int layer, VIScene &sc);
    int  SetItemAction(int layer, VIScene &sc);
    int  IsActionComplete(int i);

    int  AttachItem(int sprite, VICSpriteAttachSlot slot,
                    VICSpriteAttackType type, int projectile, VIScene &sc);
    int  DetachItem(VICSpriteAttachSlot slot, VIScene &sc);
    int  GetItemAttackType(VICSpriteAttachSlot slot);
    int  GetItemProjectile(VICSpriteAttachSlot slot);
    int  ItemHasProjectile(VICSpriteAttachSlot slot);

    int  SetName(const char *name, VICSpriteNameColor color, VIScene &sc);
    int  SetFace(int face, VIScene &sc);
    int  SetHair(int hair, int a, int b, VIScene &sc);
    int  SetRobe(int on, int robe, VICSpriteTint tint, VIScene &sc);
    int  SetHelm(VICSpriteArmorSet set, VICSpriteTint tint, VIScene &sc);
    int  SetArmorSet(VICSpriteArmorSet set, VICSpriteTint tint, VIScene &sc);
    int  SetArmorSlot(VICSpriteArmorSlot slot, VICSpriteArmorSet set,
                      VICSpriteTint tint, VIScene &sc);

    int  SetWeaponTrail(int i, VIColor32F &c);
    int  ProcessWeaponTrails(VICamera &cam, VIScene &sc, VIMatrix44 &m);
    int  CreateParticleEmitters(VIScene &sc);
    int  UpdateParticleEmitters(VICamera &cam, VIScene &sc, VIRaster &r,
                                VIMatrix44 &m);

    int  CalcSlotWorldTransform(VICSpriteNodeID id, int actor, VIScene &sc,
                                VIMatrix44 &m);
    int  CalcVolumeAndPan(VICamera &cam, const VIVect3 &at, float &vol,
                          float &pan);
    int  Process(VICamera &cam, const VIVect3 &at, VIScene &sc);
    int  Share(VIScene &sc, VIRaster *r);

    virtual int GetInstType(VIScene &sc) const;
    virtual int Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags);
    virtual int Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c);
    virtual int QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                  const VIMatrix44 &m, VIScene &sc);
    virtual int Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n);
    virtual int Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                        VISoundDevice *sd, VICollide &c);

    // bit 0 NPC, bit 1 skins visible, bit 2 player.
    int        m_flags;         // +0x190   1
    int        m_skelType;      // +0x194
    int        m_race;          // +0x198   9
    int        m_sex;           // +0x19C
    int        m_materialPal;   // +0x1A0  -1
    int        m_refMap;        // +0x1A4  -1  (its own, not VIHSprite's)
    VICSpriteAnimEntry    m_entries[66];   // +0x1A8
    int        m_body[1];       // +0xF10  -1
    int        m_node[9];       // +0xF14  -1
    VICSpriteTextSlotEntry m_slots[19];    // +0xF38
    VICSpriteHair m_hair;       // +0x1100
    VICSpriteItem m_items[3];   // +0x110C
    int        m_attach[6];     // +0x1130 -1
    VICapsule  m_capsule;       // +0x1148
    int        m_dead;          // +0x1168
    VICSpritePlay m_prev[2];    // +0x116C
    VICSpritePlay m_cur[2];     // +0x118C
    long       m_animTime;      // +0x11B0
    long       m_idleTime;      // +0x11B8
    int        m_trailSlot;     // +0x11C0   1 in the ctor, 0 in Clear
    VIColor32F m_trailColor;    // +0x11C4
    long       m_trailTime;     // +0x11D8
    VITrailFx *m_trail;         // +0x11E0
    int        m_deathAnim;     // +0x11E4  -1
    int        m_action;        // +0x11E8  -1
    int        m_actionSeq;     // +0x11EC   0
    VICSpriteSoundState m_snd;  // +0x11F0
    VIArray<VICSpritePartEmitter> m_emitters;  // +0x1208

    static int PrimSoundRefID;
    static int SecSoundRefID;
    static int PrimSoundBPURefID;
    static int SecSoundBPURefID;
    static int PrimSoundBPLRefID;
    static int SecSoundBPLRefID;
    static int Particle1RefID;
    static int Particle2RefID;
    static int Particle1BPURefID;
    static int Particle2BPURefID;
    static int Particle1BPLRefID;
    static int Particle2BPLRefID;
    static int FontResourceID;
    static unsigned char NameColors[];
};

// ---------------------------------------------------------------------------
// VICameraEffect -- SUPPORT 0x0041DBA8 .. 0x0041E370.  Phase 27.
//
// The camera's shake / manual-override sub-object; VIWalkCamera embeds one at
// +0x28C.  Layout is read straight off the accessors: ManualXyz() returns
// `this + 20` and ManualHpr() `this + 32`, ManualTerp() loads +0x2C, and the
// constructor zeroes exactly +0x14..+0x1C, +0x20..+0x28, +0x2C and +0x30 with
// Reset() covering +0x00..+0x10.
// ---------------------------------------------------------------------------
class VICameraEffect
{
public:
    enum SPEED { SPEED_NONE, SPEED_SLOW, SPEED_MEDIUM, SPEED_FAST };

    VICameraEffect();
    ~VICameraEffect();

    void  Reset();
    void  Shake(SPEED speed, float amp);
    void  ShakeTerp(float amp, float time);

    void  BeginManual(const VIVect3 &xyz, const VIVect3 &hpr, float time);
    const VIVect3 &ManualXyz() const;
    void  SetManualXyz(const VIVect3 &v);
    const VIVect3 &ManualHpr() const;
    void  SetManualHpr(const VIVect3 &v);
    float ManualTerp() const;
    void  EndManual(float time);

    void  Update(float dt);
    void  Apply(VIVect3 &xyz, VIVect3 &hpr);

    SPEED   m_speed;            // 0x00
    int     m_sample;           // 0x04
    float   m_amplitude;        // 0x08
    float   m_amplitudeEnd;     // 0x0C
    float   m_amplitudeRate;    // 0x10
    VIVect3 m_manualXyz;        // 0x14
    VIVect3 m_manualHpr;        // 0x20
    float   m_manualTerp;       // 0x2C
    float   m_manualTerpRate;   // 0x30
};

// ---------------------------------------------------------------------------
// VITimer -- SUPPORT 0x003E8400 .. 0x003E8508.  Phase 27.
//
// All static: EE timer 0 at 18750 ticks an interrupt, INTC cause 9, and a
// 64-bit millisecond counter the whole engine reads through GetTime().  The
// four statics are named in the map (`_7VITimer$SystemTime` is EIGHT bytes,
// which is `long` on this ABI).
// ---------------------------------------------------------------------------
class VITimer
{
public:
    static int  Init();
    static void Close();
    static int  Handler(int ca);
    static long GetTime();

    static long SystemTime;
    static int  TimerMode;
    static int  HandlerID;
    static int  Ready;
};

// ---------------------------------------------------------------------------
// VILanguage -- SUPPORT 0x003E96D8 .. 0x003E9818.  Phase 27.
//
// Three statics and five accessors.  The bound is EIGHT and the compare is
// `sltiu`, so VILanguageID is an unsigned enum; both range asserts pass the
// SAME two strings, so the file name and the expression text are shared -- the
// two calls differ only in the line number, which is what dates the source.
// ---------------------------------------------------------------------------
enum VILanguageID
{
    VILANG_ENGLISH,
    VILANG_FRENCH,
    VILANG_GERMAN,
    VILANG_SPANISH,
    VILANG_ITALIAN,
    VILANG_DUTCH,
    VILANG_PORTUGUESE,
    VILANG_JAPANESE,
    VILANG_COUNT
};

class VILanguage
{
public:
    static const char  *GetName(VILanguageID id);
    static const char  *GetIsoCode(VILanguageID id);
    static int          GetID(const char *name);
    static VILanguageID GetDefault();
    static void         SetDefault(VILanguageID id);

    static const char  *Name[VILANG_COUNT];
    static const char  *IsoCode[VILANG_COUNT];
    static VILanguageID Default;
};

// ---------------------------------------------------------------------------
// VIActorFilter / VISceneFilter -- SUPPORT 0x004167D8 and 0x00470820.  Phase 27.
//
// The scene traversal's accept/reject callbacks, whose SEMANTICS were settled
// on the console first (go-eqoa-vi/vicoll).  Each has exactly ONE virtual:
// `__tf`'s name-string argument is `_vt$13VISceneFilter+0x18`, so the table is
// 24 bytes -- header, one entry, NULL -- and Phase 19's `B/8 - 2` gives one.
// `__tf` takes TWO arguments, so neither has a base class, and the vptr sits at
// the END of the data (Phase 17): +0x04 here and +0x10 there.
// ---------------------------------------------------------------------------
class VIActorFilter
{
public:
    VIActorFilter();

    int Init();
    int Init(int actor);
    virtual int FilterActor(VIScene &scene, int actor);

    int m_actor;
};

class VISceneFilter
{
public:
    VISceneFilter();

    int Init();
    int Init(int actor);
    virtual int FilterActor(VIScene &scene, int actor);

    int GetActor();
    int GetSprite();
    int GetAttachment();

    int m_actor;
    int m_hitActor;
    int m_hitSprite;
    int m_hitAttachment;
};

// ---------------------------------------------------------------------------
// VISceneQuery -- SUPPORT 0x00470900 .. 0x00471150.  Phase 27.
//
// A proximity/visibility query's result set: a cap, a VIVector<int> of actor
// indices and a VIMultiMap<float,int> keyed by distance.  `__tf` takes TWO
// arguments, so it does NOT derive from VISceneFilter -- it re-declares the one
// virtual and puts its own vptr after 0x24 bytes of data.
// ---------------------------------------------------------------------------
class VISceneQuery
{
public:
    int Init();
    int Init(int n);
    int Clear();
    virtual int FilterActor(VIScene &scene, int actor);
    int EndQuery();
    int NumObjects();
    int Object(int i);

    int                      m_maxObjects;   // 0x00
    VIVector<int>            m_objects;      // 0x04
    VIMultiMap<float, int>   m_sorted;       // 0x10
};

// ---------------------------------------------------------------------------
// VIMotifBlender -- SUPPORT 0x004564E0 .. 0x004565A0.  Phase 27.
//
// A VILNode carrying one particle motif and the weight it is blended at.  No
// vtable (the destructor is the only `_$_` and there is no `_vt$`), so the
// three members sit straight on top of the 16-byte node.
// ---------------------------------------------------------------------------
class VIParticleMotif;

class VIMotifBlender : public VILNode
{
public:
    VIMotifBlender();
    ~VIMotifBlender();

    float            Factor() const;
    void             SetFactor(float f);
    VIParticleMotif *Motif();

    VIParticleMotif *m_motif;    // 0x10
    float            m_factor;   // 0x14
    int              m_changed;  // 0x18
};

// ---------------------------------------------------------------------------
// VIEnvRay / VITransEnvRay -- SUPPORT 0x00430FC8 .. 0x00431290.  Phase 27.
//
// The environment-probe ray VIScene::Collide(VIEnvRay&) drives: a segment, its
// direction and length, a surface mask and the hit time along it.  One virtual
// (the vtable name string is at `_vt$8VIEnvRay+0x18`); the derived class adds
// no data and overrides only that one, and its `__tf` is the THREE-argument
// `__rtti_si`, i.e. single inheritance (Phase 17).
// ---------------------------------------------------------------------------
class VIEnvRay
{
public:
    VIEnvRay();

    int Init(const VIVect3 &a, const VIVect3 &b, unsigned int mask);
    virtual int Collide(unsigned int mask, float t, VIVect3 &n);

    VIVect3 GetHitPoint() const;
    float   GetHitTime() const;
    int     IsFirstCollide() const;

    VIVect3      m_start;         // 0x00
    VIVect3      m_end;           // 0x0C
    VIVect3      m_delta;         // 0x18
    float        m_length;        // 0x24
    float        m_invLength;     // 0x28
    unsigned int m_mask;          // 0x2C
    int          m_hits;          // 0x30
    int          m_firstCollide;  // 0x34
    float        m_hitTime;       // 0x38
};

class VITransEnvRay : public VIEnvRay
{
public:
    virtual int Collide(unsigned int mask, float t, VIVect3 &n);
};

// ---------------------------------------------------------------------------
// VIWorldTree / VIZoneTree -- SUPPORT 0x00493308 and 0x00499EC0.  Phase 27.
//
// The world and zone BSP trees go-eqoa-vi's VIScene::Collide walks: two
// VIArrays, nodes and leaves, and nothing else.  The element types stay
// INCOMPLETE here -- VIArray<T> needs T only for `T *m_data`, and the member
// definitions live in viarray.cpp, so neither TU re-instantiates them.
// ---------------------------------------------------------------------------
class VIWorldNode;
class VIWorldLeaf;
class VIZoneNode;
class VIZoneLeaf;

class VIWorldTree
{
public:
    VIWorldTree();
    ~VIWorldTree();

    int Init(int numNodes, int numLeaves);
    int Clear();

    VIArray<VIWorldNode> m_nodes;    // 0x00
    VIArray<VIWorldLeaf> m_leaves;   // 0x08
    int                  m_state;    // 0x10
};

class VIZoneTree
{
public:
    VIZoneTree();
    ~VIZoneTree();

    int Init(int numNodes, int numLeaves);
    int Clear();

    VIArray<VIZoneNode> m_nodes;     // 0x00
    VIArray<VIZoneLeaf> m_leaves;    // 0x08
};

// ---------------------------------------------------------------------------
// VIGameOptions -- SUPPORT 0x004A2A90 .. 0x004A2CE8.  Phase 27.
//
// The 1024-byte memory-card save: login, volumes, language, home shard, and a
// magic at the very end that Load() checks.  Every size is stated outright --
// memset 1024, strncpy 63 with a terminator at +0x4F and +0x8F, strcpy16 with
// 64 -- so the layout is not a guess.
// ---------------------------------------------------------------------------
class VIGameOptions
{
public:
    VIGameOptions();

    int Load();
    int Save();
    void SetDefaults();

    const char *Username() const;
    void SetUsername(const char *s);
    const char *Password() const;
    void SetPassword(const char *s);
    const unsigned short *HomeShardName() const;
    void SetHomeShardName(const unsigned short *s);

    int            m_flags;             // 0x000
    char           m_language;          // 0x004
    char           m_x05;               // 0x005
    char           m_x06;               // 0x006
    char           m_x07;               // 0x007
    float          m_musicVolume;       // 0x008
    float          m_effectVolume;      // 0x00C
    char           m_username[64];      // 0x010
    char           m_password[64];      // 0x050
    char           m_x90[6];            // 0x090
    unsigned char  m_region;            // 0x096
    char           m_x97;               // 0x097
    unsigned short m_homeShard[64];     // 0x098
    char           m_pad[740];          // 0x118
    int            m_magic;             // 0x3FC
};

// ---------------------------------------------------------------------------
// VIConsole -- SUPPORT 0x004A0A88 .. 0x004A0C20.  Phase 27.
//
// One pad and one keyboard behind a single GetInput().  The two sub-objects'
// sizes are read off the offsets Init() passes: the pad is at +0x00 and the
// keyboard at +0x60, and the first flag word is at +0x278, so VIKeyboard is
// 0x218 bytes.  The local VIConsolePad in Init() is the frame below the saves:
// 96 bytes.
// ---------------------------------------------------------------------------
// VIConsolePad -- 96 bytes, and every field of it is written by VIPad::Read,
// which is what names them: the four sticks come through ScaleStick and the
// sixteen buttons are each one INVERTED bit of the two libpad button bytes,
// converted to a float.  The write order is the source order and it is not
// monotonic (left, down, right, up; L1, R1, L2, R2), which is the only reason
// the pairs can be told apart at all.
class VIConsolePad
{
public:
    int   m_a;          // +0x00
    float m_rx;         // +0x04
    float m_ry;         // +0x08
    float m_lx;         // +0x0C
    float m_ly;         // +0x10
    float m_triangle;   // +0x14
    float m_square;     // +0x18
    float m_cross;      // +0x1C
    float m_circle;     // +0x20
    float m_left;       // +0x24
    float m_right;      // +0x28
    float m_up;         // +0x2C
    float m_down;       // +0x30
    float m_l1;         // +0x34
    float m_l2;         // +0x38
    float m_r1;         // +0x3C
    float m_r2;         // +0x40
    float m_start;      // +0x44
    float m_select;     // +0x48
    float m_l3;         // +0x4C
    float m_r3;         // +0x50
    int   m_pad[3];     // +0x54
};

// TWO bytes, not sixteen: VIKeyboard's queue is indexed by `sll a1,a1,1` and
// each entry is copied with two `lb`/`sb` pairs.  Phase 27 guessed 16 off the
// VIConsole frame; the keyboard's own code settles it.
class VIConsoleKey
{
public:
    char m_a;
    char m_b;
};

// ------------------------------------------------------------- VILoader ----
// SUPPORT 0x0044FA78 .. 0x004508D8, its own TU.  The engine's ASYNCHRONOUS
// resource loader: a queue of transactions, each one a byte range of an open
// resource file, read into a single bounce buffer with VIFile::ReadAsynch and
// then handed to VIESFParse the frame after the read lands.  Everything the
// streaming zone loader pulls in goes through it.
//
// The directory of one resource file is a sorted array searched by DictID.
// `VIResourceDir` is a real class -- ParseResourceFile's mangled name spells
// it out -- deriving from VIArray<VIResourceElem> and adding nothing, so its
// synthesized constructor and destructor inline to the base's and no
// __13VIResourceDir symbol is emitted at all.
class VIResourceElem
{
public:
    unsigned long m_offset;   // +0x00  byte offset in the file
    unsigned int  m_size;     // +0x08
    unsigned int  m_id;       // +0x0C  the DictID, and the search key
};

class VIResourceDir : public VIArray<VIResourceElem>
{
};

// One open resource file: the VIFile itself and its directory.  200 bytes,
// which is the stride every m_files subscript multiplies by, and 192 puts the
// directory exactly past VIFile.
class VIResourceFile
{
public:
    VIFile        m_file;   // +0x00
    VIResourceDir m_dir;    // +0xC0
};

// Only its SIZE, and only because VIBattleMusic::Preload holds one as a local:
// its frame puts the 256-byte path buffer at sp+0 and the parser above it, with
// the register saves starting at sp+1328.
//
// Phase 41 SETTLES the size at 1064: VILoader holds one as a MEMBER, between
// the VIObjFile ending at +0x258 and the `int` at +0x680, and 0x680-0x258 is
// 1064.  The local's 1072 was the frame rounded up to the `sq` saves, which
// both sizes reach.
// Phase 58 opens it up.  The constructor is nine word stores and a 256-entry
// VIColor32 array -- 40 + 1024 = the 1064 phase 41 measured -- and every
// top-level entry point sets the same seven pointers from the scene it was
// handed and clears them again on the way out.  The palette is the one for
// paletted textures: Convert8 and Convert4 read it, ParseSurfaceObj fills it.
class VIESFParse
{
public:
    VIESFParse();
    ~VIESFParse();

    int Parse(const char *path, VIScene &sc);
    int Parse(VIObjFile &f, VIScene &sc);
    int ParseSprite(const char *path, VIScene &sc);
    int ParseSound(const char *path, VIScene &sc);
    int ParseWorld(const char *path, int i, VIScene &sc);
    int SetPermResource(int on);
    void SetProgressCallback(void (*fn)(void *), void *arg);
    int ParseResourceFile(const char *path, int n, VIScene &sc, VIResourceDir &d);
    int ParseZoneResource(VIObjFile &f, int zone, VIScene &sc);

    int ParseObject();
    int ParseSprite();
    int ParseSound();
    int ParseZoneResource();
    int ParseWorld(int i);
    int ParseResourceDir(int i, VIResourceDir &d);
    int ParseResourceDirObj(int i, VIResourceDir &d);
    int ParseSimpleSpriteObj();
    int ParseHSpriteObj();
    int ParseCSpriteObj();
    int ParseHSpriteAnimObj();
    int ParseLODSpriteObj();
    int ParsePointLightObj();
    int ParseSoundSpriteObj();
    int ParseParticleSpriteObj();
    int ParseGroupSpriteObj();
    int ParsePointSpriteObj();
    int ParseStreamAudioSpriteObj();
    int ParseFloraSpriteObj();
    int ParseEffectVolumeSpriteObj();
    int ParseActorObj();
    int ParseSurfaceArrayObj();
    int ParseSurfaceObj();
    int ParseFontObj();
    int ParseMaterialPalObj();
    int ParseAdpcmObj();
    int ParseParticleDefinitionObj();
    int ParseXmObj();
    int ParseSpellEffectObj();
    int ParseWorldObj(int i);
    int ParseSubSprite();
    int ParseSurfaces(VIArray<int> &a);
    int ParseSurfaceArray();
    int ParseSurface();
    int ParseMaterialPal();
    int ParseMaterial();
    int ParseMaterial(VIMaterial *m);
    int ParseSimpleSubSpriteObj();
    int ParseSkinSubSpriteObj();
    int ParseSkinLODSpriteObj();

    // ---- phase 58: the wrappers and the array walkers -------------------
    int ParseActor();
    int ParseZoneBase(unsigned long offset);
    int ParseZoneBaseObj();
    int ParseZoneResources();
    int ParseZoneActors();
    int ParseZoneRoomActors();
    int ParseZoneRoomActorsObj();
    int ParseZoneStaticLightings();
    int ParseZoneRoomStaticLightings();
    int ParseZoneRoomStaticLightingsObj();
    int ParseZoneFlora();
    int ParseZoneFloraSpriteArray(VIZoneRadialFlora &f);
    int ParseZoneFloraSets(VIZoneRadialFlora &f);
    int ParseZoneRooms(VIZone &z);
    int ParseZoneRoom(VIZoneRoom &r);
    int ParseStaticLighting(VIArray<int> &a);
    int ParseStaticLightingObj(VIArray<int> &a);
    int ParseHSpriteAnim();
    int ParseHSpriteAnimArray();
    int ParseHSpriteSpriteArray();
    int ParseCSpriteSpriteArray();
    int ParseCSpritePartDefinitions();
    int ParseGroupSpriteSpriteArray();
    int ParseSkinLODSpriteSpriteArray();
    int ParseLODSpriteSpriteArray();
    int ParseSoundArray();
    int ParseParticleDefinition();
    int ParseWorldBase(VIWorld &w);
    int ParseWorldZones(VIWorld &w);
    int ParseWorldRegions(VIWorld &w);
    int ParseHSpriteTriggers(VIHSprite &h);
    int ParseZonePreTranslations(VIZone &z);
    int ParseResourceTable(VIArray<VIResourceElem> &a);
    int ParseZoneStaticTable(VIArray<VIZoneStaticTableElem> &a);
    int ParseCSpriteNodeIDList(VICSprite &c);
    int ParseCSpriteASlotList(VICSprite &c);
    int ParseCSpriteTSlotList(VICSprite &c);
    int ParseCSpriteSkinList(VICSprite &c);
    int ParseCSpriteContSound(VICSprite &c);
    int ParseCSpritePlayList(VICSprite &c);
    int ParseCSpritePartEmitters(VICSprite &c);
    int ParseWorldBaseHeader(VIWorld &w);
    int ParseWorldZoneProxies(VIWorld &w);
    int ParseWorldTree(VIWorldTree &t);
    int ParseZoneTree(VIZoneTree &t);
    int ParseZoneActor();
    int ParseHSpriteHierarchy(VIHSprite &h);
    int ParseHSpriteAttachments(VIHSprite &h);
    int ParseGroupSpriteMembers(VIGroupSprite &g);
    int ParseLODSpriteLevels(VILODSprite &l);
    int ParseSkinLODSpriteLevels(VILODSprite &l);
    int ParseRefMap();

    // ---- phase 59: the sprite leaves ------------------------------------
    int ParseFloraPrimBuffer();
    int ParsePrimBuffer();
    int ParseSkinPrimBuffer();
    int ParseCollBuffer();
    int ParseColorBuffer();

    // ---- phase 60: the asset half ---------------------------------------
    int Convert32(VIPixelStore &ps, unsigned char *src, unsigned char *dst, int n);
    int Convert24(VIPixelStore &ps, unsigned char *src, unsigned char *dst, int n);
    int Convert16(VIPixelStore &ps, unsigned char *src, unsigned char *dst, int n);
    int Convert8(VIPixelStore &ps, unsigned char *src, unsigned char *dst, int n);
    int Convert4(VIPixelStore &ps, unsigned char *src, unsigned char *dst, int n);
    int AdpcmToPcm(void *src, int n, void *dst);
    int ParseParticleDefinition(VIObjFile &f, VIParticleDefinition &d);

    int m_permResource;              // +0x00  1
    void (*m_progress)(void *);      // +0x04
    void  *m_progressArg;            // +0x08
    VIRaster      *m_raster;         // +0x0C  scene+0x4BA0
    VISoundDevice *m_sound;          // +0x10  scene+0x4BA4
    VICollide     *m_collide;        // +0x14  scene+0x4BA8
    VIScene       *m_scene;          // +0x18
    VIZone        *m_zone;           // +0x1C  &sc.m_world.m_zones[zone]
    VIDictionary  *m_dict;           // +0x20  scene+0x4B9C
    VIObjFile     *m_file;           // +0x24
    VIColor32      m_pal[256];       // +0x28
};

// ------------------------------------------------------------ VICSFFile ----
// SUPPORT 0x0049F6D0 .. 0x004A0094, its own TU.  The COMPRESSED container: a
// .CSF is a 40-byte header followed by N zlib streams, one per segment.
// Decompress inflates each into its own `new char[]` and hands the whole set to
// VIFile as a scatter/gather memory open, so VIObjFile then reads a .CSF
// exactly as it reads a flat .ESF.  Both `Parse(path)` and `Parse(memory)` are
// a VIFile local plus a call into the third one.
//
// The header's fields come off the six VIBufferSwap calls (4, 4, 8, 8, 8, 4)
// each end makes; Compress is what NAMES the two 64-bit ones, because it is the
// end that fills them in.  Decompress reads only four of the six.
class VICSFHeader
{
public:
    unsigned int m_id;             // +0x00  'CESF'
    int          m_numSegms;       // +0x04
    long         m_totalCompressed;// +0x08  sum of the compressed sizes
    long         m_totalSize;      // +0x10  the whole uncompressed file
    long         m_offset;         // +0x18  where the segment stream starts
    int          m_maxCompressed;  // +0x20  the biggest compressed segment
    int          m_pad24;

    static unsigned int CSFFileID;
};

// One segment's own record on the wire, in front of its zlib stream.
class VICSFSegm
{
public:
    int m_compressed;
    int m_size;
};

class VICSFFile
{
public:
    VICSFFile();

    int  Compress(const char *src, const char *dst);
    int  Parse(const char *path, VIESFParse &p, VIScene &sc);
    int  Parse(void *mem, long size, VIESFParse &p, VIScene &sc);
    void SetProgressCallback(void (*fn)(void *), void *arg);
    int  Parse(VIFile &f, VIESFParse &p, VIScene &sc);
    int  Decompress(VIFile &f, int &n, VIFile::MemSegm **segms);

    void (*m_progress)(void *);   // +0x00
    void  *m_progressArg;         // +0x04
};


class VILoader
{
public:
    VILoader();
    ~VILoader();

    int Init(int bufSize, int numFiles);
    int Clear();
    int Open(int i, const char *path);
    int OpenResourceFile(int i, const char *path, VIScene &sc);
    int Close(int i);
    int Process(VIScene &sc);
    int CompleteAll(VIScene &sc);
    int AllComplete();
    int ReleaseAllComplete(VIScene &sc);
    int IsResourceAvailable(int i, unsigned int id);
    int Load(int i, unsigned int id);
    int Load(int i, unsigned int id, unsigned long off, unsigned int size);
    int Load(int i, unsigned long off, unsigned int size, int zone);
    int Load(int i, unsigned int id, unsigned long off, unsigned int size, int zone);
    int IsComplete(int i);
    int Release(int i, VIScene &sc);
    void SetProgressCallback(void (*fn)(void *), void *arg);
    int Find(int i, unsigned int id, unsigned long &off, unsigned int &size);
    int Complete(int i, VIScene &sc);
    int Allocate();
    int Align(VILoaderTrans &t);

    int             m_numFiles;    // +0x000
    VIResourceFile *m_files;       // +0x004
    VIObjFile       m_objFile;     // +0x008
    VIESFParse      m_esf;         // +0x258
    int             m_cur;         // +0x680  the transaction Process is on
    VIPool<VILoaderTrans> m_trans; // +0x684
    VIMap<unsigned int, int> m_map;// +0x69C  DictID -> transaction
    void          (*m_progress)(void *);  // +0x6B0
    void           *m_progressArg; // +0x6B4
    int             m_bufSize;     // +0x6B8
    char           *m_buffer;      // +0x6BC  m_bufferBase aligned up to 64
    char           *m_bufferBase;  // +0x6C0
};

// ---------------------------------------------------------------------------
// VIBattleMusic -- SUPPORT 0x0049E618 .. 0x0049F668, its own TU.  The combat
// music layer: it picks a track, preloads it as an XM out of its own ESF, and
// hands it to the sound device -- with a whole second path behind feature bit
// 0x100 that streams a .bgm file instead.
//
//   +0x00  VIScene *m_scene
//   +0x04  int      m_track     -1
//   +0x08  int      m_state
//   +0x0C  int      m_loaded
//   +0x10  float    m_rate      0.01
//   +0x14  int      m_stopNow
//   +0x18  int      m_sound     -1, the device's sound index
//   +0x1C  int      m_channel   -1
// ---------------------------------------------------------------------------
class VIBattleMusic
{
public:
    VIBattleMusic();
    ~VIBattleMusic();

    int  Init(VIScene *sc);
    void Free();
    int  Play(int track);
    int  Process();
    void Stop(bool now);
    void StopNow();
    int  Preload(int track);
    int  Unload();

    VIScene *m_scene;    // +0x00
    int      m_track;    // +0x04
    int      m_state;    // +0x08
    int      m_loaded;   // +0x0C
    float    m_rate;     // +0x10
    int      m_stopNow;  // +0x14
    int      m_sound;    // +0x18
    int      m_channel;  // +0x1C
};

// VIPad -- SUPPORT 0x004A1AC8 .. 0x004A2298, sharing its TU with the four
// expansion/feature flag accessors after it.  Phase 27 measured 96 bytes off
// VIConsole's offsets; phase 36 fills them in, and the last field is what
// closes the class exactly: scePadRead is handed `this + 60`, and 0x3C + 32
// is 0x5C, which rounds to 96 on the 8-alignment the three `long`s force.
//
// One controller port.  The vibration half runs on a DEADLINE: VibrateLf and
// VibrateHf stamp an expiry and raise a dirty flag, and Process is the only
// thing that talks to the actuators -- at most one scePadSetActDirect a frame,
// and only when something changed.
class VIPad
{
public:
    VIPad();
    ~VIPad();

    int   Init(int port, int slot);
    int   Close();
    int   Read(VIConsolePad &p);
    int   Process(long now);
    int   VibrateLf(unsigned char level, int ms);
    int   VibrateHf(int ms);
    void  StopVibration();
    float ScaleStick(unsigned char v);
    // Two things at once, and neither is visible from one call site.  The
    // three callers all materialise the comparison as an int and then test it,
    // which is what an inline predicate returning `int` produces rather than
    // the test written out.  And the `ld` it compiles to is NOT evidence of a
    // 64-bit member: gcc merges the two adjacent int compares into one 8-byte
    // load against the constant 2 (little-endian, low word 2 and high word 0).
    int   Active() const { return m_state == 2 && m_actAligned == 0; }

    int           m_port;       // +0x00
    int           m_slot;       // +0x04
    long          m_time;       // +0x08  stamped by Process
    long          m_lfEnd;      // +0x10  0 == off
    long          m_hfEnd;      // +0x18
    int           m_state;      // +0x20
    int           m_actAligned; // +0x24
    unsigned char m_act[6];     // +0x28
    int           m_dirty;      // +0x30
    int           m_mode;       // +0x34
    int           m_open;       // +0x38
    unsigned char m_data[32];   // +0x3C  scePadRead's buffer
};

int          VIGetExpansionFlags();
void         VISetExpansionFlags(unsigned int f);
int          VIGetFeatureFlags();
void         VISetFeatureFlags(unsigned int f);

// VIKeyboard -- SUPPORT 0x004A0E60 .. 0x004A1AC8, sharing its TU with the two
// driver entry points ahead of it.  0x218 bytes (phase 27, off VIConsole's own
// offsets), of which this phase names the head and the tail: a file descriptor,
// a 128-entry event queue reached through a POINTER to a file static, and the
// dead-key state the combining tables need.
class VIKeyboard
{
public:
    VIKeyboard();
    ~VIKeyboard();

    int  Init();
    void Free();
    int  Read(VIConsoleKey &k);
    void QueueKeyEvent(VIConsoleKey *k);
    int  DequeueKeyEvent(VIConsoleKey *k);

    int           m_x0;         // +0x000
    int           m_fd;         // +0x004  -1 == closed
    VIConsoleKey *m_queue;      // +0x008  a file static, not embedded
    int           m_numQueued;  // +0x00C  capped at 128
    char          m_pad[0x200]; // +0x010
    int           m_x210;       // +0x210
    int           m_x214;       // +0x214
};

int VIStartKeyboardDriver();
int VIStopKeyboardDriver();

class VIConsole
{
public:
    int Init();
    int Close();
    int Process(long t);
    int GetInput(int *type, void *data, int size);

    VIPad      m_pad;         // 0x000
    VIKeyboard m_keyboard;    // 0x060
    int        m_padPresent;  // 0x278
    int        m_keyReady;    // 0x27C
    int        m_padReady;    // 0x280
};

// ---------------------------------------------------------------------------
// VIWnd -- SUPPORT 0x004AB818 .. 0x004ABBA0.  Phase 27.
//
// The UI widget base.  SIX virtuals, read off the call sites: the destructor is
// entry 1 (byte 8), OnDraw 2, OnDrawSelf 3, OnKeyUp 4, OnKeyDown 5 and
// OnMessage 6 -- so the vtable is 64 bytes and Phase 19's `B/8 - 2` agrees.
// The child set is a VISet<VIWnd *> at +0x00, which is why `this` and the set
// share a register everywhere.
// ---------------------------------------------------------------------------
class VIUI;

class VIWnd
{
public:
    VIWnd();
    virtual ~VIWnd();

    virtual void OnDraw();
    virtual void OnDrawSelf();
    virtual void OnKeyUp(int key);
    virtual void OnKeyDown(int key);
    virtual void OnMessage(VIWnd *w, int msg, int p1, int p2);

    void  AddChild(VIWnd *w);
    void  RemoveChild(VIWnd *w);
    void  SetAppPtr(void *p);
    void *GetAppPtr();
    void  SetFocus(VIWnd *w, bool notify);

    VISet<VIWnd *> m_children;   // 0x00
    VIWnd         *m_parent;     // 0x14
    void          *m_appPtr;     // 0x18
    int            m_visible;    // 0x1C

    static VIWnd *mFocus;
    // Phase 42.  VISetWindowUI, the last function in viuiwindow.cpp, writes
    // `_5VIWnd$mUI` and clears `_5VIWnd$mFocus`.
    static VIUI *mUI;
};

// ---------------------------------------------------------------------------
// VIClipper / VIQueryTriList -- SUPPORT 0x0041E370 and 0x0045D9B8.  Phase 27.
//
// The polygon clipper is two scratch vertex arrays and nothing else -- sixteen
// bytes, and VIQueryTriList embeds one at +0x0C, which is why its destructor
// destroys the two VIArrays itself rather than calling ~VIClipper.
//
// The triangle list's element is 40 bytes with the first word left alone and
// nine floats zeroed: three VIVect3s behind an int.  That is `new T[n]` with
// gcc's own vec-init loop, the shape Phase 24 found VIArray does NOT have.
// ---------------------------------------------------------------------------
class VIClipper
{
public:
    int Init(int n);
    int Clear();

    VIArray<VIVect3> m_in;    // 0x00
    VIArray<VIVect3> m_out;   // 0x08
};

class VIQueryTri
{
public:
    int     m_actor;
    VIVect3 m_v0;
    VIVect3 m_v1;
    VIVect3 m_normal;
};

class VIQueryTriList
{
public:
    VIQueryTriList();
    ~VIQueryTriList();

    int Init(int n, const VIBBox &b, const VIVect3 &dir, float angle);
    int Reset(const VIBBox &b, const VIVect3 &dir, float angle);
    int Clear();
    int Volume(const VIBBox &b);
    int Allocate();
    int Intersect(int idx, const VIVect3 &a, const VIVect3 &b, const VIVect3 &c);

    int         m_numTris;    // 0x00
    int         m_maxTris;    // 0x04
    VIQueryTri *m_tris;       // 0x08
    VIClipper   m_clipper;    // 0x0C
    VIVect3     m_dir;        // 0x1C
    float       m_cosAngle;   // 0x28
    VIPlane     m_planes[6];  // 0x2C
};

// ---------------------------------------------------------------------------
// VIStaticLighting -- SUPPORT 0x0047EDC0 .. 0x0047F090.  Phase 27.
//
// One VIArray<int> of baked colour-buffer handles behind two words.  The array
// is named through a POINTER in ReleaseResources -- the count comes off `this`
// at +8 with +0 folded away, the data pointer off the pointer at +4, which is
// Phase 9's lever both halves at once.
// ---------------------------------------------------------------------------
// Phase 54: it derives from VIResource -- `new VIStaticLighting` in
// VIScene::CreateStaticLighting zeroes +4 then +0 and then runs the VIArray
// constructor, which is VIResource's `m_id = m_link = 0` followed by the
// member's own.  So the two loose ints phase 34 called m_a and m_b are the
// DictID and the share count VIScene::Share/ReleaseStaticLighting step.
class VIStaticLighting : public VIResource
{
public:
    int Init(int n);
    int Clear(VIRaster &r, VIDictionary &d);
    int ShareResources(VIRaster &r);
    int ReleaseResources(VIRaster &r, VIDictionary &d);

    VIArray<int>  m_buffers;  // 0x08
};

// ---------------------------------------------------------------------------
// VIStringTable -- SUPPORT 0x004A4050 .. 0x004A4940.  Phase 27.
//
// WORDS.TXT: a row/column table of pointers into one text blob.  GetString
// takes a ONE-BASED row (its first instruction after the fallback is
// `addiu a1,a1,-1`) and falls back to a file-scope string on every out-of-range
// index, including a null entry.  LoadRawFile is STATIC -- `this` never reaches
// a0 at its call site, the name does.
// ---------------------------------------------------------------------------
class VIStringTable
{
public:
    VIStringTable();
    ~VIStringTable();

    void Unload();
    const char *GetString(int row, int col);

    int Load(const char *name);
    int LoadFromMemory(const void *data, int size);
    int LoadFromMemCard(const char *name);

    static int LoadRawFile(const char *name, VIArray<char> &buf);
    int ProcessRawFile(const VIArray<char> &buf);
    int FixupTextBuffer();

    int    m_numRows;    // 0x00
    int    m_numCols;    // 0x04
    char  *m_text;       // 0x08
    char **m_strings;    // 0x0C
};

// ---------------------------------------------------------------------------
// VITimeOfDay -- SUPPORT 0x003E8508 .. 0x003E8998.  Phase 27.
//
// All static: `this` never reaches a0 at any call site -- GetLocalTime's a0 is
// the output buffer and GetDayOfWeek's is the day.  Both name lookups are
// jump-table switches with a shared out-of-range string.
// ---------------------------------------------------------------------------
class VITimeOfDay
{
public:
    static int GetLocalTime(char *out, int n);
    static int GetLocalTime(unsigned short *out, int n);
    static const char *GetMonth(int month);
    static const char *GetDayOfWeek(int month, int day, int year);
    static void        GetDayOfWeek(int month, int day, int year, int &dow);
};


// ---------------------------------------------------------------------------
// VICamera -- SUPPORT 0x0041C7F8 .. 0x0041DBA8.  Phase 31.
//
// The layout comes out of Init and Update with nothing left over.  The two
// thin operator() overloads name the four scalars: each forwards
// (m_fov, m_aspect, m_near, <its own float>) into Init, so Init's own four
// stores -- +0x14, +0x18, +0x00, +0x04 -- ARE those fields, in that order.
//
//   +0x000  float       m_near
//   +0x004  float       m_far
//   +0x008  VIVect3     m_dir        CalcAxes writes the normalised direction
//   +0x014  float       m_fov        DEGREES: Init multiplies by pi/180
//   +0x018  float       m_aspect
//   +0x01C  VIVect3     m_position
//   +0x030  VIMatrix44  m_view       right/up/fwd as COLUMNS, row 3 the dots
//   +0x070  VIMatrix44  m_proj       copied wholesale from Init's local
//   +0x0B0  VIMatrix44  m_world      right/up/fwd/pos as ROWS -- the inverse
//   +0x0F0  VIMatrix44  m_viewProj   m_view * m_proj
//   +0x130  VIFrustum   m_frustum    eight corners in WORLD space
//
// The 8 bytes at +0x28 are padding: VIVect4 is aligned(16) (Phase 14), so the
// first matrix has to land on 0x30 and m_position's twelve bytes end at 0x28.
// ---------------------------------------------------------------------------
class VICamera
{
public:
    float      m_near;
    float      m_far;
    VIVect3    m_dir;
    float      m_fov;
    float      m_aspect;
    VIVect3    m_position;
    VIMatrix44 m_view;
    VIMatrix44 m_proj;
    VIMatrix44 m_world;
    VIMatrix44 m_viewProj;
    VIFrustum  m_frustum;

    int  Init(VIRaster &r, const VIVect3 &pos, const VIVect3 &dir,
              float fov, float aspect, float nearZ, float farZ);
    int  operator()(VIVect3 &pos, VIVect3 &dir);
    int  operator()(VIVect3 &pos, VIVect3 &right, VIVect3 &up,
                    VIVect3 &fwd);
    int  operator()(VIRaster &r, float farZ);
    int  operator()(VIRaster &r, VIVect3 &pos, VIVect3 &dir, float farZ);
    int  CalcPick(const VIRaster &r, const VIVect2I &pt, float t,
                  VIVect3 &a, VIVect3 &b) const;
    int  CalcPickFrustum(const VIRaster &r, const VIRect &box, float t,
                         VIFrustum &f) const;
    int  CalcAxes(const VIVect3 &dir, VIVect3 &right, VIVect3 &up,
                  VIVect3 &fwd);
    int  Update(const VIVect3 &pos, VIVect3 &right, VIVect3 &up, VIVect3 &fwd,
                VIMatrix44 &proj);
    int  CalcFrustum(const VIMatrix44 &invProj, const VIMatrix44 &world);
};


// ---------------------------------------------------------------------------
// VIFlyCamera -- SUPPORT 0x00441210 .. 0x004422F8.  Phase 31.
//
// The free-flight camera, and structurally VIWalkControl's little brother: the
// same six-vector rigid body (position, angles, velocity, angular velocity,
// force, torque) integrated by the same powf-damped step, but with no liquid,
// no gravity and no locomotion state.  It is NOT derived from VIWalkControl --
// every offset is its own -- and it carries the body TWICE, once live and once
// as the value committed at the end of the last sub-step.  Init writes the
// caller's position and angles into the COMMITTED copy and then copies that
// forward into the live one.
//
// The three direction vectors sit at +0, and `this` IS `&m_facing`, which is
// why Process's HPR call passes a0 unchanged.
// ---------------------------------------------------------------------------
enum VIWalkCameraMode
{
    VIWalkCameraFirstPerson,
    VIWalkCameraOrbit,
    VIWalkCameraFixed,
    VIWalkCameraThirdPerson
};

class VIFlyCamera
{
public:
    VIVect3 m_facing;           // +0x000  HPR(heading, pitch, roll)
    VIVect3 m_forward;          // +0x00C  HPR(heading, 0, roll)
    VIVect3 m_right;            // +0x018  normalize(m_facing x up)
    long    m_time;             // +0x028  four bytes of padding at +0x24
    VIVect3 m_position;         // +0x030
    VIVect3 m_angles;           // +0x03C
    VIVect3 m_velocity;         // +0x048
    VIVect3 m_angularVel;       // +0x054
    VIVect3 m_force;            // +0x060
    VIVect3 m_torque;           // +0x06C
    float   m_moveAccel;        // +0x078  the three InitCameraIntermediate
    float   m_turnAccel;        // +0x07C  derives from speed and damping
    float   m_pitchAccel;       // +0x080
    long    m_lastTime;         // +0x088  padding at +0x84
    VIVect3 m_lastPosition;     // +0x090
    VIVect3 m_lastAngles;       // +0x09C
    VIVect3 m_lastVelocity;     // +0x0A8
    VIVect3 m_lastAngularVel;   // +0x0B4
    VIVect3 m_lastForce;        // +0x0C0
    VIVect3 m_lastTorque;       // +0x0CC
    int     m_tickMS;           // +0x0D8  32
    int     m_maxTickMS;        // +0x0DC  200
    float   m_moveSpeed;        // +0x0E0  45.0
    float   m_turnRate;         // +0x0E4  pi/2
    float   m_pitchRate;        // +0x0E8  pi/2
    float   m_moveDamping;      // +0x0EC  0.01
    float   m_turnDamping;      // +0x0F0  0.01
    float   m_pitchDamping;     // +0x0F4  0.01
    int     m_pF8;              // +0x0F8  -1
    VICameraEffect m_effect;    // +0x0FC

    int Init(VICamera &cam, VIScene &scene);
    int Init(const VIVect3 &pos, const VIVect3 &angles, VICamera &cam,
             VIScene &scene);
    int Process(VICamera &cam, VIScene &scene);
    int UpdateCameraPhysics(float dt);
    int MoveCamera(VIScene &scene);
    int TurnLeft(float t);
    int PitchDown(float t);
    int Forward(float x, float y);
    int Hover(float x, float y, float z);
    int FreeLook(float h, float p);
    int SetCamera(VICamera &cam, VIScene &scene);
    int ClearCameraForces();
    int InitCameraIntermediate();
};


// ---------------------------------------------------------------------------
// VIWalkCamera -- SUPPORT 0x00482020 .. 0x00483EB8.  Phase 31.
//
// The gameplay camera.  It DERIVES from VIWalkControl, so the whole rigid body
// above is the LOOK-AT target's, and the class's own state is a second,
// smaller body in polar coordinates -- orbit angle, orbit pitch and orbit
// distance about that point -- carried twice, live and committed, exactly the
// way VIFlyCamera carries its cartesian one.  InitCameraIntermediate copies
// +0x1A0..+0x1F0 forward onto +0x1F8..+0x248 in one run, which is what fixes
// the two blocks' shape and their 0x58 stride.
// ---------------------------------------------------------------------------
class VIWalkCamera : public VIWalkControl
{
public:
    // The orbit body, and it is a real nested type: InitCameraIntermediate
    // copies the whole of it forward through a POINTER to each half (Phase 9,
    // so the +0 access folds back onto `this` and every other field comes off
    // the pointer), which a flat member list cannot produce.
    class State
    {
    public:
        long    m_time;         // +0x00
        // UNSIGNED: every clear mask comes out `lui 0xffff` + `ori`, where a
        // signed ~mask would be one `addiu`.
        unsigned int m_flags;   // +0x08  1 snap-angle, 2/4 zoom, 8 snap-pitch
        VIVect3 m_orbit;        // +0x0C  (angle, pitch, -)
        VIVect3 m_orbitVel;     // +0x18
        VIVect3 m_orbitForce;   // +0x24
        float   m_dist;         // +0x30
        float   m_distVel;      // +0x34
        float   m_distForce;    // +0x38
        VIVect3 m_cameraPos;    // +0x3C
        float   m_snapAngle;    // +0x48
        float   m_snapPitch;    // +0x4C
    };

    State   m_cur;                  // +0x1A0
    float   m_angleAccel;           // +0x1F0  derived, like VIWalkControl's
    float   m_distAccel;            // +0x1F4
    State   m_last;                 // +0x1F8
    int     m_autoOrbit;            // +0x248
    float   m_lookAtHeight;         // +0x24C
    float   m_defOrbitAngle;        // +0x250
    float   m_defOrbitDistance;     // +0x254
    float   m_angleDamping;         // +0x258
    float   m_distDamping;          // +0x25C
    float   m_minOrbitDistance;     // +0x260
    float   m_maxOrbitDistance;     // +0x264  25.0, the zoom-out ceiling
    float   m_defOrbitPitch;        // +0x268
    float   m_altOrbitPitch;        // +0x26C
    float   m_collisionRadius;      // +0x270
    float   m_targetHeight;         // +0x274
    float   m_lookAtHeightAbove;    // +0x278
    int     m_cameraMode;           // +0x27C  0 first, 1 orbit, 2 fixed, 3 3rd
    float   m_snapSpeed;            // +0x280
    float   m_distSnapSpeed;        // +0x284
    float   m_minCollisionDist;     // +0x288
    VICameraEffect m_effect;        // +0x28C

    int Init(VICamera &cam, VIScene &scene);
    int Init(const VIVect3 &pos, const VIVect3 &angles, VICamera &cam,
             VIScene &scene);
    int SetLocation(const VIVect3 &pos, const VIVect3 &angles, VICamera &cam,
                    VIScene &scene);
    int SetCameraMode(VIWalkCameraMode mode);
    int Process(VICamera &cam, VIScene &scene, int flags);
    int UpdateCameraPhysics(float dt);
    int MoveCamera(VIScene &scene);
    int SetDefOrbitPitchAngle(float p);
    int SetOrbitPitchAngle(float p);
    int SetOrbitDistance(float d);
    int Halt();
    int SetOrbitAngle(float a);
    int SetOrbitAngleBehind();
    int ChooseOrbitAngle(float a, VIScene &scene);
    int OrbitRight(float t);
    int OrbitIn(float t);
    int SetCamera(VICamera &cam, VIScene &scene, int flags);
    int ClearCameraForces();
    int InitCameraIntermediate(VIScene &scene);
    int FindOrbitAngle(float a, VIScene &scene, float &out);
    int FindBestOrbitAngle(float a, VIScene &scene, float &out);
};


// ---------------------------------------------------------------------------
// VIWalkCameraFilter -- SUPPORT 0x00484888 .. 0x004848D8.  Phase 31.
//
// The filter FindOrbitAngle sweeps the camera sphere against: it rejects the
// character's own actor and every VICSprite (sprite type 5), so the camera
// swings through players and NPCs and stops only on world geometry.
// ---------------------------------------------------------------------------
class VIWalkCameraFilter : public VISceneFilter
{
public:
    virtual int FilterActor(VIScene &scene, int actor);
};


// ---------------------------------------------------------------------------
// VITrailFx -- SUPPORT 0x004812E0 .. 0x00481FE0.  Phase 33.
//
// A motion trail: a ring of fixed-size SEGMENTs recycled between two intrusive
// lists.  Each segment is a quad edge -- two points and two colours -- plus the
// age it has accumulated; a segment older than the trail's life span is
// retired, along with everything behind it.
//
// The layer is a free pool of 16 trails and three file-scope lists, all in
// vitrailfx.cpp: the trails not in use, the ones still emitting, and the ones
// that have been ended and are fading out.
// ---------------------------------------------------------------------------
class VITrailFx : public VILNode
{
public:
    struct SEGMENT : public VILNode
    {
        VIVect3    m_pos[2];    // +0x10, +0x1C
        VIColor32F m_color[2];  // +0x28, +0x38
        float      m_age;       // +0x48
    };                          // 76

    long     m_lastTime;        // +0x10
    float    m_life;            // +0x18
    VILList  m_used;            // +0x1C
    VILList  m_free;            // +0x28
    SEGMENT *m_segments;        // +0x34
                                // 56

    VITrailFx();
    ~VITrailFx();
    int  Init(long now, int numSegments, float life);
    int  Clear();
    int  AddSegment(const VIVect3 &a, const VIVect3 &b, const VIColor32F &c);
    int  AddSegment(const VIVect3 &a, const VIVect3 &b, const VIColor32F &ca,
                    const VIColor32F &cb);
    int  Update(long now);
    int  Render(VIRaster &r, VICamera &c);
    void ClearSegmentsForward(SEGMENT *s);
};

// ---------------------------------------------------------------------------
// VIBoltHarmonic / VIBolt / VILightning -- SUPPORT 0x0044E1F8 .. 0x0044FA78.
// Phase 33.
//
// The lightning bolt.  A bolt's shape is the sum of three HARMONICS, each of
// which is a ring of 17 random offsets interpolated toward a second ring: when
// the phase reaches 1 the second ring becomes the first and a new one is drawn,
// so the bolt writhes continuously rather than snapping.  The three harmonics
// differ only in amplitude and rate -- 0.57 at 0.64 Hz, 0.15 at 10.24, 0.08 at
// 44 -- which is a coarse sway plus two grades of jitter.
// ---------------------------------------------------------------------------
class VIBoltHarmonic
{
public:
    int     m_pad;                  // +0x000
    VIVect3 m_samples[2][17];       // +0x004, +0x0D0
    int     m_count;                // +0x19C
    float   m_phase;                // +0x1A0
    float   m_rate;                 // +0x1A4
    float   m_amplitude;            // +0x1A8
                                    // 428

    int     Create(int count, float amplitude, float rate);
    void    GenerateSamples(int which);
    VIVect3 Sample(int i);
    void    Update(int dt);
};

class VIBolt
{
public:
    VIBoltHarmonic m_harmonics[3];  // +0x000
    VIVect3        m_points[17];    // +0x504
    VIMatrix44     m_matrix;        // +0x5D0
                                    // 1552

    VIBolt();
    void Reset();
    void Update(int dt, VIVect3 &a, VIVect3 &b, VIMatrix44 &m);
};

class VILightning
{
public:
    struct BRANCH
    {
        int     m_count;                // +0x000  bolts captured in this frame
        VIVect3 m_points[4][17];        // +0x004
    };                                  // 820

    VIVect3    m_start;             // +0x0000
    VIVect3    m_end;               // +0x000C
    VIMatrix44 m_matrix;            // +0x0020
    int        m_strikeTimer;               // +0x0060
    int        m_boltCount;               // +0x0064
    VIBolt     m_bolts[4];          // +0x0070
    VIColor32F m_color;             // +0x18B0
    int        m_maxBranchCount;    // +0x18C0
    float      m_lastCharge;           // +0x18C4
    float      m_charge;            // +0x18C8
    int        m_ringTimer;             // +0x18CC
    int        m_ringHead;             // +0x18D0
    int        m_ringCount;             // +0x18D4
    BRANCH     m_branches[3];       // +0x18D8
                                    // 8832

    VILightning();
    ~VILightning();
    void  SetLocation(const VIVect3 &a, const VIVect3 &b);
    float Charge() const;
    void  SetCharge(float c);
    void  SetColor(const VIColor32F &c);
    void  SetMaxBranchCount(int n);
    void  Update(int dt);
    void  Render(VIRaster &r);
    void  RenderBoltVertices(VIRaster &r, VIVect3 *pts, int n);
};

int VILightningFxInit(VIRaster &r, VIDictionary &d);
int VILightningFxClear(VIRaster &r, VIDictionary &d);

int        VITrailFxInit(VIRaster &r, VIDictionary &d);
int        VITrailFxClear(VIRaster &r, VIDictionary &d);
VITrailFx *VITrailFxBegin(long now, int numSegments, float life);
void       VITrailFxEnd(VITrailFx *t);
void       VITrailFxRender(long now, VIRaster &r, VICamera &c);

#endif

// ------------------------------------------------------------ VIZoneRoom --
// SUPPORT 0x00499778 .. 0x00499EC0.  One room of a zone: a box, two index
// arrays and ONE allocation holding a portal table and the portal vertices,
// exactly the shape VICollBuffer and VIPrimBuffer use -- CalcDataSize writes
// byte OFFSETS into m_portals/m_verts and Init rebases them onto m_data.  A
// portal is 32 bytes and a vertex 12, said by CalcDataSize's `sll 5` and its
// `mult 12`.
class VIZonePortal
{
public:
    VIPlane m_plane;   // +0x00
    int m_firstVert;   // +0x10
    int m_numVerts;    // +0x14
    int m_destRoom;    // +0x18
    int m_p1C;         // +0x1C
};

class VIZoneRoom
{
public:
    unsigned int   m_flags;        // +0x00   Init ORs bit 1
    VIBBox         m_bbox;         // +0x04
    int            m_p1C;          // +0x1C  -1
    int            m_p20;
    int            m_p24;
    unsigned int   m_p28;          // +0x28  Init's fourth argument
    int            m_p2C;          // +0x2C  -1
    unsigned int   m_p30;          // +0x30  Init's fifth argument
    int            m_p34;          // +0x34  -1
    int            m_p38;          // +0x38  -1
    VIArray<int>   m_a3C;          // +0x3C
    VIArray<int>   m_a44;          // +0x44
    int            m_numPortals;   // +0x4C  write cursor
    int            m_maxPortals;   // +0x50
    int            m_numVerts;     // +0x54  write cursor
    int            m_maxVerts;     // +0x58
    int            m_portals;      // +0x5C  offset, then pointer
    int            m_verts;        // +0x60  offset, then pointer
    int            m_destRoom;     // +0x64  -1
    int            m_p68;          // +0x68  -1
    int            m_p6C;          // +0x6C  -1
    int            m_dataSize;     // +0x70
    char          *m_data;         // +0x74

    VIZoneRoom();
    ~VIZoneRoom();

    int Init(int maxPortals, int maxVerts, unsigned int a, unsigned int b,
             VIBBox &bb);
    int Clear();
    int CalcDataSize();
    int Allocate();

    int DestRoom(int i);
    int Begin();
    int Vertex(VIVect3 &v);
    int End();
    int CalcPortalPlane(int i);
};

// ---------------------------------------------------------------------------
// VISpellActiveEvent -- one event of a list while it is running.  A VILNode, so
// the driver can carry it on its active or inactive list; the constructor
// zeroes six VIVect3 TWICE -- three arrays of two, each element default
// constructed by the array's own ctor loop and then zeroed again in the body.
//
//   +0x00  VILNode
//   +0x10  VIScene *m_scene
//   +0x14  VIVect3  m_v14
//   +0x20  VIVect3  m_v20
//   +0x2C  VIVect3  m_a[2]
//   +0x44  VIVect3  m_b[2]
//   +0x5C  VIVect3  m_c[2]
//   +0x74  VISpellEvent *m_event
//   +0x78  VIParticleEmitter *m_emitter0
//   +0x7C  VIParticleEmitter *m_emitter1
//   +0x80  VIMotifBlender    *m_blender
//   +0x84  int m_p84          -1
//   +0x88  int                 (the ctor does not touch it)
//   +0x8C  int m_actor0       -1, released back to the scene
//   +0x90  int m_actor1       -1
// ---------------------------------------------------------------------------
class VISpellActiveEvent : public VILNode
{
public:
    VISpellActiveEvent(VISpellEvent *ev, VIScene *sc);
    ~VISpellActiveEvent();

    int ParticleCount() const;

    VIScene           *m_scene;       // +0x10
    VIVect3            m_v14;         // +0x14
    VIVect3            m_v20;         // +0x20
    VIVect3            m_a[2];        // +0x2C
    VIVect3            m_b[2];        // +0x44
    VIVect3            m_c[2];        // +0x5C
    VISpellEvent      *m_event;       // +0x74
    VIParticleEmitter *m_emitter0;    // +0x78
    VIParticleEmitter *m_emitter1;    // +0x7C
    VIMotifBlender    *m_blender;     // +0x80
    int                m_p84;         // +0x84   attractor actor, -1
    float              m_f88;         // +0x88   attractor strength
    int                m_actor0;      // +0x8C
    int                m_actor1;      // +0x90
    int                m_p94;
    int                m_p98;
    float              m_f9C;         // +0x9C
    float              m_fA0;         // +0xA0
    int                m_pad[3];
    // +0xB0, and it is what forces the class's alignment to 8: NewWait stores
    // VIScene::GetTime()'s 64-bit result here with a single `sd`.
    long               m_time;        // 0xB8 -- what NewWait hands new
};

// ---------------------------------------------------------------------------
// VISpellEffectSystem -- SUPPORT 0x0047B840 .. 0x0047CBC0, its own TU (the
// VIPool<VISpellEffect *> instantiation closes it).  A pool of loaded effects
// on one side, and twenty-four playback slots on the other.
//
// A playback HANDLE is not an index: Play packs the slot into the low byte and
// a counter, seeded at 0x16062 and bumped per cast, into the rest --
// `i | ((m_serial++ << 8) & 0x7FFFFF00)` -- so a handle held past the end of
// its cast cannot name the next tenant of that slot.  Playback() checks both
// halves, which is why Stop and IsPlaying take the handle and not the slot.
// ---------------------------------------------------------------------------
class VISpellEffectSystem
{
public:
    // 40 bytes: a VILNode, a 64-bit start time (which is what forces the
    // alignment and the four bytes of tail padding), the effect it is playing,
    // its handle and the driver running it.
    class VISpellEffectPbi : public VILNode
    {
    public:
        long                 m_startTime;  // +0x10
        int                  m_effect;     // +0x18
        int                  m_handle;     // +0x1C
        VISpellEffectDriver *m_driver;     // +0x20
    };

    VISpellEffectSystem();
    ~VISpellEffectSystem();

    int  Init(VIScene *sc);
    int  Clear();

    VIVibrateFn SetVibrateCallback(VIVibrateFn fn, void *user);
    void SetCameraEffect(VICameraEffect *e);
    void SetPlayerActor(int actor);

    int            CreateSpellEffect(int share);
    VISpellEffect *SpellEffect(int i);
    int            ShareSpellEffect(int i);
    int            ReleaseSpellEffect(int i, VIDictionary &d);
    int            ReleaseAllSpellEffects(VIDictionary &d);

    int  Play(int effect, unsigned int a, int caster, int target, int *secondary);
    int  IsPlaying(int handle);
    VISpellEffectDriver *SpellEffectDriver(int handle);
    void Stop(int handle);
    void NotifyActorWasDestroyed(int actor);
    void StopIfThisWasCaster(int actor);
    int  Process();

    VISpellEffectPbi *NewPlayback();
    void             DeletePlayback(VISpellEffectPbi *pb);
    VISpellEffectPbi *Playback(int handle);
    VISpellEffectPbi *FirstPlayback();
    VISpellEffectPbi *NextPlayback(VISpellEffectPbi *pb);

    VIScene         *m_scene;          // +0x000
    VISpellEffectPbi *m_slots[24];      // +0x004
    VILList          m_active;         // +0x064
    VILList          m_free;           // +0x070
    VISpellEffectPbi  m_playbacks[24];  // +0x080
    int              m_serial;         // +0x440
    int              m_inited;         // +0x444
    VIVibrateFn      m_vibrate;        // +0x448
    void            *m_vibrateUser;    // +0x44C
    VICameraEffect  *m_cameraEffect;   // +0x450
    int              m_playerActor;    // +0x454
    VIPool<VISpellEffect *> m_effects; // +0x458
};

// ---------------------------------------------------------------------------
// Phase 37 -- the four remaining VISceneFilter/VIActorFilter overrides.  Their
// semantics were settled on the console first (go-eqoa-vi/vicoll/filters.go):
// non-zero REJECTS, so the flora filter's bare `return 1` refuses every actor
// and the weather filter's `return 0` accepts every one, the owner included.
// ---------------------------------------------------------------------------
class WeatherFilter : public VISceneFilter
{
public:
    virtual int FilterActor(VIScene &scene, int actor);
};

class VIRadialFloraFilter : public VISceneFilter
{
public:
    virtual int FilterActor(VIScene &scene, int actor);
};

class VISceneOutdoorsFilter : public VISceneFilter
{
public:
    virtual int FilterActor(VIScene &scene, int actor);
};

class VIWalkFloorFilter : public VISceneFilter
{
public:
    virtual int FilterActor(VIScene &scene, int actor);
};

// ---------------------------------------------------------------------------
// VICapsule / VICylinder -- SUPPORT 0x003EFE60 and 0x003F0058, one TU between
// VIBRect and VIMatrix44.  Two distinct types with a byte-identical layout: a
// half-open tube, Base at one cap's centre and Base + Dir*Length at the other,
// which go-eqoa-vi/vicoll measured against the console.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// VISceneSoundTrack / VISceneSoundTrackState -- SUPPORT 0x00471150.  The zone's
// ambient track pair: two states (the playing one and the one fading in) plus
// the cross-fade timer.
// ---------------------------------------------------------------------------
class VISceneSoundTrackState
{
public:
    int Init();
    int Clear();

    int  m_p00;
    long m_p08;
    int  m_p10;
    int  m_p14;
    int  m_p18;      // -1
    int  m_p1C;      // -1
    int  m_p20;
    int  m_p24;      // -1
};

class VISceneSoundTrack
{
public:
    int Init();
    int Clear();
    int SetTracks();

    int    m_p00;    // 1
    float  m_p04;    // 1.0f
    int    m_p08;    // -1
    int    m_pad0C;
    long   m_p10;    // 15000
    long   m_p18;    // 0
    int    m_p20;    // -1
    int    m_p24;    // -1
    long   m_p28;    // 0
    float  m_p30;    // 1.0f
    int    m_pad34;
    VISceneSoundTrackState m_state[2];   // +0x38 and +0x60
    // Phase 54: the class is 224 bytes, not 136 -- VIScene's next member, the
    // spell-effect system, is at +0x1B70 and the sound track starts at +0x1A90.
    // Clear (byte-exact) stops at m_state[1], so everything below is touched
    // only by VIScene::SetSoundTrack and ::UpdateSoundEmitters.
    int   m_p88;                         // +0x88
    char  m_pad8C[0x98 - 0x8C];
    int   m_p98;                         // +0x98
    int   m_p9C;                         // +0x9C
    int   m_pA0;                         // +0xA0
    int   m_pA4;                         // +0xA4
    char  m_padA8[0xE0 - 0xA8];
};

// ---------------------------------------------------------------------------
// The 2D UI layer -- SUPPORT 0x004A4B30 .. 0x004AB818.  Phase 42.
//
// VIUI is the drawing device: three VIStringTables (the localised words, the
// texture-name table and the sound-name table), a VIFont, and pointers to the
// raster, the sound device and the dictionary.  Every atlas entry is named by
// a ROW of the texture table and a COLUMN that is the current language, so a
// blit is "look the id up in a static table, turn its row into a name, hash
// the name, hand the raster the resource".
//
// The two lookup tables are file statics reached only through the two free
// functions below; the image gives their base as `_6VIFont$sm_FontName+0xa40`
// (nearest-preceding, they are unnamed).  GetUITexture's bound is 252 and its
// stride 36; GetUISound's are 17 and 8.  Both return entry 0 out of range
// rather than a null, so the callers' null tests never fire.
// ---------------------------------------------------------------------------
enum VIUITexId  { VIUITexId_0 = 0 };
enum VIUISoundId { VIUISoundId_0 = 0 };
enum VIUICOLOR  { VIUICOLOR_0 = 0 };
enum VIScrollStyle { VIScrollStyle_0 = 0 };

class VIUITexEntry
{
public:
    int              m_x00;      // +0x00
    int              m_row;      // +0x04  row into the texture-name table
    VIVect2I         m_uv0;      // +0x08
    VIVect2I         m_uv1;      // +0x10
    VIAlphaBlendMode m_mode;     // +0x18
    float            m_alpha;    // +0x1C
    unsigned int     m_flags;    // +0x20
};                               // 36 bytes -- GetUITexture's multiplier

class VIUISoundEntry
{
public:
    int   m_x00;                 // +0x00
    float m_volume;              // +0x04
};                               // 8 bytes -- GetUISound's shift

class VIUIFrameParam
{
public:
    int m_x00;
};

class VIUI
{
public:
    VIStringTable m_words;       // +0x00
    VIStringTable m_textures;    // +0x10
    VIStringTable m_sounds;      // +0x20
    int           m_language;    // +0x30
    int           m_wordsFromCard; // +0x34
    VIFont        m_font;        // +0x38
    VIRaster        *m_raster;   // +0x48
    VISoundDevice   *m_sound;    // +0x4C
    VIDictionary    *m_dict;     // +0x50

    VIUI();
    ~VIUI();

    int Init(VIRaster *r, VISoundDevice *s, VIDictionary *d);
    void Close();
    int ReloadWords();
    void SetLanguage(int l);

    int Begin2D();
    int End2D();

    int FillRectangle(const VIVect2I *r, const VIColor32F *c);
    int FrameFillRectangle(const VIVect2I *r);
    int FrameRectangle(const VIVect2I *r, const VIColor32F *c, int w);

    int Blit(int id, VIVect2I at, VIVect2I a, VIVect2I b,
             VIAlphaBlendMode m, float alpha, unsigned int flags);
    int Blit(int id, VIVect2I at, VIAlphaBlendMode m, float alpha,
             unsigned int flags);
    int Blit(VIUITexId id, VIVect2I at);
    int Blit(VIUITexId id, VIVect2I at, VIVect2I size);

    int BlitEx(VIUITexId id, VIVect2I at, VIAlphaBlendMode m, float alpha,
               unsigned int flags);
    int BlitEx(VIUITexId id, VIVect2I at, VIVect2I size, VIAlphaBlendMode m,
               float alpha, unsigned int flags);
    int StretchBlitEx(VIUITexId id, VIVect2I at, VIVect2I size,
                      VIAlphaBlendMode m, float alpha);

    int BlitHorz(VIVect2I at, VIUITexId id, int w, VIAlphaBlendMode m,
                 float alpha, unsigned int flags);
    int BlitVert(VIVect2I at, VIUITexId id, int h, VIAlphaBlendMode m,
                 float alpha, unsigned int flags);
    int BlitRect(VIUITexId id, VIVect2I at, VIVect2I size, VIAlphaBlendMode m,
                 float alpha);
    int BlitVertBar(VIVect2I at, VIUITexId top, VIUITexId bot, VIUITexId mid,
                    int h, VIAlphaBlendMode m, float alpha);
    int BlitStuddedHorz(VIVect2I at, VIUITexId bar, VIUITexId stud, int w,
                        int n, VIAlphaBlendMode m, float alpha,
                        unsigned int flags);
    int BlitAbilityIcon(VIVect2I at, int idx, VIAlphaBlendMode m, float alpha);

    int DrawStringEx(const char *s, int x, int y, VIColor32F c, int shadow);
    int DrawStringEx(const unsigned short *s, int x, int y, VIColor32F c,
                     int shadow);
    int DrawStringEx(unsigned short id, int x, int y, VIColor32F c, int shadow);
    int DrawCenteredStringEx(const char *s, int x, int y, VIColor32F c,
                             unsigned int w, int shadow);
    int DrawCenteredStringEx(const unsigned short *s, int x, int y,
                             VIColor32F c, unsigned int w, int shadow);
    int DrawCenteredStringEx(unsigned short id, int x, int y, VIColor32F c,
                             unsigned int w, int shadow);

    int SoundPlay(VIUISoundId id);
    int SoundPlay(const char *name, float vol);
};

const VIUITexEntry  *GetUITexture(VIUITexId id);
const VIUISoundEntry *GetUISound(VIUISoundId id);

// ---------------------------------------------------------------------------
// VIUIWindow -- SUPPORT 0x004A8878 .. 0x004AB818.  Phase 42.
//
// A framed panel drawn straight onto the UI device: position, centre and size
// in screen pixels, a title, a four-entry palette (VIUICOLOR indexes it -- the
// two `9VIUICOLOR` forwarders are `sll 4; addiu 304; addu this`) and the
// open/close animation, whose two floats are the whole state machine:
// m_open runs 0..1 and m_dir is +1.0f while opening and -1.0f while closing.
// ---------------------------------------------------------------------------
class VIUIWindow
{
public:
    VIUI      *m_ui;             // +0x000
    VIVect2I   m_pos;            // +0x004
    VIVect2I   m_center;         // +0x00C
    VIVect2I   m_size;           // +0x014
    int        m_titleH;         // +0x01C
    int        m_rows;           // +0x020
    int        m_inited;         // +0x024
    float      m_open;           // +0x028
    float      m_dir;            // +0x02C
    char       m_title[256];     // +0x030
    VIColor32F m_colors[4];      // +0x130
    long       m_pulseTime;      // +0x170
    int        m_x178;           // +0x178

    VIUIWindow();
    ~VIUIWindow();

    int  Init(VIUI *ui, const VIVect2I &pos, const VIVect2I &center,
              int w, int rows, int x);
    int  Init(VIUI *ui, const VIVect2I &pos, const VIVect2I &center,
              const VIVect2I &size, int x);
    void Free();

    VIVect2I RowCenter(int row);
    void SetTitle(const char *s);
    void SetTitle(const unsigned short *s);
    void Resize(int w, int rows);

    const unsigned short *T(const char *s);
    const unsigned short *T(const unsigned short *s);
    const unsigned short *T(unsigned short id);

    int Text(const unsigned short *s, int x, int y, const VIColor32F &c,
             unsigned int f);
    int Text(const unsigned short *s, int x, int y, VIUICOLOR c,
             unsigned int f);
    int TextBox(const unsigned short *s, int x, int y, int w, int h,
                const VIColor32F &c, unsigned int f);
    int TextBox(const unsigned short *s, int x, int y, int w, int h,
                VIUICOLOR c, unsigned int f);
    int CountLines(const unsigned short *s, int w);

    void DrawVScroll(VIVect2I at, int h, float t, unsigned int flags);

    int TickR(int x, int row, bool pulse);
    int TickU(int x, int row, bool pulse);
    int TickD(int x, int row, bool pulse);
    float TickPulse() const;
    void  ResetTickPulse();

    int  IsOpening() const;
    int  IsOpen() const;
    void Open(bool now);
    int  IsClosing() const;
    int  IsClosed() const;
    void Close(bool now);

    void AppendEllipsis(unsigned short *s, int w);
    int  TextWidth(const unsigned short *s);
    void CopyAsteriks(unsigned short *d, int n, int max);
    void SetUiColors();
};

void VISetWindowUI(VIUI *ui);

// ---------------------------------------------------------------------------
// VIUITextScroll -- SUPPORT 0x004A8330 .. 0x004A8848.  Phase 42.
//
// A one-row VIUIWindow that reveals its text a character at a time.  The
// derived half is six ints past the base's 0x17C bytes.
// ---------------------------------------------------------------------------
class VIUITextScroll : public VIUIWindow
{
public:
    int m_style;      // +0x180
    int m_p184;       // +0x184
    int m_p188;       // +0x188
    int m_p18C;       // +0x18C   30
    int m_width;      // +0x190
    int m_on;         // +0x194

    VIUITextScroll();
    ~VIUITextScroll();

    int  Init(VIUI *ui, const VIVect2I &at, int w, int x, VIScrollStyle st);
    int  Reset();
    int  TextScroll(const unsigned short *s, const VIColor32F &c, int n);
    int  TextScroll(const unsigned short *s, VIUICOLOR c, int n);
    void SetOn(int v);
};

// ===========================================================================
// The spatial trees -- VIWorld (SUPPORT 0x0048E380 .. 0x004924D0) and VIZone
// (0x00493830 .. 0x00498CC0), two whole translation units.  Phase 51.
//
// They are at the BOTTOM of this header because both hold VIArrays, and the
// template is declared 2,000 lines above -- not because they are late in the
// engine.  VIScene owns a VIWorld at +0x04 and carries it as a byte pad.
//
// The world is a BSP of ZONES and each zone a BSP of ROOMS, and **the two
// descents are the same code**: `VIWorld::Collide(int, VISceneCollVars&)` and
// `VIZone::Collide(int, VISceneCollVars&)` are instruction for instruction the
// same function, differing only in where the tree lives and what a leaf means.
// go-eqoa-vi's `vicoll/scene.go` walks it against the console and is the
// semantic reference; this is the source that produced it.
//
// A child index is a TAGGED word: any bit of 0xF0000000 set means "not an
// ordinary interior index", 0x40000000 on top of that means LEAF, and any
// other tag is simply stripped and used as an interior index.
// ===========================================================================

// The room's actor list node.  VIZoneRoom::m_p68 is its head, held in an `int`
// and terminated by -1 rather than by null; the four room walkers read the
// actor id at +0x04 and the successor at +0x10.  VIScene owns the class.
class VISceneActorRef
{
public:
    int              m_p00;
    int              m_actor;   // +0x04
    int              m_p08;
    int              m_p0C;
    int              m_next;    // +0x10  the successor, or -1
};

// 24 bytes each, and the constructor is exactly VIPlane's four zero stores.
class VIZoneNode  { public: VIPlane m_plane; int m_front, m_back; };
class VIWorldNode { public: VIPlane m_plane; int m_front, m_back; };

// A world leaf names a zone; a zone leaf names a room and carries one more
// word.  The SIZE is what the two FindLeaf`s state -- `sll 2` at the world
// level and `sll 3` at the zone level.
class VIZoneLeaf  { public: int m_a, m_b; };
class VIWorldLeaf { public: int m_a; };

// One byte, and the constructor is one `sb`.  The world's region array is the
// terrain-profile grid: one profile INDEX per sub-cell, which is why
// GetTerrainProfile ends in an `lbu`.
class VIWorldRegion
{
public:
    unsigned char m_flags;

    VIWorldRegion() { m_flags = 0; }
};

// Element types of three of VIZone's arrays.  Only their SIZES are known --
// the address arithmetic states them -- and nothing in this phase indexes a
// field of one.
class VIRadialFloraSet      { public: int m_model, m_dist, m_c; };   // 12

// 28 bytes, and ParseZoneFloraSets names four of the seven words: a DictID, an
// index and three floats.  The two after the id are never read from the file.
class VIRadialFloraDist
{
public:
    unsigned int m_id;    // +0x00
    int          m_p04;
    int          m_p08;
    int          m_set;   // +0x0C
    float        m_p10;
    float        m_p14;
    float        m_p18;
};
// 32 bytes, and ParseZoneStaticTable names every field: TWO (offset, size)
// pairs, the second one the static lighting that goes with the first.
class VIZoneStaticTableElem
{
public:
    unsigned long m_offset;    // +0x00
    unsigned int  m_size;      // +0x08
    int           m_pad0C;
    unsigned long m_offset2;   // +0x10
    unsigned int  m_size2;     // +0x18
    int           m_pad1C;
};
class VIRadialFloraModel
{
public:
    int      m_a, m_b, m_c;     // +0x00
    VISphere m_sphere;          // +0x0C
};

// 40 bytes, said by the stride between the 22 stores SetProfiles makes.  One
// entry of the terrain's time-of-day table: a colour and six scalars.
// 40 bytes.  Phase 54 corrects phase 51's field order: the array starts at
// VIWorld+0x58, not +0x60, and what phase 51 read as two loose floats
// (`m_p58` 250.0 and `m_p5C` 800.0) are profile[0]'s first two.  The colour is
// at +0x08, which is what makes VIWorld's constructor zero 16 bytes at
// +0x60, +0x88, +0xB0 ... -- one VIColor32F per profile, 22 of them -- and
// makes SetProfiles' last store, +0x3C4, the last field of the last profile
// instead of eight bytes past the end of the class.
class VITerrainProfile
{
public:
    float      m_p00;     // +0x00
    float      m_p04;     // +0x04
    VIColor32F m_color;   // +0x08
    int        m_p18;     // +0x18
    float      m_p1C;     // +0x1C
    int        m_p20;     // +0x20
    int        m_p24;     // +0x24
};

// Phase 54, off VIScene::CalcTimeOfDayClipPlanes: four terrain-profile INDICES
// and the four weights they are blended with -- the four sub-cells of the
// world grid the point falls between.
class VITerrainProfileBlend
{
public:
    int   m_index[4];    // +0x00
    float m_weight[4];   // +0x10
};

// SUPPORT 0x0049A3E8, its own class: the three radial-flora tables a zone
// carries.  Its address is what VIZone::Clear hands `Clear__17VIZoneRadialFlora`
// -- `this + 0x74` -- and the three VIArray constructors the zone's own
// constructor calls in a row are its implicit one.
class VIZoneRadialFlora
{
public:
    VIArray<VIRadialFloraModel> m_models;  // +0x00
    VIArray<VIRadialFloraDist>  m_dists;   // +0x08
    VIArray<VIRadialFloraSet>   m_sets;    // +0x10

    int Clear();
    int Precalc(VIScene &sc);
};

// ---------------------------------------------------------------------------
// The Vars structs.  Every traversal in these two files takes ONE reference,
// built on VIScene's stack, and threads it all the way to the leaf -- so the
// recursion carries two registers no matter how many inputs the query has.
// Only the fields VIWorld and VIZone touch are named; VIScene (not yet in this
// corpus) owns the rest.
// ---------------------------------------------------------------------------
class VISceneCollVars
{
public:
    int            m_stamp;    // +0x00  the world's own pass counter
    VIScene       *m_scene;    // +0x04
    VISceneFilter *m_filter;   // +0x08
    VICollSphere  *m_sphere;   // +0x0C
    VICollide     *m_collide;  // +0x10
    VIVect3        m_start;    // +0x14
    VIVect3        m_end;      // +0x20
    float          m_radius;   // +0x2C
};

// The same struct with no radius: the ray descent reads the 0.03125 epsilon on
// its own where the sphere descent reads `m_radius + eps`.
class VISceneCollRayVars
{
public:
    int            m_stamp;    // +0x00
    VIScene       *m_scene;    // +0x04
    VISceneFilter *m_filter;   // +0x08
    VICollRay     *m_ray;      // +0x0C
    VICollide     *m_collide;  // +0x10
    VIVect3        m_start;    // +0x14
    VIVect3        m_end;      // +0x20
};

class VISceneRendVars
{
public:
    int          m_stamp;      // +0x00
    VIScene     *m_scene;      // +0x04
    VIWorld     *m_world;      // +0x08  RenderByFrustum stores `this` here
    VICamera    *m_camera;     // +0x0C
    VIRaster    *m_raster;     // +0x10
    VIClipStack *m_clip;       // +0x14
    int          m_numVerts;   // +0x18  the four GetTop out-params
    VIVect3     *m_verts;      // +0x1C
    int          m_numPlanes;  // +0x20
    VIPlane     *m_planes;     // +0x24
    int          m_p28;        // +0x28  RenderByFrustum sets it to 1
    int          m_p2C;        // +0x2C
    int          m_zone;       // +0x30
    float        m_viewDist;   // +0x34  squared against the room's bbox
};

class VISceneInsertVars
{
public:
    int      m_stamp;      // +0x00
    int      m_actor;      // +0x04
    int      m_zone;       // +0x08
    VIScene *m_scene;      // +0x0C
    VIVect3  m_corners[8]; // +0x10
    // Phase 54.  A VISphere, not a loose vector and float: VIScene::UpdateActor
    // zeroes FOUR words here twice over, which is VISphere's double-zeroing
    // constructor, where a VIVect3 member and a bare float would leave the
    // radius alone.
    VISphere m_bound;      // +0x70
};

class VIScenePickVars
{
public:
    int            m_stamp;    // +0x00
    VIScene       *m_scene;    // +0x04
    VISceneFilter *m_filter;   // +0x08
    VIPickSegment *m_seg;      // +0x0C
    VIRaster      *m_raster;   // +0x10  SetPreTranslations / SetModelView
    VIVect3        m_start;    // +0x14
    VIVect3        m_end;      // +0x20
};

class VISceneStreamVars
{
public:
    int           m_stamp;   // +0x00
    VIScene      *m_scene;   // +0x04
    VIDictionary *m_dict;    // +0x08
    VIRaster     *m_raster;  // +0x0C
    int           m_p10;     // +0x10
    // Phase 54, off VIScene::StreamClearCache, which fills one: +0x14 takes
    // VIScene::m_collide and +0x10 is never written.
    VICollide    *m_collide; // +0x14
    VILoader     *m_loader;  // +0x18
    int           m_file;    // +0x1C
    int           m_zone;    // +0x20
};

class VISceneStreamRadialVars : public VISceneStreamVars
{
public:
    float   m_inner;   // +0x24
    float   m_outer;   // +0x28
    // Phase 54: a FLOAT as well -- StreamFillCache stores m_p2A48[3] into it
    // with an swc1, and VIZone::StreamLeaf squares it.
    float   m_p2C;     // +0x2C
    // Phase 54: a FLOAT.  StreamClearCache stores VIScene::m_p2A48[2] here
    // with an swc1, and VIZone::Stream squares it against a float distance.
    float   m_p30;     // +0x30
    float   m_radius;  // +0x34
    VIVect3 m_at;      // +0x38
};

class VISceneBBoxStreamVars : public VISceneStreamVars
{
public:
    VIBBox  m_bbox;        // +0x24  the inner box, tested against a zone's
    VIBBox  m_outer;       // +0x3C
    VIVect3 m_corners[8];  // +0x54  the eight the descent walks
};

class VISceneProxQueryVars
{
public:
    int           m_stamp;   // +0x00
    VIScene      *m_scene;   // +0x04
    VISphere      m_sphere;  // +0x08  c at +0x08, r at +0x14
    VISceneQuery *m_query;   // +0x18
};

class VISceneVisQueryVars
{
public:
    int           m_stamp;     // +0x00
    VIScene      *m_scene;     // +0x04
    VISphere      m_sphere;    // +0x08
    VISceneQuery *m_query;     // +0x18
    VIWorld      *m_world;     // +0x1C  QueryVisibleBySphere stores `this`
    VIClipStack *m_clip;       // +0x20
    int          m_numVerts;   // +0x24
    VIVect3     *m_verts;      // +0x28
    int          m_numPlanes;  // +0x2C
    VIPlane     *m_planes;     // +0x30
    int          m_depth;      // +0x34  incremented for the walk, zeroed after
    int          m_p38;        // +0x38  QueryVisibleLeaf's skip flag
};

class VISceneBBoxQueryVars
{
public:
    int           m_stamp;      // +0x00
    VIScene      *m_scene;      // +0x04
    VIBBox        m_bbox;       // +0x08
    VISceneQuery *m_query;      // +0x20
    VIVect3       m_corners[8]; // +0x24
};

class VISceneEmitterQueryVars
{
public:
    int             m_stamp;   // +0x00
    VIScene        *m_scene;   // +0x04
    VICamera       *m_camera;  // +0x08
    VISphere        m_sphere;  // +0x0C  c at +0x0C, r at +0x18
    VIEmitterQuery *m_query;   // +0x1C
};

// Phase 54.  Read off VIScene::QueryIntersection, which BUILDS one: the bbox
// is copied into +0x04 (six floats off the caller's), the four pointers go in
// at +0x1C .. +0x28, VIBBox::Store fills the corners at +0x2C and
// VISphere::Init(bbox) the sphere at +0x8C.  VIZone::QueryIntersectionRoom
// confirms every one of them -- m_scene at +0x1C carries the stamp,
// m_filter at +0x20 and m_tris at +0x28 are handed straight to
// VIScene::QueryIntersectionActor, and m_collide at +0x24 takes the
// pre-translations.
class VISceneIntersectVars
{
public:
    int             m_stamp;      // +0x00
    VIBBox          m_bbox;       // +0x04
    VIScene        *m_scene;      // +0x1C
    VIActorFilter  *m_filter;     // +0x20
    VICollide      *m_collide;    // +0x24  the pre-translation sink
    VIQueryTriList *m_tris;       // +0x28
    VIVect3         m_corners[8]; // +0x2C
    VISphere        m_sphere;     // +0x8C
};

// ---------------------------------------------------------------------------
// VIZone -- 592 bytes, the stride every `m_zones.m_data + i` multiply states.
//
// DECLARED ONLY.  `vizone.cpp` is phase 52; the layout and the method set are
// here because `viworld.cpp` needs the size, the tree and the room array to
// index them, and the harness assembles rather than links.
// ---------------------------------------------------------------------------
class VIZone
{
public:
    VIZone();
    ~VIZone();

    int Init();
    int Clear();
    int ResetToProxy(VIScene &sc);

    int RenderByVolume(VISceneRendVars &v, int node);
    int RenderByVolume(int node, VISceneRendVars &v);
    int RenderLeaf(int leaf, VISceneRendVars &v);
    int RenderRoom(int room, VISceneRendVars &v);
    int RenderThroughPortals(int room, VISceneRendVars &v);
    int SetPretranslations(VIRaster &r, VICamera &cam);

    int Collide(VISceneCollVars &v);
    int Collide(int node, VISceneCollVars &v);
    int CollideLeaf(int leaf, VISceneCollVars &v);
    int CollideRoom(int room, VISceneCollVars &v);
    int Collide(VISceneCollRayVars &v);
    int Collide(int node, VISceneCollRayVars &v);
    int CollideLeaf(int leaf, VISceneCollRayVars &v);
    int CollideRoom(int room, VISceneCollRayVars &v);

    int InsertActor(VISceneInsertVars &v);
    int InsertActor(int node, VISceneInsertVars &v);

    int FindLeaf(const VIVect3 &v) const;

    int Pick(VIScenePickVars &v);
    int Pick(int node, VIScenePickVars &v);
    int PickLeaf(int leaf, VIScenePickVars &v);
    int PickRoom(int room, VIScenePickVars &v);

    int Stream(VISceneStreamRadialVars &v);
    int Stream(int node, VISceneStreamRadialVars &v);
    int StreamLeaf(int leaf, VISceneStreamRadialVars &v);
    int Stream(VISceneBBoxStreamVars &v);
    int Stream(int node, VISceneBBoxStreamVars &v);
    int StreamLeaf(int leaf, VISceneBBoxStreamVars &v);
    int StreamRoomIn(int room, VISceneStreamVars &v);
    int StreamRoomHalt(int room, VISceneStreamVars &v);
    int StreamRoomOut(int room, VISceneStreamVars &v);
    int StreamRoomStaticsIn(int room, VISceneStreamVars &v);
    int StreamRoomStaticsHalt(int room, VISceneStreamVars &v);
    int StreamRoomStaticsOut(int room, VISceneStreamVars &v);

    // Phase 54: the argument NAMES come from VIScene::StreamInStaticActor,
    // which passes VISceneStreamVars::m_zone second and m_file fourth.
    int Load(unsigned int id, int zone, VILoader &l, int file);
    int Find(unsigned int id, unsigned long &off, unsigned int &size);

    int Collide(VIEnvRay &r, VIVect3 &a, VIVect3 &b);
    int Collide(int node, VIEnvRay &r, VIVect3 &a, VIVect3 &b);
    int CollideLeaf(int leaf, VIEnvRay &r, VIVect3 &a);

    int QueryProximal(VISceneProxQueryVars &v);
    int QueryProximal(int node, VISceneProxQueryVars &v);
    int QueryProximalLeaf(int leaf, VISceneProxQueryVars &v);
    int QueryProximalRoom(int room, VISceneProxQueryVars &v);
    int QueryVisible(VISceneVisQueryVars &v);
    int QueryVisible(int node, VISceneVisQueryVars &v);
    int QueryVisibleLeaf(int leaf, VISceneVisQueryVars &v);
    int QueryVisibleRoom(int room, VISceneVisQueryVars &v);
    int QueryThroughPortals(int room, VISceneVisQueryVars &v);
    int QueryVolume(VISceneBBoxQueryVars &v);
    int QueryVolume(int node, VISceneBBoxQueryVars &v);
    int QueryVolumeLeaf(int leaf, VISceneBBoxQueryVars &v);
    int QueryVolumeRoom(int room, VISceneBBoxQueryVars &v);
    int QueryEmitters(VISceneEmitterQueryVars &v);
    int QueryEmitters(int node, VISceneEmitterQueryVars &v);
    int QueryEmittersLeaf(int leaf, VISceneEmitterQueryVars &v);
    int QueryEmittersRoom(int room, VISceneEmitterQueryVars &v);
    int QueryIntersection(VISceneIntersectVars &v);
    int QueryIntersection(int node, VISceneIntersectVars &v);
    int QueryIntersectionLeaf(int leaf, VISceneIntersectVars &v);
    int QueryIntersectionRoom(int room, VISceneIntersectVars &v);

    unsigned int  m_flags;      // +0x00   bit 0 == the zone's data is resident
    int           m_p04;        // +0x04
    // Phase 58: an `ld` in ParseWorldZones, handed straight to the
    // ReadBegin(offset, ...) overload -- the file offset of the zone's BASE
    // object, where m_offset at +0x10 is the offset of its streamed data.
    unsigned long m_baseOffset;  // +0x08
    unsigned long m_offset;     // +0x10   handed straight to VILoader::Load
    unsigned int  m_size;       // +0x18
    int           m_trans;      // +0x1C   the loader transaction, -1 when idle
    VIVect3       m_p20;        // +0x20
    VIBBox        m_bbox;       // +0x2C
    VIZoneTree    m_tree;       // +0x44
    VIArray<VIVect3>              m_pretrans;    // +0x54
    VIArray<VIZoneRoom>           m_rooms;       // +0x5C
    VIArray<VIResourceElem>       m_resources;   // +0x64
    VIArray<VIZoneStaticTableElem> m_statics;    // +0x6C
    VIZoneRadialFlora             m_flora;       // +0x74
    int           m_p8C;        // +0x8C
    VIMatrix44    m_matrix;     // +0x90
    VIVect3       m_p[32];      // +0xD0 .. 0x250

    static VIMatrix44 IdentityMatrix;
};

// ---------------------------------------------------------------------------
// VIWorld -- 976 bytes.  The terrain grid, the zone array, and the BSP over it.
// ---------------------------------------------------------------------------
class VIWorld
{
public:
    VIWorld();
    ~VIWorld();

    int Init(int numZones, int numRows, int numCols, int subdiv, float cellSize,
             const VIVect3 &origin, const VIBBox &bb);
    int Clear();
    int IsEmpty();
    int SetProfiles();

    int GetTerrainProfile(int row, int col) const;
    int CalcTerrainProfile(const VIVect3 &at) const;
    int CalcTerrainCell(const VIVect3 &at, int &row, int &col) const;
    int CalcTerrainCellBVolume(int row, int col, VIBBox &bb) const;
    int CalcProfileBlend(const VIVect3 &at, VITerrainProfileBlend &b);

    int RenderByFrustum(VISceneRendVars &v);
    int RenderByPortals(VISceneRendVars &v);
    int RenderByVolume(VISceneRendVars &v);
    int RenderByVolume(int node, VISceneRendVars &v);

    int Collide(VISceneCollVars &v);
    int Collide(int node, VISceneCollVars &v);
    int Collide(VISceneCollRayVars &v);
    int Collide(int node, VISceneCollRayVars &v);

    int InsertActor(VISceneInsertVars &v);
    int InsertActor(int node, VISceneInsertVars &v);

    int Pick(VIScenePickVars &v);
    int Pick(int node, VIScenePickVars &v);

    int Stream(VISceneStreamRadialVars &v);
    int Stream(int node, VISceneStreamRadialVars &v);
    int StreamLeaf(int leaf, VISceneStreamRadialVars &v);
    int Stream(VISceneBBoxStreamVars &v);
    int Stream(int node, VISceneBBoxStreamVars &v);
    int StreamLeaf(int leaf, VISceneBBoxStreamVars &v);

    int FindLeaf(const VIVect3 &at, int &zone, int &leaf) const;

    int Collide(VIEnvRay &r);
    int Collide(int node, VIEnvRay &r, VIVect3 &a, VIVect3 &b);

    int QueryProximal(VISceneProxQueryVars &v);
    int QueryProximal(int node, VISceneProxQueryVars &v);
    int QueryVisibleBySphere(VISceneVisQueryVars &v);
    int QueryVisibleWithPortals(VISceneVisQueryVars &v);
    int QueryVisible(VISceneVisQueryVars &v);
    int QueryVisible(int node, VISceneVisQueryVars &v);
    int QueryVolume(VISceneBBoxQueryVars &v);
    int QueryVolume(int node, VISceneBBoxQueryVars &v);
    int QueryEmitters(VISceneEmitterQueryVars &v);
    int QueryEmitters(int node, VISceneEmitterQueryVars &v);
    int QueryIntersection(VISceneIntersectVars &v);
    int QueryIntersection(int node, VISceneIntersectVars &v);

    VIBBox      m_bbox;      // +0x00   -1000 .. +1000
    int         m_numRows;   // +0x18   1
    int         m_numCols;   // +0x1C   1
    float       m_cellSize;  // +0x20   2000.0
    VIVect3     m_origin;    // +0x24
    int         m_subdiv;    // +0x30   4
    VIWorldTree m_tree;      // +0x34   its m_state is the pass STAMP
    VIArray<VIZone>         m_zones;    // +0x48
    VIArray<VIWorldRegion>  m_regions;  // +0x50
    VITerrainProfile m_profiles[22];    // +0x58 .. 0x3C8
};

// The three VIScene enumerations, named by the mangled parameter types:
// `SetPersonalLightMode__7VIScene19VIPersonalLightMode`,
// `SetRadialFloraMode__7VIScene17VIRadialFloraMode` and
// `SetActorBBoxDrawStyle__7VIScene20VISceneBBoxDrawStyle`.  Only the mode
// VIScene::Init selects is known (0 for both, and the constructor leaves the
// draw style at 0), so the members are spelled but the values are not.
enum VIPersonalLightMode
{
    VIPERSONALLIGHT_OFF = 0
};

enum VIRadialFloraMode
{
    VIRADIALFLORA_OFF = 0
};

enum VISceneBBoxDrawStyle
{
    VISCENEBBOX_NONE = 0
};

// A reference map: a VIResource (the DictID and the share count VIScene's
// Share/Release pair steps) with a VIMap<int,int> at +0x08 -- VIScene::Clear
// destroys exactly that member and then `delete`s the object.
class VIRefMap : public VIResource
{
public:
    VIMap<int, int> m_map;   // +0x08
};

// -------------------------------------------------------------- VIAtmosphere --
// SUPPORT 0x00416888 .. 0x0041C7C8, its own TU.  Phase 55.  The sky and the
// weather: the sky dome and its three rotation matrices, the 128 stars and the
// shooting star, the lightning flash and its 32 patches, the rain/snow particle
// field with its splash pool, and the Lavastorm fireballs.  4,528 bytes, and
// VIScene embeds one at +0x640.
//
// The LAYOUT is the constructor's member roll-call (phase 53) read against the
// destructor's reverse order: VILList at +0x14, +0x24, +0x1160 and +0x1170,
// VIMatrix44 at +0x500, +0x540 and +0x580, and the zeroing loops give
// VIVect3[2] at +0x50 and +0x68, VIVect3[64] at +0xA8, VIVect4[128] at +0x620
// and VIVect4[32] at +0xF60.  The class is aligned(16) through VIVect4, which
// is why 0x11AC rounds to 0x11B0.
class VIAtmosphere
{
public:
    enum WXTYPE
    {
        WXCLEAR = 0,
        WXRAIN,
        WXSNOW,
        WXSLEET,
        WXSANDSTORM,
        WXLAVASTORM,
        WXTHUNDERSTORM
    };

    enum SKYTYPE
    {
        SKYCLEAR = 0,
        SKYOVERCAST,
        SKYSTORM,
        SKYNIGHT
    };

    // The Lavastorm fireball: a VILNode with 48 bytes of its own and then TWO
    // VIParticleEmitters, at +0x40 and +0x420.  Clear's `delete []` is what
    // proves it a class -- the 16-byte array-new cookie, the backwards
    // destructor loop with a 2048-byte stride, and both emitter destructors in
    // reverse declaration order.
    // The weather particle: 36 bytes, and NOT a VILNode -- Init allocates the
    // whole field as one `new char[36 * n]` and Clear deletes it with no
    // array-new cookie.  RenderWeather is what fixes the colour at +0x0C: the
    // sixteen bytes it block-copies into the billboard.
    class VIWeatherParticle
    {
    public:
        VIVect3    m_location;   // +0x00
        VIColor32F m_color;      // +0x0C
        float      m_p1C;        // +0x1C
        int        m_p20;        // +0x20
    };

    // The rain splash: a VILNode, a world position, a brightness and a frame
    // counter that runs 0..4.  36 bytes, which is the stride ClearAllSplashes
    // walks the pool with.
    class VISplash : public VILNode
    {
    public:
        VIVect3 m_location;   // +0x10
        float   m_bright;     // +0x1C
        int     m_frame;      // +0x20
    };

    class VIFireball : public VILNode
    {
    public:
        char              m_p10[48];   // +0x10
        VIParticleEmitter m_e0;        // +0x40
        VIParticleEmitter m_e1;        // +0x420
    };

    VIAtmosphere();
    ~VIAtmosphere();

    int   Init(long time, VIRaster &r, int n);
    int   Clear(VIScene &sc);
    int   SetResources(VIScene &sc);
    int   SetWeather(WXTYPE t, bool fade);
    int   SetPlaneOfSky(int i);
    int   SetDefaultSky(SKYTYPE t, bool fade);
    int   SetSkyColor(VIScene &sc, const VIColor32 &c);
    int   Update(VIScene &sc, VICamera &cam, long time, float a, float b);
    int   RenderSky(VIScene &sc, VICamera &cam);
    void  RenderWeather(VIScene &sc, VICamera &cam);
    float GetOvercastInterp();
    float EffectMix() const;
    void  SetEffectMix(float f);
    float Intensity() const;
    void  SetIntensity(float f);
    float LavastormIntensity() const;
    void  SetLavastormIntensity(float f);
    float ParticleScale() const;
    void  SetParticleScale(float f);
    int   ProcessWeather(VIScene &sc, VICamera &cam);
    int   ProcessTransitions(VIScene &sc, float a, float b);
    void  ResetLightning();
    void  RecomputeLightningPatches();
    int   GetLightning(float *out);
    void  RenderStars(VIScene &sc, VICamera &cam);
    int   RenderSplashes(VIScene &sc, VICamera &cam);
    void  AddSplash(VIVect3 &p);
    void  ClearAllSplashes();
    void  ClearAllFireballs();
    int   ProcessLavastorm(VIScene &sc, VICamera &cam);

    VIParticleSystem *m_p00;        // +0x0000  the scene's own
    float       m_range;            // +0x0004  200.0
    int         m_numParticles;     // +0x0008  200
    VIWeatherParticle *m_particles; // +0x000C  m_numParticles * 36
    float       m_particleScale;    // +0x0010
    VILList     m_splashList;       // +0x0014  active splashes
    int         m_numSplashes;      // +0x0020
    VILList     m_splashFree;       // +0x0024
    char       *m_splashNodes;      // +0x0030  m_numParticles * 36
    int         m_splashTex;        // +0x0034
    int         m_splashMat;        // +0x0038
    VIVect3     m_camera;           // +0x003C
    float       m_intensity;        // +0x0048
    float       m_lavastorm;        // +0x004C
    VIVect3     m_p50[2];           // +0x0050
    VIVect3     m_p68[2];           // +0x0068
    float       m_effectMix;        // +0x0080
    float       m_p84;              // +0x0084
    float       m_p88[2];          // +0x0088
    float       m_p90[2];          // +0x0090
    float       m_p98[2];          // +0x0098
    int         m_mat[2];           // +0x00A0
    VIVect3     m_pA8[64];          // +0x00A8
    float       m_p3A8[64];         // +0x03A8
    float       m_p4A8;             // +0x04A8  DeltaTime()
    float       m_p4AC[8];          // +0x04AC  all 1/30
    int         m_initialised;      // +0x04CC
    int         m_skyTex[3];        // +0x04D0
    int         m_starMat;          // +0x04DC
    int         m_skySprite;        // +0x04E0
    int         m_p4E4[2];          // +0x04E4
    int         m_sun;           // +0x04EC
    int         m_corona;             // +0x04F0
    int         m_moon;              // +0x04F4
    VIMatrix44  m_skyRot;           // +0x0500
    VIMatrix44  m_coronaRot;        // +0x0540
    VIMatrix44  m_moonRot;          // +0x0580
    float       m_skyScale;         // +0x05C0
    int         m_overcast;         // +0x05C4
    long        m_p5C8;             // +0x05C8
    long        m_p5D0;             // +0x05D0
    long        m_p5D8;             // +0x05D8
    int         m_p5E0;             // +0x05E0
    int         m_p5E4;             // +0x05E4
    int         m_p5E8;             // +0x05E8
    int         m_p5EC;             // +0x05EC
    int         m_p5F0;             // +0x05F0
    int         m_p5F4;             // +0x05F4
    int         m_p5F8;             // +0x05F8
    int         m_p5FC;             // +0x05FC
    int         m_p600;             // +0x0600
    int         m_p604;             // +0x0604
    float       m_p608;             // +0x0608
    int         m_p60C;             // +0x060C
    int         m_p610;             // +0x0610
    int         m_p614;             // +0x0614
    int         m_p618;             // +0x0618
    float       m_p61C;             // +0x061C
    VIVect4     m_stars[128];       // +0x0620
    char        m_pE20[128];        // +0x0E20
    float       m_pEA0[32];         // +0x0EA0
    int         m_starTex;          // +0x0F20
    float       m_pF24;             // +0x0F24
    float       m_pF28;             // +0x0F28
    float       m_pF2C;             // +0x0F2C
    VIVect3     m_pF30;             // +0x0F30
    VIVect3     m_pF3C;             // +0x0F3C
    float       m_pF48;             // +0x0F48
    int         m_pF4C;             // +0x0F4C
    int         m_pF50;             // +0x0F50
    VIVect4     m_patches[32];      // +0x0F60
    VILList     m_fireballList;     // +0x1160
    int         m_numFireballs;     // +0x116C
    VILList     m_fireballFree;     // +0x1170
    VIFireball *m_fireballNodes;    // +0x117C  32 * 2048
    int         m_p1180;            // +0x1180
    int         m_p1184;            // +0x1184
    int         m_p1188;            // +0x1188
    int         m_p118C;            // +0x118C
    int         m_p1190;            // +0x1190
    int         m_p1194;            // +0x1194
    float       m_p1198;            // +0x1198
    float       m_p119C;            // +0x119C
    float       m_p11A0;            // +0x11A0
    float       m_p11A4;            // +0x11A4
    float       m_p11A8;            // +0x11A8
};

// ------------------------------------------------------------------ VIScene --
// SUPPORT 0x004613A8 .. 0x0046CC80.  Phase 54.  The scene: the world tree, the
// actors placed in it, the sprite/animation/refmap/lighting pools every actor
// draws from, the particle system, the atmosphere, the spell effects, the
// radial flora and the 24-entry time-of-day table.  19,376 bytes.
//
// The LAYOUT is read off the constructor's member roll-call (phase 53's lever):
// everything between the prologue and the tail `Clear(this)` is a member with a
// constructor of its own, in declaration order, and the address each is given
// is its offset.  Where a member has no constructor, `Clear` and `Init` name it
// instead -- both walk the whole object.
//
// **An actor "index" is an ADDRESS.**  Every Set/Get/IsActor* takes an `int`
// and dereferences it: `SetActorID(int actor, int id)` is `sw a2,132(a1)`.  The
// same spelling VIZone's room lists use (phase 52), and it is why the actor
// calls have no bounds check anywhere.
class VIScene
{
public:
    // 24 bytes, and a POD: VIList<VIScene::VISceneOccup>'s node is 32 with
    // m_next at +24, and its Clear is the same 24 instructions as the two
    // POINTER instantiations, so the element has neither constructor nor
    // destructor.
    // Phase 54 reads the fields off RemoveActor and RemoveActorsFromZone: one
    // record per (actor, room) pair, on TWO lists at once -- the actor's, whose
    // head and tail are VIActor's +0x98/+0x9C, and the room's, whose head and
    // tail are VIZoneRoom's m_p68/m_p6C.  Both are spelled as `int` cursors
    // ending at -1 (phase 52's lever).
    class VISceneOccup
    {
    public:
        short    m_zone;       // +0x00
        short    m_room;       // +0x02
        VIActor *m_actor;      // +0x04
        int      m_actorNext;  // +0x08
        int      m_actorPrev;  // +0x0C
        int      m_next;       // +0x10
        int      m_prev;       // +0x14
    };

    // One row of the time-of-day table: 108 bytes, and the constructor zeroes
    // it in exactly this order -- a two-element colour array, three ints, and
    // two more two-element colour arrays.  There are 24 of them, one per hour.
    class VISceneTimeOfDay
    {
    public:
        VIColor32F m_a[2];   // +0x00
        VIVect3    m_b;      // +0x20  zeroed by the CONSTRUCTOR, so it is a
                             //        VIVect3 and not three loose ints
        VIColor32F m_c[2];   // +0x2C
        VIColor32F m_d[2];   // +0x4C
    };

    VIScene();

    int   Init(long time, VIDictionary &d, VIRaster *r, VISoundDevice *sd,
               VICollide &c);
    int   Clear();

    int   SetTime(long t);
    long  GetTime();
    int   SetTimeOfDay(float t);
    int   InitTimeOfDay();
    int   SetupTimeOfDay(VICamera &cam, VITerrainProfileBlend &b, unsigned int f);
    int   SetupTimeOfDayLights(VICamera &cam, VITerrainProfileBlend &b);
    int   SetupTimeOfDayFog(VITerrainProfileBlend &b, unsigned int f);
    int   SetupTimeOfDaySky(VITerrainProfileBlend &b);
    int   CalcTimeOfDayClipPlanes(VITerrainProfileBlend &b, float &a, float &c);

    int   SetFarPlane(VICamera &cam);
    float CalcFarPlane(const VIVect3 &at);
    float GetViewDistance(const VIVect3 &at) const;

    int   SetSky(VICamera &cam, int i);
    int   SetSky();
    int   SetPortals(int on);
    unsigned int GetEnvFlags(const VIVect3 &at);
    int   CalcOutdoorsIntensity(VICamera &cam);

    int   SetSoundTrack(VICamera &cam, VILoader &l, int i);
    int   StopSoundTrack(VILoader &l, int i);
    int   UpdateSoundEmitters(VICamera &cam);

    int   Render(VICamera &cam);
    int   RenderDeferredList(VICamera &cam, int a, int b);
    void  ClearShadowQueue();
    int   RenderShadows();
    int   RenderShadow(int i);
    int   DebugResources();

    // Named from the DUMP, not from plausibility: eqcalls.py reads each jal's
    // target out of a resident SUPPORT and looks it up in the same symbol map.
    // Two of these were guessed wrong for four phases -- `GetSprite` is
    // `Sprite`, and VIDictionary's is `Remove`, not `Release`.
    int       DitherLevel();
    // Phase 41, off VILoader: a resource the loader parsed is handed to the
    // scene by DictID, and released the same way.
    int       ShareResource(unsigned int id);
    int       ReleaseResource(unsigned int id);

    // Phase 47, off VIHSprite::AddPlay: the scene owns the animation clips and
    // hands one back by index.
    int            CreateAnimation(int share);
    VIHSpriteAnim *Animation(int i);
    int            ShareAnimation(int i);
    int            ReleaseAnimation(int i);

    int       CreateRefMap(int share);
    VIRefMap *RefMap(int i);
    int       ShareRefMap(int i);
    int       ReleaseRefMap(int i);

    int               CreateStaticLighting(int share);
    VIStaticLighting *StaticLighting(int i);
    int               ShareStaticLighting(int i);
    int               ReleaseStaticLighting(int i);

    int       CreateSprite(VISpriteType type, int n);
    int       CopySprite(int i, int n);
    VISprite *Sprite(int i);
    int       GetSpriteType(int i);
    int       ShareSprite(int i);
    int       ReleaseSprite(int i);
    int       RasterSprite(VICamera &cam, int i, unsigned int flags);
    int       RasterUISprite(VICamera &cam, int i, unsigned int flags);

    int       GetNullSprite();
    int       SetLightUISprite(int i);
    int       GetLightUISprite();
    int       SetSoundUISprite(int i);
    int       GetSoundUISprite();
    int       SetParticleUISprite(int i);
    int       GetParticleUISprite();
    int       SetPointUISprite(int i);
    int       GetPointUISprite();
    int       SetStreamAudioUISprite(int i);
    int       GetStreamAudioUISprite();

    int   SetPersonalLightMode(VIPersonalLightMode m);
    int   SetPersonalLight(int actor, float r, const VIColor32F &c);
    int   SetPersonalLight(const VIVect3 &at, float r, const VIColor32F &c);
    int   SetRadialFloraMode(VIRadialFloraMode m);
    void  ResetRadialFlora();
    int   GetNextColorBuffer();

    int    GetWorldGridCenter(int a, int b, VIVect3 &v) const;
    int    CalcWorldGridOffset(const VIVect3 &v, int &a, int &b, VIVect3 &out) const;
    int    GetWorldGridNumRows();
    int    GetWorldGridNumCols();
    float  GetWorldGridSize();
    VIBBox *GetWorldBBox();

    int   Collide(VICollSphere &s, VISceneFilter &f);
    int   Collide(VICollRay &r, VISceneFilter &f);
    int   Collide(VICollSphere &s, VISceneFilter &f, VISceneQuery &q);
    int   Collide(VIEnvRay &r);
    int   Pick(VIPickSegment &seg, VISceneFilter &f);

    int   StreamFillCache(const VIVect3 &at, VILoader &l, int i);
    int   StreamFillCache(const VIBBox &bb, VILoader &l, int i);
    int   Stream(const VIVect3 &at, VILoader &l, int i);
    int   StreamClearCache(VILoader &l, int i);

    int   CreateActor(const VIVect3 &a, const VIVect3 &b, float f, int n);
    int   CreateStaticProxyActor(const VIVect3 &a, const VIVect3 &b, float f,
                                 unsigned int id, VIColor32 &c);
    int   ReleaseActor(int i);
    int   SetActorLighting(int i, int lighting);
    int   IsActorProxy(int i);
    int   IsActorStatic(int i);
    int   SetActorTransform(int i, const VIVect3 &hpr, float scale, const VIVect3 &at);
    int   GetActorTransform(int i, VIMatrix44 &m);
    int   SetActorLocation(int i, const VIVect3 &v);
    int   SetActorHPR(int i, const VIVect3 &v);
    int   SetActorScale(int i, float f);
    int   SetActorSprite(int i, int sprite);
    int   SetActorVisibility(int i, int on);
    int   IsActorVisible(int i);
    int   SetActorBBoxVisibility(int i, int on);
    int   IsActorBBoxVisible(int i);
    int   SetActorCollideable(int i, int on);
    int   IsActorCollideable(int i);
    int   SetActorAudible(int i, int on);
    int   IsActorAudible(int i);
    int   SetActorID(int i, int id);
    int   GetActorID(int i);
    int   SetActorBBoxDrawStyle(VISceneBBoxDrawStyle s);
    int   GetActorSpriteType(int i) const;
    // Phase 38.  GetActorSpritePtr hands back the sprite itself -- the caller
    // reads VISprite::m_type off it at +0x34 and only then treats it as a
    // VICSprite -- where GetActorSprite returns the sprite INDEX.
    VISprite *GetActorSpritePtr(int i) const;
    int       GetActorSprite(int i) const;

    int   CollideActor(VICollSphere &s, VISceneFilter &f, int i);
    int   CollideSprite(VICollSphere &s, VISceneFilter &f, int i);
    int   CollideActor(VICollRay &r, VISceneFilter &f, int i);
    int   CollideSprite(VICollRay &r, VISceneFilter &f, int i);
    int   PickActor(VIPickSegment &seg, VISceneFilter &f, int i);
    int   PickSprite(VIPickSegment &seg, VISceneFilter &f, int i);
    int   RasterActor(VICamera &cam, int i, VIClipStack &cs);
    int   RasterActorBBox(VICamera &cam, int i);

    int   QueryProximalActors(const VISphere &s, VISceneQuery &q);
    int   QueryVisibleActors(const VISphere &s, VISceneQuery &q);
    int   QueryActorsInVolume(const VIBBox &bb, VISceneQuery &q);
    int   QueryProximalActor(int i, VISphere &s, VISceneQuery &q);
    int   QueryVisibleActor(int i, VISphere &s, VISceneQuery &q);
    int   QueryVolumeActor(int i, VIBBox &bb, VISceneQuery &q);
    int   QueryEmittersAtLocation(const VICamera &cam, VIEmitterQuery &q);
    int   QueryEmittersAtLocationActor(int i, const VICamera &cam, VIEmitterQuery &q);
    int   QueryEmittersActor(int i, const VICamera &cam, const VISphere &s,
                             VIEmitterQuery &q);
    int   QueryIntersection(VIBBox &bb, VIActorFilter &f, VIQueryTriList &tl);
    int   QueryIntersectionActor(int i, VIBBox &bb, VIActorFilter &f, VIQueryTriList &tl);
    int   QueryIntersectionSprite(int i, VIBBox &bb, const VIMatrix44 &m,
                                  VIQueryTriList &tl);

    int   UpdateActor(int i);
    int   UpdateProxyActor(int i);
    int   InsertActor(int actor, int zone, int room);
    int   RemoveActor(int i);
    int   RemoveAllActors();
    int   UpdateAllActors();
    // Phase 51, off VIWorld::StreamLeaf and VIZone's room streamers.
    int   RemoveActorsFromZone(VIZone &z, VIArray<int> &a);
    int   UpdateActors(VIArray<int> &a);
    int   UpdateActors();
    int   ReplaceProxyRooms(VIZone &z);
    int   ReplaceStaticProxyActors(VIZone &z);
    int   ReplaceProxyActor(int i, int n);
    int   ReleaseStatic(VIZone &z, VILoader &l);
    int   StreamInStaticActor(int i, VIZone &z, VISceneStreamVars &v);
    int   StreamInCompleteActor(int i, VISceneStreamVars &v);
    int   StreamHaltStaticActor(int i, VISceneStreamVars &v);
    int   StreamOutStaticActor(int i, VISceneStreamVars &v);

    int              m_p00;              // +0x0000  Clear: 0
    // VIRadialFloraSystem::CalcZoneIndex calls VIWorld::FindLeaf on
    // `m_scene + 4`, and Init/Clear/Collide/Pick/Stream all hand VIWorld this
    // address.  It is 968 bytes here, NOT the 976 phase 51 derived from
    // SetProfiles' last store: m_actors begins at +972 and the live scene in
    // `inworld-eqoa.live.eememory` has a VIList{302,ptr,ptr} there, so the
    // 22nd terrain profile is TRUNCATED by the end of the class.
    VIWorld          m_world;            // +0x0004
    VIList<VIActor>  m_actors;           // +0x03CC
    VIList<VISceneOccup> m_occups;       // +0x03D8
    VIPool<VISprite *>       m_sprites;  // +0x03E4
    VIPool<VIHSpriteAnim *>  m_anims;    // +0x03FC
    VIPool<VIRefMap *>       m_refMaps;  // +0x0414
    VIPool<VIStaticLighting *> m_lightings; // +0x042C
    int              m_p444;             // +0x0444  Clear: -1
    int              m_p448;             // +0x0448  Clear: -1
    int              m_p44C;             // +0x044C
    VIArray<int>     m_p450;             // +0x0450
    VIVector<VISceneDisplayElem> m_display;  // +0x0458
    int              m_nullSprite;       // +0x0464
    int              m_soundUISprite;    // +0x0468
    int              m_lightUISprite;    // +0x046C
    int              m_particleUISprite; // +0x0470
    int              m_pointUISprite;    // +0x0474
    int              m_streamAudioUISprite;  // +0x0478
    long             m_time;             // +0x0480
    VIMatrix44       m_p490;             // +0x0490
    VIMatrix44       m_p4D0;             // +0x04D0
    VIClipStack      m_clipStack;        // +0x0510
    // Phase 52: the SCENE's pass counter, a different clock from the world
    // tree's.  A VIZone room is walked once per value of this.
    int              m_stamp;            // +0x0544
    // The particle system is EMBEDDED at +0x548: VIParticleSprite::Create and
    // ::Copy both reach it as `scene + 1352` and then null-test the result,
    // which is an inline accessor returning its address.
    VIParticleSystem m_particleSystem;   // +0x0548  112 bytes (the `long` at
                                         //          +8 makes it 8-aligned)
    int              m_p5B8[32];         // +0x05B8  Init: all -1
    int              m_p638;             // +0x0638
    int              m_p63C;             // +0x063C
    VIAtmosphere     m_atmosphere;       // +0x0640
    VIQueryTriList   m_queryTris;        // +0x17F0
    float            m_farPlane;         // +0x187C  Clear: 2500.0f
    int              m_p1880;            // +0x1880  Clear: -1
    int              m_p1884;            // +0x1884  Clear: -1
    // The shadow queue: RenderShadows walks m_shadows[0 .. m_numShadows), and
    // +0x1A88 - +0x1888 is exactly 128 entries.
    int              m_shadows[128];     // +0x1888
    int              m_numShadows;       // +0x1A88  ClearShadowQueue: 0
    int              m_p1A8C;            // +0x1A8C
    VISceneSoundTrack m_soundTrack;      // +0x1A90
    VISpellEffectSystem m_spellEffects;  // +0x1B70
    float            m_p1FE0;            // +0x1FE0  the hour, 0 .. 23.99999
    // Zeroed among the MEMBER constructors, in VIColor32F's own order, so it
    // is a colour and not four loose ints.
    VIColor32F       m_p1FE4;            // +0x1FE4
    VISceneTimeOfDay m_tod[24];          // +0x1FF4 .. 0x2A14
    VIPersonalLightMode m_personalLightMode;  // +0x2A14
    VIVect3          m_personalLightAt;  // +0x2A18
    float            m_personalLightRadius;   // +0x2A24
    VIColor32F       m_personalLightColor;    // +0x2A28
    long             m_p2A38;            // +0x2A38  Clear: 0
    int              m_p2A40;            // +0x2A40  Clear: 0
    int              m_p2A44;            // +0x2A44
    // The streaming radii -- `claude-notes/streaming-radius-and-hdd-20260814.md`
    // calls them "four immediates in VIScene::Init", and this is where they
    // land: 950 / 900 / 550 / 500, with a five-entry twin above.
    float            m_p2A48[4];         // +0x2A48  950, 900, 550, 500
    float            m_p2A58[5];         // +0x2A58  1050, 950, 900, 550, 500
    int              m_p2A6C;            // +0x2A6C
    long             m_p2A70;            // +0x2A70  Init: 0
    VISceneBBoxDrawStyle m_bboxDrawStyle;     // +0x2A78
    VIRadialFloraSystem m_flora;         // +0x2A7C
    VIRadialFloraMode m_floraMode;       // +0x4B98
    // VIEffectVolume::AttachSurface reaches both of these off the static
    // scene pointer: `lw t0,19356(v1)` is the dictionary it hands
    // VIMaterial::SetLayerTexture, `lw s0,19360(v1)` the raster.
    VIDictionary    *m_dict;             // +0x4B9C
    // VIWalkCamera::SetCamera loads this with a plain `lw` at +0x4BA0.
    VIRaster        *m_raster;           // +0x4BA0
    // VIBattleMusic reaches the sound device as `lw s2,19364(a0)`.
    VISoundDevice   *m_sound;            // +0x4BA4
    VICollide       *m_collide;          // +0x4BA8
    int              m_p4BAC;            // +0x4BAC  Clear: 0

    VIParticleSystem *ParticleSystem() { return &m_particleSystem; }
};


// ===========================================================================
// VIRaster -- SUPPORT 0x003FF2E8 .. 0x0040C758, phase 56.  The renderer: one
// 19,540-byte object that owns every GS resource the engine has, the DMA
// chain it builds them into, and the immediate-mode vertex path.
//
// The class body lives HERE, at the bottom of the header, because its members
// are VIPool / VIList instantiations and those templates are declared above.
//
// Its lighting and matrix state is NOT in the object: GetModelView returns a
// bare 0x70000000 and every light setter writes an absolute scratchpad
// address, so the whole per-frame uniform block lives in SPR.  VIRasterSPR
// below is that block, addressed by cast.
// ===========================================================================

// 28 bytes: SetDirLight copies floats at 0, 4, 8 and then an unaligned 16-byte
// block from +12, which is a VIVect3 followed by a VIColor32F.  The array it
// indexes has a stride of 28 (`mult i, 28`).
class VIRasterDirLight
{
public:
    VIVect3    m_direction;   // +0x00
    VIColor32F m_color;       // +0x0C
};

// 32 bytes, and the array stride is a SHIFT (`sll i, 5`).  SetPointLight
// assigns the first three floats with VIVect3::operator= and the fourth on its
// own, and hands VIMatrix44::Mul -- which takes a VIVect3 -- the position, so
// the range is a SEPARATE member and not the w of a VIVect4.
class VIRasterPointLight
{
public:
    VIVect3    m_position;    // +0x00
    float      m_range;       // +0x0C
    VIColor32F m_color;       // +0x10
};

// The scratchpad uniform block at 0x70000000.  Every offset here is read off
// an absolute `lui at,0x7000` store, never off `this`.
class VIRasterSPR
{
public:
    VIMatrix44         m_modelView;      // +0x000
    VIMatrix44         m_projection;     // +0x040
    char               m_pad080[0x120];  // +0x080
    VIRasterDirLight   m_sun;            // +0x1A0
    // SetDirLight indexes 0x700001BC with a stride of 28 and SetPointLight
    // 0x700001C0 with a stride of 32, so the two light arrays OVERLAP -- a
    // frame uses one kind of local light or the other, never both.  Only the
    // point array can be a member here; the dir array is addressed by cast.
    char               m_pad1BC[4];      // +0x1BC  VIRasterDirLight dirs[]
    VIRasterPointLight m_points[1];      // +0x1C0
    VIColor32F         m_ambient;        // +0x1E0
    float              m_fogScale;       // +0x1F0
    float              m_fogOffset;      // +0x1F4
    float              m_fogNear;        // +0x1F8
    float              m_fogFar;         // +0x1FC
    VIColor32F         m_staticLighting; // +0x200
    int                m_p210;           // +0x210
    int                m_p214;           // +0x214
    int                m_p218;           // +0x218
    int                m_p21C;           // +0x21C
    unsigned char      m_ambientOn;      // +0x220
    unsigned char      m_sunOn;          // +0x221
    unsigned char      m_lightOn[6];     // +0x222
    float              m_p228;           // +0x228
    float              m_p22C;           // +0x22C
};

#define VIRASTER_SPR ((VIRasterSPR *)0x70000000)

// The tessellator, VIRaster's biggest member by far: 17,480 bytes at +0x40,
// sized by the gap between the constructor call it takes and the fog block
// that follows it.  Its own seven functions are NOT written in phase 56 -- the
// body is a block so the enclosing layout is right and the constructor call
// survives.
class VITessVertex;

class VIRasterTess
{
public:
    char m_data[17480];

    VIRasterTess();

    int Init();
    int InitTree();
    int Tessellate(int n, VITessVertex *v);
    int Tessellate(int n, int m, VITessVertex *v);
    int StoreResult(int n, VITessVertex *v);
};

// A material PALETTE: a reference count and an array of material handles.
// CreateMaterialPal hands __builtin_new sixteen bytes, constructs a
// VIArray<int> at +8 and Init()s it to the requested entry count.
class VIMaterialPal
{
public:
    int          m_p00;      // +0x00  set when the pal is in the dictionary
    int          m_refCount; // +0x04
    VIArray<int> m_mats;     // +0x08

    // A MEM-INITIALISER list, not a body: gcc runs the list in DECLARATION
    // order, so the two zero stores come out BEFORE the VIArray member's
    // constructor call, which is what CreateMaterialPal and CopyMaterialPal
    // both show.  Written as a body the member ctor would go first.
    VIMaterialPal() : m_refCount(0), m_p00(0) {}
};

class VIRaster
{
public:
    static int RInitOffset;
    static int RMaterialOffset;
    static int RStripOffset;
    static int RStrip2Offset;
    static int RStrip3Offset;
    static int RStrip4Offset;
    static int RStrip5Offset;
    static int RStrip6Offset;
    static int RFanOffset;
    // The VU1 address the skinning palette is unpacked to, and the double-
    // buffered GS register block Init ships and SetClearColor patches.
    static int  ObjectBlendsBuffer;
    static int  PreTransBuffer;
    static int  PrimBuffer1;
    static int  PrimBuffer2;
    static int  ColorMatrix;
    static long DBuffer[128];

    // Named for what the call sites do with them; none of these bodies is in
    // this TU, and the image is un-relocated, so only the ARGUMENT SHAPE is
    // being asserted here -- which register carries what, and how many.
    int DrawPrimBuffer(int prim, int pal, unsigned int flags);
    int DrawPrimBuffer(int prim, int lod, int pal, unsigned int flags);
    int PickPrimBuffer(VIPickSegment &seg, int prim, int pal);
    int PickPrimBuffer(VIPickSegment &seg, int prim, int lod, int pal);
    int AddRefMaterialPal(int pal);
    int AddRefPrimBuffer(int prim);
    int ReleasePrimBuffer(int prim, VIDictionary &d);
    int ReleaseMaterialPal(int pal, VIDictionary &d);
    int ReleaseColorBuffer(int buf, VIDictionary &d);
    int ReleaseMaterialPal(int pal);
    int ShareMaterialPal(int pal);
    int MaterialPalCount(int pal);
    VIMaterial *MaterialPalEntry(int pal, int i);
    VIMaterial *GetMaterialPalEntry(int pal, int i);
    int         CopyMaterialPal(int pal, int n);
    // Phase 53, off VICSprite::SetHelm: the pal by index and its entry count.
    int         MaterialPal(int pal);
    int         GetMaterialPalSize(int pal);
    const VIMatrix44 *GetModelView() const;
    int SetModelView(const VIMatrix44 &m);
    // Phase 54, off VIScene::PickActor: the raster culls the pick segment
    // against the actor's bounding sphere, and carries the actor's baked
    // vertex colour for the sprite that is about to be picked.
    int Cull(VIPickSegment &seg, VISphere &s);
    int SetStaticLighting(const VIColor32F &c);
    // Phase 52, off VIZone::SetPretranslations and VIZone::Pick: the count, the
    // zone's own pre-translation block, and the point they are relative to.
    int SetPreTranslations(int n, VIVect3 *v, const VIVect3 &at);
    // Phase 47, off VIHSprite::SetBlendMatrices: the skinning palette, handed
    // over as a count and the vector's own block.
    int SetObjectBlends(int n, VIMatrix44 *m);
    // VICamera::Init is the only caller of Frustum, and the matrix is the
    // OUTPUT -- a0 is the raster, a1 the matrix, six floats after it.
    void Frustum(VIMatrix44 &m, float l, float r, float b, float t,
                 float n, float f);
    int GetViewport(int &x, int &y, int &w, int &h) const;
    int SharePrimBuffer(int prim);
    VIPrimBuffer *PrimBuffer(int prim);
    int CreateMaterial(int layers);
    VIMaterial *Material(int i);
    int ReleaseMaterial(int i, VIDictionary &d);
    int ShareMaterial(int i);
    int CreateSurface(int i);
    int ShareSurface(int i);
    VISurface *Surface(int i);
    int ReleaseSurface(int i, VIDictionary &d);
    int GetPointSize(int font);
    int GetWidth(int font, unsigned short c);
    int GetStringWidth(int font, const char *s);
    int GetStringWidth(int font, const unsigned short *s);
    int DrawString(int font, const VIVect2I &at, const VIColor32F &c, const char *s);
    int DrawString(int font, const VIVect2I &at, const VIColor32F &c, const unsigned short *s);
    int DrawCenteredString(int font, const VIVect2I &at, const VIColor32F &c,
                           const char *s, unsigned int w);
    int DrawCenteredString(int font, const VIVect2I &at, const VIColor32F &c,
                           const unsigned short *s, unsigned int w);
    // Phase 32, from VINameSprite: a 3D anchor rather than a screen point, and
    // no width -- the raster projects the point itself.
    int DrawCenteredString(int font, const VIVect3 &at, const VIColor32F &c,
                           const char *s);
    int CalcCenteredStringScreenRect(int font, const VIVect3 &at, const char *s,
                                     VIRect &r);
    int ShareFont(int font);
    int ReleaseFont(int font, VIDictionary &d);
    // Phase 33, from VITrailFx::Render and VILightning::Render: the immediate
    // triangle-strip path.  Only the argument shape is being asserted -- none
    // of these bodies is in this TU.
    int SetMaterial(int i);
    int BeginTriStrip();
    int EndTriStrip();
    int Color(const VIColor32F &c);
    int Vertex(const VIVect3 &v);
    int UV(const VIVect2 &uv);
    // Phase 42, from VIUI: the 2D path.  Only the argument shape is asserted.
    int Begin2D();
    int End2D();
    int BeginTriFan();
    int EndTriFan();
    int Vertex(const VIVect2I &v);
    int SetSolidFillUIMaterial();
    // Phase 55, from VIAtmosphere: the billboard path.  BeginBillboards hands
    // back a run of VIBillboard through the pointer it is given and returns
    // how many of the requested count it granted.
    int BeginBillboards(VIBillboard **bb, int n);
    int EndBillboards();
    int SetFog(int on, float near, float far, const VIColor32F &c);
    int Viewport(int x, int y, int w, int h);
    int Blit(int id, unsigned int flags, const VIVect2I &at,
             VIAlphaBlendMode m, float alpha);
    int Blit(int id, unsigned int flags, const VIVect2I &at, const VIVect2I &a,
             const VIVect2I &b, VIAlphaBlendMode m, float alpha);
    int StretchBlit(int id, unsigned int flags, const VIVect2I &at,
                    const VIVect2I &a, const VIVect2I &b, const VIVect2I &c,
                    VIAlphaBlendMode m, float alpha);
    // ------------------------------------------------------------- layout --
    int           m_p00;            // +0x0000
    int           m_p04;            // +0x0004
    int           m_p08;            // +0x0008
    // The immediate-mode vertex attributes: UV(), Normal() and Color() write
    // these and Vertex() flushes all four rows into the DMA chain.
    VIVect2       m_uv;             // +0x000C
    VIVect3       m_normal;         // +0x0014
    VIColor32F    m_color;          // +0x0020
    int           m_numVerts;       // +0x0030
    VIVect4      *m_vtx;            // +0x0034  the write cursor
    VIVect4      *m_vtxBase;        // +0x0038  the GIF tag this batch opened
    int           m_tessellate;     // +0x003C
    VIRasterTess  m_tess;           // +0x0040
    float         m_zBias;          // +0x4488
    int           m_fogEnabled;     // +0x448C
    float         m_fogNear;        // +0x4490
    float         m_fogFar;         // +0x4494
    VIColor32F    m_fogColor;       // +0x4498
    char          m_pad44A8[8];     // +0x44A8  VIMatrix44 is aligned(16)
    VIMatrix44    m_m44B0;          // +0x44B0
    VIMatrix44    m_m44F0;          // +0x44F0
    unsigned char m_b4530;          // +0x4530
    unsigned char m_b4531;          // +0x4531
    char          m_pad4532[2];
    int           m_viewportX;      // +0x4534
    int           m_viewportY;      // +0x4538
    int           m_viewportW;      // +0x453C
    int           m_viewportH;      // +0x4540
    float         m_viewportWF;     // +0x4544
    float         m_viewportHF;     // +0x4548
    float         m_invViewportW;   // +0x454C
    float         m_invViewportH;   // +0x4550
    int           m_screenW;        // +0x4554
    int           m_screenH;        // +0x4558
    float         m_screenWF;       // +0x455C
    float         m_screenHF;       // +0x4560
    float         m_invScreenW;     // +0x4564
    float         m_invScreenH;     // +0x4568
    float         m_p456C;          // +0x456C  16.0f
    float         m_p4570;          // +0x4570  8388608.0f
    float         m_aspectX;        // +0x4574
    float         m_aspectY;        // +0x4578
    VIColor32F    m_clearColor;     // +0x457C
    int           m_override;       // +0x458C
    int           m_wireframe;      // +0x4590
    int           m_solidFill;      // +0x4594
    int           m_multiTexture;   // +0x4598
    int           m_p459C;          // +0x459C
    int           m_defaultMaterial;        // +0x45A0
    int           m_solidFillMaterial;      // +0x45A4
    int           m_solidFillUIMaterial;    // +0x45A8
    int           m_solidFillUIAVGMaterial; // +0x45AC
    int           m_wireframeMaterial;      // +0x45B0
    int           m_portalMaterial;         // +0x45B4
    int           m_curMaterial;    // +0x45B8
    int           m_blitSurface;    // +0x45BC
    int           m_blitBlend;      // +0x45C0
    int           m_numBillboards;  // +0x45C4
    int           m_p45C8;          // +0x45C8
    int           m_p45CC;          // +0x45CC
    long          m_time;           // +0x45D0
    int           m_frame;          // +0x45D8
    int           m_primType;       // +0x45DC
    int           m_dirtyLights;    // +0x45E0
    int           m_dirtyMatrices;  // +0x45E4
    int           m_dirtyPreTrans;  // +0x45E8
    char          m_pad45EC[0x49EC - 0x45EC];
    VIVect3       m_preTransAt;     // +0x49EC
    VIVect3       m_preTrans[32];   // +0x49F8

    VIPool<VIPrimBuffer *>      m_primBuffers;   // +0x4B78
    VIList<VIPrimBuffer *>      m_primBufferFree;// +0x4B90
    VIPool<VIColorBuffer *>     m_colorBuffers;  // +0x4B9C
    VIPool<VISurface *>         m_surfaces;      // +0x4BB4
    VIList<VISurface *>         m_surfaceFree;   // +0x4BCC
    VIList<VIMaterial>          m_materials;     // +0x4BD8
    VIPool<VIMaterialPal *>     m_materialPals;  // +0x4BE4
    VIPool<VIRasterMaterialLRU> m_materialLRU;   // +0x4BFC

    char         *m_p4C14;          // +0x4C14
    int           m_p4C18;          // +0x4C18
    _sceDmaTag   *m_dmaBuffer0;     // +0x4C1C
    _sceDmaTag   *m_dmaBuffer1;     // +0x4C20
    _sceDmaTag   *m_dmaTag;         // +0x4C24  the DMA write cursor
    _sceDmaTag   *m_dmaStart;       // +0x4C28
    int           m_dmaFlushes;     // +0x4C2C
    int           m_sceneState;     // +0x4C30

    VIPool<VIRFont *> m_fonts;      // +0x4C34

    int           m_p4C4C;          // +0x4C4C
    unsigned char m_firstFrame;     // +0x4C50

    // ------------------------------------------------------------ phase 56 --
    VIRaster();
    ~VIRaster();

    int Init(void *p);
    int Init(void *p, int w, int h, int i);
    int Init();
    int Close();
    int Reset();
    int Clear();
    int SetScreenResolution(int w, int h);
    int SetProjection(const VIMatrix44 &m);
    const VIMatrix44 *GetProjection() const;
    int GetWidth(int font, char c);
    int DrawPrimBuffer(int prim, int pal, float f, unsigned int flags);
    int SetClearColor(const VIColor32F &c);
    const VIColor32F *GetClearColor() const;

    int BeginScene();
    int EndScene();
    int Flip();
    void Orthographic(VIMatrix44 &m, float l, float r, float b, float t,
                      float n, float f);

    int BeginLines();
    int EndLines();
    int Vertex(const VIVect2 &v);
    int Normal(const VIVect3 &n);
    int DrawBBox(const VIBBox &b, const VIColor32F &c);

    int CreatePrimBuffer(int share);
    int CreateColorBuffer(int share);
    VIColorBuffer *ColorBuffer(int i);
    int ShareColorBuffer(int i);
    int CopyMaterial(int m, int share);
    int AddMaterial(int m, _sceDmaTag **tag);
    int MaterialHasDepthSort(int m);
    int SetSolidFillMaterial();
    int SetSolidFillUIAVGMaterial();
    int SetWireframeMaterial();
    int SetPortalMaterial();
    int SetWireframe(int on);
    int SetSolidFill(int on);
    int SetOverrideMaterial();

    int CreateMaterialPal(int n, int share);
    int MaterialPalHasDepthSort(int pal);

    int CreateFont(int share);
    VIRFont *Font(int i);

    int SetMultiTexture(int on);
    int SetTime(long t);
    long GetTime();
    int SetDefaultLights(const VIMatrix44 &m);
    int SetAmbient(const VIColor32F &c);
    int EnableAmbient(int on);
    int SetSunLight(const VIMatrix44 &m, const VIRasterDirLight &l);
    int EnableSunLight(int on);
    int SetDirLight(int i, const VIMatrix44 &m, const VIRasterDirLight &l);
    int EnableDirLight(int i, int on);
    int EnableAllDirLights(int on);
    int MaxDirLights();
    int SetPointLight(int i, const VIMatrix44 &m, const VIRasterPointLight &l);
    int EnablePointLight(int i, int on);
    int EnableAllPointLights(int on);
    int MaxPointLights();
    int EnableAllLights(int on);
    const VIColor32F *GetStaticLighting() const;
    void EnableTessellation(int on);
    int NumTextureSwaps();

    int Flush();
    int FlushDMABufferIfFull();
    int SetFogRegisters();
    int CalcMatrices();
    int CalcClipToScreen();
    void SetZBias(int z);
    int SetGSDithering(int on);
    int Store2D();
    int Restore2D();
    int InitMaterialLRU(int n);
    int ClearMaterialLRU();
    int InitDMADoubleBuffer();
    int ClearDMADoubleBuffer();
    int InitSPRDoubleBuffer();
    int SwapSPRBuffer();
    int SPRBuffer();
    void SetClipAndLight(int on);
    int DebugEEMicro();
    int DrawPrimBufferVUNC(int prim, int pal, unsigned int flags);
    int DrawPrimBufferPackVUNC(int prim, int pal, unsigned int flags);
    int DrawPrimBufferPackVUNCG(int prim, int pal, unsigned int flags);
    int DrawPrimBufferPackVUNBW(int prim, int pal, unsigned int flags);
    int DrawPrimBufferPackVU(int prim, int pal, float f, unsigned int flags);
    int DrawPrimBufferPackVUNC(int prim, int lod, int pal, unsigned int flags);
    int DebugResources();
    int ReleaseDeferred();
    long GetZBufferReg() const;
    int InitDMACHandler();
    int RemoveDMACHandler();
    int UploadRasterMicro(_sceDmaTag **tag);
    int UploadPerMaterial(_sceDmaTag **tag);
    int UploadMatrices(_sceDmaTag **tag);
    int UploadPreTrans(_sceDmaTag **tag);
    int BlitBegin(_sceDmaTag **tag);
    int BlitEnd(_sceDmaTag **tag);
    int UploadBillboardMicro(_sceDmaTag **tag);
    int InitGSRegisters(_sceDmaTag **tag);
    int SetGSDitherMatrix(int on, _sceDmaTag **tag);
};
