// The auto-attack swing and the character resources: CLIENT 0x00CBDEF8 .. 0x00CC20F8,
// one TU (no template copies until its end).  File name ours.
#include "game.h"
#include "vipool_t.h"
#include "vimultimap_t.h"

template <> int VIMultiMap<VICharResKey, int>::Insert(const VICharResKey &k, const int &v);  // below

// 0x00CBDEF8: which swing the worn weapons make.  b16 (two-handed?) excludes b14, b15 and
// b18 (ranged), and b18 excludes b14 and b15; b19 means no effect.
int GetSwing(int cls, int level, int b14, int b15, int b16, int b18, int b17, int b19, int b20)
{
    if (b16 && (b14 || b15 || b18))
        return 0;
    if (b18 && (b14 || b15 || b16))
        return 0;
    switch (cls)
    {
    case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14:
        if (b18)
            return 8;
        else if (b16)
            return 1;
        else if (b19)
            return 12;
        else
            return 0;
    case 2: case 3:
        if (b18)
        {
            if (level < 20)
                return 8;
            else if (level < 35)
                return 9;
            else
                return 11;
        }
        else if (level < 20)
        {
            if (b16)
                return 1;
            else if (b19)
                return 12;
            else
                return 0;
        }
        else if (level < 35)
        {
            if (b16)
                return 3;
            else if (b19)
                return 12;
            else
                return 2;
        }
        else
        {
            if (b16)
                return 7;
            else if (b19)
                return 12;
            else
                return 2;
        }
    case 0: case 1: case 4:
        if (b18)
        {
            if (level < 20)
                return 8;
            else if (level < 35)
                return 9;
            else
                return 11;
        }
        else if (level < 20)
        {
            if (b16)
                return 1;
            else if (b19)
                return 12;
            else
                return 0;
        }
        else if (level < 35)
        {
            if (b16)
                return 3;
            else if (b19)
                return 12;
            else if (!b17 && !b20)
                return 4;
            else
                return 0;
        }
        else
        {
            if (b16)
                return 7;
            else if (b19)
                return 12;
            else if (!b17 && !b20)
                return 5;
            else
                return 2;
        }
    case 5: case 6:
        if (b18)
        {
            if (level < 20)
                return 8;
            else if (level < 35)
                return 9;
            else
                return 11;
        }
        else if (level < 20)
        {
            if (b16)
                return 1;
            else if (b19)
                return 12;
            else
                return 0;
        }
        else if (level < 35)
        {
            if (b16)
                return 1;
            else if (b19)
                return 12;
            else if (!b17 && !b20)
                return 4;
            else
                return 0;
        }
        else
        {
            if (b16)
                return 1;
            else if (b19)
                return 12;
            else if (!b17 && !b20)
                return 5;
            else
                return 2;
        }
    }
    return 0;
}

// 0x00CBE0A8: the swing's spell effect.
unsigned int SwingEffect(int swing)
{
    if (swing == 12)
        return 0;
    switch (swing)
    {
    case 0: case 1:
        return VIHashResourceID("Attack1");
    case 4:
        return VIHashResourceID("Attack2");
    case 2: case 3: case 6: case 7:
        return VIHashResourceID("Attack3");
    case 5:
        return VIHashResourceID("Attack4");
    case 8:
        return VIHashResourceID("Range1");
    case 9:
        return VIHashResourceID("Range2");
    case 10:
        return VIHashResourceID("Range3");
    case 11:
        return VIHashResourceID("Range4");
    }
    return 0;
}

// 0x00CBE190: the swing's delay, ms.
int SwingDelay(int swing)
{
    switch (swing)
    {
    case 0:
        return 3000;
    case 1: case 8:
        return 3500;
    case 2: case 4: case 6:
        return 4000;
    case 3: case 7: case 9: case 10: case 11:
        return 5000;
    case 5:
        return 6000;
    }
    return 3000;
}

// 0x00CBE1E8
int SwingDelay2(int type)
{
    switch (type)
    {
    case 12: case 14:
        return 1000;
    case 15:
        return 1500;
    case 16:
        return 1500;
    }
    return 0;
}

