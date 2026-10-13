// VICollide -- SUPPORT 0x00489250 .. 0x0048DE08.
//
// Phase 22.  The class whose SEMANTICS were settled from the console first:
// go-eqoa-vi/vicoll ports the transform pair, the buffer walk on all three
// packings, the query path and both CollideObj entry points, all bit-exact
// against a live SUPPORT.  So, as in Phases 11/13/18, every mismatch here is
// codegen and nothing else.
//
// The refcount a buffer is shared and released on is VIBuffer::m_userData
// (+0x04), not a field of its own -- CreateCollBuffer bumps `4(b)` and
// ReleaseCollBuffer decrements it.  sizeof(VICollBuffer) = 92 is the ctor's
// own `li a0,92` into __builtin_new, and it closes the declared layout exactly.
#include "vi.h"

extern "C" void  sceVu0CopyMatrix(VIMatrix44 &d, const VIMatrix44 &s);
extern "C" void *memcpy(void *, const void *, unsigned int);
extern "C" int   printf(const char *, ...);

VICollide::VICollide()
{
}

// The three member destructors (`li a1,2`, in reverse declaration order) are
// gcc's; Close is the body.
VICollide::~VICollide()
{
    Close();
}

int VICollide::Init()
{
    m_world.Identity();
    m_invWorld.Identity();
    return 0;
}

int VICollide::Close()
{
    if (Clear() < 0)
        return -1;
    return 0;
}

int VICollide::Reset()
{
    if (Clear() < 0)
        return -1;
    if (Init() < 0)
        return -1;
    return 0;
}

int VICollide::Clear()
{
    for (int i = m_buffers.Begin(); i != m_buffers.End(); i = m_buffers.Begin())
        if (ReleaseCollBuffer(i) < 0)
            return -1;

    if (m_buffers.Clear() < 0)
        return -1;
    return 0;
}

int VICollide::CreateCollBuffer(int share)
{
    VICollBuffer *b = new VICollBuffer;
    if (b == 0)
        return -1;

    int i = m_buffers.Add(b);
    if (i < 0)
        return -1;

    if (share)
        b->m_userData++;

    return i;
}

VICollBuffer *VICollide::CollBuffer(int i)
{
    VIPool<VICollBuffer *>::Node *n = &m_buffers.m_nodes[i];
    return n->m_data;
}

int VICollide::ShareCollBuffer(int i)
{
    VIPool<VICollBuffer *>::Node *n = &m_buffers.m_nodes[i];
    VICollBuffer *b = n->m_data;
    b->m_userData++;
    return 0;
}

int VICollide::ReleaseCollBuffer(int i)
{
    VIPool<VICollBuffer *> *p = &m_buffers;
    VIPool<VICollBuffer *>::Node *n = &p->m_nodes[i];
    VICollBuffer *b = n->m_data;

    b->m_userData--;
    if (b->m_userData > 0)
        return 0;

    if (p->Erase(i) < 0)
        return -1;

    delete b;
    return 0;
}

int VICollide::SetWorld(const VIMatrix44 &m)
{
    sceVu0CopyMatrix(m_world, m);
    m_invWorld.InvertAffine(m_world);
    return 0;
}

const VIMatrix44 *VICollide::GetWorld() const
{
    return &m_world;
}

int VICollide::SetPreTranslations(int n, VIVect3 *v)
{
    memcpy(m_preTranslations, v, n * sizeof(VIVect3));
    return 0;
}

// The three dispatchers.  A switch, not an if/else chain: gcc builds the
// decision tree in VALUE order (==1, then <2, then ==3) while emitting the
// bodies in SOURCE order, which is Phase 16's lever and the only shape that
// puts the V case first in memory with the ==1 test first in the tree.
int VICollide::Collide(VICollSphere &s, VICollBuffer &b)
{
    switch (b.m_type)
    {
    case VICollBufferTypeV:
        return CollideBufferV(s, b);
    case VICollBufferTypePackV:
        return CollideBufferPackV(s, b);
    case VICollBufferTypePackVGF:
        return CollideBufferPackVGF(s, b);
    }
    return -1;
}

int VICollide::Collide(VICollRay &r, VICollBuffer &b)
{
    switch (b.m_type)
    {
    case VICollBufferTypeV:
        return CollideBufferV(r, b);
    case VICollBufferTypePackV:
        return CollideBufferPackV(r, b);
    case VICollBufferTypePackVGF:
        return CollideBufferPackVGF(r, b);
    }
    return -1;
}

int VICollide::Query(const VIBBox &w, const VIBBox &o, VIQueryTriList &l,
                     VICollBuffer &b)
{
    switch (b.m_type)
    {
    case VICollBufferTypeV:
        return QueryBufferV(w, o, l, b);
    case VICollBufferTypePackV:
        return QueryBufferPackV(w, o, l, b);
    case VICollBufferTypePackVGF:
        return QueryBufferPackVGF(w, o, l, b);
    }
    return -1;
}

int VICollide::QueryObj(const VIBBox &b, VIQueryTriList &l, int obj)
{
    VIPool<VICollBuffer *>::Node *n = &m_buffers.m_nodes[obj];
    VICollBuffer *buf = n->m_data;
    VIBBox local;

    b.Transform(m_invWorld, local);
    return Query(b, local, l, *buf);
}

int VICollide::CollideObj(VICollSphere &s, int obj)
{
    VIPool<VICollBuffer *>::Node *n = &m_buffers.m_nodes[obj];
    VICollBuffer *buf = n->m_data;
    VICollSphere local;
    VIVect4 nrm;
    VIVect4 wn;

    if (local.Init(s, m_invWorld) < 0)
        return -1;

    if (Collide(local, *buf) != 0)
    {
        s.m_hitTime = local.m_hitTime;
        s.m_hitIdx = local.m_hitIdx;
        nrm.x = local.m_hitNormal.x;
        nrm.y = local.m_hitNormal.y;
        nrm.z = local.m_hitNormal.z;
        nrm.w = 0.0f;

        wn.Set(nrm * m_world);
        wn.Normalize();

        s.m_hitNormal.x = wn.x;
        s.m_hitNormal.y = wn.y;
        s.m_hitNormal.z = wn.z;
        return 1;
    }
    return 0;
}

int VICollide::CollideObj(VICollRay &r, int obj)
{
    VIPool<VICollBuffer *>::Node *n = &m_buffers.m_nodes[obj];
    VICollBuffer *buf = n->m_data;
    VICollRay local;
    VIVect4 nrm;
    VIVect4 wn;

    if (local.Init(r, m_invWorld) < 0)
        return -1;

    if (Collide(local, *buf) != 0)
    {
        r.m_hitTime = local.m_hitTime;
        r.m_hitIdx = local.m_hitIdx;
        nrm.x = local.m_hitNormal.x;
        nrm.y = local.m_hitNormal.y;
        nrm.z = local.m_hitNormal.z;
        nrm.w = 0.0f;

        wn.Set(nrm * m_world);
        wn.Normalize();

        r.m_hitNormal.x = wn.x;
        r.m_hitNormal.y = wn.y;
        r.m_hitNormal.z = wn.z;
        return 1;
    }
    return 0;
}

// The Cull family.  All four are separating-axis rejections and none of them
// reads `this` -- a0 is untouched in every one, which is what let
// go-eqoa-vi/vicoll drive them from the console with a dummy collider.
//
// The three MAX tests come first and the three MIN tests after, not
// interleaved per axis.
//
// OPEN, 30/68.  The shape is settled and it is not the source: `&&`, the
// nested `if`, an accumulator, and pointer variables for either vector pair
// all give the same 30 words, and the function has NO STACK FRAME so there are
// no locals to spell differently.  What differs is the standing phenomenon --
// the target keeps m_start.x/y/z in $f4/$f3/$f2 for the whole function and
// puts the shared `return 1` LAST, where ours rematerialises the loads and
// leaves the first `return 1` in line.
int VICollide::Cull(VICollRay &r, VIBBox &b)
{
    if ((r.m_start.x > b.max.x && r.m_end.x > b.max.x) ||
        (r.m_start.y > b.max.y && r.m_end.y > b.max.y) ||
        (r.m_start.z > b.max.z && r.m_end.z > b.max.z) ||
        (r.m_start.x < b.min.x && r.m_end.x < b.min.x) ||
        (r.m_start.y < b.min.y && r.m_end.y < b.min.y) ||
        (r.m_start.z < b.min.z && r.m_end.z < b.min.z))
        return 1;
    return 0;
}

