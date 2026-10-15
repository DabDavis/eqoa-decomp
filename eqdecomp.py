#!/usr/bin/env python3
"""
eqdecomp -- extract and diff EQOA PS2 module functions against a candidate build.

The oracle is the decrypted, decompressed SNR1 module image indexed by virtual
address.  Nothing in here needs a compiler: extraction, disassembly and the
normalising differ are offline and are useful for ordinary RE work whether or
not the matching-decomp experiment proceeds.

Two things about the target image are load-bearing and are asserted by the
canary in test_eqdecomp.py rather than assumed here:

  * the image is UN-RELOCATED, so `jal` targets are zero and hi/lo address
    pairs carry no address.  An unlinked candidate `.o` has the same shape,
    which is why the primary compare mode needs no normalisation at all.
  * the symbol map's [SUPPORT_DATA] / [SUPPORT_CODE] tags do NOT mean data and
    code.  Both regions are code; DATA is the engine (0x003E3D00..0x004C1CA8)
    and CODE is libgcc + crt (0x004C1D00..).  Do not filter on the tag.
"""

from __future__ import annotations

import hashlib
import os
import re
import shutil
import struct
import subprocess
from dataclasses import dataclass
from pathlib import Path

# The directory holding `claude-eqoa-iso/decrypted_modules/` and `memory-dumps/`.
# In the project tree that is this directory's parent; a standalone checkout
# points EQOA_ROOT at wherever those inputs live.
ROOT = Path(os.environ.get("EQOA_ROOT") or Path(__file__).resolve().parent.parent)

# Every script here runs SN's assembler under wine.  Where ROOT has a PRIVATE
# prefix (`.wine`), use it, so the assembler's wineserver is never the one
# another wine program on the machine is using -- a Windows build test on the
# default `~/.wine` and `:0` shared the box on 2026-10-12.  An explicit
# WINEPREFIX still wins, and the assembler is a console program, so it gets no
# display at all.
if (ROOT / ".wine").is_dir():
    os.environ.setdefault("WINEPREFIX", str(ROOT / ".wine"))
os.environ.pop("DISPLAY", None)
# No desktop integration and no debug chatter for a console assembler.
os.environ.setdefault("WINEDLLOVERRIDES", "winemenubuilder.exe=d")
os.environ.setdefault("WINEDEBUG", "-all")

# --- opcode tables used by the normaliser -----------------------------------

OP_J, OP_JAL, OP_LUI = 0x02, 0x03, 0x0F

# instructions that can carry a %lo(...) half paired with a preceding lui
OP_LO16 = {
    0x09,  # addiu
    0x19,  # daddiu
    0x0D,  # ori
    0x20, 0x21, 0x23, 0x24, 0x25,  # lb lh lw lbu lhu
    0x28, 0x29, 0x2B,              # sb sh sw
    0x31, 0x39,                    # lwc1 swc1
    0x37, 0x3F,                    # ld sd
    0x1E, 0x1F,                    # lq sq
}


# Phase 26.  Four map entries whose address is NOT a function start: they land
# in the MIDDLE of a function body -- `VIRStrip` is the last six instructions
# of VIMap<unsigned,VIDictEntry>::Clear, `VIRStrip2` is AddNode's `jr ra` and
# its delay slot -- so left in the address list they truncate their host by
# exactly their own length and the host reads as "6 words short" forever.
# They are dropped from the gap derivation and given span 0, the same
# treatment `foreign` gets, and listed in SymbolTable.interior.
# `test_span_ignore_names_land_inside_a_function` holds each one to that
# claim: the address must not follow a `jr ra` delay slot, and the host's span
# must grow to end one word past a `jr ra`.
SPAN_IGNORE = {
    "VIRStrip",    # inside Clear__t5VIMap2ZUiZQ212VIDictionary11VIDictEntry
    "VIRStrip2",   # inside AddNode__t5VIMap2ZUiZQ212VIDictionary11VIDictEntryii
    "VIRStrip3",   # inside Insert__t5VIMap2ZUiZQ...
    "VIRStrip4",   # inside EraseFixup__t5VIMap2ZUiZQ...
    "VIRStrip5",   # inside VIGetCurrentDiscId__Fv (phase 61: its destructor tail)
}

# Phase 61.  A map entry that is not code but whose value happens to EQUAL a
# real function's entry point: the `VIRStrip*` names are VU1 microprogram
# labels, and VIRStrip6's value is VIEEUnloadIrx's address.  It cannot go in
# SPAN_IGNORE -- dropping the address would truncate the function's
# predecessor and leave the function itself with no span -- so the address
# stays and only the alias gets span 0 (SymbolTable.alias).  Each entry names
# the function it shadows; `test_span_alias_names_shadow_a_real_function`
# holds it to that.
SPAN_ALIAS = {
    "VIRStrip6": "VIEEUnloadIrx__Fi",
}


@dataclass(frozen=True)
class Module:
    name: str
    image: Path
    symbols: Path
    base: int


def _p(rel: str) -> Path:
    return ROOT / rel

HERE = Path(__file__).resolve().parent

MODULES = {
    "SUPPORT": Module(
        name="SUPPORT",
        image=_p("claude-eqoa-iso/decrypted_modules/SUPPORT_decompressed_SNR1.bin"),
        symbols=_p("claude-eqoa-iso/decrypted_modules/SUPPORT_symbol_map.txt"),
        base=0x003E3D00,
    ),
    # The other EE modules export only their entry points, so their symbol maps
    # are OURS: seeded from the image by `eqmodule.py seed`, kept in the
    # repository, and each placeholder renamed as its function is written.  The
    # base is where the module sat in the dump its addresses were checked
    # against (MCERR_SNR1_0x002A8180.bin: all 282 local jal targets agree).
    "MCERR": Module(
        name="MCERR",
        image=_p("claude-eqoa-iso/decrypted_modules/MCERR_decompressed_SNR1.bin"),
        symbols=HERE / "modules/mcerr/symbols.txt",
        base=0x002A8180,
    ),
    # The bases the old claude-eqoa-iso maps were made at, so their labels land:
    # CLIENT's map is RELATIVE and this is where the in-world dump holds it (all
    # 3,008 of its starts are sweep starts here); STATION's and SETUP's maps put
    # their data at 0x00349858 and 0x005DDBF0, which is each base below plus the
    # data offset in the image's own header (+0x18).
    "CLIENT": Module(
        name="CLIENT",
        image=_p("claude-eqoa-iso/decrypted_modules/CLIENT_decompressed_SNR1.bin"),
        symbols=HERE / "modules/client/symbols.txt",
        base=0x00BB4980,
    ),
    "STATION": Module(
        name="STATION",
        image=_p("claude-eqoa-iso/decrypted_modules/STATION_decompressed_SNR1.bin"),
        symbols=HERE / "modules/station/symbols.txt",
        base=0x002B6A80,
    ),
    "SETUP": Module(
        name="SETUP",
        image=_p("claude-eqoa-iso/decrypted_modules/SETUP_decompressed_SNR1.bin"),
        symbols=HERE / "modules/setup/symbols.txt",
        base=0x00566B00,
    ),
}

