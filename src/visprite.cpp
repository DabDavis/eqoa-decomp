// VISprite / VISimpleSprite -- SUPPORT 0x00471D60 .. 0x0047ED80.
//
// The base of the scene's renderable leaf hierarchy, and the first class in
// the corpus with a REAL vtable to prove: ten virtuals, of which nine are
// two-instruction stubs the derived classes exist to replace.
//
// The slot ORDER here is not a guess.  It is fixed twice over: by the vtable's
// own size in the image (96 bytes, and a gcc 2.x vtable ends with a NULL entry,
// so 96/8 - 2 = ten) and by the console -- go-eqoa-vi/vicoll dispatches
// Collide(sphere) through sprite+0x3C entry 4 and Collide(ray) through entry 5,
// which is where this declaration order puts them.
#include "vi.h"

extern "C" void sceVu0CopyMatrix(VIMatrix44 &d, const VIMatrix44 &s);

VISprite::VISprite()
{
    m_offDist = 500.0f;
    m_type = 0;
    m_actor = -1;
}

VISprite::~VISprite()
{
}

int VISprite::GetInstType(VIScene &sc) const
{
    return 0;
}

int VISprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    return 0;
}

int VISprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VISprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VISprite::Pick(VIPickSegment &seg, VISceneFilter &f, VIScene &sc, VIRaster &r)
{
    return 0;
}

// The one function in the class with a body, and its LOCALS are what read it.
// Phase 61: the matrix IS e.m_matrix, filled by a sceVu0CopyMatrix call (the
// jal at 0x0047D35C), and the walk drops the map's Begin(), not its Last() --
// both read off eqcalls.py.  p at sp+0, e at sp+16 with its matrix at sp+32,
// the key at sp+96; the `_$_10VIMatrix44` at the exit is e's member dtor.
// `t` is NOT address-taken and `k` IS (Insert takes `const float &`), so `t`
// lives in $f20 across the second sqrtf and `k` is stored once, straight into
// the slot Insert is handed.
// Phase 61.  VIMatrix44's assignment is an INLINE sceVu0CopyMatrix: no
// out-of-line `__as__10VIMatrix44RC10VIMatrix44` exists in SUPPORT, and
// VICamera::Update's target calls sceVu0CopyMatrix where our build calls the
// (undefined) operator.  Called directly, the copy's `&e.m_matrix` is merged
// with the constructor's and the destructor's into one callee-saved register;
// through the inline, every use rematerialises `addiu a0,sp,32` (36 words).
// Defined here and not in vi.h because viraster.cpp declares sceVu0CopyMatrix
// with a pointer first argument, and a second C-linkage declaration would not
// compile there.
inline VIMatrix44 &VIMatrix44::operator=(const VIMatrix44 &a)
{
    sceVu0CopyMatrix(*this, a);
    return *this;
}

int VISprite::QueryEmitters(VIScene &sc, const VIMatrix44 &m, VIEmitterQuery &q)
{
    VIVect3 p;
    VIEmitterEntry e;

    if (q.m_type == m_type)
    {
        p.x = m.m[3].x;
        p.y = m.m[3].y;
        p.z = m.m[3].z;
        float s = sqrtf(m.m[0].x * m.m[0].x + m.m[0].y * m.m[0].y + m.m[0].z * m.m[0].z);
        float t = m_bsphere.r * s;
        float d = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
        float k = t * (1.0f / d);
        e.m_a = q.m_a;
        e.m_b = q.m_b;
        e.m_matrix = m;
        q.m_map.Insert(k, e);
        if (q.m_limit < q.m_map.Size())
            q.m_map.Erase(q.m_map.Begin());
    }
    return 0;
}

int VISprite::QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                const VIMatrix44 &m, VIScene &sc)
{
    return 0;
}

int VISprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    return -1;
}

int VISprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                      VISoundDevice *sd, VICollide &c)
{
    return -1;
}

int VISprite::GetType()
{
    return m_type;
}

VIBBox *VISprite::GetBBox()
{
    return &m_bbox;
}

VISphere *VISprite::GetBSphere()
{
    return &m_bsphere;
}

float VISprite::GetOffDist()
{
    return m_offDist;
}
