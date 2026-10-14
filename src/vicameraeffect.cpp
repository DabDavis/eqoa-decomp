// VICameraEffect -- SUPPORT 0x0041DBA8 .. 0x0041E370.
#include "vi.h"

VICameraEffect::VICameraEffect()
{
    Reset();
    m_manualTerp = 0.0f;
    m_manualTerpRate = 0.0f;
}

VICameraEffect::~VICameraEffect()
{
}

void VICameraEffect::Reset()
{
    m_speed = SPEED_NONE;
    m_sample = 0;
    m_amplitude = 0.0f;
    m_amplitudeEnd = 0.0f;
    m_amplitudeRate = 0.0f;
}

void VICameraEffect::Shake(SPEED speed, float amp)
{
    m_speed = speed;
    m_amplitude = amp;
    m_amplitudeEnd = amp;
    m_amplitudeRate = 0.0f;
}

void VICameraEffect::ShakeTerp(float amp, float time)
{
    if (time == 0.0f)
    {
        Shake(m_speed, amp);
        return;
    }

    float from = m_amplitudeEnd;

    m_amplitudeEnd = amp;
    m_amplitude = from;
    m_amplitudeRate = (amp - from) / time;
}

void VICameraEffect::BeginManual(const VIVect3 &xyz, const VIVect3 &hpr, float time)
{
    m_manualXyz = xyz;
    m_manualHpr = hpr;

    if (time == 0.0f)
    {
        m_manualTerp = 1.0f;
        m_manualTerpRate = 0.0f;
    }
    else
    {
        m_manualTerp = 0.0f;
        m_manualTerpRate = 1.0f / time;
    }
}

const VIVect3 &VICameraEffect::ManualXyz() const
{
    return m_manualXyz;
}

void VICameraEffect::SetManualXyz(const VIVect3 &v)
{
    m_manualXyz = v;
}

const VIVect3 &VICameraEffect::ManualHpr() const
{
    return m_manualHpr;
}

void VICameraEffect::SetManualHpr(const VIVect3 &v)
{
    m_manualHpr = v;
}

float VICameraEffect::ManualTerp() const
{
    return m_manualTerp;
}

void VICameraEffect::EndManual(float time)
{
    if (time == 0.0f)
    {
        m_manualTerp = 0.0f;
        m_manualTerpRate = 0.0f;
    }
    else
    {
        m_manualTerpRate = -1.0f / time;
    }
}

void VICameraEffect::Update(float dt)
{
    if (m_manualTerpRate != 0.0f)
    {
        m_manualTerp += m_manualTerpRate * dt;

        if (m_manualTerpRate > 0.0f)
        {
            if (m_manualTerp >= 1.0f)
            {
                m_manualTerp = 1.0f;
                m_manualTerpRate = 0.0f;
            }
        }
        else
        {
            if (m_manualTerp <= 0.0f)
            {
                m_manualTerp = 0.0f;
                m_manualTerpRate = 0.0f;
            }
        }
    }

    if (m_speed)
    {
        if (m_amplitudeRate != 0.0f)
        {
            m_amplitude += m_amplitudeRate * dt;

            if (m_amplitudeRate > 0.0f)
            {
                if (m_amplitude >= m_amplitudeEnd)
                {
                    m_amplitude = m_amplitudeEnd;
                    m_amplitudeRate = 0.0f;
                }
            }
            else
            {
                if (m_amplitude <= m_amplitudeEnd)
                {
                    m_amplitude = m_amplitudeEnd;
                    m_amplitudeRate = 0.0f;
                }
            }
        }
    }
}

