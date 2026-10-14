// VICSprite -- SUPPORT 0x00420F20 .. 0x004283C8.  The character sprite: a
// VIHSprite with a customisation layer bolted on top of it.  Sixty-six
// animation slots, nineteen texture slots, six attach points, a capsule to be
// collided against, a weapon trail, one sound and an array of node-bound
// particle emitters.
//
// It is the biggest object in the corpus at 4,624 bytes, and the class is what
// this phase is really about: every constant in the constructor is a member's
// initial value, and the constructor's vec-init loops are what say which runs
// of fields are ARRAYS with an element constructor and which are members with
// constructors of their own.
#include "vi.h"

extern "C" int  rand();
extern "C" void *memcpy(void *d, const void *s, unsigned int n);
extern "C" unsigned int VIHashRefID(const char *);
extern "C" unsigned int VIHashResourceID(const char *);

// The node ref ids the animation triggers carry, hashed at static
// initialisation (phase 61).  `_GLOBAL_$I$` takes the FIRST one's name.
const int VICSprite::PrimSoundRefID    = VIHashRefID("Primary Sound");
const int VICSprite::PrimSoundBPURefID = VIHashRefID("Primary Sound_BPU");
const int VICSprite::PrimSoundBPLRefID = VIHashRefID("Primary Sound_BPL");
const int VICSprite::SecSoundRefID     = VIHashRefID("Secondary Sound");
const int VICSprite::SecSoundBPURefID  = VIHashRefID("Secondary Sound_BPU");
const int VICSprite::SecSoundBPLRefID  = VIHashRefID("Secondary Sound_BPL");
const int VICSprite::Particle1RefID    = VIHashRefID("Particle 1");
const int VICSprite::Particle1BPURefID = VIHashRefID("Particle 1_BPU");
const int VICSprite::Particle1BPLRefID = VIHashRefID("Particle 1_BPL");
const int VICSprite::Particle2RefID    = VIHashRefID("Particle 2");
const int VICSprite::Particle2BPURefID = VIHashRefID("Particle 2_BPU");
const int VICSprite::Particle2BPLRefID = VIHashRefID("Particle 2_BPL");
int VICSprite::FontResourceID = VIHashResourceID("Arial");
VIColor32 VICSprite::NameColors[] =
{
    VIColor32(212, 192, 111, 255),
    VIColor32(100, 175, 220, 255),
    VIColor32(90, 255, 80, 255)
};

VICSprite::VICSprite() : m_emitterTime(0)
{
    int i;

    m_type = 5;
    m_flags = 1;
    m_skelType = 0;
    m_race = VICSpriteRaceNone;
    m_sex = 0;
    m_materialPal = -1;
    m_refMap = -1;
    m_dead = 0;
    m_actionSeq = 0;
    m_action = -1;
    m_cur[0].m_anim = 0;
    m_cur[0].m_play = -1;
    m_cur[0].m_sync = 0;
    m_cur[0].m_flags = -1;
    m_prev[0] = m_cur[0];
    m_cur[1] = m_cur[0];
    m_prev[1] = m_prev[0];
    m_trailSlot = 1;
    m_animTime = 0;
    m_idleTime = 0;
    m_trailColor = VIColor32F(0.25f, 0.25f, 0.25f, 1.0f);
    m_deathAnim = -1;
    m_trailTime = 0;
    m_trail = 0;
    // Phase 61: ASCENDING, all three -- gcc's check_dbra_loop reverses a
    // one-store loop, so the pointer descends either way; what separates the
    // two sources is the pre-header, where a reversed loop's NEW counter is
    // set after the hoisted -1 and a written-descending one's before it.
    for (i = 0; i < 1; i++)
        m_body[i] = -1;
    for (i = 0; i < 3; i++)
    {
        m_items[i].m_node = -1;
        m_items[i].m_attackType = 11;
        m_items[i].m_projectile = -1;
    }
    for (i = 0; i < 9; i++)
        m_node[i] = -1;
    for (i = 0; i < 6; i++)
        m_attach[i] = -1;
}

VICSprite::~VICSprite()
{
    Clear();
}

int VICSprite::Clear()
{
    int i;

    if (VIHSprite::Clear() < 0)
        return -1;
    m_type = 5;
    m_flags = 1;
    m_materialPal = -1;
    m_refMap = -1;
    m_dead = 0;
    m_actionSeq = 0;
    m_action = -1;
    m_cur[0].m_anim = 0;
    m_cur[0].m_play = -1;
    m_cur[0].m_sync = 0;
    m_cur[0].m_flags = -1;
    m_prev[0] = m_cur[0];
    m_cur[1] = m_cur[0];
    m_prev[1] = m_prev[0];
    m_animTime = 0;
    m_idleTime = 0;
    m_trailSlot = 0;
    m_trailColor = VIColor32F(0.05f, 0.05f, 0.3f, 1.0f);
    m_deathAnim = -1;
    m_trailTime = 0;
    m_trail = 0;
    for (i = 0; i < 1; i++)
        m_body[i] = -1;
    for (i = 0; i < 3; i++)
    {
        m_items[i].m_node = -1;
        m_items[i].m_attackType = 11;
        m_items[i].m_projectile = -1;
    }
    for (i = 0; i < 9; i++)
        m_node[i] = -1;
    for (i = 0; i < 6; i++)
        m_attach[i] = -1;
    return 0;
}

int VICSprite::Init(VICSpriteSkelType skel, VICSpriteRace race, VICSpriteSex sex,
                    float scale, int npc)
{
    if (Clear() < 0)
        return -1;
    if (VIHSprite::Init() < 0)
        return -1;
    if (npc)
        m_flags |= 1;
    else
        m_flags &= ~1u;
    m_defaultScale = scale;
    m_skelType = skel;
    m_race = race;
    m_sex = sex;
    return 0;
}

int VICSprite::GetSkeletonType()
{
    return m_skelType;
}

int VICSprite::GetRace()
{
    return m_race;
}

int VICSprite::GetSex()
{
    return m_sex;
}

int VICSprite::IsDead()
{
    return m_dead;
}

int VICSprite::GetInstType(VIScene &sc) const
{
    return 1;
}

int VICSprite::Collide(VICollRay &r, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    return 0;
}

int VICSprite::QueryIntersection(const VIBBox &b, VIQueryTriList &l,
                                 const VIMatrix44 &m, VIScene &sc)
{
    return 0;
}

int VICSprite::Collide(VICollSphere &s, VISceneFilter &f, VIScene &sc, VICollide &c)
{
    if (m_dead)
        return 0;
    return c.CollideObj(s, m_capsule);
}

int VICSprite::GetItemAttackType(VICSpriteAttachSlot slot)
{
    return m_items[slot].m_attackType;
}

int VICSprite::GetItemProjectile(VICSpriteAttachSlot slot)
{
    return m_items[slot].m_projectile;
}

int VICSprite::ItemHasProjectile(VICSpriteAttachSlot slot)
{
    return m_items[slot].m_projectile != -1;
}

int VICSprite::SetIsPlayer(int on)
{
    if (on)
        m_flags |= 4;
    else
        m_flags &= ~4u;
    return 0;
}

int VICSprite::SetWeaponTrail(int i, VIColor32F &c)
{
    m_trailSlot = i;
    m_trailColor = c;
    return 0;
}

int VICSprite::IsActionComplete(int i)
{
    // Phase 61: the stopped test NESTED under the play test, one `return 0`
    // at the bottom -- a flat chain if-converts the last pair to `sltu`.
    if (m_action != i)
        return 1;
    if (m_cur[0].m_play != -1)
    {
        if (IsPlaybackStopped(m_cur[0].m_play))
            return 1;
    }
    return 0;
}

int VICSprite::SetAction(VICSpriteAnimID id, VIScene &sc)
{
    int handle;

    if (m_skelType == 1)
    {
        if (SetAnimation(id, 0, sc))
            m_action = m_actionSeq;
        if (SetAnimation(id, 1, sc) < 0)
            return -1;
    }
    else
    {
        if (SetAnimation(id, 0, sc))
            m_action = m_actionSeq;
    }
    handle = m_actionSeq;
    m_actionSeq = handle + 1;
    return handle;
}

int VICSprite::SetLocomotion(VICSpriteAnimID id, VIScene &sc)
{
    if (m_skelType == 1)
    {
        if (SetAnimation(id, 0, sc))
            m_action = m_actionSeq;
        if (SetAnimation(id, 1, sc) < 0)
            return -1;
    }
    else
    {
        if (SetAnimation(id, 0, sc))
            m_action = m_actionSeq;
    }
    m_actionSeq++;
    return 0;
}