# Which module the tools work on: `EQMODULE=MCERR ./_tu.py ...`.  SUPPORT when
# unset.  `load()`, eqcheck's SRC/BUILD/MATCHED and every CLI default read it,
# so a second module is an environment variable, not a flag on every tool.
MODULE = os.environ.get("EQMODULE", "SUPPORT").upper()
if MODULE not in MODULES:
    raise SystemExit(f"EQMODULE={MODULE}: not one of {', '.join(sorted(MODULES))}")


# ---------------------------------------------------------------------------
# Spans that the gap-to-the-next-symbol rule gets WRONG, because the map does
# not name the function that follows.  Each of these four swallows one or more
# whole neighbouring functions -- `Size__Ct6VIPool1ZP15VIParticleMotif` is two
# instructions and derives 568 bytes, the next 566 of which are
# VIParticleAttributes::operator=.  The value is the function's real length,
# measured as its first `jr ra` plus the delay slot.
#
# This is a hand-audited table on purpose and NOT a heuristic: a rule that
# capped every span at its first `jr ra` would silently truncate any function
# with two returns, and a truncated target can only ever be compared against a
# candidate that agrees with the prefix.  `test_span_caps_only_ever_shrink`
# checks every entry shrinks, lands one word past a `jr ra`, and leaves at
# least one further `jr ra` inside the derived span -- i.e. that the gap really
# did swallow another function.
# ---------------------------------------------------------------------------
SPAN_CAP = {
    # Phase 61.  VIESFParse::AdpcmToPcm is 129 instructions; the TU's own
    # copies of the VIMap<int,int> members follow it with no names in the map
    # (the named ones live at 0x00415E08), so it derived 3,032 bytes.
    "AdpcmToPcm__10VIESFParsePviT1":            516,
    # Phase 61 (flora lane): each is followed by unnamed TU-local code.
    "CalcVolumeAndPan__9VICSpriteR8VICameraRC7VIVect3RfT3":     332,  # 2848
    "GetLastImpactTime__13VISpellEffectR7VISceneUiiiiPiT6":     912,  # 3424
    "ParticleCount__C19VISpellEffectDriver":                     84,  # 2600
    "Precalc__17VIZoneRadialFloraR7VIScene":                    728,  # 3240
    "SetTracks__17VISceneSoundTrack":                           320,  # 2832
    "Unload__13VIBattleMusic":                                  128,  # 2640
    "endpoint_close__FP10endpoint_t": 112,  #  528  five unnamed icmp.cpp statics follow
    "IsValidRaceClassCityComboExpansion__FiiiUi":   112,  # 4120      1 (static)
    "charTto8__H1Zc_X01_c":                          12,  #  680      4 (static)
    "ShareResources__16VIStaticLightingR8VIRaster": 128,  #  504   many (static)
    # Phase 61.  The SCE PSS sample TUs keep their helpers as unnamed statics
    # after the public entry points: audiodec's area/send/volume helpers after
    # audioDecSendToIOP, the macroblock tag builder and its GIF/DMA tag helpers
    # after setLoadImageTagsTile, and the demuxer callbacks, the reader and the
    # background clear after playPssFree.
    "audioDecSendToIOP":                         440,  # 1280      5 (static)
    "setLoadImageTagsTile":                       52,  # 1064      7 (static)
    "playPssFree":                                64,  # 1496      7 (static)
    # Phase 61.  VIEEMemCard's TU closes with the unnamed static initialiser
    # that constructs and destroys its file-scope VIIconDefinition, so
    # VIMemCardDeleteFile -- the last named function -- takes its span.
    "VIMemCardDeleteFile__FPCc":                 160,  #  224      1 (static)
    # Phase 61.  The raster TU's VIF1 DMAC handler is followed by an unnamed
    # six-instruction helper (`*p - 1`, clamped at zero) ahead of the
    # VIPool<VIPrimBuffer *> instantiation.
    "VIEERasterDMACHandler__Fi":                  76,  #  104      1 (static)
    # Phase 61.  VIGlobalEE.cpp closes with two unnamed statics -- the CD
    # streaming RPC call and the SIF command handler that clears its busy
    # flag -- so VICdStreamStatus, three instructions, takes their span.
    "VICdStreamStatus__Fv":                       12,  #  160      2 (static)
    # Phase 61.  VIEndian's TU: VIBufferSwap is followed by the unnamed
    # __static_initialization_and_destruction that sets VIEndian.
    "VIBufferSwap__FiPUc":                        56,  #  112      1 (static)
    # Phase 61, from the lanes.  Each is followed by unnamed TU-local code
    # (VIMap<int,int> statics in viscene/vizone/viworld) the map does not name.
    "StreamOutStaticActor__7VISceneiR17VISceneStreamVars": 200,
    "QueryIntersectionRoom__6VIZoneiR20VISceneIntersectVars": 244,
    "QueryIntersection__7VIWorldiR20VISceneIntersectVars": 524,
    "PrepareBillboardTransform__14VIEffectVolumeRC10VIMatrix44T1P10VIMatrix44": 560,
    "RenderFlock__14VIEffectVolumeR8VICameraR10VIMatrix44":     176,
    "ProcessLavastorm__12VIAtmosphereR7VISceneR8VICamera":      1484,
    "Release__16VIParticleSpriteR7VISceneR12VIDictionaryP8VIRasterP13VISoundDeviceR9VICollide": 128,
    # Phase 48.  connection.cpp opens with state_channel_init and is followed
    # immediately by the TU-local static that zeroes a connection_t -- the
    # symbol map has no name for it -- so state_channel_init acquires both.
    # 180 bytes: five zero stores, one `1`, and two vlist_init calls.
    "state_channel_init__FP15state_channel_t":   180,  #  640      1 (static)
    # Phase 48.  Same TU: the two state-channel XOR helpers -- one in place,
    # one into a destination -- are statics sitting between
    # connection_get_ack_mask and buffer_append_size.  172 bytes: one walk of
    # the delayed list and one shift.
    "connection_get_ack_mask__FP12connection_tPUl": 172,  #  304    2 (static)
    # Phase 47.  vihsprite.cpp closes with the TU-local copies of
    # VIMap<int,int>'s walkers, its red-black insert and the two rotations --
    # a run of unnamed statics -- so CalcLODLevelIndex(VIPickSegment &,
    # VIRaster &), the last named symbol in the file, acquires a span reaching
    # to the next instantiation.  268 bytes: three GetModelView calls, one
    # squared distance and the same peeled table walk as its sibling.
    "CalcLODLevelIndex__9VIHSpriteR13VIPickSegmentR8VIRaster": 268,  # 2784  many
    # Phase 45.  usocket.cpp opens with two unnamed statics -- the sceInet close
    # wrapper and the 440-byte open -- plus a third, an eight-byte `return 0`,
    # just before usocket_create_internal, so usocket_is_valid takes the span of
    # all three.  40 bytes: one memcmp and a `!= 0`.
    "usocket_is_valid__FP9usocket_t":             40,  #  536      3 (static)
    # Phase 45.  Same TU: the static that actually sends -- the one carrying the
    # `USOCKET_TYPE_UDP` assert at line 553 -- follows usocket_destroy.
    "usocket_destroy__FP9usocket_t":             104,  #  296      1 (static)
    # Phase 45.  And again: the static that actually receives, the assert at
    # line 729, follows usocket_sendto.
    "usocket_sendto__FP9usocket_tPCviiii":       132,  #  504      1 (static)
    # Phase 43.  vtree.cpp's delete fixup is a static -- the symbol map has no
    # name for it -- so vtree_contains, the named symbol in front of it,
    # acquires a span covering all 400 bytes of it.  112: the node lookup, the
    # assert and one call.
    "vtree_contains__FP7vtree_tPv":              112,  #  512      1 (static)
    # Phase 43.  buffer.cpp opens with TWO unnamed statics -- the byte-order
    # aware read and append helpers every scalar accessor in the file calls --
    # and the map's previous entry is the last named symbol of the route table
    # TU, so it acquires a span covering both.  44 bytes: two compares and a
    # `-1`.
    "uns64_range_t_cmp__FPC13uns64_range_tT0":    44,  #  440      2 (static)
    # Phase 43.  Same TU, same cause, in the middle of it: buffer_init_read is
    # followed by the two unnamed `m` helpers, so it acquires their span.
    # 44 bytes: the call to buffer_init and one store.
    "buffer_init_read__FP8buffer_tPCvii":         44,  #  432      2 (static)
    # Phase 41.  VICSFFile's TU closes with the same VIMap<int,int> statics
    # VILoader's does, so Decompress -- the last named symbol in the file --
    # acquires a span reaching to the next instantiation.  620 bytes.
    "Decompress__9VICSFFileR6VIFileRiPPQ26VIFile7MemSegm": 620,  # 3136  many
    # Phase 41.  VILoader's TU closes with the TU-local copies of
    # VIMap<int,int>'s walkers and destructor -- a run of unnamed statics --
    # so Align, the last named symbol in the file, acquires a span reaching
    # all the way to the VIPool<VILoaderTrans> instantiation.  192 bytes: the
    # two align-ups, one `__umoddi3` and one inline `divu`.
    "Align__8VILoaderR13VILoaderTrans":          192,  # 2704   many (static)
    # Phase 40.  VIEESurfCache's TU closes with the TU-local copies of
    # VIMultiMap<unsigned,int>'s End/Size/Begin/Next/Prev/DestroyPool, so
    # DebugVerify -- the last named symbol in the file, and shipped as a bare
    # `return 0` -- acquires a span reaching to the next instantiation.
    "DebugVerify__13VIEESurfCache":                8,  #  400      6 (static)
    # Phase 40.  Same TU, same cause, at the other end: the destructor of
    # VIMultiMap<unsigned,int> is followed by an unnamed static, so it takes
    # the span of the three other instantiations' destructors, 100 bytes.
    "_$_t10VIMultiMap2ZUiZi":                    100,  #  456      1 (static)
    # name                                    bytes   derived   whole functions
    #                                                            in the gap
    # Phase 37.  VISpellEffectSystem's TU closes with VISpellEffect::
    # GetLastImpactTime's helpers -- a run of unnamed statics -- so NextPlayback,
    # the last named symbol before the VIPool instantiation, acquires a span
    # reaching all the way to it.  Sixteen bytes: a load, a load, `movz`, `jr`.
    "NextPlayback__19VISpellEffectSystemPQ219VISpellEffectSystem16VISpellEffectPbi": 16,  # 2528  many
    # Phase 37.  VIWalkControl's TU carries VIWalkControl::CalcFloor's own
    # helpers between the floor filter and VIWalkPhysics::operator=, so the
    # filter -- two calls and a compare -- acquires a span reaching to __tf.
    "FilterActor__17VIWalkFloorFilterR7VIScenei":  40,  # 2640  many
    # Phase 37.  Same shape in the radial flora TU: the filter's __tf is the
    # last named symbol before a run of unnamed VIArray/VIList walkers.
    "__tf19VIRadialFloraFilter":                   80,  # 2592  many
    # Phase 39.  VISoundDevice's TU closes with the five unnamed SIF RPC statics
    # (bind, call, flush, the packet append and the packet search), so
    # DebugResources -- the last named symbol in the file -- acquires a span
    # reaching all the way to VIWave's constructor.  308 bytes: the 128-entry
    # census plus its six printfs.
    "DebugResources__13VISoundDevice":           308,  #  936      5 (static)
    # Phase 27.  The four-argument GetDayOfWeek ends at 0x003E8988; the two
    # instructions after the padding are an unnamed static returning 1.
    "GetDayOfWeek__11VITimeOfDayiiiRi":         388,  #  400      1 (static)
    # Phase 27.  gcc 2.x's __static_initialization_and_destruction is static
    # (Phase 14 saw the same thing in VIPrimBuffer); the file-scope
    # VIGameOptions object puts one right after the last accessor.
    "SetHomeShardName__13VIGameOptionsPCUs":     36,  #   88      1 (static)
    # Phase 27.  EndQuery ends at 0x00470A44; the fifteen instructions after it
    # are an unnamed static that walks a VIPool free chain against 0x7FFFFFFF.
    "EndQuery__12VISceneQuery":                 140,  #  224      1 (static)
    # Phase 34.  VIEESurfCache's TU carries the VIMap<int,VIEESurfCacheInfo>
    # twins, so the bucket pool's Clear -- the last named symbol before them --
    # swallows the map's unnamed Begin/Next/End walkers whole.
    "Clear__t6VIPool1Z10VIEEBucket":            104,  #  472      3+
    # Phase 34.  VILoader's TU closes with the unnamed VIArray/VILList walkers
    # and VILoaderTrans's own destructor, so the pool's last two named symbols
    # acquire spans that reach into them.
    "Init__t6VIPool1Z13VILoaderTransi":           80,  #  264      2+
    "Erase__t6VIPool1Z13VILoaderTransi":         220,  #  240      1 (static)
    # Phase 34.  The zone TU's VIArray<int> twins sit between VIZoneRoom's last
    # named symbol and the next, so Allocate's derived span swallows them.
    "Allocate__10VIZoneRoom":                     72,  #  368      3+
    "Size__Ct6VIPool1ZP15VIParticleMotif":        8,  #  568      1+
    "_$_t6VIPool1ZP16VIStaticLighting":         100,  #  304      3+
    "Init__t6VIPool1ZP16VIStaticLightingi":      80,  #  376      3+
    "Erase__t6VIPool1ZP16VIStaticLightingi":    220,  #  640      5+
    # gcc 2.x's __static_initialization_and_destruction is STATIC, so the map
    # has no entry for it; it sits between Allocate and _GLOBAL_$I$, and
    # Allocate's derived span swallows it whole.
    "Allocate__12VIPrimBuffer":                  72,  #  360      1 (static)
    # Phase 23.  The IOP-streaming CD read path ends at 0x003E714C; the ten
    # instructions after it are a static interrupt handler (iSignalSema, then
    # `sync` + `ei`) that the map does not name, and CDRead's derived span
    # swallows it whole.
    "CDRead__6VIFileUiUiPvP10sceCdRMode":        292,  #  336      1 (static)
    # Every TU that needs RTTI for a class emits its OWN __tf<class>, so the
    # image holds three copies each of __tf7VISound and __tf10VIResource and
    # the map names only one of them.  The unnamed copies sit in the gap.
    "AsXm__7VISound":                            48,  #  192      2 (__tf x2)
    "__tf7VISound":                              80,  #  144      1 (__tf)
    "__tf4VIXm":                                 80,  #  224      2 (__tf x2)
    # The sprite TU's template instantiations (VIMultiMap<float,VIEmitterEntry>
    # for QueryEmitters, VIPool walkers for the palette) sit between a named
    # function and the next named symbol, so two 2-to-33 instruction functions
    # acquire kilobyte spans.
    "Release__8VISpriteR7VISceneR12VIDictionaryP8VIRasterP13VISoundDeviceR9VICollide":
                                                  8,  # 3512      8+
    "SetMaterialPalDither__14VISimpleSpriteiR8VIRaster":
                                                132,  # 2648     14+
    # Phase 20.  The same twelve unnamed template functions close EVERY sprite
    # TU -- the jr-ra offsets after the cap are identical in all six, shifted
    # only by the capped function's own length -- so the last named symbol in
    # each leaf class acquires a two-to-three kilobyte span.
    "SetMaterialPal__12VISkinSpriteiR8VIRasterR12VIDictionary":
                                                112,  # 2624     12
    "SetMaterialPalDither__13VIFloraSpriteiR8VIRaster":
                                                132,  # 2648     12
    "Release__11VILODSpriteR7VISceneR12VIDictionaryP8VIRasterP13VISoundDeviceR9VICollide":
                                                152,  # 2664     12
    "Release__13VIGroupSpriteR7VISceneR12VIDictionaryP8VIRasterP13VISoundDeviceR9VICollide":
                                                152,  # 2664     12
    "ClampScale__13VIPointSpriteR10VIMatrix44":
                                                492,  # 3008     12
    # Phase 35.  VIPointLight is VIPointSprite's twin and closes the same way.
    "ClampScale__12VIPointLightR10VIMatrix44":
                                                492,  # 3008     12+
    # Phase 35.  VICSpriteCust's last named symbol is followed by gcc's own
    # static-initialisation helper for $Tints, which has no symbol of its own.
    "GetTintColor__13VICSpriteCust13VICSpriteTint":
                                                 20,  # 104      12+
    # Phase 32.  The four remaining sprite leaves close the same way: the last
    # named symbol in each TU is followed by the unnamed template twins (and,
    # in VINameSprite's case, the deferred-name sort's own statics).
    "RasterDeferred__12VINameSpriteR8VIRaster":
                                                540,  # 3328     12+
    "EnablePan__13VISoundSpriteb":
                                                  8,  # 2520     12+
    "ClampScale__19VIStreamAudioSpriteR10VIMatrix44":
                                                492,  # 6936     12+
    "Release__20VIEffectVolumeSpriteR7VISceneR12VIDictionaryP8VIRasterP13VISoundDeviceR9VICollide":
                                                 88,  # 2600     12+
    # Phase 24.  VIArray<T> is the second template, and every TU that uses one
    # emits its own STATIC copies of the pieces gcc chose not to make weak --
    # a second DestroyPool beside the constructor, a second Clear/Allocate
    # beside Init.  The map names one of each, so the named symbol's derived
    # span swallows the unnamed twin whole.
    "__t7VIArray1Z10VIZoneRoom":                     16,
    "__t7VIArray1Z11VIClipEntry":                    16,
    "__t7VIArray1Z14VIResourceElem":                 16,
    "__t7VIArray1Z16VIHSpriteTrigger":               16,
    "__t7VIArray1Z16VIRadialFloraSet":               16,
    "__t7VIArray1Z17VIRadialFloraDist":              16,
    "__t7VIArray1Z18VIRadialFloraModel":             16,
    "__t7VIArray1Z21VIZoneStaticTableElem":          16,
    "__t7VIArray1Z7VIPlane":                         16,
    "__t7VIArray1Z7VIVect3":                         16,
    "_$_t7VIArray1Z10VIZoneRoom":                   100,
    "_$_t7VIArray1Z11VIClipEntry":                  100,
    "_$_t7VIArray1Z16VIRadialFloraSet":             100,
    "Init__t7VIArray1Z11VIClipEntryi":               80,
    "Init__t7VIArray1Z13VIWorldRegioni":             80,
    "Init__t7VIArray1Z16VIHSpriteTriggeri":          80,
    "Init__t7VIArray1Z17VIRadialFloraInsti":         80,
    "Init__t7VIArray1Z20VICSpritePartEmitteri":      80,
    # Phase 25.  VIHSpriteAnim's TU closes with a VIMap<int,int> instantiation
    # the map does not name, so Allocate -- the last named symbol in it -- takes
    # a 2,584-byte span for a 72-byte function.
    "Allocate__13VIHSpriteAnim":                     72,  # 2584     20+
    # Phase 25.  VIVector<T> is the third template and behaves exactly as
    # VIArray did: every TU holding one emits its own static twins, so a named
    # symbol's derived span swallows them.
    "__t8VIVector1Z13VIHSpriteNode":                 20,  #  192
    "__t8VIVector1Z10VIMatrix44":                    20,  #  176
    "Init__t8VIVector1Z13VIHSpriteNodei":            80,  #  568
    "Init__t8VIVector1Z10VIMatrix44i":               80,  #  400
    "Init__t8VIVector1Zii":                          80,  #  448
    "Clear__t8VIVector1Zi":                         100,  #  320
    # Phase 25.  The three VALUE VIPool instantiations VIHSprite owns, same
    # static-twin picture again.
    "__t6VIPool1Z19VIHSpriteAttachment":             36,  #  248
    "__t6VIPool1Z13VIHSpritePlay":                   36,  #  264
    "__t6VIPool1Z17VIHSpritePlayNode":               36,  #  264
    "_$_t6VIPool1Z17VIHSpritePlayNode":             100,  #  720
    "Size__Ct6VIPool1Z13VIHSpritePlay":               8,  #  424
    "Prev__Ct6VIPool1Z19VIHSpriteAttachmenti":       76,  #  552
    "Add__t6VIPool1Z13VIHSpritePlay":               220,  #  752
    "Erase__t6VIPool1Z13VIHSpritePlayi":            220,  # 2824
    # Phase 26.  Every VIMultiMap instantiation shares its TU with a VIPool of
    # its own, whose walkers gcc did not make weak; the map names the tree's
    # functions and not the pool's, so three of them acquire the gap.
    "Clear__t10VIMultiMap2ZUiZi":                   112,  #  336
    "Clear__t10VIMultiMap2ZfZi":                    112,  #  624
    "Init__t10VIMultiMap2ZfZii":                     88,  #  232
    # Phase 31.  VIFlyCamera's last named function ends at 0x004422F8 and the
    # next symbol is VIFrustum's, 2.5 KB later: the whole of VIFlyCamera's
    # unnamed statics -- and the un-named tail of its TU -- sit in the gap.
    "InitCameraIntermediate__11VIFlyCamera":        360,  # 2872
    # Phase 31, and the same picture in VIWalkCamera's TU: the last named
    # function ends at 0x00483EB8 and the next symbol is the filter's __tf,
    # 2.5 KB later.
    "FindBestOrbitAngle__12VIWalkCamerafR7VISceneRf": 760,  # 3272
    # Phase 33.  vitrailfx.cpp's three file-scope VILLists give the TU a static
    # constructor, and gcc's unnamed __sti routine for it sits between the last
    # named function and _GLOBAL_$I$ -- which calls it at +0x68.
    "ClearSegmentsForward__9VITrailFxPQ29VITrailFx7SEGMENT": 104,  # 232
}


