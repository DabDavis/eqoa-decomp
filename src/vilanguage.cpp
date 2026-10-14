#line 1 "C:/code-frontiers-gm/core/VILanguage.cpp"
// VILanguage -- SUPPORT 0x003E96D8 .. 0x003E9818.
#include "vi.h"

extern "C" int  strcmp(const char *a, const char *b);
extern "C" void _VIAssert(const char *expr, const char *file, int line);

#define VIASSERT(x)  if (!(x)) _VIAssert(#x, __FILE__, __LINE__);
#define VILANGUAGEID_MAX VILANG_COUNT   // the name the original's asserts quote
const char  *VILanguage::Name[VILANG_COUNT] =
{
    "us", "uk", "fr", "de",
    "it", "nl", "pt", "sp"
};

const char  *VILanguage::IsoCode[VILANG_COUNT] =
{
    "en", "en", "fr", "de", "it", "nl", "pt", "sp"
};

VILanguageID VILanguage::Default;

const char *VILanguage::GetName(VILanguageID LanguageId)
{
// The two `_VIAssert` calls carry LINE NUMBERS as immediates -- 59 and 69
// -- so the original file is dated by them and this one is padded to agree.
// Everything above the first accessor is therefore filler of the right
// height, not a claim about what the original held.
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
    VIASSERT(0 <= LanguageId && LanguageId < VILANGUAGEID_MAX);
    return Name[LanguageId];
}

const char *VILanguage::GetIsoCode(VILanguageID LanguageId)
{




    VIASSERT(0 <= LanguageId && LanguageId < VILANGUAGEID_MAX);
    return IsoCode[LanguageId];
}

int VILanguage::GetID(const char *name)
{
    for (int i = 0; i < VILANG_COUNT; i++)
    {
        if (strcmp(Name[i], name) == 0)
            return i;
    }

    return -1;
}

VILanguageID VILanguage::GetDefault()
{
    return Default;
}

void VILanguage::SetDefault(VILanguageID id)
{
    Default = id;
}
