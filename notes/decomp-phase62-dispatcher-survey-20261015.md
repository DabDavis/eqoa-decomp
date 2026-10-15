# Phase 62 survey: CLIENT's network opcode dispatcher

> **Provenance (2026-10-15).** A read-only survey made by a subagent at the start of phase 62,
> kept verbatim as the dispatcher's map. Where it says "another lane" or "the parallel lane"
> is working on CLIENT, that was the same session writing `client_main.cpp`/`viclient.cpp`;
> its §6 correction (VIClient.cpp runs to 0x00BE3C0F) has been applied. What the
> dispatcher work later established beyond it is in
> `decomp-phase62-client-20261015.md`.


Read-only survey, 2026-10-15. Everything below was measured on
`CLIENT_decompressed_SNR1.bin` at base `0x00BB4980`, relocated from its own relocation
table (`eqdecomp.relocate`), with `func_<VA>` placeholder names from
`decomp/modules/client/symbols.txt`. Nothing was compiled or scored.

**Another lane is working on CLIENT right now.** `modules/client/symbols.txt`,
`src/game.h` and `src/viclient.cpp` were edited at 09:15–09:20 today. That lane has named
`__8VIClient` (0x00BBF698), `Init`, `Shutdown`, `Disconnect__8VIClientii` (0x00BC1720,
matched), `Notify`, `_$_8VIClient` (0x00BC2638), `IsRunning`, `Process`, `Reset`,
`StopMusic`, `FreeBuffer` and `client_main`. This survey uses those names where they
exist. **Coordinate before touching `game.h` / `viclient.cpp` / `symbols.txt`.** See §6
for one correction to that lane's header comment: the TU extent.

## 0. Summary

| | |
|---|---|
| Dispatcher | `func_00BD0DC8`, VIClient.cpp (its one assert, `!HaveCorpse`, is on line **10306**) |
| Size | span 8,376 B = 2,094 words = **2,093 instructions + 1 pad nop** |
| Shape | prologue 24 · compare tree **468** (0x00BD0E28–0x00BD1574) · case bodies **1,586** (0x00BD1578–0x00BD2E3C) · shared exit/epilogue 15 (0x00BD2E40–0x00BD2E78) |
| Dispatch | `switch (unsigned short opcode)` compiled as gcc's **balanced compare tree** (no top-level jump table) |
| Opcodes | **122 opcode values → 114 case bodies**; everything else goes to the exit (no-op default) |
| Nested switches | 3 jump tables inside case bodies (`0x069C`, `0x069A`, `0x0056`) + one small compare-tree switch (`0x0085`) |
| Direct CLIENT callees | 123 distinct (4 are VIMemStream reads, 1 is a copy of `VIStringTable::GetString`) |
| Handler code | case bodies 6,344 B · 118 direct handler functions **24,048 B** · whole CLIENT call closure 893 functions / 157,440 B |

## 1. The dispatcher, `func_00BD0DC8`

### 1.1 Signature, frame, prologue

```
caller   func_00BD4C10 (DRDP receive loop), the DRDP message-type 0xFB arm:
           func_00BD0DC8(this, message_get_src(m), message_get_data(m), message_get_size(m))
a0 this  -> s4 for the whole function (VIClient *; the FrontiersForge singleton 0x1FAF730)
a1 src   -> uns64 from message_get_src, passed UNconverted by the caller; the callee does
            dsll32/dsra32 s0,a1 on entry = (int)src. Only case 0x000D reads it
            (func_00BD0630 stores it with a 32-bit sw).
a2 data  -> stream.m_rbuf (sw a2,4(sp))
a3 size  -> stream.m_capacity and stream.m_size (sw a3,8(sp) / 12(sp))
return   void (v0 is never set; the exit only tests it for the inline destructor)
frame    addiu sp,-2400; sq s4,s0,ra,s5,s3,s2,s1 at sp+2352,2288,2384,2368,2336,2320,2304
```

Likely declaration: `void VIClient::<name>(uns64 src, void *data, int size)` (a member
of `VIClient`; `uns64` is `unsigned long`, 8 bytes on this ABI). No name for it is
recorded anywhere; the old map calls it `VIClient.cpp:10306` and the notes call it the
"mega-function".

Body skeleton, read off the code:

```cpp
VIMemStream stream(data, size);   // sp+0..31, inline ctor {0, data, size, size, 0,0,0,0}
unsigned short opcode;            // sp+32 (a promoted local: stack slot when &opcode taken)
stream.Read(&opcode, 2, 1);       // jal func_00CB8520; then lhu a1,32(sp)
switch (opcode) { ... 114 bodies ... }   // default: nothing
                                  // exit 0x00BD2E40: inline ~VIMemStream:
                                  //   if (m_owned && m_wbuf) free(m_wbuf)
```

Every case ends in `b 0x00BD2E40` with `lw v0,28(sp)` (the dtor's `m_owned` load) in the
delay slot. Early `break`s from inside a case go to `0x00BD2E3C`, the copy of that load.

### 1.2 How the opcode is dispatched

* One `switch` on a `u16`, lowered by gcc 2.95 `expand_end_case` into a balanced binary
  tree: `li v0,K; beq a1,v0,case; slti v0,a1,K; beqz v0,right-subtree`. When a subtree's
  range pins a single value the final `beq` is dropped.
* **reorg moved a case's first instruction(s) into a tree delay slot for 9 cases**, so
  their bodies start by using a register they never set: `0x003C` (`move a0,sp` at
  0x00BD0FB4 + `addiu a1,sp,2115` at 0x00BD0FBC), `0x003E` (the same `move a0,sp`),
  `0x00B2` (`move a0,s4` at 0x00BD10F8), `0x00B6` and `0x007E` (`lui a1,0x2` in the
  `bnezl` slots at 0x00BD1134 / 0x00BD10D0), `0x03D7` (`move a0,s4`, 0x00BD1260),
  `0x072A` (`lui a0,%hi("Disconnect-8")`, 0x00BD1328), `0x0770` (`addiu s0,sp,48` at
  0x00BD1350 + `move a0,sp` at 0x00BD1358), `0x0774` (`move a0,s4`, 0x00BD137C). Several
  sit in a non-likely slot and so also run on the sibling arm, where they are dead. In
  the source these are ordinary first statements. `0x1003/04/05` is different: it uses
  `a1` because the tree left the opcode there.
* **Case-body order is source order** (gcc reorders the dispatch code in front of the
  bodies). The table in §2 is in that order, and so must the C++ be.
* Opcodes sharing one body: `0x07F2/0x07F4`, `0x07F0/0x07F1/0x07F3`,
  `0x1250/0x1251/0x1253`, `0x00EC/0x07A4`, `0x1003/0x1004/0x1005`.
* No `beq a1,K` in the tree targets the exit, so there are no empty `case K: break;`
  labels; the only exit branches are the tree's gap/default edges (e.g. 0x00BD11D8,
  0x00BD1394). The dense-looking range checks (e.g. `slti v0,a1,984`) are tree nodes,
  not jump tables.

Reproduce the map: simulate the tree (`li/addiu/ori/slti/sltiu` on `v0`, `beq/bne` and
their `-l` forms, `lw v0,28(sp)` as a no-op) from `0x00BD0E2C` for all 65,536 values of
`a1`. The first non-tree instruction reached is the case label; when a delay slot holds a
body instruction, the label is the branch target (minus 4 when that word is the copy).

### 1.3 Nested jump tables (read from the relocated data)

| Table | Entries | Case | Index | Targets (index+1 → label) |
|---|---|---|---|---|
| `0x00CDAAB0` | 5 | `0x069C` | `u8 sub - 1 < 5` | 1→0x00BD16D8, 2→0x00BD1700, 3→0x00BD16C4, 4→0x00BD1728, 5→0x00BD1750; ≥6→break. Body order 3,1,2,4,5 |
| `0x00CDAAD0` | 8 | `0x069A` | `u8 sub - 1 < 8` | 1..5→0x00BD1828/34/40/4C/58, 6→tail, 7→0x00BD18AC, 8→tail; else tail. Body order 1,2,3,4,5,7 |
| `0x00CDAAF0` | 22 | `0x0056` | `u8 sub - 1 < 22` | 1→0x00BD294C, 2→0x00BD2938, 6→0x00BD29BC, 7→0x00BD29AC, 16→0x00BD296C, 17→0x00BD297C, 20→0x00BD295C, 21→0x00BD298C, 22→0x00BD299C, all others→0x00BD29CC (default). Body order 2,1,20,16,17,21,22,7,6,default |

The receive loop's own table, `0x00CDAB90` (message type − 248, 8 entries), is in §3.2.

### 1.4 Cross-jumped shared code (gcc merged identical tails)

| Shared code | Owner (source copy kept) | Joined by |
|---|---|---|
| `0x00BD2E14–0x00BD2E28`: `stream.Read(&field, 4, 1); break;` | `0x07D1` (`&m_50304`) | `0x001C` (`&m_C320`), `0x00D9` (`&m_C308`), `0x07B2` (`&m_7578`) at 0x00BD2E1C; `0x007E` (`&m_2BE50`) at 0x00BD2E14 |
| `0x00BD27A4–0x00BD27B0`: `Disconnect(reason, 0); break;` | `0x00EC/0x07A4` (reason 8) | `0x0000` (16), `0x0729` (3), `0x072B` (4), `0x0728` (6), `0x072A` (7), `0x07C0` (17), `0x07C1` (18). Not `0x07D2` (19): the last case falls into the exit, so its tail differs |
| `0x00BD1BB0–0x00BD1BD0`: `func_00C215F8(m_2BB84, GetString(id)); m_2BE6C--;` | `0x002F` (829) | `0x0065` (828) at 0x00BD1BB0; `0x002E` joins only the `m_2BE6C--` at 0x00BD1BC8 |
| `0x00BD18D4–0x00BD192C`: `ShowMsg(id);` + group-reset tail | `0x062F` (634) | `0x069A`'s sub-cases 1–4 (630–633) at 0x00BD18D4 (the `jal` itself is shared); 6, 8 and default at 0x00BD18DC; 5 and 7 at 0x00BD18E0 (7 keeps its own `ShowMsg(631)` call) |

Write each case with its own complete tail; cross-jumping will merge them. The owner is
always the *last* copy in source order.

### 1.5 Stack layout (2,400-byte frame) — read it as the declaration list

The rule (memory `feedback_phase61_esf_levers.md`): a promoted local (`char`, `short`)
gets a slot the moment its address is taken; an `int` is ADDRESSOF'd and lands at the end
of the frame in first-use order; block-scope aggregates share slots through
`assign_stack_temp`. The dispatcher follows it exactly:

