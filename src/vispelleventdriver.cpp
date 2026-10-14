// VISpellEventDriver -- SUPPORT 0x00475E68 .. 0x0047A978, its own TU.
//
// One event LIST of a spell effect while it is running: a cursor into the
// array, the events it has started that are still going, and the animation
// handles it waits on.  VISpellEffectDriver owns one of these per list.
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);
extern "C" int rand(void);

VISpellEventDriver::VISpellEventDriver(VISpellEffectDriver *d, VISpellEventList *l,
                                       int actor, VIScene *sc)
{
    m_driver = d;
    m_list = l;
    m_scene = sc;
    m_actor = actor;
    m_index = 0;
    m_wait = 0;
    m_projectile = 0;
    m_bolt0 = 0;
    m_bolt1 = 0;
    m_lightning = 0;
    m_action = -1;
    m_anim[0] = -1;
    m_anim[1] = -1;
    m_anim[2] = -1;
    m_anim[3] = -1;
}

// The two deletes in the middle are VILightning, not VISpellActiveEvent: the
// bolt pair the lightning event draws with is the driver's, and the active
// event at +0x30 that owns the lightning is NOT freed here.
VISpellEventDriver::~VISpellEventDriver()
{
    VISpellActiveEvent *e;

    while ((e = (VISpellActiveEvent *)m_active.RemHead()) != 0)
        delete e;
    while ((e = (VISpellActiveEvent *)m_inactive.RemHead()) != 0)
        delete e;

    delete m_wait;
    delete m_projectile;
    delete m_bolt0;
    delete m_bolt1;
}

VISpellActiveEvent *VISpellEventDriver::FindCreateEmitter(int id)
{
    for (VISpellActiveEvent *e = (VISpellActiveEvent *)m_active.m_head;
         e->m_next; e = (VISpellActiveEvent *)e->m_next)
        if (e->m_event->m_p04 == id && e->m_event->m_type == 3)
            return e;
    return 0;
}

VISpellActiveEvent *VISpellEventDriver::FindCreateSound(int id)
{
    for (VISpellActiveEvent *e = (VISpellActiveEvent *)m_active.m_head;
         e->m_next; e = (VISpellActiveEvent *)e->m_next)
        if (e->m_event->m_p04 == id && e->m_event->m_type == 11)
            return e;
    return 0;
}

VISpellActiveEvent *VISpellEventDriver::FindCreateSprite(int id)
{
    for (VISpellActiveEvent *e = (VISpellActiveEvent *)m_active.m_head;
         e->m_next; e = (VISpellActiveEvent *)e->m_next)
        if (e->m_event->m_p04 == id && e->m_event->m_type == 9)
            return e;
    return 0;
}

VISpellActiveEvent *VISpellEventDriver::FindSetEmitterMotif(int id, unsigned int motif)
{
    for (VISpellActiveEvent *e = (VISpellActiveEvent *)m_active.m_head;
         e->m_next; e = (VISpellActiveEvent *)e->m_next)
        if (e->m_event->m_type == 6 && e->m_event->m_p04 == id
            && e->m_blender->Motif()->NameHash() == motif)
            return e;
    return 0;
}

int VISpellEventDriver::NewWait(VISpellEvent *ev)
{
    m_wait = new VISpellActiveEvent(ev, m_scene);
    m_wait->m_time = m_scene->GetTime();
    return 0;
}

int VISpellEventDriver::NewVibrateController(VISpellEvent *ev)
{
    if (m_driver->m_playerActor != m_actor)
        return 0;
    if (m_driver->m_vibrate == 0)
        return 0;

    unsigned char power = 1;
    unsigned char time = 100;
    int mode = 0;

    switch (ev->m_p04)
    {
    case 0:
        power = (unsigned char)ev->m_id;
        time = (unsigned char)ev->m_p0C;
        mode = ev->m_p10;
        break;
    case 1:
        time = 80;
        mode = 200;
        break;
    case 2:
        time = 220;
        mode = 140;
        break;
    }

    m_driver->m_vibrate(power, time, mode, m_driver->m_vibrateUser);
    return 0;
}

int VISpellEventDriver::NewCreateEmitter(VISpellEvent *ev)
{
    VISpellActiveEvent *e = new VISpellActiveEvent(ev, m_scene);
    e->m_time = m_scene->GetTime();

    e->m_emitter0 = new VIParticleEmitter();

    VIParticleSystem *ps = m_scene->ParticleSystem();
    e->m_emitter0->Create(ps, ps->ParticleDefinition(ev->m_id));

    m_active.AddTail(e);
    // A chain, not two statements: gcc knows 0.0f is all-zero bits and stores
    // it with `sw zero` unless one assignment feeds the other, which is what
    // puts it in an FPR (Phase 14).  The stores come out LEFT to right here,
    // where VIMatrix44D's three-deep chain came out right to left (Phase 7).
    e->m_f9C = e->m_fA0 = 0.0f;
    return 0;
}

int VISpellEventDriver::NewSetEmitterAttractor(VISpellEvent *ev)
{
    VISpellActiveEvent *e = FindCreateEmitter(ev->m_p04);

    if (e == 0)
        return -1;
    e->m_p84 = ev->m_id;
    e->m_f88 = ev->m_f14;
    return 0;
}

int VISpellEventDriver::NewDestroyEmitter(VISpellEvent *ev)
{
    VISpellActiveEvent *e = FindCreateEmitter(ev->m_p04);

    if (e)
    {
        e->Remove();
        m_inactive.AddTail(e);
    }
    return 0;
}

int VISpellEventDriver::NewDestroySound(VISpellEvent *ev)
{
    VISpellActiveEvent *e = FindCreateSound(ev->m_p04);

    if (e)
    {
        e->Remove();
        delete e;
    }
    return 0;
}

int VISpellEventDriver::NewDestroySprite(VISpellEvent *ev)
{
    VISpellActiveEvent *e = FindCreateSprite(ev->m_p04);

    if (e)
    {
        e->Remove();
        delete e;
    }
    return 0;
}

int VISpellEventDriver::GetNodeTransform(int actor, VICSpriteNodeID node, VIMatrix44 &m)
{
    if (actor == -1)
        return -1;

    VISprite *s = m_scene->GetActorSpritePtr(actor);

    if (s == 0)
        return -1;

    if (s->m_type != 5)
        m_scene->GetActorTransform(actor, m);
    else
        ((VICSprite *)s)->CalcSlotWorldTransform(node, actor, *m_scene, m);
    return 0;
}

int VISpellEventDriver::ProcessInactiveEvents()
{
    VISpellActiveEvent *e = (VISpellActiveEvent *)m_inactive.m_head;

    while (e->m_next)
    {
        VISpellActiveEvent *n = (VISpellActiveEvent *)e->m_next;

        if (e->m_emitter0 && e->m_emitter0->ParticleCount() > 0)
        {
            if (e->m_event->m_type == 3)
                ProcessCreateEmitter(e, false);
        }
        else
        {
            e->Remove();
            delete e;
        }
        e = n;
    }
    return 0;
}

int VISpellEventDriver::ParticleCount() const
{
    int n = 0;
    VISpellActiveEvent *e;

    for (e = (VISpellActiveEvent *)m_active.m_head; e->m_next;
         e = (VISpellActiveEvent *)e->m_next)
        n += e->ParticleCount();
    for (e = (VISpellActiveEvent *)m_inactive.m_head; e->m_next;
         e = (VISpellActiveEvent *)e->m_next)
        n += e->ParticleCount();
    if (m_projectile)
        n += m_projectile->ParticleCount();
    return n;
}

