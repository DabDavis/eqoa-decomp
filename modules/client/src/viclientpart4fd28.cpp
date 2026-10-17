#line 1 "C:/code-frontiers-gm/game/VIClientPart4FD28.cpp"
// VIClientPart4FD28 (name provisional) -- CLIENT 0x00BE8DF0 .. 0x00BE9C9C: the floating
// damage numbers at VIClient+0x4FD28, then this TU's template copies (0x00BE9CA0 ..
// 0x00BEA66C, verified duplicates).  The TU before it is VICSClient's (shared with STATION).
#include "game.h"

extern "C" int sprintf(char *, const char *, ...);
extern "C" int strlen(const char *);
extern "C" int rand();
extern "C" float sinf(float);

// The vector difference each engine TU defines for itself (viclient.cpp has one too).
static inline VIVect3 operator-(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x - b.x, a.y - b.y, a.z - b.z);
}
static inline VIVect2 operator+(const VIVect2 &a, const VIVect2 &b)
{
    return VIVect2(a.x + b.x, a.y + b.y);
}
static inline VIVect2 operator-(const VIVect2 &a, const VIVect2 &b)
{
    return VIVect2(a.x - b.x, a.y - b.y);
}

// 0x00BE8DF0
VIClientPart4FD28::VIClientPart4FD28()
{
    m_material = -1;
    m_scene = 0;
}

// 0x00BE8E88
VIClientPart4FD28::~VIClientPart4FD28()
{
}

// 0x00BE8EE0
int VIClientPart4FD28::Init(VIScene &sc, VIEntity **ents, int n)
{
    if (!&sc)
        return -1;
    m_ents = ents;
    m_n = n;
    m_scene = &sc;
    m_slot[0] = 0;
    m_slot[1] = 0;
    VIDictionary *d = sc.m_dict;
    VIRaster *r = sc.m_raster;
    VIResourceType type;
    int tex;
    if (!d->Find(VIHashResourceID("damage_font.tga"), type, tex))
        return -1;
    if ((m_material = r->CreateMaterial(0)) < 0)
        return -1;
    VIMaterial *mat = r->Material(m_material);
    mat->Init(1);
    mat->SetLayerFillType(0, (VIMaterialFillType)0);
    mat->SetLayerModulate(0, 1);
    mat->SetLayerBlendMode(0, (VIAlphaBlendMode)3);
    mat->SetZWrite(0);
    mat->SetZTest(0);
    mat->SetDither(0);
    mat->SetLayerTexture(0, tex, *r, *d);
    m_active.Init();
    m_free.Init();
    for (int i = 0; i < 32; i++)
        m_free.AddTail(&m_numbers[i]);
    return 0;
}

// 0x00BE9068
void VIClientPart4FD28::Release()
{
    if (m_scene)
    {
        VIRaster *r = m_scene->m_raster;
        VIDictionary *d = m_scene->m_dict;
        if (m_material >= 0)
        {
            r->ReleaseMaterial(m_material, *d);
            m_material = -1;
        }
    }
}

// 0x00BE90B8
int VIClientPart4FD28::AddDamage(int defender, int amount, int z)
{
    if (!m_scene)
        return -1;
    VIDamageNumber *n = (VIDamageNumber *)m_free.RemHead();
    if (n)
    {
        n->m_defender = defender;
        n->m_amount = amount;
        n->m_offset = VIVect3();
        n->m_time = 0.0f;
        n->m_slot = m_slot[z];
        n->m_z = z;
        m_active.AddTail(n);
        m_slot[z] = (m_slot[z] + 1) % 3;
    }
    return 0;
}

