#line 1 "C:/code-frontiers-gm/game/VIEntity.cpp"
// VIEntity -- CLIENT 0x00CC4608 .. 0x00CC61A8: the base of every object the server
// streams (VICreatureEntity derives from it), and the zone-name grid of each world.
// Asserts on lines 298 (Dequantize), 476 (CreateEntity) and 681 (Changed).
#include "game.h"















































extern "C" double floor(double);

typedef signed char int8;   // the assert text spells it
#ifndef NULL
#define NULL 0
#endif

#define PI      3.14159265358979323846f
#define TWOPI   (2.0f * PI)
// The clamps are vi.h's VIMIN(hi, VIMAX(lo, v)): the low test is evaluated twice.

// The zone-name grids (WORDS.TXT ids, 641 = none), by world: gWorldInfo's +0x0C..+0x18
// bound the column and the row.
static const unsigned short sZones0[16][13] =   // 0x00CF3820: tunaria, 16 rows of 13
{
    { 641, 641, 641, 641, 641, 641, 641, 641, 641, 1152, 1153, 1176, 1051 },
    { 1046, 1057, 1058, 1074, 1075, 364, 1097, 1123, 1124, 1151, 1154, 1175, 1042 },
    { 1047, 1056, 1059, 1073, 1076, 1096, 1098, 1122, 1125, 1150, 361, 1174, 1045 },
    { 1047, 1055, 1060, 1072, 1077, 1095, 608, 1121, 1126, 1149, 1155, 1173, 1047 },
    { 1047, 1054, 1061, 1071, 1078, 1094, 1099, 1120, 1127, 1148, 1156, 1172, 1047 },
    { 1047, 1053, 1062, 602, 1079, 1093, 1100, 1119, 1128, 1147, 1157, 1171, 1047 },
    { 1047, 1052, 1063, 1070, 1080, 1092, 1101, 1118, 1129, 1146, 1158, 358, 1047 },
    { 1047, 357, 1064, 1069, 1081, 1091, 1102, 1117, 1130, 1145, 1159, 1170, 1047 },
    { 1047, 1050, 1065, 1068, 1082, 1090, 1103, 1116, 1131, 1144, 1160, 1047, 1047 },
    { 1048, 366, 1066, 1067, 1083, 1089, 1104, 1115, 1132, 1143, 1161, 1047, 1047 },
    { 1049, 1049, 641, 641, 641, 1088, 1105, 1114, 1133, 1142, 1162, 1047, 1047 },
    { 641, 641, 641, 641, 641, 1087, 1106, 1113, 1134, 1141, 1163, 1169, 1047 },
    { 641, 641, 641, 641, 641, 1086, 1107, 1112, 1135, 1140, 1164, 1168, 1047 },
    { 641, 641, 641, 641, 641, 1085, 1108, 1111, 1136, 1139, 1165, 1167, 1047 },
    { 641, 641, 641, 641, 641, 1084, 1109, 1110, 1137, 1138, 1166, 363, 1047 },
    { 641, 641, 641, 641, 641, 1047, 1047, 1047, 1047, 1047, 1047, 1047, 1047 },
};
static const unsigned short sZones1[4][4] =   // 0x00CF39C0
{
    { 641, 1177, 1181, 1185 },
    { 641, 1178, 1182, 1186 },
    { 641, 1179, 1183, 1187 },
    { 641, 1180, 1184, 1188 },
};
static const unsigned short sZones2[7][6] =   // 0x00CF39E0
{
    { 641, 1441, 1451, 1446, 1456, 641 },
    { 641, 1442, 1447, 1452, 1457, 641 },
    { 641, 1443, 1448, 1453, 1458, 641 },
    { 641, 1444, 1449, 1454, 1459, 641 },
    { 641, 1445, 1450, 1455, 1460, 641 },
    { 641, 641, 641, 641, 641, 641 },
    { 641, 641, 641, 641, 641, 641 },
};
static const unsigned short sZones3[2][3] =   // 0x00CF3A38
{
    { 641, 641, 641 },
    { 641, 1461, 1462 },
};
static const unsigned short sZones4[3][2] =   // 0x00CF3A48
{
    { 641, 641 },
    { 641, 1463 },
    { 641, 1463 },
};
static const unsigned short sZones5[3][2] =   // 0x00CF3A58
{
    { 641, 1785 },
    { 641, 641 },
    { 641, 1786 },
};

