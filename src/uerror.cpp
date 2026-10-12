// uerror.cpp -- SUPPORT 0x004acdb8..0x004ace90.  The OS error remap.
//
// The table below is the whole SCE INET error enum, spelled out as strings so a
// failed call could be traced by name.  Only TWO of the fifteen are mapped to a
// DRDP error, and the default arm still runs the lookup and throws the answer
// away: whatever printed it is gone and the call for it was left behind.
#include "drdp.h"

struct errno_constant_t
{
    int         m_iError;
    const char *m_pName;
};

static const errno_constant_t g_aErrnoConstants[] =
{
    { -500, "sceINETE_TIMEOUT" },
    { -501, "sceINETE_ABORT" },
    { -502, "sceINETE_BUSY" },
    { -503, "sceINETE_LINK_DOWN" },
    { -504, "sceINETE_INSUFFICIENT_RESOURCES" },
    { -505, "sceINETE_LOCAL_SOCKET_UNSPECIFIED" },
    { -506, "sceINETE_FOREIGN_SOCKET_UNSPECIFIED" },
    { -507, "sceINETE_CONNECTION_ALREADY_EXISTS" },
    { -508, "sceINETE_CONNECTION_DOES_NOT_EXIST" },
    { -509, "sceINETE_CONNECTION_CLOSING" },
    { -510, "sceINETE_CONNECTION_RESET" },
    { -511, "sceINETE_CONNECTION_REFUSED" },
    { -512, "sceINETE_INVALID_ARGUMENT" },
    { -513, "sceINETE_INVALID_CALL" },
    { -514, "sceINETE_NO_ROUTE" }
};

const char *get_errno_constant(int iError)
{
    const char *pName = "IOP KE_???";
    int i;

    for (i = 0; i < 15; i++)
    {
        if (iError == g_aErrnoConstants[i].m_iError)
        {
            pName = g_aErrnoConstants[i].m_pName;
            break;
        }
    }
    return pName;
}

// Phase 27's switch rule, both halves at once: the comparison tree comes out
// ordered by VALUE (-511 tested first) and the bodies in SOURCE order (-500's
// first), so the source order is the OPPOSITE of the order the code tests in.
// Written as an if/else chain either way round it is 6 or 7 words wrong.
int uerror_remap_os_error_code(int iError)
{
    int iResult = -1;

    switch (iError)
    {
    case -500:
        iResult = -4;
        break;
    case -511:
        iResult = -6;
        break;
    default:
        get_errno_constant(iError);
        break;
    }
    return iResult;
}

int uerror_get_error()
{
    return -1;
}

int uerror_get_net_error(int iResult)
{
    return uerror_remap_os_error_code(iResult);
}