// Cull(ray, VISphere) and Cull(sphere, VISphere) are the same quadratic with
// the radius sourced differently, and neither calls the other: the body is
// REPEATED, exactly as VICollSphere::Init repeats VICollRay::Init.  The default
// is to cull -- the one early-out that answers 0 is starting INSIDE the sphere,
// where there is nothing to reject.
// Phase 61 closed both: the callee-saved $s1 holding 1 across the sqrtf is a
// RESULT variable (`int cull = 1;`, cleared on the two accepting paths), not
// a constant, and `l` is the copy-then-subtract pair with the summed radius
// (where there is one) computed first.
int VICollide::Cull(VICollRay &r, VISphere &v)
{
    VIVect3 l;

    l = r.m_start;
    l -= v.c;

    // Phase 61: the callee-saved 1 is a RESULT, a phi of 1 and 0 -- the
    // rejections fall through to `return cull` rather than `return 1`.
    int cull = 1;
    float c = l.x * l.x + l.y * l.y + l.z * l.z - v.r * v.r;
    if (0.0f < c)
    {
        VIVect3 *d = &r.m_dir;
        float b = l.x * d->x + l.y * d->y + l.z * d->z;
        if (b < 0.0f)
        {
            float e = b * b - c;
            if (0.0f < e)
            {
                float t = -(b + sqrtf(e)) * r.m_invLength;
                if (t < r.m_hitTime)
                    cull = 0;
            }
        }
    }
    else
        cull = 0;
    return cull;
}

// The swept form: the same body with the two radii summed before squaring.
int VICollide::Cull(VICollSphere &s, VISphere &v)
{
    VIVect3 l;

    float r = s.m_radius + v.r;
    l = s.m_start;
    l -= v.c;

    int cull = 1;
    float c = l.x * l.x + l.y * l.y + l.z * l.z - r * r;
    if (0.0f < c)
    {
        VIVect3 *d = &s.m_dir;
        float b = l.x * d->x + l.y * d->y + l.z * d->z;
        if (b < 0.0f)
        {
            float e = b * b - c;
            if (0.0f < e)
            {
                float t = -(b + sqrtf(e)) * s.m_invLength;
                if (t < s.m_hitTime)
                    cull = 0;
            }
        }
    }
    else
        cull = 0;
    return cull;
}

// The three format strings come out as three `lui`s hoisted to the top in
// SOURCE order (Phase 21), and 92 is sizeof(VICollBuffer) -- the same constant
// CreateCollBuffer hands __builtin_new.  The byte total needs Phase 14's fresh
// intermediate in integer code (`t = bytes + 92; bytes = t + size;` -- one
// statement reassociates to `bytes + (92 + size)`), and the two accumulators
// come out in the REVERSE of source order, so bytes is written first.
//
// The node table is read as `8(a0)` off the pool's own `this + 528`: the
// element comes through the inline Value(i) (phase 27's lever), not a
// written-out m_nodes[i] -- phase 61, 4/61 to byte-exact.
int VICollide::DebugResources()
{
    int bytes = 0;
    int prims = 0;

    for (int i = m_buffers.Begin(); i != m_buffers.End(); i = m_buffers.Next(i))
    {
        VICollBuffer *b = m_buffers.Value(i);
        int t = bytes + sizeof(VICollBuffer);
        bytes = t + b->m_dataSize;
        prims += b->m_numPrims;
    }

    printf("VICollide: %d coll buffers\n", m_buffers.Size());
    printf("VICollide: %d prims\n", prims);
    printf("VICollide: %d bytes\n", bytes);
    return bytes;
}

// ---------------------------------------------------------------------------
// Phase 46 -- the collision core: the twenty functions Phase 22 left, which
// are the collision itself rather than the class that owns it.
//
// As in Phase 22 the SEMANTICS were settled from the console first --
// go-eqoa-vi/vicoll is a bit-exact port of every one of these, checked against
// a live SUPPORT -- so every mismatch below is codegen and nothing else, and
// the comments record which lever moved which function.
//
// 8 of the 20 are byte-exact, and Phase 22's Cull(VICollRay &, VIBBox &) closed
// on the way past, so nine functions in this file went exact.  What is still open splits in two: the three PackVGF walkers
// and the two V walkers differ only in REGISTER ALLOCATION (the target spills
// a different pseudo, which renames every callee-saved register after it), and
// the point/sphere family diverges in the entry block's SCHEDULE -- the target
// hoists all six loads before the first subtract where ours interleaves them,
// which is the shape `decomp-scheduler-rig` attributes to the R5900 `load`
// ready-delay.
// ---------------------------------------------------------------------------

int VICollide::PointInTriangle(VIVect3 &p, VIPlane &pl, VIVect3 &v0,
                               VIVect3 &v1, VIVect3 &v2)
{
    VIVect3 e;
    VIVect3 d;
    VIVect3 c;

    e.x = v1.x - v0.x;
    e.y = v1.y - v0.y;
    e.z = v1.z - v0.z;
    d.x = p.x - v1.x;
    d.y = p.y - v1.y;
    d.z = p.z - v1.z;
    c.x = e.y * d.z - e.z * d.y;
    c.y = -e.x * d.z + e.z * d.x;
    c.z = e.x * d.y - e.y * d.x;
    if (0.0f < c.x * pl.n.x + c.y * pl.n.y + c.z * pl.n.z)
        return 0;

    e.x = v2.x - v1.x;
    e.y = v2.y - v1.y;
    e.z = v2.z - v1.z;
    d.x = p.x - v2.x;
    d.y = p.y - v2.y;
    d.z = p.z - v2.z;
    c.x = e.y * d.z - e.z * d.y;
    c.y = -e.x * d.z + e.z * d.x;
    c.z = e.x * d.y - e.y * d.x;
    if (0.0f < c.x * pl.n.x + c.y * pl.n.y + c.z * pl.n.z)
        return 0;

    e.x = v0.x - v2.x;
    e.y = v0.y - v2.y;
    e.z = v0.z - v2.z;
    d.x = p.x - v0.x;
    d.y = p.y - v0.y;
    d.z = p.z - v0.z;
    c.x = e.y * d.z - e.z * d.y;
    c.y = -e.x * d.z + e.z * d.x;
    c.z = e.x * d.y - e.y * d.x;
    if (0.0f < c.x * pl.n.x + c.y * pl.n.y + c.z * pl.n.z)
        return 0;

    return 1;
}

int VICollide::Collide(VICollRay &r, int idx, VIVect3 &v0, VIVect3 &v1,
                       VIVect3 &v2)
{
    int      hit = 0;
    VIPlane  pl;
    VIVect3  hp;

    if (pl(v0, v1, v2))
    {
        VIVect3 *dir = &r.m_dir;
        if (pl.n.x * dir->x + pl.n.y * dir->y + pl.n.z * dir->z <= 0.0f)
        {
            float ds = pl.Distance(r.m_start);
            if (0.0f < ds)
            {
                float de = pl.Distance(r.m_end);
                if (de < 0.0f)
                {
                    float t = ds / (ds - de);
                    if (t < r.m_hitTime)
                    {
                        hp = r.m_delta;
                        hp.x = hp.x * t + r.m_start.x;
                        hp.y = hp.y * t + r.m_start.y;
                        hp.z = hp.z * t + r.m_start.z;
                        if (PointInTriangle(hp, pl, v0, v1, v2))
                        {
                            hit = 1;
                            r.m_hitTime = t;
                            r.m_hitNormal = pl.n;
                            r.m_hitIdx = idx;
                        }
                    }
                }
            }
        }
    }
    return hit;
}