int VISpellEventDriver::NewSetAction(VISpellEvent *ev)
{
    if (m_actor == -1)
        return 0;

    VICSprite *s = (VICSprite *)m_scene->GetActorSpritePtr(m_actor);

    if (s == 0)
        return 0;

    if (s->m_type != 5)
    {
        m_action = -1;
        m_anim[0] = -1;
        m_anim[1] = -1;
        m_anim[2] = -1;
        m_anim[3] = -1;
        return -1;
    }

    m_action = s->SetAction((VICSpriteAnimID)ev->m_p04, *m_scene);
    m_anim[0] = -1;
    m_anim[1] = -1;
    m_anim[2] = -1;
    m_anim[3] = -1;

    s->FindTrigger(VIHashResourceID("Spell FX 1"), m_anim[0]);
    s->FindTrigger(VIHashResourceID("Spell FX 2"), m_anim[1]);
    s->FindTrigger(VIHashResourceID("Spell FX 3"), m_anim[2]);
    s->FindTrigger(VIHashResourceID("Spell FX 4"), m_anim[3]);

    if (m_anim[0] != -1)
        s->ResetTrigger(m_anim[0]);
    if (m_anim[1] != -1)
        s->ResetTrigger(m_anim[1]);
    if (m_anim[2] != -1)
        s->ResetTrigger(m_anim[2]);
    if (m_anim[3] != -1)
        s->ResetTrigger(m_anim[3]);
    return 0;
}

int VISpellEventDriver::NewShakeCamera(VISpellEvent *ev)
{
    // The two guards are NOT the same shape: the actor test is an early
    // return, the camera-effect test an `if` whose body runs to the bottom.
    // With both nested, or both early, the join's `move v0,zero` and `lq ra`
    // come out the other way about.
    if (m_driver->m_playerActor != m_actor)
        return 0;

    VICameraEffect *ce = m_driver->m_cameraEffect;

    if (ce)
    {
        m_driver->m_cameraEffectOn = 1;

        // Both flags come out `li 1` + a `move` + `movz ...,zero`, which is
        // the assign-1-into-a-zero-initialised-local shape and NOT
        // `(x & bit) != 0` -- gcc turns that into `sra` + `andi 1` and never
        // materialises the mask at all (Phase 18's rule, both halves).
        int shake = 0;
        int terp = 0;

        if (ev->m_p04 & 0x10000)
            shake = 1;
        if (ev->m_p04 & 0x20000)
            terp = 1;

        // The speed is read BEFORE the shake test -- its `lbu` is hoisted
        // above the branch, which a read inside the `if` cannot be.
        int speed = (unsigned char)ev->m_id;
        float amp = ev->m_f14;
        float mag = ev->m_f18;
        float time = ev->m_f1C;

        if (shake)
        {
            switch (speed)
            {
            case 0:
                ce->Shake(VICameraEffect::SPEED_NONE, 0.0f);
                break;
            case 1:
                ce->Shake(VICameraEffect::SPEED_SLOW, amp);
                break;
            case 2:
                ce->Shake(VICameraEffect::SPEED_MEDIUM, amp);
                break;
            case 3:
                ce->Shake(VICameraEffect::SPEED_FAST, amp);
                break;
            }
        }
        if (terp)
            ce->ShakeTerp(mag, time);
    }
    return 0;
}

int VISpellEventDriver::NewPlayTargetInstant(VISpellEvent *ev)
{
    if (ev->m_p04 & 1)
        m_driver->AddSpellEventDriver(&m_driver->m_effect->m_targetEvents,
                                      m_driver->m_target);

    if (ev->m_p04 & 2)
        for (int i = 0; i < m_driver->m_numSecondary; i++)
            m_driver->AddSpellEventDriver(&m_driver->m_effect->m_areaEvents,
                                          m_driver->m_secondary[i]);
    return 0;
}

int VISpellEventDriver::NewSetEmitterMotif(VISpellEvent *ev)
{
    VISpellActiveEvent *ce = FindCreateEmitter(ev->m_p04);

    if (ce == 0)
        return -1;

    VISpellActiveEvent *e = new VISpellActiveEvent(ev, m_scene);
    e->m_time = m_scene->GetTime();

    VISpellActiveEvent *old = FindSetEmitterMotif(ev->m_p04, ev->m_id);

    if (old)
    {
        // The blend in flight is taken over, not restarted: the new event
        // inherits the blender and starts from the factor it had reached.
        VIMotifBlender *b = old->m_blender;

        e->m_blender = b;
        e->m_f9C = b->Factor();
        old->m_blender = 0;
        old->Remove();
        m_inactive.AddTail(old);
    }
    else
    {
        e->m_blender = new VIMotifBlender();
        ce->m_emitter0->LinkMotifBlender(ev->m_id, *e->m_blender);
        e->m_f9C = 0.0f;
    }
    m_active.AddTail(e);
    return 0;
}

int VISpellEventDriver::ProcessSetEmitterMotif(VISpellActiveEvent *e)
{
    VISpellEvent *ev = e->m_event;

    if (ev->m_p0C < (int)(m_scene->GetTime() - e->m_time))
    {
        if (e->m_blender->Factor() != ev->m_f14)
            e->m_blender->SetFactor(ev->m_f14);
    }
    else
    {
        float f = (float)(m_scene->GetTime() - e->m_time) / (float)ev->m_p0C;

        f = e->m_f9C + (ev->m_f14 - e->m_f9C) * f;
        e->m_blender->SetFactor(VIMIN(VIMAX(0.0f, f), 1.0f));
    }

    if (e->m_blender->Motif() == 0)
    {
        e->Remove();
        delete e;
    }
    return 0;
}

// The five wait kinds.  Every one of them retires the wait itself -- there is
// no shared tail after the switch, because the switch's own out-of-range test
// targets the bare `return 0` that the retire falls THROUGH into, and a tail
// after the switch would be on the default's path.  gcc cross-joins the five
// copies down to one.
int VISpellEventDriver::ProcessWaitEvent()
{
    // ONE sprite pointer for cases 0 and 3: a per-case local is allocated
    // straight into a0 for IsActionComplete, where the target keeps it in a2.
    VICSprite *sp;

    if (m_wait)
    {
        switch (m_wait->m_event->m_p04)
        {
        case 0:
            {
                if (m_actor != -1)
                {
                    sp = (VICSprite *)m_scene->GetActorSpritePtr(m_actor);

                    if (sp && sp->m_type == 5 && m_action != -1
                        && sp->IsActionComplete(m_action) == 0)
                        return 0;
                }
                delete m_wait;
                m_wait = 0;
                return 0;
            }

        case 1:
            {
                int el = (int)(m_scene->GetTime() - m_wait->m_time);

                if (m_wait->m_event->m_id < el)
                {
                    delete m_wait;
                    m_wait = 0;
                }
                return 0;
            }

        case 2:
            {
                int el = (int)(m_scene->GetTime() - m_driver->m_triggerTime);

                if (m_driver->m_arg0 < el)
                {
                    delete m_wait;
                    m_wait = 0;
                }
                return 0;
            }

        case 3:
            {
                if (m_actor == -1)
                {
                    delete m_wait;
                    m_wait = 0;
                    return 0;
                }

                sp = (VICSprite *)m_scene->GetActorSpritePtr(m_actor);

                if (sp == 0)
                {
                    delete m_wait;
                    m_wait = 0;
                    return 0;
                }

                if (sp->m_type != 5)
                {
                    delete m_wait;
                    m_wait = 0;
                }
                else
                {
                    int n = m_anim[m_wait->m_event->m_id];

                    if (n != -1 && sp->HasTripped(n))
                    {
                        delete m_wait;
                        m_wait = 0;
                    }
                }

                if (m_wait == 0)
                    return 0;

                int el = (int)(m_scene->GetTime() - m_wait->m_time);

                if (m_wait->m_event->m_p0C < el)
                {
                    delete m_wait;
                    m_wait = 0;
                }
                return 0;
            }

        case 4:
            {
                int el = (int)(m_scene->GetTime() - m_driver->m_triggerTime);

                if (m_driver->m_arg1 < el)
                {
                    delete m_wait;
                    m_wait = 0;
                }
                return 0;
            }
        }
        return 0;
    }
    return -1;
}

