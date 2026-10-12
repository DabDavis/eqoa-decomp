# Phase 10 — `VIString`: the first non-mathematical class

**176 of 212 → 222 of 259 byte-exact.** `VIString` is 47 symbols and **46 of them
match**, 26 of those on the first build, on the toolchain Phase 3 settled and
Phases 4–9 reproduced (`ee-gcc2.95.3-107 -O2 -G0 -fno-exceptions`, `-S`,
`ps2eeas` from the `2.95.3-114` package) — **with no re-tuning of compiler,
assembler or flags.**

Phase 9 added integer code. This phase adds everything else the compiler has:
`new[]` / `delete[]`, `char` and `unsigned short` pointer walks, C varargs,
`alloca`, a static member array, and a member function that returns a class **by
value**. None of it needed a flag that the float math did not.

## The layout

```cpp
class VIString
{
public:
    int   m_len;      // +0  characters, not counting the terminator
    int   m_size;     // +4  the allocation in BYTES
    char *m_str;      // +8
};
```

Read off `Allocate`, which news `m_size` bytes, copies `m_len` of them, and
writes the terminator at `[m_len]`. Every member reloads all three through
`this` after a call, so none of them is cached in the source either.

## The lever this phase found: the guarded do/while

> **A loop whose condition is also read in the body is written as an explicit
> guard around a `do`/`while`** — `if (*a) { do { … } while (*a); }` — not as a
> plain `while (*a)`.

A `while` leaves the exit test at the TOP of the loop, where gcc reuses its load
for the body; the guarded form emits **two** loads and a `bnezl` whose annulled
delay slot carries the body's first instruction. `strcmp16` had scored 14 of 14
wrong through six `while` spellings and matched **first try** in the guarded
form; `strcasecmp16` (33 wrong) and `strstr16`'s outer loop went the same way.

The inner loop of `strstr16` may be written either way — `while (*p == *q && *p)`
and a guarded `do` are byte-identical there — so the idiom is load-bearing only
where the condition's value is wanted inside the body.

## Second lever: a copy loop WALKS THE PARAMETER

> The moving pointer is the **parameter**; the base is a local
> (`char *r = d;` … `return r;`), never `char *d = dst;` walking a local.

Five functions (`strcpy8`, `strcpy16`, `strncpy16`, `str16to8`, `str8to16`) were
"right instructions, wrong registers" until this was applied — the target holds
the walker in the argument register's home and the base in the copy, and the
reverse spelling swaps every register in the loop. `strcat16`, whose walker
starts at `dst + strlen16(dst)`, is the opposite case and matched with the base
in the parameter, so the rule is about which variable is *initialised from* which.

## Source facts

1. **`operator=` returns by VALUE.** `a0` is the caller's sret slot, `a1` is
   `this`, `a2` the argument, and the tail is a copy constructor into `a0`
   followed by the temporary's destructor. A `VIString &` return emits none of
   that. So does `operator+` (three overloads), which builds a local `VIString`,
   `Init`s it, `Concat`s and returns it — five calls, no NRVO, exactly as Phase 5
   found.
2. **`delete []`, not `free`.** `Allocate` tests `m_str` twice: once in the
   source, once as `delete []`'s own `if (p) operator delete(p)`, which gcc
   cannot fold away because `memcpy` intervenes and `m_str` is reloaded through
   `this`. `Deallocate`, where nothing intervenes, folds the same `delete []` to
   one test. The pair of them is what identifies the operator.
3. **The loop variable of `str8to16` is `unsigned char`.** The target loads the
   byte `lbu`, tests THAT for the loop, and sign-extends it (`sll`/`sra` 24) only
   to pass it to `char8to16`, whose parameter is `char`. A `char` local loads
   `lb` and needs a second `lbu` for the call.
4. **`c - 'a' + 'A'`, not `c - 32`.** gcc folds the pair into the UNSIGNED 16-bit
   constant 65504, which does not fit a signed `addiu`, so `strupr16` comes out
   as a hoisted `li a1,0xffe0` plus `addu`. Spelled `- 32` it is one `addiu` and
   the function is one instruction wrong. `strlwr16`'s `c - 'A' + 'a'` folds to a
   positive 32 and is an `addiu` either way.
5. **`if (cond) return 1; return 0;`** — Phase 9's shape, again: `operator<`
   returns the `strcmp` sign that way, where `return strcmp(...) < 0;` gives the
   `srl 31` sign-bit trick.
6. Two guards that are one `||` in Phase 9's shape classes are **two separate
   `if`s** in `Init(const VIString &, int, int)`: the target branches the first
   test to a shared `return -1` block and falls the second into it, where the
   `||` form threads the second test straight to the epilogue.

## The varargs finding: `__builtin_next_arg` is not `va_start` here

The three `snprintf` wrappers are `va_start` + a call to the `v` form, and bare
`__builtin_next_arg(f)` lands **40 bytes too high** — `sp+128` where the target
has `sp+88`.

`SETUP_INCOMING_VARARGS` in gcc's `config/mips/abi64.h` puts the register save
block **below** `virtual_incoming_args` for the EABI (`ptr = virtual_incoming -
saved_gp_regs * 8`), while `__builtin_next_arg` returns `virtual_incoming +
named_stack_size`, i.e. the top of the block. The EE is an EABI target with 8 GPR
argument registers, so with three named arguments the gap is `(8 - 3) * 8 = 40`.
gcc's own `va-mips.h` closes it with `__builtin_args_info`, and so does this:

```c
typedef char *va_list;
#define va_start(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)
```

`__builtin_args_info(2)` is `CUM.arg_words` (measured: 3 for these functions) and
folds at compile time, so `va_start` stays a single `addiu`. All three wrappers
are byte-exact with it. No flag reaches this: `-mabi=eabi/32/o64/n32`, `-mgp32`,
`-mfp32`, `-mlong64`, `-mint64`, `-msingle-float` and `-mno-abicalls` were swept,
and `__builtin_saveregs()` emits a real `jal` on this target.

## A shipped inconsistency

**`strcat16(unsigned short *, unsigned int, const char *)` widens its bytes with a
plain sign-extending assignment** — there is no call in its loop at all — where
`str8to16`, the same loop one line different, runs every byte through
`char8to16`. Bytes 0x80..0xFF therefore land as negative UCS-2 values instead of
their CP1252 code points. It matches byte for byte written that way.

Precedent: `VIMatrix33::Translate` writing one row past the matrix (Phase 4), and
Phase 9's `min + max` `Scale` and degenerate `Store`.

## Open: one, and it is not the source

`vsnprintf16(unsigned short *, int, const char *, char *)`, **two words**: the
target sets up the alloca frame pointer (`move s8,sp`) in the delay slot of the
`strlen` call and materialises that call's argument before it; this build does
the opposite. **Eleven spellings** — declaration order, `char *` vs
`unsigned short *`, extern `alloca` vs `__builtin_alloca`, a named return
temporary, the size arithmetic — are byte-identical, so it is the instruction
scheduler, the family carried since Phase 5.