int VICollide::Cull(VICollSphere &s, VIBBox &b)
{
    VIVect3 mx;
    VIVect3 mn;

    mx = b.max;
    mx += s.m_radius;
    mn = b.min;
    mn -= s.m_radius;

    if ((s.m_start.x > mx.x && s.m_end.x > mx.x) ||
        (s.m_start.y > mx.y && s.m_end.y > mx.y) ||
        (s.m_start.z > mx.z && s.m_end.z > mx.z) ||
        (s.m_start.x < mn.x && s.m_end.x < mn.x) ||
        (s.m_start.y < mn.y && s.m_end.y < mn.y) ||
        (s.m_start.z < mn.z && s.m_end.z < mn.z))
        return 1;
    return 0;
}

// The point/sphere quadratic, written out THREE times -- here against a bare
// vertex, and again in Collide(VICollSphere &, VISphere &) and
// Collide(VICollRay &, const VISphere &) with the radius sourced differently.
// Nothing calls anything else; the body is repeated, exactly as the two
// Cull(..., VISphere &) overloads repeat each other.
// OPEN, 32/153.  The shape and the registers are settled -- what differs is
// the ENTRY BLOCK's schedule: the target issues all six loads before the first
// subtract and stores `l` before the direction loads, where ours interleaves
// them, and the accumulator's `hit = 0` lands in the first branch's delay slot
// there and in the prologue here.  Its two siblings below have the same 20-odd
// words in the same place, and Collide(VICollSphere &, VISphere &) shows why it
// is not the source: hoisting one statement out of the `if` moved it 37 -> 23
// and changed nothing else.
int VICollide::Collide(VICollSphere &s, VIVect3 &p)
{
    VIVect3 l;

    l.x = s.m_start.x - p.x;
    l.y = s.m_start.y - p.y;
    l.z = s.m_start.z - p.z;

    VIVect3 *dir = &s.m_dir;
    float b = l.x * dir->x + l.y * dir->y + l.z * dir->z;
    int hit = 0;
    if (b < 0.0f)
    {
        float c = l.x * l.x + l.y * l.y + l.z * l.z - s.m_radius * s.m_radius;
        float e = b * b - c;
        if (0.0f < e)
        {
            float tn = -(b + sqrtf(e));
            float t = tn * s.m_invLength;
            if (t < s.m_hitTime)
            {
                if (0.0f < t)
                {
                    s.m_hitTime = t;
                    s.m_hitNormal = *dir;
                    s.m_hitNormal *= tn;
                    s.m_hitNormal += s.m_start;
                    s.m_hitNormal -= p;
                    s.m_hitNormal *= s.m_invRadius;
                }
                else
                {
                    s.m_hitTime = 0.0f;
                    s.m_hitNormal = s.m_start;
                    s.m_hitNormal -= p;
                    s.m_hitNormal.Normalize();
                }
                hit = 1;
            }
        }
    }
    return hit;
}

// The same body against a static VISphere: the two radii are summed BEFORE
// squaring -- the Minkowski trick -- and the reciprocal that scales the contact
// normal is computed inline rather than read from m_invRadius.
// OPEN, 20/159.  `rad` is computed BEFORE the guard -- the delay slot of the
// first `bc1f` carries it, so it is unconditional (Phase 34's lever) -- and
// that is worth 14 words.  The rest is the entry block again.
int VICollide::Collide(VICollSphere &s, VISphere &v)
{
    int hit = 0;
    VIVect3 l;

    l = s.m_start;
    l -= v.c;

    VIVect3 *dir = &s.m_dir;
    float b = l.x * dir->x + l.y * dir->y + l.z * dir->z;
    float rad = s.m_radius + v.r;
    if (b < 0.0f)
    {
        float c = l.x * l.x + l.y * l.y + l.z * l.z - rad * rad;
        float e = b * b - c;
        if (0.0f < e)
        {
            float tn = -(b + sqrtf(e));
            float t = tn * s.m_invLength;
            if (t < s.m_hitTime)
            {
                if (0.0f < t)
                {
                    s.m_hitTime = t;
                    s.m_hitNormal = *dir;
                    s.m_hitNormal *= tn;
                    s.m_hitNormal += s.m_start;
                    s.m_hitNormal -= v.c;
                    s.m_hitNormal *= 1.0f / rad;
                }
                else
                {
                    s.m_hitTime = 0.0f;
                    s.m_hitNormal = s.m_start;
                    s.m_hitNormal -= v.c;
                    s.m_hitNormal.Normalize();
                }
                hit = 1;
            }
        }
    }
    return hit;
}

// And once more for a ray against a static sphere.
// OPEN, 33/156.  Same entry block as CollideVertex; there is no `rad` here to
// hoist.
int VICollide::Collide(VICollRay &r, const VISphere &v)
{
    int hit = 0;
    VIVect3 l;

    l = r.m_start;
    l -= v.c;

    VIVect3 *dir = &r.m_dir;
    float b = l.x * dir->x + l.y * dir->y + l.z * dir->z;
    if (b < 0.0f)
    {
        float c = l.x * l.x + l.y * l.y + l.z * l.z - v.r * v.r;
        float e = b * b - c;
        if (0.0f < e)
        {
            float tn = -(b + sqrtf(e));
            float t = tn * r.m_invLength;
            if (t < r.m_hitTime)
            {
                if (0.0f < t)
                {
                    r.m_hitTime = t;
                    r.m_hitNormal = *dir;
                    r.m_hitNormal *= tn;
                    r.m_hitNormal += r.m_start;
                    r.m_hitNormal -= v.c;
                    r.m_hitNormal *= 1.0f / v.r;
                }
                else
                {
                    r.m_hitTime = 0.0f;
                    r.m_hitNormal = r.m_start;
                    r.m_hitNormal -= v.c;
                    r.m_hitNormal.Normalize();
                }
                hit = 1;
            }
        }
    }
    return hit;
}

// The buffer walk.  Six functions in the image -- ray and swept sphere, times
// the three packings -- and they are ONE loop with two substitutions: which
// Cull overload, and which per-primitive Collide.  Only the vertex FETCH
// differs between the packings, and it is written out in each.
//
// Three levels of cull, and a rejection at any level SKIPS by adding the stored
// counts rather than iterating, so a miss costs one Cull per level and not one
// per triangle.  Inside a prim the three-vertex window slides along the strip
// and the winding ALTERNATES, starting unflipped.  The result is an OR: every
// triangle is tested even after a hit, because each gates on m_hitTime and the
// nearest has to win.
//
// The inner count is `numVerts - 2` tested against ZERO, so a prim written with
// fewer than two vertices underflows and runs about four billion times.  Only
// Begin+End with no Vertex can produce one; nothing that draws does.
// OPEN, 121/208, and so is its ray twin.  Every other walker in the family is
// byte-exact from the same body, so this is not the shape: under packing 0 the
// target keeps b.m_verts in a callee-saved register and multiplies by 12 with
// `mult`+`addu`, where ours spills it and gcc folds the reload into `mtlo`
// +`madd`.  One register, and every callee-saved name after it shifts.
int VICollide::CollideBufferV(VICollSphere &s, VICollBuffer &b)
{
    VIVect3 a;
    VIVect3 c;
    VIVect3 d;
    int hit = 0;

    if (Cull(s, b.m_bbox))
        return 0;
    if (s.m_length == 0.0f)
        return 0;
    m_count++;

    char *vp = b.m_verts;
    int vert = 0;
    int prim = 0;

    for (int g = 0; g < b.m_numGroups; g++)
    {
        if (!Cull(s, b.m_groups[g].m_bbox))
        {
            for (int np = b.m_groups[g].m_numPrims; np != 0; np--)
            {
                if (!Cull(s, b.m_prims[prim].m_bbox))
                {
                    int mat = b.m_prims[prim].m_matIdx;
                    int n = b.m_prims[prim].m_numVerts;
                    n = n - 2;

                    c = *(VIVect3 *)(vp + vert * 12);
                    vert++;
                    d = *(VIVect3 *)(vp + vert * 12);
                    vert++;
                    int flip = 0;
                    while (n != 0)
                    {
                        a = c;
                        c = d;
                        d = *(VIVect3 *)(vp + vert * 12);
                        if (flip == 0)
                        {
                            if (Collide(s, mat, a, c, d))
                                hit = 1;
                        }
                        else
                        {
                            if (Collide(s, mat, c, a, d))
                                hit = 1;
                        }
                        flip = 1 - flip;
                        vert++;
                        n--;
                    }
                }
                else
                {
                    vert += b.m_prims[prim].m_numVerts;
                }
                prim++;
            }
        }
        else
        {
            prim += b.m_groups[g].m_numPrims;
            vert += b.m_groups[g].m_numVerts;
        }
    }
    return hit;
}