// ---- The character resources (VIClientPart06D00, game.h).  The element constructors come
// first in the image: they are not inline.

// 0x00CBE240
VIResHandle::VIResHandle()
{
    m_id = 0;
    m_fallback = 0;
    m_res = -1;
    m_load = -1;
}

// 0x00CBE260
VICharResItem::VICharResItem()
{
    m_type = 0;
}

// 0x00CBE298
VICharResBody::VICharResBody()
{
    m_name[0] = 0;
    m_nameColor = 0;
    m_face = -1;
    m_hair[0] = 0;
    m_hair[1] = 1;
    m_hair[2] = 1;
    m_robe[0] = 1;
    m_robe[1] = 0;
    m_robe[2] = 0;
    for (int i = 0; i < 6; i++)
    {
        m_armorSet[i] = 0;
        m_armorTint[i] = 0;
    }
}

// 0x00CBE318
VICharResState::VICharResState()
{
    m_flags = 0;
}

// 0x00CBE390
VICharRes::VICharRes()
{
    m_actor = -1;
    m_stamp = 0;
    m_rank = -1;
}

// 0x00CBE3D8
VIClientPart06D00::VIClientPart06D00()
{
    m_loader = 0;
    m_scene = 0;
    m_raster = 0;
    m_dict = 0;
    m_frame = 1;
    m_charType = VIHashResourceID("!Character");
    m_itemType = VIHashResourceID("!Item");
    m_maxFull = 16;
    m_maxLoaded = 24;
}

// 0x00CBE458
VIClientPart06D00::~VIClientPart06D00()
{
    Clear();
}

// 0x00CBE4B8
int VIClientPart06D00::Init(VIScene &sc, VILoader &ld)
{
    if (Clear() < 0)
        return -1;
    m_scene = &sc;
    m_raster = sc.m_raster;
    m_dict = sc.m_dict;
    m_loader = &ld;
    return 0;
}

// 0x00CBE520
int VIClientPart06D00::Clear()
{
    for (int i = m_pool.Begin(); i != m_pool.End(); i = m_pool.Next(i))
    {
        if (Release(i) < 0)
            return -1;
        if (m_pool.Value(i).m_cur.m_flags & 1)
        {
            if (ReleaseState(m_pool.Value(i).m_cur) < 0)
                return -1;
        }
        if (m_pool.Value(i).m_new.m_flags & 1)
        {
            if (ReleaseState(m_pool.Value(i).m_new) < 0)
                return -1;
        }
    }
    if (m_pool.Clear() < 0)
        return -1;
    if (m_lru.Clear() < 0)
        return -1;
    m_scene = 0;
    m_raster = 0;
    m_dict = 0;
    m_loader = 0;
    return 0;
}

// 0x00CBE640
int VIClientPart06D00::Create(int model, const VIVect3 &pos, const VIVect3 &hpr, float scale)
{
    int slot = m_pool.Add();
    if (slot < 0)
        return -1;
    m_pool.Value(slot).m_new.m_model.m_id = model;
    m_pool.Value(slot).m_new.m_flags |= 1;
    int actor = m_scene->CreateActor(pos, hpr, scale, m_scene->GetNullSprite());
    if (actor < 0)
        return -1;
    m_pool.Value(slot).m_actor = actor;
    return slot;
}

// 0x00CBE728
int VIClientPart06D00::Release(int slot)
{
    if (m_pool.Value(slot).m_actor != -1)
    {
        m_scene->ReleaseActor(m_pool.Value(slot).m_actor);
        m_pool.Value(slot).m_actor = -1;
    }
    return 0;
}

// 0x00CBE790
int VIClientPart06D00::GetActor(int slot)
{
    return m_pool.Value(slot).m_actor;
}

