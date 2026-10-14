// VIRadialFloraSystem -- SUPPORT 0x0045E000 .. 0x0045FAF0, its own TU, with
// VIZoneRadialFlora's Init/Clear/Precalc at the end.
//
// The 3x3 swatch grid of ground flora: every function written, all exact
// (phase 61).
#include "vi.h"

extern "C" void qsort(void *, unsigned int, unsigned int, int (*)(const void *, const void *));
extern "C" int  rand(void);
extern "C" void srand(unsigned int);

// A random fraction in [0, 1], ten bits of rand() -- and no outer parentheses:
// `k * VIRAND` multiplies by the raw ten bits first and the scale after.
#define VIRAND (float)(rand() & 0x3FF) * (1.0f / 1023.0f)

static inline VIVect3 Cross(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.y * b.z - a.z * b.y,
                   -a.x * b.z + a.z * b.x,
                   a.x * b.y - a.y * b.x);
}

static inline void Cross(VIVect3 &out, const VIVect3 &a, const VIVect3 &b)
{
    out.x = a.y * b.z - a.z * b.y;
    out.y = -a.x * b.z + a.z * b.x;
    out.z = a.x * b.y - a.y * b.x;
}

static inline float Dot(const VIVect3 &a, const VIVect3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline void Scale(VIVect3 &out, const VIVect3 &v, float k)
{
    out.x = v.x * k;
    out.y = v.y * k;
    out.z = v.z * k;
}

static inline void AddVec(VIVect3 &accum, const VIVect3 &v, float k)
{
    accum.x += v.x * k;
    accum.y += v.y * k;
    accum.z += v.z * k;
}

VIRadialFloraSystem::VIRadialFloraSystem()
{
    // m_loc's own constructor runs first (its two VIVect2I zeroings, then its
    // `m_zone = -1`); phase 32 wrote it out as five loose stores here.
    m_scene = 0;
    m_raster = 0;
    m_collide = 0;
    for (int i = 0; i < 9; i++)
        m_swatches[i] = 0;
}

int VIRadialFloraSystem::Init(VIScene &sc)
{
    if (Clear() < 0)
        return -1;

    m_scene = &sc;
    m_raster = sc.m_raster;
    m_collide = sc.m_collide;
    m_rebuild = 1;
    return 0;
}

int VIRadialFloraSystem::Clear()
{
    m_scene = 0;
    m_raster = 0;
    m_collide = 0;

    for (int i = 0; i < 9; i++)
    {
        VIRadialFloraSwatch *s = m_swatches[i];
        if (s)
        {
            delete s;
            m_swatches[i] = 0;
        }
    }
    m_numSorted = 0;
    return 0;
}

void VIRadialFloraSystem::Reset()
{
    for (int i = 0; i < 9; i++)
    {
        VIRadialFloraSwatch *s = m_swatches[i];
        if (s)
        {
            delete s;
            m_swatches[i] = 0;
        }
    }
    m_numSorted = 0;
    m_rebuild = 1;
}

int VIRadialFloraSystem::ReleaseSwatch(int i)
{
    if (m_swatches[i])
        delete m_swatches[i];
    m_swatches[i] = 0;
    return 0;
}

// Rebuilds the 3x3 grid around the viewer's sub-cell.  When the location
// moved, the swatches that are still inside the grid slide to their new slot
// and the rest are released; then every empty slot whose cell is on the world
// grid gets a fresh swatch.  The sub-cell walk wraps at +/-25 into the next
// cell unless the world is a single row or column.
int VIRadialFloraSystem::Update(const VIVect3 &v)
{
    VIRadialFloraSwatch *grid[9];
    VIVect2I d;
    VIRadialFloraLoc loc;
    VIRadialFloraLoc l;
    int single;
    int i, r, c;

    if (m_scene->GetWorldGridNumRows() != 1 && m_scene->GetWorldGridNumCols() != 1)
        single = 0;
    else
        single = 1;

    CalcRadialLoc(v, loc);

    if (m_rebuild || loc.m_zone != m_loc.m_zone || loc.m_sub != m_loc.m_sub ||
        loc.m_cell != m_loc.m_cell)
    {
        m_rebuild = 0;
        for (i = 0; i < 9; i++)
            grid[i] = 0;

        d[0] = (m_loc.m_sub[0] - loc.m_sub[0]) + (m_loc.m_cell[0] - loc.m_cell[0]) * 50;
        d[1] = (m_loc.m_sub[1] - loc.m_sub[1]) + (m_loc.m_cell[1] - loc.m_cell[1]) * 50;
        int k = d[0] * 3 + d[1];

        i = 0;
        for (r = 0; r < 3; r++)
        {
            for (c = 0; c < 3; c++, i++)
            {
                VIRadialFloraSwatch *s = m_swatches[i];

                if (s)
                {
                    if (r + d[0] >= 0 && r + d[0] < 3 && c + d[1] >= 0 && c + d[1] < 3)
                    {
                        grid[k + i] = s;
                        m_swatches[i] = 0;
                    }
                    else
                        ReleaseSwatch(i);
                }
            }
        }

        for (i = 0; i < 9; i++)
            m_swatches[i] = grid[i];
    }

    i = 0;
    l.m_sub[0] = loc.m_sub[0] - 1;
    l.m_cell[0] = loc.m_cell[0];
    if (!single && l.m_sub[0] < -25)
    {
        l.m_cell[0]--;
        l.m_sub[0] += 50;
    }

    for (r = 0; r < 3; r++)
    {
        l.m_sub[1] = loc.m_sub[1] - 1;
        l.m_cell[1] = loc.m_cell[1];
        if (!single && l.m_sub[1] < -25)
        {
            l.m_cell[1]--;
            l.m_sub[1] += 50;
        }

        for (c = 0; c < 3; c++, i++)
        {
            if (m_swatches[i] == 0 &&
                l.m_cell[0] >= 0 && l.m_cell[0] < m_scene->GetWorldGridNumRows() &&
                l.m_cell[1] >= 0 && l.m_cell[1] < m_scene->GetWorldGridNumCols())
            {
                CalcZoneIndex(l);
                if (CreateSwatch(i, l) < 0)
                    return -1;
            }

            l.m_sub[1]++;
            if (!single && l.m_sub[1] >= 25)
            {
                l.m_sub[1] = -25;
                l.m_cell[1]++;
            }
        }

        l.m_sub[0]++;
        if (!single && l.m_sub[0] >= 25)
        {
            l.m_sub[0] = -25;
            l.m_cell[0]++;
        }
    }

    m_loc = loc;
    return 0;
}

// Draws every swatch the frustum keeps, collecting the instances near enough
// into m_sorted; then draws them nearest first, fading out past 10 units.
int VIRadialFloraSystem::Render(VICamera &cam)
{
    VIMatrix44 id;
    VIMatrix44 m;
    int i;

    m_numSorted = 0;
    id.Identity();
    for (i = 0; i < 9; i++)
        if (m_swatches[i] && !cam.m_frustum.Cull(m_swatches[i]->m_bbox, id))
            DrawSwatch(i, cam);

    qsort(m_sorted, m_numSorted, sizeof(VIRadialFloraSort), QSortCompare);

    for (i = m_numSorted - 1; i >= 0; i--)
    {
        VIArray<VIRadialFloraInst> &insts = m_swatches[m_sorted[i].m_a]->m_insts;

        m.Mul(insts[m_sorted[i].m_b].m_world, cam.m_view);
        m_raster->SetModelView(m);

        float a = 1.0f;
        if (10.0f < m_sorted[i].m_dist)
            a = 1.0f - (m_sorted[i].m_dist - 10.0f) * (1.0f / 30.0f);
        m_raster->DrawPrimBuffer(insts[m_sorted[i].m_b].m_prim,
                                 insts[m_sorted[i].m_b].m_pal, a, 3);
    }
    return 0;
}

// Queues every instance of swatch i within 40 units of the camera whose sphere
// survives the frustum, up to the 700 the sort buffer holds.
int VIRadialFloraSystem::DrawSwatch(int i, VICamera &cam)
{
    VIVect3 p;
    VIRadialFloraSwatch *s = m_swatches[i];

    for (int j = 0; j < s->m_insts.m_size && m_numSorted < 700; j++)
    {
        p.x = s->m_insts[j].m_world.m[3].x;
        p.y = s->m_insts[j].m_world.m[3].y;
        p.z = s->m_insts[j].m_world.m[3].z;
        // The camera position through a REFERENCE (its +4/+8 come off their own
        // address register, +0 folds into the camera's) into named floats: an
        // inline by-reference helper leaves y/z below the stores (34 words).
        const VIVect3 &c = cam.m_position;
        float cx = c.x, cy = c.y, cz = c.z;

        float d = (cx - p.x) * (cx - p.x) + (cy - p.y) * (cy - p.y) + (cz - p.z) * (cz - p.z);

        if (d < 1600.0f && !cam.m_frustum.Cull(s->m_insts[j].m_sphere))
        {
            m_sorted[m_numSorted].m_a = i;
            m_sorted[m_numSorted].m_b = j;
            m_sorted[m_numSorted].m_dist = sqrtf(d);
            m_numSorted++;
        }
    }
    return 0;
}

// One swatch: its location, a bounding box spanning the world's height over
// its 40-unit sub-cell, and its instances.
int VIRadialFloraSystem::CreateSwatch(int i, VIRadialFloraLoc &loc)
{
    VIVect3 c;
    VIBBox wb;
    VIRadialFloraSwatch *s = new VIRadialFloraSwatch;

    if (s == 0)
        return -1;
    m_swatches[i] = s;
    s->m_loc = loc;
    wb = *m_scene->GetWorldBBox();
    m_scene->GetWorldGridCenter(s->m_loc.m_cell.y, s->m_loc.m_cell.x, c);
    s->m_bbox.min.x = c.x + s->m_loc.m_sub.y * 40.0f;
    s->m_bbox.min.y = wb.min.y;
    s->m_bbox.min.z = c.z + s->m_loc.m_sub.x * 40.0f;
    s->m_bbox.max = s->m_bbox.min;
    s->m_bbox.max.y = wb.max.y;
    s->m_bbox.max.x += 40.0f;
    s->m_bbox.max.z += 40.0f;
    if (CreateFloraInsts(i) < 0)
        return -1;
    return 0;
}

// Scatters the swatch's instances over the ground under it.  Every triangle
// the scene query returns carries a flora set; the set's weight times the
// triangle's area says how many plants, a binary search over the set's
// cumulative dist weights picks each one's model, and a random point, heading
// and scale place it -- tilted to the surface when the dist's id has bit 0.
// Seeded off the sub-cell, so a swatch always grows the same plants.
int VIRadialFloraSystem::CreateFloraInsts(int i)
{
    VIVect3 pos, hpr;
    VIQueryTriList q;
    VIRadialFloraFilter f;
    VIVect3 e1, e2, c, n;
    VIArray<VIRadialFloraInst> insts;
    VIRadialFloraSwatch *s = m_swatches[i];
    int k, count, m, mid;

    srand(s->m_loc.m_sub.x * 100 + s->m_loc.m_sub.y);

    VIZoneRadialFlora &flora = m_scene->m_world.m_zones[s->m_loc.m_zone].m_flora;
    if (m_scene->m_world.m_zones[s->m_loc.m_zone].m_flora.m_sets.m_size == 0)
        return 0;

    if (insts.Init(500) < 0)
        return -1;
    count = 0;
    if (q.Init(100, s->m_bbox, VIVect3(0.0f, 1.0f, 0.0f), 30.0f) < 0)
        return -1;
    if (m_scene->QueryIntersection(m_swatches[i]->m_bbox, f, q) < 0)
        return -1;

    for (k = 0; k < q.m_numTris && count < 500; k++)
    {
        VIQueryTri &t = q.m_tris[k];
        int set = t.m_actor;

        if (set == -1)
            continue;

        e1 = t.m_v1;
        e1 -= t.m_v0;
        e2 = t.m_v2;
        e2 -= t.m_v0;
        c = Cross(e2, e1);
        n = c;
        n.Normalize();

        int num = (int)(Dot(c, n) * 0.5f * flora.m_sets[set].m_c);

        for (m = 0; m < num && count < 500; m++)
        {
            float r = VIRAND;
            int lo = flora.m_sets[set].m_model;
            int hi = lo + flora.m_sets[set].m_dist - 1;

            while (lo <= hi)
            {
                mid = (lo + hi) >> 1;
                if (r >= flora.m_dists[mid].m_p04 && r < flora.m_dists[mid].m_p08)
                    break;
                if (r >= flora.m_dists[mid].m_p08)
                    lo = mid + 1;
                else
                    hi = mid - 1;
            }

            int model = flora.m_dists[mid].m_set;

            insts[count].m_prim = flora.m_models[model].m_b;
            insts[count].m_pal = flora.m_models[model].m_c;

            float jitter = flora.m_dists[mid].m_p18 + 1.0f -
                           flora.m_dists[mid].m_p18 * VIRAND * 2.0f;
            float scale = flora.m_dists[mid].m_p14 * jitter;
            scale = VIMAX(0.0f, scale);

            hpr.x = 6.2831855f * VIRAND;
            if (flora.m_dists[mid].m_id & 1)
            {
                e1.HPR(hpr.x, 0.0f, 0.0f);
                Cross(e2, n, e1);
                e2.Normalize();
                Cross(e1, e2, n);
                hpr.y = -atan2f(e1.y, sqrtf(1.0f - e1.y * e1.y));
                hpr.z = atan2f(e2.y, sqrtf(1.0f - e2.y * e2.y));
            }
            else
            {
                hpr.y = 0.0f;
                hpr.z = 0.0f;
            }

            float u = VIRAND;
            float v = VIRAND;
            if (u + v > 1.0f)
            {
                u = 1.0f - u;
                v = 1.0f - v;
            }
            float w = 1.0f - u - v;

            // A second reference, not the outer one: the outer `t` is dead
            // across the inner loop and the target re-derives the address here.
            VIQueryTri &t = q.m_tris[k];
            Scale(pos, t.m_v0, w);
            AddVec(pos, t.m_v1, u);
            AddVec(pos, t.m_v2, v);

            insts[count].m_world.Transform(hpr, scale, pos);
            insts[count].m_world.Mul(flora.m_models[model].m_sphere.c, insts[count].m_sphere.c);
            insts[count].m_sphere.r = scale * flora.m_models[model].m_sphere.r;
            count++;
        }
    }

    if (s->m_insts.Init(count) < 0)
        return -1;

    for (k = 0; k < count; k++)
    {
        VIRadialFloraInst &dst = s->m_insts[k];
        VIRadialFloraInst &src = insts[k];
        sceVu0CopyMatrix(dst.m_world, src.m_world);
        dst.m_sphere = src.m_sphere;
        dst.m_prim = src.m_prim;
        dst.m_pal = src.m_pal;
    }

    srand((int)m_scene->GetTime());
    return 0;
}

// Which world-grid cell the point is in, and which 40-unit sub-cell of it --
// floored, clamped to [-25, 24] unless the world is a single row or column.
// A cell off the grid has no zone.
int VIRadialFloraSystem::CalcRadialLoc(VIVect3 p, VIRadialFloraLoc &loc)
{
    VIVect3 off;
    int col, row;

    m_scene->CalcWorldGridOffset(p, col, row, off);
    loc.m_cell[0] = row;
    loc.m_cell[1] = col;
    loc.m_sub[0] = (int)(off.z / 40.0f);
    if (off.z < 0.0f)
        loc.m_sub[0]--;
    loc.m_sub[1] = (int)(off.x / 40.0f);
    if (off.x < 0.0f)
        loc.m_sub[1]--;

    if (m_scene->GetWorldGridNumRows() != 1 && m_scene->GetWorldGridNumCols() != 1)
    {
        loc.m_sub[0] = VIMIN(24, VIMAX(-25, loc.m_sub[0]));
        loc.m_sub[1] = VIMIN(24, VIMAX(-25, loc.m_sub[1]));
    }

    if (col >= 0 && col < m_scene->GetWorldGridNumCols() &&
        row >= 0 && row < m_scene->GetWorldGridNumRows())
        CalcZoneIndex(loc);
    else
        loc.m_zone = -1;
    return 0;
}

int VIRadialFloraSystem::CalcZoneIndex(VIRadialFloraLoc &loc)
{
    VIVect3 v;
    int     n;

    m_scene->GetWorldGridCenter(loc.m_cell.y, loc.m_cell.x, v);
    m_scene->m_world.FindLeaf(v, loc.m_zone, n);
    return 0;
}

int VIRadialFloraSystem::QSortCompare(const void *a, const void *b)
{
    if (*(float *)a < *(float *)b)
        return -1;
    if (*(float *)a == *(float *)b)
        return 0;
    return 1;
}

// 0x0045F798 -- and it is the whole function.  Non-zero rejects, so the radial
// flora walk never collides an actor at all: it reaches rooms and their own
// geometry and nothing else.
int VIRadialFloraFilter::FilterActor(VIScene &scene, int actor)
{
    return 1;
}

// ---------------------------------------------------------------------------
// VIZoneRadialFlora -- 0x0045F7A0 .. 0x0045FAF0, the tail of the same TU.
// ---------------------------------------------------------------------------
int VIZoneRadialFlora::Init()
{
    if (Clear() < 0)
        return -1;
    return 0;
}

int VIZoneRadialFlora::Clear()
{
    if (m_models.Clear() < 0)
        return -1;
    if (m_dists.Clear() < 0)
        return -1;
    if (m_sets.Clear() < 0)
        return -1;
    return 0;
}

// Shares every flora sprite, caches each one's prim buffer, palette and
// bounding sphere, then turns each set's dist weights into a normalised
// cumulative [start, end) range -- the table CreateFloraInsts draws against.
int VIZoneRadialFlora::Precalc(VIScene &sc)
{
    int i;

    for (i = 0; i < m_models.m_size; i++)
        sc.ShareSprite(m_models[i].m_a);

    if (sc.m_raster)
    {
        for (i = 0; i < m_models.m_size; i++)
        {
            VIFloraSprite *s = (VIFloraSprite *)sc.Sprite(m_models[i].m_a);
            s->SetMaterialPalDither(0, *sc.m_raster);
            m_models[i].m_b = s->GetPrimBuffer();
            m_models[i].m_c = s->GetMaterialPal();
            m_models[i].m_sphere = s->m_bsphere;
        }
    }

    for (i = 0; i < m_sets.m_size; i++)
    {
        m_sets[i].m_c = 0.0f;
        for (int j = m_sets[i].m_model; j < m_sets[i].m_model + m_sets[i].m_dist; j++)
            m_sets[i].m_c += m_dists[j].m_p10;
    }

    for (i = 0; i < m_sets.m_size; i++)
    {
        float inv = 1.0f / m_sets[i].m_c;
        float acc = 0.0f;
        for (int j = m_sets[i].m_model; j < m_sets[i].m_model + m_sets[i].m_dist; j++)
        {
            m_dists[j].m_p04 = acc * inv;
            acc += m_dists[j].m_p10;
            m_dists[j].m_p08 = acc * inv;
        }
    }
    return 0;
}