@dataclass
class Symbol:
    name: str
    addr: int
    span: int      # next symbol's address minus this one
    tag: str
    capped: bool = False

    @property
    def end(self) -> int:
        return self.addr + self.span


class SymbolTable:
    """Addresses and derived sizes for one module.

    Sizes come from the gap to the next symbol, so a `span` includes any
    alignment padding the linker left.  `trimmed_span` drops trailing zero
    words; both are reported because which one is right depends on whether the
    candidate is padded too.
    """

    # a seeded map row may end in `# hint` (what an assert or the old RE map says
    # about a placeholder); a row this does not match is SKIPPED, so the hint
    # must stay optional here or every hinted row would silently vanish
    _LINE = re.compile(r"^(0x[0-9A-Fa-f]+)\s+(?:\[([^\]]*)\]\s+)?(\S+)\s*(?:#.*)?$")

    # no real function in these modules approaches this; a larger derived span
    # means the gap crossed a region boundary, not that the function is huge
    MAX_SPAN = 0x10000

    def __init__(self, module: Module, bounds: tuple[int, int] | None = None):
        self.module = module
        self.bounds = bounds
        rows: list[tuple[int, str, str]] = []
        for line in module.symbols.read_text(errors="replace").splitlines():
            if not line.startswith("0x"):
                continue
            m = self._LINE.match(line)
            if not m:
                continue
            rows.append((int(m.group(1), 16), (m.group(2) or "").strip(), m.group(3)))
        rows.sort(key=lambda r: r[0])

        # distinct addresses drive the size derivation, so aliases at one
        # address do not produce zero-length functions -- and an address that
        # is not a function start at all (SPAN_IGNORE) is kept out of the list
        # entirely, so it cannot truncate the function it sits inside
        ignored = {r[0] for r in rows if r[2] in SPAN_IGNORE}
        addrs = sorted({r[0] for r in rows} - ignored)
        nxt = {a: b for a, b in zip(addrs, addrs[1:])}
        last = addrs[-1] if addrs else 0

        self.symbols: list[Symbol] = []
        self._by_name: dict[str, Symbol] = {}
        self.foreign: list[Symbol] = []   # outside this module's image
        self.interior: list[Symbol] = []  # SPAN_IGNORE: not a function start
        self.alias: list[Symbol] = []     # SPAN_ALIAS: shadows a real function
        self.oversize: list[Symbol] = []  # derived span crossed a region gap
        self.capped: list[Symbol] = []    # SPAN_CAP: the map omits the next function
        for addr, tag, name in rows:
            span = (nxt[addr] - addr) if addr in nxt else 0
            reason = None
            if name in SPAN_IGNORE:
                reason, span = "interior", 0
            elif name in SPAN_ALIAS:
                reason, span = "alias", 0
            if bounds is not None and not (bounds[0] <= addr < bounds[1]):
                reason, span = "foreign", 0
            elif span > self.MAX_SPAN:
                reason, span = "oversize", 0
            capped = False
            if name in SPAN_CAP and 0 < SPAN_CAP[name] < span:
                span, capped = SPAN_CAP[name], True
            sym = Symbol(name=name, addr=addr, span=span, tag=tag, capped=capped)
            if capped:
                self.capped.append(sym)
            if reason == "interior":
                self.interior.append(sym)
            elif reason == "alias":
                self.alias.append(sym)
            elif reason == "foreign":
                self.foreign.append(sym)
            elif reason == "oversize":
                self.oversize.append(sym)
            self.symbols.append(sym)
            self._by_name.setdefault(name, sym)

    @property
    def sized(self) -> list["Symbol"]:
        """Symbols with a usable extent in this module's image."""
        return [s for s in self.symbols if s.span > 0]

    def __len__(self) -> int:
        return len(self.symbols)

    def get(self, name: str) -> Symbol:
        try:
            return self._by_name[name]
        except KeyError:
            raise KeyError(f"no symbol {name!r} in {self.module.name}") from None

    def find(self, pattern: str) -> list[Symbol]:
        rx = re.compile(pattern)
        return [s for s in self.symbols if rx.search(s.name)]

    def add_foreign(self, name: str, addr: int, tag: str) -> None:
        """A name outside the image (an import's stand-in address): it resolves
        addresses but is never scored."""
        sym = Symbol(name=name, addr=addr, span=0, tag=tag)
        self.symbols.append(sym)
        self.foreign.append(sym)
        self._by_name.setdefault(name, sym)

    def at(self, addr: int) -> Symbol | None:
        best = None
        for s in self.symbols:
            if s.addr <= addr and (s.span == 0 or addr < s.end):
                if best is None or s.addr > best.addr:
                    best = s
        return best