// 0x00CC4608: the name of the zone at (row, col) of a world's grid.
unsigned short ZoneName(int world, int row, int col)
{
    if (col < gWorldInfo[world].m_p0C || gWorldInfo[world].m_p10 < col
        || row < gWorldInfo[world].m_p14 || gWorldInfo[world].m_p18 < row)
        return 641;
    switch (world)
    {
    case 0:
        return sZones0[row][col];
    case 1:
        return sZones1[row][col];
    case 2:
        return sZones2[row][col];
    case 3:
        return sZones3[row][col];
    case 4:
        return sZones4[row][col];
    case 5:
        return sZones5[row][col];
    case -1:
    default:
        return 641;
    }
}

// 0x00CC4728
VIEntity::VIEntity(VIScene *scene)
    : m_DimensionId(-1)
{
    m_scene = scene;
    m_p28 = -1;
    m_actor = -1;
    m_resSlot = -1;
    m_model = 0;
    m_model2 = 0;
    m_res = 0;
    m_p20 = 0;
    m_p10 = 0;
    VIVect3 &p = m_pos;
    p.x = -1138.0f;
    p.y = -1138.0f;
    p.z = -1138.0f;
}

// 0x00CC47A0
VIEntity::~VIEntity()
{
    ReleaseActor();
}

// 0x00CC47F0
int VIEntity::IsSelectable()
{
    return 1;
}

// 0x00CC47F8
const VIVect3 &VIEntity::GetActorPos()
{
    return m_scene->GetActorLocation(m_actor);
}

// 0x00CC4818
void VIEntity::SetActorPos(const VIVect3 &v)
{
    if (m_actor != -1)
        m_scene->SetActorLocation(m_actor, v);
}

// 0x00CC4848
const VIVect3 &VIEntity::GetActorHpr()
{
    return m_scene->GetActorHPR(m_actor);
}

// 0x00CC4868
void VIEntity::SetActorHpr(const VIVect3 &v)
{
    if (m_actor != -1)
        m_scene->SetActorHPR(m_actor, v);
}

// 0x00CC4898
int VIEntity::GetDimension()
{
    return m_DimensionId;
}

// 0x00CC48A0
VIVect3 *VIEntity::GetPos()
{
    return &m_pos;
}

// 0x00CC48A8
void VIEntity::StorePos(const VIVect3 &p)
{
    m_pos = p;
}

// 0x00CC48C8
VIVect3 *VIEntity::GetHpr()
{
    return &m_hpr;
}

// 0x00CC48D0
void VIEntity::StoreHpr(const VIVect3 &h)
{
    m_hpr = h;
}

// 0x00CC48F0
void VIEntity::SetDimension(int d)
{
    if (m_DimensionId != d)
    {
        m_DimensionId = d;
        OnWorldLoaded();
    }
}

// 0x00CC4918
void VIEntity::SetPos(const VIVect3 &p)
{
    StorePos(p);
    SetActorPos(p);
}

// 0x00CC4958
void VIEntity::SetHpr(const VIVect3 &hpr)
{
    StoreHpr(hpr);
    SetActorHpr(hpr);
}

// 0x00CC4998
int VIEntity::GetModel()
{
    return m_model2 ? m_model2 : m_model;
}

// 0x00CC49B0
void VIEntity::SetModel(int m)
{
    m_model = m;
}

// 0x00CC49B8
void VIEntity::SetModel2(int m)
{
    m_model2 = m;
}

// 0x00CC49C0
float VIEntity::GetScale()
{
    return m_scale;
}