// Phase 61: the TAIL's stores go through a do/while(0) macro, the two arms'
// do not.  Each do/while is a pair of LOOP notes, and haifa treats a loop note
// as a scheduling barrier: without them sched2 issues the return value's
// `move v0,zero` in the tail's first cycle (35 words wrong) and swaps the last
// two stores; with them the tail keeps source order and the move comes last,
// so reorg puts it in the jr slot.  The arms must NOT have them -- the else
// arm's last store is scheduled first, into the beql's slot (19 words).
#define VICSPRITE_CHANNEL(i, c) do { m_entries[i].m_channel = (c); } while (0)

int VICSprite::SetAnimSoundChannels()
{
    if (m_skelType != 1)
    {
        m_entries[0].m_channel = 0;
        m_entries[1].m_channel = 0;
        m_entries[2].m_channel = 0;
        m_entries[3].m_channel = 0;
        m_entries[4].m_channel = 0;
        m_entries[5].m_channel = 0;
        m_entries[6].m_channel = 0;
        m_entries[7].m_channel = 0;
        m_entries[8].m_channel = 0;
        m_entries[9].m_channel = 0;
        m_entries[10].m_channel = 0;
        m_entries[11].m_channel = 0;
        m_entries[12].m_channel = 0;
        m_entries[13].m_channel = 0;
        m_entries[14].m_channel = 0;
        m_entries[15].m_channel = 0;
        m_entries[16].m_channel = 0;
        m_entries[17].m_channel = 0;
        m_entries[18].m_channel = 0;
    }
    else
    {
        m_entries[0].m_channel = 1;
        m_entries[1].m_channel = 1;
        m_entries[2].m_channel = 1;
        m_entries[3].m_channel = 1;
        m_entries[4].m_channel = 1;
        m_entries[5].m_channel = 1;
        m_entries[6].m_channel = 1;
        m_entries[7].m_channel = 1;
        m_entries[8].m_channel = 1;
        m_entries[9].m_channel = 1;
        m_entries[10].m_channel = 1;
        m_entries[11].m_channel = 1;
        m_entries[12].m_channel = 1;
        m_entries[13].m_channel = 1;
        m_entries[14].m_channel = 1;
        m_entries[15].m_channel = 1;
        m_entries[16].m_channel = 1;
        m_entries[17].m_channel = 1;
        m_entries[18].m_channel = 1;
    }
    VICSPRITE_CHANNEL(22, 0);
    VICSPRITE_CHANNEL(23, 0);
    VICSPRITE_CHANNEL(24, 0);
    VICSPRITE_CHANNEL(25, 0);
    VICSPRITE_CHANNEL(26, 0);
    VICSPRITE_CHANNEL(27, 0);
    VICSPRITE_CHANNEL(28, 0);
    VICSPRITE_CHANNEL(29, 0);
    VICSPRITE_CHANNEL(30, 0);
    VICSPRITE_CHANNEL(31, 0);
    VICSPRITE_CHANNEL(32, 0);
    VICSPRITE_CHANNEL(33, 0);
    VICSPRITE_CHANNEL(34, 0);
    VICSPRITE_CHANNEL(35, 0);
    VICSPRITE_CHANNEL(36, 0);
    VICSPRITE_CHANNEL(64, 0);
    VICSPRITE_CHANNEL(37, 0);
    VICSPRITE_CHANNEL(38, 0);
    VICSPRITE_CHANNEL(39, 0);
    VICSPRITE_CHANNEL(40, 0);
    VICSPRITE_CHANNEL(41, 0);
    VICSPRITE_CHANNEL(42, 0);
    VICSPRITE_CHANNEL(43, 0);
    VICSPRITE_CHANNEL(44, 0);
    VICSPRITE_CHANNEL(45, 0);
    VICSPRITE_CHANNEL(46, 0);
    VICSPRITE_CHANNEL(50, 0);
    VICSPRITE_CHANNEL(51, 0);
    VICSPRITE_CHANNEL(52, 0);
    VICSPRITE_CHANNEL(53, 0);
    VICSPRITE_CHANNEL(56, 0);
    VICSPRITE_CHANNEL(57, 0);
    VICSPRITE_CHANNEL(59, 0);
    VICSPRITE_CHANNEL(60, 0);
    VICSPRITE_CHANNEL(61, 0);
    VICSPRITE_CHANNEL(63, 0);
    return 0;
}

int VICSprite::CalcLODOffDist(VIScene &sc)
{
    if (m_body[0] != -1)
    {
        if (sc.GetSpriteType(m_body[0]) == 7 || sc.GetSpriteType(m_body[0]) == 3)
            m_offDist = sc.Sprite(m_body[0])->m_offDist;
    }
    return 0;
}

int VICSprite::InitTextSlots(VIRaster &r)
{
    int i;

    for (i = 0; i < 19; i++)
    {
        if (m_slots[i].m_palEntry == -1)
            continue;
        m_slots[i].m_texture =
            r.GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
                ->GetLayerTexture(m_slots[i].m_layer);
    }
    return 0;
}

int VICSprite::SetArmorSet(VICSpriteArmorSet set, VICSpriteTint tint, VIScene &sc)
{
    int i;

    for (i = 0; i < 6; i++)
        SetArmorSlot((VICSpriteArmorSlot)i, set, tint, sc);
    return 0;
}

int VICSprite::DetachItem(VICSpriteAttachSlot slot, VIScene &sc)
{
    if (m_attach[slot] != -1)
    {
        if (Detach(m_attach[slot], sc) < 0)
            return -1;
        m_attach[slot] = -1;
    }
    if (m_items[slot].m_projectile != -1)
    {
        sc.ReleaseSprite(m_items[slot].m_projectile);
        m_items[slot].m_projectile = -1;
    }
    m_items[slot].m_attackType = 11;
    return 0;
}

int VICSprite::SetSkinsVisible(int on, VIScene &sc)
{
    int i;

    if (on)
        m_flags |= 2;
    else
        m_flags &= ~2u;
    for (i = 0; i < 1; i++)
    {
        int sprite;

        if (m_attach[VICSpriteAttachSlotSkin + i] == -1)
            continue;
        sprite = GetAttachSprite(m_attach[VICSpriteAttachSlotSkin + i]);
        switch (sc.GetSpriteType(sprite))
        {
        case 3:
            ((VISkinSprite *)sc.Sprite(sprite))->SetVisible(on);
            break;
        case 7:
            ((VILODSprite *)sc.Sprite(sprite))->SetVisible(on);
            break;
        }
    }
    return 0;
}

int VICSprite::CreateParticleEmitters(VIScene &sc)
{
    VIParticleSystem *ps = sc.ParticleSystem();
    int i;

    for (i = 0; i < m_emitters.m_size; i++)
    {
        VIParticleDefinition *d;

        if (m_emitters[i].m_definition == -1)
            continue;
        d = ps->ParticleDefinition(m_emitters[i].m_definition);
        if (m_emitters[i].m_emitter.Create(ps, d) < 0)
            return -1;
    }
    return 0;
}

int VICSprite::CalcSlotWorldTransform(VICSpriteNodeID id, int actor, VIScene &sc,
                                      VIMatrix44 &m)
{
    VIMatrix44 t;

    if (m_node[id] != -1)
    {
        sc.GetActorTransform(actor, t);
        CalcNodeTransform(m_node[id], t, sc, m);
    }
    else
    {
        sc.GetActorTransform(actor, m);
    }
    return 0;
}

int VICSprite::Raster(VICamera &cam, VIScene &sc, VIRaster &r, unsigned int flags)
{
    VIMatrix44 m;
    int rc;

    sceVu0CopyMatrix(m, *r.GetModelView());
    rc = VIHSprite::Raster(cam, sc, r, flags);
    ProcessWeaponTrails(cam, sc, m);
    UpdateParticleEmitters(cam, sc, r, m);
    return rc;
}

int VICSprite::SetFace(int face, VIScene &sc)
{
    VIRaster *r = sc.m_raster;
    VIMaterial *mat;
    int tex;

    if (!r)
        return -1;
    if (m_slots[7].m_palEntry == -1)
        return 0;
    mat = r->GetMaterialPalEntry(m_materialPal, m_slots[7].m_palEntry);
    if (mat)
    {
        tex = VICSpriteCust::GetFaceTexture(face, m_sex, (VICSpriteRace)m_race, *sc.m_dict);
        if (tex >= 0)
            mat->SetLayerTexture(m_slots[7].m_layer, tex, *r, *sc.m_dict);
    }
    return 0;
}

