// VIParticleAttributes -- SUPPORT 0x004597C0 .. 0x0045A3D8.
//
// A pure property bag: 26 accessor/setter pairs, an implicit operator=, an
// empty destructor and a constructor that memsets the whole object and then
// calls its own setters for the defaults.  Nothing here is mathematical, so
// every miss is codegen -- which is what makes it a good regression surface
// for the toolchain settled in Phase 3.
//
// Two facts the class itself establishes:
//
//  1. EVERY SETTER STAMPS +0x2CC.  The stamp is read BEFORE the field is
//     written in every one of the fourteen float setters (`lw v0,716(a0)` is
//     the first instruction, the `swc1` the second) -- that is the scheduler,
//     not the source; the source writes the field and then bumps the counter.
//  2. THE FLAG WRITTEN BY THE VECTOR SETTERS IS ONE `||`.  SetInnerOffset and
//     its four siblings test x, y and z against zero and store the disjunction
//     into a companion int, which comes out as three compares feeding one
//     store through two branch-likelys -- Phase 9's "a multi-term rejection is
//     one ||", at the width where it is most visible.
//
// SetNozzleAxis is the odd one out: no flag, and it normalises the vector it
// was handed after storing it.
#include "vi.h"

extern "C" void *memset(void *, int, int);

VIParticleAttributes::VIParticleAttributes()
{
    memset(this, 0, sizeof(VIParticleAttributes));

    SetFriction(0.0f);
    SetBirthrate(100.0f);
    SetBirthrateVar(0.0f);
    SetLifespan(0.5f);
    SetLifespanVar(0.0f);
    SetVelocity(2.0f);
    SetVelocityVar(0.0f);
    SetStartSize(0.2f);
    SetStartSizeVar(0.0f);
    SetEndSize(0.2f);
    SetEndSizeVar(0.0f);
    SetStartColorVar(VIColor32F());
    SetEndColorVar(VIColor32F());
    SetInheritVelocity(0.0f);
    SetDeltaSpawn(0.0f);
    SetNozzleAxis(VIVect3(0.0f, 1.0f, 0.0f));
    SetNozzleHprVar(VIVect3(1.0f, 1.0f, 1.0f));
    SetGradientRepeat(1.0f);
    SetGravityOn(0);

    for (int i = 0; i < 32; i++)
    {
        float t = 1.0f - (float)i / 31.0f;
        SetGradientColor(i, VIColor32F(1.0f, t, 0.0f, t));
    }
}

VIParticleAttributes::~VIParticleAttributes()
{
}

float VIParticleAttributes::Friction() const { return m_friction; }
void  VIParticleAttributes::SetFriction(float f) { m_friction = f; m_version++; }

float VIParticleAttributes::Birthrate() const { return m_birthrate; }
void  VIParticleAttributes::SetBirthrate(float f) { m_birthrate = f; m_version++; }

float VIParticleAttributes::BirthrateVar() const { return m_birthrateVar; }
void  VIParticleAttributes::SetBirthrateVar(float f) { m_birthrateVar = f; m_version++; }

float VIParticleAttributes::Lifespan() const { return m_lifespan; }
void  VIParticleAttributes::SetLifespan(float f) { m_lifespan = f; m_version++; }

float VIParticleAttributes::LifespanVar() const { return m_lifespanVar; }
void  VIParticleAttributes::SetLifespanVar(float f) { m_lifespanVar = f; m_version++; }

float VIParticleAttributes::Velocity() const { return m_velocity; }
void  VIParticleAttributes::SetVelocity(float f) { m_velocity = f; m_version++; }

float VIParticleAttributes::VelocityVar() const { return m_velocityVar; }
void  VIParticleAttributes::SetVelocityVar(float f) { m_velocityVar = f; m_version++; }

float VIParticleAttributes::StartSize() const { return m_startSize; }
void  VIParticleAttributes::SetStartSize(float f) { m_startSize = f; m_version++; }

float VIParticleAttributes::StartSizeVar() const { return m_startSizeVar; }
void  VIParticleAttributes::SetStartSizeVar(float f) { m_startSizeVar = f; m_version++; }

float VIParticleAttributes::EndSize() const { return m_endSize; }
void  VIParticleAttributes::SetEndSize(float f) { m_endSize = f; m_version++; }

float VIParticleAttributes::EndSizeVar() const { return m_endSizeVar; }
void  VIParticleAttributes::SetEndSizeVar(float f) { m_endSizeVar = f; m_version++; }