// 0x00CBE7A8
int VIClientPart06D00::SetVisible(int slot, int visible)
{
    if (m_pool.Value(slot).m_new.m_flags & 1)
    {
        int f = m_pool.Value(slot).m_new.m_flags & 2;
        if ((f == 0) == visible)
            return 0;
    }
    else
    {
        int f = m_pool.Value(slot).m_cur.m_flags & 2;
        if ((f == 0) == visible)
            return 0;
        if (BeginEdit(slot) < 0)
            return -1;
    }
    m_pool.Value(slot).m_new.m_flags |= 1;
    if (visible)
        m_pool.Value(slot).m_new.m_flags &= ~2;
    else
        m_pool.Value(slot).m_new.m_flags |= 2;
    return 0;
}

// 0x00CBE898
int VIClientPart06D00::SetName(int slot, const char *name, int color)
{
    if (m_pool.Value(slot).m_new.m_flags & 1)
    {
        if (!strcmp(m_pool.Value(slot).m_new.m_body.m_name, name)
            && m_pool.Value(slot).m_new.m_body.m_nameColor == color)
            return 0;
    }
    else
    {
        if (!strcmp(m_pool.Value(slot).m_cur.m_body.m_name, name)
            && m_pool.Value(slot).m_cur.m_body.m_nameColor == color)
            return 0;
        if (BeginEdit(slot) < 0)
            return -1;
    }
    m_pool.Value(slot).m_new.m_flags |= 1;
    VIString::strcpy8(m_pool.Value(slot).m_new.m_body.m_name, 32, name);
    m_pool.Value(slot).m_new.m_body.m_nameColor = color;
    return 0;
}

// 0x00CBE9C0
int VIClientPart06D00::SetModel(int slot, int model)
{
    if (m_pool.Value(slot).m_new.m_flags & 1)
    {
        if (m_pool.Value(slot).m_new.m_model.m_id == model)
            return 0;
    }
    else
    {
        if (m_pool.Value(slot).m_cur.m_model.m_id == model)
            return 0;
        if (BeginEdit(slot) < 0)
            return -1;
    }
    m_pool.Value(slot).m_new.m_flags |= 1;
    if (ReleaseRes(m_pool.Value(slot).m_new.m_model) < 0)
        return -1;
    if (ReleaseRes(m_pool.Value(slot).m_new.m_body.m_faceRes) < 0)
        return -1;
    m_pool.Value(slot).m_new.m_model.m_id = model;
    return 0;
}

// 0x00CBEAB8
int VIClientPart06D00::SetItem(int slot, int i, int model, int type, int model2)
{
    if (m_pool.Value(slot).m_new.m_flags & 1)
    {
        if (m_pool.Value(slot).m_new.m_items[i].m_model.m_id == model
            && m_pool.Value(slot).m_new.m_items[i].m_model2.m_id == model2)
            return 0;
    }
    else
    {
        if (m_pool.Value(slot).m_cur.m_items[i].m_model.m_id == model
            && m_pool.Value(slot).m_cur.m_items[i].m_model2.m_id == model2)
            return 0;
        if (BeginEdit(slot) < 0)
            return -1;
    }
    m_pool.Value(slot).m_new.m_flags |= 1;
    if (m_pool.Value(slot).m_new.m_items[i].m_model.m_id != model)
    {
        if (ReleaseRes(m_pool.Value(slot).m_new.m_items[i].m_model) < 0)
            return -1;
    }
    if (m_pool.Value(slot).m_new.m_items[i].m_model2.m_id != model2)
    {
        if (ReleaseRes(m_pool.Value(slot).m_new.m_items[i].m_model2) < 0)
            return -1;
    }
    m_pool.Value(slot).m_new.m_items[i].m_model.m_id = model;
    m_pool.Value(slot).m_new.m_items[i].m_type = type;
    m_pool.Value(slot).m_new.m_items[i].m_model2.m_id = model2;
    return 0;
}

