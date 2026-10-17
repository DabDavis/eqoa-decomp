// Moving a creature between updates: CLIENT 0x00CBCD10 .. 0x00CBDEF8 (file name ours).
// ExtrapolateMove carries the position along the velocity and drops it onto the ground;
// ExtrapolateGround probes 1.5 m above and below for the ground and tips the creature to
// the slope (pitch for skeleton type 2, pitch and roll for 3), easing a corpse onto it
// over its first second dead.  The pitch/roll math is VIWalkControl::CalcPitchRollToLandscape's.

#include "game.h"

extern "C" float atan2f(float, float);
extern "C" float sqrtf(float);

// The clamps are vi.h's VIMIN(hi, VIMAX(lo, v)): the low test is evaluated twice.

// 0x00CBCD10
void ExtrapolateMove(VICreatureEntity *e, VIScene &scene, float dt)
{
    VIVect3 pos;
    VIVect3 hpr;
    VIVect3 a;
    VIVect3 b;
    VICollSphere sphere;
    VIAIExtrapolateFilter filter;
    filter.Init(e->m_actor);
    pos = *e->GetPos();
    pos.x += e->m_p218.x * dt;
    pos.y += e->m_p218.y * dt;
    pos.z += e->m_p218.z * dt;
    hpr = *e->GetHpr();
    ExtrapolateGround(e, pos, hpr, scene);
    e->StorePos(pos);
    e->StoreHpr(hpr);
}

// 0x00CBCE40
void ExtrapolateGround(VICreatureEntity *e, VIVect3 &pos, VIVect3 &hpr, VIScene &scene)
{
    VIVect3 a;
    VIVect3 b;
    VIVect3 n;
    VIVect3 fwd;
    VIVect3 r;
    VICollSphere sphere;
    VIAIExtrapolateFilter filter;
    VICSprite *spr = 0;
    int type;

    if (e->m_actor != -1 && scene.GetActorSpriteType(e->m_actor) == 5)
    {
        spr = (VICSprite *)scene.GetActorSpritePtr(e->m_actor);
        type = spr->m_skelType;
    }
    else
        type = 0;

    a = pos;
    a.y += 1.5f;
    b = pos;
    b.y -= 1.5f;
    sphere.Init(a, b, 0.1f);
    if (scene.Collide(sphere, filter))
    {
        VIVect3 hit = sphere.GetHitPoint();
        pos.x = hit.x;
        pos.y = hit.y - 0.1f;
        pos.z = hit.z;
    }

    if (filter.m_hitActor == -1)
    {
        switch (type)
        {
        case 2:
            n = sphere.GetHitNormal();
            fwd.HPR(hpr.x, 0.0f, 0.0f);
            r.x = n.y * fwd.z - n.z * fwd.y;
            r.y = -n.x * fwd.z + n.z * fwd.x;
            r.z = n.x * fwd.y - n.y * fwd.x;
            r.Normalize();
            fwd.x = r.y * n.z - r.z * n.y;
            fwd.y = -r.x * n.z + r.z * n.x;
            fwd.z = r.x * n.y - r.y * n.x;
            hpr.y = -atan2f(fwd.y, sqrtf(1.0f - fwd.y * fwd.y));
            hpr.z = 0.0f;
            break;
        case 3:
            n = sphere.GetHitNormal();
            fwd.HPR(hpr.x, 0.0f, 0.0f);
            r.x = n.y * fwd.z - n.z * fwd.y;
            r.y = -n.x * fwd.z + n.z * fwd.x;
            r.z = n.x * fwd.y - n.y * fwd.x;
            r.Normalize();
            fwd.x = r.y * n.z - r.z * n.y;
            fwd.y = -r.x * n.z + r.z * n.x;
            fwd.z = r.x * n.y - r.y * n.x;
            hpr.y = -atan2f(fwd.y, sqrtf(1.0f - fwd.y * fwd.y));
            hpr.z = atan2f(r.y, sqrtf(1.0f - r.y * r.y));
            break;
        default:
            hpr.y = 0.0f;
            hpr.z = 0.0f;
            break;
        }

        if (spr && spr->m_dead)
        {
            VIVect3 t;
            n = sphere.GetHitNormal();
            fwd.HPR(hpr.x, 0.0f, 0.0f);
            r.x = n.y * fwd.z - n.z * fwd.y;
            r.y = -n.x * fwd.z + n.z * fwd.x;
            r.z = n.x * fwd.y - n.y * fwd.x;
            r.Normalize();
            fwd.x = r.y * n.z - r.z * n.y;
            fwd.y = -r.x * n.z + r.z * n.x;
            fwd.z = r.x * n.y - r.y * n.x;
            t.x = hpr.x;
            t.y = -atan2f(fwd.y, sqrtf(1.0f - fwd.y * fwd.y));
            t.z = atan2f(r.y, sqrtf(1.0f - r.y * r.y));
            float k = (int)(scene.GetTime() - e->m_deathTime) * 0.001f;
            k = VIMIN(1.0f, VIMAX(0.0f, k));
            hpr.x = hpr.x + (t.x - hpr.x) * k;
            hpr.y = hpr.y + (t.y - hpr.y) * k;
            hpr.z = hpr.z + (t.z - hpr.z) * k;
        }
    }
    else
    {
        hpr.y = 0.0f;
        hpr.z = 0.0f;
    }
}
