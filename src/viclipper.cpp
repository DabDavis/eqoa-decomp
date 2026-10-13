// VIClipper -- SUPPORT 0x0041E370 .. 0x0041E688.
#include "vi.h"

int VIClipper::Init(int n)
{
    if (m_in.Init(n) < 0)
        return -1;

    if (m_out.Init(n) < 0)
        return -1;

    return 0;
}

int VIClipper::Clear()
{
    if (m_in.Clear() < 0)
        return -1;

    if (m_out.Clear() < 0)
        return -1;

    return 0;
}

// 0x0041E408.  Sutherland-Hodgman against each plane in turn, ping-ponging
// between the two scratch arrays (the first pass writes m_in).  Negative
// distance is INSIDE; a point exactly on a plane is kept and is never the end
// of a crossing.  An empty result returns 0 with both outputs cleared.
int VIClipper::Clip(int n, VIVect3 *v, int np, VIPlane *planes, int &numOut,
                    VIVect3 **out)
{
    int cnt = 0;
    VIVect3 *dst = m_in.m_data;
    int flip = 1;

    for (; np != 0; np--, planes++)
    {
        VIVect3 *prev = v + n - 1;
        float dPrev = planes->Distance(*prev);

        for (; n != 0; n--, v++)
        {
            // ONE `t` for both crossings: a block-scoped one per branch puts
            // the difference in t's own register.
            float t;
            float dCur = planes->Distance(*v);

            if (dCur < 0.0f)
            {
                if (0.0f < dPrev)
                {
                    t = dPrev / (dPrev - dCur);
                    *dst = *v;
                    *dst -= *prev;
                    *dst *= t;
                    *dst += *prev;
                    cnt++;
                    dst++;
                }
                *dst = *v;
                cnt++;
                dst++;
            }
            else if (0.0f < dCur)
            {
                if (dPrev < 0.0f)
                {
                    t = dPrev / (dPrev - dCur);
                    *dst = *v;
                    *dst -= *prev;
                    *dst *= t;
                    *dst += *prev;
                    cnt++;
                    dst++;
                }
            }
            else
            {
                *dst = *v;
                cnt++;
                dst++;
            }
            prev = v;
            dPrev = dCur;
        }

        if (cnt == 0)
        {
            numOut = 0;
            *out = 0;
            return 0;
        }
        if (flip)
        {
            v = &m_in[0];
            dst = &m_out[0];
        }
        else
        {
            v = &m_out[0];
            dst = &m_in[0];
        }
        n = cnt;
        flip = 1 - flip;
        cnt = 0;
    }

    numOut = n;
    *out = v;
    return 1;
}