// 0x00CBEC80
int VIClientPart06D00::SetFace(int slot, int face)
{
    if (m_pool.Value(slot).m_new.m_flags & 1)
    {
        if (m_pool.Value(slot).m_new.m_body.m_face == face)
            return 0;
    }
    else
    {
        if (m_pool.Value(slot).m_cur.m_body.m_face == face)
            return 0;
        if (BeginEdit(slot) < 0)
            return -1;
    }
    m_pool.Value(slot).m_new.m_flags |= 1;
    if (m_pool.Value(slot).m_new.m_body.m_face != face)
    {
        if (ReleaseRes(m_pool.Value(slot).m_new.m_body.m_faceRes) < 0)
            return -1;
    }
    m_pool.Value(slot).m_new.m_body.m_face = face;
    return 0;
}

// 0x00CBED78
int VIClientPart06D00::SetHair(int slot, int a, int b, int c)
{
    if (m_pool.Value(slot).m_new.m_flags & 1)
    {
        if (m_pool.Value(slot).m_new.m_body.m_hair[0] == a
            && m_pool.Value(slot).m_new.m_body.m_hair[1] == b
            && m_pool.Value(slot).m_new.m_body.m_hair[2] == c)
            return 0;
    }
    else
    {
        if (m_pool.Value(slot).m_cur.m_body.m_hair[0] == a
            && m_pool.Value(slot).m_cur.m_body.m_hair[1] == b
            && m_pool.Value(slot).m_cur.m_body.m_hair[2] == c)
            return 0;
        if (BeginEdit(slot) < 0)
            return -1;
    }
    m_pool.Value(slot).m_new.m_flags |= 1;
    m_pool.Value(slot).m_new.m_body.m_hair[0] = a;
    m_pool.Value(slot).m_new.m_body.m_hair[1] = b;
    m_pool.Value(slot).m_new.m_body.m_hair[2] = c;
    return 0;
}

// 0x00CBEE90
int VIClientPart06D00::SetRobe(int slot, int a, int b, int c)
{
    if (m_pool.Value(slot).m_new.m_flags & 1)
    {
        if (m_pool.Value(slot).m_new.m_body.m_robe[0] == a
            && m_pool.Value(slot).m_new.m_body.m_robe[1] == b
            && m_pool.Value(slot).m_new.m_body.m_robe[2] == c)
            return 0;
    }
    else
    {
        if (m_pool.Value(slot).m_cur.m_body.m_robe[0] == a
            && m_pool.Value(slot).m_cur.m_body.m_robe[1] == b
            && m_pool.Value(slot).m_cur.m_body.m_robe[2] == c)
            return 0;
        if (BeginEdit(slot) < 0)
            return -1;
    }
    m_pool.Value(slot).m_new.m_flags |= 1;
    m_pool.Value(slot).m_new.m_body.m_robe[0] = a;
    m_pool.Value(slot).m_new.m_body.m_robe[1] = b;
    m_pool.Value(slot).m_new.m_body.m_robe[2] = c;
    return 0;
}

// 0x00CBEFA8
int VIClientPart06D00::SetArmorSlot(int slot, int i, int set, int tint)
{
    if (m_pool.Value(slot).m_new.m_flags & 1)
    {
        if (m_pool.Value(slot).m_new.m_body.m_armorSet[i] == set
            && m_pool.Value(slot).m_new.m_body.m_armorTint[i] == tint)
            return 0;
    }
    else
    {
        if (m_pool.Value(slot).m_cur.m_body.m_armorSet[i] == set
            && m_pool.Value(slot).m_cur.m_body.m_armorTint[i] == tint)
            return 0;
        if (BeginEdit(slot) < 0)
            return -1;
    }
    m_pool.Value(slot).m_new.m_flags |= 1;
    m_pool.Value(slot).m_new.m_body.m_armorSet[i] = set;
    m_pool.Value(slot).m_new.m_body.m_armorTint[i] = tint;
    return 0;
}

// 0x00CBF0C0
int VIClientPart06D00::Update(const VICamera &cam)
{
    if (Rank(cam.m_position) < 0)
        return -1;
    if (Process() < 0)
        return -1;
    m_frame++;
    return 0;
}