// A swept sphere against one triangle: the face first, then the three edges.
// Only the FACE path writes m_hitIdx, so an edge or vertex hit leaves whatever
// index was there.  If the two adjusted distances are equal the sweep is
// parallel to the plane and it goes straight to the edges -- and because
// Collide(s, v0, v1) falls through to a vertex test on its FIRST endpoint, the
// three calls cover all three vertices exactly once.
// OPEN, 16/173, and all sixteen are one register rotation: the target holds
// `this`/v1/v2 in s3/s4/s5 where ours holds v1/v2/`this` in s3/s4/s5, and the
// four uses of each are a three-way tie the allocator breaks the other way.
// Two levers got it here.  `ds -= s.m_radius` sits ABOVE the `de < 0` guard --
// the guard's delay slot carries it -- and the two early exits are `goto done`
// and not `return hit`: a `return hit` gives gcc a block whose only reaching
// definition is the initial 0, so it folds to `move v0,zero`, where the shared
// exit has two and keeps the accumulator (`move v0,s2`).
int VICollide::Collide(VICollSphere &s, int idx, VIVect3 &v0, VIVect3 &v1,
                       VIVect3 &v2)
{
    int      hit = 0;
    VIPlane  pl;
    VIVect3  hp;

    if (pl(v0, v1, v2))
    {
        VIVect3 *dir = &s.m_dir;
        if (pl.n.x * dir->x + pl.n.y * dir->y + pl.n.z * dir->z <= 0.0f)
        {
            float ds = pl.Distance(s.m_start);
            if (0.0f < ds)
            {
                float de = pl.Distance(s.m_end) - s.m_radius;
                ds = ds - s.m_radius;
                if (de < 0.0f)
                {
                    if (ds != de)
                    {
                        float t = ds / (ds - de);
                        if (!(t < s.m_hitTime))
                            goto done;

                        if (0.0f < t)
                        {
                            hp = s.m_delta;
                            hp.x = hp.x * t + s.m_start.x - pl.n.x * s.m_radius;
                            hp.y = hp.y * t + s.m_start.y - pl.n.y * s.m_radius;
                            hp.z = hp.z * t + s.m_start.z - pl.n.z * s.m_radius;
                        }
                        else
                        {
                            float k = s.m_radius - ds;
                            hp.x = s.m_start.x - pl.n.x * k;
                            hp.y = s.m_start.y - pl.n.y * k;
                            hp.z = s.m_start.z - pl.n.z * k;
                            t = 0.0f;
                        }

                        if (PointInTriangle(hp, pl, v0, v1, v2))
                        {
                            hit = 1;
                            s.m_hitTime = t;
                            s.m_hitNormal = pl.n;
                            s.m_hitIdx = idx;
                            goto done;
                        }
                    }

                    hit = 0;
                    if (Collide(s, v0, v1))
                        hit = 1;
                    if (Collide(s, v1, v2))
                        hit = 1;
                    if (Collide(s, v2, v0))
                        hit = 1;
                }
            }
        }
    }
done:
    return hit;
}

// The capsule form of CollideObj: the same transform sandwich as the buffer
// form, with the swept sphere taken into object space, collided against the
// capsule, and the contact normal brought back out through m_world.
int VICollide::CollideObj(VICollSphere &s, VICapsule &c)
{
    VICollSphere local;
    VIVect4 nrm;
    VIVect4 wn;

    if (local.Init(s, m_invWorld) < 0)
        return -1;

    if (Collide(local, c) != 0)
    {
        s.m_hitTime = local.m_hitTime;
        s.m_hitIdx = local.m_hitIdx;
        nrm.x = local.m_hitNormal.x;
        nrm.y = local.m_hitNormal.y;
        nrm.z = local.m_hitNormal.z;
        nrm.w = 0.0f;

        wn.Set(nrm * m_world);
        wn.Normalize();

        s.m_hitNormal.x = wn.x;
        s.m_hitNormal.y = wn.y;
        s.m_hitNormal.z = wn.z;
        return 1;
    }
    return 0;
}

// A swept sphere against a capsule -- how an attack or a targeting query hits
// a creature.  It never sweeps a sphere against a capsule directly: every path
// expands the capsule by the sweep's radius and then treats the sweep as a bare
// RAY, which is why every call below goes to a VICollRay overload.
//
// Starting OUTSIDE is the ordinary case: tube plus both caps, all three
// evaluated, each gating on m_hitTime so the nearest wins.  Starting INSIDE
// picks exactly one test, and answers 0 outright if the start is inside the
// capsule's TRUE cylinder -- a sweep that begins buried in the body reports no
// hit at all rather than an escape vector.
// OPEN, 142/236.  The three locals, the four calls and the branch structure
// all line up; the target carries &cyl.m_dir and &c.m_dir in callee-saved
// registers for the whole function where ours rematerialises them, and it has
// one more saved register to do it with.
int VICollide::Collide(VICollSphere &s, VICapsule &c)
{
    VICollRay &ray = s;
    VICylinder cyl;
    VISphere   end;
    VICapsule  ex;
    int        hit = 0;

    ex.m_base = c.m_base;
    ex.m_dir = c.m_dir;
    ex.m_length = c.m_length;
    ex.m_radius = c.m_radius + s.m_radius;

    if (ex.PointInSolid(s.m_start))
    {
        cyl.m_base = c.m_base;
        cyl.m_dir = c.m_dir;
        cyl.m_length = c.m_length;
        cyl.m_radius = c.m_radius;
        if (cyl.PointInSolid(s.m_start))
            return hit;

        cyl.m_base = c.m_base;
        cyl.m_dir = c.m_dir;
        cyl.m_length = c.m_length;
        cyl.m_radius = c.m_radius + s.m_radius;
        if (cyl.PointInSolid(s.m_start))
        {
            hit = CollideNoCaps(ray, cyl) != 0;
            goto done;
        }

        {
            float ds = s.m_start.x * c.m_dir.x + s.m_start.y * c.m_dir.y +
                       s.m_start.z * c.m_dir.z;
            float db = c.m_base.x * c.m_dir.x + c.m_base.y * c.m_dir.y +
                       c.m_base.z * c.m_dir.z;
            if (ds - db < 0.0f)
            {
                end.c = c.m_base;
                end.r = c.m_radius + s.m_radius;
            }
            else
            {
                end.c = c.m_base;
                end.c.x = end.c.x + c.m_dir.x * c.m_length;
                end.c.y = end.c.y + c.m_dir.y * c.m_length;
                end.c.z = end.c.z + c.m_dir.z * c.m_length;
                end.r = c.m_radius + s.m_radius;
            }
        }
        hit = Collide(ray, end) != 0;
        goto done;
    }

    cyl.m_base = c.m_base;
    cyl.m_dir = c.m_dir;
    cyl.m_length = c.m_length;
    cyl.m_radius = c.m_radius + s.m_radius;
    if (CollideNoCaps(ray, cyl))
        hit = 1;

    end.c = c.m_base;
    end.r = c.m_radius + s.m_radius;
    if (Collide(ray, end))
        hit = 1;

    end.c.x = end.c.x + c.m_dir.x * c.m_length;
    end.c.y = end.c.y + c.m_dir.y * c.m_length;
    end.c.z = end.c.z + c.m_dir.z * c.m_length;
    if (Collide(ray, end))
        hit = 1;

done:
    return hit;
}