// 0x00BE9198
int VIClientPart4FD28::Process(VICamera &cam, float dt)
{
    if (!m_scene)
        return -1;
    VIRaster *r = m_scene->m_raster;
    r->SetMaterial(m_material);
    VIDamageNumber *n = (VIDamageNumber *)m_active.m_head;
    while (n->m_next)
    {
        VIDamageNumber *next = (VIDamageNumber *)n->m_next;
        VIEntity *e = FindEntity(n->m_defender);
        if (!e || e->m_actor == -1 || n->m_time > 1.0f)
        {
            n->Remove();
            m_free.AddTail(n);
            n = next;
            continue;
        }
        if (n->m_time == 0.0f)
        {
            VIVect3 off;
            VIEntity *t = FindEntity(n->m_defender);
            if (!t || t->m_actor == -1)
                return -1;
            VISprite *spr = m_scene->GetActorSpritePtr(t->m_actor);
            if (!spr)
                return -1;
            if (spr->m_type == 5)
            {
                VIMatrix44 m;
                ((VICSprite *)spr)->CalcSlotWorldTransform(VICSpriteNodeID4, t->m_actor, *m_scene, m);
                off = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z) - t->GetActorPos();
            }
            else
                off = VIVect3();
            n->m_offset = off;
        }
        n->m_time += dt * 1.1f;
        float bounce[3] = { 0.8f, 1.0f, 1.25f };
        VIVect3 pos = e->GetActorPos();
        pos += n->m_offset;
        pos.y += sinf(n->m_time * 3.1415f * 0.7f) * bounce[n->m_slot];
        if (!cam.m_frustum.Cull(pos, 0.0f))
        {
        VIVect2 sc;
        VIVect4 h = VIVect4(pos.x, pos.y, pos.z, 1.0f) * cam.m_viewProj;
        float w = 1.0f / h.w;
        float sw = (float)r->m_screenW;
        float sh = (float)r->m_screenH;
        VIColor32F col;
        sc.x = sw * 0.5f + h.x * w * sw * 0.5f;
        sc.y = sh * 0.5f - h.y * w * sh * 0.5f;
        if (n->m_amount > 0)
            col = VIColor32F(0.0f, 1.0f, 0.0f, 1.0f);
        else if (n->m_z)
            col = VIColor32F(0.8f, 0.8f, 0.1f, 1.0f);
        else
            col = VIColor32F(1.0f, 1.0f, 1.0f, 1.0f);
        Draw(sc, n->m_amount, col, n->m_time);
        }
        n = next;
    }
    return 0;
}

// 0x00BE96A8
void VIClientPart4FD28::Draw(const VIVect2 &pos, int amount, const VIColor32F &col, float t)
{
    VIRaster *r = m_scene->m_raster;
    char buf[32];
    VIVect2 uv[4];
    float a;
    if (t < 0.2f)
        a = t / 0.2f;
    else if (t > 0.5f)
        a = 1.0f - (t - 0.5f) * 2.0f;
    else
        a = 1.0f;
    float shake = t < 0.6f ? 1.0f - t / 0.6f : 0.0f;
    a = VIMIN(VIMAX(0.0f, a), 1.0f);
    sprintf(buf, "%d", amount < 0 ? -amount : amount);
    int len = strlen(buf);
    float scale = t * 1.2f + 0.9f;
    VIVect2 p;   // declared, then ASSIGNED: p's slot comes before the call's two temps
    p = pos - VIVect2(len * 10.0f * 0.5f * scale + scale, scale * 13.0f);
    r->Begin2D();
    r->Color(VIColor32F(col.r, col.g, col.b, a));
    for (int i = 0; i < len; i++)
    {
        GlyphUV(buf[i] - '0', uv);
        r->BeginTriFan();
        r->UV(uv[0]);
        r->Vertex(p + VIVect2((float)((rand() & 0xFF) - 128) * (1.0f / 128) * 10.0f * shake, (float)((rand() & 0xFF) - 128) * (1.0f / 128) * 10.0f * shake));
        r->UV(uv[1]);
        r->Vertex(p + VIVect2(scale * 12.0f + (float)((rand() & 0xFF) - 128) * (1.0f / 128) * 10.0f * shake, (float)((rand() & 0xFF) - 128) * (1.0f / 128) * 10.0f * shake));
        r->UV(uv[2]);
        r->Vertex(p + VIVect2(scale * 12.0f + (float)((rand() & 0xFF) - 128) * (1.0f / 128) * 10.0f * shake, scale * 24.0f + (float)((rand() & 0xFF) - 128) * (1.0f / 128) * 10.0f * shake));
        r->UV(uv[3]);
        r->Vertex(p + VIVect2((float)((rand() & 0xFF) - 128) * (1.0f / 128) * 10.0f * shake, scale * 24.0f + (float)((rand() & 0xFF) - 128) * (1.0f / 128) * 10.0f * shake));
        r->EndTriFan();
        p.x += scale * 10.0f;
    }
    r->End2D();
}

// 0x00BE9BD8
void VIClientPart4FD28::GlyphUV(int digit, VIVect2 *uv)
{
    float v = (float)(digit >> 2) * 0.25f;
    float u = (float)(digit % 4) * 0.25f;
    uv[0].x = u;
    uv[0].y = v;
    uv[1].x = u + 0.25f;
    uv[1].y = v;
    uv[2].x = u + 0.25f;
    uv[2].y = v + 0.25f;
    uv[3].x = u;
    uv[3].y = v + 0.25f;
}

// 0x00BE9C50
VIEntity *VIClientPart4FD28::FindEntity(int id)
{
    VIEntity *found = 0;
    for (int i = 0; i < m_n; i++)
        if (m_ents[i] && m_ents[i]->m_id == id)
        {
            found = m_ents[i];
            break;
        }
    return found;
}