// 0x00CBF118
int VIClientPart06D00::BeginEdit(int slot)
{
    int i;
    m_pool.Value(slot).m_new = m_pool.Value(slot).m_cur;
    m_pool.Value(slot).m_new.m_model.m_load = -1;
    for (i = 0; i < 3; i++)
    {
        m_pool.Value(slot).m_new.m_items[i].m_model.m_load = -1;
        m_pool.Value(slot).m_new.m_items[i].m_model2.m_load = -1;
    }
    m_pool.Value(slot).m_new.m_body.m_faceRes.m_load = -1;
    if (m_pool.Value(slot).m_new.m_model.m_res != -1)
        m_scene->ShareSprite(m_pool.Value(slot).m_new.m_model.m_res);
    for (i = 0; i < 3; i++)
    {
        if (m_pool.Value(slot).m_new.m_items[i].m_model.m_res != -1)
            m_scene->ShareSprite(m_pool.Value(slot).m_new.m_items[i].m_model.m_res);
        if (m_pool.Value(slot).m_new.m_items[i].m_model2.m_res != -1)
            m_scene->ShareSprite(m_pool.Value(slot).m_new.m_items[i].m_model2.m_res);
    }
    if (m_pool.Value(slot).m_new.m_body.m_faceRes.m_res != -1)
        m_scene->m_raster->ShareSurface(m_pool.Value(slot).m_new.m_body.m_faceRes.m_res);
    return 0;
}

// 0x00CBF340
int VIClientPart06D00::Apply(int slot)
{
    int changed = 0;
    VICSprite *sp = 0;
    int i;
    if (m_pool.Value(slot).m_actor != -1)
    {
        if (m_pool.Value(slot).m_new.m_model.m_id != m_pool.Value(slot).m_cur.m_model.m_id)
        {
            changed = 1;
            m_scene->SetActorSprite(m_pool.Value(slot).m_actor, m_pool.Value(slot).m_new.m_model.m_res);
        }
        if (m_scene->GetActorSpriteType(m_pool.Value(slot).m_actor) == 5)
            sp = (VICSprite *)m_scene->GetActorSpritePtr(m_pool.Value(slot).m_actor);
    }
    if (sp)
    {
        for (i = 0; i < 3; i++)
        {
            if (!changed && m_pool.Value(slot).m_new.m_items[i].m_model.m_id
                            == m_pool.Value(slot).m_cur.m_items[i].m_model.m_id)
                continue;
            if (m_pool.Value(slot).m_cur.m_items[i].m_model.m_res != -1 && !changed)
                sp->DetachItem((VICSpriteAttachSlot)i, *m_scene);
            if (m_pool.Value(slot).m_new.m_items[i].m_model.m_id)
            {
                if (sp->AttachItem(m_pool.Value(slot).m_new.m_items[i].m_model.m_res, (VICSpriteAttachSlot)i,
                                   (VICSpriteAttackType)m_pool.Value(slot).m_new.m_items[i].m_type,
                                   m_pool.Value(slot).m_new.m_items[i].m_model2.m_res, *m_scene) < 0)
                    return -1;
            }
        }
        if (changed || m_pool.Value(slot).m_new.m_body.m_face != m_pool.Value(slot).m_cur.m_body.m_face)
            sp->SetFace(m_pool.Value(slot).m_new.m_body.m_face, *m_scene);
        if (changed || m_pool.Value(slot).m_new.m_body.m_hair[0] != m_pool.Value(slot).m_cur.m_body.m_hair[0]
            || m_pool.Value(slot).m_new.m_body.m_hair[1] != m_pool.Value(slot).m_cur.m_body.m_hair[1]
            || m_pool.Value(slot).m_new.m_body.m_hair[2] != m_pool.Value(slot).m_cur.m_body.m_hair[2])
            sp->SetHair(m_pool.Value(slot).m_new.m_body.m_hair[0], m_pool.Value(slot).m_new.m_body.m_hair[1],
                        m_pool.Value(slot).m_new.m_body.m_hair[2], *m_scene);
        for (i = 0; i < 6; i++)
        {
            if (!changed && m_pool.Value(slot).m_new.m_body.m_armorSet[i] == m_pool.Value(slot).m_cur.m_body.m_armorSet[i]
                && m_pool.Value(slot).m_new.m_body.m_armorTint[i] == m_pool.Value(slot).m_cur.m_body.m_armorTint[i])
                continue;
            sp->SetArmorSlot((VICSpriteArmorSlot)i, (VICSpriteArmorSet)m_pool.Value(slot).m_new.m_body.m_armorSet[i],
                             (VICSpriteTint)m_pool.Value(slot).m_new.m_body.m_armorTint[i], *m_scene);
        }
        if (changed || m_pool.Value(slot).m_new.m_body.m_robe[1] != m_pool.Value(slot).m_cur.m_body.m_robe[1]
            || m_pool.Value(slot).m_new.m_body.m_robe[2] != m_pool.Value(slot).m_cur.m_body.m_robe[2])
            sp->SetRobe(m_pool.Value(slot).m_new.m_body.m_robe[0], m_pool.Value(slot).m_new.m_body.m_robe[1],
                        (VICSpriteTint)m_pool.Value(slot).m_new.m_body.m_robe[2], *m_scene);
        if (changed || strcmp(m_pool.Value(slot).m_new.m_body.m_name, m_pool.Value(slot).m_cur.m_body.m_name)
            || m_pool.Value(slot).m_new.m_body.m_nameColor != m_pool.Value(slot).m_cur.m_body.m_nameColor)
            sp->SetName(m_pool.Value(slot).m_new.m_body.m_name,
                        (VICSpriteNameColor)m_pool.Value(slot).m_new.m_body.m_nameColor, *m_scene);
        if (changed || ((m_pool.Value(slot).m_new.m_flags ^ m_pool.Value(slot).m_cur.m_flags) & 2))
            sp->SetSkinsVisible((m_pool.Value(slot).m_new.m_flags & 2) == 0, *m_scene);
    }
    if (ReleaseState(m_pool.Value(slot).m_cur) < 0)
        return -1;
    m_pool.Value(slot).m_cur = m_pool.Value(slot).m_new;
    m_pool.Value(slot).m_new.m_flags &= ~1;
    return 0;
}