// The infinite-cylinder half of the capsule test: no end caps, so the caller
// runs the two cap spheres itself.  Three details are not guessable -- t is
// CLAMPED UP to zero rather than rejected, the sqrt term is ADDED so this finds
// the FAR intersection with the tube, and m_hitIdx is never written.
// OPEN, 211/284.  The VIPlane local and the accumulator took it 238 -> 211.
// The target has four stack locals to our three and is 29 words longer, so at
// least one vector we keep in registers is a named local there.
int VICollide::CollideNoCaps(VICollRay &r, const VICylinder &c)
{
    int      hit = 0;
    VIVect3  v;
    VIVect3  h;
    VIPlane  pl;
    float    lsq;
    float    k;
    float    d;
    float    t;
    float    half;
    float    u;
    float    along;

    VIVect3       *rd = &r.m_dir;
    const VIVect3 *cd = &c.m_dir;

    pl.n.x = rd->y * cd->z - rd->z * cd->y;
    pl.n.y = -rd->x * cd->z + rd->z * cd->x;
    pl.n.z = rd->x * cd->y - rd->y * cd->x;

    lsq = pl.n.x * pl.n.x + pl.n.y * pl.n.y + pl.n.z * pl.n.z;
    if (!(0.0f < lsq))
        goto done;

    k = 1.0f / sqrtf(lsq);
    pl.n.x = pl.n.x * k;
    pl.n.y = pl.n.y * k;
    pl.n.z = pl.n.z * k;
    pl.d = -(pl.n.x * c.m_base.x + pl.n.y * c.m_base.y + pl.n.z * c.m_base.z);

    d = pl.Distance(r.m_start);
    if (!(-c.m_radius < d))
        goto done;
    if (!(d < c.m_radius))
        goto done;

    v.x = r.m_start.x - c.m_base.x;
    v.y = r.m_start.y - c.m_base.y;
    v.z = r.m_start.z - c.m_base.z;

    h.x = v.y * cd->z - v.z * cd->y;
    h.y = -v.x * cd->z + v.z * cd->x;
    h.z = v.x * cd->y - v.y * cd->x;

    t = -(h.x * pl.n.x + h.y * pl.n.y + h.z * pl.n.z) * k;
    if (!(0.0f < t))
        goto done;

    h.x = pl.n.y * cd->z - pl.n.z * cd->y;
    h.y = -pl.n.x * cd->z + pl.n.z * cd->x;
    h.z = pl.n.x * cd->y - pl.n.y * cd->x;

    half = sqrtf(c.m_radius * c.m_radius - d * d);
    t = t + half / (rd->x * h.x + rd->y * h.y + rd->z * h.z);
    if (t < 0.0f)
        t = 0.0f;

    t = t * r.m_invLength;
    if (!(t < r.m_hitTime))
        goto done;

    h.x = r.m_delta.x * t + r.m_start.x;
    h.y = r.m_delta.y * t + r.m_start.y;
    h.z = r.m_delta.z * t + r.m_start.z;

    u = (h.x * cd->x + h.y * cd->y + h.z * cd->z) -
        (c.m_base.x * cd->x + c.m_base.y * cd->y + c.m_base.z * cd->z);
    if (!(0.0f <= u))
        goto done;
    if (!(u <= c.m_length))
        goto done;

    v.x = h.x - c.m_base.x;
    v.y = h.y - c.m_base.y;
    v.z = h.z - c.m_base.z;
    along = v.x * cd->x + v.y * cd->y + v.z * cd->z;
    v.x = v.x - cd->x * along;
    v.y = v.y - cd->y * along;
    v.z = v.z - cd->z * along;
    v.Normalize();

    r.m_hitNormal = v;
    r.m_hitTime = t;
    hit = 1;
done:
    return hit;
}

// The two packed forms of the swept-sphere walk: only the vertex FETCH differs.
//
// Three levers settle this family, and they are worth more than the walk is:
//
//  * `int n = ...m_numVerts; n = n - 2;` -- the subtraction as its OWN
//    statement.  Written `- 2` in the initialiser the load lands in a scratch
//    register and the subtract reads it; split, the load goes straight into the
//    loop counter's register.  Worth 12 of 16 words in QueryBufferV alone.
//  * the group-skip pair reads `prim += ...m_numPrims;` FIRST and
//    `vert += ...m_numVerts;` second.  The two adds still come out
//    vert-then-prim; what the source order decides is which of the two LOADS is
//    issued first, and it is the last four words of QueryBufferV.
//  * `char *vp;` is declared BEFORE the accumulator and assigned after the two
//    early returns.  Reload assigns stack slots in the order the pseudos were
//    created, which is DECLARATION order and not first-assignment order, so
//    declaring it after `hit` puts the two spills the wrong way round -- ten
//    words, and byte-exact once they swap.

int VICollide::CollideBufferPackV(VICollSphere &s, VICollBuffer &b)
{
    VIVect3 a;
    VIVect3 c;
    VIVect3 d;
    char *vp;
    int hit = 0;

    if (Cull(s, b.m_bbox))
        return 0;
    if (s.m_length == 0.0f)
        return 0;
    m_count++;

    vp = b.m_verts;
    int vert = 0;
    int prim = 0;

    for (int g = 0; g < b.m_numGroups; g++)
    {
        if (!Cull(s, b.m_groups[g].m_bbox))
        {
            for (int np = b.m_groups[g].m_numPrims; np != 0; np--)
            {
                if (!Cull(s, b.m_prims[prim].m_bbox))
                {
                    int mat = b.m_prims[prim].m_matIdx;
                    int n = b.m_prims[prim].m_numVerts;
                    n = n - 2;

                    {
                        short *q = (short *)(vp + vert * 6);
                        c.x = (float)q[0] * b.m_scale;
                        c.y = (float)q[1] * b.m_scale;
                        c.z = (float)q[2] * b.m_scale;
                    }
                    vert++;
                    {
                        short *q = (short *)(vp + vert * 6);
                        d.x = (float)q[0] * b.m_scale;
                        d.y = (float)q[1] * b.m_scale;
                        d.z = (float)q[2] * b.m_scale;
                    }
                    vert++;

                    int flip = 0;
                    while (n != 0)
                    {
                        a = c;
                        c = d;
                        {
                            short *q = (short *)(vp + vert * 6);
                            d.x = (float)q[0] * b.m_scale;
                            d.y = (float)q[1] * b.m_scale;
                            d.z = (float)q[2] * b.m_scale;
                        }
                        if (flip == 0)
                        {
                            if (Collide(s, mat, a, c, d))
                                hit = 1;
                        }
                        else
                        {
                            if (Collide(s, mat, c, a, d))
                                hit = 1;
                        }
                        flip = 1 - flip;
                        vert++;
                        n--;
                    }
                }
                else
                {
                    vert += b.m_prims[prim].m_numVerts;
                }
                prim++;
            }
        }
        else
        {
            prim += b.m_groups[g].m_numPrims;
            vert += b.m_groups[g].m_numVerts;
        }
    }
    return hit;
}

