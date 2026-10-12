#include "vi.h"

// SUPPORT 0x0049D918 .. 0x0049DA40.  The abstract base of the sound resources.
// Its constructor is inline (both derived constructors zero the base fields
// with no call), so the only out-of-line members are the virtual destructor,
// the type accessor and the two down-casts.  This TU emits its own copies of
// __tf7VISound and __tf10VIResource -- every TU that needs the RTTI for a
// class emits one, which is why the image holds three of each.

VISound::~VISound()
{
}

VISound::TYPE VISound::Type() const
{
    return m_type;
}

// Type() is a real call here, not an inline read: it is declared in the class
// body but DEFINED out of line, so gcc 2.95 cannot inline it inside this TU.
VIWave *VISound::AsWave()
{
    return Type() == WAVE ? (VIWave *)this : 0;
}

VIXm *VISound::AsXm()
{
    return Type() == XM ? (VIXm *)this : 0;
}
