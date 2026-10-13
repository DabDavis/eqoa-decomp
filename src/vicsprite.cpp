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

extern "C" void sceVu0CopyMatrix(VIMatrix44 &d, const VIMatrix44 &s);
extern "C" int  rand();
extern "C" unsigned int VIHashRefID(const char *s);
extern "C" unsigned int VIHashResourceID(const char *s);

// The node and resource names a character's sound and particle attachments are
// looked up under, hashed once by the TU's static initialiser (phase 61), in
// the order the .data holds them.
int VICSprite::PrimSoundRefID    = VIHashRefID("Primary Sound");
int VICSprite::PrimSoundBPURefID = VIHashRefID("Primary Sound_BPU");
int VICSprite::PrimSoundBPLRefID = VIHashRefID("Primary Sound_BPL");
int VICSprite::SecSoundRefID     = VIHashRefID("Secondary Sound");
int VICSprite::SecSoundBPURefID  = VIHashRefID("Secondary Sound_BPU");
int VICSprite::SecSoundBPLRefID  = VIHashRefID("Secondary Sound_BPL");
int VICSprite::Particle1RefID    = VIHashRefID("Particle 1");
int VICSprite::Particle1BPURefID = VIHashRefID("Particle 1_BPU");
int VICSprite::Particle1BPLRefID = VIHashRefID("Particle 1_BPL");
int VICSprite::Particle2RefID    = VIHashRefID("Particle 2");
int VICSprite::Particle2BPURefID = VIHashRefID("Particle 2_BPU");
int VICSprite::Particle2BPLRefID = VIHashRefID("Particle 2_BPL");
int VICSprite::FontResourceID    = VIHashResourceID("Arial");

// Three name-plate colours, built by VIColor32's four-byte constructor at
// static-init time -- byte stores, not .data.
VIColor32 VICSprite::NameColors[3] =
{
    VIColor32(212, 192, 111, 255),
    VIColor32(100, 175, 220, 255),
    VIColor32( 90, 255,  80, 255)
};

VICSprite::VICSprite()
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
    for (i = 0; i >= 0; i--)
        m_body[i] = -1;
    for (i = 0; i < 3; i++)
    {
        m_items[i].m_node = -1;
        m_items[i].m_attackType = 11;
        m_items[i].m_projectile = -1;
    }
    for (i = 8; i >= 0; i--)
        m_node[i] = -1;
    for (i = 5; i >= 0; i--)
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
    for (i = 0; i >= 0; i--)
        m_body[i] = -1;
    for (i = 0; i < 3; i++)
    {
        m_items[i].m_node = -1;
        m_items[i].m_attackType = 11;
        m_items[i].m_projectile = -1;
    }
    for (i = 8; i >= 0; i--)
        m_node[i] = -1;
    for (i = 5; i >= 0; i--)
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
    m_sex = sex;
    m_defaultScale = scale;
    m_skelType = skel;
    m_race = race;
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
    int stopped;

    if (m_action != i)
        return 1;
    if (m_cur[0].m_play == -1)
        return 0;
    stopped = IsPlaybackStopped(m_cur[0].m_play);
    if (stopped)
        return 1;
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
    m_entries[22].m_channel = 0;
    m_entries[23].m_channel = 0;
    m_entries[24].m_channel = 0;
    m_entries[25].m_channel = 0;
    m_entries[26].m_channel = 0;
    m_entries[27].m_channel = 0;
    m_entries[28].m_channel = 0;
    m_entries[29].m_channel = 0;
    m_entries[30].m_channel = 0;
    m_entries[31].m_channel = 0;
    m_entries[32].m_channel = 0;
    m_entries[33].m_channel = 0;
    m_entries[34].m_channel = 0;
    m_entries[35].m_channel = 0;
    m_entries[36].m_channel = 0;
    m_entries[64].m_channel = 0;
    m_entries[37].m_channel = 0;
    m_entries[38].m_channel = 0;
    m_entries[39].m_channel = 0;
    m_entries[40].m_channel = 0;
    m_entries[41].m_channel = 0;
    m_entries[42].m_channel = 0;
    m_entries[43].m_channel = 0;
    m_entries[44].m_channel = 0;
    m_entries[45].m_channel = 0;
    m_entries[46].m_channel = 0;
    m_entries[50].m_channel = 0;
    m_entries[51].m_channel = 0;
    m_entries[52].m_channel = 0;
    m_entries[53].m_channel = 0;
    m_entries[56].m_channel = 0;
    m_entries[57].m_channel = 0;
    m_entries[59].m_channel = 0;
    m_entries[60].m_channel = 0;
    m_entries[61].m_channel = 0;
    m_entries[63].m_channel = 0;
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
    if (attach == -1)
        return 0;
    sprite = GetAttachSprite(attach);
    if (sc.GetSpriteType(sprite) != 4)
        return 0;
    hs = (VIHSprite *)sc.Sprite(sprite);
    if (hs->EraseAllPlay(sc) < 0)
        return -1;
    rc = hs->AddPlay(hs->m_defaultAnim, VIPlaybackTypeOnce, m_entries[anim].m_weight, sc);
    if (rc < 0)
        return -1;
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
    if (m_slots[slot].m_palEntry == -1)
        return 0;
    mat = r->GetMaterialPalEntry(m_materialPal, m_slots[slot].m_palEntry);
    if (!mat)
        return 0;
    if (set)
        tex = VICSpriteCust::GetArmorSetTexture((VICSpriteRace)m_race, set,
                                                (VICSpriteTextSlot)slot);
    else
        tex = m_slots[slot].m_texture;
    m_slots[slot].m_armorSet = set;
    m_slots[slot].m_tint = tint;
    if (tex < 0)
        return 0;
    if (m_slots[slot].m_flags & 1)
        return 0;
    mat->SetLayerTexture(m_slots[slot].m_layer, tex, *r, *sc.m_dict);
    mat->SetLayerColor(m_slots[slot].m_layer, *VICSpriteCust::GetTintColor(tint));
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
    if (!ps)
        return 0;
    for (i = 0; i < m_emitters.m_size; i++)
    {
        if (m_emitters[i].m_definition == -1)
            continue;
        ps->ShareParticleDefinition(m_emitters[i].m_definition);
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
    return SetHair(m_hair.m_hair, m_hair.m_a, m_hair.m_b, sc) >= 0 ? 0 : -1;
}