| sp offset | What | Notes |
|---|---|---|
| +0 .. +31 | `VIMemStream stream` | function scope, first |
| +32 | `unsigned short opcode` | |
| +48 .. +2111 | **one region reused by every case's block-scope aggregate** | 0x069E/0x00CE `unsigned short buf[1024]` (2,048 B; `VIColor32F` follows at +2096); 0x0046 `unsigned short[512]`; 0x0758 `char[512]`; 0x069C `unsigned short[128]` + `char name[24]` at +304 (so `snprintf16`'s 256 is a byte count); 0x069A `unsigned short[128]`; 0x003B item object (VIArray member at +780); 0x03D5 172-B buddy record (+ u8 at +224); 0x00B1 `int arr[]`; 0x0770/0x0771/0x0012/0x07A3 `char[24]`; 0x00D3 `char[64]`; 0x007C `unsigned short[128]`; 0x077A `VIColor32F` |
| +2112 .. +2142 | every `unsigned char` local, **in body order**, never reused | 2112 `0x069C`, 2113 `0x069A`, 2114 `0x0775`, 2115/16 `0x003C`, 2117/18 `0x003D`, 2119/20 `0x003E`, 2121/22 `0x003F`, 2123 `0x0040`, 2124 `0x0019`, 2125 `0x0085`, 2126 `0x00B1`, 2127/28 `0x00B4`, 2129/30 `0x00B5`, 2131/32 `0x00B6`, 2133 `0x00CD`, 2134 `0x00FC`, 2135 `0x0771`, 2136 `0x0036`, 2137/38 `0x000A`, 2139 `0x0056`, 2140 `0x00D3`, 2141 `0x00C5`, 2142 `0x00C7` |
| +2144 .. +2279 | one `int` per use, **first-use order** | 2144 `0x069E`, 2148 `0x0775`, 2152 `0x0046`, 2156 `0x0000`, 2160 `0x003C`, 2164 `0x0019`, 2168/72/76/80 `0x00B1`, 2184 `0x00B8`, 2188 `0x00CD`, 2192 `0x03D6`, 2196 `0x001D`, 2200/04/08 `0x0776`, 2212/16 `0x0057`, 2220 `0x0056`, 2224/28 `0x0060`, 2232 `0x0061`, **2240/2248 two 8-byte `long`s** `0x0013`, 2256 `0x0045`, 2260 `0x007B`, 2264 `0x007D`, 2268/72/76 `0x00DB` |
| +2288 .. +2399 | `sq` saves s0, s1, s2, s3, s4, s5, ra | |

So every case's scalars are in-block locals whose address goes to `stream.Read`, and
each case with a buffer or object needs its own `{ }`.

## 2. Per-opcode table (source order)

Field names are `m_<hex offset>` into `VIClient`. `Read(x,n,1)` is `func_00CB8520`,
`ReadSigned` the zigzag read `func_00CB8610`, `ReadString`/`ReadString16` are
`func_00CB8740`/`func_00CB87E8` (§3.3). `ShowMsg(id)` is `func_00BD90F8`,
`ShowText(s)` is `func_00BD9138`, `GetString(id)` is `func_00C0B970` on `m_ui.m_words`
with `m_ui.m_language`. "Handler" lists the CLIENT functions the case calls besides
those; *own* is the number of distinct CLIENT placeholders that handler calls, and the
names after it are its SUPPORT/SLUS imports. **Bold** in the last column marks a note
claim the image contradicts.