// OPEN, 77/277, and so are its ray twin and the query walker.  The address of
// the per-vertex-group translation is settled (see the grouping note in the
// body); what is left is that the target serialises the three offset adds --
// load, add, store, three times over one register -- where ours loads all three
// first.
int VICollide::CollideBufferPackVGF(VICollSphere &s, VICollBuffer &b)
{
    VIVect3 a;
    VIVect3 c;
    VIVect3 d;
    char *vp;
    int hit = 0;

    if (Cull(s, b.m_bbox))
        return 0;
    if (s.m_length == 0.0f)
        return 0;
    m_count++;

    vp = b.m_verts;
    int vert = 0;
    int prim = 0;

    for (int g = 0; g < b.m_numGroups; g++)
    {
        if (!Cull(s, b.m_groups[g].m_bbox))
        {
            for (int np = b.m_groups[g].m_numPrims; np != 0; np--)
            {
                if (!Cull(s, b.m_prims[prim].m_bbox))
                {
                    int mat = b.m_prims[prim].m_matIdx;
                    int n = b.m_prims[prim].m_numVerts;
                    n = n - 2;

                    {
                        short *q = (short *)(vp + vert * 8);
                        c.x = (float)q[0] * b.m_scale;
                        c.y = (float)q[1] * b.m_scale;
                        c.z = (float)q[2] * b.m_scale;
                        int pg = ((signed char *)q)[6];
                        // The GROUPING is the target's: (this + pg*12) + 144,
                        // where `&m_preTranslations[pg]` folds the other way
                        // round to this + (144 + pg*12) and costs 26 words.
                        VIVect3 *pt = (VIVect3 *)((char *)this +
                                                  pg * sizeof(VIVect3)) + 12;
                        c.x = c.x + pt->x;
                        c.y = c.y + pt->y;
                        c.z = c.z + pt->z;
                    }
                    vert++;
                    {
                        short *q = (short *)(vp + vert * 8);
                        d.x = (float)q[0] * b.m_scale;
                        d.y = (float)q[1] * b.m_scale;
                        d.z = (float)q[2] * b.m_scale;
                        int pg = ((signed char *)q)[6];
                        // The GROUPING is the target's: (this + pg*12) + 144,
                        // where `&m_preTranslations[pg]` folds the other way
                        // round to this + (144 + pg*12) and costs 26 words.
                        VIVect3 *pt = (VIVect3 *)((char *)this +
                                                  pg * sizeof(VIVect3)) + 12;
                        d.x = d.x + pt->x;
                        d.y = d.y + pt->y;
                        d.z = d.z + pt->z;
                    }
                    vert++;

                    int flip = 0;
                    while (n != 0)
                    {
                        a = c;
                        c = d;
                        {
                            short *q = (short *)(vp + vert * 8);
                            d.x = (float)q[0] * b.m_scale;
                            d.y = (float)q[1] * b.m_scale;
                            d.z = (float)q[2] * b.m_scale;
                            int pg = ((signed char *)q)[6];
                            // The GROUPING is the target's: (this + pg*12) + 144,
                        // where `&m_preTranslations[pg]` folds the other way
                        // round to this + (144 + pg*12) and costs 26 words.
                        VIVect3 *pt = (VIVect3 *)((char *)this +
                                                  pg * sizeof(VIVect3)) + 12;
                            d.x = d.x + pt->x;
                            d.y = d.y + pt->y;
                            d.z = d.z + pt->z;
                        }
                        if (flip == 0)
                        {
                            if (Collide(s, mat, a, c, d))
                                hit = 1;
                        }
                        else
                        {
                            if (Collide(s, mat, c, a, d))
                                hit = 1;
                        }
                        flip = 1 - flip;
                        vert++;
                        n--;
                    }
                }
                else
                {
                    vert += b.m_prims[prim].m_numVerts;
                }
                prim++;
            }
        }
        else
        {
            prim += b.m_groups[g].m_numPrims;
            vert += b.m_groups[g].m_numVerts;
        }
    }
    return hit;
}

// And the three ray walkers.

// OPEN, 121/208 -- the same one register as the sphere twin above.
int VICollide::CollideBufferV(VICollRay &r, VICollBuffer &b)
{
    VIVect3 a;
    VIVect3 c;
    VIVect3 d;
    int hit = 0;

    if (Cull(r, b.m_bbox))
        return 0;
    if (r.m_length == 0.0f)
        return 0;
    m_count++;

    char *vp = b.m_verts;
    int vert = 0;
    int prim = 0;

    for (int g = 0; g < b.m_numGroups; g++)
    {
        if (!Cull(r, b.m_groups[g].m_bbox))
        {
            for (int np = b.m_groups[g].m_numPrims; np != 0; np--)
            {
                if (!Cull(r, b.m_prims[prim].m_bbox))
                {
                    int mat = b.m_prims[prim].m_matIdx;
                    int n = b.m_prims[prim].m_numVerts;
                    n = n - 2;

                    c = *(VIVect3 *)(vp + vert * 12);
                    vert++;
                    d = *(VIVect3 *)(vp + vert * 12);
                    vert++;

                    int flip = 0;
                    while (n != 0)
                    {
                        a = c;
                        c = d;
                        d = *(VIVect3 *)(vp + vert * 12);
                        if (flip == 0)
                        {
                            if (Collide(r, mat, a, c, d))
                                hit = 1;
                        }
                        else
                        {
                            if (Collide(r, mat, c, a, d))
                                hit = 1;
                        }
                        flip = 1 - flip;
                        vert++;
                        n--;
                    }
                }
                else
                {
                    vert += b.m_prims[prim].m_numVerts;
                }
                prim++;
            }
        }
        else
        {
            prim += b.m_groups[g].m_numPrims;
            vert += b.m_groups[g].m_numVerts;
        }
    }
    return hit;
}

int VICollide::CollideBufferPackV(VICollRay &r, VICollBuffer &b)
{
    VIVect3 a;
    VIVect3 c;
    VIVect3 d;
    char *vp;
    int hit = 0;

    if (Cull(r, b.m_bbox))
        return 0;
    if (r.m_length == 0.0f)
        return 0;
    m_count++;

    vp = b.m_verts;
    int vert = 0;
    int prim = 0;

    for (int g = 0; g < b.m_numGroups; g++)
    {
        if (!Cull(r, b.m_groups[g].m_bbox))
        {
            for (int np = b.m_groups[g].m_numPrims; np != 0; np--)
            {
                if (!Cull(r, b.m_prims[prim].m_bbox))
                {
                    int mat = b.m_prims[prim].m_matIdx;
                    int n = b.m_prims[prim].m_numVerts;
                    n = n - 2;

                    {
                        short *q = (short *)(vp + vert * 6);
                        c.x = (float)q[0] * b.m_scale;
                        c.y = (float)q[1] * b.m_scale;
                        c.z = (float)q[2] * b.m_scale;
                    }
                    vert++;
                    {
                        short *q = (short *)(vp + vert * 6);
                        d.x = (float)q[0] * b.m_scale;
                        d.y = (float)q[1] * b.m_scale;
                        d.z = (float)q[2] * b.m_scale;
                    }
                    vert++;

                    int flip = 0;
                    while (n != 0)
                    {
                        a = c;
                        c = d;
                        {
                            short *q = (short *)(vp + vert * 6);
                            d.x = (float)q[0] * b.m_scale;
                            d.y = (float)q[1] * b.m_scale;
                            d.z = (float)q[2] * b.m_scale;
                        }
                        if (flip == 0)
                        {
                            if (Collide(r, mat, a, c, d))
                                hit = 1;
                        }
                        else
                        {
                            if (Collide(r, mat, c, a, d))
                                hit = 1;
                        }
                        flip = 1 - flip;
                        vert++;
                        n--;
                    }
                }
                else
                {
                    vert += b.m_prims[prim].m_numVerts;
                }
                prim++;
            }
        }
        else
        {
            prim += b.m_groups[g].m_numPrims;
            vert += b.m_groups[g].m_numVerts;
        }
    }
    return hit;
}

