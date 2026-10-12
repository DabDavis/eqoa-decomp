# Phase 42 — the 2D UI layer (`VIUI`, `VIUIWindow`, `VIUITextScroll`)

SUPPORT `0x004A4B30 .. 0x004AB818`, 92 symbols laid out contiguously in three
translation units.  This is the layer every menu, panel and HUD element in the
game is drawn through: `VIUI` is the drawing device (three `VIStringTable`s, a
`VIFont`, and pointers to the raster, the sound device and the dictionary),
`VIUIWindow` is the framed panel, and `VIUITextScroll` is a one-row window that
reveals its text a character at a time.

Everything here stands on classes earlier phases settled — `VIStringTable`
(phase 27), `VIFont`/`VIRFont` (phase 30), `VIVect2I`/`VIRect` (phase 9),
`VIColor32F`, `VIString` (phase 10), `VISoundDevice` (phase 39) — so almost
every miss was codegen rather than semantics.

## The class layouts

    class VIUI                       // >= 0x54
    {
        VIStringTable m_words;       // +0x00   WORDS.TXT / words.wad
        VIStringTable m_textures;    // +0x10   data\textures.txt
        VIStringTable m_sounds;      // +0x20   data\UISound.txt
        int           m_language;    // +0x30   the COLUMN of every lookup
        int           m_wordsFromCard; // +0x34
        VIFont        m_font;        // +0x38
        VIRaster      *m_raster;     // +0x48
        VISoundDevice *m_sound;      // +0x4C
        VIDictionary  *m_dict;       // +0x50
    };

    class VIUIWindow                 // 0x17C
    {
        VIUI      *m_ui;             // +0x000
        VIVect2I   m_pos;            // +0x004
        VIVect2I   m_center;         // +0x00C
        VIVect2I   m_size;           // +0x014
        int        m_titleH;         // +0x01C   18 plain, 24 with a title
        int        m_rows;           // +0x020
        int        m_inited;         // +0x024
        float      m_open;           // +0x028   0..1
        float      m_dir;            // +0x02C   +1 opening, -1 closing
        char       m_title[256];     // +0x030
        VIColor32F m_colors[4];      // +0x130   VIUICOLOR indexes THIS
        long       m_pulseTime;      // +0x170
        int        m_x178;           // +0x178
    };

`VIUITextScroll` derives from `VIUIWindow` and adds six ints at +0x180.

The two static lookup tables are unnamed file statics reached only through two
free functions, and their shape is read straight off those functions:
**`GetUITexture` has a bound of 252 and a stride of 36; `GetUISound`'s are 17
and 8.**  Both return entry 0 out of range rather than a null, so every caller's
`if (e == 0) return -1;` is dead code that still has to be written.

    class VIUITexEntry               // 36 bytes
    {
        int              m_x00;      // a SECOND texture id (the fallback art)
        int              m_row;      // row into m_textures
        VIVect2I         m_uv0;      // +0x08
        VIVect2I         m_uv1;      // +0x10
        VIAlphaBlendMode m_mode;     // +0x18
        float            m_alpha;    // +0x1C
        unsigned int     m_flags;    // +0x20
    };

## Levers

**A by-value class parameter that is MODIFIED is a separate local in the
source.**  gcc gives an invisible-reference parameter no home slot of its own:
`DECL_RTL` is the caller's object, so `at.x += d` writes through the incoming
pointer and the by-value argument temporary is then merged with it.  The target
copies the parameter into a local at entry and makes a fresh temporary per call.
Writing `VIVect2I p = at;` and modifying `p` reproduces that exactly —
`BlitAbilityIcon` went 65 wrong words to 25 on that one line, and to **zero**
once the copy was moved AHEAD of the first `GetUITexture` call, which is where
the target's entry copy sits.

**A named local for the size in one arm, an inline temporary in the other.**
`BlitHorz`/`BlitVert` construct the same `VIVect2I(w, e->m_uv1.y)` in both arms
of one `if`, and the else arm writes it TWICE — once into a slot the other arm
uses for something else, once into the argument temporary.  That second write is
a named local; the if arm has only one write, so its size is an unnamed
`VIVect2I(...)` in the call.  One line each and both went byte-exact, 58 wrong
to 0.  The rule generalises: **count the writes of a constructed value.  Two
slots means a named local, one means a temporary in the argument list.**

