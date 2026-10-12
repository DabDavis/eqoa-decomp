// VISpellEffectSystem -- SUPPORT 0x0047B840 .. 0x0047CBC0, its own TU.
//
// The scene's spell-effect service: a VIPool of the loaded VISpellEffects and
// twenty-four playback slots, each of which owns one VISpellEffectDriver while
// a cast is in flight.
#include "vi.h"

VISpellEffectSystem::VISpellEffectSystem()
{
    m_inited = 0;
}

VISpellEffectSystem::~VISpellEffectSystem()
{
}

int VISpellEffectSystem::Init(VIScene *sc)
{
    m_scene = sc;
    m_active.Init();
    m_free.Init();

    int i;
    for (i = 0; i < 24; i++)
        m_slots[i] = 0;
    for (i = 0; i < 24; i++)
        m_free.AddTail(&m_playbacks[i]);

    // Phase 16's hoist reads this order out: the store emitted FIRST after the
    // two loops is the source's LAST constant store, and the rest keep source
    // order -- so m_inited closes the block, not opens it.
    m_serial = 0x16062;
    m_playerActor = -1;
    m_vibrate = 0;
    m_vibrateUser = 0;
    m_cameraEffect = 0;
    m_inited = 1;
    return 0;
}

int VISpellEffectSystem::Clear()
{
    if (m_inited)
    {
        VISpellEffectPbi *pb = FirstPlayback();
        while (pb)
        {
            VISpellEffectPbi *next = NextPlayback(pb);
            Stop(pb->m_handle);
            pb = next;
        }
        ReleaseAllSpellEffects(*m_scene->m_dict);
        m_inited = 0;
    }
    m_cameraEffect = 0;
    return 0;
}

VIVibrateFn VISpellEffectSystem::SetVibrateCallback(VIVibrateFn fn, void *user)
{
    VIVibrateFn old = m_vibrate;

    m_vibrate = fn;
    m_vibrateUser = user;
    return old;
}

void VISpellEffectSystem::SetCameraEffect(VICameraEffect *e)
{
    m_cameraEffect = e;
}

void VISpellEffectSystem::SetPlayerActor(int actor)
{
    m_playerActor = actor;
}

int VISpellEffectSystem::CreateSpellEffect(int share)
{
    VISpellEffect *e = new VISpellEffect;

    if (e == 0)
        return -1;
    if (e->Create(m_scene) < 0)
    {
        if (e)
            delete e;
        return -1;
    }
    if (share)
        e->m_link++;
    return m_effects.Add(e);
}

VISpellEffect *VISpellEffectSystem::SpellEffect(int i)
{
    return m_effects.Value(i);
}

int VISpellEffectSystem::ShareSpellEffect(int i)
{
    m_effects.Value(i)->m_link++;
    return 0;
}

int VISpellEffectSystem::ReleaseSpellEffect(int i, VIDictionary &d)
{
    VISpellEffect *e = m_effects.Value(i);

    // Phase 13's nesting: the whole body inside the guard, with ONE `return 0`
    // at the bottom.  Written as an early `if (--link > 0) return 0;` the
    // function grows a second return path, and the Erase test at the end
    // if-converts to slti+movz where the target branches on a copy in v1.
    if (--e->m_link <= 0)
    {
        if (e->m_id)
            d.Remove(*e);
        if (e->Destroy() < 0)
            return -1;
        if (e)
            delete e;
        if (m_effects.Erase(i) < 0)
            return -1;
    }
    return 0;
}

int VISpellEffectSystem::ReleaseAllSpellEffects(VIDictionary &d)
{
    // Begin() is re-read every iteration and Next() is never called: Erase
    // takes the element out from under the walk, so this always releases the
    // FIRST one.  The `while` is what puts the entry branch on the test.
    int i;

    while ((i = m_effects.Begin()) != m_effects.End())
        ReleaseSpellEffect(i, d);
    m_effects.Clear();
    return 0;
}