int VICSprite::SetItemAction(int layer, VIScene &sc)
{
    int anim = m_cur[layer].m_anim;
    int attach = -1;
    int sprite;
    int rc;
    VIHSprite *hs;

    if (anim == 25 || anim == 28 || anim == 35 || anim == 41)
        attach = m_attach[0];
    else if (anim == 24 || anim == 27 || anim == 32 || anim == 40 || anim == 31 ||
             anim == 33)
        attach = m_attach[1];
    // Phase 61: the tail NESTED under the sprite-type test.  Flat, the
    // `attach == -1` compare's -1 reaches the last test through one extended
    // basic block, cse hands it to the final `return -1`, and jump.c
    // if-converts it to a movz off that register.  The join breaks the chain.
    if (attach == -1)
        return 0;
    sprite = GetAttachSprite(attach);
    if (sc.GetSpriteType(sprite) == 4)
    {
        hs = (VIHSprite *)sc.Sprite(sprite);
        if (hs->EraseAllPlay(sc) < 0)
            return -1;
        rc = hs->AddPlay(hs->m_defaultAnim, VIPlaybackTypeOnce, m_entries[anim].m_weight, sc);
        if (rc < 0)
            return -1;
    }
    return 0;
}

int VICSprite::SetAttackAction(VICSpriteAttachSlot slot, VIScene &sc)
{
    int id;

    if ((unsigned int)slot >= 2)
        return -1;
    if (m_items[0].m_attackType == 2 || m_items[0].m_attackType == 4 ||
        m_items[0].m_attackType == 6)
    {
        switch (m_items[0].m_attackType)
        {
        case 2:
            id = 26;
            break;
        case 4:
            id = 42;
            break;
        case 6:
            id = 29;
            break;
        default:
            return -1;
        }
    }
    else
    {
        switch (m_items[slot].m_attackType)
        {
        case 11:
            id = slot == 0 ? 23 : 22;
            break;
        case 1:
            id = slot == 0 ? 25 : 24;
            break;
        case 3:
            id = slot == 0 ? 41 : 40;
            break;
        case 5:
            id = slot == 0 ? 28 : 27;
            break;
        case 7:
            id = 31;
            break;
        case 9:
            id = 33;
            break;
        case 8:
            id = slot == 0 ? 35 : 32;
            break;
        case 10:
            id = 36;
            break;
        default:
            return -1;
        }
    }
    return SetAction((VICSpriteAnimID)id, sc);
}

int VICSprite::SetAttackAltAction(VICSpriteAttachSlot slot, VIScene &sc)
{
    int id;

    if ((unsigned int)slot >= 2)
        return -1;
    if (m_items[0].m_attackType == 2 || m_items[0].m_attackType == 4 ||
        m_items[0].m_attackType == 6 || m_items[0].m_attackType == 9)
    {
        switch (m_items[0].m_attackType)
        {
        case 2:
            id = 42;
            break;
        case 4:
            id = 29;
            break;
        case 6:
            id = 26;
            break;
        case 9:
            id = 33;
            break;
        default:
            return -1;
        }
    }
    else
    {
        switch (m_items[slot].m_attackType)
        {
        case 1:
            id = slot == 0 ? 41 : 40;
            break;
        case 3:
            id = slot == 0 ? 28 : 27;
            break;
        case 5:
        case 11:
            id = slot == 0 ? 25 : 24;
            break;
        case 7:
            id = 31;
            break;
        case 8:
            id = slot == 0 ? 35 : 32;
            break;
        case 10:
            id = 36;
            break;
        default:
            return -1;
        }
    }
    return SetAction((VICSpriteAnimID)id, sc);
}

int VICSprite::SetArmorSlot(VICSpriteArmorSlot slot, VICSpriteArmorSet set,
                            VICSpriteTint tint, VIScene &sc)
{
    VIRaster *r = sc.m_raster;
    VIMaterial *mat;
    int tex;
    int i;

    if (!r)
        return -1;
    if (set >= 9)
        set = (VICSpriteArmorSet)0;
    if (tint >= 15)
        tint = (VICSpriteTint)0;
    if ((unsigned int)slot >= 6)
        return -1;
    if (slot == VICSpriteArmorSlotHelm)
        return SetHelm(set, tint, sc);
    i = slot;
    if (m_slots[i].m_palEntry == -1)
        return 0;
    mat = r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry);
    if (!mat)
        return 0;
    if (!set)
        tex = m_slots[i].m_texture;
    else
        tex = VICSpriteCust::GetArmorSetTexture((VICSpriteRace)m_race, set,
                                                (VICSpriteTextSlot)i);
    m_slots[i].m_armorSet = set;
    m_slots[i].m_tint = tint;
    if (tex < 0 || (m_slots[i].m_flags & 1))
        return 0;
    mat->SetLayerTexture(m_slots[i].m_layer, tex, *r, *sc.m_dict);
    mat->SetLayerColor(m_slots[i].m_layer, *VICSpriteCust::GetTintColor(tint));
    return 0;
}

int VICSprite::SetRobe(int on, int robe, VICSpriteTint tint, VIScene &sc)
{
    VIRaster *r = sc.m_raster;
    int i;

    if (!r)
        return -1;
    if (on)
    {
        for (i = 0; i < 5; i++)
        {
            VIMaterial *mat;
            int tex;

            if (m_slots[i].m_palEntry == -1)
                continue;
            mat = r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry);
            if (!mat)
                continue;
            tex = VICSpriteCust::GetRobeTexture(robe, (VICSpriteTextSlot)i);
            if (tex < 0)
                continue;
            m_slots[i].m_flags |= 1;
            mat->SetLayerTexture(m_slots[i].m_layer, tex, *r, *sc.m_dict);
            mat->SetLayerColor(m_slots[i].m_layer, *VICSpriteCust::GetTintColor(tint));
        }
    }
    else
    {
        for (i = 0; i < 5; i++)
        {
            if (m_slots[i].m_flags & 1)
            {
                m_slots[i].m_flags &= ~1u;
                SetArmorSlot((VICSpriteArmorSlot)i,
                             (VICSpriteArmorSet)m_slots[i].m_armorSet,
                             (VICSpriteTint)m_slots[i].m_tint, sc);
            }
        }
    }
    return 0;
}

int VICSprite::Share(VIScene &sc, VIRaster *r)
{
    VISoundDevice *sd = sc.m_sound;
    VIParticleSystem *ps = sc.ParticleSystem();
    int i;
    int j;

    for (i = 0; i < 66; i++)
    {
        for (j = 0; j < 2; j++)
        {
            if (m_entries[i].m_animation[j] < 0)
                continue;
            if (sc.ShareAnimation(m_entries[i].m_animation[j]) < 0)
                return -1;
        }
        for (j = 0; j < 2; j++)
        {
            if (!sd)
                continue;
            if (m_entries[i].m_sound[j] < 0)
                continue;
            if (sd->ShareSound(m_entries[i].m_sound[j]) < 0)
                return -1;
        }
    }
    for (i = 0; i < 1; i++)
    {
        if (m_body[i] == -1)
            continue;
        if (sc.ShareSprite(m_body[i]) < 0)
            return -1;
    }
    if (r)
    {
        if (r->ShareMaterialPal(m_materialPal) < 0)
            return -1;
        if (sc.ShareRefMap(m_refMap) < 0)
            return -1;
    }
    if (m_defaultAnim != -1)
    {
        if (sc.ShareAnimation(m_defaultAnim) < 0)
            return -1;
    }
    if (sc.ShareRefMap(VIHSprite::m_refMap) < 0)
        return -1;
    if (r)
    {
        for (i = 0; i < 19; i++)
        {
            if (m_slots[i].m_texture == -1)
                continue;
            r->ShareSurface(m_slots[i].m_texture);
        }
    }
    if (sd)
    {
        if (m_snd.m_sound != -1)
            sd->ShareSound(m_snd.m_sound);
    }
    if (ps)
    {
        for (i = 0; i < m_emitters.m_size; i++)
        {
            if (m_emitters[i].m_definition == -1)
                continue;
            ps->ShareParticleDefinition(m_emitters[i].m_definition);
        }
    }
    return 0;
}