// 0x00CBF818
int VIClientPart06D00::Rank(const VIVect3 &eye)
{
    VIVect3 d;
    int i;
    for (i = m_lru.Begin(); i != m_lru.End(); i = m_lru.Begin())
        m_lru.Erase(i);
    for (i = m_pool.Begin(); i != m_pool.End(); i = m_pool.Next(i))
    {
        VICharResKey k;
        if (m_pool.Value(i).m_actor != -1)
        {
            d = m_scene->GetActorLocation(m_pool.Value(i).m_actor);
            d -= eye;
            k.m_age = m_frame - m_pool.Value(i).m_stamp;
            k.m_dist2 = d.x * d.x + d.y * d.y + d.z * d.z;
            m_pool.Value(i).m_stamp = m_frame;
        }
        else
        {
            k.m_age = m_frame - m_pool.Value(i).m_stamp;
            k.m_dist2 = 0.0f;
        }
        if (m_lru.Insert(k, i) < 0)
            return -1;
    }
    int rank = 0;
    for (i = m_lru.Begin(); i != m_lru.End(); i = m_lru.Next(i))
        m_pool.Value(m_lru.Value(i)).m_rank = rank++;
    return 0;
}

// 0x00CBFA08
int VIClientPart06D00::Process()
{
    int i = m_pool.Begin();
    while (i != m_pool.End())
    {
        int next = m_pool.Next(i);
        int ready;
        if (m_pool.Value(i).m_rank < m_maxFull)
        {
            if (m_pool.Value(i).m_new.m_flags & 1)
            {
                if (LoadState(m_pool.Value(i).m_new, ready) < 0)
                    return -1;
                if (ready)
                    Apply(i);
            }
        }
        else if (m_pool.Value(i).m_rank < m_maxLoaded)
        {
            if (m_pool.Value(i).m_actor == -1)
            {
                if (m_pool.Value(i).m_cur.m_flags & 1)
                {
                    if (ReleaseState(m_pool.Value(i).m_cur) < 0)
                        return -1;
                }
                if (m_pool.Value(i).m_new.m_flags & 1)
                {
                    if (ReleaseState(m_pool.Value(i).m_new) < 0)
                        return -1;
                }
                if (m_pool.Erase(i) < 0)
                    return -1;
            }
            else if (m_pool.Value(i).m_new.m_flags & 1)
            {
                if (LoadState(m_pool.Value(i).m_new, ready) < 0)
                    return -1;
                if (ready)
                    Apply(i);
            }
        }
        else
        {
            if (m_pool.Value(i).m_cur.m_flags & 1)
            {
                if (ReleaseState(m_pool.Value(i).m_cur) < 0)
                    return -1;
            }
            if (m_pool.Value(i).m_new.m_flags & 1)
            {
                if (ReleaseState(m_pool.Value(i).m_new) < 0)
                    return -1;
            }
            if (m_pool.Value(i).m_actor == -1)
            {
                if (m_pool.Erase(i) < 0)
                    return -1;
            }
            else if ((m_pool.Value(i).m_cur.m_flags & 1) && (m_pool.Value(i).m_new.m_flags & 1))
                m_pool.Value(i).m_cur.m_flags &= ~1;
            else if (m_pool.Value(i).m_cur.m_flags & 1)
            {
                m_pool.Value(i).m_new = m_pool.Value(i).m_cur;
                m_pool.Value(i).m_cur.m_flags &= ~1;
            }
        }
        i = next;
    }
    return 0;
}

