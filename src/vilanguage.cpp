// VILanguage -- SUPPORT 0x003E96D8 .. 0x003E9818.
#include "vi.h"

extern "C" int  strcmp(const char *a, const char *b);
extern "C" void _VIAssert(const char *file, const char *expr, int line);

#define VIASSERT(x)  if (!(x)) _VIAssert(__FILE__, #x, __LINE__);

const char  *VILanguage::Name[VILANG_COUNT] =
{
    "English", "French", "German", "Spanish",
    "Italian", "Dutch", "Portuguese", "Japanese"
};

const char  *VILanguage::IsoCode[VILANG_COUNT] =
{
    "en", "fr", "de", "es", "it", "nl", "pt", "ja"
};

VILanguageID VILanguage::Default;

const char *VILanguage::GetName(VILanguageID id)
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
    VIASSERT(id < sizeof(Name) / sizeof(Name[0]));
    return Name[id];
}

const char *VILanguage::GetIsoCode(VILanguageID id)
{




    VIASSERT(id < sizeof(IsoCode) / sizeof(IsoCode[0]));
    return IsoCode[id];
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