// The two file-scope tables the shake reads.  Neither has a symbol in the map,
// so both are static; the gap between the two %hi sites is 16 bytes, which
// sizes the count table at four ints -- i.e. four SPEED values -- and the
// sample table's row stride is the `li t0,24` the index arithmetic multiplies
// by, six floats a row.
static const int   ShakeCount[4] = { 0, 6, 4, 2 };
static const float ShakeSample[4][6] =
{
    { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.7f, -0.7f, -1.0f, -0.7f, 0.7f },
    { 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
};

// Both crosses return by VALUE: the frame is exactly 32 bytes longer than a
// component-wise version -- two 16-byte temporaries -- and the SAME slot that
// later holds the lerp's scaled delta is written with the second cross's
// result and never read.  That dead store survives because a call follows it
// before the slot is reused (Phase 11); the first cross's identical dead store
// does not, because nothing is called between it and the second.
static inline VIVect3 CrossV(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.y * b.z - a.z * b.y, -a.x * b.z + a.z * b.x, a.x * b.y - a.y * b.x);
}

inline VIVect3 operator-(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline VIVect3 operator+(const VIVect3 &a, const VIVect3 &b)
{
    return VIVect3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline VIVect3 operator*(const VIVect3 &a, float f)
{
    return VIVect3(a.x * f, a.y * f, a.z * f);
}

// Through an inline wrapper the Euler call's &hpr is rematerialised and the
// pseudo cse would have shared with the quaternion calls is the one spilled.
static inline void ToEulerZXY(VIMatrix44 &m, VIVect3 &hpr)
{
    m.ConvertToEulerZXY(hpr);
}

// The scaled delta is a NAMED local: as an unnamed temporary it takes the
// slot after the sum's instead of the cross's dead slot.
void VICameraEffect::Apply(VIVect3 &pos, VIVect3 &dir)
{
    if (m_manualTerp > 0.0f)
    {
        VIVect3 xyz;
        VIVect3 hpr;

        if (m_manualTerp < 1.0f)
        {
            float k = cosf(m_manualTerp * 3.14159274f * 0.5f + 3.14159274f) + 1.0f;

            VIQuat     from;
            VIQuat     to;
            VIQuat     q;
            VIVect3    right;
            VIVect3    up;
            VIVect3    fwd;
            VIMatrix44 m;

            const VIVect3 *pmh = &m_manualHpr;
            const VIVect3 *pmx = &m_manualXyz;

            m.Identity();

            fwd.x = dir.x;
            fwd.y = dir.y;
            fwd.z = dir.z;

            VIVect3 *pu = &up;

            up.x = 0.0f;
            pu->y = 1.0f;
            up.z = 0.0f;

            right = CrossV(up, fwd);
            up = CrossV(fwd, right);
            right.Normalize();
            up.Normalize();

            m.m[0].x = right.x;
            m.m[0].y = right.y;
            m.m[0].z = right.z;
            m.m[1].x = up.x;
            m.m[1].y = up.y;
            m.m[1].z = up.z;
            m.m[2].x = fwd.x;
            m.m[2].y = fwd.y;
            m.m[2].z = fwd.z;

            ToEulerZXY(m, hpr);

            from.ConvertFrom(hpr);
            to.ConvertFrom(*pmh);
            q.Slerp(from, to, k);
            q.ConvertTo(hpr);

            VIVect3 dd((pmx->x - pos.x) * k, (pmx->y - pos.y) * k,
                       (pmx->z - pos.z) * k);

            xyz = pos + dd;
        }
        else
        {
            xyz = m_manualXyz;
            hpr = m_manualHpr;
        }

        pos.x = xyz.x;
        pos.y = xyz.y;
        pos.z = xyz.z;

        dir.x = sinf(hpr.x) * cosf(hpr.y);
        dir.y = -sinf(hpr.y);
        dir.z = cosf(hpr.x) * cosf(hpr.y);
    }

    if (m_speed)
    {
        dir.Normalize();

        m_sample = m_sample % ShakeCount[m_speed];
        pos.y += m_amplitude * 0.025f * ShakeSample[m_speed][m_sample];
        dir.y += m_amplitude * 0.009f * ShakeSample[m_speed][m_sample];
        m_sample++;
    }
}
