// vifader.h -- the screen fade.  One source file, linked into two modules:
// SETUP (0x0056B088) and STATION (0x002B95B8), byte-identical in both.
// STATION's src/vifader.cpp and src/vifader.h are symlinks to SETUP's.
#ifndef VIFADER_H
#define VIFADER_H

#include "vi.h"

// A screen fade: one solid-fill material drawn over the whole frame at m_alpha,
// which moves toward m_target at m_rate per second.  32 bytes; VISetup embeds
// one at +0xAE48.
class VIFader
{
public:
    VIFader();
    ~VIFader();

    int  Init(VIRaster *r, VIDictionary *d, float alpha);
    void Free();
    void Begin(float target, float seconds);
    void End();
    void Update();
    // Inline (VIFader has no vtable, so no copy is emitted anywhere): its
    // callers read m_alpha off `&m_fader` as a value, not off `this`.
    float Alpha() const { return m_alpha; }

    VIRaster     *m_raster;     // +0x00
    VIDictionary *m_dict;       // +0x04  never set by Init
    float         m_alpha;      // +0x08
    float         m_target;     // +0x0C
    float         m_rate;       // +0x10  alpha per second
    int           m_material;   // +0x14
    long          m_time;       // +0x18
};

#endif
