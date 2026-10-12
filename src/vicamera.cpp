// VICamera -- SUPPORT 0x0041C7F8 .. 0x0041DBA8.  Phase 31.
#include "vi.h"

extern "C" float tanf(float);

// The cross product returned BY VALUE, the shape VIParticleEmitter settled in
// Phase 28: the result is built in a stack temporary whose address is taken by
// the reference VIVect3::operator= wants, so its three stores survive as dead
// code beside the real ones.  CalcAxes leaves three such temporaries.
static inline VIVect3 Cross(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.y * b.z - a.z * b.y,
                   -a.x * b.z + a.z * b.x,
                   a.x * b.y - a.y * b.x);
}

// 0x0041C7F8.  Build the projection matrix out of a vertical field of view in
// DEGREES, then hand the position and direction to CalcAxes/Update.
int VICamera::Init(VIRaster &r, const VIVect3 &pos, const VIVect3 &dir,
                   float fov, float aspect, float nearZ, float farZ)
{
    VIMatrix44 m;
    VIVect3 right, up, fwd;

    float t = tanf(fov * 0.017453292f * 0.5f);

    r.Frustum(m, -t * nearZ * aspect, t * nearZ * aspect,
              t * nearZ, -t * nearZ, nearZ, farZ);

    m_near = nearZ;
    m_far = farZ;
    m_fov = fov;
    m_aspect = aspect;

    CalcAxes(dir, right, up, fwd);
    if (Update(pos, right, up, fwd, m) < 0)
        return -1;
    return 0;
}

// 0x0041C9B0.  Re-aim from a position and a direction, keeping the projection
// matrix that is already stored.
int VICamera::operator()(VIVect3 &pos, VIVect3 &dir)
{
    VIVect3 right, up, fwd;

    CalcAxes(dir, right, up, fwd);
    if (Update(pos, right, up, fwd, m_proj) < 0)
        return -1;
    return 0;
}

// 0x0041CA58.  The same, with the caller supplying the basis itself.  Only the
// forward vector reaches m_dir.
int VICamera::operator()(VIVect3 &pos, VIVect3 &right, VIVect3 &up,
                         VIVect3 &fwd)
{
    right.Normalize();
    up.Normalize();
    fwd.Normalize();

    m_dir = fwd;

    if (Update(pos, right, up, fwd, m_proj) < 0)
        return -1;
    return 0;
}

// 0x0041CB08 / 0x0041CB38.  Re-init with the fields already stored.
int VICamera::operator()(VIRaster &r, float farZ)
{
    return Init(r, m_position, m_dir, m_fov, m_aspect, m_near, farZ);
}

int VICamera::operator()(VIRaster &r, VIVect3 &pos, VIVect3 &dir, float farZ)
{
    return Init(r, pos, dir, m_fov, m_aspect, m_near, farZ);
}

// 0x0041D3C8.  The orthonormal basis.  `fwd` takes a copy of the direction and
// is normalised in place; `right` is fwd x world-up, and if that degenerates
// (the camera looking straight up or down) the world x axis is used instead.
int VICamera::CalcAxes(const VIVect3 &dir, VIVect3 &right, VIVect3 &up,
                       VIVect3 &fwd)
{
    VIVect3 u;

    fwd = dir;
    fwd.Normalize();
    m_dir = fwd;

    u.x = 0.0f;
    u.y = 1.0f;
    u.z = 0.0f;
    right = Cross(fwd, u);
    if (!right.Normalize())
    {
        u.x = 1.0f;
        u.y = 0.0f;
        u.z = 0.0f;
        right = Cross(fwd, u);
        right.Normalize();
    }

    up = Cross(right, fwd);
    return 0;
}

