// VIESFParse -- SUPPORT 0x00431290 .. 0x0043ED88, its own TU.  THE PARSER: the
// one piece of code that turns an .ESF/.CSF object tree into live engine
// objects.  Everything the game draws, hears or collides with comes through
// here.
//
// The shape is the same all the way down: VIObjFile hands out a tagged object,
// a Parse*Obj reads its fields with VIObjFile::Read, and its sub-objects are
// walked by a loop that calls back into the object dispatcher.  The seven
// pointers at the top of the class are the parse's context -- scene, raster,
// sound device, collide, dictionary, the open file and the zone being streamed
// -- set on the way in by whichever entry point was called and cleared again
// on the way out.
#include "vi.h"

VIESFParse::VIESFParse()
{
    m_permResource = 1;
    m_file = 0;
    m_raster = 0;
    m_collide = 0;
    m_scene = 0;
    m_dict = 0;
    m_zone = 0;
    m_progress = 0;
    m_progressArg = 0;
}

VIESFParse::~VIESFParse()
{
}

int VIESFParse::SetPermResource(int on)
{
    m_permResource = on;
    return 0;
}

void VIESFParse::SetProgressCallback(void (*fn)(void *), void *arg)
{
    m_progress = fn;
    m_progressArg = arg;
}

int VIESFParse::Parse(VIObjFile &f, VIScene &sc)
{
    m_scene = &sc;
    m_dict = sc.m_dict;
    m_raster = sc.m_raster;
    m_collide = sc.m_collide;
    m_sound = sc.m_sound;
    m_zone = 0;
    m_file = &f;

    if (f.FileType() != 0xAB4F) return -1;

    int n = f.NumObjects();
    for (int i = 0; i < n; i++)
        if (ParseObject() < 0) return -1;

    m_scene = 0;
    m_dict = 0;
    m_raster = 0;
    m_sound = 0;
    m_collide = 0;
    m_zone = 0;
    m_file = 0;
    return 0;
}

int VIESFParse::Parse(const char *path, VIScene &sc)
{
    VIObjFile f;

    m_scene = &sc;
    m_dict = sc.m_dict;
    m_raster = sc.m_raster;
    m_collide = sc.m_collide;
    m_sound = sc.m_sound;
    m_zone = 0;
    m_file = &f;

    if (f.Open(path, VIObjFile::Mode_Read) >= 0 && f.FileType() == 0xAB4F)
    {
        int n = f.NumObjects();
        for (int i = 0; i < n; i++)
            if (ParseObject() < 0) return -1;
        if (f.Close() >= 0)
        {
            m_scene = 0;
            m_dict = 0;
            m_raster = 0;
            m_sound = 0;
            m_collide = 0;
            m_zone = 0;
            m_file = 0;
            return 0;
        }
    }
    return -1;
}

int VIESFParse::ParseZoneResource(VIObjFile &f, int zone, VIScene &sc)
{
    m_scene = &sc;
    m_dict = sc.m_dict;
    m_raster = sc.m_raster;
    m_collide = sc.m_collide;
    m_sound = sc.m_sound;
    m_zone = &sc.m_world.m_zones[zone];
    m_file = &f;

    if (f.FileType() != 0xAB4F) return -1;

    int n = f.NumObjects();
    for (int i = 0; i < n; i++)
        if (ParseZoneResource() < 0) return -1;

    m_scene = 0;
    m_dict = 0;
    m_raster = 0;
    m_sound = 0;
    m_collide = 0;
    m_zone = 0;
    m_file = 0;
    return 0;
}

int VIESFParse::ParseSprite(const char *path, VIScene &sc)
{
    VIObjFile f;

    m_scene = &sc;
    m_dict = sc.m_dict;
    m_raster = sc.m_raster;
    m_collide = sc.m_collide;
    m_sound = sc.m_sound;
    m_zone = 0;
    m_file = &f;

    if (f.Open(path, VIObjFile::Mode_Read) >= 0 && f.FileType() == 0xAB4F &&
        f.NumObjects() == 1)
    {
        int i = ParseSprite();
        if (i >= 0 && f.Close() >= 0)
        {
            m_scene = 0;
            m_dict = 0;
            m_raster = 0;
            m_sound = 0;
            m_collide = 0;
            m_zone = 0;
            m_file = 0;
            return i;
        }
    }
    return -1;
}

int VIESFParse::ParseSound(const char *path, VIScene &sc)
{
    VIObjFile f;

    m_scene = &sc;
    m_dict = sc.m_dict;
    m_raster = sc.m_raster;
    m_collide = sc.m_collide;
    m_sound = sc.m_sound;
    m_zone = 0;
    m_file = &f;

    if (f.Open(path, VIObjFile::Mode_Read) >= 0 && f.FileType() == 0xAB4F &&
        f.NumObjects() == 1)
    {
        int i = ParseSound();
        if (i >= 0 && f.Close() >= 0)
        {
            m_scene = 0;
            m_dict = 0;
            m_raster = 0;
            m_sound = 0;
            m_collide = 0;
            m_zone = 0;
            m_file = 0;
            return i;
        }
    }
    return -1;
}

int VIESFParse::ParseWorld(const char *path, int i, VIScene &sc)
{
    VIObjFile f;

    m_scene = &sc;
    m_dict = sc.m_dict;
    m_raster = sc.m_raster;
    m_collide = sc.m_collide;
    m_sound = sc.m_sound;
    m_zone = 0;
    m_file = &f;

    if (f.Open(path, VIObjFile::Mode_Read) >= 0 && f.FileType() == 0xAB4F &&
        f.NumObjects() == 1 && ParseWorld(i) >= 0 && f.Close() >= 0)
    {
        m_scene = 0;
        m_dict = 0;
        m_raster = 0;
        m_sound = 0;
        m_collide = 0;
        m_zone = 0;
        m_file = 0;
        return 0;
    }
    return -1;
}

int VIESFParse::ParseResourceFile(const char *path, int n, VIScene &sc,
                                  VIResourceDir &d)
{
    VIObjFile f;

    m_scene = &sc;
    m_dict = sc.m_dict;
    m_raster = sc.m_raster;
    m_collide = sc.m_collide;
    m_sound = sc.m_sound;
    m_zone = 0;
    m_file = &f;

    if (f.Open(path, VIObjFile::Mode_Read) >= 0 && f.FileType() == 0xAB4F &&
        ParseResourceDir(n, d) >= 0 && f.Close() >= 0)
    {
        m_scene = 0;
        m_dict = 0;
        m_raster = 0;
        m_sound = 0;
        m_collide = 0;
        m_zone = 0;
        m_file = 0;
        return 0;
    }
    return -1;
}

// The object dispatcher.  VIObjFile::ReadBegin has already been told nothing
// about the type -- it hands one back -- so this is where an ESF tag becomes a
// call.  The cases are in source order; the comparison tree the compiler builds
// over them is not.
int VIESFParse::ParseObject()
{
    unsigned short type, version;

    if (m_progress) m_progress(m_progressArg);

    if (m_file->ReadBegin(type, version) < 0) goto fail;

    switch (type)
    {
    case 0x2000: if (ParseSimpleSpriteObj() >= 0) return 0; goto fail;
    case 0x2200: if (ParseHSpriteObj() >= 0) return 0; goto fail;
    case 0x2700: if (ParseCSpriteObj() >= 0) return 0; goto fail;
    case 0x2600: if (ParseHSpriteAnimObj() >= 0) return 0; goto fail;
    case 0x2A00: if (ParseLODSpriteObj() >= 0) return 0; goto fail;
    case 0x2B00: if (ParsePointLightObj() >= 0) return 0; goto fail;
    case 0xB100: if (ParseSoundSpriteObj() >= 0) return 0; goto fail;
    case 0xC100: if (ParseParticleSpriteObj() >= 0) return 0; goto fail;
    case 0x2C00: if (ParseGroupSpriteObj() >= 0) return 0; goto fail;
    case 0x2D00: if (ParsePointSpriteObj() >= 0) return 0; goto fail;
    case 0x2E00: if (ParseStreamAudioSpriteObj() >= 0) return 0; goto fail;
    case 0x2F00: if (ParseFloraSpriteObj() >= 0) return 0; goto fail;
    case 0xC300: if (ParseEffectVolumeSpriteObj() >= 0) return 0; goto fail;
    case 0x6000: if (ParseActorObj() >= 0) return 0; goto fail;
    case 0x1001: if (ParseSurfaceArrayObj() >= 0) return 0; goto fail;
    case 0x1000: if (ParseSurfaceObj() >= 0) return 0; goto fail;
    case 0x7000: if (ParseFontObj() >= 0) return 0; goto fail;
    case 0x1110: if (ParseMaterialPalObj() >= 0) return 0; goto fail;
    case 0xB000: if (ParseAdpcmObj() >= 0) return 0; goto fail;
    case 0xC000: if (ParseParticleDefinitionObj() >= 0) return 0; goto fail;
    case 0xB030: if (ParseXmObj() >= 0) return 0; goto fail;
    case 0xC200: if (ParseSpellEffectObj() >= 0) return 0; goto fail;
    case 0xA000:
        {
            VIResourceDir dir;
            if (ParseResourceDirObj(0, dir) < 0) return -1;
            return 0;
        }
    case 0x8000: if (ParseWorldObj(0) >= 0) return 0; goto fail;
    default:
        if (m_file->ReadEnd() < 0) goto fail;
        return 0;
    }
    return 0;
fail:
    return -1;
}

// The sprite dispatcher.  Same shape as ParseObject, but it hands the sprite
// index back rather than a status, so the tail returns `i` and not zero.
int VIESFParse::ParseSprite()
{
    unsigned short type, version;
    int i;

    if (m_file->ReadBegin(type, version) < 0) goto fail;

    switch (type)
    {
    case 0x2000: i = ParseSimpleSpriteObj(); break;
    case 0x2200: i = ParseHSpriteObj(); break;
    case 0x2700: i = ParseCSpriteObj(); break;
    case 0x2A00: i = ParseLODSpriteObj(); break;
    case 0x2B00: i = ParsePointLightObj(); break;
    case 0xB100: i = ParseSoundSpriteObj(); break;
    case 0xC100: i = ParseParticleSpriteObj(); break;
    case 0x2C00: i = ParseGroupSpriteObj(); break;
    case 0x2D00: i = ParsePointSpriteObj(); break;
    case 0x2E00: i = ParseStreamAudioSpriteObj(); break;
    case 0x2F00: i = ParseFloraSpriteObj(); break;
    case 0xC300: i = ParseEffectVolumeSpriteObj(); break;
    default: goto dflt;
    }
    if (i < 0) goto fail;
    goto out;
fail:
    return -1;
dflt:
    i = -1;
    if (m_file->ReadEnd() < 0) return -1;
out:
    return i;
}

int VIESFParse::ParseSound()
{
    unsigned short type, version;
    int i;

    if (m_file->ReadBegin(type, version) < 0) goto fail;

    switch (type)
    {
    case 0xB000: i = ParseAdpcmObj(); break;
    case 0xB030: i = ParseXmObj(); break;
    default: goto dflt;
    }
    if (i < 0) goto fail;
    goto out;
fail:
    return -1;
dflt:
    i = -1;
    if (m_file->ReadEnd() < 0) return -1;
out:
    return i;
}

int VIESFParse::ParseSubSprite()
{
    unsigned short type, version;
    int i;

    if (m_progress) m_progress(m_progressArg);

    if (m_file->ReadBegin(type, version) < 0) goto fail;

    switch (type)
    {
    case 0x2310: i = ParseSimpleSubSpriteObj(); break;
    case 0x2320: i = ParseSkinSubSpriteObj(); break;
    case 0x2A40: i = ParseSkinLODSpriteObj(); break;
    default: goto dflt;
    }
    if (i < 0) goto fail;
    goto out;
fail:
    return -1;
dflt:
    i = -1;
    if (m_file->ReadEnd() < 0) return -1;
out:
    return i;
}

int VIESFParse::ParseSurfaces(VIArray<int> &a)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1001) return -1;
    if (a.Init(m_file->NumSubObjects()) < 0) return -1;

    for (int i = 0; i < m_file->NumSubObjects(); i++)
    {
        int s = ParseSurface();
        if (s < 0) return -1;
        a[i] = s;
    }
    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseSurfaceArray()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1001) return -1;

    for (int i = 0; i < m_file->NumSubObjects(); i++)
        if (ParseSurface() < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseSurfaceArrayObj()
{
    if (m_raster)
    {
        for (int i = 0; i < m_file->NumSubObjects(); i++)
            if (ParseSurface() < 0) return -1;
    }
    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseSurface()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1000) return -1;

    int i = ParseSurfaceObj();
    if (i < 0) return -1;
    return i;
}

int VIESFParse::ParseMaterialPal()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1110) return -1;
    return ParseMaterialPalObj();
}

int VIESFParse::ParseMaterial()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1100) return -1;

    int i = m_raster->CreateMaterial(m_permResource);
    if (i < 0) return -1;
    if (ParseMaterial(m_raster->Material(i)) < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// --------------------------------------------------------------------------
// Phase 58.  The wrappers and the array walkers.
//
// Three shapes repeat all the way down the file, and every one of them is a
// ReadBegin / tag test / body / ReadEnd sandwich:
//
//   the WRAPPER      -- ReadBegin, check the tag, hand off to the *Obj that
//                       does the work, and forward its result;
//   the WALKER       -- ReadBegin, check the tag, loop over the sub-objects
//                       calling one parse per sub-object, ReadEnd;
//   the ARRAY WALKER -- the same, but the count is hoisted into a local
//                       because the array being filled was sized from it.
//
// A wrapper that hands back an INDEX ends `i < 0 ? -1 : i` (a `movn`); one that
// hands back a STATUS ends `< 0 ? -1 : 0` (a `movz`), and the two are told
// apart by which constant the conditional move puts in v0.
// --------------------------------------------------------------------------

int VIESFParse::ParseActor()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x6000) return -1;
    return ParseActorObj();
}

// The only ReadBegin(offset, ...) call site in the parser: a zone's base object
// is reached by seeking to the offset the world's zone table gave, not by
// walking the object stream.  The offset is not passed on.
int VIESFParse::ParseZoneBase(unsigned long offset)
{
    unsigned short type, version;

    if (m_file->ReadBegin(offset, type, version) < 0) return -1;
    if (type != 0x3200) return -1;
    return ParseZoneBaseObj();
}

int VIESFParse::ParseZoneRoomActors()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x3270) return -1;
    return ParseZoneRoomActorsObj();
}

int VIESFParse::ParseZoneRoomStaticLightings()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x3290) return -1;
    return ParseZoneRoomStaticLightingsObj();
}

int VIESFParse::ParseHSpriteAnim()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2600) return -1;

    int i = ParseHSpriteAnimObj();
    if (i < 0) return -1;
    return i;
}

int VIESFParse::ParseWorld(int i)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x8000) return -1;
    if (ParseWorldObj(i) < 0) return -1;
    return 0;
}

int VIESFParse::ParseStaticLighting(VIArray<int> &a)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x6010) return -1;

    int i = ParseStaticLightingObj(a);
    if (i < 0) return -1;
    return i;
}

int VIESFParse::ParseResourceDir(int i, VIResourceDir &d)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0xA000) return -1;
    if (ParseResourceDirObj(i, d) < 0) return -1;
    return 0;
}

// ---- the walkers: the count is re-read every iteration --------------------