// The state machine.  A projectile BLOCKS everything; lightning and a wait do
// not.  Exactly one new event is dispatched per call, so two consecutive
// non-Wait events are a frame apart.
//
// Both switches are jump tables, so the CASE LABELS come out of the table and
// the case ORDER out of the body addresses (Phase 34) -- which is why the
// active switch runs emitter, sound, sprite, motif, fade and the dispatch
// switch runs 1, 2, 3, 6, 5, 4, 7, 8, 11, 12, 9, 10, 13..17.
int VISpellEventDriver::Process()
{
    if (m_projectile)
    {
        ProcessProjectileEvent();
        if (m_projectile)
            return 1;
    }
    if (m_lightning)
        ProcessLightningEvent();
    if (m_wait)
        ProcessWaitEvent();

    ProcessInactiveEvents();

    VISpellActiveEvent *e = (VISpellActiveEvent *)m_active.m_head;

    while (e->m_next)
    {
        VISpellActiveEvent *n = (VISpellActiveEvent *)e->m_next;

        switch (e->m_event->m_type)
        {
        case 3:
            ProcessCreateEmitter(e, true);
            break;
        case 11:
            ProcessCreateSound(e, true);
            break;
        case 9:
            ProcessCreateSprite(e, true);
            break;
        case 6:
            ProcessSetEmitterMotif(e);
            break;
        case 14:
            ProcessFadeSprite(e);
            break;
        }
        e = n;
    }

    if (m_wait == 0 && m_index < m_list->m_count)
    {
        VISpellEvent *ev = &m_list->m_events[m_index];

        switch (ev->m_type)
        {
        case 1:
            NewWait(ev);
            break;
        case 2:
            NewSetAction(ev);
            break;
        case 3:
            NewCreateEmitter(ev);
            break;
        case 6:
            NewSetEmitterMotif(ev);
            break;
        case 5:
            NewSetEmitterAttractor(ev);
            break;
        case 4:
            NewDestroyEmitter(ev);
            break;
        case 7:
            NewPlayTargetInstant(ev);
            break;
        case 8:
            NewPlayTargetProjectile(ev);
            break;
        case 11:
            NewCreateSound(ev);
            break;
        case 12:
            NewDestroySound(ev);
            break;
        case 9:
            NewCreateSprite(ev);
            break;
        case 10:
            NewDestroySprite(ev);
            break;
        case 13:
            NewPlayTargetLightning(ev);
            break;
        case 14:
            NewFadeSprite(ev);
            break;
        case 15:
            NewPlayAttack(ev);
            break;
        case 16:
            NewVibrateController(ev);
            break;
        case 17:
            NewShakeCamera(ev);
            break;
        }
        m_index++;
    }

    // Nested, not five `if (x) return 1;` -- the innermost test if-converts to
    // `sltu` when it is the last statement of a flat chain, and the target
    // branches (Phase 17's VIXm::IsCached, the same shape).
    if (m_index == m_list->m_count)
        if (m_active.m_head->m_next == 0)
            if (m_inactive.m_head->m_next == 0)
                if (m_wait == 0)
                    if (m_lightning == 0)
                        return 0;
    return 1;
}