// OPEN, 77/277 -- see the sphere twin.
int VICollide::CollideBufferPackVGF(VICollRay &r, VICollBuffer &b)
{
    VIVect3 a;
    VIVect3 c;
    VIVect3 d;
    char *vp;
    int hit = 0;

    if (Cull(r, b.m_bbox))
        return 0;
    if (r.m_length == 0.0f)
        return 0;
    m_count++;

    vp = b.m_verts;
    int vert = 0;
    int prim = 0;

    for (int g = 0; g < b.m_numGroups; g++)
    {
        if (!Cull(r, b.m_groups[g].m_bbox))
        {
            for (int np = b.m_groups[g].m_numPrims; np != 0; np--)
            {
                if (!Cull(r, b.m_prims[prim].m_bbox))
                {
                    int mat = b.m_prims[prim].m_matIdx;
                    int n = b.m_prims[prim].m_numVerts;
                    n = n - 2;

                    {
                        short *q = (short *)(vp + vert * 8);
                        c.x = (float)q[0] * b.m_scale;
                        c.y = (float)q[1] * b.m_scale;
                        c.z = (float)q[2] * b.m_scale;
                        int pg = ((signed char *)q)[6];
                        // The GROUPING is the target's: (this + pg*12) + 144,
                        // where `&m_preTranslations[pg]` folds the other way
                        // round to this + (144 + pg*12) and costs 26 words.
                        VIVect3 *pt = (VIVect3 *)((char *)this +
                                                  pg * sizeof(VIVect3)) + 12;
                        c.x = c.x + pt->x;
                        c.y = c.y + pt->y;
                        c.z = c.z + pt->z;
                    }
                    vert++;
                    {
                        short *q = (short *)(vp + vert * 8);
                        d.x = (float)q[0] * b.m_scale;
                        d.y = (float)q[1] * b.m_scale;
                        d.z = (float)q[2] * b.m_scale;
                        int pg = ((signed char *)q)[6];
                        // The GROUPING is the target's: (this + pg*12) + 144,
                        // where `&m_preTranslations[pg]` folds the other way
                        // round to this + (144 + pg*12) and costs 26 words.
                        VIVect3 *pt = (VIVect3 *)((char *)this +
                                                  pg * sizeof(VIVect3)) + 12;
                        d.x = d.x + pt->x;
                        d.y = d.y + pt->y;
                        d.z = d.z + pt->z;
                    }
                    vert++;

                    int flip = 0;
                    while (n != 0)
                    {
                        a = c;
                        c = d;
                        {
                            short *q = (short *)(vp + vert * 8);
                            d.x = (float)q[0] * b.m_scale;
                            d.y = (float)q[1] * b.m_scale;
                            d.z = (float)q[2] * b.m_scale;
                            int pg = ((signed char *)q)[6];
                            // The GROUPING is the target's: (this + pg*12) + 144,
                        // where `&m_preTranslations[pg]` folds the other way
                        // round to this + (144 + pg*12) and costs 26 words.
                        VIVect3 *pt = (VIVect3 *)((char *)this +
                                                  pg * sizeof(VIVect3)) + 12;
                            d.x = d.x + pt->x;
                            d.y = d.y + pt->y;
                            d.z = d.z + pt->z;
                        }
                        if (flip == 0)
                        {
                            if (Collide(r, mat, a, c, d))
                                hit = 1;
                        }
                        else
                        {
                            if (Collide(r, mat, c, a, d))
                                hit = 1;
                        }
                        flip = 1 - flip;
                        vert++;
                        n--;
                    }
                }
                else
                {
                    vert += b.m_prims[prim].m_numVerts;
                }
                prim++;
            }
        }
        else
        {
            prim += b.m_groups[g].m_numPrims;
            vert += b.m_groups[g].m_numVerts;
        }
    }
    return hit;
}

// The query walk.  Four differences from the collide walk, and each one is a
// fact about the engine: EVERY vertex is transformed by m_world before it is
// tested; there is NO whole-buffer cull; the cull is a VIBBox overlap against
// the SECOND box, and the FIRST parameter is dead in all three walkers (QueryObj
// passes the WORLD-space box there and its image under m_invWorld second); and
// the per-triangle call APPENDS rather than answering a hit.
//
// Under PackVGF the index handed to the list is the VERTEX's flora byte, signed,
// not the prim's material -- the prim's material never reaches the list at all.

int VICollide::QueryBufferV(const VIBBox &w, const VIBBox &o,
                                 VIQueryTriList &l, VICollBuffer &b)
{
    VIVect3 raw;
    VIVect3 a;
    VIVect3 c;
    VIVect3 d;

    int vert = 0;
    int prim = 0;
    int g;
    char *vp = b.m_verts;

    for (g = 0; g < b.m_numGroups; g++)
    {
        if (o.Intersects(b.m_groups[g].m_bbox))
        {
            for (int np = b.m_groups[g].m_numPrims; np != 0; np--)
            {
                if (o.Intersects(b.m_prims[prim].m_bbox))
                {
                    int mat = b.m_prims[prim].m_matIdx;
                    int n = b.m_prims[prim].m_numVerts;
                    n = n - 2;

                    m_world.Mul(*(VIVect3 *)(vp + vert * 12), c);
                    vert++;
                    m_world.Mul(*(VIVect3 *)(vp + vert * 12), d);
                    vert++;

                    int flip = 0;
                    while (n != 0)
                    {
                        a = c;
                        c = d;
                        m_world.Mul(*(VIVect3 *)(vp + vert * 12), d);
                        int idx = mat;
                        if (flip == 0)
                        {
                            if (l.Intersect(idx, a, c, d) < 0)
                                return -1;
                        }
                        else
                        {
                            if (l.Intersect(idx, c, a, d) < 0)
                                return -1;
                        }
                        flip = 1 - flip;
                        vert++;
                        n--;
                    }
                }
                else
                {
                    vert += b.m_prims[prim].m_numVerts;
                }
                prim++;
            }
        }
        else
        {
            prim += b.m_groups[g].m_numPrims;
            vert += b.m_groups[g].m_numVerts;
        }
    }
    return 0;
}

int VICollide::QueryBufferPackV(const VIBBox &w, const VIBBox &o,
                                 VIQueryTriList &l, VICollBuffer &b)
{
    VIVect3 raw;
    VIVect3 a;
    VIVect3 c;
    VIVect3 d;

    int vert = 0;
    int prim = 0;
    int g;
    char *vp = b.m_verts;

    for (g = 0; g < b.m_numGroups; g++)
    {
        if (o.Intersects(b.m_groups[g].m_bbox))
        {
            for (int np = b.m_groups[g].m_numPrims; np != 0; np--)
            {
                if (o.Intersects(b.m_prims[prim].m_bbox))
                {
                    int mat = b.m_prims[prim].m_matIdx;
                    int n = b.m_prims[prim].m_numVerts;
                    n = n - 2;

                    {
                        short *q = (short *)(vp + vert * 6);
                        raw.x = (float)q[0] * b.m_scale;
                        raw.y = (float)q[1] * b.m_scale;
                        raw.z = (float)q[2] * b.m_scale;
                    }
                    m_world.Mul(raw, c);
                    vert++;
                    {
                        short *q = (short *)(vp + vert * 6);
                        raw.x = (float)q[0] * b.m_scale;
                        raw.y = (float)q[1] * b.m_scale;
                        raw.z = (float)q[2] * b.m_scale;
                    }
                    m_world.Mul(raw, d);
                    vert++;

                    int flip = 0;
                    while (n != 0)
                    {
                        a = c;
                        c = d;
                        {
                            short *q = (short *)(vp + vert * 6);
                            raw.x = (float)q[0] * b.m_scale;
                            raw.y = (float)q[1] * b.m_scale;
                            raw.z = (float)q[2] * b.m_scale;
                        }
                        m_world.Mul(raw, d);
                        int idx = mat;
                        if (flip == 0)
                        {
                            if (l.Intersect(idx, a, c, d) < 0)
                                return -1;
                        }
                        else
                        {
                            if (l.Intersect(idx, c, a, d) < 0)
                                return -1;
                        }
                        flip = 1 - flip;
                        vert++;
                        n--;
                    }
                }
                else
                {
                    vert += b.m_prims[prim].m_numVerts;
                }
                prim++;
            }
        }
        else
        {
            prim += b.m_groups[g].m_numPrims;
            vert += b.m_groups[g].m_numVerts;
        }
    }
    return 0;
}