// 0x00CC49C8: 1/128 m, clamped to 24 bits, high byte first.
void VIEntity::Quantize(float v, unsigned char *Value)
{
    float f = floor(v * 128.0f);
    int FixedPoint = (int)VIMIN(8388607.0f, VIMAX(-8388608.0f, f));
    Value[0] = FixedPoint >> 16;
    Value[1] = FixedPoint >> 8;
    Value[2] = FixedPoint;
}

// 0x00CC4A78
void VIEntity::Dequantize(const unsigned char *Value, float &f)
{
    int FixedPoint = (int8)Value[0];
    FixedPoint <<= 8;
    FixedPoint |= Value[1];
    FixedPoint <<= 8;
    FixedPoint |= Value[2];
    VIASSERT(((int8)Value[0]) >= 0 || FixedPoint < 0);
    f = FixedPoint / 128.0f;
}























































// 0x00CC4B00: the position quantized, then the heading and pitch as a byte each.
void VIEntity::WriteUpdate(VIMemStream &s)
{
    // ONE counter for both loops: it crosses the first loop's calls, so it
    // takes a callee-saved register and the second loop's PRE'd `i + 1`
    // coalesces with it (an `int j` of its own gives a second register).
    int i;
    float *p = &m_pos.x;
    for (i = 0; i < 3; i++)
    {
        unsigned char Value[3];
        Quantize(*p++, Value);
        s.Write(Value, 1, 3);
    }
    for (i = 0; i < 2; i++)
    {
        while (m_hpr[i] >= PI)
            m_hpr[i] -= TWOPI;
        while (m_hpr[i] < -PI)
            m_hpr[i] += TWOPI;
        s.WriteByte((int)(m_hpr[i] / TWOPI * 256.0f));
    }
    WriteState(s);
}

// 0x00CC4CC8
void VIEntity::ReadUpdate(VIMemStream &s)
{
    int oldDim = m_DimensionId;
    int oldModel = m_model;
    float oldScale = m_scale;
    int i;
    for (i = 0; i < 3; i++)
    {
        unsigned char Value[3];
        s.Read(Value, 1, 3);
        Dequantize(Value, m_pos[i]);
    }
    m_hpr.x = 0.0f;
    m_hpr.y = 0.0f;
    m_hpr.z = 0.0f;
    for (i = 0; i < 2; i++)
    {
        signed char c;
        s.Read(c);
        m_hpr[i] = c * (PI / 128.0f);
    }
    ReadState(s);
    Changed(oldDim, oldModel, oldScale);
}

// 0x00CC4DE8
int VIEntity::Refresh()
{
    return 0;
}

// 0x00CC4DF0
void VIEntity::WriteState(VIMemStream &s)
{
    s.WriteByte(m_DimensionId);
    s.WriteU64(m_p10);
    s.WriteByte(m_p18);
    s.WriteByte(m_p19);
    s.WriteU32(m_model);
    s.WriteU32(m_model2);
    s.WriteF32(m_scale);
}

// 0x00CC4EE0
void VIEntity::ReadState(VIMemStream &s)
{
    signed char d;
    s.Read(&d, 1, 1);
    m_DimensionId = d;
    s.Read(&m_p10, 8, 1);
    s.Read(&m_p18, 1, 1);
    s.Read(&m_p19, 1, 1);
    s.Read(&m_model, 4, 1);
    s.Read(&m_model2, 4, 1);
    s.Read(&m_scale, 4, 1);
}

// 0x00CC4FA0
void VIEntity::Write(VIMemStream &s)
{
    s.Write(&m_pos, 4, 3);
    s.Write(&m_hpr, 4, 3);
    WriteState(s);
}

// 0x00CC5000
void VIEntity::Read(VIMemStream &s)
{
    float oldScale = m_scale;
    int oldDim = m_DimensionId;
    int oldModel = m_model;
    s.Read(&m_pos, 4, 3);
    s.Read(&m_hpr, 4, 3);
    ReadState(s);
    Changed(oldDim, oldModel, oldScale);
}