// 0x00CBFD08
int VIClientPart06D00::LoadState(VICharResState &s, int &ready)
{
    ready = 1;
    if (LoadRes(s.m_model, 0, m_charType, ready) < 0)
        return -1;
    for (int i = 0; i < 3; i++)
    {
        if (LoadRes(s.m_items[i].m_model, 2, m_itemType, ready) < 0)
            return -1;
        if (LoadRes(s.m_items[i].m_model2, 2, m_itemType, ready) < 0)
            return -1;
    }
    if (s.m_model.m_res != -1 && m_scene->GetSpriteType(s.m_model.m_res) == 5)
    {
        VICSprite *sp = (VICSprite *)m_scene->Sprite(s.m_model.m_res);
        if (sp->m_race != 9)
        {
            if (s.m_body.m_face == -1)
                return -1;
            s.m_body.m_faceRes.m_id = VICSpriteCust::GetFaceResourceID(s.m_body.m_face, sp->m_sex,
                                                                       (VICSpriteRace)sp->m_race);
            if (LoadRes(s.m_body.m_faceRes, 4, 0, ready) < 0)
                return -1;
        }
    }
    return 0;
}

// 0x00CBFE60
int VIClientPart06D00::ReleaseState(VICharResState &s)
{
    if (ReleaseRes(s.m_model) < 0)
        return -1;
    for (int i = 0; i < 3; i++)
    {
        if (ReleaseRes(s.m_items[i].m_model) < 0)
            return -1;
        if (ReleaseRes(s.m_items[i].m_model2) < 0)
            return -1;
    }
    if (ReleaseRes(s.m_body.m_faceRes) < 0)
        return -1;
    return 0;
}

