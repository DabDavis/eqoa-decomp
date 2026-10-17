#line 1 "C:/code-frontiers-gm/game/VICreatureEntity.cpp"
// A creature (a player or an NPC): CLIENT 0x00CC20F8.. 0x00CC4608.
// The walking physics is VIWalkControl's; the look and the items are handed to the
// character resources (VIClientPart06D00) by slot.
#include "game.h"

// The asserts spell the components this way (no out-of-line copy exists, so not inlines).
#define AVel(i)   m_avel[i]
#define AAccel(i) m_aaccel[i]


































































































































































// 0x00CC20F8
VICreatureEntity::VICreatureEntity(VIScene *scene)
    : VIEntity(scene)
{
    int i;
    m_p299 = 0;
    memset(m_name, 0, 24);
    m_level = 0;
    m_p74 = 0xFFFFFFFF;
    m_p218.Zero();
    m_avel.Zero();
    m_accel.Zero();
    m_aaccel.Zero();
    m_walkFlags = 0;
    m_floor = 0.0f;
    m_targetHeading = 0.0f;
    m_state = 0;
    m_p255 = 0;
    m_p256 = 0;
    m_p25C = -1;
    m_p260 = 0;
    m_race = 0;
    m_class = 0;
    for (i = 0; i < 3; i++)
    {
        m_item[i] = 0;
        m_itemOver[i] = 0;
    }
    for (i = 0; i < 6; i++)
    {
        m_armorSet[i] = 0;
        m_armorTint[i] = 0;
    }
    m_robe = -1;
    m_itemType[0] = 0;
    m_itemType[1] = 0;
    m_face = 0;
    m_hair[0] = 0;
    m_hair[1] = 0;
    m_hair[2] = 0;
    m_robeTint = 0;
    m_flags262 = 0;
    m_extrapolated = 0;
    m_p2A8 = 1;
    m_deathTime = 0;
    m_p2B8 = 1;
    m_p298 = 0;
    m_p264 = 1;
}

// 0x00CC22A8
VICreatureEntity::~VICreatureEntity()
{
}

// 0x00CC22F8
int VICreatureEntity::IsSelectable()
{
    return m_p256 == 2 || VIEntity::IsSelectable();
}

// 0x00CC2340
void VICreatureEntity::Sync()
{
    ApplyItems();
    ApplyLook();
    VIWalkPhysics p;
    VIWalkControl &w = m_walk;
    p.m_position = m_pos;
    p.m_angles = m_hpr;
    p.m_velocity = m_p218;
    p.m_angularVel = m_avel;
    p.m_force = m_accel;
    p.m_torque = m_aaccel;
    p.m_floor = m_floor;
    p.m_locoState = m_state;
    p.m_targetHeading = m_targetHeading;
    p.m_flags = m_walkFlags;
    w.m_parm64 = m_actor;
    w.SetUpdatePhysics(p, *m_scene);
    m_extrapolated = m_scene->GetTime();
    if (!m_p255)
    {
        VIVect3 pos = *GetPos();
        VIVect3 hpr = *GetHpr();
        ExtrapolateGround(this, pos, hpr, *m_scene);
        StorePos(pos);
        StoreHpr(hpr);
    }
    if (m_actor != -1)
    {
        if (m_npcFlags & 0x200)
            m_scene->SetActorVisibility(m_actor, 0);
        else
            m_scene->SetActorVisibility(m_actor, 1);
    }
}

// ApplyItems' slot-1 second model, by attack type; a SHORT param: compares on an unextended copy.
static inline int SecondModel(short type, int model) { if (type == 7) return 0x845FECDD; else if (type == 9) return 0x08F54A95; else if (type == 10) return model; return 0; }

// 0x00CC2590
void VICreatureEntity::ApplyItems()
{
    if (m_res && m_resSlot != -1)
    {
        for (int i = 0; i < 3; i++)
        {
            int model = m_itemOver[i] ? m_itemOver[i] : m_item[i];
            int type;
            int model2;
            if (i == 0)
            {
                type = m_itemType[0];
                model2 = 0;
            }
            else if (i == 1)
            {
                type = m_itemType[1];
                model2 = SecondModel(type, model);
            }
            else
            {
                type = 0;
                model2 = 0;
            }
            m_res->SetItem(m_resSlot, i, model, type, model2);
        }
    }
}

