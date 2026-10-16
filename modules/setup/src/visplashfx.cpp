// visplashfx.cpp (name ours) -- SETUP 0x0056B3A0 .. 0x0056D170.  The title
// screen's animated overlays, drawn over the painting by every window that
// shows it.  A bird flaps by now and then, a flock crosses the sky along one of
// three paths, and in Frontiers (expansion flag 2) the island layer replaces
// both: twinkles, a sail, the island bobbing, a glow and a comet.
#include "setup.h"

// A source rectangle in a texture atlas, in texels.
struct VISplashFrame
{
    int x, y, w, h;

    VISplashFrame(int _x, int _y, int _w, int _h) { x = _x; y = _y; w = _w; h = _h; }
};

// A flock's flight path, entry to exit, in screen pixels.
struct VISplashPath
{
    VIVect2 from, to;

    VISplashPath(float x0, float y0, float x1, float y1) : from(x0, y0), to(x1, y1) {}
};

// ---- .data -----------------------------------------------------------------
// The bird's four flap sequences: frame 1..3 of sBirdFrames, 0 = not drawn.
static unsigned char sBirdAnim[4][8] =
{
    { 0, 1, 2, 3, 2, 3, 2, 1 },
    { 0, 1, 2, 3, 3, 3, 2, 1 },
    { 3, 3, 3, 3, 0, 0, 2, 0 },
    { 0, 1, 2, 3, 2, 3, 2, 1 },
};
static int sBirdSub = 0;      // 1/256 frame accumulators
static int sFlapSub = 0;
static int sFlockClock = 0;   // the flock's wing beat, 1/256 frames, mod 12
static int sStarFrame = 0;    // island: the three stars' frame, mod 8
static int sOddFrame = 0;     // island: toggles every frame, half-rate steps
static int sWave = 0;         // island: frames, for the bob
static int sGlow = 0;         // island: the glow cycle, 30..119
static int sAnims[4] = { 0, 0, 0, 0 };  // splashAnims1..4.tga
static int sIsland = 0;       // island.tga
static int sMaterial = 0;     // its material

// ---- .bss, constructed by the static initialiser ----------------------------
static VIVect2 sFlock[4];     // each bird's offset from the flock's centre
static VISplashPath sPaths[3] =
{
    VISplashPath(296.0f, 207.0f, -200.0f, 119.0f),
    VISplashPath(296.0f, 207.0f, -200.0f, 119.0f),
    VISplashPath(369.0f, 167.0f, -200.0f, 131.0f),
};
// The flock's wing beat: seven frames, large and small.
static VISplashFrame sFlockFrames[2][7] =
{
    {
        VISplashFrame( 0,  0, 17, 27), VISplashFrame(19,  0, 17, 27),
        VISplashFrame(38,  0, 17, 27), VISplashFrame(57,  0, 17, 27),
        VISplashFrame(76,  0, 17, 27), VISplashFrame(95,  0, 17, 27),
        VISplashFrame( 0, 29, 17, 27),
    },
    {
        VISplashFrame(19, 29, 8, 13), VISplashFrame(29, 29, 8, 13),
        VISplashFrame(39, 29, 8, 13), VISplashFrame(49, 29, 8, 13),
        VISplashFrame(59, 29, 8, 13), VISplashFrame(69, 29, 8, 13),
        VISplashFrame(79, 29, 8, 13),
    },
};
// The perched bird's flap, three 27x25 frames.
static VIVect2I sFlap[3] = { VIVect2I(19, 44), VIVect2I(48, 44), VIVect2I(77, 44) };
// The flying bird's three frames.
static VISplashFrame sBirdFrames[3] =
{
    VISplashFrame( 0, 73, 39, 54), VISplashFrame(40, 73, 44, 53), VISplashFrame(84, 73, 43, 54),
};
// Island: the stars' eight frames, and where the three stars are.
static VISplashFrame sStarFrames[8] =
{
    VISplashFrame( 0, 100, 14, 28), VISplashFrame( 15, 100, 14, 28),
    VISplashFrame(30, 100, 14, 28), VISplashFrame( 45, 100, 14, 28),
    VISplashFrame(60, 100, 14, 28), VISplashFrame( 75, 100, 14, 28),
    VISplashFrame(90, 100, 14, 28), VISplashFrame(105, 100, 14, 28),
};
static VIVect2I sStars[3] = { VIVect2I(401, 272), VIVect2I(436, 297), VIVect2I(521, 293) };
// Island: a twinkle's three phases.
static VISplashFrame sTwinkleFrames[3] =
{
    VISplashFrame(88, 29, 40, 28), VISplashFrame(88, 0, 40, 28), VISplashFrame(88, 58, 40, 28),
};
// Island: the glow's three layers.
static VISplashFrame sGlowFrames[3] =
{
    VISplashFrame(0, 57, 60, 56), VISplashFrame(0, 0, 60, 56), VISplashFrame(61, 0, 60, 56),
};
// Island: the comet's frames, and where it may appear.
static VISplashFrame sCometFrames[12] =
{
    VISplashFrame( 0,  0, 22, 18), VISplashFrame( 0, 19, 22, 18),
    VISplashFrame( 0, 38, 22, 18), VISplashFrame( 0, 57, 22, 18),
    VISplashFrame( 0, 76, 22, 18), VISplashFrame( 0, 95, 22, 18),
    VISplashFrame(23,  0, 22, 18), VISplashFrame(23, 19, 22, 18),
    VISplashFrame(23, 38, 22, 18), VISplashFrame(23, 57, 22, 18),
    VISplashFrame(23, 76, 22, 18), VISplashFrame(23, 95, 22, 18),
};
static VIVect2I sComets[7] =
{
    VIVect2I(182, 248), VIVect2I(45, 267), VIVect2I(77, 264), VIVect2I(204, 237),
    VIVect2I(0, 0), VIVect2I(0, 0), VIVect2I(0, 0),
};
static VIVect2I sTwinkles[8];