| # | Opcode(s) | Label | Insns | Case body | Handler(s): bytes, *own*, imports | Notes / flags |
|---|---|---|---|---|---|---|
| 1 | `000D` | 0x00BD1578 | 6 | → func_00BD0630(this, (int)src, &stream). The only use of `src` | `func_00BD0630` 1740 B, *33*, EndManual, Reset, SetLocation, SetOrbitAngleBehind, SetSky, StopNow, StopSoundTrack, VIDisplayScreen16, VIGetGameOptions, VIHasDiscIdChanged, _VIAssert, connection_send, memset | DSPDataSection; MemoryDump — **notes place it at 0x00BD21EC (that is 0x00B7)** |
| 2 | `0A7B` | 0x00BD1590 | 5 | → func_00BD3600(this, &stream) | `func_00BD3600` 204 B, *3* | ColoredChat |
| 3 | `0A7A` | 0x00BD15A4 | 5 | → func_00BD36D0(this, &stream) | `func_00BD36D0` 104 B, *2* | ClientMessage |
| 4 | `069E` | 0x00BD15B8 | 28 | Read i32 (never used); ReadString16(buf[1024], 2048); func_00C56BE0(m_2BB78, buf, &VIColor32F(.5,.5,1,1)) | `func_00C56BE0` 64 B, *1* | GroupChat |
| 5 | `00CE` | 0x00BD1628 | 23 | ReadString16(buf[1024], 2048); func_00C56BE0(m_2BB78, buf, &VIColor32F(1,.25,.25,1)) | `func_00C56BE0` 64 B, *1* | ErrorMessage |
| 6 | `069C` | 0x00BD1684 | 72 | Read u8 sub; switch (table 0x00CDAAB0, body order 3,1,2,4,5): 3 → ShowMsg(627); 1/2/4/5 → ReadString(name[24]); snprintf16(buf, 256, GetString(558/628/642/1022), name); ShowText(buf) | `func_00BD90F8` 60 B, *2*<br>`func_00BD9138` 40 B, *1* | GroupStuff |
| 7 | `0772` | 0x00BD17A4 | 5 | → func_00BD31C0(this, &stream) | `func_00BD31C0` 664 B, *4* |  |
| 8 | `0773` | 0x00BD17B8 | 5 | → func_00BD3458(this, &stream) | `func_00BD3458` 236 B, *7*, strcpy8 | GroupInviteResponse |
| 9 | `0774` | 0x00BD17CC | 4 | → func_00BD3548(this, &stream) | `func_00BD3548` 184 B, *6* |  |
| 10 | `069A` | 0x00BD17DC | 57 | if (!func_00BD7A68()) break; Read u8 sub; switch (table 0x00CDAAD0): 1–4 ShowMsg(630–633); 5 snprintf16(buf, 256, "%S  %S", GetString(629), GetString(827)) (unused); 7 ShowMsg(631); then the group-reset tail (cross-jumped into 0x062F) | `func_00BD7A68` 20 B, *0*<br>`func_00BD90F8` 60 B, *2*<br>`func_00BD9B48` 204 B, *2* | RemoveGroupMember |
| 11 | `062F` | 0x00BD18C0 | 28 | if (!func_00BD7A68()) break; ShowMsg(634); tail: m_2BD70 = -1; m_2BE4C = m_2BD74 = m_2BD78 = 0; if (u8 m_382E4 == 2) func_00BD9B48(this, 0) | `func_00BD7A68` 20 B, *0*<br>`func_00BD90F8` 60 B, *2*<br>`func_00BD9B48` 204 B, *2* | ServerDisbandGroup |
| 12 | `0775` | 0x00BD1930 | 42 | Read i32 id, u8 action; e = FindEntity(id); if (!e) break; if (id == m_71F8 && action != 34) break; if (m_scene.GetActorSpriteType(e->m_24) != 5) break; ((VICSprite *)m_scene.Sprite(m_scene.GetActorSprite(e->m_24)))->SetAction(action, m_scene) | `func_00BC9870` 56 B, *0* | ResourceUpdate |
| 13 | `0020` | 0x00BD19D8 | 19 | old = m_C300; ReadSigned(&m_C300); ReadSigned(&m_C304); if (old < m_C300) func_00BD8E78(this, 16) (m_ui.SoundPlay) | `func_00BD8E78` 28 B, *0*, SoundPlay | GrantXP — **notes: HP/MaxHP; +0xC300 is player level** |
| 14 | `001C` | 0x00BD1A24 | 5 | Read(&m_C320, 4, 1) (shared Read tail) | inline | NPCInteraction |
| 15 | `0034` | 0x00BD1A38 | 5 | → func_00BDBD58(this, &stream) | `func_00BDBD58` 804 B, *7*, memset | QuestPopupMulti — fall-through claim refuted |
| 16 | `0046` | 0x00BD1A4C | 18 | Read i32 id; ReadString16(buf, 1024); func_00BDBC70(this, id, buf, 1, 1) | `func_00BDBC70` 232 B, *3*, strcpy16 | DialogueBox — one notes table gives 0x00BD1A94 (that is 0x0758) |
| 17 | `0758` | 0x00BD1A94 | 13 | ReadString(buf[512], 512); func_00C56BA0(m_2BB78, buf) | `func_00C56BA0` 28 B, *1* | ReconnectWorldEntry — **notes: reconnect token that falls into 0x0728; it prints an 8-bit string to the chat window** |
| 18 | `0000` | 0x00BD1AC8 | 14 | Read i32 ver; if (ver != 18) { printf("Disconnect-??\n"); Disconnect(16, 0); } | `Disconnect__8VIClientii` 412 B (**matched** by the other lane) | DiscVersion |
| 19 | `0729` | 0x00BD1B00 | 6 | printf("Disconnect-5\n"); Disconnect(3, 0) | `Disconnect__8VIClientii` 412 B (**matched** by the other lane) | BadLoginPassword — fall-through claim refuted |
| 20 | `072B` | 0x00BD1B18 | 6 | printf("Disconnect-6\n"); Disconnect(4, 0) | `Disconnect__8VIClientii` 412 B (**matched** by the other lane) | NotSubscribed — fall-through claim refuted |
| 21 | `0728` | 0x00BD1B30 | 6 | printf("Disconnect-7\n"); Disconnect(6, 0) | `Disconnect__8VIClientii` 412 B (**matched** by the other lane) | ServerLoginResponse — **notes: login response that "falls through 1235 insns"; it is Disconnect(6)** |
| 22 | `072A` | 0x00BD1B48 | 5 | printf("Disconnect-8\n"); Disconnect(7, 0) | `Disconnect__8VIClientii` 412 B (**matched** by the other lane) |  |
| 23 | `002E` | 0x00BD1B5C | 8 | func_00C21668(m_2BB84, 269) (window text = word 269); m_2BE6C-- | `func_00C21668` 64 B, *2* | WorldEntry — **notes: world-entry ack "sends 0x010D"; it shows word 269** |
| 24 | `0065` | 0x00BD1B7C | 7 | func_00C215F8(m_2BB84, GetString(828)); m_2BE6C-- (shared tail) | `func_00C215F8` 108 B, *1*, strcpy16, strlen16 | ItemListUpdate — **notes: "the largest handler", falls through four others; it is 7 insns: word 828 + count--** |
| 25 | `002F` | 0x00BD1B98 | 15 | func_00C215F8(m_2BB84, GetString(829)); m_2BE6C-- | `func_00C215F8` 108 B, *1*, strcpy16, strlen16 | NameTaken; NameTaken (variant A) — one notes table gives 0x00BD1B7C (that is 0x0065) |
| 26 | `002C` | 0x00BD1BD4 | 16 | if ((unsigned)(m_58 - 2) < 2) { m_58 = 3; m_4F0E8 = 21; func_00BD2E80(this, &stream) (character list) } | `func_00BD2E80` 612 B, *6* | CharacterSelect |
| 27 | `0777` | 0x00BD1C14 | 4 | → func_00BD89F0(this) | `func_00BD89F0` 112 B, *2*, printf | DisconnectNoError — fall-through claim refuted |
| 28 | `003B` | 0x00BD1C24 | 14 | item at sp+48 (its VIArray member at +0x2DC: ctor func_00BBB758, dtor func_00BBB7B8(…, 2) inline); func_00CBB040(&stream, &item); func_00BCB0D8(this, &item) | `func_00BBB758` 16 B, *0*<br>`func_00CBB040` 136 B, *3*<br>`func_00BCB0D8` 92 B, *3*<br>`func_00BBB7B8` 100 B, *3* | AddInvItem |
| 29 | `003C` | 0x00BD1C5C | 18 | Read u8 a, u8 b; ReadSigned c; func_00BCB138(this, a, c, b) | `func_00BCB138` 208 B, *6* |  |
| 30 | `003D` | 0x00BD1CA4 | 16 | Read u8 a, b; func_00BCB208(this, a, b) | `func_00BCB208` 32 B, *1* | ArrangeItem |
| 31 | `003E` | 0x00BD1CE4 | 15 | Read u8 a, b; func_00BCB228(this, a, b) | `func_00BCB228` 200 B, *6* |  |
| 32 | `003F` | 0x00BD1D20 | 16 | Read u8 a, b; func_00BCB2F0(this, a, b) | `func_00BCB2F0` 800 B, *5* | EquipItem |
| 33 | `0040` | 0x00BD1D60 | 10 | Read u8 a; func_00BCB8A0(this, a) | `func_00BCB8A0` 232 B, *6* | UnequipItem |
| 34 | `0019` | 0x00BD1D88 | 14 | Read u8 a; ReadSigned b; func_00BD99A0(this, a, b) | `func_00BD99A0` 164 B, *5* | ClientLoot |
| 35 | `001A` | 0x00BD1DC0 | 4 | → func_00BD98B0(this) | `func_00BD98B0` 236 B, *5* | LootResult |
| 36 | `0018` | 0x00BD1DD0 | 32 | VIASSERT(!HaveCorpse) **line 10306** (u8 m_13B94); func_00BBC8D0(&m_13B98, &stream); HaveCorpse = 1; if (func_00C56CA8(m_2BB78)) func_00C556D8(m_2BB78); else func_00BCB098(this) | `func_00BBC8D0` 140 B, *3*<br>`func_00C56CA8` 12 B, *0*<br>`func_00C556D8` 56 B, *2*<br>`func_00BCB098` 28 B, *1* | Loot |
| 37 | `0016` | 0x00BD1E50 | 14 | func_00C55710(m_2BB78); HaveCorpse = 0; m_13B98 words 1, 0 = 0 | `func_00C55710` 48 B, *1* | ClientCloseLoot |
| 38 | `0085` | 0x00BD1E88 | 34 | Read u8 opt; switch (compare tree; body order 1,2,3): func_00C56C20(m_2BB78, 673 / 674 / 675); other → nothing | `func_00C56C20` 32 B, *1* | LootOptions |
| 39 | `00B1` | 0x00BD1F10 | 52 | Read i32 a, b; Read u8 n; for (i < n) Read(&arr[i], 4, 1) (arr at sp+48); Read i32 c, d; func_00BD3738(this, a, b, n, arr, c, d) | `func_00BD3738` 400 B, *1*, FindTyped, Play | CastingCombat |
| 40 | `00B3` | 0x00BD1FE0 | 5 | → func_00BCF3A0(this, &stream) | `func_00BCF3A0` 196 B, *7* | FullEntityCreate |
| 41 | `00B4` | 0x00BD1FF4 | 23 | Read(&m_2BB60, 4, 1); Read u8 a, b; func_00BCF0F8(this, a, b) | `func_00BCF0F8` 468 B, *6*, snprintf16 | EntityAppearanceUpdate |
| 42 | `00FF` | 0x00BD2050 | 5 | → func_00BCF2D0(this, &stream) | `func_00BCF2D0` 204 B, *4* | FullEntityRefresh |
| 43 | `00B5` | 0x00BD2064 | 23 | Read(&m_2BB60, 4, 1); Read u8 a, b; func_00BCF5A8(this, a, b) | `func_00BCF5A8` 392 B, *3* | EntityUpdate |
| 44 | `00B6` | 0x00BD20C0 | 22 | Read(&m_2BB60, 4, 1); Read u8 a, b; func_00BCF730(this, a, b) | `func_00BCF730` 384 B, *3* | EntityUpdateB |
| 45 | `00FD` | 0x00BD2118 | 5 | → func_00BCF8B0(this, &stream) | `func_00BCF8B0` 216 B, *3* | EntityDataUpdate |
| 46 | `00B8` | 0x00BD212C | 15 | Read(&m_2BB60, 4, 1); ReadSigned z; func_00BCF468(this, z) | `func_00BCF468` 152 B, *4* | EntityDetailUpdate |
| 47 | `00CD` | 0x00BD2168 | 21 | Read u8 a; Read(&m_2BB60, 4, 1); ReadSigned z; func_00BCF500(this, a, z) (FrontiersForge: the ability-lockout call) | `func_00BCF500` 164 B, *3* | CooldownTimer |
| 48 | `00B2` | 0x00BD21BC | 4 | → func_00BD38C8(this, &stream) | `func_00BD38C8` 404 B, *10*, EndManual, Reset |  |
| 49 | `00FC` | 0x00BD21CC | 8 | Read u8 v; m_6DC8 = v (an int) | inline | EntityStateRefresh |
| 50 | `00B7` | 0x00BD21EC | 37 | memset(m_22A78, 0, 256); Read(&m_1B300, 4, 1); ReadSigned(&m_1B308); ReadSigned(&m_1B30C); func_00BBC8D0(&m_1B310, &stream); ReadString16(m_22A78, 256); func_00BD9A48(this) | `func_00BBC8D0` 140 B, *3*<br>`func_00BD9A48` 92 B, *2* | MerchantBox — notes say it shares 0x000D's code; it does not |
| 51 | `03D5` | 0x00BD2280 | 83 | record r at sp+48 (172 B; func_00CB7788 clears it); func_00CB7850(&stream, &r); Read u8 f; r.m_28 = (f != 0); ReadString16(r.m_2C, 128); i = m_2EC70.Find(r.key); if (i == End()) Insert(r.key, r); else Value(i) = r (inline 172-B copy) | `func_00CB7788` 72 B, *0*, memset<br>`func_00CB7850` 148 B, *2*<br>`func_00BE1F68` 104 B, *1*<br>`func_00BDF388` 8 B, *0*<br>+1 more (§4) | BuddyListEntry |
| 52 | `03D6` | 0x00BD23CC | 17 | ReadSigned id; i = m_2EC70.Find(id); func_00BE2C10(&m_2EC70, i) (erase); also stores &id to sp+48 and sp+52, never read | `func_00BE1F68` 104 B, *1*<br>`func_00BE2C10` 468 B, *3* | BuddyOnlineStatus |
| 53 | `03D7` | 0x00BD2410 | 4 | ShowMsg(590) | `func_00BD90F8` 60 B, *2* |  |
| 54 | `03D8` | 0x00BD2420 | 5 | ShowMsg(591) | `func_00BD90F8` 60 B, *2* | AddBuddyFailed |
| 55 | `076F` | 0x00BD2434 | 5 | → func_00BDB3E8(this, &stream) | `func_00BDB3E8` 616 B, *9*, free, snprintf16 | BuddyInvite |
| 56 | `0770` | 0x00BD2448 | 8 | ReadString(name[24]); func_00BDB650(this, name) | `func_00BDB650` 104 B, *2*, snprintf16 |  |
| 57 | `0771` | 0x00BD2468 | 16 | Read u8 a; ReadString(name[24]); func_00BDB6B8(this, a, name) | `func_00BDB6B8` 208 B, *2*, snprintf16 | BuddyListStuff |
| 58 | `0468` | 0x00BD24A8 | 7 | → func_00BF0D38(&m_2EC84, &stream) (vtable slot 4) | `func_00BF0D38` 400 B, *8* | GuildInfo |
| 59 | `0700` | 0x00BD24C4 | 7 | → func_00BB5DD0(&m_382E8, &stream) (slot 10) | `func_00BB5DD0` 128 B, *3* | AdminCodeResponse — fall-through claim refuted |
| 60 | `07E2` | 0x00BD24E0 | 7 | → func_00BEA820(&m_4F078, &stream) (slot 15) | `func_00BEA820` 92 B, *2* | FactionPage — fall-through claim refuted |
| 61 | `07E0` | 0x00BD24FC | 5 | → func_00BDC308(this, &stream) | `func_00BDC308` 300 B, *8*, free | ServerReady |
| 62 | `057A` | 0x00BD2510 | 7 | → func_00BEF6D8(&m_2EF24, &stream) (slot 5) | `func_00BEF6D8` 176 B, *4* | GuildInviteStuff |
| 63 | `0579` | 0x00BD252C | 7 | → func_00BEF788(&m_2EF24, &stream) (slot 5) | `func_00BEF788` 532 B, *4*, snprintf16 | GuildWorldEntry |
| 64 | `0D41` | 0x00BD2548 | 7 | → func_00BF5670(&m_2EFD0, &stream) (slot 7) | `func_00BF5670` 196 B, *5* | MailDelivery — fall-through claim refuted |
| 65 | `0E02` | 0x00BD2564 | 7 | → func_00C03168(&m_38334, &stream) (slot 11) | `func_00C03168` 92 B, *2* | WhoListResponse — fall-through claim refuted |
| 66 | `0F02` | 0x00BD2580 | 7 | → func_00BEB900(&m_3FC10, &stream) (slot 13) | `func_00BEB900` 268 B, *11* | WhoListFiltered — fall-through claim refuted |
| 67 | `1102` | 0x00BD259C | 7 | → func_00BFE118(&m_3834C, &stream) (slot 12) | `func_00BFE118` 400 B, *16* | IgnoreListResult |
| 68 | `1402` | 0x00BD25B8 | 7 | → func_00BF7240(&m_4EC58, &stream) (slot 14) | `func_00BF7240` 280 B, *10* | ClassMasteryServer |
| 69 | `07F2`, `07F4` | 0x00BD25D4 | 5 | func_00BD30E8(this, 1662) | `func_00BD30E8` 108 B, *2* | EntityUpdateFullUI — **notes: "Request DSP 1662"; a char-select message** |
| 70 | `07F0`, `07F1`, `07F3` | 0x00BD25E8 | 5 | func_00BD30E8(this, 676) | `func_00BD30E8` 108 B, *2* | EntityUpdateMapPanel — **notes: "Request DSP 676"; a char-select message** |
| 71 | `07A1` | 0x00BD25FC | 5 | → func_00BD3158(this, &stream) = Read 8 bytes, Disconnect(13, 0) | `func_00BD3158` 68 B, *2* | EntityUpdateAudio — **notes: "audio + state 13"; it is Disconnect(13)** |
| 72 | `07A2` | 0x00BD2610 | 5 | → func_00BD31A0(this, &stream) = Disconnect(14, 0); stream unused | `func_00BD31A0` 32 B, *1* | EntityUpdateMinimal — **notes: "state 14"; it is Disconnect(14)** |
| 73 | `07C0` | 0x00BD2624 | 3 | Disconnect(17, 0) (shared tail) | `Disconnect__8VIClientii` 412 B (**matched** by the other lane) | EntityUpdateDirect — **notes: "state 17, 18"; it is Disconnect(17)** |
| 74 | `07C1` | 0x00BD2630 | 3 | Disconnect(18, 0) (shared tail) | `Disconnect__8VIClientii` 412 B (**matched** by the other lane) | AdminShutdown — **notes: "state 18 + tunar + tail"; Disconnect(18) only** |
| 75 | `0052` | 0x00BD263C | 6 | ReadSigned(&m_C310) | inline | PlayerTunar — fall-through claim refuted |
| 76 | `124D` | 0x00BD2654 | 15 | if (func_00C56CA8(m_2BB78)) { func_00C55888(m_2BB78); func_00BB8C28(&m_47388) (bank +0x78C8 = 0) } | `func_00C56CA8` 12 B, *0*<br>`func_00C55888` 100 B, *3*, GetAppPtr<br>`func_00BB8C28` 8 B, *0* | BankUI |
| 77 | `1250`, `1251`, `1253` | 0x00BD2690 | 8 | func_00BB9CF0(&m_47388, opcode, &stream) (slot 3; opcode reloaded with lhu) | `func_00BB9CF0` 160 B, *5* | ConfirmBankTunar |
| 78 | `1206` | 0x00BD26B0 | 7 | → func_00BB9D90(&m_47388, &stream) (VIBankControl.cpp:593) | `func_00BB9D90` 344 B, *11*, _VIAlert | AuctionStuff |
| 79 | `0036` | 0x00BD26CC | 20 | Read u8 a; old = m_C31C; ReadSigned(&m_C31C); if (m_C31C != old) func_00BD9AA8(this, a, m_C31C - old) | `func_00BD9AA8` 156 B, *6*, snprintf16 | QuestStageComplete; QuestDialogue — one notes table gives 0x00BD1A4C (that is 0x0046) |
| 80 | `001D` | 0x00BD271C | 8 | ReadSigned z; func_00BDCAF0(this, z) | `func_00BDCAF0` 60 B, *1* | UpdateTrainingPts |
| 81 | `000A` | 0x00BD273C | 16 | Read u8 a, b; func_00BD9C18(this, a, b) | `func_00BD9C18` 328 B, *4* | PrepareItem — **notes place it at 0x00BD2A94 (that is 0x0012)** |
| 82 | `0010` | 0x00BD277C | 5 | → func_00BD9D60(this, &stream) | `func_00BD9D60` 236 B, *1* | ClassicTargetInfo — fall-through claim refuted |
| 83 | `00EC`, `07A4` | 0x00BD2790 | 9 | printf("Disconnect-9\n"); Disconnect(8, 0) (owns the shared Disconnect tail) | `Disconnect__8VIClientii` 412 B (**matched** by the other lane) | ForceDisconnect; LoggedInElsewhere — fall-through claim refuted |
| 84 | `0776` | 0x00BD27B4 | 22 | Read i32 a, b, c; func_00BD9E50(this, a, b, c) | `func_00BD9E50` 172 B, *1*, FindTyped, Play |  |
| 85 | `077A` | 0x00BD280C | 23 | func_00C56C40(m_2BB78, 1196, &VIColor32F(1,.25,.25,1)); func_00BD5B60(this) (m_71E4 = 0); func_00BE4B78(this, 1915) | `func_00C56C40` 68 B, *1*<br>`func_00BD5B60` 8 B, *0*<br>`func_00BE4B78` 172 B, *2*, free | OutMatched — fall-through claim refuted |
| 86 | `0057` | 0x00BD2868 | 16 | Read i32 a, b; func_00BDA1A8(this, a, b) | `func_00BDA1A8` 276 B, *7*, snprintf16, strcpy | TradeRequest |
| 87 | `0056` | 0x00BD28A8 | 87 | Read u8 sub, i32 id; if (id != m_2BB08 && sub is not 1, 2 or 20) break; switch (table 0x00CDAAF0, body order 2,1,20,16,17,21,22,7,6,default): 2 → func_00BDA2C0(this, id); 1, 20, 16, 17, 21, 22, 7, 6 → one call each (this); default → m_2BB04 ? func_00BDA528(this, sub, &stream) : func_00BDA688(this, sub, &stream) | `func_00BDA2C0` 60 B, *2*<br>`func_00BDA300` 28 B, *1*<br>`func_00BDA320` 152 B, *4*, snprintf16<br>`func_00BCCDC8` 60 B, *2*<br>+7 more (§4) | TradeUI — **notes: 22 dedicated calls; 9 + a shared default** |
| 88 | `005E` | 0x00BD2A04 | 4 | → func_00BDAEB8(this) | `func_00BDAEB8` 396 B, *10*, GetActorSprite, GetActorSpriteType, GetTime, SetDeath, Sprite, StopIfThisWasCaster | CharacterDied |
| 89 | `0060` | 0x00BD2A14 | 18 | Read i32 id; ReadSigned z; func_00BCB988(this, func_00BDFFA8(&m_C428, id), z) | `func_00BDFFA8` 56 B, *0*<br>`func_00BCB988` 76 B, *3* | AdjustItemHP |
| 90 | `0061` | 0x00BD2A5C | 10 | Read i32 a; func_00BDB048(this, a) | `func_00BDB048` 96 B, *2* | BlackSmithMenu |
| 91 | `0063` | 0x00BD2A84 | 4 | → func_00BDB0A8(this) | `func_00BDB0A8` 60 B, *1* |  |
| 92 | `0012` | 0x00BD2A94 | 13 | ReadString(name[24]); func_00C23460(m_2BB88, name) | `func_00C23460` 32 B, *1* |  |
| 93 | `0013` | 0x00BD2AC8 | 16 | Read two 8-byte `long`s (sp+2240/2248); func_00BD0508(this, a, b) (passed with ld) | `func_00BD0508` 88 B, *1* | Time — fall-through claim refuted |
| 94 | `0045` | 0x00BD2B08 | 8 | ReadSigned z; func_00BDB820(this, z) | `func_00BDB820` 204 B, *2*, snprintf16, strcat16 | QuestDialogueResp — fall-through claim refuted |
| 95 | `006E` | 0x00BD2B28 | 4 | → func_00BDB8F0(this): an empty function | `func_00BDB8F0` 8 B, *0* | StubNOP — fall-through claim refuted |
| 96 | `007B` | 0x00BD2B38 | 10 | Read i32 a; func_00BDBAC8(this, a) | `func_00BDBAC8` 104 B, *2* | UpdateQuestProgress — fall-through claim refuted |
| 97 | `007C` | 0x00BD2B60 | 17 | Read(&m_2BB0C, 4, 1); ReadString16(buf[128], 256); func_00BDBB30(this, buf) | `func_00BDBB30` 100 B, *0*, strcpy16 |  |
| 98 | `007D` | 0x00BD2BA4 | 15 | Read(&m_2BB0C, 4, 1); ReadSigned z; func_00BDBB98(this, z) | `func_00BDBB98` 212 B, *0*, strcpy16 | **notes place it at 0x00BD2C28 (that is 0x007F)** |
| 99 | `00F8` | 0x00BD2BE0 | 15 | Read(&m_2CEE4 (float), 4, 1); func_00BDD540(this, m_2CEE4) | `func_00BDD540` 112 B, *0*, InitIntermediate | PlayerSpeed |
| 100 | `007E` | 0x00BD2C1C | 3 | Read(&m_2BE50, 4, 1) (shared Read tail) | inline |  |
| 101 | `007F` | 0x00BD2C28 | 8 | m_2BE50 = -1; u8 m_2BE54 = 0 | inline | ClearQuestTracker — fall-through claim refuted |
| 102 | `00D3` | 0x00BD2C48 | 16 | ReadString(name[64], 64); Read u8 c; func_00BDB0E8(this, name, c) | `func_00BDB0E8` 228 B, *2*, str8to16, strcat16 | NPCMessage — fall-through claim refuted |
| 103 | `1003`, `1004`, `1005` | 0x00BD2C88 | 9 | func_00BFA850(&m_38300, opcode, &stream) (slot 9; opcode still in a1); func_00BC7218(this) | `func_00BFA850` 484 B, *8*, strcpy, strcpy8<br>`func_00BC7218` 168 B, *0* |  |
| 104 | `0790` | 0x00BD2CAC | 5 | → func_00BDC080(this, &stream) | `func_00BDC080` 156 B, *2* | ServerAssignment |
| 105 | `00C5` | 0x00BD2CC0 | 11 | Read u8 w; func_00BD9F00(this, w, 0) | `func_00BD9F00` 216 B, *0*, SetWeather | WeatherControl — fall-through claim refuted |
| 106 | `00C7` | 0x00BD2CEC | 11 | Read u8 w; func_00BD9F00(this, w, 1) | `func_00BD9F00` 216 B, *0*, SetWeather | SkyboxWeather — fall-through claim refuted |
| 107 | `00D7` | 0x00BD2D18 | 4 | → func_00BD9FD8(this) | `func_00BD9FD8` 76 B, *1* | InventoryFull — fall-through claim refuted |
| 108 | `07A3` | 0x00BD2D28 | 13 | ReadString(name[24]); VIString::str8to16(m_4F0B8, 48, name) | inline | ChatMessage — fall-through claim refuted |
| 109 | `00D9` | 0x00BD2D5C | 5 | Read(&m_C308, 4, 1) (shared Read tail) | inline | HPResourceUpdate — **notes: power/HP; it writes player+0x2C** |
| 110 | `00DB` | 0x00BD2D70 | 27 | Read i32 attacker (sp+0x8DC); ReadSigned amount (+0x8E0); Read i32 defender (+0x8E4); if (attacker == m_71F8) func_00BDC220(this, defender, amount); else func_00BDC288(this, attacker, amount) | `func_00BDC220` 100 B, *1*, VIGetGameOptions<br>`func_00BDC288` 128 B, *2*, VIGetGameOptions | DamageIndication |
| 111 | `07B0` | 0x00BD2DDC | 7 | → func_00BE71B0(&m_33950, &stream) (slot 8) | `func_00BE71B0` 228 B, *7* | ZoneTransferInit |
| 112 | `07B2` | 0x00BD2DF8 | 4 | Read(&m_7578, 4, 1) (ctor default 100; shared Read tail) | inline | ZoneTransferConfirm |
| 113 | `07D1` | 0x00BD2E08 | 9 | Read(&m_50304, 4, 1) (owns the shared Read tail) | inline | Camera1 — fall-through claim refuted |
| 114 | `07D2` | 0x00BD2E2C | 5 | Disconnect(19, 0): the last case, falls into the exit | `Disconnect__8VIClientii` 412 B (**matched** by the other lane) | CharModifiedDC — **notes: "state 19"; it is Disconnect(19)** |

