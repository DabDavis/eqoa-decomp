#!/usr/bin/env python3
"""
eqrtl -- ask an RTL dump WHICH PASS produced a codegen difference.

`eqcheck.py` says a function is N words wrong; `eqasm.py` says which words.
This says which of gcc's passes decided it, which is what a fix has to name.

The shipped cc1plus accepts `-da`, so this needs no compiler source: gcc 2.95
dumps every insn as `(insn[:MODE] N PREV NEXT (...)`, and that linked list IS
the emission order.  Follow one insn's neighbours across the pass dumps and the
pass that reordered it is the one where its neighbours change.

  ./eqrtl.py dumps src/vimatrix44d.cpp
  ./eqrtl.py funcs build/rtl/vimatrix44d BlendAccum
  ./eqrtl.py find  build/rtl/vimatrix44d 'BlendAccum(const VIVect3D' 'reg:DF 2 v0'
  ./eqrtl.py chain build/rtl/vimatrix44d 'BlendAccum(const VIVect3D' 25,27,48,50

`chain` prints the requested insns in emission order per pass and marks the
first pass where that order changes -- `>>` is the culprit.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent

# gcc 2.95's pass order.  The dump suffix is the pass name; a dump is absent
# when the pass is disabled, so missing files are skipped, never faked.
PASSES = ["rtl", "jump", "cse", "addressof", "gcse", "loop", "cse2", "flow",
          "combine", "regmove", "sched", "lreg", "greg", "flow2", "sched2",
          "dbr", "mach", "jump2"]

# `(insn:TI 48 25 27 (set ...`  /  `(call_insn/u 130 128 140 (parallel ...`
# Not anchored: some passes indent the insn, and the scheduling dumps print an
# insn nested inside another insn's text.  The nested form cannot match here --
# the outer `(insn/f:TI 4 (` has no three integers after the flags -- and the
# first match for a given number wins, so the outer chain is what is recorded.
INSN = re.compile(r"\((insn|call_insn|jump_insn|note|code_label|barrier)"
                  r"(?:/[a-z]+)*(?::[A-Z]{2})?"
                  r"\s+(\d+)\s+(\d+)\s+(\d+)\s")

# Notes, labels and barriers are recorded because the chain runs THROUGH them:
# walking `next` from one insn to the next lands on a note constantly, and a
# walk that only knows insns stops at the first one.  They are dropped from the
# reported order, not from the traversal.
REAL = {"insn", "call_insn", "jump_insn"}


def dump_paths(stem: str) -> list[tuple[str, Path]]:
    """(pass, path) for every dump belonging to one TU, in pass order."""
    p = Path(stem)
    base, d = p.name, p.parent if str(p.parent) != "" else Path(".")
    out = []
    for name in PASSES:
        for cand in (d / f"{base}.cc.{name}", d / f"{base}.c.{name}"):
            if cand.is_file():
                out.append((name, cand))
                break
    return out


def function_bodies(path: Path, want: str) -> list[tuple[str, list[str]]]:
    """Every function in `path` whose header contains `want`.

    Pass dumps do NOT agree on the name form: `.rtl` prints the full C++
    signature, `.gcse` and `.loop` print the bare name.  So two overloads are
    indistinguishable by name in half the dumps, and the caller disambiguates
    by which body actually holds the insns it asked about.
    """
    lines = path.read_text(errors="replace").splitlines()
    heads = [i for i, l in enumerate(lines) if l.startswith(";; Function")]
    out = []
    for k, s in enumerate(heads):
        if want not in lines[s]:
            continue
        e = heads[k + 1] if k + 1 < len(heads) else len(lines)
        out.append((lines[s], lines[s:e]))
    return out


def function_body(path: Path, want: str) -> list[str]:
    """The single function matching `want`; ambiguity is refused, not guessed."""
    hits = function_bodies(path, want)
    if not hits:
        raise SystemExit(f"{path.name}: no function matching {want!r}")
    if len(hits) > 1:
        names = "\n  ".join(h for h, _ in hits)
        raise SystemExit(f"{path.name}: {want!r} is ambiguous:\n  {names}")
    return hits[0][1]


def insn_chain(body: list[str]) -> tuple[dict[int, tuple[int, int]], list[int]]:
    """{insn: (prev, next)} for real insns, plus their emission order.

    Only the first occurrence of a number counts: REG_NOTES and the scheduling
    dumps repeat insn text inside other insns.
    """
    nodes: dict[int, tuple[int, int, str]] = {}
    for l in body:
        for m in INSN.finditer(l):
            n = int(m.group(2))
            if n not in nodes:
                nodes[n] = (int(m.group(3)), int(m.group(4)), m.group(1))

    # The head is the node nothing points to; fall back to file order if the
    # dump is truncated rather than silently reporting a one-element chain.
    heads = [n for n, (prev, _, _) in nodes.items() if prev not in nodes]
    at = heads[0] if heads else (next(iter(nodes), None))

    order, walked = [], set()
    while at is not None and at in nodes and at not in walked:
        walked.add(at)
        if nodes[at][2] in REAL:
            order.append(at)
        at = nodes[at][1]

    seen = {n: (p, x) for n, (p, x, k) in nodes.items() if k in REAL}
    return seen, order


def cmd_dumps(args) -> int:
    src = Path(args.source).resolve()
    out = Path(args.out or (HERE / "build" / "rtl")).resolve()
    out.mkdir(parents=True, exist_ok=True)

    sys.path.insert(0, str(HERE))
    import eqcheck as C

    # The dumps land beside the source, named after it, so compile a copy in
    # the output directory -- never next to src/, which is checked in.
    work = out / src.name
    work.write_bytes(src.read_bytes())
    for h in src.parent.glob("*.h"):
        (out / h.name).write_bytes(h.read_bytes())

    cmd = C.driver(args.toolchain) + ["-S"] + args.flags.split() + \
        ["-da", str(work), "-o", str(work.with_suffix(".s"))]
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=out)
    made = dump_paths(str(work.with_suffix("")))
    if not made:
        sys.stderr.write(r.stderr)
        raise SystemExit(f"no dumps produced in {out}")
    print(f"{len(made)} dumps in {out}: " + " ".join(p for p, _ in made))
    print(f"stem for the other subcommands: {work.with_suffix('')}")
    return 0


def cmd_funcs(args) -> int:
    dumps = dump_paths(args.stem)
    if not dumps:
        raise SystemExit(f"no dumps for stem {args.stem}")
    body = dumps[0][1].read_text(errors="replace").splitlines()
    for l in body:
        if l.startswith(";; Function") and (not args.filter or args.filter in l):
            print(l)
    return 0


def cmd_find(args) -> int:
    pat = re.compile(args.regex)
    dumps = dict(dump_paths(args.stem))
    name = args.pass_ if args.pass_ in dumps else "sched2"
    if name not in dumps:
        raise SystemExit(f"no {name} dump; have {' '.join(dumps)}")
    body = function_body(dumps[name], args.function)
    seen, order = insn_chain(body)
    cur = None
    for l in body:
        m = INSN.search(l)
        if m:
            cur = int(m.group(2))
        if cur is not None and pat.search(l):
            prev, nxt = seen.get(cur, (0, 0))
            print(f"  insn {cur:>5}  prev={prev:<5} next={nxt:<5} {l.strip()[:120]}")
            cur = None
    return 0


def cmd_chain(args) -> int:
    targets = [int(x) for x in args.insns.split(",")]
    rows, prev_order = [], None
    for name, path in dump_paths(args.stem):
        # Pick the overload that actually contains these insns -- the only
        # discriminator that works in the dumps that print bare names.  Later
        # passes renumber, so two bodies can hold the same numbers; that is
        # only a problem when they disagree about the ORDER.
        cands = []
        for head, body in function_bodies(path, args.function):
            seen, order = insn_chain(body)
            hits = sum(1 for t in targets if t in seen)
            if hits:
                pos = {n: i for i, n in enumerate(order)}
                key = tuple(sorted((t for t in targets if t in pos),
                                   key=lambda t: pos[t]))
                cands.append((hits, key, seen, order))
        if not cands:
            rows.append((name, "(none of these insns exist yet)", False))
            continue
        top = max(h for h, _, _, _ in cands)
        best_set = [c for c in cands if c[0] == top]
        if len({c[1] for c in best_set}) > 1:
            raise SystemExit(f"{path.name}: {args.function!r} matches two bodies "
                             f"that order these insns differently -- name it "
                             f"more precisely")
        best = (best_set[0][2], best_set[0][3])

        seen, order = best
        pos = {n: i for i, n in enumerate(order)}
        present = [t for t in targets if t in pos]
        cur = sorted(present, key=lambda t: pos[t])
        changed = prev_order is not None and cur != prev_order
        missing = [t for t in targets if t not in pos]
        text = " -> ".join(str(t) for t in cur)
        if missing:
            text += f"   [absent: {','.join(str(m) for m in missing)}]"
        rows.append((name, text, changed))
        prev_order = cur

    for name, text, changed in rows:
        print(f"{'>>' if changed else '  '} {name:<10} {text}")
    if not any(c for _, _, c in rows):
        print("\nno pass reorders these insns -- the difference is not ordering")
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    d = sub.add_parser("dumps", help="compile a TU with -da")
    d.add_argument("source")
    d.add_argument("-o", "--out")
    d.add_argument("-t", "--toolchain", default="ee-gcc2.95.3-107")
    d.add_argument("--flags", default="-O2 -G0 -fno-exceptions")
    d.set_defaults(fn=cmd_dumps)

    f = sub.add_parser("funcs", help="list the functions in a dump")
    f.add_argument("stem")
    f.add_argument("filter", nargs="?", default="")
    f.set_defaults(fn=cmd_funcs)

    g = sub.add_parser("find", help="find insns matching a regex")
    g.add_argument("stem")
    g.add_argument("function")
    g.add_argument("regex")
    g.add_argument("--pass", dest="pass_", default="sched2")
    g.set_defaults(fn=cmd_find)

    c = sub.add_parser("chain", help="trace insn order across passes")
    c.add_argument("stem")
    c.add_argument("function")
    c.add_argument("insns")
    c.set_defaults(fn=cmd_chain)

    args = p.parse_args(argv)
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