// ---- .bss ------------------------------------------------------------------
static VIRaster     *sRaster;
static VIDictionary *sDict;
static int   sBird;            // Bird_Animation.tga
static int   sFlockSize;       // 1..4 birds
static int   sFlockPath;       // which of sPaths
static int   sFlockSmall[4];   // per bird: the small frames
static int   sFlockWait;       // frames to the next flock; -1 while one flies
static float sFlockT;          // how far along the path, 0..1
static float sFlockAlpha;
static long  sTime;
static int   sFlapFrame;       // the perched bird, 0..2
static int   sFlapDir;         // +1 / -1
static int   sBirdWait;        // frames to the next flight; -1 while flying
static int   sBirdCol;         // where in its sequence
static int   sBirdRow;         // which sequence
static int   sSail;            // island: the sail's position, counts down
static int   sSailDir;         // island: which way it crosses
static int   sTwinkle[8];      // island: each twinkle's frames left
static int   sComet;           // island: the comet's frames left
static int   sCometAt;         // island: which of sComets

static int VISplashFxRenderIsland(int draw);

// 0x0056B3A0.
int VISplashFxInit(VIRaster *r, VIDictionary *d)
{
    VIResourceType type;

    sRaster = r;
    sDict = d;
    sAnims[0] = sAnims[1] = sAnims[2] = sAnims[3] = -1;
    sBird = -1;
    sIsland = -1;
    sMaterial = -1;
    if (!d->Find(VIHashResourceID("Bird_Animation.tga"), type, sBird))
        return -1;
    if (VIGetExpansionFlags() & 2)
    {
        if (!d->Find(VIHashResourceID("splashAnims1.tga"), type, sAnims[0])
            || !d->Find(VIHashResourceID("splashAnims2.tga"), type, sAnims[1])
            || !d->Find(VIHashResourceID("splashAnims3.tga"), type, sAnims[2])
            || !d->Find(VIHashResourceID("splashAnims4.tga"), type, sAnims[3])
            || !d->Find(VIHashResourceID("island.tga"), type, sIsland))
            return -1;
        if ((sMaterial = r->CreateMaterial(0)) < 0)
            return -1;
        VIMaterial *m = r->Material(sMaterial);
        m->Init(1);
        m->SetLayerFillType(0, (VIMaterialFillType)0);
        m->SetLayerModulate(0, 1);
        m->SetLayerBlendMode(0, (VIAlphaBlendMode)3);
        m->SetZWrite(0);
        m->SetZTest(1);
        m->SetDither(0);
        m->SetLayerTexture(0, sIsland, *r, *d);
    }
    VISplashFxReset();
    return 0;
}