// The four big traversals phase 38 left as stubs (609, 420, 399 and 1187
// instructions), written in phase 61.
// The projectile: an active event per target, an optional stand-in actor
// carrying the caster's item projectile (pinned at y = -8000 until the flight
// places it), a trail emitter, and the flight state for the four modes.  Mode
// 3 runs two endpoints, expressed in the target's local frame.  `~x` is the
// source's test for "x is not -1": it is a `nor`, where `x != -1` is a `li`
// and a compare.  Shipped: the sprite type is read again after the switch
// with no null test -- an actor without a sprite reads `((VISprite *)0)->m_type`.
int VISpellEventDriver::NewPlayTargetProjectile(VISpellEvent *ev)
{
    VIMatrix44 m;
    VIMatrix44 m2;
    VIVect3    a;
    VIVect3    b;
    VIVect3    c;

    for (int i = 0; i < m_driver->m_numSecondary + 1; i++)
    {
        VISpellEventDriver *d = 0;

        if (ev->m_p04 & 1)
            if (i == 0)
                d = m_driver->AddSpellEventDriver(
                        &m_driver->m_effect->m_targetEvents, m_driver->m_target);

        if (ev->m_p04 & 2)
            if (i != 0)
                d = m_driver->AddSpellEventDriver(
                        &m_driver->m_effect->m_areaEvents,
                        m_driver->m_secondary[i - 1]);

        if (m_actor != -1 && d != 0)
        {
            VISpellActiveEvent *e = new VISpellActiveEvent(ev, m_scene);
            e->m_time = m_scene->GetTime();

            VISprite *sp = m_scene->GetActorSpritePtr(m_actor);

            if (sp && sp->m_type == 5 && ((ev->m_p0C >> 12) & 1))
            {
                VICSprite *cs = (VICSprite *)m_scene->GetActorSpritePtr(m_actor);

                if (cs == 0)
                    return -1;

                int s = -1;

                if (~cs->m_items[0].m_projectile)
                    s = cs->m_items[0].m_projectile;
                else if (~cs->m_items[1].m_projectile)
                    s = cs->m_items[1].m_projectile;

                if (s >= 0)
                {
                    e->m_actor0 = m_scene->CreateActor(VIVect3(0.0f, -8000.0f, 0.0f),
                                                       VIVect3(), 1.0f, s);
                    m_scene->SetActorCollideable(e->m_actor0, 0);
                }
            }

            if (ev->m_id >= 0)
            {
                e->m_emitter0 = new VIParticleEmitter();

                VIParticleSystem *ps = m_scene->ParticleSystem();
                e->m_emitter0->Create(ps, ps->ParticleDefinition(ev->m_id));
            }

            switch ((ev->m_p0C >> 8) & 0xF)
            {
            case 0:
                break;
            case 1:
                if ((ev->m_p0C & 0xFF) == 3)
                    e->m_f9C = 0.0f;
                else
                    e->m_f9C = (float)rand() / 5347.0f;
                e->m_fA0 = 0.0f;
                break;
            case 2:
                e->m_f9C = 1.0f;
                break;
            }

            if (sp->m_type != 5)
            {
                e->m_v14 = m_scene->GetActorLocation(m_actor);
                e->m_c[0].Zero();
            }
            else
            {
                switch ((unsigned char)ev->m_p0C)
                {
                case 0:
                    GetNodeTransform(m_actor, (VICSpriteNodeID)0, m);
                    e->m_v14 = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
                    e->m_c[0].Zero();
                    break;
                case 1:
                    GetNodeTransform(m_actor, (VICSpriteNodeID)1, m);
                    e->m_v14 = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
                    e->m_c[0].Zero();
                    break;
                case 2:
                    GetNodeTransform(m_actor, (VICSpriteNodeID)0, m);
                    a = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
                    GetNodeTransform(m_actor, (VICSpriteNodeID)1, m);
                    b = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
                    e->m_v14 = a;
                    e->m_v14 += b;
                    e->m_v14 *= 0.5f;
                    e->m_c[0].Zero();
                    break;
                case 3:
                    GetNodeTransform(m_actor, (VICSpriteNodeID)0, m);
                    a = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
                    GetNodeTransform(m_actor, (VICSpriteNodeID)1, m);
                    b = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
                    e->m_v14 = a;
                    e->m_v14 += b;
                    e->m_v14 *= 0.5f;

                    m_scene->GetActorTransform(m_actor, m);
                    m2.Transpose(m);
                    m2.m[3].x = -(m2.m[0].w * m2.m[0].x + m2.m[1].w * m2.m[1].x
                                  + m2.m[2].w * m2.m[2].x);
                    m2.m[3].y = -(m2.m[0].w * m2.m[0].y + m2.m[1].w * m2.m[1].y
                                  + m2.m[2].w * m2.m[2].y);
                    m2.m[3].z = -(m2.m[0].w * m2.m[0].z + m2.m[1].w * m2.m[1].z
                                  + m2.m[2].w * m2.m[2].z);
                    m2.m[0].w = m2.m[1].w = m2.m[2].w = 0.0f;

                    m2.Mul(a, e->m_a[0]);
                    m2.Mul(b, e->m_a[1]);
                    m2.Mul(e->m_v14, c);
                    e->m_a[0] -= c;
                    e->m_a[1] -= c;

                    if ((ev->m_p0C >> 12) & 1)
                    {
                        VICSprite *cs = (VICSprite *)m_scene->GetActorSpritePtr(m_actor);

                        if (cs == 0)
                            return -1;

                        int s = -1;

                        if (~cs->m_items[0].m_projectile)
                            s = cs->m_items[0].m_projectile;
                        else if (~cs->m_items[1].m_projectile)
                            s = cs->m_items[1].m_projectile;

                        if (s >= 0)
                        {
                            e->m_actor1 = m_scene->CreateActor(
                                VIVect3(0.0f, -8000.0f, 0.0f), VIVect3(), 1.0f, s);
                            m_scene->SetActorCollideable(e->m_actor1, 0);
                        }
                    }

                    if (ev->m_id >= 0)
                    {
                        e->m_emitter1 = new VIParticleEmitter();

                        VIParticleSystem *ps = m_scene->ParticleSystem();
                        e->m_emitter1->Create(ps, ps->ParticleDefinition(ev->m_id));
                    }

                    e->m_c[0] = e->m_a[0];
                    e->m_c[1] = e->m_a[1];
                    break;
                }
            }

            e->m_v20 = e->m_v14;
            d->m_projectile = e;
        }
    }
    return 0;
}
// ProcessCreateSprite's actor choice twice -- once off the event's flags and
// once off the attractor word -- into ONE `actor` and ONE `sp` declared above
// both, which is what keeps the attractor's sprite in a callee-saved register.
// An unresolvable actor skips the attractor as well, straight to the expiry.
int VISpellEventDriver::ProcessCreateEmitter(VISpellActiveEvent *e, bool active)
{
    VIVect3       pos;
    VIVect3       off;
    VIMatrix44    m;
    VISpellEvent *ev = e->m_event;

    if (m_actor != -1)
    {
        VISprite *sp;
        int       actor;

        if (active == true)
        {
            if (ev->m_p0C & 0x10000)
                actor = m_driver->m_caster;
            else
                actor = m_actor;
            if (ev->m_p0C & 0x20000)
                actor = m_driver->m_target;

            if (actor == -1)
                goto expire;

            {
                sp = m_scene->GetActorSpritePtr(actor);

                if (sp)
                {
                    VIMatrix44 m2;
                    VIMatrix33 r;

                    if (sp->m_type != 5 || (ev->m_p0C & 0xFFFF) == 0xFFFF)
                        m_scene->GetActorTransform(actor, m2);
                    else
                        ((VICSprite *)sp)->CalcSlotWorldTransform(
                            (VICSpriteNodeID)(ev->m_p0C & 0xFFFF), actor,
                            *m_scene, m2);

                    int shape = (ev->m_p0C >> 20) & 3;

                    if (shape)
                    {
                        VIVect3 v;
                        float angle = e->m_f9C;
                        float t = (float)sin(e->m_fA0) * 0.5f + 0.5f;

                        switch (shape)
                        {
                        case 1:
                            m.RotateHPR(VIVect3(0.0f, angle, 0.0f));
                            v = VIVect3(ev->m_f14 + (ev->m_f18 - ev->m_f14) * t,
                                        ev->m_f24, 0.0f);
                            break;
                        case 2:
                            m.RotateHPR(VIVect3(angle, 0.0f, 0.0f));
                            v = VIVect3(0.0f, ev->m_f14 + (ev->m_f18 - ev->m_f14) * t,
                                        ev->m_f24);
                            break;
                        case 3:
                            m.RotateHPR(VIVect3(0.0f, 0.0f, angle));
                            v = VIVect3(ev->m_f24, 0.0f,
                                        ev->m_f14 + (ev->m_f18 - ev->m_f14) * t);
                            break;
                        }

                        m.Mul(v, off);

                        e->m_f9C += ev->m_f20 * 2.0f
                                    * m_scene->m_particleSystem.DeltaTime();
                        e->m_fA0 += ev->m_f1C * m_scene->m_particleSystem.DeltaTime();

                        if (6.2831855f < e->m_f9C)
                            e->m_f9C = e->m_f9C - 6.2831855f;
                        else if (e->m_f9C < -6.2831855f)
                            e->m_f9C = e->m_f9C + 6.2831855f;

                        if (6.2831855f < e->m_fA0)
                            e->m_fA0 = e->m_fA0 - 6.2831855f;
                        else if (e->m_fA0 < -6.2831855f)
                            e->m_fA0 = e->m_fA0 + 6.2831855f;

                        m2.Mul(off, pos);
                    }
                    else
                        pos = VIVect3(m2.m[3].x, m2.m[3].y, m2.m[3].z);

                    r.m[0].x = m2.m[0].x;
                    r.m[0].y = m2.m[0].y;
                    r.m[0].z = m2.m[0].z;
                    r.m[1].x = m2.m[1].x;
                    r.m[1].y = m2.m[1].y;
                    r.m[1].z = m2.m[1].z;
                    r.m[2].x = m2.m[2].x;
                    r.m[2].y = m2.m[2].y;
                    r.m[2].z = m2.m[2].z;

                    e->m_emitter0->SetLocation(pos);
                    e->m_emitter0->SetDirection(r);
                    e->m_emitter0->Update();
                }
            }
        }

        {
            if (e->m_p84 != -1)
            {
                if (e->m_p84 & 0x10000)
                    actor = m_driver->m_caster;
                else
                    actor = m_actor;
                if (e->m_p84 & 0x20000)
                    actor = m_driver->m_target;

                sp = m_scene->GetActorSpritePtr(actor);

                if (sp)
                {
                    VIMatrix44 m3;

                    if (sp->m_type != 5 || (e->m_p84 & 0xFFFF) == 0xFFFF)
                        m_scene->GetActorTransform(actor, m3);
                    else
                        ((VICSprite *)sp)->CalcSlotWorldTransform(
                            (VICSpriteNodeID)(e->m_p84 & 0xFFFF), actor,
                            *m_scene, m3);

                    e->m_emitter0->SetAttractor(
                        VIVect3(m3.m[3].x, m3.m[3].y, m3.m[3].z), e->m_f88);
                }
            }
            else
                e->m_emitter0->ClearAttractor();
        }
    }

expire:
    if (active == true && ev->m_p10 != 0
        && ev->m_p10 < (int)(m_scene->GetTime() - e->m_time))
    {
        e->Remove();
        m_inactive.AddTail(e);
    }
    return 0;
}
// The bolt pair: endpoint mode 0/1 is a single node of the source actor, 2 the
// midpoint of its two nodes, 3 a second bolt from the other node; the far end
// is always the midpoint of this driver's actor's two nodes.  While the event
// lasts the bolts are recharged; after it, the event and both bolts go once
// either bolt has faded (`alive` starts {1, 1}, copied out of .rodata).
int VISpellEventDriver::ProcessLightningEvent()
{
    VIVect3     a;
    VIVect3     b;
    VIMatrix44  m;
    VIVect3     src[2];
    VIVect3     dst;
    int         alive[2] = { 1, 1 };

    VIRaster *r = m_scene->m_raster;
    VISpellEvent *ev = m_lightning->m_event;
    int actor = m_lightning->m_actor0;

    if (m_actor == -1 || actor == -1)
    {
        alive[0] = 0;
        alive[1] = 0;
    }
    else
    {
        if (m_scene->GetActorSpritePtr(m_actor) == 0)
            return -1;

        switch ((unsigned char)ev->m_id)
        {
        case 0:
            GetNodeTransform(actor, (VICSpriteNodeID)0, m);
            src[0] = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
            break;
        case 1:
            GetNodeTransform(actor, (VICSpriteNodeID)1, m);
            src[0] = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
            break;
        case 2:
            GetNodeTransform(actor, (VICSpriteNodeID)0, m);
            a = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
            GetNodeTransform(actor, (VICSpriteNodeID)1, m);
            b = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
            src[0].x = (a.x + b.x) * 0.5f;
            src[0].y = (a.y + b.y) * 0.5f;
            src[0].z = (a.z + b.z) * 0.5f;
            break;
        case 3:
            GetNodeTransform(actor, (VICSpriteNodeID)0, m);
            src[0] = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
            GetNodeTransform(actor, (VICSpriteNodeID)1, m);
            src[1] = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
            break;
        }

        GetNodeTransform(m_actor, (VICSpriteNodeID)0, m);
        a = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
        GetNodeTransform(m_actor, (VICSpriteNodeID)1, m);
        b = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
        dst.x = (a.x + b.x) * 0.5f;
        dst.y = (a.y + b.y) * 0.5f;
        dst.z = (a.z + b.z) * 0.5f;

        if (m_bolt0 == 0)
            m_bolt0 = new VILightning;
        if ((unsigned char)ev->m_id == 3 && m_bolt1 == 0)
            m_bolt1 = new VILightning;

        if ((int)(m_scene->GetTime() - m_lightning->m_time) <= ev->m_p0C)
        {
            m_bolt0->SetCharge(1.0f);
            if (m_bolt1)
                m_bolt1->SetCharge(1.0f);
        }

        {
            VIColor32F c(ev->m_f14, ev->m_f18, ev->m_f1C, 1.0f);

            m_bolt0->SetColor(c);
        }
        m_bolt0->SetLocation(src[0], dst);
        m_bolt0->SetMaxBranchCount(ev->m_p10);
        m_bolt0->Update((int)(m_scene->m_particleSystem.DeltaTime() * 1000.0f));
        m_bolt0->Render(*r);
        alive[0] = 0.0f < m_bolt0->Charge();

        if (m_bolt1)
        {
            m_bolt1->SetColor(VIColor32F(ev->m_f14, ev->m_f18, ev->m_f1C, 1.0f));
            m_bolt1->SetLocation(src[1], dst);
            m_bolt1->SetMaxBranchCount(ev->m_p10);
            m_bolt1->Update((int)(m_scene->m_particleSystem.DeltaTime() * 1000.0f));
            m_bolt1->Render(*r);
            alive[1] = 0.0f < m_bolt1->Charge();
        }
    }

    if (ev->m_p0C < (int)(m_scene->GetTime() - m_lightning->m_time))
    {
        if (!alive[0] || !alive[1])
        {
            m_lightning->m_actor0 = -1;
            delete m_lightning;
            m_lightning = 0;
            if (m_bolt0)
            {
                delete m_bolt0;
                m_bolt0 = 0;
            }
            if (m_bolt1)
            {
                delete m_bolt1;
                m_bolt1 = 0;
            }
        }
    }
    return 0;
}
// The vector helpers the projectile flight is written with: a difference
// returned BY VALUE (its temporary is what Magnitude is handed) and a cross
// product built in a temporary and then assigned, which is why the one whose
// destination is also an operand still comes out right.
static inline VIVect3 Diff(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline VIVect3 Sum(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline VIVect3 Cross(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.y * b.z - a.z * b.y, -a.x * b.z + a.z * b.x, a.x * b.y - a.y * b.x);
}

// One frame of flight.  The projectile flies at Floats[0] toward the target's
// node 4; the emitter positions are offsets in a frame built on the flight
// direction, scaled in over the first 1/0.6 units.  Mode 1 orbits, mode 2
// jitters between random points every 1/15 s, mode 3 adds a sine arc.  It
// arrives when it has stopped closing in AND is within 3 units, and moves to
// the inactive list to drain.  The zero the basis vectors start from is a
// float VARIABLE (cse loads it back out of the first zeroed vector), and it is
// the same variable the 0.6 ramp is computed into.
int VISpellEventDriver::ProcessProjectileEvent()
{
    VIMatrix44 m;
    VIMatrix44 m2;
    VIVect3    a;
    VIVect3    p0;
    VIVect3    p1;
    VIVect3    tgt;
    VIVect3    d;
    VIVect3    q0;
    VIVect3    q1;
    float      z = 0.0f;
    VIVect3    right(z, z, z);
    VIVect3    up(z, 1.0f, z);
    VIVect3    fwd(z, z, z);
    bool       done = false;
    float      dist;
    float      dist1;
    float      total;
    float      scale;

    VISpellActiveEvent *e = m_projectile;

    if (m_actor == -1)
        done = true;
    else
    {
        if (m_scene->GetActorSpritePtr(m_actor) == 0)
            return -1;

        GetNodeTransform(m_actor, (VICSpriteNodeID)4, m);
        p0 = VIVect3(m.m[3].x, m.m[3].y, m.m[3].z);
        tgt = p0;

        a = e->m_v14;
        a -= tgt;
        total = Diff(tgt, e->m_v20).Magnitude();
        dist = a.Magnitude();

        d = tgt;
        d -= e->m_v14;
        fwd = d;
        right = Cross(up, fwd);
        up = Cross(fwd, right);
        right.Normalize();
        up.Normalize();
        fwd.Normalize();

        m.m[0].x = right.x;
        m.m[0].y = right.y;
        m.m[0].z = right.z;
        m.m[0].w = 0.0f;
        m.m[1].x = up.x;
        m.m[1].y = up.y;
        m.m[1].z = up.z;
        m.m[1].w = 0.0f;
        m.m[2].x = fwd.x;
        m.m[2].y = fwd.y;
        m.m[2].z = fwd.z;
        m.m[2].w = 0.0f;
        m.m[3].x = e->m_v14.x;
        m.m[3].y = e->m_v14.y;
        m.m[3].z = e->m_v14.z;
        m.m[3].w = 1.0f;

        z = dist * 0.6f;
        scale = VIMIN(z, 1.0f);

        switch ((e->m_event->m_p0C >> 8) & 0xF)
        {
        case 0:
            if (e->m_emitter1 || e->m_actor1 >= 0)
            {
                q0 = e->m_a[0];
                q1 = e->m_a[1];
                q0 *= scale;
                q1 *= scale;
                m.Mul(q0, p0);
                m.Mul(q1, p1);
            }
            else
                p0 = e->m_v14;
            break;

        case 1:
            m2.RotateHPR(VIVect3(0.0f, 0.0f, e->m_f9C));
            e->m_f9C += e->m_event->m_f1C * 5.0f * m_scene->m_particleSystem.DeltaTime();

            if (e->m_emitter1 || e->m_actor1 >= 0)
            {
                p0 = e->m_a[0];
                p1 = e->m_a[1];
                p0 *= scale;
                p1 *= scale;
                m2.Mul(p0, q0);
                m2.Mul(p1, q1);
                m.Mul(q0, p0);
                m.Mul(q1, p1);
            }
            else
            {
                e->m_fA0 += m_scene->m_particleSystem.DeltaTime() * 3.0f;
                e->m_fA0 = VIMIN(e->m_fA0, e->m_event->m_f18);
                p0.Zero();
                p0.x = e->m_fA0 * scale;
                m2.Mul(p0, p1);
                m.Mul(p1, p0);
            }
            break;

        case 2:
            if (e->m_emitter1 || e->m_actor1 >= 0)
            {
                if (1.0f <= e->m_f9C)
                {
                    e->m_b[0] = e->m_c[0];
                    e->m_b[1] = e->m_c[1];
                    e->m_c[0].x = e->m_a[0].x
                        + ((float)rand() - (float)rand()) * 4.656613e-10f * 0.2f;
                    e->m_c[0].y = e->m_a[0].y
                        + ((float)rand() - (float)rand()) * 4.656613e-10f * 0.2f;
                    e->m_c[0].z = e->m_a[0].z
                        + ((float)rand() - (float)rand()) * 4.656613e-10f * 0.05f;
                    e->m_c[1].x = e->m_a[1].x
                        + ((float)rand() - (float)rand()) * 4.656613e-10f * 0.2f;
                    e->m_c[1].y = e->m_a[1].y
                        + ((float)rand() - (float)rand()) * 4.656613e-10f * 0.2f;
                    e->m_c[1].z = e->m_a[1].z
                        + ((float)rand() - (float)rand()) * 4.656613e-10f * 0.05f;
                    e->m_c[0] *= e->m_event->m_f18;
                    e->m_c[1] *= e->m_event->m_f18;
                    e->m_f9C = 0.0f;
                }

                q0 = e->m_c[0];
                q0 -= e->m_b[0];
                q0 *= e->m_f9C;
                q0 += e->m_b[0];
                q0 *= scale;
                q1 = e->m_c[1];
                q1 -= e->m_b[1];
                q1 *= e->m_f9C;
                q1 += e->m_b[1];
                q1 *= scale;
                e->m_f9C += m_scene->m_particleSystem.DeltaTime() * 15.0f;
                m.Mul(q0, p0);
                m.Mul(q1, p1);
            }
            else
            {
                if (1.0f <= e->m_f9C)
                {
                    e->m_b[0] = e->m_c[0];
                    e->m_c[0].x = ((float)rand() - (float)rand()) * 4.656613e-10f * 0.2f;
                    e->m_c[0].y = ((float)rand() - (float)rand()) * 4.656613e-10f * 0.2f;
                    e->m_c[0].z = ((float)rand() - (float)rand()) * 4.656613e-10f * 0.05f;
                    e->m_c[0] *= e->m_event->m_f18;
                    e->m_f9C = 0.0f;
                }

                p1 = e->m_c[0];
                p1 -= e->m_b[0];
                p1 *= e->m_f9C;
                p1 += e->m_b[0];
                p1 *= scale;
                e->m_f9C += m_scene->m_particleSystem.DeltaTime() * 15.0f;
                m.Mul(p1, p0);
            }
            break;

        case 3:
            if (e->m_emitter1 || e->m_actor1 >= 0)
            {
                q0 = e->m_a[0];
                q1 = e->m_a[1];
                q0 *= scale;
                q1 *= scale;
                m.Mul(q0, p0);
                m.Mul(q1, p1);
            }
            else
                p0 = e->m_v14;

            {
                float h = sinf(Diff(e->m_v14, e->m_v20).Magnitude() / total * 3.1415927f)
                          * (1.0f / e->m_event->m_f14) * (1.0f / e->m_event->m_f14)
                          * total * total;

                p0.y += h;
                p1.y += h;
            }
            break;
        }

        if (e->m_emitter0)
        {
            e->m_emitter0->SetLocation(p0);
            e->m_emitter0->SetDirection(d);
            e->m_emitter0->Update();
        }
        if (e->m_emitter1)
        {
            e->m_emitter1->SetLocation(p1);
            e->m_emitter1->SetDirection(d);
            e->m_emitter1->Update();
        }

        {
            VIMatrix44 m3;
            VIVect3    hpr;

            if (e->m_actor0 != -1 || e->m_actor1 != -1)
            {
                VIVect3 r2;
                VIVect3 u2(0.0f, 1.0f, 0.0f);
                VIVect3 f2;

                if (((e->m_event->m_p0C >> 8) & 0xF) == 3)
                {
                    f2 = d;

                    VIVect3 next;

                    f2.Normalize();
                    f2 *= e->m_event->m_f14;
                    f2 *= m_scene->m_particleSystem.DeltaTime();
                    f2 *= 2.0f;
                    next = Sum(e->m_v14, f2);

                    float h = sinf(Diff(next, e->m_v20).Magnitude() / total * 3.1415927f)
                              * (1.0f / e->m_event->m_f14) * (1.0f / e->m_event->m_f14)
                              * total * total;

                    next.y += h;
                    f2 = Diff(next, p0);
                }
                else
                    f2 = d;

                r2 = Cross(u2, f2);
                u2 = Cross(f2, r2);
                r2.Normalize();
                u2.Normalize();
                f2.Normalize();

                m3.Identity();
                m3.m[0].x = r2.x;
                m3.m[0].y = r2.y;
                m3.m[0].z = r2.z;
                m3.m[1].x = u2.x;
                m3.m[1].y = u2.y;
                m3.m[1].z = u2.z;
                m3.m[2].x = f2.x;
                m3.m[2].y = f2.y;
                m3.m[2].z = f2.z;
                m3.ConvertToEulerZXY(hpr);
            }

            if (e->m_actor0 >= 0)
                m_scene->SetActorTransform(e->m_actor0, hpr, 1.0f, p0);
            if (e->m_actor1 >= 0)
                m_scene->SetActorTransform(e->m_actor1, hpr, 1.0f, p1);
        }

        d.Normalize();
        d *= e->m_event->m_f14;
        d *= m_scene->m_particleSystem.DeltaTime();
        e->m_v14 += d;

        a = e->m_v14;
        a -= tgt;
        dist1 = a.Magnitude();
    }

    if (done == true || (dist <= dist1 && dist1 < 3.0f))
    {
        m_inactive.AddTail(e);
        m_projectile = 0;
    }
    return 0;
}

// NewSetAction's sibling: the same reset / find / reset-trigger tail, with the
// action chosen out of a four-way switch and a weapon trail hung on it.
int VISpellEventDriver::NewPlayAttack(VISpellEvent *ev)
{
    if (m_actor == -1)
        return 0;

    VICSprite *sp = (VICSprite *)m_scene->GetActorSpritePtr(m_actor);

    if (sp == 0)
        return 0;

    if (sp->m_type != 5)
    {
        m_action = -1;
        m_anim[0] = -1;
        m_anim[1] = -1;
        m_anim[2] = -1;
        m_anim[3] = -1;
        return -1;
    }

    switch (ev->m_p04)
    {
    case 0:
        m_action = sp->SetAttackAction(VICSpriteAttachSlot0, *m_scene);
        break;
    case 1:
        m_action = sp->SetAttackAction(VICSpriteAttachSlot1, *m_scene);
        break;
    case 2:
        m_action = sp->SetAttackAltAction(VICSpriteAttachSlot0, *m_scene);
        break;
    case 3:
        m_action = sp->SetAttackAltAction(VICSpriteAttachSlot1, *m_scene);
        break;
    }

    VIColor32F c(ev->m_f14, ev->m_f18, ev->m_f1C, 1.0f);

    sp->SetWeaponTrail(ev->m_id, c);

    m_anim[0] = -1;
    m_anim[1] = -1;
    m_anim[2] = -1;
    m_anim[3] = -1;

    sp->FindTrigger(VIHashResourceID("Spell FX 1"), m_anim[0]);
    sp->FindTrigger(VIHashResourceID("Spell FX 2"), m_anim[1]);
    sp->FindTrigger(VIHashResourceID("Spell FX 3"), m_anim[2]);
    sp->FindTrigger(VIHashResourceID("Spell FX 4"), m_anim[3]);

    if (m_anim[0] != -1)
        sp->ResetTrigger(m_anim[0]);
    if (m_anim[1] != -1)
        sp->ResetTrigger(m_anim[1]);
    if (m_anim[2] != -1)
        sp->ResetTrigger(m_anim[2]);
    if (m_anim[3] != -1)
        sp->ResetTrigger(m_anim[3]);
    return 0;
}

int VISpellEventDriver::NewCreateSound(VISpellEvent *ev)
{
    if (m_actor == -1)
        return 0;

    VISpellActiveEvent *e = new VISpellActiveEvent(ev, m_scene);
    e->m_time = m_scene->GetTime();

    VIVect3 at;
    VIVect3 hpr;

    e->m_actor0 = m_scene->CreateActor(at, hpr, ev->m_f14, ev->m_id);

    VISoundSprite *sp = (VISoundSprite *)m_scene->GetActorSpritePtr(e->m_actor0);

    sp->EnablePan((ev->m_p0C & 0x40000) != 0);

    if (ev->m_p0C & 0x80000)
        sp->SetAttenuationMode(VISoundSprite::ATTENUATION_1);
    else if (ev->m_p0C & 0x100000)
        sp->SetAttenuationMode(VISoundSprite::ATTENUATION_2);
    else
        sp->SetAttenuationMode(VISoundSprite::ATTENUATION_0);

    m_active.AddTail(e);
    ProcessCreateSound(e, true);
    return 0;
}

// Through an inline wrapper the Euler call's &hpr is rematerialised, as in the
// target; called directly, cse shares it with SetActorHPR's callee-saved copy.
static inline void ToEulerZXY(VIMatrix44 &m, VIVect3 &hpr)
{
    m.ConvertToEulerZXY(hpr);
}

// The actor choice is an if/ELSE: with the default assigned up front, cse
// follows the path straight through both flag tests and shares the -1 and the
// flags word with the guard above; with the default in the else arm it does
// not, the -1 is rematerialised and gcse copies the flags word (`move a0,v0`).
// The node is `& 0xFFFF`, which is ONE expression for the test and the
// argument, where a cast is two loads.
int VISpellEventDriver::ProcessCreateSprite(VISpellActiveEvent *e, bool active)
{
    VISpellEvent *ev = e->m_event;

    if (m_actor != -1 && active == true)
    {
        int actor;

        if (ev->m_p0C & 0x10000)
            actor = m_driver->m_caster;
        else
            actor = m_actor;
        if (ev->m_p0C & 0x20000)
            actor = m_driver->m_target;

        if (actor != -1)
        {
            VISprite *sp = m_scene->GetActorSpritePtr(actor);

            if (sp)
            {
                VIMatrix44 m;
                VIVect3 hpr;

                if (sp->m_type != 5 || (ev->m_p0C & 0xFFFF) == 0xFFFF)
                    m_scene->GetActorTransform(actor, m);
                else
                    ((VICSprite *)sp)->CalcSlotWorldTransform(
                        (VICSpriteNodeID)(ev->m_p0C & 0xFFFF), actor,
                        *m_scene, m);

                ToEulerZXY(m, hpr);
                m_scene->SetActorLocation(e->m_actor0,
                                          VIVect3(m.m[3].x, m.m[3].y, m.m[3].z));
                m_scene->SetActorHPR(e->m_actor0, hpr);
            }
        }
    }

    if (active == true && ev->m_p10 != 0
        && ev->m_p10 < (int)(m_scene->GetTime() - e->m_time))
    {
        e->Remove();
        delete e;
    }
    return 0;
}

// ProcessCreateSprite's twin, and it does something odd: it builds the
// rotation into a VIMatrix33 that nothing then reads -- only the translation
// reaches the scene.  gcc cannot delete the matrix, its constructor and
// destructor being out of line.
int VISpellEventDriver::ProcessCreateSound(VISpellActiveEvent *e, bool active)
{
    VISpellEvent *ev = e->m_event;

    if (m_actor != -1 && active == true)
    {
        int actor;

        if (ev->m_p0C & 0x10000)
            actor = m_driver->m_caster;
        else
            actor = m_actor;
        if (ev->m_p0C & 0x20000)
            actor = m_driver->m_target;

        if (actor != -1)
        {
            VISprite *sp = m_scene->GetActorSpritePtr(actor);

            if (sp)
            {
                VIMatrix44 m;
                VIMatrix33 dir;

                if (sp->m_type != 5 || (ev->m_p0C & 0xFFFF) == 0xFFFF)
                    m_scene->GetActorTransform(actor, m);
                else
                    ((VICSprite *)sp)->CalcSlotWorldTransform(
                        (VICSpriteNodeID)(ev->m_p0C & 0xFFFF), actor,
                        *m_scene, m);

                dir.m[0].x = m.m[0].x;
                dir.m[0].y = m.m[0].y;
                dir.m[0].z = m.m[0].z;
                dir.m[1].x = m.m[1].x;
                dir.m[1].y = m.m[1].y;
                dir.m[1].z = m.m[1].z;
                dir.m[2].x = m.m[2].x;
                dir.m[2].y = m.m[2].y;
                dir.m[2].z = m.m[2].z;

                m_scene->SetActorLocation(e->m_actor0,
                                          VIVect3(m.m[3].x, m.m[3].y, m.m[3].z));
            }
        }
    }

    if (active == true && ev->m_p10 != 0
        && ev->m_p10 < (int)(m_scene->GetTime() - e->m_time))
    {
        e->Remove();
        delete e;
    }
    return 0;
}

int VISpellEventDriver::NewCreateSprite(VISpellEvent *ev)
{
    if (m_actor == -1)
        return 0;

    VISpellActiveEvent *e = new VISpellActiveEvent(ev, m_scene);
    e->m_time = m_scene->GetTime();

    // The id is read TWICE -- once as the lookup's argument and once as the
    // else arm's assignment, which is the branch-likely's delay slot.  Written
    // as an initialiser it is read once and lives in a register.
    int sprite;

    if (m_scene->Sprite(ev->m_id)->m_type == 4)
    {
        sprite = m_scene->CopySprite(ev->m_id, 0);
        if (sprite < 0)
            return -1;
    }
    else
        sprite = ev->m_id;

    VIHSprite *sp = (VIHSprite *)m_scene->Sprite(sprite);

    if (sp->m_type == 4)
    {
        sp->EraseAllPlay(*m_scene);

        if (ev->m_p0C & 0x40000)
            sp->AddPlay(sp->m_defaultAnim, VIPlaybackTypeOnce, *m_scene);
        else if (ev->m_p0C & 0x80000)
            sp->AddPlay(sp->m_defaultAnim, VIPlaybackTypeLoop, *m_scene);
        else
            sp->AddPlay(sp->m_defaultAnim, *m_scene);
    }

    VIVect3 at;
    VIVect3 hpr;

    e->m_actor0 = m_scene->CreateActor(at, hpr, 1.0f, sprite);
    m_scene->SetActorCollideable(e->m_actor0, 0);
    m_scene->GetActorSpritePtr(e->m_actor0);

    m_active.AddTail(e);
    ProcessCreateSprite(e, true);
    return 0;
}

// Three zeroed VIVect3 that nothing reads -- the constructor stores survive
// because gcc will not delete a class local's initialisation (Phase 6).
int VISpellEventDriver::NewPlayTargetLightning(VISpellEvent *ev)
{
    VIVect3 a;
    VIVect3 b;
    VIVect3 c;

    for (int i = 0; i < m_driver->m_numSecondary + 1; i++)
    {
        VISpellEventDriver *d = 0;

        if (ev->m_p04 & 1)
            if (i == 0)
                d = m_driver->AddSpellEventDriver(
                        &m_driver->m_effect->m_targetEvents, m_driver->m_target);

        if (ev->m_p04 & 2)
            if (i != 0)
                d = m_driver->AddSpellEventDriver(
                        &m_driver->m_effect->m_areaEvents,
                        m_driver->m_secondary[i - 1]);

        if (m_actor != -1 && d != 0 && d->m_lightning == 0)
        {
            d->m_lightning = new VISpellActiveEvent(ev, m_scene);
            d->m_lightning->m_actor0 = m_actor;
            d->m_lightning->m_time = m_scene->GetTime();
        }
    }
    return 0;
}

int VISpellEventDriver::ProcessFadeSprite(VISpellActiveEvent *e)
{
    VISpellEvent *ev = e->m_event;
    VIRaster *r = m_scene->m_raster;
    float f;

    if (ev->m_id < (int)(m_scene->GetTime() - e->m_time))
        f = ev->m_f14;
    else
    {
        // ONE variable, not a separate `t`: the division's destination is the
        // register the clamped result ends up in, which a distinct local
        // cannot be.
        f = (float)(m_scene->GetTime() - e->m_time) / (float)ev->m_id;
        f = e->m_f9C + (ev->m_f14 - e->m_f9C) * f;
        f = VIMIN(VIMAX(0.0f, f), 1.0f);
    }

    VISpellActiveEvent *ce = FindCreateSprite(ev->m_p04);

    if (ce)
    {
        VISprite *sp = m_scene->Sprite(m_scene->GetActorSprite(ce->m_actor0));
        int pal;

        if (sp->m_type == 1)
            pal = ((VISimpleSprite *)sp)->GetMaterialPal();
        else
            pal = ((VISimpleSprite *)m_scene->Sprite(
                       ((VIHSprite *)sp)->GetAttachSprite(
                           ((VIHSprite *)sp)->GetFirstAttach())))->GetMaterialPal();

        VIMaterial *mat = r->GetMaterialPalEntry(pal, 0);

        if (mat)
        {
            VIColor32 c;

            c = mat->GetLayerColor(0);
            c.a = (unsigned char)(f * 255.0f);
            mat->SetLayerColor(0, c);
        }
    }

    if (ev->m_id < (int)(m_scene->GetTime() - e->m_time))
    {
        e->Remove();
        delete e;
    }
    return 0;
}

// The sprite is CLONED so its opacity can be driven independently: a simple
// sprite has its own copy made and the actor re-pointed at it, a hierarchical
// one has its first attachment replaced.  Either way the material palette is
// copied too, and the fade starts from whatever alpha it already had.
int VISpellEventDriver::NewFadeSprite(VISpellEvent *ev)
{
    VIRaster *r = m_scene->m_raster;
    VISpellActiveEvent *ce = FindCreateSprite(ev->m_p04);

    if (ce == 0)
        return -1;

    VISpellActiveEvent *e = new VISpellActiveEvent(ev, m_scene);
    e->m_time = m_scene->GetTime();

    int i = m_scene->GetActorSprite(ce->m_actor0);
    VISprite *sp = m_scene->Sprite(i);
    int pal;

    if (sp->m_type == 1)
    {
        m_scene->SetActorSprite(ce->m_actor0, m_scene->CopySprite(i, 0));

        sp = m_scene->GetActorSpritePtr(ce->m_actor0);
        pal = r->CopyMaterialPal(((VISimpleSprite *)sp)->GetMaterialPal(), 0);
        ((VISimpleSprite *)sp)->SetMaterialPal(pal, *m_scene->m_dict, *r);
    }
    else
    {
        int at = ((VIHSprite *)sp)->GetFirstAttach();

        if (at < 0)
            return -1;

        int as = ((VIHSprite *)sp)->GetAttachSprite(at);
        int node = ((VIHSprite *)sp)->GetAttachNode(at);

        if (as < 0)
            return -1;
        if (node < 0)
            return -1;

        as = m_scene->CopySprite(as, 0);
        ((VIHSprite *)sp)->Detach(at, *m_scene);
        ((VIHSprite *)sp)->Attach(as, node, *m_scene);

        sp = m_scene->Sprite(as);
        pal = r->CopyMaterialPal(((VISimpleSprite *)sp)->GetMaterialPal(), 0);
        ((VISimpleSprite *)sp)->SetMaterialPal(pal, *m_scene->m_dict, *r);
    }

    VIMaterial *mat = m_scene->m_raster->GetMaterialPalEntry(pal, 0);

    // The success path in the `if` (Phase 7): written as a guard returning -1
    // the failure block lands after the body, where the three `bltz` returns
    // above have already put it in front of one.
    if (mat)
    {
        VIColor32 c;

        c = mat->GetLayerColor(0);
        e->m_f9C = (float)c.a / 255.0f;
        m_active.AddTail(e);
        return 0;
    }
    return -1;
}