// 0x00CC2690
void VICreatureEntity::ApplyLook()
{
    if (m_res && m_resSlot != -1)
    {
        int color;
        if (m_p256 < 2)
            color = 0;
        else if (m_p299 & 1)
            color = 2;
        else
            color = 1;
        m_res->SetName(m_resSlot, m_name, color);
        m_res->SetRobe(m_resSlot, m_robe >= 0, m_robe, m_robeTint);
        for (int i = 0; i < 6; i++)
            m_res->SetArmorSlot(m_resSlot, i, m_armorSet[i], m_armorTint[i]);
        m_res->SetHair(m_resSlot, m_hair[0], m_hair[1], m_hair[2]);
        m_res->SetFace(m_resSlot, m_face);
        if (Flag1())
            m_res->SetVisible(m_resSlot, 0);
        else
            m_res->SetVisible(m_resSlot, 1);
    }
}

// 0x00CC27D0
int VICreatureEntity::Refresh()
{
    return 0;
}

// 0x00CC27D8: three components to signed bytes, full scale = range.  The clamp is min(v[i], |range|) as shipped (no lower bound).
static inline float QElem(const VIVect3 &v, int i) { return (&v.x)[i]; }
static void QuantizeVect(const VIVect3 &v, unsigned char *out, float range)
{
    float k = 128.0f / range;
    for (int i = 0; i < 3; i++)
    {
        float x = VIMIN(QElem(v, i), VIMAX(range, -range));
        x *= k;
        if (x == 128.0f) x -= 1.0f;
        out[i] = (int)x;
    }
}

// 0x00CC2890
static void DequantizeVect(const unsigned char *in, VIVect3 &v, float range)
{
    range *= 1.0f / 128.0f;
    for (int i = 0; i < 3; i++)
        v[i] = (signed char)in[i] * range;
}

// 0x00CC28D0
void VICreatureEntity::WriteUpdate(VIMemStream &s)
{
    unsigned char Value[3];
    VIEntity::WriteUpdate(s);
    QuantizeVect(m_p218, Value, 21.0f);
    s.Write(Value, 1, 3);
    QuantizeVect(m_avel, Value, 8.0f);
    s.Write(Value, 1, 2);
    VIASSERT(AVel(2)==0);


    QuantizeVect(m_accel, Value, 63.0f);
    s.Write(Value, 1, 3);
    QuantizeVect(m_aaccel, Value, 32.0f);
    s.Write(Value, 1, 2);
    VIASSERT(AAccel(2)==0);
    s.WriteF32(m_floor);
    s.WriteF32(m_targetHeading);
    s.WriteByte(m_state);
    s.WriteU32(m_p74);
    s.WriteU32(m_walkFlags);
    s.Write(m_item, 4, 3);
    s.Write(m_itemOver, 4, 3);
    s.WriteByte(m_itemType[0]);
    s.WriteByte(m_itemType[1]);
    s.Write(m_armorSet, 1, 6);
    s.Write(m_armorTint, 1, 6);
    s.WriteByte(m_hair[0]);
    s.WriteByte(m_hair[1]);
    s.WriteByte(m_hair[2]);
    s.WriteByte(m_face);
    s.WriteByte(m_robe);
    s.WriteByte(m_robeTint);
    s.WriteU32(m_p294);
    s.WriteByte(m_p298);
    s.Write(m_name, 1, 24);
    s.WriteByte(m_level);
    s.WriteByte(m_p255);
    s.WriteByte(m_p256);
    s.WriteByte(m_p257);
    s.WriteByte(m_race);
    s.WriteByte(m_class);
    s.WriteU16(m_npcFlags);
    s.WriteU32(m_p25C);
    s.WriteByte(m_p260);
    s.WriteByte(m_p261);
    s.WriteByte(m_flags262);
    s.WriteByte(m_p263);
    s.WriteByte(m_p264);
    s.WriteByte(m_p299);
    s.WriteU32(0x71727374);
}