// 0x00CBFF18
int VIClientPart06D00::LoadRes(VIResHandle &h, int pack, unsigned int fallback, int &ready)
{
    VIResourceType type;
    if (h.m_res == -1)
    {
        if (h.m_id)
        {
            if (h.m_load != -1)
            {
                if (m_loader->IsComplete(h.m_load))
                {
                    if (!m_dict->Find(h.m_id, type, h.m_res))
                        return -1;
                    h.m_fallback = 0;
                    m_scene->ShareResource(h.m_id);
                    m_loader->Release(h.m_load, *m_scene);
                    h.m_load = -1;
                }
                else
                    ready = 0;
            }
            else if (m_dict->Find(h.m_id, type, h.m_res))
            {
                h.m_fallback = 0;
                m_scene->ShareResource(h.m_id);
            }
            else if (m_loader && m_loader->IsResourceAvailable(pack, h.m_id))
            {
                if ((h.m_load = m_loader->Load(pack, h.m_id)) < 0)
                    return -1;
                ready = 0;
            }
            else
            {
                if (!m_dict->Find(fallback, type, h.m_res))
                    return -1;
                h.m_fallback = fallback;
                m_scene->ShareResource(fallback);
            }
        }
        else
        {
            h.m_fallback = 0;
            h.m_res = m_scene->GetNullSprite();
        }
    }
    return 0;
}

// 0x00CC0090
int VIClientPart06D00::ReleaseRes(VIResHandle &h)
{
    if (h.m_load != -1)
    {
        m_loader->Release(h.m_load, *m_scene);
        h.m_load = -1;
    }
    if (h.m_res != -1)
    {
        if (!h.m_fallback)
        {
            if (h.m_id)
                m_scene->ReleaseResource(h.m_id);
        }
        else
            m_scene->ReleaseResource(h.m_fallback);
        h.m_res = -1;
    }
    return 0;
}

// 0x00CC1CD8: this TU's copy of VIMultiMap::Insert is NOT SUPPORT's source: it compares
// through the caller's key (no `K key = k` copy -- for a struct key that copy is an
// ldl/ldr block move the image does not have).  An explicit specialization, so the shared
// template stays SUPPORT's.
template <> int VIMultiMap<VICharResKey, int>::Insert(const VICharResKey &k, const int &v)
{
    int p = 0;
    int side = 0;
    int c = m_root;

    if (c != 0)
    {
        Node *nodes = m_nodes;
        const VICharResKey &key = k;
        do
        {
            p = c;
            if (key < nodes[p].m_key)
            {
                side = 1;
                c = nodes[p].m_left;
            }
            else if (nodes[p].m_key < key)
            {
                side = 0;
                c = nodes[p].m_right;
            }
            else
            {
                side = 0;
                c = nodes[p].m_right;
            }
        } while (c != 0);
    }

    int x = AddNode(p, side);
    if (x < 0)
        return -1;

    int n = x;
    new (&NODE(n)->m_key) VICharResKey(k);
    new (&NODE(n)->m_value) int(v);
    while (x != m_root && NODE(NODE(x)->m_parent)->m_color != 0)
    {
        p = NODE(x)->m_parent;
        if (p == NODE(NODE(p)->m_parent)->m_left)
        {
            c = NODE(NODE(p)->m_parent)->m_right;
            if (NODE(c)->m_color != 0)
            {
                NODE(p)->m_color = 0;
                NODE(c)->m_color = 0;
                NODE(NODE(p)->m_parent)->m_color = 1;
                x = NODE(p)->m_parent;
            }
            else
            {
                if (x == NODE(p)->m_right)
                {
                    x = p;
                    LeftRotate(x);
                }
                p = NODE(x)->m_parent;
                NODE(p)->m_color = 0;
                NODE(NODE(p)->m_parent)->m_color = 1;
                RightRotate(NODE(p)->m_parent);
            }
        }
        else
        {
            c = NODE(NODE(p)->m_parent)->m_left;
            if (NODE(c)->m_color != 0)
            {
                NODE(p)->m_color = 0;
                NODE(c)->m_color = 0;
                NODE(NODE(p)->m_parent)->m_color = 1;
                x = NODE(p)->m_parent;
            }
            else
            {
                if (x == NODE(p)->m_left)
                {
                    x = p;
                    RightRotate(x);
                }
                p = NODE(x)->m_parent;
                NODE(p)->m_color = 0;
                NODE(NODE(p)->m_parent)->m_color = 1;
                LeftRotate(NODE(p)->m_parent);
            }
        }
    }
    NODE(m_root)->m_color = 0;
    return n;
}