int VICSprite::SetAnimPriorities()
{
    m_entries[0].m_priority[0] = 2;
    m_entries[1].m_priority[0] = 2;
    m_entries[2].m_priority[0] = 2;
    m_entries[3].m_priority[0] = 2;
    m_entries[4].m_priority[0] = 2;
    m_entries[5].m_priority[0] = 2;
    m_entries[6].m_priority[0] = 2;
    m_entries[7].m_priority[0] = 3;
    m_entries[8].m_priority[0] = 3;
    m_entries[9].m_priority[0] = 3;
    m_entries[10].m_priority[0] = 2;
    m_entries[11].m_priority[0] = 2;
    m_entries[12].m_priority[0] = 2;
    m_entries[13].m_priority[0] = 2;
    m_entries[14].m_priority[0] = 7;
    m_entries[15].m_priority[0] = 0;
    m_entries[16].m_priority[0] = 1;
    m_entries[17].m_priority[0] = 2;
    m_entries[18].m_priority[0] = 6;
    m_entries[22].m_priority[0] = 5;
    m_entries[23].m_priority[0] = 5;
    m_entries[24].m_priority[0] = 5;
    m_entries[25].m_priority[0] = 5;
    m_entries[26].m_priority[0] = 5;
    m_entries[27].m_priority[0] = 5;
    m_entries[28].m_priority[0] = 5;
    m_entries[29].m_priority[0] = 5;
    m_entries[30].m_priority[0] = 5;
    m_entries[31].m_priority[0] = 5;
    m_entries[32].m_priority[0] = 5;
    m_entries[33].m_priority[0] = 5;
    m_entries[34].m_priority[0] = 4;
    m_entries[35].m_priority[0] = 5;
    m_entries[36].m_priority[0] = 5;
    m_entries[37].m_priority[0] = 5;
    m_entries[38].m_priority[0] = 5;
    m_entries[39].m_priority[0] = 5;
    m_entries[40].m_priority[0] = 5;
    m_entries[41].m_priority[0] = 5;
    m_entries[42].m_priority[0] = 5;
    m_entries[43].m_priority[0] = 5;
    m_entries[44].m_priority[0] = 5;
    m_entries[45].m_priority[0] = 5;
    m_entries[46].m_priority[0] = 5;
    m_entries[50].m_priority[0] = 5;
    m_entries[51].m_priority[0] = 5;
    m_entries[52].m_priority[0] = 5;
    m_entries[53].m_priority[0] = 5;
    m_entries[56].m_priority[0] = 4;
    m_entries[57].m_priority[0] = 4;
    m_entries[59].m_priority[0] = 4;
    m_entries[60].m_priority[0] = 4;
    m_entries[61].m_priority[0] = 4;
    m_entries[63].m_priority[0] = 4;
    m_entries[64].m_priority[0] = 4;
    m_entries[65].m_priority[0] = 4;
    m_entries[0].m_priority[1] = 2;
    m_entries[1].m_priority[1] = 6;
    m_entries[2].m_priority[1] = 6;
    m_entries[3].m_priority[1] = 6;
    m_entries[4].m_priority[1] = 6;
    m_entries[5].m_priority[1] = 6;
    m_entries[6].m_priority[1] = 6;
    m_entries[7].m_priority[1] = 7;
    m_entries[8].m_priority[1] = 3;
    m_entries[9].m_priority[1] = 3;
    m_entries[10].m_priority[1] = 6;
    m_entries[11].m_priority[1] = 6;
    m_entries[12].m_priority[1] = 6;
    m_entries[13].m_priority[1] = 6;
    m_entries[14].m_priority[1] = 8;
    m_entries[15].m_priority[1] = 0;
    m_entries[16].m_priority[1] = 1;
    m_entries[17].m_priority[1] = 1;
    m_entries[18].m_priority[1] = 7;
    m_entries[22].m_priority[1] = 5;
    m_entries[23].m_priority[1] = 5;
    m_entries[24].m_priority[1] = 5;
    m_entries[25].m_priority[1] = 5;
    m_entries[26].m_priority[1] = 5;
    m_entries[27].m_priority[1] = 5;
    m_entries[28].m_priority[1] = 5;
    m_entries[29].m_priority[1] = 5;
    m_entries[30].m_priority[1] = 5;
    m_entries[31].m_priority[1] = 5;
    m_entries[32].m_priority[1] = 5;
    m_entries[33].m_priority[1] = 5;
    m_entries[34].m_priority[1] = 4;
    m_entries[35].m_priority[1] = 5;
    m_entries[36].m_priority[1] = 5;
    m_entries[37].m_priority[1] = 5;
    m_entries[38].m_priority[1] = 5;
    m_entries[39].m_priority[1] = 5;
    m_entries[40].m_priority[1] = 5;
    m_entries[41].m_priority[1] = 5;
    m_entries[42].m_priority[1] = 5;
    m_entries[43].m_priority[1] = 7;
    m_entries[44].m_priority[1] = 7;
    m_entries[45].m_priority[1] = 5;
    m_entries[46].m_priority[1] = 5;
    m_entries[50].m_priority[1] = 5;
    m_entries[51].m_priority[1] = 5;
    m_entries[52].m_priority[1] = 6;
    m_entries[53].m_priority[1] = 6;
    m_entries[56].m_priority[1] = 4;
    m_entries[57].m_priority[1] = 4;
    m_entries[59].m_priority[1] = 4;
    m_entries[60].m_priority[1] = 4;
    m_entries[61].m_priority[1] = 4;
    m_entries[63].m_priority[1] = 4;
    m_entries[64].m_priority[1] = 4;
    m_entries[65].m_priority[1] = 4;
    return 0;
}

int VICSprite::SetHelm(VICSpriteArmorSet set, VICSpriteTint tint, VIScene &sc)
{
    VIRaster *r = sc.m_raster;
    int sprite;
    int i;

    if (m_node[VICSpriteNodeIDHelm] == -1)
        return 0;
    if (m_attach[VICSpriteAttachSlotHelm] != -1)
    {
        if (Detach(m_attach[VICSpriteAttachSlotHelm], sc) < 0)
            return -1;
        m_attach[VICSpriteAttachSlotHelm] = -1;
    }
    if (set)
    {
        sprite = VICSpriteCust::GetHelm(set);
        if (sc.GetSpriteType(sprite) == 1)
        {
            sprite = sc.CopySprite(sprite, 0);
            if (sprite < 0)
                return -1;
            if (r)
            {
                VISimpleSprite *ss = (VISimpleSprite *)sc.Sprite(sprite);
                int pal = r->CopyMaterialPal(ss->GetMaterialPal(), 0);

                if (pal < 0)
                    return -1;
                r->MaterialPal(pal);
                for (i = 0; i < r->GetMaterialPalSize(pal); i++)
                {
                    VIMaterial *m = r->GetMaterialPalEntry(pal, 0);
                    int j;

                    for (j = 0; j < m->m_numLayers; j++)
                        m->SetLayerColor(j, *VICSpriteCust::GetTintColor(tint));
                }
                if (ss->SetMaterialPal(pal, *sc.m_dict, *r) < 0)
                    return -1;
            }
        }
        if (sprite != -1)
        {
            m_attach[VICSpriteAttachSlotHelm] =
                Attach(sprite, m_node[VICSpriteNodeIDHelm], sc);
            if (m_attach[VICSpriteAttachSlotHelm] < 0)
                return -1;
        }
    }
    if (SetHair(m_hair.m_hair, m_hair.m_a, m_hair.m_b, sc) < 0)
        return -1;
    return 0;
}

int VICSprite::SetHair(int hair, int a, int b, VIScene &sc)
{
    VIRaster *r = sc.m_raster;
    VIDictionary *d;
    int tex;
    int null;
    int i;

    if (!r)
        return -1;
    d = sc.m_dict;
    if (!d)
        return -1;
    if (m_slots[11].m_palEntry == -1)
        return 0;
    if (m_slots[12].m_palEntry == -1)
        return 0;
    if (m_slots[13].m_palEntry == -1)
        return 0;
    if (m_slots[14].m_palEntry == -1)
        return 0;
    if (m_slots[15].m_palEntry == -1)
        return 0;
    m_hair.m_hair = hair;
    if (hair < 0)
        hair = 0;
    m_hair.m_a = a;
    m_hair.m_b = b;
    tex = VICSpriteCust::GetHairTexture(hair < 8 ? hair : 7);
    null = VICSpriteCust::HairNull;
    if (tex < 0)
    {
        tex = null;
        if (tex < 0)
            return 0;
    }
    for (i = 8; i < 11; i++)
    {
        if (m_slots[i].m_palEntry == -1)
            continue;
        if (i - 8 >= b)
            r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
                ->SetLayerTexture(m_slots[i].m_layer, null, *r, *d);
        else
            r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
                ->SetLayerTexture(m_slots[i].m_layer, tex, *r, *d);
    }
    for (i = 16; i < 19; i++)
    {
        if (m_slots[i].m_palEntry == -1)
            continue;
        if (i - 16 >= b || m_attach[VICSpriteAttachSlotHelm] != -1)
            r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
                ->SetLayerTexture(m_slots[i].m_layer, null, *r, *d);
        else
            r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
                ->SetLayerTexture(m_slots[i].m_layer, tex, *r, *d);
    }
    for (i = 11; i < 14; i++)
    {
        if (i - 11 >= a || m_attach[VICSpriteAttachSlotHelm] != -1)
            r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
                ->SetLayerTexture(m_slots[i].m_layer, null, *r, *d);
        else
            r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
                ->SetLayerTexture(m_slots[i].m_layer, tex, *r, *d);
    }
    i = 14;
    if (a >= 2 && b >= 2 && m_attach[VICSpriteAttachSlotHelm] == -1)
        r->GetMaterialPalEntry(m_materialPal, m_slots[14].m_palEntry)
            ->SetLayerTexture(m_slots[14].m_layer, tex, *r, *d);
    else
        r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
            ->SetLayerTexture(m_slots[i].m_layer, null, *r, *d);
    r->GetMaterialPalEntry(m_materialPal, m_slots[15].m_palEntry)
        ->SetLayerTexture(m_slots[15].m_layer, tex, *r, *d);
    return 0;
}