## 3. The objects it works on

### 3.1 `VIClient` (this)

* **Hierarchy and RTTI.** `VIClient : VIClientI`. Type names `"8VIClient"` at
  `0x00CDB560` and `"9VIClientI"` at `0x00CDB570`; `__tf8VIClient` is `func_00BE37E8`
  (80 B; byte-identical in shape to SUPPORT's `__tf` functions) and `__tf9VIClientI`
  is `func_00BE3BD0`. RTTI is on in this build.
* **vtable at `0x00CDB4C0`**, 160 bytes, 8-byte entries: entry 0 = `__tf8VIClient`,
  18 virtuals, NULL end. The ctor (`__8VIClient`, 0x00BBF698) and the dtor
  (`_$_8VIClient`, 0x00BC2638 = slot 2) store it at `this+0`, so **the vptr is at +0**
  (VIClientI has no data). Slot 1 `func_00BD5EE8` (24 B) returns a 2-word struct
  `{m_conn (+0x7564), arg}` by value. **Slots 3–18 are one-line accessors returning
  sub-objects**, and the dispatcher passes the same addresses directly:

| Slot | Accessor | Returns | Dispatcher cases passing it |
|---|---|---|---|
| 3 | `func_00BCCB58` | `this+0x47388` (bank, VIBankControl.cpp) | `0x124D`, `0x1250/51/53`, `0x1206` |
| 4 | `func_00BCDE38` | `this+0x2EC84` | `0x0468` |
| 5 | `func_00BCDE48` | `this+0x2EF24` | `0x057A`, `0x0579` |
| 6 | `func_00BCDE58` | `this+0x2EFCC` | — |
| 7 | `func_00BCDE68` | `this+0x2EFD0` (game.h: `VIClientPart2EFD0`) | `0x0D41` |
| 8 | `func_00BCDE78` | `this+0x33950` | `0x07B0` |
| 9 | `func_00BCE1A0` | `this+0x38300` | `0x1003/04/05` |
| 10 | `func_00BCE1B0` | `this+0x382E8` | `0x0700` |
| 11 | `func_00BCE1C0` | `this+0x38334` | `0x0E02` |
| 12 | `func_00BCE1D0` | `this+0x3834C` | `0x1102` |
| 13 | `func_00BCE1E0` | `this+0x3FC10` | `0x0F02` |
| 14 | `func_00BCE1F0` | `this+0x4EC58` | `0x1402` |
| 15 | `func_00BCE200` | `this+0x4F078` | `0x07E2` |
| 16 | `func_00BCE7A0` | `this+0x4F234` | — |
| 17 | `func_00BE39B0` | `this+0xC2DC` (player record) | fields read directly, see below |
| 18 | `func_00BD7BD8` | `this+0x2EFC8` | — |

* **Constructor member roll-call** (`__8VIClient`): `VIUI` at **+0x4**, CLIENT ctors at
  +0x90 and +0x190, `VIRaster` +0x380, `VISoundDevice` +0x4FE0, `VICollide` +0x6160,
  `VIConsole` +0x6390 (no ctor of its own: its `VIPad` +0x6390 and `VIKeyboard` +0x63F0
  are constructed inline; game.h agrees), `VIDictionary`
  +0x6618, `VILoader` +0x6638, CLIENT ctors +0x6D00 and +0x6D50, `VIWalkControl` +0x6DD0,
  `VICameraEffect` +0x705C, `m_7578 = 100`, `VIScene` **+0x7590**; then CLIENT members:
  VIArray-shaped `{0,0}` ctors (`func_00BBB758`) at +0xC70C, +0x13E7C, +0x1B5F4,
  +0x22E60, +0x2A5C0, +0x2B1C4; +0x2BB2C, +0x2BB98, +0x2BD58, +0x2E7E8, VIMap +0x2EC70,
  +0x2EC84, +0x2EF24, +0x2EFC8, +0x2EFCC, +0x2EFD0, +0x33950, +0x382E8, +0x38300,
  +0x38320, +0x38334, +0x3834C, +0x3FC10, +0x47388, +0x4EC58, +0x4F078, +0x4F098,
  +0x4F234, `VIBattleMusic` +0x4FC00, +0x4FC24, +0x4FD28.
* `VIWnd::mUI` (SUPPORT static, `_5VIWnd$mUI`) points at `this+4`, which is why SUPPORT's
  `0x004E37F0` reads singleton+4 in the FrontiersForge note. `m_ui.m_words` is the
  1,817-row × 2-language word table.

**Fields the dispatcher itself touches** (beyond the sub-objects above):

| Offset | Type | Cases | Meaning (where known) |
|---|---|---|---|
| +0x4 / +0x34 | `VIUI m_ui` / `m_ui.m_language` | 0x069C, 0x069A, 0x0065, 0x002F | word table for `GetString` |
| +0x58 | int | 0x002C (also 0x07Fx via func_00BD30E8) | client state; 2–3 → 3 = character select (Disconnect stores -1) |
| +0x6DC8 | int | 0x00FC | u8 value stored as int |
| +0x7500 | `VIEntity *[24]` | 0x0775 via FindEntity | entity id at `entity+0x0C`, actor index at `entity+0x24` |
| +0x71F8 | int | 0x0775, 0x00DB | own entity id (FrontiersForge 0x1FB6928) |
| +0x7578 | int | 0x07B2 | ctor default 100 |
| +0x7590 | `VIScene m_scene` | 0x0775 | `GetActorSpriteType/GetActorSprite/Sprite` |
| +0xC2DC.. | player record | 0x0020 (+0xC300/+0xC304), 0x00D9 (+0xC308), 0x0052 (+0xC310), 0x0036 (+0xC31C), 0x001C (+0xC320) | +0xC300 = player+0x24 = **level** (FrontiersForge-verified 49), +0xC304 = player+0x28 (exp, unverified) |
| +0xC428 | item list (inventory) | 0x0060 (`func_00BDFFA8` = find slot by id, stride 764) | |
| +0x13B94 | u8 `HaveCorpse` | 0x0018 (asserted false, set 1), 0x0016 (cleared) | the assert text names it |
| +0x13B98 | item list (corpse loot) | 0x0018 (`func_00BBC8D0(list, stream)`), 0x0016 (words 0,1 zeroed) | |
| +0x1B300/+0x1B308/+0x1B30C/+0x1B310 | merchant: id, 2 zigzags, item list | 0x00B7 | |
| +0x22A78 | `unsigned short[128]` | 0x00B7 (memset + ReadString16) | merchant name |
| +0x2BB04 / +0x2BB08 | int / int | 0x0056 | trade active? / trade partner id |
| +0x2BB0C | int | 0x007C, 0x007D | |
| +0x2BB60 | int | 0x00B4, 0x00B5, 0x00B6, 0x00B8, 0x00CD | read straight from the stream |
| +0x2BB78 | window * | 0x069E, 0x00CE, 0x0758, 0x0018, 0x0016, 0x0085, 0x124D, 0x077A (and ShowText) | chat/message window; methods at 0x00C556D8–0x00C56CA8 (VIWndGame.cpp neighbourhood) |
| +0x2BB84 | window * | 0x002E, 0x0065, 0x002F (and func_00BD30E8) | char-select/login message window; `func_00C21668(id)`, `func_00C215F8(text)` |
| +0x2BB88 | window * | 0x0012 | `func_00C23460(name)` |
| +0x2BD70/74/78, +0x2BE4C | int | 0x069A/0x062F tail | group: leader(-1)/in-group (`func_00BD7A68` returns +0x2BD74)/… |
| +0x2BE50 / +0x2BE54 | int / u8 | 0x007E, 0x007F | |
| +0x2BE6C | int | 0x002E, 0x0065, 0x002F | character count (0x002C's parser writes it) |
| +0x2CEE4 | float | 0x00F8 | passed in `$f12` |
| +0x2EC70 | `VIMap<int, T>`, sizeof(T)=172 | 0x03D5, 0x03D6 | buddy list (node = 16-B header + key + value = 192 B) |
| +0x382E4 | u8 | 0x069A/0x062F tail | `== 2` → `func_00BD9B48(this, 0)` |
| +0x4F0B8 | `unsigned short[24]` | 0x07A3 | `VIString::str8to16(…, 48, name)` |
| +0x4F0E8 | int | 0x002C (=21) | Disconnect's reason field (game.h) |
| +0x50304 | int | 0x07D1 | |

### 3.2 The caller: `func_00BD4C10`, the DRDP receive loop (592 B)

`while ((m = drdp_receive(m_drdp /* +0x7560 */)))`: builds its own `VIMemStream`, then
`switch (message_get_type(m))` through the table at `0x00CDAB90` (type − 248):

| DRDP type | Arm |
|---|---|
| 0xF8 | `func_00BD0D00(this, src, &stream)`: reads u16; if 2451 → `func_00BDC120(this, &stream)` |
| 0xF9 | `func_00BD0D60(this, data, size)`: own stream, reads one u8, nothing else |
| 0xFA, < 0xF8 | `func_00BD3C18(this, type, data, size)`: the state-channel types (it tests 0x40, 0x42, 0x43, …) |
| **0xFB** | **`func_00BD0DC8(this, src, data, size)` — this dispatcher** |
| 0xFC | `func_00BD3AF8(this, data, size)` |
| 0xFD | `m_4F0EC = data[0]`; reason 14→16, 1→1, 15→15, else 10; `printf("Disconnect-11\n")`; `Disconnect(reason, 0)` |
| 0xFE | nothing |
| 0xFF | `connection_close(m_conn)`, `func_00BD0560(this)`, `m_conn = message_get_connection(m)`, `func_00BF5738(&m_2EFD0)`, `func_00BE7298(&m_33950)` |

then `message_destroy(m)` and the inline `~VIMemStream`. Not named yet; a good companion
to the dispatcher (same TU, uses the same stream idiom).

### 3.3 `VIMemStream` — a new CLIENT type the dispatcher inlines

VIMemStream.cpp (asserts `"MaxBytes >= 1"` line 399, `"MaxBytes >= sizeof(uint16)"`
line 422). 32 bytes, all `int`/pointers:

| Off | Field (proposed) | Evidence |
|---|---|---|
| +0x00 | `char *m_wbuf` | Write's destination base; freed by the dtor when owned |
| +0x04 | `const char *m_rbuf` | Read's source base; the dispatcher's `data` |
| +0x08 | `int m_capacity` | Write's limit; `size` |
| +0x0C | `int m_size` | Write position / Read's limit; `size` |
| +0x10 | `int m_rpos` | Read position |
| +0x14 | `int m_error` | sticky overrun flag; set by Read/Write/Skip |
| +0x18 | `int m_dryRun` | when set, Read/Write advance without `memcpy` |
| +0x1C | `int m_owned` | dtor: `if (m_owned && m_wbuf) free(m_wbuf)` |

| VA | Bytes | Proposed name | Behaviour |
|---|---|---|---|
| 0x00CB8260 | 152 | copy ctor | copies 8 words; if owned: `malloc(cap)` + `memcpy`, `m_rbuf = m_wbuf` |
| 0x00CB82F8 | 120 | `OpenWrite(void *, int)` | frees owned; wbuf=rbuf=buf, cap=n, rest 0 |
| 0x00CB8370 | 120 | `OpenRead(const void *, int)` | the out-of-line twin of the dispatcher's inline ctor: wbuf 0, rbuf data, cap=size=n |
| 0x00CB83E8 | 120 | `Reserve(int)` | becomes owned, `realloc`, wbuf=rbuf=p |
| 0x00CB8460 | 192 | `Write(const void *, int size, int count)` | grows when owned (`+32` slack) |
| **0x00CB8520** | **136** | **`Read(void *, int size, int count)`** | **44 dispatcher cases** |
| 0x00CB85A8 | 56 | `Skip(int)` | |
| 0x00CB85E0 | 48 | `WriteSigned(int)` | zigzag, then WriteUnsigned |
| **0x00CB8610** | **72** | **`ReadSigned(int &)`** | `(v & 1) ? ~(v >> 1) : v >> 1` (a `movz`); 14 cases |
| 0x00CB8658 | 96 | `WriteUnsigned(unsigned)` | 7-bit varint |
| 0x00CB86B8 | 136 | `ReadUnsigned(unsigned &)` | 7-bit varint, stops on error |
| 0x00CB8740 | 168 | `ReadString(char *, unsigned MaxBytes)` | u32 length `n`; if `n + 1 <= MaxBytes`: `Read(dst, 1, n)`, `dst[n] = 0`. Else `VIASSERT(MaxBytes >= 1)` (line 399; fires only when MaxBytes is 0), `Skip(n)`, `dst[0] = 0` |
| 0x00CB87E8 | 184 | `ReadString(unsigned short *, unsigned MaxBytes)` | u32 count `n`; if `n + 1 <= MaxBytes / 2`: `Read(dst, 2, n)`, `dst[n] = 0`. Else `VIASSERT(MaxBytes >= sizeof(uint16))` (line 422), `Skip(2n)`, `dst[0] = 0` |
| 0x00CB88A0 / 0x00CB8918 | 120 / 120 | `WriteString(const char *)` / `(const unsigned short *)` | `strlen` / `VIString::strlen16` |
| 0x00CB8990 / 0x00CB8998 | 8 / 8 | set / get `m_dryRun` | |
| 0x00CB8A98 / 0x00CB8AA0 / 0x00CB8AB0 | 8 / 16 / 8 | `m_rpos` / `m_size - m_rpos` / `m_rbuf` | |

### 3.4 Other types the cases need

* SUPPORT, already in `vi.h`: `VIUI` (+0x30 `m_language`), `VIStringTable`,
  `VIColor32F` (4 floats; the chat colours are (0.5,0.5,1,1), (1,0.25,0.25,1)),
  `VIScene::GetActorSpriteType/GetActorSprite/Sprite`,
  `VICSprite::SetAction(VICSpriteAnimID, VIScene &)`, `VIString::snprintf16/str8to16`,
  `VIMap<K,V>` (template; `Find`, `End` (returns 0), `Insert`, `Value(i)` inline).
* CLIENT, new: `VIMemStream`; the player record at +0xC2DC; the item list (VIItemList.h;
  `func_00BBC8D0` reads one from a stream); the 0x003B item (≥ 0x2E4 bytes, a VIArray
  member at +0x2DC); the 172-byte buddy record (`func_00CB7788` clears it,
  `func_00CB7850(stream, rec)` reads it, `+0x28` online flag, `+0x2C` wide name[64]);
  the three window classes behind `m_2BB78/84/88`; one class per vtable-slot sub-object.
* `func_00C0B970` **is byte-identical to SUPPORT's `GetString__13VIStringTableii`**
  (88 B): CLIENT compiled its own VIStringTable.cpp (an assert hint
  `VIStringTable.cpp:245` sits at 0x00C0BD88). SUPPORT's source is the source.

## 4. Handler functions (direct CLIENT callees of the dispatcher, by size)

"TU" is **VIClient.cpp** when the function lies in that TU's code range (§6); otherwise
the assert-anchored functions on either side of it (a bracket, not an attribution). *own* = distinct CLIENT placeholders it
calls; *imports* = SUPPORT/SLUS names it calls.

| VA (name) | Bytes | Insns | Opcode(s) | TU | *own* | Imports |
|---|---|---|---|---|---|---|
| `func_00BB8C28` | 8 | 2 | 124D | before VIBankControl.cpp:593 (CLIENT's first TUs) | 0 |  |
| `func_00BD5B60` | 8 | 2 | 077A | VIClient.cpp | 0 |  |
| `func_00BDB8F0` | 8 | 2 | 006E | VIClient.cpp | 0 |  |
| `func_00BDF388` | 8 | 2 | 03D5 | VIClient.cpp | 0 |  |
| `func_00C56CA8` | 12 | 3 | 0018, 124D | between VIWndGame.cpp:4007 and VIWndGuild.cpp:503 | 0 |  |
| `func_00BBB758` | 16 | 4 | 003B | between VIBankControl.cpp:593 and VIClient.cpp:1705 | 0 |  |
| `func_00BD7A68` | 20 | 5 | 062F, 069A | VIClient.cpp | 0 |  |
| `func_00BCB098` | 28 | 7 | 0018 | VIClient.cpp | 1 |  |
| `func_00BD8E78` | 28 | 7 | 0020 | VIClient.cpp | 0 | SoundPlay |
| `func_00BDA300` | 28 | 7 | 0056 | VIClient.cpp | 1 |  |
| `func_00C56BA0` | 28 | 7 | 0758 | between VIWndGame.cpp:4007 and VIWndGuild.cpp:503 | 1 |  |
| `func_00BCB208` | 32 | 8 | 003D | VIClient.cpp | 1 |  |
| `func_00BD31A0` | 32 | 8 | 07A2 | VIClient.cpp | 1 |  |
| `func_00C23460` | 32 | 8 | 0012 | between VIStringTable.cpp:245 and VIWndGame.cpp:4007 | 1 |  |
| `func_00C56C20` | 32 | 8 | 0085 | between VIWndGame.cpp:4007 and VIWndGuild.cpp:503 | 1 |  |
| `func_00BD9138` | 40 | 10 | 069C | VIClient.cpp | 1 |  |
| `func_00BDA4D8` | 40 | 10 | 0056 | VIClient.cpp | 1 |  |
| `func_00BDA500` | 40 | 10 | 0056 | VIClient.cpp | 1 |  |
| `func_00C55710` | 48 | 12 | 0016 | between VIWndGame.cpp:4007 and VIWndGuild.cpp:503 | 1 |  |
| `func_00BC9870` | 56 | 14 | 0775 | VIClient.cpp | 0 |  |
| `func_00BDFFA8` | 56 | 14 | 0060 | VIClient.cpp | 0 |  |
| `func_00C556D8` | 56 | 14 | 0018 | between VIWndGame.cpp:4007 and VIWndGuild.cpp:503 | 2 |  |
| `func_00BCCDC8` | 60 | 15 | 0056 | VIClient.cpp | 2 |  |
| `func_00BD90F8` | 60 | 15 | 03D7, 03D8, 062F, 069A, 069C | VIClient.cpp | 2 |  |
| `func_00BDA2C0` | 60 | 15 | 0056 | VIClient.cpp | 2 |  |
| `func_00BDB0A8` | 60 | 15 | 0063 | VIClient.cpp | 1 |  |
| `func_00BDCAF0` | 60 | 15 | 001D | VIClient.cpp | 1 |  |
| `func_00C21668` | 64 | 16 | 002E | between VIStringTable.cpp:245 and VIWndGame.cpp:4007 | 2 |  |
| `func_00C56BE0` | 64 | 16 | 00CE, 069E | between VIWndGame.cpp:4007 and VIWndGuild.cpp:503 | 1 |  |
| `func_00BD3158` | 68 | 17 | 07A1 | VIClient.cpp | 2 |  |
| `func_00C56C40` | 68 | 17 | 077A | between VIWndGame.cpp:4007 and VIWndGuild.cpp:503 | 1 |  |
| `func_00CB8610` (stream) | 68 | 17 | 0019, 001D, 0020, 0036, 003C, 0045, 0052, 0060, 007D, 00B7, 00B8, 00CD, 00DB, 03D6 | VIMemStream.cpp | 1 |  |
| `func_00CB7788` | 72 | 18 | 03D5 | between VIFileStream.cpp:219 and VIJournal.cpp:99 | 0 | memset |
| `func_00BCB988` | 76 | 19 | 0060 | VIClient.cpp | 3 |  |
| `func_00BD9FD8` | 76 | 19 | 00D7 | VIClient.cpp | 1 |  |
| `func_00BD0508` | 88 | 22 | 0013 | VIClient.cpp | 1 |  |
| `func_00C0B970` (= SUPPORT GetString) | 88 | 22 | 002F, 0065, 069A, 069C | VIStringTable.cpp (CLIENT's copy) | 0 |  |
| `func_00BCB0D8` | 92 | 23 | 003B | VIClient.cpp | 3 |  |
| `func_00BD9A48` | 92 | 23 | 00B7 | VIClient.cpp | 2 |  |
| `func_00BEA820` | 92 | 23 | 07E2 | between VIClient.cpp:11682 and VIShardInfo.cpp:166 | 2 |  |
| `func_00C03168` | 92 | 23 | 0E02 | between VIShardInfo.cpp:166 and VIStringTable.cpp:245 | 2 |  |
| `func_00BDA3B8` | 96 | 24 | 0056 | VIClient.cpp | 2 |  |
| `func_00BDA418` | 96 | 24 | 0056 | VIClient.cpp | 2 |  |
| `func_00BDA478` | 96 | 24 | 0056 | VIClient.cpp | 2 |  |
| `func_00BDB048` | 96 | 24 | 0061 | VIClient.cpp | 2 |  |
| `func_00BBB7B8` | 100 | 25 | 003B | between VIBankControl.cpp:593 and VIClient.cpp:1705 | 3 |  |
| `func_00BDBB30` | 100 | 25 | 007C | VIClient.cpp | 0 | strcpy16 |
| `func_00BDC220` | 100 | 25 | 00DB | VIClient.cpp | 1 | VIGetGameOptions |
| `func_00C55888` | 100 | 25 | 124D | between VIWndGame.cpp:4007 and VIWndGuild.cpp:503 | 3 | GetAppPtr |
| `func_00BD36D0` | 104 | 26 | 0A7A | VIClient.cpp | 2 |  |
| `func_00BDB650` | 104 | 26 | 0770 | VIClient.cpp | 2 | snprintf16 |
| `func_00BDBAC8` | 104 | 26 | 007B | VIClient.cpp | 2 |  |
| `func_00BE1F68` | 104 | 26 | 03D5, 03D6 | VIClient.cpp | 1 |  |
| `func_00BD30E8` | 108 | 27 | 07F0,07F1,07F3, 07F2,07F4 | VIClient.cpp | 2 |  |
| `func_00C215F8` | 108 | 27 | 002F, 0065 | between VIStringTable.cpp:245 and VIWndGame.cpp:4007 | 1 | strcpy16, strlen16 |
| `func_00BD89F0` | 112 | 28 | 0777 | VIClient.cpp | 2 | printf |
| `func_00BDD540` | 112 | 28 | 00F8 | VIClient.cpp | 0 | InitIntermediate |
| `func_00BB5DD0` | 128 | 32 | 0700 | before VIBankControl.cpp:593 (CLIENT's first TUs) | 3 |  |
| `func_00BDC288` | 128 | 32 | 00DB | VIClient.cpp | 2 | VIGetGameOptions |
| `func_00CB8520` (stream) | 132 | 33 | 0000, 000A, 0013, 0019, 001C, 0036, 003C, 003D, 003E, 003F, 0040, 0046, 0056, 0057, 0060, 0061, 007B, 007C, 007D, 007E, 0085, 00B1, 00B4, 00B5, 00B6, 00B7, 00B8, 00C5, 00C7, 00CD, 00D3, 00D9, 00DB, 00F8, 00FC, 03D5, 069A, 069C, 069E, 0771, 0775, 0776, 07B2, 07D1 | VIMemStream.cpp | 0 | memcpy |
| `func_00CBB040` | 136 | 34 | 003B | between VINetItem.cpp:58 and VISyncTime.cpp:77 | 3 |  |
| `func_00BBC8D0` | 140 | 35 | 0018, 00B7 | between VIBankControl.cpp:593 and VIClient.cpp:1705 | 3 |  |
| `func_00CB7850` | 148 | 37 | 03D5 | between VIFileStream.cpp:219 and VIJournal.cpp:99 | 2 |  |
| `func_00BCF468` | 152 | 38 | 00B8 | VIClient.cpp | 4 |  |
| `func_00BDA320` | 152 | 38 | 0056 | VIClient.cpp | 4 | snprintf16 |
| `func_00BD9AA8` | 156 | 39 | 0036 | VIClient.cpp | 6 | snprintf16 |
| `func_00BDC080` | 156 | 39 | 0790 | VIClient.cpp | 2 |  |
| `func_00BB9CF0` | 160 | 40 | 1250,1251,1253 | before VIBankControl.cpp:593 (CLIENT's first TUs) | 5 |  |
| `func_00BCF500` | 164 | 41 | 00CD | VIClient.cpp | 3 |  |
| `func_00BD99A0` | 164 | 41 | 0019 | VIClient.cpp | 5 |  |
| `func_00BC7218` | 168 | 42 | 1003,1004,1005 | VIClient.cpp | 0 |  |
| `func_00CB8740` (stream) | 168 | 42 | 0012, 00D3, 069C, 0758, 0770, 0771, 07A3 | VIMemStream.cpp (assert line 399) | 2 | _VIAssert |
| `func_00BD9E50` | 172 | 43 | 0776 | VIClient.cpp | 1 | FindTyped, Play |
| `func_00BE4B78` | 172 | 43 | 077A | between VIClient.cpp:11682 and VIShardInfo.cpp:166 | 2 | free |
| `func_00BEF6D8` | 176 | 44 | 057A | between VIClient.cpp:11682 and VIShardInfo.cpp:166 | 4 |  |
| `func_00CB87E8` (stream) | 180 | 45 | 0046, 007C, 00B7, 00CE, 03D5, 069E | VIMemStream.cpp (assert line 422) | 2 | _VIAssert |
| `func_00BD3548` | 184 | 46 | 0774 | VIClient.cpp | 6 |  |
| `func_00BCF3A0` | 196 | 49 | 00B3 | VIClient.cpp | 7 |  |
| `func_00BF5670` | 196 | 49 | 0D41 | between VIClient.cpp:11682 and VIShardInfo.cpp:166 | 5 |  |
| `func_00BCB228` | 200 | 50 | 003E | VIClient.cpp | 6 |  |
| `func_00BCF2D0` | 204 | 51 | 00FF | VIClient.cpp | 4 |  |
| `func_00BD3600` | 204 | 51 | 0A7B | VIClient.cpp | 3 |  |
| `func_00BD9B48` | 204 | 51 | 062F, 069A | VIClient.cpp | 2 |  |
| `func_00BDB820` | 204 | 51 | 0045 | VIClient.cpp | 2 | snprintf16, strcat16 |
| `func_00BCB138` | 208 | 52 | 003C | VIClient.cpp | 6 |  |
| `func_00BDB6B8` | 208 | 52 | 0771 | VIClient.cpp | 2 | snprintf16 |
| `func_00BDBB98` | 212 | 53 | 007D | VIClient.cpp | 0 | strcpy16 |
| `func_00BCF8B0` | 216 | 54 | 00FD | VIClient.cpp | 3 |  |
| `func_00BD9F00` | 216 | 54 | 00C5, 00C7 | VIClient.cpp | 0 | SetWeather |
| `func_00BDB0E8` | 228 | 57 | 00D3 | VIClient.cpp | 2 | str8to16, strcat16 |
| `func_00BE71B0` | 228 | 57 | 07B0 | between VIClient.cpp:11682 and VIShardInfo.cpp:166 | 7 |  |
| `func_00BCB8A0` | 232 | 58 | 0040 | VIClient.cpp | 6 |  |
| `func_00BDBC70` | 232 | 58 | 0046 | VIClient.cpp | 3 | strcpy16 |
| `func_00BD3458` | 236 | 59 | 0773 | VIClient.cpp | 7 | strcpy8 |
| `func_00BD98B0` | 236 | 59 | 001A | VIClient.cpp | 5 |  |
| `func_00BD9D60` | 236 | 59 | 0010 | VIClient.cpp | 1 |  |
| `func_00BEB900` | 268 | 67 | 0F02 | between VIClient.cpp:11682 and VIShardInfo.cpp:166 | 11 |  |
| `func_00BDA1A8` | 276 | 69 | 0057 | VIClient.cpp | 7 | snprintf16, strcpy |
| `func_00BF7240` | 280 | 70 | 1402 | between VIClient.cpp:11682 and VIShardInfo.cpp:166 | 10 |  |
| `func_00BDC308` | 300 | 75 | 07E0 | VIClient.cpp | 8 | free |
| `func_00BD9C18` | 328 | 82 | 000A | VIClient.cpp | 4 |  |
| `func_00BB9D90` | 344 | 86 | 1206 | VIBankControl.cpp:593 | 11 | _VIAlert |
| `func_00BDA528` | 352 | 88 | 0056 | VIClient.cpp | 11 |  |
| `func_00BDA688` | 352 | 88 | 0056 | VIClient.cpp | 11 |  |
| `func_00BCF730` | 384 | 96 | 00B6 | VIClient.cpp | 3 |  |
| `func_00BCF5A8` | 392 | 98 | 00B5 | VIClient.cpp | 3 |  |
| `func_00BDAEB8` | 396 | 99 | 005E | VIClient.cpp | 10 | GetActorSprite, GetActorSpriteType, GetTime, SetDeath, Sprite, StopIfThisWasCaster |
| `func_00BD3738` | 400 | 100 | 00B1 | VIClient.cpp | 1 | FindTyped, Play |
| `func_00BF0D38` | 400 | 100 | 0468 | between VIClient.cpp:11682 and VIShardInfo.cpp:166 | 8 |  |
| `func_00BFE118` | 400 | 100 | 1102 | between VIShardInfo.cpp:166 and VIStringTable.cpp:245 | 16 |  |
| `func_00BD38C8` | 404 | 101 | 00B2 | VIClient.cpp | 10 | EndManual, Reset |
| `Disconnect__8VIClientii` | 412 | 103 | 0000, 00EC,07A4, 0728, 0729, 072A, 072B, 07C0, 07C1, 07D2 | VIClient.cpp | 4 | IsBgmPlaying, StopAllBgm, StopSoundTrack, VIGetFeatureFlags, connection_close, drdp_destroy, drdp_flush |
| `func_00BCF0F8` | 468 | 117 | 00B4 | VIClient.cpp | 6 | snprintf16 |
| `func_00BE2C10` | 468 | 117 | 03D6 | VIClient.cpp | 3 |  |
| `func_00BFA850` | 484 | 121 | 1003,1004,1005 | between VIClient.cpp:11682 and VIShardInfo.cpp:166 | 8 | strcpy, strcpy8 |
| `func_00BEF788` | 532 | 133 | 0579 | between VIClient.cpp:11682 and VIShardInfo.cpp:166 | 4 | snprintf16 |
| `func_00BD2E80` | 612 | 153 | 002C | VIClient.cpp | 6 |  |
| `func_00BDB3E8` | 616 | 154 | 076F | VIClient.cpp | 9 | free, snprintf16 |
| `func_00BD31C0` | 664 | 166 | 0772 | VIClient.cpp | 4 |  |
| `func_00BCB2F0` | 800 | 200 | 003F | VIClient.cpp | 5 |  |
| `func_00BDBD58` | 804 | 201 | 0034 | VIClient.cpp | 7 | memset |
| `func_00BE24A8` | 880 | 220 | 03D5 | VIClient.cpp | 3 |  |
| `func_00BD0630` | 1740 | 435 | 000D | VIClient.cpp | 33 | EndManual, Reset, SetLocation, SetOrbitAngleBehind, SetSky, StopNow, StopSoundTrack, VIDisplayScreen16, VIGetGameOptions, VIHasDiscIdChanged, _VIAssert, connection_send, memset |

## 5. Notes vs image: what is wrong in `claude-notes/`

`mega-function-handlers.md` / `client-mega-function.md` agree with the image on 100
opcode → address pairs. These do not hold:

| Claim | Image |
|---|---|
| "Late-chain fall-through block": handlers 0x00BD24C4–0x00BD2E40 "execute all subsequent code down to the shared exit"; 0x0728 "1235 instructions, falls through" | **Refuted.** A control-flow walk of every case reaches only its own code plus the four cross-jumped tails of §1.4. Each case ends in `b 0x00BD2E40` |
| 0x0728 = ServerLoginResponse ("login → world entry", reads a server list) | `printf("Disconnect-7\n"); Disconnect(6, 0)` — 6 own instructions. (Consistent with CLAUDE.md: never sent by a real server) |
| 0x002E = WorldEntry, "network send of opcode 0x010D" | `func_00C21668(m_2BB84, 269)` shows **word 269** in a window via `VIWnd::mUI`'s table, then `m_2BE6C--` (the character count) |
| 0x0020 GrantXP reads "HP / MaxHP" into +0xC300/+0xC304 | +0xC300 is player+0x24 = **level** (FrontiersForge-verified); sound 16 plays when it rises |
| `ps2-entity-structs.md`: +0xC308 = current power (0x00D9) | +0xC308 is player+0x2C; power is player+0x88 (= +0xC364) per FrontiersForge |
| 0x07A1/0x07A2/0x07C0/0x07C1/0x07D2 = entity-update/UI "state 13/14/17/18/19" | All are **`Disconnect(13/14/17/18/19, 0)`**; "SetEntityState(N)" in the notes is `Disconnect` |
| 0x07F4/0x07F0 = "Request DSP 1662 / 676" | `func_00BD30E8(this, 1662/676)`: `m_58 = 3`, toggles the two login windows, `func_00C21668(m_2BB84, id)` — a message on the character-select screen |
| 0x000D = DSPDataSection at 0x00BD21EC (shared with 0x00B7) | 0x000D is at 0x00BD1578 and calls `func_00BD0630(this, (int)src, &stream)`; 0x00BD21EC is 0x00B7 alone |
| 0x000A PrepareItem at 0x00BD2A94 | 0x000A is at 0x00BD273C (two u8 → `func_00BD9C18`); 0x00BD2A94 is 0x0012 |
| 0x002F at 0x00BD1B7C; 0x0036 at 0x00BD1A4C; 0x0046 at 0x00BD1A94 (one table) | 0x002F 0x00BD1B98 (0x00BD1B7C is 0x0065), 0x0036 0x00BD26CC, 0x0046 0x00BD1A4C (0x00BD1A94 is 0x0758) |
| 0x003A DeleteInvItem at 0x00BD1C14; 0x00B9 at 0x00BD2168; 0x00BA at 0x00BD21BC | **Not dispatched at all.** Those labels are 0x0777, 0x00CD, 0x00B2 |
| 0x007D DeleteQuest shares 0x00BD2C28 with 0x007F | 0x007D 0x00BD2BA4, 0x007E 0x00BD2C1C, 0x007F 0x00BD2C28 |
| 0x0056 "each of 1–22 calls a dedicated trade function" | 9 sub-ops have their own call; 13 share the default, which picks `func_00BDA528` or `func_00BDA688` on `m_2BB04` |
| "102+ opcodes" | 122 opcode values, 114 bodies. 21 values have no note at all: 0x0012, 0x003C, 0x003E, 0x0063, 0x007C, 0x007E, 0x00B2, 0x03D7, 0x072A, 0x0770, 0x0772, 0x0774, 0x0776, 0x07F1–0x07F3, 0x1003–0x1005, 0x1250, 0x1251 |

Disconnect reasons the dispatcher sends (useful to the Go server): 0x0729→3,
0x072B→4, 0x0728→6, 0x072A→7, 0x00EC/0x07A4→8, 0x07A1→13, 0x07A2→14,
0x0000 (version ≠ **18**)→16, 0x07C0→17, 0x07C1→18, 0x07D2→19.

## 6. Source-file attribution

* **VIClient.cpp**, code `0x00BBF0D8`–`0x00BE3C0F` (~150 KB). Evidence: its rodata block
  `[0x00CDA318, 0x00CDB7F8)` (from the `VINet.h` string that opens each TU's rodata to
  the next one) holds the dispatcher's three tables, the receive loop's table, the tables
  of `func_00BD9B48`, `func_00BD9F00`, `func_00BDA528/688`, `func_00BDB6B8`,
  `func_00BDC470/4B8`, `func_00BDCEA0`, then the VIClient vtable and type names.
  The first function referencing it is `func_00BBF0D8` (`"sheet1"`), the last
  `func_00BE3BD0` (`__tf9VIClientI`, a TU-end emission).
  **This corrects the parallel lane's `viclient.cpp` header** ("0x00BBF698 .. 0x00BD4630,
  about 86 KB") and `game.h`'s "`FreeBuffer` 0x00BDD1E8 (another TU)": both are inside
  VIClient.cpp; the asserts simply stop at line 11727.
* VIClient.cpp's asserts, in address order (14 in the whole TU): 1705 (0x00BC18C0), 1790,
  2207, 2277 (0x00BC2F30), 9800/9812/9818/9821 (`func_00BD0630`, the 0x000D handler),
  **10306 (`func_00BD0DC8`, case 0x0018)**, 11682/11693/11704/11715/11727
  (`func_00BD4630`, journal). Lines increase with address, so functions are in source
  order; the dispatcher lies between line ~9822 and line ~11681, with 35 case bodies
  before line 10306 and 78 after.
* 85 of the dispatcher's 123 direct callees lie in VIClient.cpp (§4), including the
  `VIMap`/`VIItemList` template and inline copies emitted at the TU's end
  (`0x00BDE210`–`0x00BE3388`).
* Elsewhere: VIMemStream.cpp `0x00CB8260`–`0x00CB8AB8`; VIStringTable.cpp copy at
  `0x00C0B970`; bank `0x00BB8C28`/`0x00BB9CF0`/`0x00BB9D90` (VIBankControl.cpp:593);
  windows `0x00C556D8`–`0x00C56CA8` (between VIWndGame.cpp:4007 and VIWndGuild.cpp:503);
  `0x00C215F8`/`0x00C21668`/`0x00C23460`; item reads `0x00BBC8D0` (VIItemList area) and
  `0x00CBB040` (VINetItem.cpp:58 area); subsystem handlers `0x00BE71B0`–`0x00C03168`.

## 7. Suggested writing order for phase 62

The dispatcher only *calls* its handlers, and calls are relocations the differ masks, so
it can be scored as soon as every callee is **declared** with the right signature (§2
gives each call's argument list). What it cannot do without is the **types it inlines or
indexes**: `VIMemStream` (ctor/dtor are inline in it) and the `VIClient` field offsets.

1. **Types first (no scoring):** `VIMemStream` (§3.3) in a `game.h`-level header; the
   `VIClient` fields of §3.1 added to the parallel lane's `game.h` (its CHK idiom
   already guards offsets); declarations for the window classes, item list, buddy record
   and sub-object classes (one method each is enough).
2. **VIMemStream.cpp leaves** (new TU, ~1.9 KB, all leaf or self-calling; two asserts pin
   lines 399 and 422): `Read` 136, `ReadUnsigned` 136, `ReadSigned` 72, `Skip` 56,
   `ReadString` 168, `ReadString16` 184, then the write side (`Write` 192, `Reserve` 120,
   `WriteUnsigned` 96, `WriteSigned` 48, `WriteString` ×2 120), the two `Open`s 120 each,
   the copy ctor 152, the accessors 8–16.
3. **VIClient.cpp leaves that need only `game.h` + SUPPORT types:** `func_00BDB8F0` 8
   (empty), `func_00BD5B60` 8 (`m_71E4 = 0`), `func_00BD7A68` 20 (`return m_2BD74`),
   `func_00BD8E78` 28 (`m_ui.SoundPlay(id)`), `func_00BD31A0` 32 (`Disconnect(14,0)`),
   `func_00BC9870` 56 (FindEntity), `func_00BD90F8` 60 + `func_00BD9138` 40
   (ShowMsg/ShowText), `func_00BD3158` 68, `func_00BD30E8` 108, `func_00BD89F0` 112,
   `func_00BDD540` 112 (`VIWalkControl::InitIntermediate`), `func_00BD9F00` 216
   (`VIAtmosphere::SetWeather`, own jump table), `func_00BD0508` 88 (two `long` args).
4. **The dispatcher** (8,372 B). Write the 114 bodies in §2's order, each with its own
   `{ }` and in-block scalars, the `VIASSERT(!HaveCorpse)` on line 10306, `break`
   everywhere, and each shared tail written out in full (§1.4).
5. **Stream-reading VIClient handlers** (need VIMemStream): `func_00BD36D0` 104,
   `func_00BD3548` 184, `func_00BCF3A0` 196, `func_00BD3600` 204, `func_00BCF2D0` 204,
   `func_00BCF8B0` 216, `func_00BD3458` 236, `func_00BD9D60` 236, `func_00BDC308` 300,
   `func_00BD38C8` 404, `func_00BD2E80` 612 (character list; wire format in
   `client-wire-format-parsers.md`), `func_00BDB3E8` 616, `func_00BD31C0` 664,
   `func_00BDBD58` 804, `func_00BD0630` 1,740 (0x000D world entry; four pinned asserts).
6. **Other-TU subsystem handlers** (each needs its own class first): §4 rows whose TU is
   not VIClient.cpp.

**First five:** `func_00CB8520` VIMemStream::Read (136 B, leaf, 44 call sites, fixes the
layout), `func_00CB86B8` ReadUnsigned (136 B), `func_00CB8610` ReadSigned (72 B),
`func_00BC9870` VIClient::FindEntity (56 B, leaf over `m_entities`), `func_00BD90F8`
VIClient::ShowMsg(unsigned short) (60 B, establishes `m_ui` at +4 and the GetString
call). Then the dispatcher.

## 8. Surprises

1. **The notes' "fall-through cascade" model is wrong.** Cases do not run into each
   other; what looked like it is four cross-jumped tails. Several note semantics built
   on it (0x0728 "login", the 0x07xx "state N" chain) are actually `Disconnect(N, 0)`.
2. **There are jump tables after all, just not at the top.** The opcode switch is a
   468-instruction compare tree; three sub-opcode switches inside cases use tables in
   VIClient.cpp's rodata, and their case bodies are emitted in a different order from
   their index order (read the source order from the bodies, §1.3).
3. **The dispatcher is not the only DRDP entry.** It handles one DRDP message type
   (0xFB). The receive loop sends 0xF8, 0xF9, 0xFC and every type below 0xF8 (the state
   channels) to four other handlers, and handles 0xFD (a disconnect) and 0xFF (a new
   connection) inline (§3.2).
4. **Its stack frame is a readable declaration list.** u8 slots in body order, ints in
   first-use order, buffers reused at sp+48 (§1.5). That fixes scoping for every case.
5. **A third of CLIENT is SUPPORT code.** 2,455 CLIENT functions (351 KB of 1,117 KB;
   2,182 / 345 KB at ≥ 64 B) are byte-identical, relocations masked, to a SUPPORT
   function: mostly per-TU `VIArray`/`VIMap`/`VIPool` instantiations and `__tf`
   functions, plus whole copies like `VIStringTable::GetString`. Short matches (< 32 B)
   can be coincidental shapes (e.g. a 28-B wrapper matching `GetTerrainProfile`).
6. **`src` crosses the call as a 64-bit value** and is truncated by the callee; the
   0x000D handler is its only consumer.
7. **Dead code survives.** 0x069E reads an int it never uses; 0x069A's sub-op 5 formats
   `"%S  %S"` into a buffer it never shows; 0x03D6 stores `&id` to two stack words it
   never reads. Each is a source fact to reproduce, not noise.
8. **Two cases forward the opcode itself:** 0x1250/51/53 reload it (`lhu 32(sp)`),
   0x1003/04/05 rely on `a1` still holding it from the tree (no load). Expect the source
   to pass `opcode` in both; the difference is codegen.
9. **Sandstorm patches inside this function**: per the FrontiersForge note, the S2
   payload replaces case 0x00DB's first two compare words (`lw a1,0x8DC(sp)` at
   0x00BD2DA4 and the `lw v0,0x71F8(s4)` after it) with `jal` + `nop`. A live Sandstorm
   dump will not match the image there; the image is the oracle.