// 0x00CC5098: 129 a plain entity, 130 a creature.
VIEntity *CreateEntity(unsigned char type, unsigned int id, VIScene &sc, VIClientPart06D00 &res)
{
    VIEntity *pEntity = 0;
    switch (type)
    {
    case 129:
        pEntity = new VIEntity(&sc);
        break;
    case 130:
        {
            VIVect3 v;
            pEntity = new VICreatureEntity(&sc);
            ((VICreatureEntity *)pEntity)->m_walk.Init(v, v, sc);
        }
        break;
    }
    VIASSERT(pEntity != NULL);
    pEntity->m_id = id;
    pEntity->m_res = &res;
    pEntity->m_p20 = 0;
    return pEntity;
}























































































// 0x00CC5180
int VIEntity::IsType(unsigned char type)
{
    return type == 129;
}

// 0x00CC5190: the swing animation for an item's attack type.
int AttackAnim(int attackType, int alt)
{
    int anim = 23;
    switch (attackType)
    {
    case 0:
        anim = 23;
        break;
    case 3:
        anim = alt ? 40 : 41;
        break;
    case 1:
        anim = alt ? 24 : 25;
        break;
    case 4:
        anim = 42;
        break;
    case 2:
        anim = 26;
        break;
    case 5:
        anim = alt ? 27 : 28;
        break;
    case 6:
        anim = 29;
        break;
    case 7:
        anim = 31;
        break;
    case 10:
        anim = 36;
        break;
    case 8:
        anim = alt ? 32 : 35;
        break;
    case 9:
        anim = 33;
        break;
    }
    return anim;
}

// 0x00CC5238: a scene actor, through the character resources when the scene renders.
int VIEntity::CreateActor()
{
    if (m_scene->m_raster)
    {
        int slot = m_res->Create(GetModel(), m_pos, m_hpr, m_scale);
        m_resSlot = slot;
        if (slot < 0)
            return -1;
        m_actor = m_res->GetActor(slot);
    }
    else
    {
        VIDictionary *dict = m_scene->m_dict;
        VIResourceType type;
        int i;
        if (!dict->Find(m_model, type, i) && !dict->Find(VIHashResourceID("!Character"), type, i))
            return -1;
        m_actor = m_scene->CreateActor(m_pos, m_hpr, m_scale, i);
        if (m_actor < 0)
            return -1;
    }
    m_scene->SetActorID(m_actor, (int)this);
    return 0;
}

// 0x00CC5338
int VIEntity::ReleaseActor()
{
    if (m_res)
    {
        if (m_resSlot != -1)
        {
            m_scene->m_spellEffects.NotifyActorWasDestroyed(m_actor);
            m_res->Release(m_resSlot);
            m_resSlot = -1;
            m_actor = -1;
        }
    }
    else if (m_actor != -1)
    {
        m_scene->ReleaseActor(m_actor);
        m_actor = -1;
    }
    return 0;
}

// 0x00CC53C0: an actor while the entity is in the scene's dimension, none otherwise.
void VIEntity::OnWorldLoaded()
{
    if (GetSceneDimension() == m_DimensionId && m_actor == -1)
        CreateActor();
    else if (GetSceneDimension() != m_DimensionId && m_actor != -1)
        ReleaseActor();
}

// 0x00CC5438: after an update, follow what changed.
int VIEntity::Changed(int oldDim, int oldModel, float oldScale)
{
    if (oldDim != m_DimensionId)
        OnWorldLoaded();
    if (m_actor == -1)
        return 0;
    VIASSERT(GetSceneDimension() == m_DimensionId);
    int model = m_model2 ? m_model2 : m_model;
    if (m_scene->m_raster)
    {
        if (oldModel != model)
            m_res->SetModel(m_resSlot, model);
    }
    else if (oldModel != model)
    {
        VIDictionary *dict = m_scene->m_dict;
        VIResourceType type;
        int i;
        if (!dict->Find(model, type, i) && !dict->Find(VIHashResourceID("!Character"), type, i))
            return -1;
        m_scene->SetActorSprite(m_actor, i);
    }
    if (oldScale != m_scale)
        m_scene->SetActorScale(m_actor, m_scale);
    return 0;
}

// 0x00CC5580
int VIEntity::GetSceneDimension()
{
    return m_scene->m_p4BAC;
}