int VIESFParse::ParseZoneResources()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x3100) return -1;

    for (int i = 0; i < m_file->NumSubObjects(); i++)
        if (ParseZoneResource() < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseZoneActors()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x3290) return -1;

    for (int i = 0; i < m_file->NumSubObjects(); i++)
        if (ParseZoneRoomActors() < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseZoneStaticLightings()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x32B0) return -1;

    for (int i = 0; i < m_file->NumSubObjects(); i++)
        if (ParseZoneRoomStaticLightings() < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseCSpritePartDefinitions()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2950) return -1;

    for (int i = 0; i < m_file->NumSubObjects(); i++)
        if (ParseParticleDefinition() < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The sound array is the one walker with a device test: with no VISoundDevice
// the sub-objects are skipped and only the ReadEnd runs.
int VIESFParse::ParseSoundArray()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0xB070) return -1;

    if (m_sound)
    {
        for (int i = 0; i < m_file->NumSubObjects(); i++)
            if (ParseSound() < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseZoneFlora()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x32D0) return -1;
    if (ParseZoneFloraSpriteArray(m_zone->m_flora) < 0) return -1;
    if (ParseZoneFloraSets(m_zone->m_flora) < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// ---- the array walkers: the count is hoisted ------------------------------

int VIESFParse::ParseHSpriteSpriteArray()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2220) return -1;

    int n = m_file->NumSubObjects();
    for (int i = 0; i < n; i++)
        if (ParseSubSprite() < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseHSpriteAnimArray()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2610) return -1;

    int n = m_file->NumSubObjects();
    for (int i = 0; i < n; i++)
        if (ParseHSpriteAnim() < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseCSpriteSpriteArray()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2800) return -1;

    int n = m_file->NumSubObjects();
    for (int i = 0; i < n; i++)
        if (ParseSubSprite() < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseGroupSpriteSpriteArray()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2C20) return -1;

    int n = m_file->NumSubObjects();
    for (int i = 0; i < n; i++)
        if (ParseSprite() < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseSkinLODSpriteSpriteArray()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2A60) return -1;

    int n = m_file->NumSubObjects();
    for (int i = 0; i < n; i++)
        if (ParseSubSprite() < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The one array walker that reads the VERSION: an old LOD sprite lists sub
// sprites, a new one lists whole sprites.
int VIESFParse::ParseLODSpriteSpriteArray()
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2A20) return -1;

    if (version == 0)
    {
        int n = m_file->NumSubObjects();
        for (int i = 0; i < n; i++)
            if (ParseSubSprite() < 0) return -1;
    }
    else
    {
        int n = m_file->NumSubObjects();
        for (int i = 0; i < n; i++)
            if (ParseSprite() < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseZoneRooms(VIZone &z)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x3230) return -1;
    if (z.m_rooms.Init(m_file->NumSubObjects()) < 0) return -1;

    for (int i = 0; i < m_file->NumSubObjects(); i++)
        if (ParseZoneRoom(z.m_rooms[i]) < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The region grid is one profile byte per cell, and the array is already sized
// -- the world header allocated it -- so this is a straight read loop with no
// status test on the read at all.
int VIESFParse::ParseWorldRegions(VIWorld &w)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x8240) return -1;

    for (int i = 0; i < w.m_regions.m_size; i++)
        m_file->Read(w.m_regions[i].m_flags);

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseWorldObj(int i)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x8100) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    if (ParseWorldBase(m_scene->m_world) < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    if (i == 0)
    {
        if (ParseWorldZones(m_scene->m_world) < 0) return -1;
    }
    return 0;
}

// The flora sprite table: each sub-object is a whole sprite, and the index it
// hands back is the first field of a VIRadialFloraModel.
int VIESFParse::ParseZoneFloraSpriteArray(VIZoneRadialFlora &f)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x32E0) return -1;
    if (f.m_models.Init(m_file->NumSubObjects()) < 0) return -1;

    for (int i = 0; i < m_file->NumSubObjects(); i++)
    {
        int s = ParseSprite();
        if (s < 0) return -1;
        f.m_models[i].m_a = s;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// A trigger array is sized by a COUNT off the wire, not by the sub-object
// count, and only the first word of each 16-byte trigger comes off the file.
int VIESFParse::ParseHSpriteTriggers(VIHSprite &h)
{
    unsigned short type, version;
    int n;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2450) return -1;

    m_file->Read(n);
    if (h.m_triggers.Init(n) < 0) return -1;

    for (int i = 0; i < n; i++)
        m_file->Read(h.m_triggers[i].m_a);

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseResourceDirObj(int i, VIResourceDir &d)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0xA010) return -1;

    if (i == 0)
    {
        int n = m_file->NumSubObjects();
        for (int j = 0; j < n; j++)
            if (ParseObject() < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    if (ParseResourceTable(d) < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The pre-translation table -- the per-instance offsets a zone's actors are
// placed at.  The VIVect3 is ONE local reused every iteration, which is what
// the three stack slots at sp+0 and the single VIVect3 assignment say.
int VIESFParse::ParseZonePreTranslations(VIZone &z)
{
    VIVect3 v;
    unsigned short type, version;
    int n;

    if (m_progress) m_progress(m_progressArg);

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x3250) return -1;

    m_file->Read(n);
    if (z.m_pretrans.Init(n) < 0) return -1;

    for (int i = 0; i < n; i++)
    {
        m_file->Read(v.x);
        m_file->Read(v.y);
        m_file->Read(v.z);
        z.m_pretrans[i] = v;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The resource table is the DIRECTORY of a .ESF/.CSF: (offset, size, DictID)
// per entry, and it is what VILoader binary-searches to stream one object out
// of a file without reading the rest.
int VIESFParse::ParseResourceTable(VIArray<VIResourceElem> &a)
{
    unsigned short type, version;
    int n;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x9000) return -1;

    m_file->Read(n);
    if (a.Init(n) < 0) return -1;

    for (int i = 0; i < n; i++)
    {
        m_file->Read(a[i].m_offset);
        m_file->Read(a[i].m_size);
        m_file->Read(a[i].m_id);
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

int VIESFParse::ParseZoneStaticTable(VIArray<VIZoneStaticTableElem> &a)
{
    unsigned short type, version;
    int n;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x32C0) return -1;

    m_file->Read(n);
    if (a.Init(n) < 0) return -1;

    for (int i = 0; i < n; i++)
    {
        m_file->Read(a[i].m_offset);
        m_file->Read(a[i].m_size);
        m_file->Read(a[i].m_offset2);
        m_file->Read(a[i].m_size2);
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The node list: nine named skeleton nodes by index.  Version 0 files carry a
// slot number that is not the one the engine uses, so every entry in one lands
// in slot 3 -- the `movz` is the whole of that rule.
int VIESFParse::ParseCSpriteNodeIDList(VICSprite &c)
{
    unsigned short type, version;
    int n;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2915) return -1;

    m_file->Read(n);

    for (int i = 0; i < n; i++)
    {
        int slot, node;
        m_file->Read(slot);

        int j = slot;
        if (version == 0) j = 3;

        m_file->Read(node);
        c.m_node[j] = node;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The attach-slot list: which skeleton node each of the three held items hangs
// off.  Nothing else in VICSprite ever writes m_items[].m_node.
int VIESFParse::ParseCSpriteASlotList(VICSprite &c)
{
    unsigned short type, version;
    int n;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2920) return -1;

    m_file->Read(n);

    for (int i = 0; i < n; i++)
    {
        int slot, node;
        m_file->Read(slot);

        int j = slot;

        m_file->Read(node);
        c.m_items[j].m_node = node;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The continuous sound: a DictID resolved to a device sound index at parse
// time, and left at -1 when there is no sound device or no sound named.
int VIESFParse::ParseCSpriteContSound(VICSprite &c)
{
    unsigned short type, version;
    unsigned int id;
    int i;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2940) return -1;

    m_file->Read(id);

    if (m_sound && id)
    {
        if (m_dict->FindTyped(id, VIRT_Sound, i) == 0) return -1;
        c.m_snd.m_sound = i;
    }
    else
    {
        c.m_snd.m_sound = -1;
    }
    m_file->Read(c.m_snd.m_volume);

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The particle-definition wrapper.  Unlike every other one it does NOT fail on
// a foreign tag: it closes the object and hands back -1 as a legitimate "no
// definition here" answer, which is what the `li s0,-1` in the ReadEnd's delay
// slot says.
int VIESFParse::ParseParticleDefinition()
{
    unsigned short type, version;
    int i;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    if (type != 0xC000)
    {
        i = -1;
        if (m_file->ReadEnd() < 0) return -1;
    }
    else
    {
        i = ParseParticleDefinitionObj();
        if (i < 0) return -1;
    }

    return i;
}

// The world base.  Its VIWorld argument is never touched -- every sub-parse is
// handed the SCENE's world -- and the version gates two of the four: a header
// only from version 1 up, the region grid only from version 2 up.
int VIESFParse::ParseWorldBase(VIWorld &w)
{
    unsigned short type, version;

    if (m_scene->RemoveAllActors() < 0) return -1;
    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x8200) return -1;

    if (version != 0)
        if (ParseWorldBaseHeader(m_scene->m_world) < 0) return -1;

    if (ParseWorldZoneProxies(m_scene->m_world) < 0) return -1;
    if (ParseWorldTree(m_scene->m_world.m_tree) < 0) return -1;

    if (version >= 2)
        if (ParseWorldRegions(m_scene->m_world) < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    if (m_scene->UpdateAllActors() < 0) return -1;
    return 0;
}

// The skin list: a DictID per body/node slot, resolved through the dictionary
// and left at -1 when the file names none.
int VIESFParse::ParseCSpriteSkinList(VICSprite &c)
{
    unsigned short type, version;
    int n;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2900) return -1;

    m_file->Read(n);

    for (int i = 0; i < n; i++)
    {
        unsigned int id;
        int slot;
        VIResourceType t;
        int r;

        m_file->Read(id);
        m_file->Read(slot);

        int j = slot;

        r = -1;

        if (id)
            if (m_dict->Find(id, t, r) == 0) return -1;

        c.m_body[j] = r;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The texture-slot list: which palette entry and which material layer each of
// the nineteen swap slots drives.
int VIESFParse::ParseCSpriteTSlotList(VICSprite &c)
{
    unsigned short type, version;
    int n;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2930) return -1;

    m_file->Read(n);

    for (int i = 0; i < n; i++)
    {
        int slot, pal, layer;
        m_file->Read(slot);

        int j = slot;

        m_file->Read(pal);
        m_file->Read(layer);
        c.m_slots[j].m_palEntry = pal;
        c.m_slots[j].m_layer = layer;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// A point sprite is the smallest leaf there is -- a DictID and a point type --
// so this is the cleanest example of the shape every *Obj parser has: look the
// id up first and hand back what is already loaded, else create the sprite,
// register it and initialise it.
int VIESFParse::ParsePointSpriteObj()
{
    unsigned int id;
    int t;
    VIPointType type;
    VIResourceType rt;
    int i;

    m_file->Read(id);
    m_file->Read(t);
    type = (VIPointType)t;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 23) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypePoint, m_permResource);
    if (i < 0) return -1;

    VISprite *p = m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)23, i) < 0) return -1;

    ((VIPointSprite *)p)->Init(type);

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The zone base object.  The VIArray<int> local is the actor list the scene
// hands back when it evicts the zone's actors, and it is handed back in at the
// end -- so every failure path has to run its destructor, which is why they all
// cross-jump onto one.
int VIESFParse::ParseZoneBaseObj()
{
    VIArray<int> actors;

    if (m_scene->RemoveActorsFromZone(*m_zone, actors) < 0) return -1;
    if (ParseZoneTree(m_zone->m_tree) < 0) return -1;
    if (ParseZonePreTranslations(*m_zone) < 0) return -1;
    if (ParseZoneRooms(*m_zone) < 0) return -1;
    if (ParseResourceTable(m_zone->m_resources) < 0) return -1;
    if (ParseZoneStaticTable(m_zone->m_statics) < 0) return -1;

    if (m_file->ObjectVersion() != 0)
    {
        if (ParseZoneFlora() < 0) return -1;
        m_zone->m_flora.Precalc(*m_scene);
    }

    if (m_scene->UpdateActors(actors) < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// A room's actor list.  The room index arrives in its OWN object, which is
// closed before the actor object is opened -- two ReadBegin/ReadEnd pairs, not
// one nested pair.
int VIESFParse::ParseZoneRoomActorsObj()
{
    unsigned short type, version;
    int room;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x3280) return -1;

    m_file->Read(room);
    if (m_file->ReadEnd() < 0) return -1;

    VIZoneRoom &r = m_zone->m_rooms[room];

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x6040) return -1;

    if (r.m_a3C.Init(m_file->NumSubObjects()) < 0) return -1;

    for (int i = 0; i < m_file->NumSubObjects(); i++)
    {
        r.m_a3C[i] = ParseZoneActor();
        if (r.m_a3C[i] < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The same shape again, and the one that says what m_a44 IS: a static lighting
// index per ACTOR, since the parse is handed the room's actor array.
int VIESFParse::ParseZoneRoomStaticLightingsObj()
{
    unsigned short type, version;
    int room;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x32A0) return -1;

    m_file->Read(room);
    if (m_file->ReadEnd() < 0) return -1;

    VIZoneRoom &r = m_zone->m_rooms[room];

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x6030) return -1;

    if (r.m_a44.Init(m_file->NumSubObjects()) < 0) return -1;

    for (int i = 0; i < m_file->NumSubObjects(); i++)
    {
        r.m_a44[i] = ParseStaticLighting(r.m_a3C);
        if (r.m_a44[i] < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The LOD table: a switch distance and a sprite per level, and the sprite is
// SHARED rather than parsed -- it is already in the dictionary.  The sprite's
// own cull distance is set from the last level's.
int VIESFParse::ParseLODSpriteLevels(VILODSprite &l)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2A30) return -1;

    m_file->Read(l.m_numLevels);

    for (int i = 0; i < l.m_numLevels; i++)
    {
        unsigned int id;
        VIResourceType t;

        m_file->Read(id);
        if (m_dict->Find(id, t, l.m_levels[i].m_sprite) == 0
            || m_scene->ShareSprite(l.m_levels[i].m_sprite) < 0)
            return -1;
        m_file->Read(l.m_levels[i].m_dist);
    }

    if (l.m_numLevels)
        l.m_offDist = l.m_levels[l.m_numLevels - 1].m_dist;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The world's zone table.  m_zone is the parse's context and it is set and
// cleared around every zone, which is what makes the whole Zone* family
// argument-free.
int VIESFParse::ParseWorldZones(VIWorld &w)
{
    unsigned short type, version;

    for (int i = 0; i < w.m_zones.m_size; i++)
    {
        m_zone = &w.m_zones[i];

        if (ParseZoneBase(m_zone->m_offset) < 0) return -1;
        if (m_file->ReadBegin(m_zone->m_baseOffset, type, version) < 0) return -1;
        if (type != 0x3000) return -1;
        if (ParseZoneResources() < 0) return -1;
        if (ParseZoneActors() < 0) return -1;
        if (ParseZoneStaticLightings() < 0) return -1;
        if (m_scene->ReplaceProxyRooms(w.m_zones[i]) < 0) return -1;
        if (m_scene->ReplaceStaticProxyActors(w.m_zones[i]) < 0) return -1;
        if (m_file->ReadEnd() < 0) return -1;

        m_zone = 0;
    }
    return 0;
}

// The skinned LOD table -- the same code as ParseLODSpriteLevels under a
// different tag.
int VIESFParse::ParseSkinLODSpriteLevels(VILODSprite &l)
{
    unsigned short type, version;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2A70) return -1;

    m_file->Read(l.m_numLevels);

    for (int i = 0; i < l.m_numLevels; i++)
    {
        unsigned int id;
        VIResourceType t;

        m_file->Read(id);
        if (m_dict->Find(id, t, l.m_levels[i].m_sprite) == 0
            || m_scene->ShareSprite(l.m_levels[i].m_sprite) < 0)
            return -1;
        m_file->Read(l.m_levels[i].m_dist);
    }

    if (l.m_numLevels)
        l.m_offDist = l.m_levels[l.m_numLevels - 1].m_dist;

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The attachment table.  Kind 0 is a skin (resource type 4) and kind 1 is a
// sprite (type 7); node -1 means AttachSkin, anything else Attach.  A kind the
// engine does not know is a hard failure, which is the one place in the parser
// where a FIELD value can fail a parse.
int VIESFParse::ParseHSpriteAttachments(VIHSprite &h)
{
    unsigned short type, version;
    int n;

    if (m_file->ReadBegin(type, version) < 0 || type != 0x2500) return -1;

    m_file->Read(n);

    for (int i = 0; i < n; i++)
    {
        int kind, node;
        unsigned int id;
        VIResourceType t;

        m_file->Read(kind);
        m_file->Read(id);
        m_file->Read(node);

        switch (kind)
        {
        case 0: t = (VIResourceType)4; break;
        case 1: t = (VIResourceType)7; break;
        default: return -1;
        }

        if (id)
        {
            int r;
            int a;
            if (m_dict->FindTyped(id, t, r) == 0
                || (a = (node == -1 ? h.AttachSkin(r, *m_scene)
                                    : h.Attach(r, node, *m_scene))) < 0)
                return -1;
        }
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// One placed actor: a DictID, a position, a rotation, a scale and a baked
// colour.  The scene makes it a STATIC PROXY -- the art is not resident yet.
int VIESFParse::ParseZoneActor()
{
    VIVect3 pos;
    VIVect3 rot;
    VIColor32 col;
    unsigned short type, version;
    unsigned int id;
    float scale;

    if (m_progress) m_progress(m_progressArg);

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x6000) return -1;

    m_file->Read(id);
    m_file->Read(pos.x);
    m_file->Read(pos.y);
    m_file->Read(pos.z);
    m_file->Read(rot.x);
    m_file->Read(rot.y);
    m_file->Read(rot.z);
    m_file->Read(scale);
    m_file->Read(col.r);
    m_file->Read(col.g);
    m_file->Read(col.b);
    m_file->Read(col.a);

    int i = m_scene->CreateStaticProxyActor(pos, rot, scale, id, col);
    if (i < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A ref map is a VIMap<int,int> and nothing else: the indirection table an
// animation or a material uses to name something by id rather than by index.
int VIESFParse::ParseRefMap()
{
    unsigned short type, version;
    unsigned int id;
    int n;
    VIResourceType rt;
    int i;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x5000) return -1;

    m_file->Read(id);
    m_file->Read(n);

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 8) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateRefMap(m_permResource);
    if (i < 0) return -1;

    VIRefMap *p = m_scene->RefMap(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)8, i) < 0) return -1;

    if (p->m_map.Init(n) < 0) return -1;

    for (int j = 0; j < n; j++)
    {
        int k, v;
        m_file->Read(k);
        m_file->Read(v);
        // Insert takes const int &, and it is handed COPIES, made value first:
        // two more stack words and the loads in that order.
        int b = v, a = k;
        if (p->m_map.Insert(a, b) < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A colour buffer is baked per-vertex lighting for one prim buffer, so the
// prim buffer has to be resident already -- there is no `if (id)` here, the
// lookup MUST succeed.
int VIESFParse::ParseColorBuffer()
{
    VIColor32 c;
    unsigned short type, version;
    unsigned int id;
    int n;
    int pb;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1220) return -1;

    m_file->Read(id);
    m_file->Read(n);

    if (m_dict->FindTyped(id, (VIResourceType)11, pb) == 0) return -1;

    int i = m_raster->CreateColorBuffer(m_permResource);
    if (i < 0) return -1;

    VIColorBuffer *p = m_raster->ColorBuffer(i);

    if (p->Init(pb, *m_raster) < 0) return -1;
    if (p->Lock(*m_raster) < 0) return -1;

    for (int j = 0; j < n; j++)
    {
        m_file->Read(c.r);
        m_file->Read(c.g);
        m_file->Read(c.b);
        m_file->Read(c.a);
        p->Color(c);
    }

    if (p->Unlock() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The world BSP.  Six reads per node -- a plane and two tagged child indices --
// then one word per leaf.  The child indices come off the wire UNSIGNED
// because the high bit is the leaf tag.
int VIESFParse::ParseWorldTree(VIWorldTree &t)
{
    unsigned short type, version;
    int numNodes, numLeaves;
    int i;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x8230) return -1;

    m_file->Read(numNodes);
    m_file->Read(numLeaves);
    if (t.Init(numNodes, numLeaves) < 0) return -1;

    for (i = 0; i < numNodes; i++)
    {
        m_file->Read(t.m_nodes[i].m_plane.n.x);
        m_file->Read(t.m_nodes[i].m_plane.n.y);
        m_file->Read(t.m_nodes[i].m_plane.n.z);
        m_file->Read(t.m_nodes[i].m_plane.d);
        m_file->Read(*(unsigned int *)&t.m_nodes[i].m_front);
        m_file->Read(*(unsigned int *)&t.m_nodes[i].m_back);
    }

    for (i = 0; i < numLeaves; i++)
        m_file->Read(t.m_leaves[i].m_a);

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The zone BSP: the same node record, and a two-word leaf.
int VIESFParse::ParseZoneTree(VIZoneTree &t)
{
    unsigned short type, version;
    int numNodes, numLeaves;

    if (m_progress) m_progress(m_progressArg);

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x3220) return -1;

    m_file->Read(numNodes);
    m_file->Read(numLeaves);
    if (t.Init(numNodes, numLeaves) < 0) return -1;

    for (int i = 0; i < numNodes; i++)
    {
        m_file->Read(t.m_nodes[i].m_plane.n.x);
        m_file->Read(t.m_nodes[i].m_plane.n.y);
        m_file->Read(t.m_nodes[i].m_plane.n.z);
        m_file->Read(t.m_nodes[i].m_plane.d);
        m_file->Read(*(unsigned int *)&t.m_nodes[i].m_front);
        m_file->Read(*(unsigned int *)&t.m_nodes[i].m_back);
    }

    for (int j = 0; j < numLeaves; j++)
    {
        m_file->Read(t.m_leaves[j].m_a);
        m_file->Read(*(unsigned int *)&t.m_leaves[j].m_b);
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The world header.  Version 0 files carry neither a zone count nor a
// subdivision -- both default, which is what the two `li` constants sitting on
// the FALL-THROUGH of a version test are.
int VIESFParse::ParseWorldBaseHeader(VIWorld &w)
{
    VIBBox bbox;
    VIVect3 origin;
    unsigned short type, version;
    int numZones, numRows, numCols, subdiv;
    float cellSize;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x8220) return -1;

    if (version != 0)
        m_file->Read(numZones);
    else
        numZones = 1;

    m_file->Read(bbox.min.x);
    m_file->Read(bbox.min.y);
    m_file->Read(bbox.min.z);
    m_file->Read(bbox.max.x);
    m_file->Read(bbox.max.y);
    m_file->Read(bbox.max.z);
    m_file->Read(numRows);
    m_file->Read(numCols);

    if (version != 0)
        m_file->Read(subdiv);
    else
        subdiv = 4;

    m_file->Read(cellSize);
    m_file->Read(origin.x);
    m_file->Read(origin.y);
    m_file->Read(origin.z);

    if (w.Init(numZones, numRows, numCols, subdiv, cellSize, origin, bbox) < 0)
        return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The zone proxy table: everything the streamer needs to decide whether a zone
// is worth loading, and nothing that costs memory -- two file offsets, a size,
// a centre and a bounding box.  A version 1 file also carries the zone's NAME,
// which is read into a stack buffer and thrown away.
int VIESFParse::ParseWorldZoneProxies(VIWorld &w)
{
    unsigned short type, version;
    int n;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x8210) return -1;

    m_file->Read(n);

    for (int i = 0; i < n; i++)
    {
        if (w.m_zones[i].Init() < 0) return -1;

        m_file->Read(w.m_zones[i].m_baseOffset);
        m_file->Read(w.m_zones[i].m_offset);
        m_file->Read(w.m_zones[i].m_size);
        m_file->Read(w.m_zones[i].m_p20.x);
        m_file->Read(w.m_zones[i].m_p20.y);
        m_file->Read(w.m_zones[i].m_p20.z);
        m_file->Read(w.m_zones[i].m_bbox.min.x);
        m_file->Read(w.m_zones[i].m_bbox.min.y);
        m_file->Read(w.m_zones[i].m_bbox.min.z);
        m_file->Read(w.m_zones[i].m_bbox.max.x);
        m_file->Read(w.m_zones[i].m_bbox.max.y);
        m_file->Read(w.m_zones[i].m_bbox.max.z);

        if (version == 1)
        {
            char name[32];
            m_file->ReadString(name, 31);
        }
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// One placed actor whose art is already resident: the DictID is resolved here
// and the scene gets a real actor, not a proxy.  The colour it reads from a
// version 2 file goes nowhere -- CreateActor does not take one.
int VIESFParse::ParseActorObj()
{
    VIVect3 pos;
    VIVect3 rot;
    VIColor32 col;
    unsigned int id;
    unsigned int flags;
    float scale;
    VIResourceType t;
    int r;

    m_file->Read(id);

    if (m_file->ObjectVersion() != 0)
        m_file->Read(flags);
    else
        flags = 0;

    m_file->Read(pos.x);
    m_file->Read(pos.y);
    m_file->Read(pos.z);
    m_file->Read(rot.x);
    m_file->Read(rot.y);
    m_file->Read(rot.z);
    m_file->Read(scale);

    if (m_file->ObjectVersion() >= 2)
    {
        m_file->Read(col.r);
        m_file->Read(col.g);
        m_file->Read(col.b);
        m_file->Read(col.a);
    }

    if (m_dict->Find(id, t, r) == 0) return -1;

    int i = m_scene->CreateActor(pos, rot, scale, r);
    if (i < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// ---- phase 59: the sprite leaves --------------------------------------
//
// Every `*SpriteObj` is the same six steps: read the id and the bounds, ask the
// dictionary whether that id is already live (and if so close the object and
// hand its index straight back), create the sprite, register it, initialise it
// and walk whatever sub-objects the leaf owns.  Only the middle two lines --
// the VISpriteType and the VIResourceType -- differ between them.

// A point light.  It reads a RADIUS, not a box, so the bounds are derived: the
// sphere is centred on the origin and the box is the sphere's, which is
// VIPointLight::Init's body written out again with a zeroing of the centre in
// front of it.
int VIESFParse::ParsePointLightObj()
{
    VIColor32F   c;
    unsigned int id;
    float        radius;
    VIResourceType rt;
    int          i;

    m_file->Read(id);
    m_file->Read(radius);
    m_file->Read(c.r);
    m_file->Read(c.g);
    m_file->Read(c.b);
    m_file->Read(c.a);

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 16) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypePointLight, m_permResource);
    if (i < 0) return -1;

    VIPointLight *p = (VIPointLight *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)16, i) < 0) return -1;

    VISphere *b = &p->m_bsphere;

    p->m_bsphere.r = radius;
    p->m_bsphere.c.Zero();
    p->m_bbox.min = b->c;
    p->m_bbox.min -= b->r;
    p->m_bbox.max = b->c;
    p->m_bbox.max += b->r;
    p->m_color = c;

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A sound emitter.  The radius and the volume are a version 1 addition; a
// version 0 file gets 1.0 for both.  The sound itself is an inner object, and
// when the scene has no sound device it is opened and closed unread.
int VIESFParse::ParseSoundSpriteObj()
{
    unsigned short type, version;
    int            id;
    float          radius;
    float          volume;
    VIResourceType rt;
    int            i;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read(id);

    if (version)
    {
        m_file->Read(radius);
        m_file->Read(volume);
    }
    else
    {
        radius = 1.0f;
        volume = 1.0f;
    }

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 17) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeSound, m_permResource);
    if (i < 0) return -1;

    VISoundSprite *p = (VISoundSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)17, i) < 0) return -1;

    VISphere *b = &p->m_bsphere;

    p->m_bsphere.r = radius;
    p->m_bsphere.c.Zero();
    p->m_bbox.min = b->c;
    p->m_bbox.min -= b->r;
    p->m_bbox.max = b->c;
    p->m_bbox.max += b->r;
    p->m_volume = volume;

    if (m_sound)
    {
        p->m_sound = ParseSound();
        if (p->m_sound < 0) return -1;
        m_sound->ShareSound(p->m_sound);
    }
    else
    {
        if (m_file->ReadBegin(type, version) < 0) return -1;
        if (m_file->ReadEnd() < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A streaming-audio volume: a box, a relative pathname and two flags.
int VIESFParse::ParseStreamAudioSpriteObj()
{
    VIBBox         b;
    unsigned int   id;
    VIResourceType rt;
    int            i;

    m_file->Read(id);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 24) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeStreamAudio, m_permResource);
    if (i < 0) return -1;

    VIStreamAudioSprite *p = (VIStreamAudioSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)24, i) < 0) return -1;

    p->Init(b);

    m_file->ReadString(p->m_pathname, 127);
    m_file->Read((int &)p->m_looping);

    int pri;
    m_file->Read(pri);
    p->m_priority = pri;

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A group sprite: a header object carrying the id and the bounds, then the
// members' own sprite array and the member list.
int VIESFParse::ParseGroupSpriteObj()
{
    VIBBox         b;
    unsigned short type, version;
    unsigned int   id;
    VIResourceType rt;
    int            i;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2C10) return -1;

    m_file->Read(id);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 20) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeGroup, m_permResource);
    if (i < 0) return -1;

    VIGroupSprite *p = (VIGroupSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)20, i) < 0) return -1;

    p->m_bbox = b;
    p->m_bsphere.Init(b);

    if (ParseGroupSpriteSpriteArray() < 0) return -1;
    if (ParseGroupSpriteMembers(*p) < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The skinned LOD sprite -- the same object as the group one, level list for
// member list.
int VIESFParse::ParseSkinLODSpriteObj()
{
    VIBBox         b;
    unsigned short type, version;
    unsigned int   id;
    VIResourceType rt;
    int            i;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2A50) return -1;

    m_file->Read(id);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 15) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeLOD, m_permResource);
    if (i < 0) return -1;

    VILODSprite *p = (VILODSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)15, i) < 0) return -1;

    p->m_bbox = b;
    p->m_bsphere.Init(b);

    if (ParseSkinLODSpriteSpriteArray() < 0) return -1;
    if (ParseSkinLODSpriteLevels(*p) < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The LOD sprite.  A version 0 file carries a material palette of its own here;
// with no raster the palette object is opened and closed unread.
int VIESFParse::ParseLODSpriteObj()
{
    VIBBox         b;
    unsigned short type, version;
    unsigned int   id;
    VIResourceType rt;
    int            i;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2A10) return -1;

    m_file->Read(id);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 15) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeLOD, m_permResource);
    if (i < 0) return -1;

    VILODSprite *p = (VILODSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)15, i) < 0) return -1;

    p->m_bbox = b;
    p->m_bsphere.Init(b);

    if (m_file->ObjectVersion() == 0)
    {
        if (m_raster)
        {
            if (ParseMaterialPal() < 0) return -1;
        }
        else
        {
            m_file->ReadBegin(type, version);
            m_file->ReadEnd();
        }
    }

    if (ParseLODSpriteSpriteArray() < 0) return -1;
    if (ParseLODSpriteLevels(*p) < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The radial-flora sprite: a material palette and a prim buffer of its own.
// With no raster both objects are opened and closed unread and the two indices
// are left at -1.
int VIESFParse::ParseFloraSpriteObj()
{
    VIBBox         b;
    unsigned short type, version;
    unsigned int   id;
    VIResourceType rt;
    int            i;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read(id);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 25) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeFlora, m_permResource);
    if (i < 0) return -1;

    VIFloraSprite *p = (VIFloraSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)25, i) < 0) return -1;

    p->m_bbox = b;
    p->m_bsphere.Init(b);

    if (m_raster)
    {
        p->m_materialPal = ParseMaterialPal();
        if (p->m_materialPal < 0) return -1;
        if (m_raster->ShareMaterialPal(p->m_materialPal) < 0) return -1;

        p->m_primBuffer = ParseFloraPrimBuffer();
        if (p->m_primBuffer < 0) return -1;
        if (m_raster->SharePrimBuffer(p->m_primBuffer) < 0) return -1;
    }
    else
    {
        m_file->ReadBegin(type, version);
        m_file->ReadEnd();
        p->m_materialPal = -1;
        m_file->ReadBegin(type, version);
        p->m_primBuffer = -1;
        m_file->ReadEnd();
    }

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A skinned sub-sprite.  The material palette is NOT parsed here -- it arrives
// as a DictID and is looked up typed -- but the prim buffer is.
int VIESFParse::ParseSkinSubSpriteObj()
{
    VIBBox         b;
    unsigned short type, version;
    unsigned int   id;
    unsigned int   pal;
    VIResourceType rt;
    int            i;
    int            j;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read(id);
    m_file->Read(pal);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 7) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeSkin, m_permResource);
    if (i < 0) return -1;

    VISkinSprite *p = (VISkinSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)7, i) < 0) return -1;

    p->m_bbox = b;
    p->m_bsphere.Init(b);

    if (m_raster)
    {
        if (pal)
        {
            if (m_dict->FindTyped(pal, (VIResourceType)3, j) == 0) return -1;
            p->m_materialPal = j;
            if (m_raster->ShareMaterialPal(p->m_materialPal) < 0) return -1;
        }

        p->m_primBuffer = ParseSkinPrimBuffer();
        if (p->m_primBuffer < 0) return -1;
        if (m_raster->SharePrimBuffer(p->m_primBuffer) < 0) return -1;
    }
    else
    {
        m_file->ReadBegin(type, version);
        m_file->ReadEnd();
        p->m_primBuffer = -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The unskinned sub-sprite: the same, plus a collision buffer in a version 1
// file.
int VIESFParse::ParseSimpleSubSpriteObj()
{
    VIBBox         b;
    unsigned short type, version;
    unsigned int   id;
    unsigned int   pal;
    VIResourceType rt;
    int            i;
    int            j;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read(id);
    m_file->Read(pal);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 4) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeSimple, m_permResource);
    if (i < 0) return -1;

    VISimpleSprite *p = (VISimpleSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)4, i) < 0) return -1;

    p->m_bbox = b;
    p->m_bsphere.Init(b);

    if (m_raster)
    {
        if (pal)
        {
            if (m_dict->FindTyped(pal, (VIResourceType)3, j) == 0) return -1;
            p->m_materialPal = j;
            if (m_raster->ShareMaterialPal(p->m_materialPal) < 0) return -1;
        }

        p->m_primBuffer = ParsePrimBuffer();
        if (p->m_primBuffer < 0) return -1;
        if (m_raster->SharePrimBuffer(p->m_primBuffer) < 0) return -1;
    }
    else
    {
        m_file->ReadBegin(type, version);
        m_file->ReadEnd();
        p->m_primBuffer = -1;
    }

    if (m_file->ObjectVersion())
    {
        p->m_objIdx = ParseCollBuffer();
        if (p->m_objIdx < 0) return -1;
        if (m_collide->ShareCollBuffer(p->m_objIdx) < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The plain sprite: a surface array (dropped from version 2 on, where the
// surfaces travel inside the material palette), a palette, a prim buffer and,
// from version 1, a collision buffer.
int VIESFParse::ParseSimpleSpriteObj()
{
    VIBBox         b;
    unsigned short type, version;
    unsigned int   id;
    float          offDist;
    VIResourceType rt;
    int            i;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read(id);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (version)
        m_file->Read(offDist);
    else
        offDist = 500.0f;

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 4) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeSimple, m_permResource);
    if (i < 0) return -1;

    VISimpleSprite *p = (VISimpleSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)4, i) < 0) return -1;

    p->m_bbox = b;
    p->m_bsphere.Init(b);
    p->m_offDist = offDist;

    if (m_raster)
    {
        if (m_file->ObjectVersion() < 2)
        {
            if (ParseSurfaceArray() < 0) return -1;
        }

        p->m_materialPal = ParseMaterialPal();
        if (p->m_materialPal < 0) return -1;
        if (m_raster->ShareMaterialPal(p->m_materialPal) < 0) return -1;

        p->m_primBuffer = ParsePrimBuffer();
        if (p->m_primBuffer < 0) return -1;
        if (m_raster->SharePrimBuffer(p->m_primBuffer) < 0) return -1;
    }
    else
    {
        if (m_file->ObjectVersion() < 2)
        {
            m_file->ReadBegin(type, version);
            m_file->ReadEnd();
        }

        m_file->ReadBegin(type, version);
        m_file->ReadEnd();
        p->m_materialPal = -1;
        m_file->ReadBegin(type, version);
        p->m_primBuffer = -1;
        m_file->ReadEnd();
    }

    if (m_file->ObjectVersion())
    {
        p->m_objIdx = ParseCollBuffer();
        if (p->m_objIdx < 0) return -1;
        if (m_collide->ShareCollBuffer(p->m_objIdx) < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A particle emitter placed in the world.  The bounds it reads are its own
// only until the emitter exists: once created, the sprite takes the emitter's
// static bounds instead.
int VIESFParse::ParseParticleSpriteObj()
{
    unsigned short type, version;
    int            id;
    int            flags;
    VIResourceType rt;
    int            i;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read(id);

    if (version)
        m_file->Read(flags);
    else
        flags = 0;

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 19) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeParticle, m_permResource);
    if (i < 0) return -1;

    VIParticleSprite *p = (VIParticleSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)19, i) < 0) return -1;

    VISphere *sph = &p->m_bsphere;

    p->m_bsphere.r = 1.0f;
    p->m_bsphere.c.Zero();
    p->m_bbox.min = sph->c;
    p->m_bbox.min -= sph->r;
    p->m_bbox.max = sph->c;
    p->m_bbox.max += sph->r;
    p->m_unk430 = flags;

    if (m_scene->ParticleSystem() && m_raster)
    {
        p->m_defIndex = ParseParticleDefinition();
        if (p->m_defIndex < 0) return -1;
        m_scene->ParticleSystem()->ShareParticleDefinition(p->m_defIndex);
        p->m_emitter.Create(m_scene->ParticleSystem(),
                            m_scene->ParticleSystem()->ParticleDefinition(p->m_defIndex));
        p->m_bbox = p->m_emitter.StaticBounds();
        p->m_bsphere.Init(p->m_bbox);
    }
    else
    {
        if (m_file->ReadBegin(type, version) < 0) return -1;
        if (m_file->ReadEnd() < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The effect volume: a 0xC320 object holding whatever sprites the volume
// renders, then a 0xC330 object holding the volume's own parameters.  Both are
// optional and the second test is reached from either arm of the first, which
// is why one `li` serves both comparisons.
int VIESFParse::ParseEffectVolumeSpriteObj()
{
    VIBBox         b;
    unsigned short type, version;
    unsigned int   id;
    VIResourceType rt;
    int            i;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read(id);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 26) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeEffectVol, m_permResource);
    if (i < 0) return -1;

    VIEffectVolumeSprite *p = (VIEffectVolumeSprite *)m_scene->Sprite(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)26, i) < 0) return -1;

    p->Init(b);

    if (m_file->ReadBegin(type, version) < 0) return -1;

    if (type == 0xC320)
    {
        if (m_raster)
        {
            int n = m_file->NumSubObjects();
            for (int j = 0; j < n; j++)
                if (ParseObject() < 0) return -1;
        }

        if (m_file->ReadEnd() < 0) return -1;
        if (m_file->ReadBegin(type, version) < 0) return -1;
    }

    if (type == 0xC330)
    {
        int          t;
        float        p28, p30, p34, p38, density;
        int          surfId, sprId;
        unsigned int flags = 0;

        m_file->Read(t);
        m_file->Read(p34);
        m_file->Read(p38);
        m_file->Read(p30);
        m_file->Read(p28);
        m_file->Read(density);
        m_file->Read(surfId);
        m_file->Read(sprId);

        if (version >= 2)
            m_file->Read(flags);

        if (m_raster)
        {
            int surf = -1;
            int spr = -1;

            if (surfId != -1)
            {
                if (m_dict->Find(surfId, rt, surf))
                    if (rt != 1) return -1;
            }

            if (sprId != -1)
            {
                if (m_dict->Find(sprId, rt, spr))
                    if (rt != 9) return -1;
            }

            VIEffectVolume *v = &p->m_volume;

            v->SetType((VIEffectVolumeType)t, density);
            v->m_p28 = p28;
            v->m_p34 = p34;
            v->m_p38 = p38;
            v->m_p30 = p30;
            v->AttachSurface(surf);
            v->AttachSprite(spr);
            v->SetFlags(flags);
        }
    }

    if (m_file->ReadEnd() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The group's member list: a sprite DictID and a transform per member, written
// straight into the sprite's fixed five-entry array.
int VIESFParse::ParseGroupSpriteMembers(VIGroupSprite &g)
{
    unsigned short type, version;
    unsigned int   id;
    VIResourceType rt;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2C30) return -1;

    // OPEN (29/114).  The target steps TWO registers for m_sprite: Find's
    // out-param is a single-use DEST_REG giv, which loop.c's combine_givs
    // never uses as a combiner, and ShareSprite's load is a DEST_ADDR giv,
    // which never absorbs a DEST_REG one -- so they stay apart only if cse
    // has not merged the two addresses.  Through `m` (set outside the loop,
    // an opaque invariant to express_from) they do stay apart; still open:
    // where `m` is computed, and the 72/76 and 88/92 register swaps (the
    // preheader init order sets each giv's live length, hence its priority).
    VIGroupMember *m = g.m_members;
    m_file->Read(g.m_numMembers);

    for (int i = 0; i < g.m_numMembers; i++)
    {
        m_file->Read(id);

        if (m_dict->Find(id, rt, m[i].m_sprite) == 0
            || m_scene->ShareSprite(g.m_members[i].m_sprite) < 0)
            return -1;

        m_file->Read(g.m_members[i].m_loc.x);
        m_file->Read(g.m_members[i].m_loc.y);
        m_file->Read(g.m_members[i].m_loc.z);
        m_file->Read(g.m_members[i].m_scale);
        m_file->Read(g.m_members[i].m_hpr.x);
        m_file->Read(g.m_members[i].m_hpr.y);
        m_file->Read(g.m_members[i].m_hpr.z);
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// One room's baked lighting.  The colour buffers go into the lighting object's
// own array, one per PrimBuffer in the room; with no raster each is opened and
// closed unread.  `a` is the actor list the caller built, and the index read out
// of the file selects which actor the lighting attaches to.
int VIESFParse::ParseStaticLightingObj(VIArray<int> &a)
{
    unsigned short type, version;
    int            actor;
    int            n;

    if (m_progress) m_progress(m_progressArg);

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x6020) return -1;

    m_file->Read(actor);
    m_file->Read(n);

    if (m_file->ReadEnd() < 0) return -1;

    int i = m_scene->CreateStaticLighting(m_permResource);
    if (i < 0) return -1;

    VIStaticLighting *l = m_scene->StaticLighting(i);

    if (m_raster)
        if (l->Init(n) < 0) return -1;

    for (int j = 0; j < n; j++)
    {
        if (m_raster)
        {
            l->m_buffers[j] = ParseColorBuffer();
            if (l->m_buffers[j] < 0) return -1;
        }
        else
        {
            m_file->ReadBegin(type, version);
            m_file->ReadEnd();
        }
    }

    if (l->ShareResources(*m_raster) < 0) return -1;
    if (m_scene->SetActorLighting(a[actor], i) < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The material palette itself.  Version 0 files carry a surface array in front
// of the palette; from version 1 the surfaces travel inside each material.
int VIESFParse::ParseMaterialPalObj()
{
    unsigned short type, version;
    unsigned int   id;
    VIResourceType rt;
    int            i;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1111) return -1;

    m_file->Read(id);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 3) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    if (m_file->ObjectVersion())
        if (ParseSurfaceArray() < 0) return -1;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1101) return -1;

    int n = m_file->NumSubObjects();

    i = m_raster->CreateMaterialPal(n, m_permResource);
    if (i < 0) return -1;

    VIResource *p = (VIResource *)m_raster->MaterialPal(i);

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)3, i) < 0) return -1;

    for (int j = 0; j < n; j++)
    {
        m_file->ReadBegin(type, version);
        if (type != 0x1100) return -1;

        VIMaterial *m = m_raster->GetMaterialPalEntry(i, j);
        if (m == 0) return -1;
        if (ParseMaterial(m) < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// One room of a zone.  The bounding box is a version 2 addition -- an older
// file leaves the zeroed one, which is why the VIBBox local's constructor is
// load-bearing -- and the portal loop hands each portal's ring to the room one
// vertex at a time.
int VIESFParse::ParseZoneRoom(VIZoneRoom &r)
{
    VIBBox         bb;
    VIColor32F     col;
    VIVect3        v;
    unsigned short type, version;
    unsigned int   a, b;
    int            nPortals, nVerts;
    int            n, dest, extra;

    if (m_progress) m_progress(m_progressArg);

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x3240) return -1;

    m_file->Read(a);
    m_file->Read(b);
    m_file->Read(r.m_flags);

    if (version >= 2)
    {
        m_file->Read(bb.min.x);
        m_file->Read(bb.min.y);
        m_file->Read(bb.min.z);
        m_file->Read(bb.max.x);
        m_file->Read(bb.max.y);
        m_file->Read(bb.max.z);
    }

    m_file->Read(nPortals);
    m_file->Read(nVerts);

    if (r.Init(nPortals, nVerts, a, b, bb) < 0) return -1;

    for (int i = 0; i < nPortals; i++)
    {
        m_file->Read(n);
        m_file->Read(dest);

        if (version < 2)
            m_file->Read(extra);

        r.DestRoom(dest);
        r.Begin();

        for (int j = 0; j < n; j++)
        {
            m_file->Read(v.x);
            m_file->Read(v.y);
            m_file->Read(v.z);
            r.Vertex(v);
        }

        r.End();
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The radial flora's distribution and set tables.  Two sized arrays in two
// nested objects, each one sized from its own count and then filled field by
// field.
int VIESFParse::ParseZoneFloraSets(VIZoneRadialFlora &f)
{
    unsigned short type, version;
    unsigned short t, v;
    int n, m;
    int i;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x32F0) return -1;

    if (m_file->ReadBegin(t, v) < 0) return -1;
    if (t != 0x32F4) return -1;

    m_file->Read(n);
    if (f.m_dists.Init(n) < 0) return -1;

    for (i = 0; i < n; i++)
    {
        m_file->Read(f.m_dists[i].m_id);
        m_file->Read(f.m_dists[i].m_set);
        m_file->Read(f.m_dists[i].m_p10);
        m_file->Read(f.m_dists[i].m_p14);
        m_file->Read(f.m_dists[i].m_p18);
    }

    if (m_file->ReadEnd() < 0) return -1;

    if (m_file->ReadBegin(t, v) < 0) return -1;
    if (t != 0x32F8) return -1;

    m_file->Read(m);
    if (f.m_sets.Init(m) < 0) return -1;

    for (i = 0; i < m; i++)
    {
        m_file->Read(f.m_sets[i].m_model);
        m_file->Read(f.m_sets[i].m_dist);
    }

    if (m_file->ReadEnd() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The zone's own object dispatcher.  Everything a zone file can hold, and the
// two resource kinds that need a raster -- with none, the object is opened and
// closed and the zone loads without art.
int VIESFParse::ParseZoneResource()
{
    unsigned short type, version;
    int i = 0;

    if (m_progress) m_progress(m_progressArg);

    if (m_file->ReadBegin(type, version) < 0) return -1;

    switch (type)
    {
    case 0x2000: i = ParseSimpleSpriteObj(); if (i < 0) return -1; break;
    case 0x2200: i = ParseHSpriteObj(); if (i < 0) return -1; break;
    case 0x2700: i = ParseCSpriteObj(); if (i < 0) return -1; break;
    case 0x2A00: i = ParseLODSpriteObj(); if (i < 0) return -1; break;
    case 0x2B00: i = ParsePointLightObj(); if (i < 0) return -1; break;
    case 0xB100: i = ParseSoundSpriteObj(); if (i < 0) return -1; break;
    case 0xC100: i = ParseParticleSpriteObj(); if (i < 0) return -1; break;
    case 0x2310: i = ParseSimpleSubSpriteObj(); if (i < 0) return -1; break;
    case 0x2C00: i = ParseGroupSpriteObj(); if (i < 0) return -1; break;
    case 0x2D00: if (ParsePointSpriteObj() < 0) return -1; break;
    case 0x2E00: if (ParseStreamAudioSpriteObj() < 0) return -1; break;
    case 0x2F00: if (ParseFloraSpriteObj() < 0) return -1; break;
    case 0xC300: if (ParseEffectVolumeSpriteObj() < 0) return -1; break;
    case 0x6000: if (ParseActorObj() < 0) return -1; break;
    case 0x1001:
        if (m_raster)
        {
            i = ParseSurfaceArrayObj();
            if (i < 0) return -1;
        }
        else if (m_file->ReadEnd() < 0)
            return -1;
        break;
    case 0x1110:
        if (m_raster)
        {
            i = ParseMaterialPalObj();
            if (i < 0) return -1;
        }
        else if (m_file->ReadEnd() < 0)
            return -1;
        break;
    case 0x3200: if (ParseZoneBaseObj() < 0) return -1; break;
    case 0x3270: if (ParseZoneRoomActorsObj() < 0) return -1; break;
    case 0x3290: if (ParseZoneRoomStaticLightingsObj() < 0) return -1; break;
    default:
        if (m_file->ReadEnd() < 0) return -1;
        break;
    }
    return i;
}

// The five pixel converters.  Each takes the store the file names, the source
// bytes and a destination, and either passes the run through untouched when the
// two formats agree or expands it into 32-bit RGBA.  A destination store of 6
// is what the surface parser asks for when the raster cannot upload the file's
// own format.
int VIESFParse::Convert8(VIPixelStore &ps, unsigned char *src, unsigned char *dst, int n)
{
    switch (ps)
    {
    case VIPixelStoreI8:
        memcpy(dst, src, n);
        break;

    case VIPixelStore6:
        {
            VIColor32 c;
            unsigned char *s = src;
            unsigned char *d = dst;

            for (; n > 0; n--)
            {
                c = m_pal[*s++];
                d[3] = c.a;
                d[2] = c.r;
                d[1] = c.g;
                d[0] = c.b;
                d += 4;
            }
        }
        break;

    default:
        return -1;
    }

    return 0;
}

// Four bits per pixel.  Every case handles TWO pixels per source byte -- the
// low nibble first, then the high -- and the odd-length case is why the pass-
// through arm is two memcpys and not one: the byte count rounds UP.
int VIESFParse::Convert4(VIPixelStore &ps, unsigned char *src, unsigned char *dst, int n)
{
    switch (ps)
    {
    case VIPixelStoreI4:
        if (n & 1)
            memcpy(dst, src, (n >> 1) + 1);
        else
            memcpy(dst, src, n >> 1);
        break;

    case VIPixelStoreI8:
        while (n > 0)
        {
            *dst++ = *src & 0x0F;
            if (--n > 0)
            {
                *dst++ = *src >> 4;
                --n;
            }
            src++;
        }
        break;

    case VIPixelStore6:
        {
            VIColor32 c;
            unsigned char *s = src;
            unsigned char *d = dst;

            while (n > 0)
            {
                c = m_pal[*s & 0x0F];
                d[3] = c.a;
                d[2] = c.r;
                d[1] = c.g;
                d[0] = c.b;
                d += 4;

                if (--n > 0)
                {
                    c = m_pal[*s >> 4];
                    d[3] = c.a;
                    d[2] = c.r;
                    d[1] = c.g;
                    d[0] = c.b;
                    d += 4;
                    --n;
                }
                s++;
            }
        }
        break;

    default:
        return -1;
    }

    return 0;
}

// Sixteen bits per pixel, 5-5-5-1 little-endian.  The destination byte stores
// may alias the source halfword, so every component re-reads it -- four loads
// per pixel, which is what the target does and not an accident of spelling.
int VIESFParse::Convert16(VIPixelStore &ps, unsigned char *src, unsigned char *dst, int n)
{
    switch (ps)
    {
    case VIPixelStoreRGBA32:
        {
            unsigned short *s = (unsigned short *)src;

            for (; n > 0; n--)
            {
                dst[0] = (*s & 0x1F) << 3;
                dst[1] = ((*s >> 5) & 0x1F) << 3;
                dst[2] = ((*s >> 10) & 0x1F) << 3;
                dst[3] = (*s >> 15) << 7;
                s++;
                dst += 4;
            }
        }
        break;

    case VIPixelStore3:
        {
            unsigned short *s = (unsigned short *)src;

            for (; n > 0; n--)
            {
                dst[0] = (*s & 0x1F) << 3;
                dst[1] = ((*s >> 5) & 0x1F) << 3;
                dst[2] = ((*s >> 10) & 0x1F) << 3;
                dst[3] = 255;
                s++;
                dst += 4;
            }
        }
        break;

    case VIPixelStore6:
        {
            unsigned short *s = (unsigned short *)src;

            for (; n > 0; n--)
            {
                dst[3] = (*s >> 15) << 7;
                dst[2] = (*s & 0x1F) << 3;
                dst[1] = ((*s >> 5) & 0x1F) << 3;
                dst[0] = ((*s >> 10) & 0x1F) << 3;
                s++;
                dst += 4;
            }
        }
        break;

    case VIPixelStoreRGBA16:
        memcpy(dst, src, n * 2);
        break;

    default:
        return -1;
    }

    return 0;
}

// Twenty-four bits per pixel, packed.  Five contiguous cases, so this is the
// one converter gcc gives a JUMP TABLE -- and cases 2 and 3 have textually
// identical bodies that it does not cross-jump.
int VIESFParse::Convert24(VIPixelStore &ps, unsigned char *src, unsigned char *dst, int n)
{
    switch (ps)
    {
    case VIPixelStoreRGBA32:
        {
            unsigned char *d = dst;

            for (; n > 0; n--)
            {
                d[0] = src[0];
                d[1] = src[1];
                d[2] = src[2];
                d[3] = 255;
                src += 3;
                d += 4;
            }
        }
        break;

    case VIPixelStore3:
        {
            unsigned char *d = dst;

            for (; n > 0; n--)
            {
                d[0] = src[0];
                d[1] = src[1];
                d[2] = src[2];
                d[3] = 255;
                src += 3;
                d += 4;
            }
        }
        break;

    case VIPixelStore6:
        {
            unsigned char *d = dst;

            for (; n > 0; n--)
            {
                d[3] = 255;
                d[2] = src[0];
                d[1] = src[1];
                d[0] = src[2];
                src += 3;
                d += 4;
            }
        }
        break;

    case VIPixelStoreRGB24:
        memcpy(dst, src, n * 3);
        break;

    case VIPixelStoreRGBA16:
        {
            unsigned short *d = (unsigned short *)dst;

            for (; n > 0; n--)
            {
                unsigned int r = src[0] >> 3;

                *d = (*d & 0xFFE0) | r;
                *d = (*d & 0xFC1F) | ((src[1] >> 3) << 5);
                *d = (*d & 0x83FF) | ((src[2] >> 3) << 10) | 0x8000u;
                src += 3;
                d++;
            }
        }
        break;

    default:
        return -1;
    }

    return 0;
}

// Thirty-two bits per pixel.  The destination store for VIPixelStore3 is a
// four-byte memcpy with an alpha overwrite -- unaligned on both ends, so it
// expands to lwl/lwr and swl/swr rather than a call.
int VIESFParse::Convert32(VIPixelStore &ps, unsigned char *src, unsigned char *dst, int n)
{
    switch (ps)
    {
    case VIPixelStoreRGBA32:
        memcpy(dst, src, n * 4);
        break;

    case VIPixelStore6:
        {
            unsigned char *s = src;

            for (; n > 0; n--)
            {
                dst[3] = s[3];
                dst[2] = s[0];
                dst[1] = s[1];
                dst[0] = s[2];
                s += 4;
                dst += 4;
            }
        }
        break;

    case VIPixelStore3:
        {
            unsigned char *s = src;

            for (; n > 0; n--)
            {
                memcpy(dst, s, 4);
                dst[3] = 255;
                s += 4;
                dst += 4;
            }
        }
        break;

    case VIPixelStoreRGBA16:
        {
            unsigned char *s = src;
            unsigned short *d = (unsigned short *)dst;

            for (; n > 0; n--)
            {
                unsigned int r = s[0] >> 3;

                *d = (*d & 0xFFE0) | r;
                *d = (*d & 0xFC1F) | ((s[1] >> 3) << 5);
                *d = (*d & 0x83FF) | ((s[2] >> 3) << 10);
                *d = (*d & 0x7FFF) | ((s[3] >> 7) << 15);
                s += 4;
                d++;
            }
        }
        break;

    default:
        return -1;
    }

    return 0;
}

// The part emitters: a VICSprite's own particle emitters, one array entry per
// attachment.  The definition is only looked up when the scene has a particle
// system AND there is a raster to build the surface with; without either the
// slot keeps the constructor's -1 and the emitter never fires.
int VIESFParse::ParseCSpritePartEmitters(VICSprite &c)
{
    unsigned short type, version;
    int n;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2960) return -1;

    m_file->Read(n);

    if (c.m_emitters.Init(n) < 0) return -1;

    for (int i = 0; i < n; i++)
    {
        unsigned int id;
        int j;
        int t;

        m_file->Read(id);

        if (m_scene->ParticleSystem() && m_raster)
        {
            if (m_dict->FindTyped(id, VIRT_18, j) == 0) return -1;
            c.m_emitters[i].m_definition = j;
        }
        else
        {
            c.m_emitters[i].m_definition = -1;
        }

        m_file->Read(t);
        c.m_emitters[i].m_oneShot = t;

        m_file->Read(t);
        if (t & 1) c.m_emitters[i].m_flags |= 1;

        m_file->Read(c.m_emitters[i].m_node);

        m_file->Read(t);
        c.m_emitters[i].m_useLocation = t;

        m_file->Read(t);
        c.m_emitters[i].m_unk400 = t;

        m_file->Read(c.m_emitters[i].m_unk404);
        m_file->Read(c.m_emitters[i].m_life);
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// The play list: one VICSpriteAnimEntry per animation slot, and the whole of a
// character's animation table.  Every field has a default because the format
// grew -- the second animation arrived in version 1, the sounds and their
// volume in version 2, the particle and trigger in version 3 -- so an old file
// leaves the slot at 1.0 volume, no sound and no particle.
int VIESFParse::ParseCSpritePlayList(VICSprite &c)
{
    int anim[2];
    unsigned int animId[2];
    int snd[2];
    unsigned int sndId[2];
    float vol[2];
    float part[2];
    unsigned short type, version;
    int n;
    int t;
    float weight;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2910) return -1;

    m_file->Read(n);

    for (int i = 0; i < n; i++)
    {
        animId[1] = 0;
        animId[0] = 0;
        sndId[1] = 0;
        sndId[0] = 0;
        vol[0] = vol[1] = 1.0f;
        part[0] = part[1] = 0.0f;

        m_file->Read(animId[0]);
        if (version != 0) m_file->Read(animId[1]);

        m_file->Read(t);

        int slot = t;

        m_file->Read(weight);
        m_file->Read(t);

        int sync = t;

        if (version >= 2)
        {
            m_file->Read(sndId[0]);
            m_file->Read(vol[0]);
            if (version >= 3) m_file->Read(part[0]);
            m_file->Read(sndId[1]);
            m_file->Read(vol[1]);
            if (version >= 3) m_file->Read(part[1]);
        }

        anim[1] = -1;
        anim[0] = -1;

        if (animId[0])
            if (m_dict->FindTyped(animId[0], VIRT_Animation, anim[0]) == 0) return -1;
        if (animId[1])
            if (m_dict->FindTyped(animId[1], VIRT_Animation, anim[1]) == 0) return -1;

        snd[1] = -1;
        snd[0] = -1;

        if (m_sound)
        {
            if (sndId[0])
                if (m_dict->FindTyped(sndId[0], VIRT_Sound, snd[0]) == 0) return -1;
            if (sndId[1])
                if (m_dict->FindTyped(sndId[1], VIRT_Sound, snd[1]) == 0) return -1;
        }

        c.m_entries[slot].m_animation[0] = anim[0];
        c.m_entries[slot].m_animation[1] = anim[1];
        c.m_entries[slot].m_weight = weight;
        c.m_entries[slot].m_sync = sync;
        c.m_entries[slot].m_sound[0] = snd[0];
        c.m_entries[slot].m_sound[1] = snd[1];
        c.m_entries[slot].m_volume[0] = vol[0];
        c.m_entries[slot].m_volume[1] = vol[1];
        c.m_entries[slot].m_pitchVar[0] = part[0];
        c.m_entries[slot].m_pitchVar[1] = part[1];
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// A particle definition.  It needs a raster to build its surface, so a scene
// without one skips the whole object rather than failing the parse -- and the
// surface sub-object is skipped the same way, opened and closed on the spot.
int VIESFParse::ParseParticleDefinitionObj()
{
    unsigned short type, version;
    int j;
    VIResourceType rt;
    unsigned int id;

    if (m_scene->ParticleSystem() == 0) return -1;

    if (m_scene->m_raster == 0)
    {
        if (m_file->ReadEnd() < 0) return -1;
        return 0;
    }

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read((int &)id);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, j))
        {
            if (rt != 18) return -1;
            m_file->ReadEnd();
            return j;
        }
    }

    int i = m_scene->ParticleSystem()->CreateParticleDefinition(m_permResource);
    if (i < 0) return -1;

    VIParticleDefinition *p = m_scene->ParticleSystem()->ParticleDefinition(i);

    if (id)
        if (m_dict->Add(p->m_resource, id, (VIResourceType)18, i) < 0) return -1;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    if (type == 0x1000)
    {
        if (m_raster)
        {
            j = ParseSurfaceObj();
            if (j < 0) return -1;
        }
        else
        {
            m_file->ReadBegin(type, version);
            m_file->ReadEnd();
            j = -1;
        }

        if (m_file->ReadBegin(type, version) < 0) return -1;
    }

    if (ParseParticleDefinition(*m_file, *p) < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The particle definition's fields.  Entry 0 of the loop fills the definition
// itself and every entry after it a MOTIF, which is why the whole run of
// setters is written against a VIParticleAttributes pointer -- both classes
// derive from it, and the definition's own four setters are the only ones that
// go to the definition directly.
int VIESFParse::ParseParticleDefinition(VIObjFile &f, VIParticleDefinition &d)
{
    VIColor32F     c;
    VIVect3        v;
    char           name[32];
    unsigned int   id;
    int            t;
    int            n;
    VIResourceType rt;
    int            j;
    float          x;

    int version = f.ObjectVersion();

    f.Read(id);

    f.Read(t);
    d.SetBlendMode((VIParticleDefinitionEx::BLENDMODE)t);

    f.Read(t);
    d.SetZWrite(t != 0);

    f.Read(t);
    d.SetZTest(t != 0);

    f.Read(t);
    d.SetTextureConfiguration(t);

    f.Read(n);

    if (id)
    {
        if (m_dict)
        {
            if (m_dict->Find(id, rt, j))
            {
                if (rt != VIRT_Surface) return -1;
                d.SetTexture(j, *m_raster, *m_dict);
            }
        }
    }

    for (int i = 0; i < n + 1; i++)
    {
        VIParticleAttributes *a;

        if (i == 0)
        {
            a = &d;
        }
        else
        {
            VIParticleMotif *m = d.CreateMotif();

            f.Read((unsigned char *)name, 32);
            m->SetName(name);
            a = m;
        }

        f.Read(x); a->SetFriction(x);
        f.Read(x); a->SetBirthrate(x);
        f.Read(x); a->SetBirthrateVar(x);
        f.Read(x); a->SetLifespan(x);
        f.Read(x); a->SetLifespanVar(x);
        f.Read(x); a->SetVelocity(x);
        f.Read(x); a->SetVelocityVar(x);
        f.Read(x); a->SetStartSize(x);
        f.Read(x); a->SetStartSizeVar(x);
        f.Read(x); a->SetEndSize(x);
        f.Read(x); a->SetEndSizeVar(x);
        f.Read(x); a->SetInheritVelocity(x);
        f.Read(x); a->SetDeltaSpawn(x);

        f.Read((unsigned char *)&c, 16);
        a->SetStartColorVar(c);

        f.Read((unsigned char *)&c, 16);
        a->SetEndColorVar(c);

        for (int k = 0; k < 32; k++)
        {
            f.Read((unsigned char *)&c, 16);
            a->SetGradientColor(k, c);
        }

        f.Read(x); a->SetGradientRepeat(x);

        f.Read((unsigned char *)&v, 12); a->SetInnerOffset(v);
        f.Read((unsigned char *)&v, 12); a->SetInnerHprVar(v);
        f.Read((unsigned char *)&v, 12); a->SetOuterOffset(v);
        f.Read((unsigned char *)&v, 12); a->SetOuterHprVar(v);
        f.Read((unsigned char *)&v, 12); a->SetNozzleAxis(v);
        f.Read((unsigned char *)&v, 12); a->SetNozzleHprVar(v);

        if (version > 0)
        {
            f.Read(t);
            a->SetGravityOn(t);
        }
        else
        {
            a->SetGravityOn(0);
        }
    }

    f.ReadEnd();
    return 0;
}

// An XM module.  Two objects, back to back: 0xB040 is the module minus its
// twelve-byte header and 0xB060 the sample bank, and both buffers are handed
// straight to VIXm::Create, which takes them over.  The first word of the
// module has to be zero -- an XM whose first word is set is rejected and the
// buffer freed on the spot.
int VIESFParse::ParseXmObj()
{
    unsigned short type, version;
    unsigned int   id;
    float          vol;
    float          pan;
    VIResourceType rt;
    int            i;

    vol = 1.0f;
    pan = 0.0f;

    if (m_progress) m_progress(m_progressArg);

    if (m_file->ReadBegin(type, version) < 0) return -1;

    if (type != 0xB040)
    {
        m_file->ReadEnd();
        return -1;
    }

    m_file->Read(id);

    if (version)
    {
        m_file->Read(vol);
        m_file->Read(pan);
    }

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != VIRT_Sound) return -1;
            m_file->ReadEnd();
            m_file->ReadEnd();
            return i;
        }
    }

    int n1 = m_file->ObjectSize() - 12;
    unsigned char *b1 = new unsigned char[n1];

    m_file->Read(b1, n1);

    if (m_file->ReadEnd() < 0) return -1;

    if (*(int *)b1)
    {
        delete [] b1;
        return -1;
    }

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0xB060) return -1;

    int n2 = m_file->ObjectSize();
    unsigned char *b2 = new unsigned char[n2];

    m_file->Read(b2, n2);

    if (m_file->ReadEnd() < 0) return -1;

    i = m_sound->CreateSound(VISound::XM, m_permResource);

    VISound *snd = m_sound->Sound(i);

    snd->SetVolume(vol);
    snd->SetPan(pan);

    if (snd->AsXm()->Create(*m_sound, b1, n1, b2, n2) < 0)
    {
        i = -1;
        m_sound->ReleaseSound(i, *m_dict);
    }
    else if (id)
    {
        if (m_dict->Add(*snd, id, VIRT_Sound, i) < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// An ADPCM wave.  Format 0 is VAG ADPCM and has to be decoded into the wave's
// own locked buffer; format 1 is already PCM and is read straight into it.  A
// STEREO source decodes into a scratch buffer whose two halves are the two
// channels, and the interleave back into the wave is this loop.
int VIESFParse::ParseAdpcmObj()
{
    unsigned short type, version;
    unsigned int   id;
    int            channels;
    int            blocks;
    int            rate;
    float          vol;
    float          pan;
    int            loop;
    VIResourceType rt;
    int            i;
    unsigned char *p;

    if (m_progress) m_progress(m_progressArg);

    if (m_file->ReadBegin(type, version) < 0) return -1;

    if (type != 0xB010)
    {
        m_file->ReadEnd();
        return -1;
    }

    m_file->Read(id);
    m_file->Read(channels);
    m_file->Read(blocks);
    m_file->Read(rate);
    m_file->Read(vol);
    m_file->Read(pan);
    m_file->Read(loop);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != VIRT_Sound) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_sound->CreateSound(VISound::WAVE, m_permResource);
    if (i < 0) return -1;

    VISound *snd = m_sound->Sound(i);

    if (id)
        if (m_dict->Add(*snd, id, VIRT_Sound, i) < 0) return -1;

    snd->SetVolume(vol);
    snd->SetPan(pan);

    VIWave *w = snd->AsWave();

    w->SetSampleRate(rate);

    int last = blocks - 1;
    if (w->Create(*m_sound, channels, 16, (last - loop) * 28, loop) < 0) return -1;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    if (type != 0xB020)
    {
        m_file->ReadEnd();
        return -1;
    }

    int n = blocks * (channels << 4);

    switch (w->Format())
    {
    case 0:
        {
            unsigned char *b = new unsigned char[n];

            m_file->Read(b, n);

            if (w->LockBuffer(&p) >= 0)
            {
                if (channels == 1)
                {
                    AdpcmToPcm(b + 16, n - ((loop + 1) << 4), p);
                }
                else
                {
                    int cb = channels * 56;
                    int half = (blocks * cb) >> 1;
                    unsigned char *pcm = new unsigned char[half << 1];

                    AdpcmToPcm(b, n, pcm);

                    short *d = (short *)p;
                    short *l = (short *)&pcm[56];
                    short *r = (short *)&pcm[half + 56];

                    // OPEN.  The target's bound is `half - (loop + 1) * 56`: fold
                    // never distributes a MULT_EXPR, so split_tree finds no
                    // constant to reassociate, `half` stays live and the loop
                    // comes out as the target's `mult; addu R56; subu half`.
                    // That spelling scores worse (154) only because local-alloc
                    // then puts the 56 in s0 and `this` moves to s1.
                    for (int k = 0; k < half - (loop * 56 + 56); k += 2)
                    {
                        d[0] = *l++;
                        d[1] = *r++;
                        d += 2;
                    }

                    delete [] pcm;
                }

                w->UnlockBuffer();
            }

            delete [] b;
        }
        break;

    case 1:
        if (w->LockBuffer(&p) >= 0)
        {
            m_file->Read(p, n);
            w->UnlockBuffer();
        }
        break;
    }

    if (m_file->ReadEnd() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// The skeleton.  A node names its parent, or -1 for the root, so the file's
// node order IS the tree order and Add()/AddRoot() build it as they are read.
// The LOD distance table arrived in version 1 and the per-node fix scale in
// version 2 -- an older file gets a fix scale of one on all three axes.
int VIESFParse::ParseHSpriteHierarchy(VIHSprite &h)
{
    VIQuat  q;
    VIVect3 t;
    VIVect3 fix;
    unsigned short type, version;
    int numNodes;
    int parent;
    float scale;
    int i;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x2400) return -1;

    if (version)
    {
        m_file->Read(h.m_lod.m_num);

        for (i = 0; i < h.m_lod.m_num; i++)
            m_file->Read(h.m_lod.m_dist[i]);
    }

    m_file->Read(numNodes);

    VIVector<VIHSpriteNode> &nodes = h.m_nodes;

    if (nodes.Init(numNodes) < 0) return -1;
    if (h.m_blend.Init(numNodes) < 0) return -1;

    for (i = 0; i < numNodes; i++)
    {
        int j;

        m_file->Read(parent);

        if (parent != -1)
        {
            j = h.Add(parent);
            if (j < 0) return -1;
        }
        else
        {
            j = h.AddRoot();
            if (j < 0) return -1;
        }

        m_file->Read(q.x);
        m_file->Read(q.y);
        m_file->Read(q.z);
        m_file->Read(q.w);
        m_file->Read(scale);
        m_file->Read(t.x);
        m_file->Read(t.y);
        m_file->Read(t.z);

        if (h.SetNodeBindPose(j, q, scale, t) < 0) return -1;

        if (version)
            m_file->Read(nodes.m_data[j].m_lod);

        if (version >= 2)
        {
            m_file->Read(fix.x);
            m_file->Read(fix.y);
            m_file->Read(fix.z);
        }
        else
        {
            fix.z = 1.0f;
            fix.y = 1.0f;
            fix.x = 1.0f;
        }

        if (h.SetNodeFixScale(j, fix) < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// A hierarchical sprite: the skeleton, its geometry and its default animation.
// The bounding box is the only field in the header -- everything else is a
// sub-object -- and without a raster the two art objects are opened and closed
// on the spot so the file position still lands on the hierarchy.
int VIESFParse::ParseHSpriteObj()
{
    VIBBox         b;
    unsigned short type, version;
    unsigned int   id;
    VIResourceType rt;
    int            i;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read(id);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 5) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeH, m_permResource);
    if (i < 0) return -1;

    VIHSprite *p = (VIHSprite *)m_scene->Sprite(i);

    if (p->Init() < 0) return -1;

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)5, i) < 0) return -1;

    p->m_bbox = b;
    p->m_bsphere.Init(b);

    if (m_raster)
    {
        if (m_file->ObjectVersion() == 0)
            if (ParseSurfaceArray() < 0) return -1;

        if (ParseMaterialPal() < 0) return -1;
    }
    else
    {
        if (m_file->ObjectVersion() == 0)
        {
            m_file->ReadBegin(type, version);
            m_file->ReadEnd();
        }

        m_file->ReadBegin(type, version);
        m_file->ReadEnd();
    }

    if (ParseHSpriteSpriteArray() < 0) return -1;
    if (ParseHSpriteHierarchy(*p) < 0) return -1;

    p->m_refMap = ParseRefMap();
    if (p->m_refMap < 0) return -1;
    if (m_scene->ShareRefMap(p->m_refMap) < 0) return -1;

    if (m_file->ObjectVersion() >= 2)
        if (ParseHSpriteTriggers(*p) < 0) return -1;

    if (ParseHSpriteAttachments(*p) < 0) return -1;

    p->m_defaultAnim = ParseHSpriteAnim();
    if (p->m_defaultAnim < 0) return -1;
    if (m_scene->ShareAnimation(p->m_defaultAnim) < 0) return -1;
    if (p->AddPlay(p->m_defaultAnim, *m_scene) < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A texture.  Without a raster the object is skipped whole, because there is
// nowhere to put the pixels.  The file's depth code selects one of the five
// pixel converters, and the surface's own store decides what they convert TO --
// so a 4-bit file lands in a 4-bit surface or is expanded to RGBA, and the same
// code does both.  Each mip level is read a ROW at a time into a 2 KB staging
// buffer, and the level's dimensions halve with a floor of one.
int VIESFParse::ParseSurfaceObj()
{
    unsigned char  buf[2048];
    unsigned int   id;
    VIResourceType rt;
    int            i;
    int            width;
    int            height;
    int            depth;
    int            numMips;
    int            palCount;
    int            palN;
    VIPixelStore   ps;
    unsigned char *p;
    int            stride;
    int            n;

    if (m_raster == 0)
    {
        m_file->ReadEnd();
        return 0;
    }

    if (m_progress) m_progress(m_progressArg);

    m_file->Read(id);

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 1) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_raster->CreateSurface(m_permResource);
    if (i < 0) return -1;

    VISurface *s = m_raster->Surface(i);

    if (id)
        if (m_dict->Add((VIResource &)*s, id, (VIResourceType)1, i) < 0) return -1;

    m_file->Read(width);
    m_file->Read(height);
    m_file->Read(depth);
    m_file->Read(numMips);

    if (s->Init(*m_raster, width, height, (VIBitDepth)depth, numMips) < 0) return -1;

    if (depth >= 0 && depth < 2)
    {
        m_file->Read(palCount);

        if (m_file->Read((unsigned char *)m_pal, palCount * 4) < 0) return -1;

        if (s->m_paletteCount)
        {
            if (s->LockPalette(palN, ps, &p) < 0) return -1;

            if (Convert32(ps, (unsigned char *)m_pal, p,
                          palCount < palN ? palCount : palN) < 0) return -1;

            if (s->UnlockPalette() < 0) return -1;
        }
    }

    for (int level = 0; level < numMips && level < s->m_numMipLevels; level++)
    {
        if (s->LockMipLevel(level, stride, ps, &p) < 0) return -1;

        m_file->Read(n);

        for (int y = 0; y < height; y++)
        {
            if (m_file->Read(buf, n) < 0) return -1;

            switch (depth)
            {
            case 0: if (Convert4(ps, buf, p, width) < 0) return -1; break;
            case 1: if (Convert8(ps, buf, p, width) < 0) return -1; break;
            case 2: if (Convert16(ps, buf, p, width) < 0) return -1; break;
            case 3: if (Convert24(ps, buf, p, width) < 0) return -1; break;
            case 4: if (Convert32(ps, buf, p, width) < 0) return -1; break;
            default: return -1;
            }

            p += stride;
        }

        if (s->UnlockMipLevel(level) < 0) return -1;

        width = (width >> 1) > 0 ? (width >> 1) : 1;
        height = (height >> 1) > 0 ? (height >> 1) : 1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// One material: the layer count, then per layer the fill, the texture, the
// blend and the UV transform.  A version 0 file's LOD bias is READ and then
// thrown away for a fixed 0.8 -- the read still has to happen or the stream
// desynchronises, which is why this is two Read calls and not one.
int VIESFParse::ParseMaterial(VIMaterial *p)
{
    VIColor32    c;
    VIMatrix33   m;
    VIVect2      uv;
    int          numLayers;
    unsigned int flags;
    unsigned int texId;
    int          tex;
    int          wrap;
    int          blend;
    float        lodBias;

    if (m_progress) m_progress(m_progressArg);

    m_file->Read(numLayers);

    if (p->Init(numLayers) < 0) return -1;

    if (m_file->ObjectVersion() >= 2)
    {
        m_file->Read(flags);
        if (flags & 1) p->SetTessellate(1);
    }

    if (m_file->ObjectVersion() >= 3)
    {
        m_file->Read(c.r);
        m_file->Read(c.g);
        m_file->Read(c.b);
        m_file->Read(c.a);
        p->SetEmissiveColor(c);
    }

    for (int i = 0; i < numLayers; i++)
    {
        m_file->Read(flags);

        if (!(flags & 1))
            p->SetLayerFillType(i, (VIMaterialFillType)0);
        else
            p->SetLayerFillType(i, (VIMaterialFillType)1);

        if ((flags & 2) == 0)
            p->SetLayerUVSet(i, 0);
        else
            p->SetLayerUVSet(i, 1);

        m_file->Read(texId);

        if (texId)
        {
            if (m_dict->FindTyped(texId, (VIResourceType)1, tex) == 0) return -1;
            p->SetLayerTexture(i, tex, *m_raster, *m_dict);
        }

        m_file->Read(wrap);
        p->SetLayerWrapMode(i, (VIMaterialWrapMode)wrap);

        m_file->Read(blend);
        p->SetLayerBlendMode(i, (VIAlphaBlendMode)blend);

        m_file->Read(c.r);
        m_file->Read(c.g);
        m_file->Read(c.b);
        m_file->Read(c.a);
        p->SetLayerColor(i, c);

        VIVect3 &r0 = m.m[0];
        VIVect3 &r1 = m.m[1];
        VIVect3 &r2 = m.m[2];

        m_file->Read(r0.x);
        m_file->Read(r0.y);
        m_file->Read(r0.z);
        m_file->Read(r1.x);
        m_file->Read(r1.y);
        m_file->Read(r1.z);
        m_file->Read(r2.x);
        m_file->Read(r2.y);
        m_file->Read(r2.z);
        p->SetLayerUVTransform(i, m);

        if (m_file->ObjectVersion() == 0)
        {
            m_file->Read(lodBias);
            lodBias = 0.8f;
        }
        else
        {
            m_file->Read(lodBias);
        }

        p->SetLayerTextureLODBias(i, lodBias);

        m_file->Read(uv.x);
        m_file->Read(uv.y);
        p->SetLayerUVRate(i, uv);
    }

    if (m_file->ReadEnd() < 0) return -1;
    return 0;
}

// A font.  The per-character WIDTH table is read first and kept for the whole
// parse, because it is what every glyph's converter is given as its pixel
// count -- a glyph is `width` pixels by `height` rows, and the rows are read
// one at a time into the same staging buffer the surface parser uses.
int VIESFParse::ParseFontObj()
{
    unsigned char  buf[256];
    unsigned short type, version;
    unsigned short ch;
    unsigned int   id;
    int            numChars;
    int            height;
    int            depth;
    int            extra;
    VIResourceType rt;
    int            i;
    int            palCount;
    int            palN;
    VIPixelStore   ps;
    unsigned char *p;
    int            n;
    int            stride;

    extra = 1;
    p = 0;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read(id);
    m_file->Read(numChars);
    m_file->Read(height);
    m_file->Read(depth);

    int d = depth;

    if (version) m_file->Read(extra);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 12) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_raster->CreateFont(m_permResource);
    if (i < 0) return -1;

    VIRFont *f = m_raster->Font(i);

    unsigned char *b = new unsigned char[numChars];

    if (b == 0) return -1;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (m_file->Read(b, numChars) < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;

    if (f->Init(*m_raster, *m_dict, numChars, b, height, (VIBitDepth)d, extra) < 0) return -1;

    if (id)
        if (m_dict->Add((VIResource &)*f, id, (VIResourceType)12, i) < 0) return -1;

    if (f->Lock(*m_raster) < 0) return -1;
    if (m_file->ReadBegin(type, version) < 0) return -1;

    if (d >= 0 && d < 2)
    {
        m_file->Read(palCount);

        if (m_file->Read((unsigned char *)m_pal, palCount * 4) < 0) return -1;

        if (f->IsPalettized())
        {
            if (f->Palette(palN, ps, &p) < 0) return -1;

            if (Convert32(ps, (unsigned char *)m_pal, p,
                          palCount < palN ? palCount : palN) < 0) return -1;
        }
    }

    ps = f->PixelStore();

    for (int c = 0; c < numChars; c++)
    {
        if (m_progress) m_progress(m_progressArg);

        m_file->Read(ch);
        m_file->Read(n);

        if (f->BeginChar(ch, stride, &p) < 0) return -1;

        for (int y = 0; y < height; y++)
        {
            if (m_file->Read(buf, n) < 0) return -1;

            switch (d)
            {
            case 0: if (Convert4(ps, buf, p, b[c]) < 0) return -1; break;
            case 1: if (Convert8(ps, buf, p, b[c]) < 0) return -1; break;
            case 2: if (Convert16(ps, buf, p, b[c]) < 0) return -1; break;
            case 3: if (Convert24(ps, buf, p, b[c]) < 0) return -1; break;
            case 4: if (Convert32(ps, buf, p, b[c]) < 0) return -1; break;
            default: return -1;
            }

            p += stride;
        }

        if (f->EndChar() < 0) return -1;
    }

    if (m_file->ReadEnd() < 0) return -1;
    if (f->Unlock(*m_raster) < 0) return -1;

    delete [] b;
    return 0;
}

// ---------------------------------------------------------------------------
// Phase 61 -- the rest of the class.
// ---------------------------------------------------------------------------

// A spell effect: an optional 0xC230 block of the art it uses, then three event
// lists -- caster, target, area -- of 44-byte records.  A record of type 3 or 8
// names a particle definition and one of type 9 or 11 a sprite; both are
// resolved through the dictionary and shared, and without a raster they are
// cleared to -1 instead.  Version 0/1 files had no duration on a type-1 event,
// so it is given 5000.
//
// `j` is ONE variable throughout -- the Find out-parameter at the top, the new
// effect's index, and every Find in the event loop -- and the function returns
// it, so an effect with any resolved event returns that event's index, not its
// own.  Every caller only tests the sign.
// The particle case tests the raster and the id in NESTED ifs, each with its
// own `ev->m_id = -1` else: jump2 cross-jumps the three -1 stores into one
// AFTER register allocation, so global-alloc counts all three and the hoisted
// -1 outranks `l` for s5 (as one `&&` with one else it is two stores, and l,
// -1, 1 and ver rotate through s5..s8: 22 words).
int VIESFParse::ParseSpellEffectObj()
{
    unsigned short type, version;
    unsigned int id;
    VIResourceType rt;
    int j;
    int n;
    int k;

    int ver = m_file->ObjectVersion();

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read((int &)id);

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, j))
        {
            if (rt != 21) return -1;
            m_file->ReadEnd();
            return j;
        }
    }

    int read = 0;

    if (ver > 0)
    {
        if (m_file->ReadBegin(type, version) < 0) return -1;

        if (type == 0xC230)
        {
            if (m_scene->m_raster)
            {
                int c = m_file->NumSubObjects();
                for (k = 0; k < c; k++)
                    if (ParseObject() < 0) return -1;
            }

            if (m_file->ReadEnd() < 0) return -1;
        }
        else
            read = 1;
    }

    j = m_scene->m_spellEffects.CreateSpellEffect(m_permResource);
    if (j == -1) return -1;

    VISpellEffect *e = m_scene->m_spellEffects.SpellEffect(j);
    if (e == 0) return -1;

    if (e->Create(m_scene) < 0) return -1;

    if (id)
        if (m_dict->Add(*e, id, (VIResourceType)21, j) < 0) return -1;

    if (!read)
        if (m_file->ReadBegin(type, version) < 0) return -1;

    for (k = 0; k < 3; k++)
    {
        VISpellEventList *l;

        switch (k)
        {
        case 0: l = &e->m_casterEvents; break;
        case 1: l = &e->m_targetEvents; break;
        case 2: l = &e->m_areaEvents; break;
        }

        m_file->Read(n);
        l->Create(n, m_scene);

        for (int m = 0; m < n; m++)
        {
            VISpellEvent *ev = &l->m_events[m];

            m_file->Read((unsigned int &)ev->m_type);
            m_file->Read(ev->m_p04);
            m_file->Read(ev->m_id);
            m_file->Read(ev->m_p0C);
            m_file->Read(ev->m_p10);
            m_file->Read(ev->m_f14);
            m_file->Read(ev->m_f18);
            m_file->Read(ev->m_f1C);
            m_file->Read(ev->m_f20);
            m_file->Read(ev->m_f24);
            m_file->Read(ev->m_f28);

            if (ver < 2 && ev->m_type == 1)
                ev->m_p0C = 5000;

            switch (ev->m_type)
            {
            case 3:
            case 8:
                if (m_scene->m_raster)
                {
                    if (ev->m_id != -1 && ev->m_id != 0)
                    {
                        if (m_dict->Find(ev->m_id, rt, j) == 0) return -1;
                        ev->m_id = j;
                        m_scene->ParticleSystem()->ShareParticleDefinition(j);
                    }
                    else
                        ev->m_id = -1;
                }
                else
                    ev->m_id = -1;
                break;

            case 9:
            case 11:
                if (m_scene->m_raster)
                {
                    if (m_dict->Find(ev->m_id, rt, j) == 0) return -1;
                    ev->m_id = j;
                    m_scene->ShareSprite(j);
                }
                else
                    ev->m_id = -1;
                break;
            }
        }
    }

    if (m_file->ReadEnd() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return j;
}

// Version 0 of a prim buffer: no id, so never shared, and no packing -- every
// vertex is eight floats and four colour bytes.  The material changes between
// prims, so it is begun and ended around each run of prims that share one.
int VIESFParse::ParsePrimBufferObjV0()
{
    VIVect3 v;
    VIVect2 uv;
    VIVect3 n;
    VIColor32 c;
    int numMats, numPrims, numVerts;
    int nv, mat;
    int curMat = -1;

    m_file->Read(numMats);
    m_file->Read(numPrims);
    m_file->Read(numVerts);

    int i = m_raster->CreatePrimBuffer(m_permResource);
    if (i < 0) return -1;

    VIPrimBuffer *p = m_raster->PrimBuffer(i);

    if (p->Init(*m_raster, VIPrimBufferTypeVUNC, numMats, numPrims, numVerts) < 0) return -1;
    if (p->Lock() < 0) return -1;

    for (int j = 0; j < numPrims; j++)
    {
        m_file->Read(nv);
        m_file->Read(mat);

        if (curMat != mat)
        {
            if (curMat != -1) p->EndMaterial();
            p->BeginMaterial(mat);
            curMat = mat;
        }

        p->Begin(nv);

        for (int k = 0; k < nv; k++)
        {
            m_file->Read(v.x);
            m_file->Read(v.y);
            m_file->Read(v.z);
            m_file->Read(uv.x);
            m_file->Read(uv.y);
            m_file->Read(n.x);
            m_file->Read(n.y);
            m_file->Read(n.z);
            m_file->Read(c.r);
            m_file->Read(c.g);
            m_file->Read(c.b);
            m_file->Read(c.a);
            p->UV0(uv);
            p->Normal(n);
            p->Color(c);
            p->Vertex(v);
        }

        p->End();
    }

    if (numMats) p->EndMaterial();
    if (p->Unlock() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A prim buffer.  Version 0 is the old float-only record with a parser of its
// own; version 2 added the id that lets a buffer be shared.  On the wire the
// packing is 0 (floats), 2 (16-bit position and UV, 8-bit normal) or 4 (the
// same plus a vertex group), and a raster that cannot draw the packed form has
// demoted the buffer by the time Init returns -- 2 to 0, 4 to 6 -- in which
// case every vertex is scaled back up to floats here.
//
// Each arm's locals are its OWN block: they share stack slots arm to arm, and
// the frame only fits that way (sixteen 16-byte slots, not thirty).
int VIESFParse::ParsePrimBuffer()
{
    int curMat = -1;
    unsigned short type, version;
    unsigned int id = 0;
    int t;
    VIPrimBufferType packing;
    int numMats, numPrims, numVerts;
    int vbits, uvbits, nbits;
    VIResourceType rt;
    int i;
    int nv, mat;
    int k;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1200) return -1;

    if (version == 0)
    {
        i = ParsePrimBufferObjV0();
        if (i < 0) return -1;
        return i;
    }

    if (version >= 2) m_file->Read(id);
    m_file->Read(t);
    packing = (VIPrimBufferType)t;
    m_file->Read(numMats);
    m_file->Read(numPrims);
    m_file->Read(numVerts);
    m_file->Read(vbits);
    m_file->Read(uvbits);
    m_file->Read(nbits);

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 11) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_raster->CreatePrimBuffer(m_permResource);
    if (i < 0) return -1;

    VIPrimBuffer *p = m_raster->PrimBuffer(i);

    if (id)
        if (m_dict->Add((VIResource &)*p, id, (VIResourceType)11, i) < 0) return -1;

    if (p->Init(*m_raster, packing, numMats, numPrims, numVerts) < 0) return -1;
    if (p->Lock() < 0) return -1;

    p->SetPacking(vbits, uvbits, nbits);

    float vscale = 1.0f / powf(2.0f, vbits);
    float uvscale = 1.0f / powf(2.0f, uvbits);
    float nscale = 1.0f / 127.0f;

    for (int j = 0; j < numPrims; j++)
    {
        m_file->Read(nv);
        m_file->Read(mat);

        if (curMat != mat)
        {
            if (curMat != -1) p->EndMaterial();
            p->BeginMaterial(mat);
            curMat = mat;
        }

        switch (packing)
        {
        case 0:
        {
            VIVect3 v;
            VIVect2 uv;
            VIVect3 n;
            VIColor32 c;

            p->Begin(nv);

            for (k = 0; k < nv; k++)
            {
                m_file->Read(v.x);
                m_file->Read(v.y);
                m_file->Read(v.z);
                m_file->Read(uv.x);
                m_file->Read(uv.y);
                m_file->Read(n.x);
                m_file->Read(n.y);
                m_file->Read(n.z);
                m_file->Read(c.r);
                m_file->Read(c.g);
                m_file->Read(c.b);
                m_file->Read(c.a);
                p->UV0(uv);
                p->Normal(n);
                p->Color(c);
                p->Vertex(v);
            }
            p->End();
            break;
        }

        case 2:
            switch (p->m_type)
            {
            case 2:
            {
                VIVect3S16 sv;
                VIVect2S16 suv;
                VIVect3S8 sn;
                VIColor32 c;

                p->Begin(nv);

                for (k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(suv.x);
                    m_file->Read(suv.y);
                    m_file->Read(sn.x);
                    m_file->Read(sn.y);
                    m_file->Read(sn.z);
                    m_file->Read(c.r);
                    m_file->Read(c.g);
                    m_file->Read(c.b);
                    m_file->Read(c.a);
                    p->UV0(suv);
                    p->Normal(sn);
                    p->Color(c);
                    p->Vertex(sv);
                }
                p->End();
                break;
            }

            case 0:
            {
                VIVect3 v;
                VIVect2 uv;
                VIVect3 n;
                VIColor32 c;
                VIVect3S16 sv;
                VIVect2S16 suv;
                VIVect3S8 sn;
                VIColor32 sc;

                p->Begin(nv);

                for (k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(suv.x);
                    m_file->Read(suv.y);
                    m_file->Read(sn.x);
                    m_file->Read(sn.y);
                    m_file->Read(sn.z);
                    m_file->Read(sc.r);
                    m_file->Read(sc.g);
                    m_file->Read(sc.b);
                    m_file->Read(sc.a);
                    v.x = sv.x * vscale;
                    v.y = sv.y * vscale;
                    v.z = sv.z * vscale;
                    uv.x = suv.x * uvscale;
                    uv.y = suv.y * uvscale;
                    n.x = sn.x * nscale;
                    n.y = sn.y * nscale;
                    n.z = sn.z * nscale;
                    c = sc;
                    p->UV0(uv);
                    p->Normal(n);
                    p->Color(c);
                    p->Vertex(v);
                }
                p->End();
                break;
            }

            default:
                return -1;
            }
            break;

        case 4:
            switch (p->m_type)
            {
            case 6:
            {
                VIVect3 v;
                VIVect2 uv;
                VIVect3 n;
                VIColor32 c;
                VIVect3S16 sv;
                VIVect2S16 suv;
                VIVect3S8 sn;
                VIColor32 sc;
                short g;

                p->Begin(nv);

                for (k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(suv.x);
                    m_file->Read(suv.y);
                    m_file->Read(sn.x);
                    m_file->Read(sn.y);
                    m_file->Read(sn.z);
                    m_file->Read(sc.r);
                    m_file->Read(sc.g);
                    m_file->Read(sc.b);
                    m_file->Read(sc.a);
                    m_file->Read(g);
                    v.x = sv.x * vscale;
                    v.y = sv.y * vscale;
                    v.z = sv.z * vscale;
                    uv.x = suv.x * uvscale;
                    uv.y = suv.y * uvscale;
                    n.x = sn.x * nscale;
                    n.y = sn.y * nscale;
                    n.z = sn.z * nscale;
                    c = sc;
                    p->UV0(uv);
                    p->Normal(n);
                    p->Color(c);
                    p->VertexGroup(g);
                    p->Vertex(v);
                }
                p->End();
                break;
            }

            case 4:
            {
                VIVect3S16 sv;
                VIVect2S16 suv;
                VIVect3S8 sn;
                VIColor32 c;
                short g;

                p->Begin(nv);

                for (k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(suv.x);
                    m_file->Read(suv.y);
                    m_file->Read(sn.x);
                    m_file->Read(sn.y);
                    m_file->Read(sn.z);
                    m_file->Read(c.r);
                    m_file->Read(c.g);
                    m_file->Read(c.b);
                    m_file->Read(c.a);
                    m_file->Read(g);
                    p->UV0(suv);
                    p->Normal(sn);
                    p->Color(c);
                    p->VertexGroup(g);
                    p->Vertex(sv);
                }
                p->End();
                break;
            }

            default:
                return -1;
            }
            break;

        default:
            return -1;
        }
    }

    if (numMats) p->EndMaterial();

    // The pointer is assigned INSIDE the test: declared outside, the address
    // is formed before the null check; inside the arm, cse folds it away.
    VIArray<VIVect3> *pt;

    if (m_zone && (pt = &m_zone->m_pretrans)->m_size)
    {
        if (p->CalcBVolumes(pt->m_size, pt->Data()) < 0) return -1;
    }
    else
    {
        if (p->CalcBVolumes(0, 0) < 0) return -1;
    }

    if (p->Unlock() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// Version 0 of a skinned prim buffer: floats throughout, four bone indices and
// four float weights per vertex, and -- unlike the plain one -- the last
// material is ended unconditionally.
int VIESFParse::ParseSkinPrimBufferObjV0()
{
    VIVect3 v;
    VIVect2 uv;
    VIVect3 n;
    VIColor32 c;
    VIVect4U8 b;
    VIVect4 w;
    int numMats, numPrims, numVerts;
    int nv, mat;
    int i;
    int curMat = -1;

    m_file->Read(numMats);
    m_file->Read(numPrims);
    m_file->Read(numVerts);

    i = m_raster->CreatePrimBuffer(m_permResource);
    if (i < 0) return -1;

    VIPrimBuffer *p = m_raster->PrimBuffer(i);

    if (p->Init(*m_raster, VIPrimBufferTypeUnused1, numMats, numPrims, numVerts) < 0) return -1;
    if (p->Lock() < 0) return -1;

    for (int j = 0; j < numPrims; j++)
    {
        m_file->Read(nv);
        m_file->Read(mat);

        if (curMat != mat)
        {
            if (curMat != -1) p->EndMaterial();
            p->BeginMaterial(mat);
            curMat = mat;
        }

        p->Begin(nv);

        for (int k = 0; k < nv; k++)
        {
            m_file->Read(v.x);
            m_file->Read(v.y);
            m_file->Read(v.z);
            m_file->Read(uv.x);
            m_file->Read(uv.y);
            m_file->Read(n.x);
            m_file->Read(n.y);
            m_file->Read(n.z);
            m_file->Read(c.r);
            m_file->Read(c.g);
            m_file->Read(c.b);
            m_file->Read(c.a);
            m_file->Read(b.x);
            m_file->Read(b.y);
            m_file->Read(b.z);
            m_file->Read(b.w);
            m_file->Read(w.x);
            m_file->Read(w.y);
            m_file->Read(w.z);
            m_file->Read(w.w);
            p->UV0(uv);
            p->Normal(n);
            p->Color(c);
            p->Bones(b);
            p->Weights(w);
            p->Vertex(v);
        }

        p->End();
    }

    p->EndMaterial();
    if (p->Unlock() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A skinned prim buffer.  The packings are 1 (floats with colour), 3 (floats,
// no colour) and 5 (16-bit position and UV, 8-bit normal and weights, no
// colour); a raster that cannot draw 5 has demoted the buffer to 1, and then
// the packed weights are scaled by 1/127 like the normal and an opaque colour
// is supplied.  Unlike the plain prim buffer, Begin() runs before the switch,
// the progress callback fires as each material closes, and there is no
// bounding-volume pass.
int VIESFParse::ParseSkinPrimBuffer()
{
    int curMat = -1;
    unsigned short type, version;
    unsigned int id = 0;
    int t;
    VIPrimBufferType packing;
    int numMats, numPrims, numVerts;
    int vbits, uvbits, nbits;
    VIResourceType rt;
    int i;
    int nv, mat;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1210) return -1;

    if (version == 0)
    {
        i = ParseSkinPrimBufferObjV0();
        if (i < 0) return -1;
        return i;
    }

    if (version >= 2) m_file->Read(id);
    m_file->Read(t);
    packing = (VIPrimBufferType)t;
    m_file->Read(numMats);
    m_file->Read(numPrims);
    m_file->Read(numVerts);
    m_file->Read(vbits);
    m_file->Read(uvbits);
    m_file->Read(nbits);

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 11) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_raster->CreatePrimBuffer(m_permResource);
    if (i < 0) return -1;

    VIPrimBuffer *p = m_raster->PrimBuffer(i);

    if (id)
        if (m_dict->Add((VIResource &)*p, id, (VIResourceType)11, i) < 0) return -1;

    if (p->Init(*m_raster, packing, numMats, numPrims, numVerts) < 0) return -1;
    if (p->Lock() < 0) return -1;

    p->SetPacking(vbits, uvbits, nbits);

    float vscale = 1.0f / powf(2.0f, vbits);
    float uvscale = 1.0f / powf(2.0f, uvbits);
    float nscale = 1.0f / 127.0f;

    for (int j = 0; j < numPrims; j++)
    {
        m_file->Read(nv);
        m_file->Read(mat);

        if (curMat != mat)
        {
            if (curMat != -1)
            {
                if (m_progress) m_progress(m_progressArg);
                p->EndMaterial();
            }
            p->BeginMaterial(mat);
            curMat = mat;
        }

        p->Begin(nv);

        switch (packing)
        {
        case 3:
        {
            VIVect3 v;
            VIVect2 uv;
            VIVect3 n;
            VIVect4U8 b;
            VIVect4 w;

            for (int k = 0; k < nv; k++)
            {
                m_file->Read(v.x);
                m_file->Read(v.y);
                m_file->Read(v.z);
                m_file->Read(uv.x);
                m_file->Read(uv.y);
                m_file->Read(n.x);
                m_file->Read(n.y);
                m_file->Read(n.z);
                m_file->Read(b.x);
                m_file->Read(b.y);
                m_file->Read(b.z);
                m_file->Read(b.w);
                m_file->Read(w.x);
                m_file->Read(w.y);
                m_file->Read(w.z);
                m_file->Read(w.w);
                p->UV0(uv);
                p->Normal(n);
                p->Bones(b);
                p->Weights(w);
                p->Vertex(v);
            }
            break;
        }

        case 5:
            switch (p->m_type)
            {
            case 5:
            {
                VIVect3S16 sv;
                VIVect2S16 suv;
                VIVect3S8 sn;
                VIVect4U8 b;
                VIVect4S8 sw;

                for (int k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(suv.x);
                    m_file->Read(suv.y);
                    m_file->Read(sn.x);
                    m_file->Read(sn.y);
                    m_file->Read(sn.z);
                    m_file->Read(b.x);
                    m_file->Read(b.y);
                    m_file->Read(b.z);
                    m_file->Read(b.w);
                    m_file->Read(sw.x);
                    m_file->Read(sw.y);
                    m_file->Read(sw.z);
                    m_file->Read(sw.w);
                    p->UV0(suv);
                    p->Normal(sn);
                    p->Bones(b);
                    p->Weights(sw);
                    p->Vertex(sv);
                }
                break;
            }

            case 1:
            {
                VIVect3S16 sv;
                VIVect2S16 suv;
                VIVect3S8 sn;
                VIVect4U8 b;
                VIVect4S8 sw;
                VIVect3 v;
                VIVect2 uv;
                VIVect3 n;
                VIColor32 c(0, 0, 0, 255);
                VIVect4 w;

                for (int k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(suv.x);
                    m_file->Read(suv.y);
                    m_file->Read(sn.x);
                    m_file->Read(sn.y);
                    m_file->Read(sn.z);
                    m_file->Read(b.x);
                    m_file->Read(b.y);
                    m_file->Read(b.z);
                    m_file->Read(b.w);
                    m_file->Read(sw.x);
                    m_file->Read(sw.y);
                    m_file->Read(sw.z);
                    m_file->Read(sw.w);
                    v.x = sv.x * vscale;
                    v.y = sv.y * vscale;
                    v.z = sv.z * vscale;
                    uv.x = suv.x * uvscale;
                    uv.y = suv.y * uvscale;
                    n.x = sn.x * nscale;
                    n.y = sn.y * nscale;
                    n.z = sn.z * nscale;
                    w.x = sw.x * nscale;
                    w.y = sw.y * nscale;
                    w.z = sw.z * nscale;
                    w.w = sw.w * nscale;
                    p->UV0(uv);
                    p->Normal(n);
                    p->Color(c);
                    p->Bones(b);
                    p->Weights(w);
                    p->Vertex(v);
                }
                break;
            }

            default:
                return -1;
            }
            break;

        case 1:
        {
            VIVect3 v;
            VIVect2 uv;
            VIVect3 n;
            VIColor32 c;
            VIVect4U8 b;
            VIVect4 w;

            for (int k = 0; k < nv; k++)
            {
                m_file->Read(v.x);
                m_file->Read(v.y);
                m_file->Read(v.z);
                m_file->Read(uv.x);
                m_file->Read(uv.y);
                m_file->Read(n.x);
                m_file->Read(n.y);
                m_file->Read(n.z);
                m_file->Read(c.r);
                m_file->Read(c.g);
                m_file->Read(c.b);
                m_file->Read(c.a);
                m_file->Read(b.x);
                m_file->Read(b.y);
                m_file->Read(b.z);
                m_file->Read(b.w);
                m_file->Read(w.x);
                m_file->Read(w.y);
                m_file->Read(w.z);
                m_file->Read(w.w);
                p->UV0(uv);
                p->Normal(n);
                p->Color(c);
                p->Bones(b);
                p->Weights(w);
                p->Vertex(v);
            }
            break;
        }

        default:
            return -1;
        }

        p->End();
    }

    if (numMats)
    {
        if (m_progress) m_progress(m_progressArg);
        p->EndMaterial();
    }

    if (p->Unlock() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A flora prim buffer: position and UV only, packing 7 (floats) or 8 (16-bit),
// always with an id, and never a version 0.  A raster that cannot draw 8 has
// demoted it to 7.  The demoted arm declares a normal and a colour it never
// uses -- the slots it is given only fit with both of them in the block.
int VIESFParse::ParseFloraPrimBuffer()
{
    VIPrimBuffer *p;
    int curMat = -1;
    unsigned short type, version;
    unsigned int id = 0;
    int t;
    VIPrimBufferType packing;
    int numMats, numPrims, numVerts;
    int vbits, uvbits;
    VIResourceType rt;
    int i;
    int nv, mat;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x1230) return -1;

    m_file->Read(id);
    m_file->Read(t);
    packing = (VIPrimBufferType)t;
    m_file->Read(numMats);
    m_file->Read(numPrims);
    m_file->Read(numVerts);
    m_file->Read(vbits);
    m_file->Read(uvbits);

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 11) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_raster->CreatePrimBuffer(m_permResource);
    if (i < 0) return -1;

    p = m_raster->PrimBuffer(i);

    if (id)
        if (m_dict->Add((VIResource &)*p, id, (VIResourceType)11, i) < 0) return -1;

    if (p->Init(*m_raster, packing, numMats, numPrims, numVerts) < 0) return -1;
    if (p->Lock() < 0) return -1;

    p->SetPacking(vbits, uvbits, 7);

    float vscale = 1.0f / powf(2.0f, vbits);
    float uvscale = 1.0f / powf(2.0f, uvbits);

    for (int j = 0; j < numPrims; j++)
    {
        m_file->Read(nv);
        m_file->Read(mat);

        if (curMat != mat)
        {
            if (curMat != -1) p->EndMaterial();
            p->BeginMaterial(mat);
            curMat = mat;
        }

        switch (packing)
        {
        case 7:
        {
            VIVect3 v;
            VIVect2 uv;

            p->Begin(nv);

            for (int k = 0; k < nv; k++)
            {
                m_file->Read(v.x);
                m_file->Read(v.y);
                m_file->Read(v.z);
                m_file->Read(uv.x);
                m_file->Read(uv.y);
                p->UV0(uv);
                p->Vertex(v);
            }
            p->End();
            break;
        }

        case 8:
            switch (p->m_type)
            {
            case 8:
            {
                VIVect3S16 sv;
                VIVect2S16 suv;

                p->Begin(nv);

                for (int k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(suv.x);
                    m_file->Read(suv.y);
                    p->UV0(suv);
                    p->Vertex(sv);
                }
                p->End();
                break;
            }

            case 7:
            {
                VIVect3 v;
                VIVect2 uv;
                VIVect3 n;
                VIColor32 c;
                VIVect3S16 sv;
                VIVect2S16 suv;

                p->Begin(nv);

                for (int k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(suv.x);
                    m_file->Read(suv.y);
                    v.x = sv.x * vscale;
                    v.y = sv.y * vscale;
                    v.z = sv.z * vscale;
                    uv.x = suv.x * uvscale;
                    uv.y = suv.y * uvscale;
                    p->UV0(uv);
                    p->Vertex(v);
                }
                p->End();
                break;
            }

            default:
                return -1;
            }
            break;

        default:
            return -1;
        }
    }

    if (numMats) p->EndMaterial();

    if (p->CalcBVolumes(0, 0) < 0) return -1;
    if (p->Unlock() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A collision buffer.  Packing 0 is float positions, 1 is 16-bit positions,
// 2 adds a vertex group (Init stores it as 3, which also carries a flora type,
// so a 2 read back as 3 gets the "no flora" -1), and 3 adds the flora type on
// the wire.  A collider that cannot hold the packed form has demoted a 2 to 0,
// and then each vertex is scaled up and moved by its group's pre-translation,
// since a float buffer has no group to carry it.
int VIESFParse::ParseCollBuffer()
{
    int curGroup = -1;
    unsigned short type, version;
    int t;
    VICollBufferType packing;
    int numGroups, numPrims, numVerts;
    int bits;
    int nv, grp, mat;
    int k;

    if (m_file->ReadBegin(type, version) < 0) return -1;
    if (type != 0x4200) return -1;

    if (version >= 2) { m_file->Read(t); packing = (VICollBufferType)t; }
    else packing = VICollBufferTypeV;
    m_file->Read(numGroups);
    m_file->Read(numPrims);
    m_file->Read(numVerts);
    if (version >= 2) m_file->Read(bits);
    else bits = 0;

    int i = m_collide->CreateCollBuffer(m_permResource);
    if (i < 0) return -1;

    VICollBuffer *p = m_collide->CollBuffer(i);

    if (p->Init(packing, numGroups, numPrims, numVerts) < 0) return -1;
    if (p->Lock() < 0) return -1;
    if (p->SetPacking(bits) < 0) return -1;

    float scale = 1.0f / powf(2.0f, bits);

    for (int j = 0; j < numPrims; j++)
    {
        m_file->Read(nv);
        m_file->Read(grp);
        m_file->Read(mat);

        if (curGroup != grp)
        {
            if (curGroup != -1) p->EndPrimGroup();
            p->BeginPrimGroup();
            curGroup = grp;
        }

        switch (packing)
        {
        case 0:
        {
            VIVect3 v;

            p->Begin(mat);

            for (k = 0; k < nv; k++)
            {
                m_file->Read(v.x);
                m_file->Read(v.y);
                m_file->Read(v.z);
                p->Vertex(v);
            }
            p->End();
            break;
        }

        case 1:
        {
            VIVect3S16 sv;

            p->Begin(mat);

            for (k = 0; k < nv; k++)
            {
                m_file->Read(sv.x);
                m_file->Read(sv.y);
                m_file->Read(sv.z);
                p->Vertex(sv);
            }
            p->End();
            break;
        }

        case 2:
            switch (p->m_type)
            {
            case 2:
            {
                VIVect3S16 sv;
                short g;

                p->Begin(mat);

                for (k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(g);
                    p->VertexGroup(g);
                    p->Vertex(sv);
                }
                p->End();
                break;
            }

            case 3:
            {
                VIVect3S16 sv;
                short g;

                p->Begin(mat);

                for (k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(g);
                    p->VertexGroup(g);
                    p->FloraType(-1);
                    p->Vertex(sv);
                }
                p->End();
                break;
            }

            case 0:
            {
                VIVect3 v;
                VIVect3S16 sv;
                short g;

                p->Begin(mat);

                for (k = 0; k < nv; k++)
                {
                    m_file->Read(sv.x);
                    m_file->Read(sv.y);
                    m_file->Read(sv.z);
                    m_file->Read(g);
                    v.x = sv.x * scale;
                    v.y = sv.y * scale;
                    v.z = sv.z * scale;
                    VIVect3 *t = m_zone->m_pretrans.m_data + g;
                    v += *t;
                    p->Vertex(v);
                }
                p->End();
                break;
            }

            default:
                return -1;
            }
            break;

        case 3:
            switch (p->m_type)
            {
            case 3:
            {
            VIVect3S16 sv;
            signed char g, f;

            p->Begin(mat);

            for (k = 0; k < nv; k++)
            {
                m_file->Read(sv.x);
                m_file->Read(sv.y);
                m_file->Read(sv.z);
                m_file->Read(g);
                m_file->Read(f);
                p->VertexGroup(g);
                p->FloraType(f);
                p->Vertex(sv);
            }
            p->End();
            break;
            }

            default:
                return -1;
            }
            break;

        default:
            return -1;
        }
    }

    if (numGroups) p->EndPrimGroup();

    // The pointer is assigned INSIDE the test: declared outside, the address
    // is formed before the null check; inside the arm, cse folds it away.
    VIArray<VIVect3> *pt;

    if (m_zone && (pt = &m_zone->m_pretrans)->m_size)
    {
        if (p->CalcBVolumes(pt->m_size, pt->Data()) < 0) return -1;
    }
    else
    {
        if (p->CalcBVolumes(0, 0) < 0) return -1;
    }

    if (p->Unlock() < 0) return -1;
    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// A character sprite: a hierarchical sprite plus everything that makes it a
// customisable character.  The header grew one field per version -- skeleton
// (1), scale (2), race and sex (3), the NPC flag (4) -- read through ONE
// scratch int, and the sub-objects grew the same way, so every one after the
// hierarchy is gated on the object version.  Without a raster the surfaces, the
// palette and the art ref map are opened and closed unread.
int VIESFParse::ParseCSpriteObj()
{
    VIBBox            b;
    unsigned short    type, version;
    unsigned int      id;
    int               t;
    VICSpriteSkelType skel;
    float             scale;
    VICSpriteRace     race;
    VICSpriteSex      sex;
    int               npc;
    VIResourceType    rt;
    int               i;

    if (m_file->ReadBegin(type, version) < 0) return -1;

    m_file->Read(id);
    m_file->Read(b.min.x);
    m_file->Read(b.min.y);
    m_file->Read(b.min.z);
    m_file->Read(b.max.x);
    m_file->Read(b.max.y);
    m_file->Read(b.max.z);

    if (version >= 1) { m_file->Read(t); skel = (VICSpriteSkelType)t; }
    else skel = VICSpriteSkelType0;

    if (version >= 2) m_file->Read(scale);
    else scale = 1.0f;

    if (version >= 3)
    {
        m_file->Read(t); race = (VICSpriteRace)t;
        m_file->Read(t); sex = (VICSpriteSex)t;
    }
    else
    {
        race = VICSpriteRaceNone;
        sex = VICSpriteSexMale;
    }

    if (version >= 4) m_file->Read(npc);
    else npc = 1;

    if (m_file->ReadEnd() < 0) return -1;

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 9) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateSprite(VISpriteTypeC, m_permResource);
    if (i < 0) return -1;

    VICSprite *p = (VICSprite *)m_scene->Sprite(i);

    if (p->Init(skel, race, sex, scale, npc) < 0) return -1;

    if (id)
        if (m_dict->Add(*p, id, (VIResourceType)9, i) < 0) return -1;

    p->m_bbox = b;
    p->m_bsphere.Init(b);

    if (m_raster)
    {
        if (m_file->ObjectVersion() < 3)
            if (ParseSurfaceArray() < 0) return -1;

        p->m_materialPal = ParseMaterialPal();
        if (p->m_materialPal < 0) return -1;

        p->m_refMap = ParseRefMap();
        if (p->m_refMap < 0) return -1;
    }
    else
    {
        if (m_file->ObjectVersion() < 3)
        {
            m_file->ReadBegin(type, version);
            m_file->ReadEnd();
        }

        m_file->ReadBegin(type, version);
        m_file->ReadEnd();
        m_file->ReadBegin(type, version);
        m_file->ReadEnd();
    }

    if (m_file->ObjectVersion() >= 4)
        if (ParseSoundArray() < 0) return -1;

    if (ParseCSpriteSpriteArray() < 0) return -1;
    if (ParseHSpriteAnimArray() < 0) return -1;
    if (ParseHSpriteHierarchy(*p) < 0) return -1;

    p->VIHSprite::m_refMap = ParseRefMap();
    if (p->VIHSprite::m_refMap < 0) return -1;

    if (m_file->ObjectVersion() >= 4)
        if (ParseHSpriteTriggers(*p) < 0) return -1;

    if (ParseCSpriteSkinList(*p) < 0) return -1;
    if (ParseCSpritePlayList(*p) < 0) return -1;

    if (m_file->ObjectVersion() >= 2)
        if (ParseCSpriteNodeIDList(*p) < 0) return -1;

    if (m_file->ObjectVersion() >= 1)
        if (ParseCSpriteASlotList(*p) < 0) return -1;

    if (m_file->ObjectVersion() >= 5)
        if (ParseCSpriteTSlotList(*p) < 0) return -1;

    if (m_file->ObjectVersion() >= 6)
        if (ParseCSpriteContSound(*p) < 0) return -1;

    if (m_file->ObjectVersion() >= 7)
        if (ParseCSpritePartDefinitions() < 0) return -1;

    if (m_file->ObjectVersion() >= 7)
        if (ParseCSpritePartEmitters(*p) < 0) return -1;

    if (p->SetDefaultEntries() < 0) return -1;
    if (m_raster) p->InitTextSlots(*m_raster);
    if (p->SetAnimPriorities() < 0) return -1;
    if (p->SetAnimSoundChannels() < 0) return -1;
    if (p->Share(*m_scene, m_raster) < 0) return -1;
    if (p->SetDefaults(*m_scene) < 0) return -1;
    if (p->CalcLODOffDist(*m_scene) < 0) return -1;
    if (p->CreateParticleEmitters(*m_scene) < 0) return -1;

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// An animation clip.  The header grew a field per version -- the frame format
// (2), the trigger count (3), the play speed and playback type (1) -- and the
// frames are either whole float frames or packed ones: rotation in 1/32767ths,
// scale and translation in 1/512ths.  An animation the engine keeps unpacked
// has the packed frames expanded here.  One scratch int carries every node's
// ref id, whichever arm reads it.
int VIESFParse::ParseHSpriteAnimObj()
{
    unsigned int        id;
    int                 t;
    VIHSpriteAnimFormat format;
    int                 numNodes, numFrames, numTriggers;
    float               playRate, playSpeed;
    VIPlaybackType      playType;
    VIResourceType      rt;
    int                 i;
    int                 ref;
    int                 trig;
    float               frame;
    int                 j, k;

    if (m_progress) m_progress(m_progressArg);

    m_file->Read(id);

    if (m_file->ObjectVersion() >= 2) { m_file->Read(t); format = (VIHSpriteAnimFormat)t; }
    else format = VIHSpriteAnimFormatFrame;

    m_file->Read(numNodes);
    m_file->Read(numFrames);

    if (m_file->ObjectVersion() >= 3) m_file->Read(numTriggers);
    else numTriggers = 0;

    m_file->Read(playRate);

    if (m_file->ObjectVersion() >= 1)
    {
        m_file->Read(playSpeed);
        m_file->Read((int &)playType);
    }
    else
    {
        playSpeed = 1.0f;
        playType = VIPlaybackTypeLoop;
    }

    if (id)
    {
        if (m_dict->Find(id, rt, i))
        {
            if (rt != 6) return -1;
            m_file->ReadEnd();
            return i;
        }
    }

    i = m_scene->CreateAnimation(m_permResource);
    if (i < 0) return -1;

    VIHSpriteAnim *a = m_scene->Animation(i);

    if (id)
        if (m_dict->Add((VIResource &)*a, id, (VIResourceType)6, i) < 0) return -1;

    if (a->Init(format, numNodes, numFrames, numTriggers, playRate, playSpeed, playType) < 0) return -1;

    switch (format)
    {
    case VIHSpriteAnimFormatFrame:
    {
        if (a->m_format != VIHSpriteAnimFormatFrame) return -1;

        VIHSpriteFrame f;

        for (j = 0; j < numNodes; j++)
        {
            m_file->Read(ref);
            if (a->SetNodeRefID(j, ref) < 0) return -1;

            for (k = 0; k < numFrames; k++)
            {
                m_file->Read(f.m_rot.x);
                m_file->Read(f.m_rot.y);
                m_file->Read(f.m_rot.z);
                m_file->Read(f.m_rot.w);
                m_file->Read(f.m_scale);
                m_file->Read(f.m_trans.x);
                m_file->Read(f.m_trans.y);
                m_file->Read(f.m_trans.z);
                if (a->SetNodeFrame(j, k, f) < 0) return -1;
            }
        }
        break;
    }

    case VIHSpriteAnimFormatPack:
        switch (a->m_format)
        {
        case VIHSpriteAnimFormatPack:
        {
            VIHSpritePackFrame pf;

            for (j = 0; j < numNodes; j++)
            {
                m_file->Read(ref);
                if (a->SetNodeRefID(j, ref) < 0) return -1;

                for (k = 0; k < numFrames; k++)
                {
                    m_file->Read(pf.m_rot.x);
                    m_file->Read(pf.m_rot.y);
                    m_file->Read(pf.m_rot.z);
                    m_file->Read(pf.m_rot.w);
                    m_file->Read(pf.m_scaleTrans.x);
                    m_file->Read(pf.m_scaleTrans.y);
                    m_file->Read(pf.m_scaleTrans.z);
                    m_file->Read(pf.m_scaleTrans.w);
                    if (a->SetNodeFrame(j, k, pf) < 0) return -1;
                }
            }
            break;
        }

        case VIHSpriteAnimFormatFrame:
        {
            VIHSpritePackFrame pf;
            VIHSpriteFrame f;
            for (j = 0; j < numNodes; j++)
            {
                m_file->Read(ref);
                if (a->SetNodeRefID(j, ref) < 0) return -1;

                for (k = 0; k < numFrames; k++)
                {
                    m_file->Read(pf.m_rot.x);
                    m_file->Read(pf.m_rot.y);
                    m_file->Read(pf.m_rot.z);
                    m_file->Read(pf.m_rot.w);
                    m_file->Read(pf.m_scaleTrans.x);
                    m_file->Read(pf.m_scaleTrans.y);
                    m_file->Read(pf.m_scaleTrans.z);
                    m_file->Read(pf.m_scaleTrans.w);
                    // The scale and the translation share one 1/512, so the
                    // four are READ as a run off a pointer (GetNodeFrame's
                    // lever) -- but stored by member: a second pointer for the
                    // destination costs a callee-saved register.
                    short *sp = &pf.m_scaleTrans.x;
                    f.m_rot.x = pf.m_rot.x * (1.0f / 32767.0f);
                    f.m_rot.y = pf.m_rot.y * (1.0f / 32767.0f);
                    f.m_rot.z = pf.m_rot.z * (1.0f / 32767.0f);
                    f.m_rot.w = pf.m_rot.w * (1.0f / 32767.0f);
                    f.m_scale = sp[0] * (1.0f / 512.0f);
                    f.m_trans.x = sp[1] * (1.0f / 512.0f);
                    f.m_trans.y = sp[2] * (1.0f / 512.0f);
                    f.m_trans.z = sp[3] * (1.0f / 512.0f);
                    if (a->SetNodeFrame(j, k, f) < 0) return -1;
                }
            }
            break;
        }

        default:
            return -1;
        }
        break;

    default:
        return -1;
    }

    for (j = 0; j < numTriggers; j++)
    {
        m_file->Read(trig);
        m_file->Read(frame);
        a->SetKeyframe(j, trig, frame);
    }

    if (m_file->ReadEnd() < 0) return -1;
    return i;
}

// PlayStation ADPCM to 16-bit PCM, little-endian, in place of the SPU doing it:
// the well-known VAG decoder, in DOUBLES.  Each 16-byte block is a shift/filter
// byte, a flag byte that is skipped, and 28 four-bit deltas; the filter is the
// standard five-pair table in 64ths.  Returns the bytes written.
int VIESFParse::AdpcmToPcm(void *src, int n, void *dst)
{
    unsigned char *p = (unsigned char *)src;
    unsigned char *q = (unsigned char *)dst;
    double f[5][2] = { {   0.0 / 64.0,   0.0 / 64.0 },
                       {  60.0 / 64.0,   0.0 / 64.0 },
                       { 115.0 / 64.0, -52.0 / 64.0 },
                       {  98.0 / 64.0, -55.0 / 64.0 },
                       { 122.0 / 64.0, -60.0 / 64.0 } };
    double samples[28];
    double s_1 = 0.0, s_2 = 0.0;
    int predict, shift, d, s, i;

    do
    {
        predict = *p;
        shift = predict & 0xf;
        predict >>= 4;
        p += 2;

        for (i = 0; i < 28; i += 2)
        {
            d = *p++;
            s = (d & 0xf) << 12;
            if (s & 0x8000) s |= 0xffff0000;
            samples[i] = (double)(s >> shift);
            s = (d & 0xf0) << 8;
            if (s & 0x8000) s |= 0xffff0000;
            samples[i + 1] = (double)(s >> shift);
        }

        for (i = 0; i < 28; i++)
        {
            samples[i] = samples[i] + s_1 * f[predict][0] + s_2 * f[predict][1];
            s_2 = s_1;
            s_1 = samples[i];
            d = (int)(samples[i] + 0.5);
            *q++ = d & 0xff;
            *q++ = d >> 8;
        }
    }
    while (p - (unsigned char *)src < n);

    return q - (unsigned char *)dst;
}
