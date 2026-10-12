// VISpellEffectDriver -- SUPPORT 0x0047A978 .. 0x0047B8A8, its own TU.
//
// One cast in flight.  It owns a VILList of VISpellEventDriver -- one per
// event list the effect carries -- and retires them as they report done.
#include "vi.h"

VISpellEffectDriver::VISpellEffectDriver()
{
    m_effect = 0;
    m_scene = 0;
    m_caster = -1;
    m_target = -1;
    m_numSecondary = 0;
    m_vibrate = 0;
    m_vibrateUser = 0;
    m_cameraEffect = 0;
    // The array loop is LAST in both this and Destroy: the three zero stores
    // above it are emitted ahead of it, and moved below they land after.
    for (int i = 0; i < 8; i++)
        m_secondary[i] = -1;
}

VISpellEffectDriver::~VISpellEffectDriver()
{
    Destroy();
}

int VISpellEffectDriver::Create(VISpellEffect *e, VIScene *sc)
{
    m_effect = e;
    m_scene = sc;
    m_playerActor = -1;
    m_cameraEffect = 0;
    m_cameraEffectOn = 0;
    return 0;
}

void VISpellEffectDriver::Destroy()
{
    if (m_scene == 0)
        return;

    if (m_drivers.Length() && m_cameraEffectOn)
        m_cameraEffect->Reset();
    m_cameraEffectOn = 0;

    VISpellEventDriver *d;
    while ((d = (VISpellEventDriver *)m_drivers.RemHead()) != 0)
        delete d;

    // Phase 16's constant hoist reads this order straight out: the store that
    // comes FIRST after the join is the source's LAST store of that constant,
    // so m_numSecondary closes the block.  All 720 orderings were swept; this
    // is the only one that is byte-exact, and it is not the constructor's.
    m_effect = 0;
    m_scene = 0;
    m_cameraEffect = 0;
    m_caster = -1;
    m_target = -1;
    m_numSecondary = 0;
    for (int i = 0; i < 8; i++)
        m_secondary[i] = -1;
}

int VISpellEffectDriver::SetCaster(int actor)
{
    m_caster = actor;
    return 0;
}

int VISpellEffectDriver::SetTarget(int actor)
{
    m_target = actor;
    return 0;
}

int VISpellEffectDriver::AddSecondaryTarget(int actor)
{
    // The count is read ONCE: the target keeps it in a register for both the
    // element address and the increment, where re-reading the member costs 7
    // of 14 words and moves the address onto the count's own register.
    int n = m_numSecondary;

    if (n == 8)
        return -1;
    m_secondary[n] = actor;
    m_numSecondary = n + 1;
    return 0;
}

void VISpellEffectDriver::SetVibrateCallback(VIVibrateFn fn, void *user)
{
    m_vibrate = fn;
    m_vibrateUser = user;
}

void VISpellEffectDriver::SetCameraEffect(VICameraEffect *e)
{
    m_cameraEffect = e;
}

void VISpellEffectDriver::SetPlayerActor(int actor)
{
    m_playerActor = actor;
}

void VISpellEffectDriver::NotifyActorWasDestroyed(int actor)
{
    if (actor == m_playerActor && m_cameraEffect)
        m_cameraEffect->Reset();

    if (m_caster == actor)
        m_caster = -1;
    if (m_target == actor)
        m_target = -1;

    for (int i = 0; i < m_numSecondary; i++)
        if (m_secondary[i] == actor)
            m_secondary[i] = -1;

    for (VISpellEventDriver *d = (VISpellEventDriver *)m_drivers.m_head;
         d->m_next; d = (VISpellEventDriver *)d->m_next)
        if (d->m_actor == actor)
            d->m_actor = -1;
}

int VISpellEffectDriver::Trigger(int a, int b)
{
    if (m_drivers.m_head->m_next)
        return -1;
    if (m_caster == -1)
        return -1;

    m_triggerTime = m_scene->GetTime();
    m_arg0 = a;
    m_arg1 = b;
    AddSpellEventDriver(&m_effect->m_casterEvents, m_caster);
    return 0;
}

VISpellEventDriver *VISpellEffectDriver::AddSpellEventDriver(VISpellEventList *l, int actor)
{
    VISpellEventDriver *d = new VISpellEventDriver(this, l, actor, m_scene);
    m_drivers.AddTail(d);
    return d;
}

int VISpellEffectDriver::Process()
{
    VISpellEventDriver *d = (VISpellEventDriver *)m_drivers.m_head;

    while (d->m_next)
    {
        VISpellEventDriver *n = (VISpellEventDriver *)d->m_next;
        if (!d->Process())
        {
            d->Remove();
            delete d;
        }
        d = n;
    }
    return m_drivers.m_head->m_next != 0;
}