float VIParticleAttributes::InheritVelocity() const { return m_inheritVelocity; }
void  VIParticleAttributes::SetInheritVelocity(float f) { m_inheritVelocity = f; m_version++; }

float VIParticleAttributes::DeltaSpawn() const { return m_deltaSpawn; }
void  VIParticleAttributes::SetDeltaSpawn(float f) { m_deltaSpawn = f; m_version++; }

const VIColor32F &VIParticleAttributes::StartColorVar() const { return m_startColorVar; }
void VIParticleAttributes::SetStartColorVar(const VIColor32F &c)
{
    m_startColorVar = c;
    m_version++;
}

const VIColor32F &VIParticleAttributes::EndColorVar() const { return m_endColorVar; }
void VIParticleAttributes::SetEndColorVar(const VIColor32F &c)
{
    m_endColorVar = c;
    m_version++;
}

const VIColor32F &VIParticleAttributes::GradientColor(int i) const { return m_gradient[i]; }
void VIParticleAttributes::SetGradientColor(int i, const VIColor32F &c)
{
    m_gradient[i] = c;
    m_version++;
}

float VIParticleAttributes::GradientRepeat() const { return m_gradientRepeat; }
void  VIParticleAttributes::SetGradientRepeat(float f) { m_gradientRepeat = f; m_version++; }

const VIVect3 &VIParticleAttributes::InnerOffset() const { return m_innerOffset; }
void VIParticleAttributes::SetInnerOffset(const VIVect3 &v)
{
    if (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f)
        m_innerOffsetSet = 1;
    else
        m_innerOffsetSet = 0;
    m_innerOffset = v;
    m_version++;
}

const VIVect3 &VIParticleAttributes::InnerHprVar() const { return m_innerHprVar; }
void VIParticleAttributes::SetInnerHprVar(const VIVect3 &v)
{
    if (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f)
        m_innerHprVarSet = 1;
    else
        m_innerHprVarSet = 0;
    m_innerHprVar = v;
    m_version++;
}

const VIVect3 &VIParticleAttributes::OuterOffset() const { return m_outerOffset; }
void VIParticleAttributes::SetOuterOffset(const VIVect3 &v)
{
    if (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f)
        m_outerOffsetSet = 1;
    else
        m_outerOffsetSet = 0;
    m_outerOffset = v;
    m_version++;
}

const VIVect3 &VIParticleAttributes::OuterHprVar() const { return m_outerHprVar; }
void VIParticleAttributes::SetOuterHprVar(const VIVect3 &v)
{
    if (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f)
        m_outerHprVarSet = 1;
    else
        m_outerHprVarSet = 0;
    m_outerHprVar = v;
    m_version++;
}

const VIVect3 &VIParticleAttributes::NozzleAxis() const { return m_nozzleAxis; }
void VIParticleAttributes::SetNozzleAxis(const VIVect3 &v)
{
    m_nozzleAxis = v;
    m_nozzleAxis.Normalize();
    m_version++;
}

const VIVect3 &VIParticleAttributes::NozzleHprVar() const { return m_nozzleHprVar; }
void VIParticleAttributes::SetNozzleHprVar(const VIVect3 &v)
{
    if (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f)
        m_nozzleHprVarSet = 1;
    else
        m_nozzleHprVarSet = 0;
    m_nozzleHprVar = v;
    m_version++;
}

int VIParticleAttributes::GravityOn() const
{
    if (m_gravity < 0.0f)
        return 1;
    return 0;
}

void VIParticleAttributes::SetGravityOn(int on)
{
    if (on)
        m_gravity = -5.0f;
    else
        m_gravity = 0.0f;
    m_version++;
}

int VIParticleAttributes::IsPatternSet() const { return m_pattern != 0; }
int VIParticleAttributes::PatternSize() const { return m_patternSize; }
VIVect3 &VIParticleAttributes::Pattern(int i) const { return m_pattern[i]; }

int VIParticleAttributes::SetPattern(VIVect3 *pts, int count)
{
    if (!pts)
        return -1;
    if (count <= 0)
        return -1;

    ClearPattern();

    m_pattern = new VIVect3[count];
    if (!m_pattern)
        return -1;

    m_patternSize = count;
    for (int i = 0; i < count; i++)
        m_pattern[i] = pts[i];

    m_version++;
    return 0;
}

void VIParticleAttributes::ClearPattern()
{
    delete [] m_pattern;
    m_pattern = 0;
    m_patternSize = 0;
    m_version++;
}