class Image:
    """The module image, addressed by virtual address."""

    def __init__(self, module: Module):
        self.module = module
        self.data = module.image.read_bytes()
        self.base = module.base

    def covers(self, addr: int, length: int) -> bool:
        off = addr - self.base
        return off >= 0 and length >= 0 and off + length <= len(self.data)

    def read(self, addr: int, length: int) -> bytes:
        if not self.covers(addr, length):
            raise ValueError(
                f"VA 0x{addr:08X}+{length} outside {self.module.name} image "
                f"(base 0x{self.base:08X}, {len(self.data)} bytes)"
            )
        off = addr - self.base
        return self.data[off:off + length]

    def func(self, sym: Symbol, trim: bool = False) -> bytes:
        b = self.read(sym.addr, sym.span)
        return trim_padding(b) if trim else b


JR_RA = 0x03E00008


def trim_padding(b: bytes) -> bytes:
    """Drop trailing alignment padding, but NOT the final delay slot.

    A naive "strip trailing zero words" eats the `nop` in the delay slot of the
    terminating `jr ra`, which is part of the function -- it made a byte-exact
    Transpose__10VIMatrix44RC10VIMatrix44 read as a one-instruction failure.
    So the function ends one word after the last `jr ra`; only zero words
    beyond that are padding.
    """
    n = len(b) - (len(b) % 4)
    words = [struct.unpack_from("<I", b, i)[0] for i in range(0, n, 4)]
    last_jr = None
    for i, w in enumerate(words):
        if w == JR_RA:
            last_jr = i
    if last_jr is not None and last_jr + 1 < len(words):
        end = (last_jr + 2) * 4          # jr ra, plus its delay slot
        while end < n and words[end // 4] == 0:
            end += 4
            break                        # only strip; never extend past padding
        return b[:(last_jr + 2) * 4]
    while n >= 4 and b[n - 4:n] == b"\0\0\0\0":
        n -= 4
    # A function that never RETURNS has no `jr ra` to anchor on, and the rule
    # above then eats the delay slot of whatever branch does terminate it -- the
    # `for (;;) ;` both DRDP asserts end in reads as one instruction short.  Same
    # argument as the `jr ra` case: a terminating branch owns the word after it.
    if n >= 4 and is_branch(struct.unpack_from("<I", b, n - 4)[0]) and n < len(b) - (len(b) % 4):
        n += 4
    return b[:n]


def is_branch(w: int) -> bool:
    """True for an instruction that has a delay slot."""
    op = w >> 26
    if op in (0x01, 0x02, 0x03) or 0x04 <= op <= 0x07 or 0x14 <= op <= 0x17:
        return True
    return op == 0x00 and (w & 0x3F) in (0x08, 0x09)


# --- relocation normalisation -----------------------------------------------

@dataclass
class NormReport:
    j26: int = 0
    hi16: int = 0
    lo16: int = 0

    @property
    def total(self) -> int:
        return self.j26 + self.hi16 + self.lo16

    def __str__(self) -> str:
        # counted by kind, never as one number: a single total cannot say
        # whether a clean diff was earned or normalised into existence
        return f"j26={self.j26} hi16={self.hi16} lo16={self.lo16}"


def normalise(data: bytes, modes: set[str]) -> tuple[bytes, NormReport]:
    """Zero the fields a relocation would have written.

    `j26` is the only mode that is safe by construction -- the image genuinely
    holds zero there, and an unlinked .o does too.  `hi16lo16` is a heuristic
    pairing pass and is opt-in: it can erase a real constant, which is why the
    report counts what it touched.
    """
    rep = NormReport()
    if not modes:
        return data, rep

    words = bytearray(data)
    n = len(words) // 4

    def w(i: int) -> int:
        return struct.unpack_from("<I", words, i * 4)[0]

    def setw(i: int, v: int) -> None:
        struct.pack_into("<I", words, i * 4, v & 0xFFFFFFFF)

    if "j26" in modes:
        for i in range(n):
            x = w(i)
            if (x >> 26) in (OP_J, OP_JAL) and (x & 0x03FFFFFF):
                setw(i, x & ~0x03FFFFFF)
                rep.j26 += 1

    if "hi16lo16" in modes:
        for i in range(n):
            x = w(i)
            if (x >> 26) != OP_LUI:
                continue
            rt = (x >> 16) & 0x1F
            paired = False
            for j in range(i + 1, min(i + 9, n)):
                y = w(j)
                op = y >> 26
                if op in OP_LO16 and ((y >> 21) & 0x1F) == rt:
                    if y & 0xFFFF:
                        setw(j, y & ~0xFFFF)
                        rep.lo16 += 1
                    paired = True
                    break
                # rt redefined by a lui or another lo16 target: stop looking
                if op == OP_LUI and ((y >> 16) & 0x1F) == rt:
                    break
            if paired and (x & 0xFFFF):
                setw(i, x & ~0xFFFF)
                rep.hi16 += 1

    return bytes(words), rep


# --- disassembly -------------------------------------------------------------

def _find_objdump() -> str:
    ps2dev = os.environ.get("PS2DEV", os.path.expanduser("~/ps2dev/ps2dev"))
    for cand in (
        os.environ.get("EQ_OBJDUMP"),
        f"{ps2dev}/ee/bin/mips64r5900el-ps2-elf-objdump",
        "mips64r5900el-ps2-elf-objdump",
        "mips-linux-gnu-objdump",
    ):
        if cand and (Path(cand).is_file() or shutil.which(cand)):
            return cand
    raise RuntimeError(
        "no objdump able to decode mips:5900; set EQ_OBJDUMP or PS2DEV. "
        "The EE binutils decode VU0 macro-mode (vsub/vmr32/sqc2); a generic "
        "mips objdump does not, and half of VIMatrix44 is VU0."
    )


OBJDUMP = None
_INSN = re.compile(r"^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s+(.*)$")


def disasm(data: bytes, vma: int = 0) -> list[tuple[int, int, str]]:
    """(address, word, text) per instruction. Raises if the tool is missing."""
    global OBJDUMP
    if OBJDUMP is None:
        OBJDUMP = _find_objdump()
    import tempfile
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        f.write(data)
        path = f.name
    try:
        out = subprocess.run(
            [OBJDUMP, "-D", "-z", "-b", "binary", "-m", "mips:5900", "-EL",
             f"--adjust-vma=0x{vma:X}", path],
            capture_output=True, text=True, check=True,
        ).stdout
    finally:
        os.unlink(path)

    rows = []
    for line in out.splitlines():
        m = _INSN.match(line)
        if m:
            rows.append((int(m.group(1), 16), int(m.group(2), 16),
                         m.group(3).strip()))
    return rows


# --- helpers -----------------------------------------------------------------

def load(module_name: str | None = None) -> tuple[Module, SymbolTable, Image]:
    mod = MODULES[module_name or MODULE]
    if not mod.image.exists():
        raise FileNotFoundError(f"module image missing: {mod.image}")
    if not mod.symbols.exists():
        raise FileNotFoundError(f"symbol map missing: {mod.symbols}")
    img = Image(mod)
    st = SymbolTable(mod, bounds=(img.base, img.base + len(img.data)))
    if mod.name != "SUPPORT":
        for k, name in module_imports(img.data).items():
            st.add_foreign(name, import_address(k), "IMPORT")
    return mod, st, img


# --- a module's own tables ---------------------------------------------------
#
# Every SNR1 module carries the tables the loader links it with: header
# +0x04/+0x08 is the relocation table and its count, +0x0C/+0x10 the symbol
# table, 12 bytes an entry each.  A relocation is (site, (import << 8) | type,
# target) and the types are the ELF numbers: 2 a 32-bit word, 4 a j/jal, 5 HI16,
# 6 LO16.  Import 0 is this module (address = base + target); anything else is
# that entry of the symbol table, bound by NAME at load (address = the symbol's
# + target).  Measured 2026-10-15: SUPPORT relocated this way equals the
# in-world dump in every word of its code, and MCERR equals its region dump at
# every local site.

R_WORD, R_JAL, R_HI16, R_LO16 = 2, 4, 5, 6


def module_relocations(data: bytes) -> list[tuple[int, int, int, int]]:
    """(site, type, import, target) for every relocation in an SNR1 image."""
    table, count = struct.unpack_from("<II", data, 4)
    out = []
    for i in range(count):
        site, info, target = struct.unpack_from("<III", data, table + 12 * i)
        out.append((site, info & 0xFF, info >> 8, target))
    return out


def _cstr(data: bytes, off: int) -> str:
    end = data.find(b"\0", off)
    return data[off:end].decode("latin-1")


def module_symbols(data: bytes) -> list[tuple[str, int]]:
    """(name, value) per symbol-table entry; value 0 is an import."""
    table, count = struct.unpack_from("<II", data, 0x0C)
    return [(_cstr(data, n), v) for n, v, _ in
            (struct.unpack_from("<III", data, table + 12 * k) for k in range(count))]


def module_imports(data: bytes) -> dict[int, str]:
    """Import index -> name, for every index a relocation uses."""
    syms = module_symbols(data)
    used = {imp for _, _, imp, _ in module_relocations(data) if imp}
    out = {}
    for k in sorted(used):
        name, value = syms[k]
        if value or not name:
            raise ValueError(f"relocation import {k} is {name!r} = {value:#x}, not an import")
        out[k] = name
    return out


# Where SUPPORT's imports resolved at load (SLUS and the linker's own symbols),
# read once out of the in-world dump.  They are build-specific, so they are
# only used for the image whose md5 is SUPPORT_IMAGE_MD5.
SUPPORT_IMAGE_MD5 = "c47e958fe5e4d213de8f01761145c136"
SUPPORT_IMPORTS = {
    1: 0x00116D38, 2: 0x00117828, 3: 0x00100920, 4: 0x00131470, 5: 0x001192B0,
    6: 0x001192C0, 7: 0x001193B8, 8: 0x00116C70, 9: 0x001192D0, 10: 0x0013C460,
    11: 0x00119328, 12: 0x00117BC0, 13: 0x00116558, 14: 0x001009D0, 15: 0x0014C89C,
    16: 0x0012E928, 17: 0x00130B70, 18: 0x00133304, 19: 0x00133030, 20: 0x00133E80,
    21: 0x001331C0, 22: 0x00117850, 23: 0x00117B58, 24: 0x00117DF8, 25: 0x00143DB8,
    26: 0x00143788, 27: 0x00143990, 28: 0x00131298, 29: 0x001443C8, 30: 0x0013E430,
    31: 0x0013E460, 32: 0x00144410, 33: 0x0013EEB8, 34: 0x0013C090, 35: 0x0013F278,
    36: 0x0013F088, 37: 0x001404B8, 38: 0x00140950, 39: 0x001309BC, 40: 0x0011B5C0,
    41: 0x0011BA60, 42: 0x0011AE38, 43: 0x0011B060, 44: 0x0011B880, 45: 0x0013C210,
    46: 0x0013C1E0, 47: 0x0013BF60, 48: 0x0013C220, 49: 0x0013C1F0, 50: 0x00114130,
    51: 0x00113FF8, 52: 0x00114630, 53: 0x00132BE0, 54: 0x00114320, 55: 0x001148D0,
    56: 0x00113AA0, 57: 0x00113978, 58: 0x00113E10, 59: 0x00114818, 60: 0x00113AD8,
    61: 0x00113CF8, 62: 0x00114700, 63: 0x0013CCC0, 64: 0x0013BEC0, 65: 0x0013CD28,
    66: 0x0013BEE0, 67: 0x0011BBB0, 68: 0x001338A8, 69: 0x00132F04, 70: 0x001339E0,
    71: 0x00135598, 72: 0x00132E50, 73: 0x00133A38, 74: 0x001008D8, 75: 0x001197B0,
    76: 0x00126470, 77: 0x00126368, 78: 0x00127830, 79: 0x001270C0, 80: 0x00125FC8,
    81: 0x00125CB0, 82: 0x00126EB0, 83: 0x001266D8, 84: 0x00126FC0, 85: 0x001265E8,
    86: 0x00127328, 87: 0x001165E8, 88: 0x00116DD0, 89: 0x001170B8, 90: 0x00119A70,
    91: 0x0013BEF0, 92: 0x0013CDF8, 93: 0x0013CD90, 94: 0x0013BF10, 95: 0x00131EF8,
    96: 0x00126560, 97: 0x001260C0, 98: 0x001271E8, 99: 0x001314C0, 100: 0x00131EE8,
    101: 0x0013C580, 102: 0x0013C5A0, 103: 0x00134040, 104: 0x00133CC8, 105: 0x001030E8,
    106: 0x0010B880, 107: 0x00111C40, 108: 0x00111BB0, 109: 0x00110FE0, 110: 0x00110D78,
    111: 0x00110EC0, 112: 0x001115A8, 113: 0x00110E90, 114: 0x00111160, 115: 0x00118290,
    116: 0x00118478, 117: 0x00118778, 118: 0x00118660, 119: 0x00118A40, 120: 0x00118800,
    121: 0x00118CF0, 122: 0x001185E0, 123: 0x00118B78, 124: 0x00118C30, 125: 0x00130928,
    126: 0x001356F8, 127: 0x001257C0, 128: 0x00114DC8, 129: 0x001256E8, 130: 0x00125840,
    131: 0x00125C78, 132: 0x00124B88, 133: 0x00124C28, 134: 0x001688F0, 135: 0x0013BFE0,
    136: 0x0013C000, 137: 0x0013C070, 138: 0x0013C0E0, 139: 0x0013C030, 140: 0x0013BFF0,
    141: 0x00124AE8, 142: 0x00124D88, 143: 0x00124CD0, 144: 0x00124D40, 145: 0x00125188,
    146: 0x00124DD0, 147: 0x0013C200, 148: 0x00124E98, 149: 0x0013C100, 150: 0x00125058,
    151: 0x0013C110, 152: 0x00137B58, 153: 0x00133478, 154: 0x00137B28, 155: 0x00123DA0,
    156: 0x00123C60, 157: 0x0011D038, 158: 0x0011C728, 159: 0x0011D450, 160: 0x0011D3A8,
    161: 0x0011D240, 162: 0x0011C708, 163: 0x001164B8, 164: 0x0011CF90, 165: 0x00116838,
    166: 0x00116BB0, 167: 0x0011D270, 168: 0x0011D388, 169: 0x0012F840, 170: 0x0015ED40,
    171: 0x0011B2E0, 172: 0x0011B7B8, 173: 0x00142EF0, 174: 0x0011BCA8, 175: 0x0011BCD8,
    176: 0x001400B0, 177: 0x00140C10, 178: 0x00140338, 179: 0x0011BD70, 180: 0x00143058,
    181: 0x0011BDA8, 182: 0x001406F0, 183: 0x00160900, 184: 0x00170A18, 185: 0x01F80000,
    186: 0x00080000, 187: 0xFFFFFFFF, 188: 0x00144650, 189: 0x00100200, 190: 0x00144A38,
    191: 0x0012E6D0, 192: 0x0012DE58, 193: 0x0013E0A0, 194: 0x0013C290, 195: 0x0013C4F0,
}

# Any other module's imports get stand-in addresses, 64 KB apart so that an
# import plus a field offset still names that import: no tool needs the real
# address, only the NAME, and the module's own symbol table carries it.
IMPORT_BASE, IMPORT_STRIDE = 0x01000000, 0x10000
EE_RAM = 32 * 1024 * 1024

# A seeded map names every function it cannot name yet `func_<VA>`; a callee or
# global with such a name is UNNAMED to the checkers, never a mismatch.
_PLACEHOLDER = re.compile(r"^(?:func|data)_[0-9A-F]{8}$")


def is_placeholder(name: str) -> bool:
    return bool(_PLACEHOLDER.match(name))


def import_address(k: int) -> int:
    return IMPORT_BASE + IMPORT_STRIDE * k


def relocate(mod: Module, data: bytes) -> bytearray:
    """A 32 MB EE image holding the module at its base with every relocation
    applied: the same words its loader writes, so the tools that name callees
    and data references need no memory dump."""
    if mod.name == "SUPPORT":
        if hashlib.md5(data).hexdigest() != SUPPORT_IMAGE_MD5:
            raise SystemExit("SUPPORT image is not the build SUPPORT_IMPORTS belongs to")
        resolve = SUPPORT_IMPORTS.__getitem__
    else:
        resolve = import_address
    ram = bytearray(EE_RAM)
    ram[mod.base:mod.base + len(data)] = data
    for site, kind, imp, target in module_relocations(data):
        addr = ((resolve(imp) if imp else mod.base) + target) & 0xFFFFFFFF
        at = mod.base + site
        w = struct.unpack_from("<I", ram, at)[0]
        if kind == R_WORD:
            w = addr
        elif kind == R_JAL:
            w = (w & 0xFC000000) | ((addr >> 2) & 0x03FFFFFF)
        elif kind == R_HI16:
            w = (w & 0xFFFF0000) | (((addr + 0x8000) >> 16) & 0xFFFF)
        elif kind == R_LO16:
            w = (w & 0xFFFF0000) | (addr & 0xFFFF)
        else:
            raise ValueError(f"{mod.name}: relocation type {kind} at +{site:#x}")
        struct.pack_into("<I", ram, at, w)
    return ram


def code_extent(data: bytes) -> tuple[int, int]:
    """(first, end) image offsets of the code: the j/jal/HI16/LO16 sites, which
    can only sit in instructions, bound it (the library tail included)."""
    sites = [s for s, kind, _, _ in module_relocations(data) if kind in (R_JAL, R_HI16, R_LO16)]
    return min(sites), max(sites) + 4


def vu0_census(data: bytes) -> tuple[int, int]:
    """(instructions, VU0 instructions). COP2 / LQC2 / SQC2."""
    tot = vu = 0
    for i in range(0, len(data) - 3, 4):
        x = struct.unpack_from("<I", data, i)[0]
        if x == 0:
            continue
        tot += 1
        if (x >> 26) in (0x12, 0x36, 0x3E):
            vu += 1
    return tot, vu