// 0x00CC2DB0
void VICreatureEntity::ReadUpdate(VIMemStream &s)
{
    unsigned char Value[3];
    unsigned int magic;
    VIEntity::ReadUpdate(s);
    s.Read(Value, 1, 3);
    DequantizeVect(Value, m_p218, 21.0f);
    s.Read(Value, 1, 2);
    Value[2] = 0;
    DequantizeVect(Value, m_avel, 8.0f);
    s.Read(Value, 1, 3);
    DequantizeVect(Value, m_accel, 63.0f);
    s.Read(Value, 1, 2);
    Value[2] = 0;
    DequantizeVect(Value, m_aaccel, 32.0f);
    s.Read(&m_floor, 4, 1);
    s.Read(&m_targetHeading, 4, 1);
    s.Read(&m_state, 1, 1);
    s.Read(&m_p74, 4, 1);
    s.Read(&m_walkFlags, 4, 1);
    s.Read(m_item, 4, 3);
    s.Read(m_itemOver, 4, 3);
    s.Read(&m_itemType[0], 1, 1);
    s.Read(&m_itemType[1], 1, 1);
    s.Read(m_armorSet, 1, 6);
    s.Read(m_armorTint, 1, 6);
    s.Read(&m_hair[0], 1, 1);
    s.Read(&m_hair[1], 1, 1);
    s.Read(&m_hair[2], 1, 1);
    s.Read(&m_face, 1, 1);
    s.Read(&m_robe, 1, 1);
    s.Read(&m_robeTint, 1, 1);
    s.Read(&m_p294, 4, 1);
    s.Read(&m_p298, 1, 1);
    s.Read(m_name, 1, 24);
    s.Read(&m_level, 1, 1);
    s.Read(&m_p255, 1, 1);
    s.Read(&m_p256, 1, 1);
    s.Read(&m_p257, 1, 1);
    s.Read(&m_race, 1, 1);
    s.Read(&m_class, 1, 1);
    s.Read(&m_npcFlags, 2, 1);
    s.Read(&m_p25C, 4, 1);
    s.Read(&m_p260, 1, 1);
    s.Read(&m_p261, 1, 1);
    s.Read(&m_flags262, 1, 1);
    s.Read(&m_p263, 1, 1);
    s.Read(&m_p264, 1, 1);
    s.Read(&m_p299, 1, 1);
    s.Read(&magic, 4, 1);
    Sync();
}

// 0x00CC3150
void VICreatureEntity::Write(VIMemStream &s)
{
    VIEntity::Write(s);
    s.Write(&m_p218, 4, 3);
    s.Write(&m_avel, 4, 3);
    s.Write(&m_accel, 4, 3);
    s.Write(&m_aaccel, 4, 3);
    s.WriteF32(m_floor);
    s.WriteF32(m_targetHeading);
    s.WriteByte(m_state);
    s.WriteU32(m_p74);
    s.WriteU32(m_walkFlags);
    s.Write(m_item, 4, 3);
    s.Write(m_itemOver, 4, 3);
    s.WriteByte(m_itemType[0]);
    s.WriteByte(m_itemType[1]);
    s.Write(m_armorSet, 1, 6);
    s.Write(m_armorTint, 1, 6);
    s.WriteByte(m_hair[0]);
    s.WriteByte(m_hair[1]);
    s.WriteByte(m_hair[2]);
    s.WriteByte(m_face);
    s.WriteByte(m_robe);
    s.WriteByte(m_robeTint);
    s.WriteSigned(m_p294);
    s.WriteByte(m_p298);
    s.Write(m_name, 1, 24);
    s.WriteByte(m_level);
    s.WriteByte(m_p255);
    s.WriteByte(m_p256);
    s.WriteByte(m_p257);
    s.WriteByte(m_race);
    s.WriteByte(m_class);
    s.WriteU16(m_npcFlags);
    s.WriteU32(m_p25C);
    s.WriteByte(m_p260);
    s.WriteByte(m_flags262);
    s.WriteByte(m_p263);
    s.WriteByte(m_p264);
    s.WriteByte(m_p299);
}

