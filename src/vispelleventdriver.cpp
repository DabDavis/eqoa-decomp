// VISpellEventDriver -- SUPPORT 0x00475E68 .. 0x0047A978, its own TU.
//
// One event LIST of a spell effect while it is running: a cursor into the
// array, the events it has started that are still going, and the animation
// handles it waits on.  VISpellEffectDriver owns one of these per list.
#include "vi.h"

extern "C" unsigned int VIHashResourceID(const char *);

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
    if (m_wait)
    {
        switch (m_wait->m_event->m_p04)
        {
        case 0:
            {
                if (m_actor != -1)
                {
                    VICSprite *sp =
                        (VICSprite *)m_scene->GetActorSpritePtr(m_actor);

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

                VICSprite *sp =
                    (VICSprite *)m_scene->GetActorSpritePtr(m_actor);

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

// The four big traversals, not attempted in phase 38: 609, 420, 399 and 1187
// instructions.  They are stubbed so the rest of the TU links and so eqcheck
// reports them as `wrong` rather than `no-target`.
int VISpellEventDriver::NewPlayTargetProjectile(VISpellEvent *ev) { return 0; }
int VISpellEventDriver::ProcessCreateEmitter(VISpellActiveEvent *e, bool a) { return 0; }
int VISpellEventDriver::ProcessLightningEvent() { return 0; }
int VISpellEventDriver::ProcessProjectileEvent() { return 0; }

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

int VISpellEventDriver::ProcessCreateSprite(VISpellActiveEvent *e, bool active)
{
    VISpellEvent *ev = e->m_event;

    if (m_actor != -1 && active == true)
    {
        int actor = m_actor;

        if (ev->m_p0C & 0x10000)
            actor = m_driver->m_caster;
        if (ev->m_p0C & 0x20000)
            actor = m_driver->m_target;

        if (actor != -1)
        {
            VISprite *sp = m_scene->GetActorSpritePtr(actor);

            if (sp)
            {
                VIMatrix44 m;
                VIVect3 hpr;

                if (sp->m_type != 5 || (unsigned short)ev->m_p0C == 0xFFFF)
                    m_scene->GetActorTransform(actor, m);
                else
                    ((VICSprite *)sp)->CalcSlotWorldTransform(
                        (VICSpriteNodeID)(unsigned short)ev->m_p0C, actor,
                        *m_scene, m);

                m.ConvertToEulerZXY(hpr);
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
        int actor = m_actor;

        if (ev->m_p0C & 0x10000)
            actor = m_driver->m_caster;
        if (ev->m_p0C & 0x20000)
            actor = m_driver->m_target;

        if (actor != -1)
        {
            VISprite *sp = m_scene->GetActorSpritePtr(actor);

            if (sp)
            {
                VIMatrix44 m;
                VIMatrix33 dir;

                if (sp->m_type != 5 || (unsigned short)ev->m_p0C == 0xFFFF)
                    m_scene->GetActorTransform(actor, m);
                else
                    ((VICSprite *)sp)->CalcSlotWorldTransform(
                        (VICSpriteNodeID)(unsigned short)ev->m_p0C, actor,
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