int VICSprite::SetDefaults(VIScene &sc)
{
    long now;

    m_cur[0].m_anim = 0;
    m_cur[0].m_play = AddPlay(m_entries[0].m_animation[0],
                              (VIPlaybackType)m_entries[0].m_sync,
                              m_entries[0].m_weight, sc);
    if (m_cur[0].m_play < 0)
        return -1;
    if (m_skelType == 1)
    {
        m_cur[1].m_anim = 0;
        m_cur[1].m_play = AddPlay(m_entries[0].m_animation[1],
                                  (VIPlaybackType)m_entries[0].m_sync,
                                  m_entries[0].m_weight, sc);
        if (m_cur[1].m_play < 0)
            return -1;
    }
    m_attach[VICSpriteAttachSlotSkin] = AttachSkin(m_body[0], sc);
    if (m_attach[VICSpriteAttachSlotSkin] < 0)
        return -1;
    now = sc.GetTime();
    m_animTime = now + rand() % 20000 + 20000;
    m_capsule.Set(VIVect3(0.0f, 0.0f, 0.0f), VIVect3(0.0f, 1.0f, 0.0f), 2.0f, 0.5f);
    if (sc.m_raster)
    {
        int rc = SetHair(0, 2, 1, sc);

        if (rc < 0)
            return -1;
    }
    return 0;
}

int VICSprite::SetDeath(int on, int erase, VIScene &sc)
{
    VISoundDevice *sd = sc.m_sound;
    int i;

    // The store goes BEFORE the test (it fills the branch's delay slot) and
    // the body nests under `if (on)` so the test threads to the one bottom
    // `return 0`.
    m_dead = on;
    if (on)
    {
        if (erase)
        {
            for (i = 0; i < 2; i++)
            {
                if (m_cur[i].m_play != -1)
                {
                    if (ErasePlay(m_cur[i].m_play, sc) < 0)
                        return -1;
                    if (sd)
                    {
                        if (m_cur[i].m_flags != -1)
                        {
                            sd->Stop(m_cur[i].m_flags);
                            m_cur[i].m_flags = -1;
                        }
                    }
                    m_cur[i].m_play = -1;
                    m_cur[i].m_anim = 0;
                }
                if (m_prev[i].m_play != -1)
                {
                    if (ErasePlay(m_prev[i].m_play, sc) < 0)
                        return -1;
                    if (sd)
                    {
                        if (m_prev[i].m_flags != -1)
                        {
                            sd->Stop(m_prev[i].m_flags);
                            m_prev[i].m_flags = -1;
                        }
                    }
                    m_prev[i].m_play = -1;
                    m_prev[i].m_anim = 0;
                }
            }
        }
        if (SetLocomotion((VICSpriteAnimID)14, sc) < 0)
            return -1;
        if (erase)
        {
            for (i = 0; i < 2; i++)
            {
                if (m_cur[i].m_play == -1)
                    continue;
                SetPlayFrame(m_cur[i].m_play,
                             (float)(sc.Animation(m_plays.Value(m_cur[i].m_play).m_animID)
                                         ->GetNumFramesPerNode() - 1),
                             sc);
            }
        }
    }
    return 0;
}

int VICSprite::AttachItem(int sprite, VICSpriteAttachSlot slot,
                          VICSpriteAttackType type, int projectile, VIScene &sc)
{
    // Phase 61: the two dead initialisers are worth their insns.  flow
    // deletes them, but gcse runs BEFORE flow and sizes its PRE hash table
    // as n_insns/2|1 -- 77 buckets without them, 79 with -- and PRE creates
    // its reaching registers in BUCKET order, which is the order reload
    // hands out the spill slots: slot*4 at sp+4 needs the 79.
    VISprite *sp = 0;
    VIHSprite *hs = 0;
    VIHSpriteAnim *anim;
    int play;
    int def;

    if (!(m_flags & 1))
        return 0;
    if (m_attach[slot] != -1)
    {
        if (DetachItem(slot, sc) < 0)
            return -1;
    }
    if (m_items[slot].m_projectile != -1)
        sc.ReleaseSprite(m_items[slot].m_projectile);
    if (m_items[slot].m_node != -1)
    {
        sp = sc.Sprite(sprite);
        if (sp->m_type == 4 || sp->GetInstType(sc) == 1)
        {
            sprite = sc.CopySprite(sprite, 0);
            if (sprite < 0)
                return -1;
            if (sc.GetSpriteType(sprite) == 4)
            {
                hs = (VIHSprite *)sc.Sprite(sprite);
                if (hs->EraseAllPlay(sc) < 0)
                    return -1;
                anim = sc.Animation(hs->m_defaultAnim);
                def = hs->m_defaultAnim;
                play = hs->AddPlay(def, anim->GetPlayMode(),
                                   anim->GetPlaySpeed(), sc);
                if (play < 0)
                    return -1;
                if (anim->GetPlayMode() == 1)
                    hs->SetPlayFrame(play, (float)anim->GetNumFramesPerNode(), sc);
            }
        }
        m_attach[slot] = Attach(sprite, m_items[slot].m_node, sc);
        if (m_attach[slot] < 0)
            return -1;
    }
    m_items[slot].m_attackType = type;
    m_items[slot].m_projectile = projectile;
    if (projectile != -1)
        sc.ShareSprite(projectile);
    return 0;
}