// 0x00CC3538
void VICreatureEntity::Read(VIMemStream &s)
{
    VIEntity::Read(s);
    s.Read(&m_p218, 4, 3);
    s.Read(&m_avel, 4, 3);
    s.Read(&m_accel, 4, 3);
    s.Read(&m_aaccel, 4, 3);
    s.Read(&m_floor, 4, 1);
    s.Read(&m_targetHeading, 4, 1);
    s.Read(&m_state, 1, 1);
    s.Read(&m_p74, 4, 1);
    s.Read(&m_walkFlags, 4, 1);
    s.Read(m_item, 4, 3);
    s.Read(m_itemOver, 4, 3);
    s.Read(&m_itemType[0], 1, 1);
    s.Read(&m_itemType[1], 1, 1);
    s.Read(m_armorSet, 1, 6);
    s.Read(m_armorTint, 1, 6);
    s.Read(&m_hair[0], 1, 1);
    s.Read(&m_hair[1], 1, 1);
    s.Read(&m_hair[2], 1, 1);
    s.Read(&m_face, 1, 1);
    s.Read(&m_robe, 1, 1);
    s.Read(&m_robeTint, 1, 1);
    s.ReadSigned(m_p294);
    s.Read(&m_p298, 1, 1);
    s.Read(m_name, 1, 24);
    s.Read(&m_level, 1, 1);
    s.Read(&m_p255, 1, 1);
    s.Read(&m_p256, 1, 1);
    s.Read(&m_p257, 1, 1);
    s.Read(&m_race, 1, 1);
    s.Read(&m_class, 1, 1);
    s.Read(&m_npcFlags, 2, 1);
    s.Read(&m_p25C, 4, 1);
    s.Read(&m_p260, 1, 1);
    s.Read(&m_flags262, 1, 1);
    s.Read(&m_p263, 1, 1);
    s.Read(&m_p264, 1, 1);
    s.Read(&m_p299, 1, 1);
    Sync();
}

// 0x00CC3850
int VICreatureEntity::IsType(unsigned char type)
{
    return VIEntity::IsType(type) || type == 130;
}

// 0x00CC38A0
int VICreatureEntity::Flag1()
{
    return m_flags262 & 1;
}

// 0x00CC38B0
int VICreatureEntity::Flag2()
{
    return m_flags262 & 2;
}

// 0x00CC38C0
int VICreatureEntity::Flag4()
{
    return m_flags262 & 4;
}

// 0x00CC38D0
int VICreatureEntity::Flag8()
{
    return m_flags262 & 8;
}

// 0x00CC38E0
int VICreatureEntity::Flag10()
{
    return m_flags262 & 0x10;
}

// 0x00CC38F0
unsigned char VICreatureEntity::Byte263()
{
    return m_p263;
}

// 0x00CC38F8
void VICreatureEntity::Extrapolate(long dt)
{
    ExtrapolateMove(this, *m_scene, (int)dt * 0.001f);
}

// 0x00CC3938
void VICreatureEntity::Walk()
{
    if (m_p255)
    {
        VIVect3 hpr;
        m_walk.Process(1, *m_scene);
        StorePos(m_walk.m_committed.m_position);
        hpr.x = m_walk.m_committed.m_angles.x;
        hpr.y = 0.0f;
        hpr.z = 0.0f;
        if (m_actor != -1 && m_scene->GetActorSpriteType(m_actor) == 5
            && ((VICSprite *)m_scene->GetActorSpritePtr(m_actor))->m_dead)
        {
            m_walk.CalcPitchRollToLandscape(hpr);
            float k = (int)(m_scene->GetTime() - m_deathTime) * 0.001f;
            k = VIMIN(1.0f, VIMAX(0.0f, k));
            hpr.y *= k;
            hpr.z *= k;
        }
        StoreHpr(hpr);
    }
    else
    {
        Extrapolate(m_scene->GetTime() - m_extrapolated);
        m_extrapolated = m_scene->GetTime();
    }
}

// 0x00CC3AB8
void VICreatureEntity::WriteIdentity(VIMemStream &s)
{
    s.WriteU32(m_id);
    s.WriteByte(m_p19);
    s.Write(m_name, 1, 24);
}

// 0x00CC3B30
void VICreatureEntity::SnapToGround()
{
    VIVect3 pos = *GetPos();
    VIVect3 hpr = *GetHpr();
    ExtrapolateGround(this, pos, hpr, *m_scene);
    StorePos(pos);
    StoreHpr(hpr);
}