// 0x0056B5E0.  Only the bird's handle is reset.
void VISplashFxFree()
{
    if (VIGetExpansionFlags() & 2)
    {
        if (sAnims[0] != -1)
            sRaster->ReleaseSurface(sAnims[0], *sDict);
        if (sAnims[1] != -1)
            sRaster->ReleaseSurface(sAnims[1], *sDict);
        if (sAnims[2] != -1)
            sRaster->ReleaseSurface(sAnims[2], *sDict);
        if (sAnims[3] != -1)
            sRaster->ReleaseSurface(sAnims[3], *sDict);
        if (sMaterial != -1)
            sRaster->ReleaseMaterial(sMaterial, *sDict);
        if (sIsland != -1)
            sRaster->ReleaseSurface(sIsland, *sDict);
    }
    if (sBird != -1)
        sRaster->ReleaseSurface(sBird, *sDict);
    sBird = -1;
}

// 0x0056B6F8.
void VISplashFxReset()
{
    sTime = VITimer::GetTime();
    sFlockWait = 0;
    sBirdWait = (rand() & 0xFF) + 150;
    sFlapFrame = 0;
    sFlapDir = 1;
    sSail = 316;
    sSailDir = 0;
    sComet = 0;
    sCometAt = 0;
    for (int i = 7; i >= 0; i--)
        sTwinkle[i] = 0;
}

// 0x0056B7A8.
int VISplashFxRender(int draw)
{
    int i;

    if (VIGetExpansionFlags() & 2)
        return VISplashFxRenderIsland(draw);

    // The flying bird.
    if (sBirdWait == -1)
    {
        if ((sBirdSub += 140) > 256)
        {
            sBirdSub -= 256;
            sBirdCol++;
        }
        if (sBirdCol < 8)
        {
            int f = sBirdAnim[sBirdRow][sBirdCol];
            if (f)
            {
                VISplashFrame &r = sBirdFrames[f - 1];
                if (draw)
                    sRaster->Blit(sBird, 0, VIVect2I(45, 210), VIVect2I(r.x, r.y),
                                  VIVect2I(r.w, r.h), (VIAlphaBlendMode)3, 1.0f);
            }
        }
        else
            sBirdWait = (rand() & 0xFF) + 150;
    }
    else if (sBirdWait == 0)
    {
        sBirdWait = -1;
        sBirdCol = 0;
        sBirdRow = rand() & 3;
        sBirdSub = 0;
    }
    else
        sBirdWait--;

    // The perched bird.
    if ((sFlapSub += 66) > 256)
    {
        sFlapSub -= 256;
        sFlapFrame = VIMIN(VIMAX(sFlapFrame + sFlapDir, 0), 2);
        sFlapDir = (rand() & 1) ? 1 : -1;
    }
    if (draw)
        sRaster->Blit(sBird, 0, VIVect2I(562, 185), sFlap[sFlapFrame], VIVect2I(27, 25),
                      (VIAlphaBlendMode)3, 1.0f);

    // The flock.
    long now = VITimer::GetTime();
    int dt = now - sTime;
    sTime = now;
    if ((sFlockClock += dt * 4) >= 3072)
        sFlockClock -= 3072;
    if (sFlockWait == -1)
    {
        VIVect2 p;
        sFlockAlpha = VIMIN(sFlockAlpha + 0.04f, 0.7f);
        VISplashPath &path = sPaths[sFlockPath];
        float t = sFlockT;
        float fdt = dt;
        p.x = path.from.x + (path.to.x - path.from.x) * t;
        p.y = path.from.y + (path.to.y - path.from.y) * t;
        for (i = 0; i < sFlockSize; i++)
        {
            int f = ((sFlockClock >> 8) + i * 5) % 12;
            f = f < 7 ? f : 12 - f;
            // The `+ 0.0f` is in the image: an add.s of zero gcc may not fold.
            if (draw)
                sRaster->Blit(sBird, 4,
                              VIVect2I((int)(p.x + sFlock[i].x), (int)(p.y + sFlock[i].y + 0.0f)),
                              VIVect2I(sFlockFrames[sFlockSmall[i]][f].x, sFlockFrames[sFlockSmall[i]][f].y),
                              VIVect2I(sFlockFrames[sFlockSmall[i]][f].w, sFlockFrames[sFlockSmall[i]][f].h),
                              (VIAlphaBlendMode)3, sFlockAlpha);
            switch (i)
            {
            case 1:
                sFlock[1].x += 0.11f;
                break;
            case 2:
                sFlock[2].x += 0.06f;
                sFlock[2].y += 0.12f;
                break;
            case 3:
                sFlock[3].x += 0.09f;
                sFlock[3].y += 0.045f;
                break;
            }
        }
        // In double, by libcall: 0.0001 is a double literal and 0.73f is widened.
        sFlockT = sFlockT + fdt * 0.0001 * 0.73f;
        if (sFlockT >= 1.0f)
            sFlockWait = (rand() & 0x7F) + 100;
    }
    else if (sFlockWait == 0)
    {
        sFlockWait = -1;
        sFlockSize = (rand() & 3) + 1;
        sFlockPath = rand() % 3;
        for (i = 0; i < sFlockSize; i++)
        {
            sFlockSmall[i] = rand() & 1;
            sFlock[i].x = ((rand() & 63) - 31) * 1.2f;
            sFlock[i].y = ((rand() & 63) - 31) * 0.5f;
        }
        sFlockT = 0.0f;
        sFlockAlpha = 0.0f;
    }
    else
        sFlockWait--;
    return 0;
}

