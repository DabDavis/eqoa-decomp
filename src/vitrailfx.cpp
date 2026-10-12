// VITrailFx -- SUPPORT 0x004812E0 .. 0x00481FE0.  Phase 33.
//
// The motion trail: a ring of SEGMENTs recycled between a free list and a used
// list, and above it a file-scope pool of 16 trails moving between three more
// lists -- unused, emitting, and ending.
#include "vi.h"

static int        sMaterial = -1;
static VITrailFx *sTrails;
static VILList    sUnusedList;
static VILList    sActiveList;
static VILList    sEndingList;

int VITrailFxInit(VIRaster &r, VIDictionary &d)
{
    int i;

    sMaterial = r.CreateMaterial(0);
    if (sMaterial == -1)
        return -1;

    VIMaterial *m = r.Material(sMaterial);
    m->Init(1);
    m->SetLayerFillType(0, VIMaterialFillTypeTexture);
    m->SetLayerModulate(0, 1);
    m->SetDither(0);
    m->SetLayerBlendMode(0, VIAlphaBlendMode1);
    m->SetZWrite(0);
    m->SetZTest(1);

    sUnusedList.Init();
    sActiveList.Init();
    sEndingList.Init();

    sTrails = new VITrailFx[16];

    for (i = 0; i < 16; i++)
        sUnusedList.AddTail(&sTrails[i]);

    return 0;
}

int VITrailFxClear(VIRaster &r, VIDictionary &d)
{
    delete [] sTrails;

    if (sMaterial != -1) {
        r.ReleaseMaterial(sMaterial, d);
        sMaterial = -1;
    }
    return 0;
}

VITrailFx *VITrailFxBegin(long now, int numSegments, float life)
{
    VITrailFx *t = (VITrailFx *)sUnusedList.RemHead();

    if (t) {
        t->Init(now, numSegments, life);
        sActiveList.AddTail(t);
    }
    return t;
}

void VITrailFxEnd(VITrailFx *t)
{
    t->Remove();
    sEndingList.AddTail(t);
}

void VITrailFxRender(long now, VIRaster &r, VICamera &c)
{
    VITrailFx *t;

    t = (VITrailFx *)sActiveList.m_head;
    while (t->m_next) {
        if (t->Update(now) > 0)
            t->Render(r, c);
        t = (VITrailFx *)t->m_next;
    }

    t = (VITrailFx *)sEndingList.m_head;
    while (t->m_next) {
        VITrailFx *n = (VITrailFx *)t->m_next;
        if (t->Update(now) <= 0) {
            t->Remove();
            sUnusedList.AddTail(t);
        } else {
            t->Render(r, c);
        }
        t = n;
    }
}

VITrailFx::VITrailFx()
{
    m_segments = 0;
}

VITrailFx::~VITrailFx()
{
    Clear();
}

int VITrailFx::Init(long now, int numSegments, float life)
{
    int i;

    Clear();
    m_lastTime = now;
    m_life = life;
    m_segments = new SEGMENT[numSegments];

    for (i = 0; i < numSegments; i++)
        m_free.AddTail(&m_segments[i]);

    return 0;
}

int VITrailFx::Clear()
{
    if (m_segments) {
        delete [] m_segments;
        m_segments = 0;
    }
    m_used.Init();
    m_free.Init();
    return 0;
}

int VITrailFx::AddSegment(const VIVect3 &a, const VIVect3 &b, const VIColor32F &c)
{
    SEGMENT *s;

    while ((s = (SEGMENT *)m_free.RemHead()) == 0)
        ClearSegmentsForward((SEGMENT *)m_used.m_tailPred);

    m_used.AddHead(s);
    s->m_pos[0] = a;
    s->m_pos[1] = b;
    s->m_color[0] = c;
    s->m_color[1] = c;
    s->m_age = 0.0f;
    return 0;
}

int VITrailFx::AddSegment(const VIVect3 &a, const VIVect3 &b, const VIColor32F &ca,
                          const VIColor32F &cb)
{
    SEGMENT *s;

    while ((s = (SEGMENT *)m_free.RemHead()) == 0)
        ClearSegmentsForward((SEGMENT *)m_used.m_tailPred);

    m_used.AddHead(s);
    s->m_pos[0] = a;
    s->m_pos[1] = b;
    s->m_color[0] = ca;
    s->m_color[1] = cb;
    s->m_age = 0.0f;
    return 0;
}

int VITrailFx::Update(long now)
{
    float dt = (int)(now - m_lastTime) * 0.001f;
    SEGMENT *s = (SEGMENT *)m_used.m_head;

    m_lastTime = now;

    while (s->m_next) {
        s->m_age += dt;
        if (m_life <= s->m_age) {
            SEGMENT *p = (SEGMENT *)s->m_prev;
            if (p->m_prev) {
                if (m_life <= p->m_age) {
                    ClearSegmentsForward(s);
                    break;
                }
            } else {
                ClearSegmentsForward(s);
                break;
            }
        }
        s = (SEGMENT *)s->m_next;
    }
    return m_used.m_head->m_next != 0;
}

int VITrailFx::Render(VIRaster &r, VICamera &c)
{
    SEGMENT *s;
    int n;

    if (m_used.Length() < 2)
        return 0;

    r.SetModelView(c.m_view);

    n = 0;
    r.SetMaterial(sMaterial);
    r.BeginTriStrip();

    s = (SEGMENT *)m_used.m_head;
    while (s->m_next) {
        s->m_color[0].a = 1.0f - s->m_age / m_life;
        s->m_color[0].a = VIMIN(VIMAX(0.0f, s->m_color[0].a), 1.0f);
        s->m_color[1].a = s->m_color[0].a;

        r.Color(s->m_color[0]);
        r.Vertex(s->m_pos[0]);
        r.Color(s->m_color[1]);
        r.Vertex(s->m_pos[1]);
        n += 2;

        if (n == 24) {
            r.EndTriStrip();
            n = 0;
            if (s->m_next->m_next) {
                r.BeginTriStrip();
                n = 2;
                r.Color(s->m_color[0]);
                r.Vertex(s->m_pos[0]);
                r.Color(s->m_color[1]);
                r.Vertex(s->m_pos[1]);
            }
        }
        s = (SEGMENT *)s->m_next;
    }
    if (n)
        r.EndTriStrip();

    n = 0;
    r.BeginTriStrip();

    s = (SEGMENT *)m_used.m_head;
    while (s->m_next) {
        s->m_color[0].a = 1.0f - s->m_age / m_life;
        s->m_color[0].a = VIMIN(VIMAX(0.0f, s->m_color[0].a), 1.0f);
        s->m_color[1].a = s->m_color[0].a;

        r.Color(s->m_color[1]);
        r.Vertex(s->m_pos[1]);
        r.Color(s->m_color[0]);
        r.Vertex(s->m_pos[0]);
        n += 2;

        if (n == 24) {
            r.EndTriStrip();
            n = 0;
            if (s->m_next->m_next) {
                r.BeginTriStrip();
                n = 2;
                r.Color(s->m_color[1]);
                r.Vertex(s->m_pos[1]);
                r.Color(s->m_color[0]);
                r.Vertex(s->m_pos[0]);
            }
        }
        s = (SEGMENT *)s->m_next;
    }
    if (n)
        r.EndTriStrip();

    return 0;
}

void VITrailFx::ClearSegmentsForward(SEGMENT *s)
{
    SEGMENT *n = s;

    do {
        SEGMENT *next = (SEGMENT *)s->m_next;
        n->Remove();
        m_free.AddTail(n);
        n = next;
    } while (n->m_next);
}