// 0x0041D580.  Stamp the basis into the two matrices -- m_view with the axes
// as COLUMNS and the translation as three dot products, m_world with them as
// ROWS and the position in row 3 -- then compose and rebuild the frustum.
int VICamera::Update(const VIVect3 &pos, VIVect3 &right, VIVect3 &up,
                     VIVect3 &fwd, VIMatrix44 &proj)
{
    VIMatrix44 m;
    VIMatrix44 &view = m_view;
    VIMatrix44 &wld = m_world;

    m_frustum.Init();

    m_position = pos;

    m_view.m[0].x = right.x;
    m_view.m[0].y = up.x;
    m_view.m[0].z = fwd.x;
    m_view.m[0].w = 0.0f;
    m_view.m[1].x = right.y;
    m_view.m[1].y = up.y;
    m_view.m[1].z = fwd.y;
    m_view.m[1].w = 0.0f;
    m_view.m[2].x = right.z;
    m_view.m[2].y = up.z;
    m_view.m[2].z = fwd.z;
    m_view.m[2].w = 0.0f;
    // The three dot products read the COLUMN back out of m_view, not the
    // vector it came from: a store and a load of the same slot forward, where
    // a second read of `right.x` has to be re-loaded because the stores above
    // it may alias the caller's vector.  That is the whole 6-instruction gap.
    m_view.m[3].x = -pos.x * m_view.m[0].x + -pos.y * m_view.m[1].x +
                    -pos.z * m_view.m[2].x;
    m_view.m[3].y = -pos.x * m_view.m[0].y + -pos.y * m_view.m[1].y +
                    -pos.z * m_view.m[2].y;
    m_view.m[3].z = -pos.x * m_view.m[0].z + -pos.y * m_view.m[1].z +
                    -pos.z * m_view.m[2].z;
    m_view.m[3].w = 1.0f;

    m_world.m[0].x = right.x;
    m_world.m[0].y = right.y;
    m_world.m[0].z = right.z;
    m_world.m[0].w = 0.0f;
    m_world.m[1].x = up.x;
    m_world.m[1].y = up.y;
    m_world.m[1].z = up.z;
    m_world.m[1].w = 0.0f;
    m_world.m[2].x = fwd.x;
    m_world.m[2].y = fwd.y;
    m_world.m[2].z = fwd.z;
    m_world.m[2].w = 0.0f;
    m_world.m[3].x = pos.x;
    m_world.m[3].y = pos.y;
    m_world.m[3].z = pos.z;
    m_world.m[3].w = 1.0f;

    m_proj = proj;
    m_viewProj.Mul(view, m_proj);

    m.Invert(m_proj);
    CalcFrustum(m, wld);
    return 0;
}

// 0x0041D808.  Eight NDC cube corners through the inverse projection and the
// perspective divide, then out to world space.
int VICamera::CalcFrustum(const VIMatrix44 &invProj, const VIMatrix44 &world)
{
    VIVect4 v;

    {
        v.x = -1.0f;
        v.y = 1.0f;
        v.z = -1.0f;
        v.w = 1.0f;

        VIVect4 t = v * invProj;

        float k = 1.0f / t.w;
        v.x = t.x * k;
        v.y = t.y * k;
        v.z = t.z * k;
        v.w = t.w * k;

        world.Mul(v, m_frustum.m_verts[0]);
    }

    {
        v.x = 1.0f;
        v.y = 1.0f;
        v.z = -1.0f;
        v.w = 1.0f;

        VIVect4 t = v * invProj;

        float k = 1.0f / t.w;
        v.x = t.x * k;
        v.y = t.y * k;
        v.z = t.z * k;
        v.w = t.w * k;

        world.Mul(v, m_frustum.m_verts[1]);
    }

    {
        v.x = 1.0f;
        v.y = -1.0f;
        v.z = -1.0f;
        v.w = 1.0f;

        VIVect4 t = v * invProj;

        float k = 1.0f / t.w;
        v.x = t.x * k;
        v.y = t.y * k;
        v.z = t.z * k;
        v.w = t.w * k;

        world.Mul(v, m_frustum.m_verts[2]);
    }

    {
        v.x = -1.0f;
        v.y = -1.0f;
        v.z = -1.0f;
        v.w = 1.0f;

        VIVect4 t = v * invProj;

        float k = 1.0f / t.w;
        v.x = t.x * k;
        v.y = t.y * k;
        v.z = t.z * k;
        v.w = t.w * k;

        world.Mul(v, m_frustum.m_verts[3]);
    }

    {
        v.x = -1.0f;
        v.y = 1.0f;
        v.z = 1.0f;
        v.w = 1.0f;

        VIVect4 t = v * invProj;

        float k = 1.0f / t.w;
        v.x = t.x * k;
        v.y = t.y * k;
        v.z = t.z * k;
        v.w = t.w * k;

        world.Mul(v, m_frustum.m_verts[4]);
    }

    {
        v.x = 1.0f;
        v.y = 1.0f;
        v.z = 1.0f;
        v.w = 1.0f;

        VIVect4 t = v * invProj;

        float k = 1.0f / t.w;
        v.x = t.x * k;
        v.y = t.y * k;
        v.z = t.z * k;
        v.w = t.w * k;

        world.Mul(v, m_frustum.m_verts[5]);
    }

    {
        v.x = 1.0f;
        v.y = -1.0f;
        v.z = 1.0f;
        v.w = 1.0f;

        VIVect4 t = v * invProj;

        float k = 1.0f / t.w;
        v.x = t.x * k;
        v.y = t.y * k;
        v.z = t.z * k;
        v.w = t.w * k;

        world.Mul(v, m_frustum.m_verts[6]);
    }

    {
        v.x = -1.0f;
        v.y = -1.0f;
        v.z = 1.0f;
        v.w = 1.0f;

        VIVect4 t = v * invProj;

        float k = 1.0f / t.w;
        v.x = t.x * k;
        v.y = t.y * k;
        v.z = t.z * k;
        v.w = t.w * k;

        world.Mul(v, m_frustum.m_verts[7]);
    }

    m_frustum.CalcPlanes();
    return 0;
}