int VICSprite::SetHair(int hair, int a, int b, VIScene &sc)
{
    VIRaster *r = sc.m_raster;
    VIDictionary *d = sc.m_dict;
    int tex;
    int null;
    int i;

    if (!r || !d)
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
        tex = null;
    if (tex < 0)
        return 0;
    for (i = 8; i < 11; i++)
    {
        if (m_slots[i].m_palEntry == -1)
            continue;
        r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
            ->SetLayerTexture(m_slots[i].m_layer, i - 8 < b ? tex : null, *r, *d);
    }
    for (i = 16; i < 19; i++)
    {
        if (m_slots[i].m_palEntry == -1)
            continue;
        r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
            ->SetLayerTexture(m_slots[i].m_layer,
                              i - 16 < b && m_attach[VICSpriteAttachSlotHelm] == -1
                                  ? tex : null, *r, *d);
    }
    for (i = 11; i < 14; i++)
    {
        r->GetMaterialPalEntry(m_materialPal, m_slots[i].m_palEntry)
            ->SetLayerTexture(m_slots[i].m_layer,
                              i - 11 < a && m_attach[VICSpriteAttachSlotHelm] == -1
                                  ? tex : null, *r, *d);
    }
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
    m_capsule.m_base = VIVect3(0.0f, 0.0f, 0.0f);
    m_capsule.m_dir = VIVect3(0.0f, 1.0f, 0.0f);
    m_capsule.m_length = 2.0f;
    m_capsule.m_radius = 0.5f;
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

    if (!on)
        return 0;
    m_dead = on;
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
    if (!erase)
        return 0;
    for (i = 0; i < 2; i++)
    {
        if (m_cur[i].m_play == -1)
            continue;
        SetPlayFrame(m_cur[i].m_play,
                     (float)(sc.Animation(m_plays.Value(m_cur[i].m_play).m_animID)
                                 ->GetNumFramesPerNode() - 1),
                     sc);
    }
    return 0;
}

int VICSprite::AttachItem(int sprite, VICSpriteAttachSlot slot,
                          VICSpriteAttackType type, int projectile, VIScene &sc)
{
    VISprite *sp;
    VIHSprite *hs;
    VIHSpriteAnim *anim;
    int play;

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
                play = hs->AddPlay(hs->m_defaultAnim, anim->GetPlayMode(),
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
    if (projectile != -1)
    {
        m_items[slot].m_projectile = projectile;
        sc.ShareSprite(projectile);
    }
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
    if (!ps)
        return 0;
    for (i = 0; i < m_emitters.m_size; i++)
    {
        if (m_emitters[i].m_definition == -1)
            continue;
        m_emitters[i].m_emitter.Destroy();
        ps->ReleaseParticleDefinition(m_emitters[i].m_definition, d);
    }
    return 0;
}
