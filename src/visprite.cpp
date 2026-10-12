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
// Three of them, and the declaration order is FIXED by the stack offsets: p at
// sp+0, e at sp+16, mat at sp+32 (VIMatrix44 is aligned(16)), the key at sp+96.
// The 64-byte block at sp+32 is zeroed TWICE -- gcc's array-constructor loop
// over VIVect4 m[4], then Zero()'s four VU0 stores -- so the second is a
// statement and the matrix is a plain local, not a member of the entry: making
// it e.m_matrix scores 42/113 against 32/112 here.
//
// Two shapes are load-bearing.  `t` is NOT address-taken and `k` IS -- Insert
// takes `const float &` -- so `t` lives in $f20 across the second sqrtf while
// `k` is stored once, straight into the slot Insert will be handed; writing it
// as one variable assigned twice spills the intermediate as well.  And `t` is
// computed BEFORE `d`, which is what puts m_bsphere.r's load inside the second
// dot product and the multiply in the NaN retest's delay slot.
//
// OPEN, and it is the standing phenomenon with its pass NAMED: the target
// rematerialises `addiu a0,sp,32` at each of the four uses, where gcse merges
// ours into one pseudo that then needs a callee-saved register (s3) and a
// 208-byte frame against 192.  `-fno-gcse` takes it to 18/110 -- and costs the
// tree 16 functions, so it is not the build flag.  Declaring mat FIRST puts it
// at sp+0, where the address is a bare `move a0,sp` with nothing to merge, and
// the register allocation then matches the target instruction for instruction
// (109 words, the target's own length) -- with every local at the wrong offset.
int VISprite::QueryEmitters(VIScene &sc, const VIMatrix44 &m, VIEmitterQuery &q)
{
    VIVect3 p;
    VIEmitterEntry e;
    VIMatrix44 mat;

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
        mat.Invert(m);
        q.m_map.Insert(k, e);
        if (q.m_limit < q.m_map.Size())
            q.m_map.Erase(q.m_map.Last());
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