**`VIVect2I` needs a zeroing default constructor AND a user copy
constructor.**  Phase 9 recorded that it needed neither.  Both are visible only
here: the zeroing ctor because `VIVect2I r[2];` emits gcc's array-construction
loop (a countdown to -1 with an up-walking pointer — the same shape as
`VIUIWindow`'s `VIColor32F m_colors[4]`), and the copy ctor because
`VIUI::Blit` copies an atlas rectangle out of the entry and then copies it AGAIN
into the by-value argument's temporary, six copies that are all `lw`/`sw` pairs
in the target and unaligned `ldl`/`ldr` block moves without it.  Neither change
moves a single word in `vibrect.cpp`, `virfont.cpp` or `vifont.cpp` — A/B'd both
ways — because every `VIVect2I` local there is fully assigned before use.

**The statement order inside a loop tail.**  `BlitRect`'s outer loop ends with
three assignments; four orderings scored 20, 39, 26 and **0**.  The winning one
is `p.x += uv.x; d.y = ...; p.y = at.y;` — the induction step FIRST.  Nothing in
the diff points at it; only the sweep does.

**The ternary inside the call, not a named float.**  `TickR`/`TickU`/`TickD`
choose an alpha between `TickPulse()` and `1.0f`.  Written as `float a; if
(pulse) a = TickPulse(); else a = 1.0f;` the argument temporary for the
`VIVect2I` is built AFTER the branch; written inline as `pulse ? TickPulse() :
1.0f` in the call it is built before, which is what parks its address in a
callee-saved register the way the target does.  17 wrong words each, three
functions, one edit.

**Phase 24's offset-first element address, in a plain pointer.**
`AppendEllipsis`'s `s + n` comes out base-first where the target is index-first;
`(unsigned short *)(n * 2 + (unsigned int)s)` is the spelling.

**`if (n <= max) max = n;` is not `max = VIMIN(max, n);`.**  Both are the same
`slt`, but the assignment form keeps `max`'s register as the fall-through and
gives `movz`, where the comparison form gcc canonicalises into
`if (max < n) n = max;` and gives `movn` on the OTHER register.  `CopyAsteriks`.

**Phase 9's assign-then-modify-in-place, on a float chain.**  `VIUIWindow::Text`
fades a colour by `1 - (1-open)^2`; written as one expression the FP registers
come out permuted, and split into `float d = 1.0f - m_open; d = 1.0f - d * d;
col.a = col.a * d;` it drops from 9 wrong words to 3.

**Phase 14's fresh local for the intermediate, again.**  `RowCenter`'s
`+ ((h - 4) * row + 5)` is reassociated by gcc unless the product and the 5 are
bound to a local first: 17 wrong to 0.

**Phase 21's `lui` order.**  `VISetWindowUI` writes two file statics; the store
order is `mUI` then `mFocus` and the `lui` order is the reverse, and the `lui`s
are the source order.

**Phase 16's constant hoist, read forward.**  `VIUITextScroll`'s constructor
sets six ints; the emitted order is 396, 404, 384, 388, 392, 400, which is plain
DECLARATION order with the last store of 30 and the last store of 1 hoisted and
the zeros not hoisted at all — **a zero store needs no register, so it is never
hoisted**, which is the missing half of the rule.

## Shipped

* **`VIUIWindow::Text` computes two values and throws both away.**  It calls
  `RowCenter(y)` and adjusts the result's `.x` by the window's left edge, then
  passes the ORIGINAL `x` to `TextBox`; and it copies the caller's colour,
  scales its alpha by the open-animation curve, and then passes the CALLER's
  colour.  So neither the row centering nor the fade-in reaches the text.
* **The second `VIUIWindow::Init` calls `VIFont::GetHeight` and drops the
  answer** — the four-vector form takes its size from its argument, so the
  measurement is pure cost.
* **`VIUI::Init` returns whatever `VIStringTable::Load` returned**, not 0/-1,
  and resets `m_language` to 0 only when the sound-name table loaded cleanly.
* **`VIUI::SoundPlay(const char *, float)` returns 0 on every path**, including
  the two failures (resource not found, `Play` refused).
* **`VIUI::Blit`'s null test never fires** — `GetUITexture`/`GetUISound` clamp
  out-of-range ids to entry 0 instead of returning 0, in all fourteen callers.
* **`VIUIWindow::AppendEllipsis` measures `"..."` with the FONT and the text
  with `TextWidth`**, which strips the `|`-escapes; a string made entirely of
  escapes therefore never shrinks and the loop walks the terminator backwards
  through the whole buffer.
* **`VIUIWindow::CopyAsteriks` ignores `this`.**

## Open

* The six `DrawStringEx` / `DrawCenteredStringEx` overloads are **14 words each
  and one instruction apart**: gcc's `cse` merges the address of the saved font
  colour (`sp+16`, formed by `move_by_pieces`' `copy_addr_to_reg` at the block
  copy) with the same address at the final `SetTextColor`, and the merged pseudo
  is then live across the whole function and gets a callee-saved register, where
  the target rematerialises it with `addiu a1,sp,16`.  Confirmed against the RTL
  dumps (`eqrtl.py`: two `plus:SI (reg 112) (const_int 16)` in `.rtl`, one after
  `.cse`); **neither `-fno-gcse` nor `-fno-cse-follow-jumps` changes it**, and
  five source spellings of the save (assignment, a `VIFont` copy, an explicit
  copy-construction, a function-scope shadow colour) all score the same.  This is
  phase 19's standing phenomenon with the pass named.
* `Blit(VIUITexId, VIVect2I)` 8/60 and `BlitEx(VIUITexId, VIVect2I, ...)` 8/67:
  the target copies each atlas rectangle load-load-store-store and the second one
  y-before-x; ours interleaves load-store-load-store.  Same instructions.
* `VIUIWindow::Init(…,int,int,int)` 5/51 — `s4`/`s5` swap between two parameters;
  three statement orderings score identically.
* `VIUI::FrameFillRectangle` 7/74 and `FrameRectangle` 21/85: the two-corner copy
  is emitted in a different interleaving.
* `VIUIWindow::Text` 3/59, `Blit(VIUITexId, VIVect2I, VIVect2I)` 3/69 — three
  words of scheduling each.

## Not attempted

`VIUI`: the four frame builders (`BlitFrame` 1056, `BlitStuddedFrame` 1152,
`BlitFillStuddedFrame` 680, `BlitFillFrame` 520 — `VIUIFrameParam` is **64
bytes, eight `{VIUITexId id; unsigned int flags}` pairs**, read off which field
pair reaches `BlitEx`/`BlitHorz`/`BlitVert`), the six `BlitBar` overloads and
`ReloadWords` (560).  `VIUIWindow`: `DrawTextInternal` (2416), `TextBox` (1872),
`Draw` (1720), `DrawFrame` (688), `DrawProgress` (624), `DrawHScroll` (536),
`DrawFrameVScroll` (400), `DrawColorBar` (304), `TextWidth` (272).
`VIUITextScroll::TextScroll` (1048).