// 0x0056BEC0 (name ours).  The Frontiers island layer.
static int VISplashFxRenderIsland(int draw)
{
    if (sAnims[0] == -1 || sAnims[1] == -1 || sAnims[2] == -1 || sAnims[3] == -1)
        return 0;

    sOddFrame ^= 1;

    // The twinkles: each fades through three frames over 60.
    int i;
    for (i = 0; i < 8; i++)
    {
        if (sTwinkle[i])
        {
            int t = --sTwinkle[i];
            float a0 = 0.0f, a1 = 0.0f, a2 = 0.0f;
            if (t < 30)
                a0 = t < 15 ? t / 15.0f : 1.0f - (t - 15) / 15.0f;
            if (t >= 15 && t < 45)
                a1 = t < 30 ? (t - 15) / 15.0f : 1.0f - (t - 30) / 15.0f;
            if (t >= 30 && t < 60)
                a2 = t < 45 ? (t - 30) / 15.0f : 1.0f - (t - 45) / 15.0f;
            if (draw)
            {
                if (a0 > 0.0f)
                    sRaster->Blit(sAnims[0], 4, sTwinkles[i],
                                  VIVect2I(sTwinkleFrames[0].x, sTwinkleFrames[0].y),
                                  VIVect2I(sTwinkleFrames[0].w, sTwinkleFrames[0].h),
                                  (VIAlphaBlendMode)3, a0);
                if (a1 > 0.0f)
                    sRaster->Blit(sAnims[0], 4, sTwinkles[i],
                                  VIVect2I(sTwinkleFrames[1].x, sTwinkleFrames[1].y),
                                  VIVect2I(sTwinkleFrames[1].w, sTwinkleFrames[1].h),
                                  (VIAlphaBlendMode)3, a1);
                if (a2 > 0.0f)
                    sRaster->Blit(sAnims[0], 4, sTwinkles[i],
                                  VIVect2I(sTwinkleFrames[2].x, sTwinkleFrames[2].y),
                                  VIVect2I(sTwinkleFrames[2].w, sTwinkleFrames[2].h),
                                  (VIAlphaBlendMode)3, a2);
            }
        }
    }
    if ((rand() / 17) % 30 < 3)
    {
        for (i = 0; i < 8; i++)
        {
            if (sTwinkle[i] == 0)
            {
                sTwinkle[i] = 60;
                sTwinkles[i].x = (rand() / 17) % 51 + 184;
                sTwinkles[i].y = (rand() / 17) % 43 + 265;
                break;
            }
        }
    }

    // The sail, and its reflection.
    if (--sSail < 216)
    {
        if (sOddFrame)
            sSail++;
        int x = sSailDir ? 309 - sSail : sSail + 93;
        if (draw)
        {
            int y = 289;    // the reflection, flipped, then the sail 20 above it
            sRaster->Blit(sAnims[1], sSailDir ? 7 : 6, VIVect2I(x, y), VIVect2I(0, 95),
                          VIVect2I(54, 33), (VIAlphaBlendMode)3, 0.2f);
            y -= 20;
            sRaster->Blit(sAnims[1], sSailDir ? 5 : 4, VIVect2I(x, y), VIVect2I(0, 95),
                          VIVect2I(54, 33), (VIAlphaBlendMode)3, 1.0f);
        }
        if (sSail == 0)
        {
            sSailDir ^= 1;
            sSail = (rand() / 17) % 600 + 516;
        }
    }

    sRaster->Blit(sAnims[0], 0, VIVect2I(73, 234), VIVect2I(0, 0), VIVect2I(77, 99),
                  (VIAlphaBlendMode)3, 1.0f);
    sRaster->Blit(sAnims[1], 0, VIVect2I(293, 233), VIVect2I(56, 41), VIVect2I(72, 87),
                  (VIAlphaBlendMode)3, 1.0f);

    // The three stars.
    for (i = 0; i < 3; i++)
    {
        int f = (sStarFrame + i * 3) & 7;
        if (draw)
            sRaster->Blit(sAnims[0], 4, VIVect2I(sStars[i].x, sStars[i].y),
                          VIVect2I(sStarFrames[f].x, sStarFrames[f].y),
                          VIVect2I(sStarFrames[f].w, sStarFrames[f].h), (VIAlphaBlendMode)3,
                          1.0f);
    }
    if (sOddFrame)
        sStarFrame = (sStarFrame + 1) & 7;

    // The island, bobbing.
    float x = sinf((sWave % 4000) / 4000.0f * 2.0f * 3.14159265f) * 17.0f + 451.0f;
    float y = sinf((sWave % 340) / 340.0f * 2.0f * 3.14159265f) * 8.0f + 110.0f;
    if (draw)
    {
        sRaster->SetMaterial(sMaterial);
        sRaster->BeginTriFan();
        sRaster->Color(VIColor32F(1.0f, 1.0f, 1.0f, 1.0f));
        sRaster->UV(VIVect2(0.0f, 0.0f));
        sRaster->Vertex(VIVect2(x, y));
        sRaster->UV(VIVect2(1.0f, 0.0f));
        sRaster->Vertex(VIVect2(x + 64.0f, y));
        sRaster->UV(VIVect2(1.0f, 1.0f));
        sRaster->Vertex(VIVect2(x + 64.0f, y + 64.0f));
        sRaster->UV(VIVect2(0.0f, 1.0f));
        sRaster->Vertex(VIVect2(x, y + 64.0f));
        sRaster->EndTriFan();
    }
    sWave++;

    // The glow: three layers come up one after another, then all go out.
    float glow[4];
    memset(glow, 0, sizeof(glow));
    if (sGlow < 30)
        glow[0] = sGlow / 30.0f;
    if (sGlow >= 30 && sGlow < 60)
    {
        glow[0] = 1.0f;
        glow[1] = (sGlow - 30) / 30.0f;
    }
    if (sGlow >= 60 && sGlow < 90)
    {
        glow[0] = 1.0f;
        glow[1] = 1.0f;
        glow[2] = (sGlow - 60) / 30.0f;
    }
    if (sGlow >= 90 && sGlow < 120)
    {
        glow[0] = 1.0f;
        glow[1] = 1.0f;
        glow[2] = 1.0f;
        glow[3] = (sGlow - 90) / 30.0f;
    }
    if (draw)
    {
        for (i = 0; i < 4; i++)
        {
            if (glow[i] > 0.0f)
                sRaster->Blit(sAnims[2], 4, VIVect2I(171, 100),
                              VIVect2I(sGlowFrames[i % 3].x, sGlowFrames[i % 3].y),
                              VIVect2I(sGlowFrames[i % 3].w, sGlowFrames[i % 3].h),
                              (VIAlphaBlendMode)3, glow[i]);
        }
    }
    if (++sGlow == 120)
        sGlow = 30;

    // The comet.
    if (sComet)
    {
        int f = (22 - sComet) >> 1;
        if (sOddFrame)
            sComet--;
        if (draw)
            sRaster->Blit(sAnims[3], 4, VIVect2I(sComets[sCometAt]),
                          VIVect2I(sCometFrames[f].x, sCometFrames[f].y),
                          VIVect2I(sCometFrames[f].w, sCometFrames[f].h), (VIAlphaBlendMode)3,
                          0.7f);
    }
    else if ((rand() / 17) % 100 < 2)
    {
        sComet = 22;
        sCometAt = (rand() / 17) % 4;
    }
    return 0;
}