int VICSprite::Release(VIScene &sc, VIDictionary &d, VIRaster *r,
                       VISoundDevice *sd, VICollide &c)
{
    VIParticleSystem *ps = sc.ParticleSystem();
    int i;
    int j;

    if (VIHSprite::Release(sc, d, r, sd, c) < 0)
        return -1;
    for (i = 0; i < 66; i++)
    {
        for (j = 0; j < 2; j++)
        {
            if (m_entries[i].m_animation[j] < 0)
                continue;
            if (sc.ReleaseAnimation(m_entries[i].m_animation[j]) < 0)
                return -1;
        }
        for (j = 0; j < 2; j++)
        {
            if (!sd)
                continue;
            if (m_entries[i].m_sound[j] < 0)
                continue;
            if (sd->ReleaseSound(m_entries[i].m_sound[j], d) < 0)
                return -1;
        }
    }
    for (i = 0; i < 1; i++)
    {
        if (m_body[i] == -1)
            continue;
        if (sc.ReleaseSprite(m_body[i]) < 0)
            return -1;
    }
    if (r)
    {
        if (r->ReleaseMaterialPal(m_materialPal, d) < 0)
            return -1;
        if (sc.ReleaseRefMap(m_refMap) < 0)
            return -1;
        for (i = 0; i < 19; i++)
        {
            if (m_slots[i].m_texture == -1)
                continue;
            r->ReleaseSurface(m_slots[i].m_texture, d);
        }
    }
    for (i = 0; i < 3; i++)
    {
        if (m_items[i].m_projectile == -1)
            continue;
        sc.ReleaseSprite(m_items[i].m_projectile);
    }
    if (m_trail)
    {
        VITrailFxEnd(m_trail);
        m_trail = 0;
    }
    if (sd)
    {
        if (m_snd.m_sound != -1)
        {
            if (m_snd.m_channel != -1)
                sd->Stop(m_snd.m_channel);
            sd->ReleaseSound(m_snd.m_sound, d);
        }
    }
    if (ps)
    {
        for (i = 0; i < m_emitters.m_size; i++)
        {
            if (m_emitters[i].m_definition == -1)
                continue;
            m_emitters[i].m_emitter.Destroy();
            ps->ReleaseParticleDefinition(m_emitters[i].m_definition, d);
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Phase 61.
// ---------------------------------------------------------------------------

static inline float DistanceSquared(const VIVect3 &a, const VIVect3 &b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;

    return dx * dx + dy * dy + dz * dz;
}

// A NameColors entry as a float colour, each byte over 255.
static inline void NameColor(VIColor32F &cf, const VIColor32 &c)
{
    cf.r = c.r * (1.0f / 255.0f);
    cf.g = c.g * (1.0f / 255.0f);
    cf.b = c.b * (1.0f / 255.0f);
    cf.a = c.a * (1.0f / 255.0f);
}

// The name plate: a VINameSprite attached at the name node, created the first
// time and renamed and recoloured after that.  The VIColor32 is never used.
int VICSprite::SetName(const char *name, VICSpriteNameColor color, VIScene &sc)
{
    VIColor32 c;
    VIColor32F cf;
    int font;

    if (sc.m_raster == 0)
        return -1;
    if (m_node[VICSpriteNodeIDName] == -1)
        return 0;

    if (m_attach[4] == -1)
    {
        if (!sc.m_dict->FindTyped(FontResourceID, VIRT_Font, font))
            return -1;

        int spr = sc.CreateSprite(VISpriteTypeName, 0);
        if (spr < 0)
            return -1;

        NameColor(cf, NameColors[color]);
        if (((VINameSprite *)sc.Sprite(spr))->Init(name, font, cf, sc.m_raster) < 0)
            return -1;
        m_attach[4] = Attach(spr, m_node[VICSpriteNodeIDName], sc);
        if (m_attach[4] < 0)
            return -1;
    }
    else
    {
        VINameSprite *ns =
            (VINameSprite *)sc.Sprite(VIHSprite::m_attach.Value(m_attach[4]).m_sprite);

        if (ns->SetName(name) < 0)
            return -1;
        NameColor(cf, NameColors[color]);
        ns->SetColor(cf);
    }
    return 0;
}

// Volume falls off linearly to nothing at 50 units; pan is the cosine between
// the listener-to-source direction and the camera's right, in the ground plane.
int VICSprite::CalcVolumeAndPan(VICamera &cam, const VIVect3 &at, float &vol, float &pan)
{
    VIVect3 d;
    VIVect3 r;

    vol = cam.m_position.Distance(at) * 0.02f;
    vol = VIMIN(1.0f, VIMAX(0.0f, 1.0f - vol));
    if (vol > 0.0f)
    {
        d.x = cam.m_position.x - at.x;
        d.y = 0.0f;
        d.z = cam.m_position.z - at.z;
        r.x = cam.m_dir.z;
        r.y = 0.0f;
        r.z = -cam.m_dir.x;
        d.Normalize();
        r.Normalize();
        pan = d.x * r.x + d.z * r.z;
    }
    else
        pan = 0.0f;
    return 0;
}

// Fills every animation slot the ESF left empty from a related one: most from
// the idle (0), the attack families from 22 or 23, and 65 from 41 at half
// weight.  The table is read out of the dump at 0x004EA970.
int VICSprite::SetDefaultEntries()
{
    for (int i = 0; i < 66; i++)
    {
        if (m_entries[i].m_animation[0] != -1)
            continue;

        switch (i)
        {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6:
        case 8: case 9: case 10: case 11: case 12: case 13: case 14:
        case 15: case 16: case 17: case 18: case 37: case 38: case 39:
        case 45: case 46:
            m_entries[i] = m_entries[0];
            break;

        case 65:
            if (m_race == 9)
            {
                m_entries[i] = m_entries[0];
                m_entries[i].m_sync = 1;
            }
            else if (m_entries[41].m_animation[0] != -1)
            {
                m_entries[i] = m_entries[41];
                m_entries[i].m_weight *= 0.5f;
            }
            else
            {
                m_entries[i] = m_entries[0];
                m_entries[i].m_sync = 1;
            }
            m_entries[i].m_sound[1] = -1;
            m_entries[i].m_sound[0] = -1;
            break;

        case 7: case 34: case 56: case 57: case 59: case 60: case 61:
        case 63: case 64:
            m_entries[i] = m_entries[0];
            m_entries[i].m_sync = 1;
            break;

        case 23: case 25: case 26: case 28: case 29: case 31: case 33:
        case 35: case 41: case 42: case 43: case 44: case 50: case 51:
        case 52: case 53:
            if (m_entries[23].m_animation[0] != -1)
                m_entries[i] = m_entries[23];
            else
            {
                m_entries[i] = m_entries[0];
                m_entries[i].m_sync = 1;
            }
            break;

        case 22: case 24: case 27: case 30: case 32: case 36: case 40:
            if (m_entries[22].m_animation[0] != -1)
                m_entries[i] = m_entries[22];
            else if (m_entries[23].m_animation[0] != -1)
                m_entries[i] = m_entries[23];
            else
            {
                m_entries[i] = m_entries[0];
                m_entries[i].m_sync = 1;
            }
            break;
        }
    }
    return 0;
}

// The weapon trail.  Only the swinging attack animations have one, and which
// attach slot carries the weapon is a property of the animation (the table at
// 0x004EA910).  A new trail starts with each new action; a segment is added
// every frame between the two point sprites of the weapon's group sprite.  The
// first matrix is never used.
int VICSprite::ProcessWeaponTrails(VICamera &cam, VIScene &sc, VIMatrix44 &m)
{
    VIVect3 a, b;
    VIMatrix44 unused, center, world, member;
    int slot;
    int k;
    int n;

    if (m_trailSlot == 0)
        return 0;
    if (sc.GetTime() - m_trailTime < 50)
        return 0;

    if (m_trail && m_deathAnim != -1 && m_deathAnim != m_action)
    {
        VITrailFxEnd(m_trail);
        m_trail = 0;
    }

    switch (m_cur[0].m_anim)
    {
    case 23: case 25: case 26: case 28: case 29: case 41: case 42: case 44:
        slot = 0;
        break;
    case 22: case 24: case 27: case 40:
        slot = 1;
        break;
    case 30:
        slot = 2;
        break;
    default:
        return 0;
    }

    if (m_action != m_deathAnim || m_action == -1)
    {
        m_trail = VITrailFxBegin(sc.GetTime(), 25, 0.4f);
        if (m_trail == 0)
            return -1;
        m_deathAnim = m_action;
    }

    world.Mul(m, cam.m_world);
    n = 0;
    CalcTransAboutCenter(world, sc, center);

    if (m_attach[slot] != -1)
    {
        int spr = GetAttachSprite(m_attach[slot]);

        if (sc.GetSpriteType(spr) == VISpriteTypeGroup)
        {
            VIGroupSprite *g = (VIGroupSprite *)sc.Sprite(spr);

            for (k = 0; k < g->m_numMembers; k++)
            {
                int ms = g->m_members[k].m_sprite;

                if (sc.GetSpriteType(ms) != VISpriteTypePoint)
                    continue;

                VIPointSprite *ps = (VIPointSprite *)sc.Sprite(ms);
                g->CalcMemberTransform(k, member);
                int node = m_items[slot].m_node;
                VIVector<VIHSpriteNode> *nodes = &m_nodes;
                world.Mul(member, nodes->m_data[node].m_world);
                member.Mul(world, center);

                if (ps->m_pointType == 0)
                {
                    a.x = member.m[3].x;
                    a.y = member.m[3].y;
                    a.z = member.m[3].z;
                    n++;
                }
                else if (ps->m_pointType == 1)
                {
                    b.x = member.m[3].x;
                    b.y = member.m[3].y;
                    b.z = member.m[3].z;
                    n++;
                }

                if (n == 2)
                {
                    m_trail->AddSegment(a, b, m_trailColor, VIColor32F(0.0f, 0.0f, 0.0f, 1.0f));
                    break;
                }
            }
        }
    }
    return 0;
}

// Moves every node-bound particle emitter to its node, once per scene tick:
// player-only emitters are skipped on NPCs, anything past 100 units is purged,
// and a one-shot emitter stops updating when its life runs out.
int VICSprite::UpdateParticleEmitters(VICamera &cam, VIScene &sc, VIRaster &r,
                                      VIMatrix44 &m)
{
    VIMatrix44 c;
    VIMatrix44 w;
    VIVect3 pos;

    if (sc.GetTime() == m_emitterTime)
        return 0;
    m_emitterTime = sc.GetTime();

    w.Mul(m, cam.m_world);
    CalcTransAboutCenter(w, sc, c);

    for (int i = 0; i < m_emitters.m_size; i++)
    {
        VICSpritePartEmitter *e = &m_emitters[i];
        VIVector<VIHSpriteNode> *nodes = &m_nodes;

        if ((e->m_flags & 1) && !(m_flags & 4))
            continue;

        w.Mul(nodes->m_data[e->m_node].m_world, c);
        float d = DistanceSquared(*(VIVect3 *)&w.m[3], cam.m_position);
        pos.x = w.m[3].x;
        pos.y = w.m[3].y;
        pos.z = w.m[3].z;

        if (10000.0f < d)
        {
            m_emitters[i].m_emitter.Purge();
            continue;
        }

        if (m_emitters[i].m_useLocation)
            m_emitters[i].m_emitter.SetLocation(pos);
        else
            m_emitters[i].m_emitter.SetLocation(VIVect3());

        if (m_emitters[i].m_useLocation)
        {
            VIMatrix33 d;

            d.m[0].x = w.m[0].x;
            d.m[0].y = w.m[0].y;
            d.m[0].z = w.m[0].z;
            d.m[1].x = w.m[1].x;
            d.m[1].y = w.m[1].y;
            d.m[1].z = w.m[1].z;
            d.m[2].x = w.m[2].x;
            d.m[2].y = w.m[2].y;
            d.m[2].z = w.m[2].z;
            m_emitters[i].m_emitter.SetDirection(d);
        }
        else
        {
            m_emitters[i].m_emitter.SetDirection(VIVect3(0.0f, 0.0f, 1.0f));
            m_emitters[i].m_emitter.SetSpace(w);
        }

        if (m_emitters[i].m_oneShot == 1 &&
            m_emitters[i].m_start + m_emitters[i].m_life < sc.GetTime())
            continue;
        m_emitters[i].m_emitter.Update();
    }
    return 0;
}

// Fires the sprite's tripped animation triggers -- a sound on one of the two
// layers, or a restart of the matching one-shot particle emitters -- and then
// keeps the looping ambient sound (m_snd) playing at the right volume and pan.
int VICSprite::Process(VICamera &cam, const VIVect3 &at, VIScene &sc)
{
    float vol, pan;
    VISoundDevice *sd = sc.m_sound;
    int i;

    for (i = 0; i < m_triggers.m_size; i++)
    {
        if (!HasTripped(i))
            continue;

        // ONE variable for two jobs, and the register allocation says so: first
        // "is a particle trigger", then which half (upper/lower body, BPU or
        // BPL) the trigger belongs to.  Two variables put them in two
        // registers.
        int layer = -1;
        int sel = 0;
        int id = m_triggers[i].m_a;

        if (id == PrimSoundRefID || id == PrimSoundBPLRefID || id == PrimSoundBPURefID)
            layer = 0;
        else if (id == SecSoundRefID || id == SecSoundBPLRefID || id == SecSoundBPURefID)
            layer = 1;
        else if (id == Particle1RefID || id == Particle1BPLRefID ||
                 id == Particle1BPURefID || id == Particle2RefID ||
                 id == Particle2BPLRefID || id == Particle2BPURefID)
            sel = 1;

        if (layer != -1)
        {
            if (sd == 0)
                continue;

            sel = 1;
            if (m_triggers[i].m_a != PrimSoundBPLRefID &&
                m_triggers[i].m_a != SecSoundBPLRefID)
                sel = 0;

            if (sel == m_entries[m_cur[sel].m_anim].m_channel &&
                m_entries[m_cur[sel].m_anim].m_sound[layer] != -1)
            {
                if (m_cur[sel].m_flags != -1)
                {
                    sd->Stop(m_cur[sel].m_flags);
                    m_cur[sel].m_flags = -1;
                }
                CalcVolumeAndPan(cam, at, vol, pan);
                vol *= m_entries[m_cur[sel].m_anim].m_volume[layer];
                if (vol > 0.0f)
                {
                    float pitch = (float)(rand() & 0x1FFF) - 4096.0f;
                    pitch = pitch * m_entries[m_cur[sel].m_anim].m_pitchVar[layer];

                    m_cur[sel].m_flags = sd->Play(m_entries[m_cur[sel].m_anim].m_sound[layer], 0,
                                                  VISOUNDCLASS_SFX);
                    pitch += 4096.0f;
                    if (m_cur[sel].m_flags >= 0)
                    {
                        sd->SetVolume(m_cur[sel].m_flags, vol);
                        sd->SetPan(m_cur[sel].m_flags, pan);
                        sd->SetPitch(m_cur[sel].m_flags, (int)pitch);
                    }
                    else
                        m_cur[sel].m_flags = -1;
                }
            }
            ResetTrigger(i);
        }
        else if (sel)
        {
            sel = 1;
            if (m_triggers[i].m_a != Particle1BPLRefID &&
                m_triggers[i].m_a != Particle2BPLRefID)
                sel = 0;

            for (int k = 0; k < m_emitters.m_size; k++)
            {
                VICSpritePartEmitter *e = &m_emitters[k];

                if (e->m_oneShot == 1 && e->m_unk400 == m_cur[sel].m_anim &&
                    e->m_unk404 == m_triggers[i].m_a)
                    e->m_start = sc.GetTime();
            }
            ResetTrigger(i);
        }
    }

    if (sd && m_snd.m_sound != -1)
    {
        CalcVolumeAndPan(cam, at, vol, pan);
        vol *= m_snd.m_volume;
        if (vol > 0.0f && !m_dead)
        {
            if (m_snd.m_channel == -1 || !sd->IsPlaying(m_snd.m_channel))
                m_snd.m_channel = sd->Play(m_snd.m_sound, 0, VISOUNDCLASS_SFX);
            if (m_snd.m_channel >= 0)
            {
                sd->SetVolume(m_snd.m_channel, vol);
                sd->SetPan(m_snd.m_channel, pan);
            }
        }
        else if (m_snd.m_channel != -1)
        {
            sd->Stop(m_snd.m_channel);
            m_snd.m_channel = -1;
        }
    }
    return 0;
}

// The one door every animation change goes through.  Idle becomes the special
// idle every 20-40 s and the battle-stance idle within 5 s of an attack; an
// attack over a hit reaction becomes 44; and a new animation must out-rank
// the one it replaces on the same layer (the entry's m_priority) unless that
// one has stopped.  Then the old play is kept as m_prev for the cross-fade,
// the new one started -- synced to layer 1 when the humanoid's two layers
// agree -- and the layer's sound and particle triggers re-armed.
//
// Phase 61 wave 5 (168 -> 0): the exits are the source.  Every place the
// target keeps an address in a register across a label, cse followed a jump
// into it, which needs the label to have ONE use: the Freeze/Erase/Stop
// failures `goto fail` (the play < 0 block) so the erase label is reached
// only from IsBlendComplete; `(id != 0 && id != 17) || anim >= 14` is one
// condition so its inner label has one use; and the "already playing"
// return is `goto done`, a direct jump, so the join's &m_cur[layer].m_anim
// (t1) reaches the AddPlay arm.  That drops one use of the gcse'd this+4492,
// whose priority then falls below &m_cur[layer].m_play's: s8/s7 as target.
int VICSprite::SetAnimation(VICSpriteAnimID id, int layer, VIScene &sc)
{
    int ids[2];
    int idx;
    int play;

    if (m_animTime < sc.GetTime())
    {
        if (id == VICSpriteAnimID0)
            id = VICSpriteAnimID15;
        if (m_skelType != 1 || layer == 1)
            m_animTime = sc.GetTime() + rand() % 20000 + 20000;
    }

    if (sc.GetTime() > 5000 && id == VICSpriteAnimID0)
        id = sc.GetTime() - m_idleTime > 4999 ? VICSpriteAnimID0 : VICSpriteAnimID17;

    if (m_skelType == 1)
    {
        if ((unsigned)(id - 22) < 32 && id != 34)
        {
            int a = m_cur[1].m_anim;
            if ((unsigned)(a - 12) < 2 || a == 44)
                id = (VICSpriteAnimID)44;
        }
    }
    else if (layer == 0)
    {
        if ((unsigned)(id - 22) < 32 && id != 34)
        {
            int a = m_cur[0].m_anim;
            if ((unsigned)(a - 12) < 2 || a == 44)
                id = (VICSpriteAnimID)44;
        }
    }

    if (m_dead)
        id = VICSpriteAnimID14;

    // -1 as a VARIABLE: one long-lived register serves both tests and the
    // store after ErasePlay (s5 in the target); `play` holds each Add*Play
    // result so the final test reads the call's v0, not memory (182 -> 168).
    int none = -1;

    if (m_cur[layer].m_play != none && m_prev[layer].m_play != none)
    {
        if (!IsBlendComplete(m_cur[layer].m_play, sc))
        {
            int cur = m_cur[layer].m_anim;

            if (m_entries[id].m_priority[layer] < m_entries[cur].m_priority[layer])
                goto check;
            if (id == cur)
                goto check;
            if (cur == 15)
                goto special;
            if (id == 0)
                goto check;
            if (FreezePlayback(m_cur[layer].m_play, sc) < 0)
                goto fail;
        }
        if (ErasePlay(m_prev[layer].m_play, sc) < 0)
            goto fail;
        m_prev[layer].m_play = none;
    }

check:
    if (m_cur[layer].m_anim == 15)
        goto special;
    if (m_cur[layer].m_sync == 1 && !IsPlaybackStopped(m_cur[layer].m_play))
    {
        if (m_entries[id].m_priority[layer] < m_entries[m_cur[layer].m_anim].m_priority[layer])
            return 0;
    }
    goto rank;

special:
    if (id == 0 && m_cur[layer].m_sync == 1 && !IsPlaybackStopped(m_cur[layer].m_play))
        return 0;

rank:
    if (m_entries[id].m_priority[layer] < m_entries[m_cur[layer].m_anim].m_priority[layer])
    {
        if (m_cur[layer].m_sync != 1 || !IsPlaybackStopped(m_cur[layer].m_play))
        {
            if (id != 15 || m_cur[layer].m_anim != 0)
            {
                if (layer == 0 || (id != 0 && id != 17) ||
                    m_cur[layer].m_anim >= 14)
                    return 0;
            }
        }
    }

    if (((unsigned)(id - 22) < 22 || (unsigned)(id - 50) < 4) &&
        (unsigned)(id - 37) >= 3 && (unsigned)(id - 31) >= 2 && id != 35 && id != 33)
        m_idleTime = sc.GetTime();

    if (id == m_cur[layer].m_anim && m_cur[layer].m_play != -1)
        goto done;

    if (m_prev[layer].m_play == -1)
    {
        m_prev[layer] = m_cur[layer];
        if (m_cur[layer].m_play != -1)
        {
            if (StopPlayback(m_cur[layer].m_play, sc) < 0)
                goto fail;
            if (m_skelType == 1 && layer == 0 && m_cur[1].m_anim == id)
            {
                m_cur[0].m_anim = id;
                m_cur[0].m_sync = m_entries[id].m_sync;
                play = AddInterpolatedPlayWithSync(m_entries[id].m_animation[0],
                                                   m_cur[1].m_play, 250, sc);
                m_cur[0].m_play = play;
            }
            else
            {
                m_cur[layer].m_anim = id;
                m_cur[layer].m_sync = m_entries[id].m_sync;
                play = AddInterpolatedPlay(
                    m_entries[m_cur[layer].m_anim].m_animation[layer],
                    (VIPlaybackType)m_entries[m_cur[layer].m_anim].m_sync,
                    m_entries[m_cur[layer].m_anim].m_weight, 250, sc);
                m_cur[layer].m_play = play;
            }
        }
        else
        {
            m_cur[layer].m_anim = id;
            m_cur[layer].m_sync = m_entries[id].m_sync;
            play = AddPlay(
                m_entries[m_cur[layer].m_anim].m_animation[layer],
                (VIPlaybackType)m_entries[m_cur[layer].m_anim].m_sync,
                m_entries[m_cur[layer].m_anim].m_weight, sc);
            m_cur[layer].m_play = play;
        }
        if (play < 0)
        {
fail:
            return -1;
        }
    }

    if (m_skelType != 1)
    {
        ids[0] = PrimSoundRefID;
        ids[1] = SecSoundRefID;
    }
    else if (layer == 0)
    {
        ids[0] = PrimSoundBPURefID;
        ids[1] = SecSoundBPURefID;
    }
    else
    {
        ids[0] = PrimSoundBPLRefID;
        ids[1] = SecSoundBPLRefID;
    }
    if (FindTrigger(ids[0], idx))
        ResetTrigger(idx);
    if (FindTrigger(ids[1], idx))
        ResetTrigger(idx);

    if (m_skelType != 1)
    {
        ids[0] = Particle1RefID;
        ids[1] = Particle2RefID;
    }
    else if (layer == 0)
    {
        ids[0] = Particle1BPURefID;
        ids[1] = Particle2BPURefID;
    }
    else
    {
        ids[0] = Particle1BPLRefID;
        ids[1] = Particle2BPLRefID;
    }
    if (FindTrigger(ids[0], idx))
        ResetTrigger(idx);
    if (FindTrigger(ids[1], idx))
        ResetTrigger(idx);

    if (layer == 0 && SetItemAction(0, sc) < 0)
        return -1;
    return 1;
done:
    return 0;
}

// A whole second character: the hierarchy's containers, every field, the
// customisation tables (memcpy'd -- the block moves take their alignment from
// the offset, which only a pointer-plus-offset source gives), and then fresh
// copies of the attachments that cannot be shared -- the name plate and the
// skins, a LOD's skin levels too, each re-palettised -- before everything else
// is shared.
int VICSprite::Copy(VIScene &sc, VIRaster *r, VISoundDevice *sd, VICollide &c, int n)
{
    int idx = sc.CreateSprite((VISpriteType)m_type, n);
    int i, k;

    if (idx < 0)
        return -1;

    VICSprite *d = (VICSprite *)sc.Sprite(idx);

    if (d->m_nodes.Init(m_nodes) < 0)
        return -1;
    if (d->m_blend.Init(m_blend) < 0)
        return -1;
    if (d->m_triggers.Init(m_triggers) < 0)
        return -1;
    if (d->VIHSprite::m_attach.Init(VIHSprite::m_attach) < 0)
        return -1;
    if (d->m_plays.Init(m_plays) < 0)
        return -1;
    if (d->m_playNodes.Init(m_playNodes) < 0)
        return -1;

    d->m_bbox = m_bbox;
    d->m_bsphere = m_bsphere;
    d->m_type = m_type;
    d->m_flags = m_flags;
    d->m_offDist = m_offDist;
    d->m_lod = m_lod;
    d->m_defaultScale = m_defaultScale;
    d->VIHSprite::m_refMap = VIHSprite::m_refMap;
    d->m_defaultAnim = m_defaultAnim;
    d->m_skelType = m_skelType;
    d->m_sex = m_sex;
    d->m_materialPal = -1;
    d->m_race = m_race;
    d->m_refMap = m_refMap;
    d->m_capsule = m_capsule;
    memcpy(d->m_entries, m_entries, sizeof(m_entries));
    memcpy(d->m_body, m_body, sizeof(m_body));
    memcpy(d->m_items, m_items, sizeof(m_items));
    memcpy(d->m_node, m_node, sizeof(m_node));
    memcpy(d->m_attach, m_attach, sizeof(m_attach));
    memcpy(d->m_slots, m_slots, sizeof(m_slots));
    memcpy(&d->m_hair, &m_hair, sizeof(m_hair));
    memcpy(&d->m_snd, &m_snd, sizeof(m_snd));
    d->m_cur[0] = m_cur[0];
    d->m_cur[1] = m_cur[1];
    d->m_prev[0] = m_prev[0];
    d->m_prev[1] = m_prev[1];

    if (r)
    {
        d->m_materialPal = r->CopyMaterialPal(m_materialPal, n);
        if (d->m_materialPal < 0)
            return -1;
    }

    VIPool<VIHSpriteAttachment> *da = &d->VIHSprite::m_attach;

    for (i = da->Begin(); i != da->End(); i = da->Next(i))
    {
        if (sc.GetSpriteType(da->Value(i).m_sprite) == VISpriteTypeName ||
            sc.GetSpriteType(da->Value(i).m_sprite) == VISpriteTypeSkin)
        {
            int ns = sc.CopySprite(da->Value(i).m_sprite, 0);

            if (ns < 0)
                return -1;
            if (sc.GetSpriteType(da->Value(i).m_sprite) == VISpriteTypeSkin && r)
            {
                if (((VISkinSprite *)sc.Sprite(ns))->SetMaterialPal(d->m_materialPal, *r,
                                                                    *sc.m_dict) < 0)
                    return -1;
            }
            da->Value(i).m_sprite = ns;
        }
        else if (sc.GetSpriteType(da->Value(i).m_sprite) == VISpriteTypeLOD)
        {
            int ns = sc.CopySprite(da->Value(i).m_sprite, 0);

            if (ns < 0)
                return -1;

            VILODSprite *lod = (VILODSprite *)sc.Sprite(ns);

            for (k = 0; k < lod->m_numLevels; k++)
            {
                int ls = lod->m_levels[k].m_sprite;

                if (sc.GetSpriteType(ls) != VISpriteTypeSkin)
                    continue;

                int nls = sc.CopySprite(ls, 0);
                if (nls < 0)
                    return -1;
                if (r)
                {
                    if (((VISkinSprite *)sc.Sprite(nls))->SetMaterialPal(d->m_materialPal, *r,
                                                                         *sc.m_dict) < 0)
                        return -1;
                }
                if (lod->SetLevelSprite(k, nls, sc) < 0)
                    return -1;
            }
            da->Value(i).m_sprite = ns;
        }
    }

    if (d->m_emitters.Init(m_emitters.m_size) < 0)
        return -1;
    for (k = 0; k < m_emitters.m_size; k++)
    {
        d->m_emitters[k].m_definition = m_emitters[k].m_definition;
        d->m_emitters[k].m_oneShot = m_emitters[k].m_oneShot;
        d->m_emitters[k].m_flags = m_emitters[k].m_flags;
        d->m_emitters[k].m_node = m_emitters[k].m_node;
        d->m_emitters[k].m_useLocation = m_emitters[k].m_useLocation;
        d->m_emitters[k].m_unk400 = m_emitters[k].m_unk400;
        d->m_emitters[k].m_unk404 = m_emitters[k].m_unk404;
        d->m_emitters[k].m_life = m_emitters[k].m_life;
    }
    if (d->CreateParticleEmitters(sc) < 0)
        return -1;

    for (i = da->Begin(); i != da->End(); i = da->Next(i))
        if (sc.ShareSprite(da->Value(i).m_sprite) < 0)
            return -1;
    for (i = m_plays.Begin(); i != m_plays.End(); i = m_plays.Next(i))
        if (sc.ShareAnimation(m_plays.Value(i).m_animID) < 0)
            return -1;
    for (k = 0; k < 3; k++)
        if (m_items[k].m_projectile != -1)
            sc.ShareSprite(m_items[k].m_projectile);

    if (d->Share(sc, r) < 0)
        return -1;
    return idx;
}