// The handle is the slot in the low byte and a per-cast counter above it, so a
// stale handle cannot address the slot's next tenant.
int VISpellEffectSystem::Play(int effect, unsigned int a, int caster, int target,
                              int *secondary)
{
    // `handle` is declared and initialised BEFORE the guards: its -1 lives in a
    // callee-saved register across the SpellEffect call, and cse hands the same
    // register to all three guard comparisons.
    int handle = -1;

    if (m_scene->m_raster == 0)
        return -1;
    if (caster == -1 || target == -1)
        return -1;

    VISpellEffect *e = SpellEffect(effect);
    VISpellEffectPbi *pb;
    int i;

    for (i = 0; i < 24; i++)
    {
        if (m_slots[i] == 0)
        {
            handle = i | ((m_serial++ << 8) & 0x7fffff00);
            pb = NewPlayback();
            m_slots[i] = pb;
            break;
        }
    }
    if (handle == -1)
        return -1;

    pb->m_effect = effect;
    pb->m_handle = handle;
    pb->m_driver = new VISpellEffectDriver;
    pb->m_driver->Create(e, m_scene);
    pb->m_driver->SetCaster(caster);
    pb->m_driver->SetTarget(target);
    pb->m_driver->SetVibrateCallback(m_vibrate, m_vibrateUser);
    pb->m_driver->SetCameraEffect(m_cameraEffect);
    pb->m_driver->SetPlayerActor(m_playerActor);
    if (secondary)
    {
        for (i = 0; i < 8 && secondary[i] != -1; i++)
            pb->m_driver->AddSecondaryTarget(secondary[i]);
    }
    pb->m_driver->Trigger(a, 10000);
    pb->m_startTime = m_scene->GetTime();
    return handle;
}

int VISpellEffectSystem::IsPlaying(int handle)
{
    return Playback(handle) != 0;
}

VISpellEffectDriver *VISpellEffectSystem::SpellEffectDriver(int handle)
{
    VISpellEffectPbi *pb = Playback(handle);

    if (pb == 0)
        return 0;
    return pb->m_driver;
}

void VISpellEffectSystem::Stop(int handle)
{
    VISpellEffectPbi *pb = Playback(handle);

    if (pb)
    {
        if (pb->m_driver)
            delete pb->m_driver;
        m_slots[(pb->m_handle & 0xff) % 24] = 0;
        DeletePlayback(pb);
    }
}

void VISpellEffectSystem::NotifyActorWasDestroyed(int actor)
{
    VISpellEffectPbi *pb = FirstPlayback();

    while (pb)
    {
        if (pb->m_driver)
            pb->m_driver->NotifyActorWasDestroyed(actor);
        pb = NextPlayback(pb);
    }
}

void VISpellEffectSystem::StopIfThisWasCaster(int actor)
{
    VISpellEffectPbi *pb = FirstPlayback();

    while (pb)
    {
        if (pb->m_driver && pb->m_driver->m_caster == actor)
            Stop(pb->m_handle);
        pb = NextPlayback(pb);
    }
}

// A playback that reports done is stopped, and so is one that has been running
// for a minute -- a backstop, well past the 10000 Play hands Trigger.
int VISpellEffectSystem::Process()
{
    VISpellEffectPbi *pb = FirstPlayback();

    while (pb)
    {
        VISpellEffectPbi *next = NextPlayback(pb);

        // The elapsed time is an INT: the target truncates the 64-bit
        // difference with dsll32/dsra32 before comparing, which a comparison on
        // the long itself does not do.
        if (pb->m_driver->Process() == 0)
        {
            Stop(pb->m_handle);
        }
        else
        {
            int elapsed = m_scene->GetTime() - pb->m_startTime;
            if (elapsed > 60000)
                Stop(pb->m_handle);
        }
        pb = next;
    }
    return 0;
}

VISpellEffectSystem::VISpellEffectPbi *VISpellEffectSystem::NewPlayback()
{
    VISpellEffectPbi *pb = (VISpellEffectPbi *)m_free.RemHead();

    if (pb)
        m_active.AddTail(pb);
    return pb;
}

void VISpellEffectSystem::DeletePlayback(VISpellEffectPbi *pb)
{
    pb->Remove();
    m_free.AddTail(pb);
}

VISpellEffectSystem::VISpellEffectPbi *VISpellEffectSystem::Playback(int handle)
{
    // `handle & 0xff` and not a cast to unsigned char: the cast lets gcc prove
    // the value non-negative and the remainder comes out `divu`.  And Phase 13's
    // nesting -- two flat guards if-convert the handle compare to `xor`+`movn`
    // where the target branches to a shared `return 0`.
    VISpellEffectPbi *pb = m_slots[(handle & 0xff) % 24];

    if (pb)
    {
        if (pb->m_handle == handle)
            return pb;
    }
    return 0;
}

VISpellEffectSystem::VISpellEffectPbi *VISpellEffectSystem::FirstPlayback()
{
    VISpellEffectPbi *pb = (VISpellEffectPbi *)m_active.m_head;

    if (pb->m_next == 0)
        return 0;
    return pb;
}

VISpellEffectSystem::VISpellEffectPbi *VISpellEffectSystem::NextPlayback(VISpellEffectPbi *pb)
{
    VISpellEffectPbi *n = (VISpellEffectPbi *)pb->m_next;

    if (n->m_next == 0)
        return 0;
    return n;
}