// OPEN, 81/260 -- the same offset-add serialisation as the collide walkers.
int VICollide::QueryBufferPackVGF(const VIBBox &w, const VIBBox &o,
                                 VIQueryTriList &l, VICollBuffer &b)
{
    VIVect3 raw;
    VIVect3 a;
    VIVect3 c;
    VIVect3 d;

    int vert = 0;
    int prim = 0;
    int g;
    char *vp = b.m_verts;

    for (g = 0; g < b.m_numGroups; g++)
    {
        if (o.Intersects(b.m_groups[g].m_bbox))
        {
            for (int np = b.m_groups[g].m_numPrims; np != 0; np--)
            {
                if (o.Intersects(b.m_prims[prim].m_bbox))
                {
                    int mat = b.m_prims[prim].m_matIdx;
                    int n = b.m_prims[prim].m_numVerts;
                    n = n - 2;

                    {
                        short *q = (short *)(vp + vert * 8);
                        raw.x = (float)q[0] * b.m_scale;
                        raw.y = (float)q[1] * b.m_scale;
                        raw.z = (float)q[2] * b.m_scale;
                        int pg = ((signed char *)q)[6];
                        // The GROUPING is the target's: (this + pg*12) + 144,
                        // where `&m_preTranslations[pg]` folds the other way
                        // round to this + (144 + pg*12) and costs 26 words.
                        VIVect3 *pt = (VIVect3 *)((char *)this +
                                                  pg * sizeof(VIVect3)) + 12;
                        raw.x = raw.x + pt->x;
                        raw.y = raw.y + pt->y;
                        raw.z = raw.z + pt->z;
                    }
                    m_world.Mul(raw, c);
                    vert++;
                    {
                        short *q = (short *)(vp + vert * 8);
                        raw.x = (float)q[0] * b.m_scale;
                        raw.y = (float)q[1] * b.m_scale;
                        raw.z = (float)q[2] * b.m_scale;
                        int pg = ((signed char *)q)[6];
                        // The GROUPING is the target's: (this + pg*12) + 144,
                        // where `&m_preTranslations[pg]` folds the other way
                        // round to this + (144 + pg*12) and costs 26 words.
                        VIVect3 *pt = (VIVect3 *)((char *)this +
                                                  pg * sizeof(VIVect3)) + 12;
                        raw.x = raw.x + pt->x;
                        raw.y = raw.y + pt->y;
                        raw.z = raw.z + pt->z;
                    }
                    m_world.Mul(raw, d);
                    vert++;

                    int flip = 0;
                    while (n != 0)
                    {
                        a = c;
                        c = d;
                        {
                            short *q = (short *)(vp + vert * 8);
                            raw.x = (float)q[0] * b.m_scale;
                            raw.y = (float)q[1] * b.m_scale;
                            raw.z = (float)q[2] * b.m_scale;
                            int pg = ((signed char *)q)[6];
                            // The GROUPING is the target's: (this + pg*12) + 144,
                        // where `&m_preTranslations[pg]` folds the other way
                        // round to this + (144 + pg*12) and costs 26 words.
                        VIVect3 *pt = (VIVect3 *)((char *)this +
                                                  pg * sizeof(VIVect3)) + 12;
                            raw.x = raw.x + pt->x;
                            raw.y = raw.y + pt->y;
                            raw.z = raw.z + pt->z;
                        }
                        m_world.Mul(raw, d);
                        int idx = ((signed char *)(vp + vert * 8))[7];
                        if (flip == 0)
                        {
                            if (l.Intersect(idx, a, c, d) < 0)
                                return -1;
                        }
                        else
                        {
                            if (l.Intersect(idx, c, a, d) < 0)
                                return -1;
                        }
                        flip = 1 - flip;
                        vert++;
                        n--;
                    }
                }
                else
                {
                    vert += b.m_prims[prim].m_numVerts;
                }
                prim++;
            }
        }
        else
        {
            prim += b.m_groups[g].m_numPrims;
            vert += b.m_groups[g].m_numVerts;
        }
    }
    return 0;
}

// One swept sphere against one SEGMENT -- the largest function in the class.
//
// Two things here are not guessable.  The edge test ALWAYS falls through to a
// vertex test against v0, even after a successful edge hit; the vertex test's
// own m_hitTime gate is what makes the nearer of the two win, and because the
// triangle wrapper calls this for (v0,v1), (v1,v2) and (v2,v0), every vertex
// gets tested exactly once.  And the two slab rejections are the ONLY exits
// that skip that fallback: if the sphere centre is further than its radius
// from the edge's plane, v0 is not tested at all.
// OPEN, 189/334.  The plane local is settled -- the four zeroed words at the
// top of the frame are a VIPlane, and `dist` is its Distance() -- and so is the
// &m_dir pointer variable, which together took it 252 -> 189.  What is left is
// one more stack local than we have (224 vs 208 bytes) and the callee-saved
// register that goes with it.
int VICollide::Collide(VICollSphere &s, VIVect3 &v0, VIVect3 &v1)
{
    int     hit = 0;
    int     over = 0;
    VIVect3 e;
    VIVect3 q;
    VIVect3 cp;
    VIPlane pl;
    float   lsq;
    float   invC;
    float   dist;
    float   f;
    float   t;
    float   chord;
    float   along;
    float   k;

    VIVect3 *dir = &s.m_dir;

    e.x = v1.x - v0.x;
    e.y = v1.y - v0.y;
    e.z = v1.z - v0.z;

    pl.n.x = dir->y * e.z - dir->z * e.y;
    pl.n.y = -dir->x * e.z + dir->z * e.x;
    pl.n.z = dir->x * e.y - dir->y * e.x;

    lsq = pl.n.x * pl.n.x + pl.n.y * pl.n.y + pl.n.z * pl.n.z;
    if (!(0.0f < lsq))
        return Collide(s, v0);

    invC = 1.0f / sqrtf(lsq);
    pl.n.x = pl.n.x * invC;
    pl.n.y = pl.n.y * invC;
    pl.n.z = pl.n.z * invC;

    pl.d = -(pl.n.x * v0.x + pl.n.y * v0.y + pl.n.z * v0.z);
    dist = pl.Distance(s.m_start);
    if (!(-s.m_radius < dist))
        return hit;
    if (!(dist < s.m_radius))
        return hit;

    q.x = s.m_start.x - v0.x;
    q.y = s.m_start.y - v0.y;
    q.z = s.m_start.z - v0.z;

    cp.x = q.y * e.z - q.z * e.y;
    cp.y = -q.x * e.z + q.z * e.x;
    cp.z = q.x * e.y - q.y * e.x;

    f = -(cp.x * pl.n.x + cp.y * pl.n.y + cp.z * pl.n.z) * invC;
    if (!(0.0f < f))
        goto vertex;
    if (e.Normalize() == 0)
        goto vertex;

    chord = sqrtf(s.m_radius * s.m_radius - dist * dist);
    q.x = pl.n.y * e.z - pl.n.z * e.y;
    q.y = -pl.n.x * e.z + pl.n.z * e.x;
    q.z = pl.n.x * e.y - pl.n.y * e.x;
    f = f + chord / (dir->x * q.x + dir->y * q.y + dir->z * q.z);

    if (f < 0.0f)
    {
        over = 1;
        f = 0.0f;
    }

    t = f * s.m_invLength;
    if (!(t < s.m_hitTime))
        goto vertex;

    cp.x = s.m_delta.x * t + s.m_start.x;
    cp.y = s.m_delta.y * t + s.m_start.y;
    cp.z = s.m_delta.z * t + s.m_start.z;

    along = e.x * cp.x + e.y * cp.y + e.z * cp.z;
    if (!(0.0f < along - (e.x * v0.x + e.y * v0.y + e.z * v0.z)))
        goto vertex;
    if (!(along - (e.x * v1.x + e.y * v1.y + e.z * v1.z) < 0.0f))
        goto vertex;

    q.x = cp.x - v0.x;
    q.y = cp.y - v0.y;
    q.z = cp.z - v0.z;
    k = q.x * e.x + q.y * e.y + q.z * e.z;
    q.x = q.x - e.x * k;
    q.y = q.y - e.y * k;
    q.z = q.z - e.z * k;
    if (over)
    {
        q.Normalize();
    }
    else
    {
        q.x = q.x * s.m_invRadius;
        q.y = q.y * s.m_invRadius;
        q.z = q.z * s.m_invRadius;
    }
    s.m_hitNormal = q;
    s.m_hitTime = t;
    hit = 1;

vertex:
    if (Collide(s, v0))
        hit = 1;
    return hit;
}
