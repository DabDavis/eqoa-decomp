#!/usr/bin/env python3
"""Isolated work lanes, so more than one function can be ground at a time.

Every scorer here shares `build/` (and MATCHED.txt), and two at once invent
regressions.  A lane is a sibling directory `../decomp-<name>/` with its own
copy of `src/` and its own `build/`, the tooling copied (so HERE is the lane)
and the toolchains symlinked.  `.base/` is the src snapshot the lane started
from, which is what makes the merge back a THREE-way merge.

  ./_lane.py new   <name>    # create (or reset) a lane from the current src/
  ./_lane.py diff  <name>    # which files the lane changed
  ./_lane.py merge <name>    # 3-way merge lane src -> main src (never MATCHED.txt)
  ./_lane.py sync  <name>    # lane src and base := main src (lane must be idle)

With EQMODULE set to another module (SETUP, CLIENT, ...), `new` also copies
modules/<m>/src and modules/<m>/symbols.txt into the lane (snapshot in
.base-<m>/), and `diff`/`merge` cover those too -- symbols.txt included, since
a lane renames the entries of the functions it writes.
"""
import filecmp, os, shutil, subprocess, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
MOD = os.environ.get("EQMODULE", "SUPPORT").upper()
MODDIR = None if MOD == "SUPPORT" else Path("modules") / MOD.lower()

def _modfiles(root):
    """(relative path) of every file a module lane carries: src/ and symbols.txt."""
    if MODDIR is None: return []
    out = [MODDIR / "symbols.txt"]
    src = root / MODDIR / "src"
    if src.exists():
        out += [p.relative_to(root) for p in sorted(src.rglob("*"))
                if p.is_file() and p.name != "MATCHED.txt"]
    return out

def lane(name): return ROOT / f"decomp-{name}"

def new(name):
    d = lane(name)
    if d.exists():
        shutil.rmtree(d)
    d.mkdir()
    for p in HERE.glob("*.py"):
        shutil.copy2(p, d / p.name)
    for link in ("toolchains", "tools", "README.md"):
        (d / link).symlink_to(HERE / link)
    shutil.copytree(HERE / "src", d / "src")
    shutil.copytree(HERE / "src", d / ".base")
    (d / "build").mkdir()
    if MODDIR is not None:
        (d / MODDIR).mkdir(parents=True)
        shutil.copytree(HERE / MODDIR / "src", d / MODDIR / "src")
        shutil.copy2(HERE / MODDIR / "symbols.txt", d / MODDIR / "symbols.txt")
        (d / MODDIR / "build").mkdir()
        base = d / (".base-" + MOD.lower())
        shutil.copytree(HERE / MODDIR / "src", base / "src")
        shutil.copy2(HERE / MODDIR / "symbols.txt", base / "symbols.txt")
    print(f"lane {d}")

def _base(d, rel):
    """Where a lane file's snapshot lives: .base/ for src/, .base-<m>/ for a module's."""
    rel = Path(rel)
    if MODDIR is not None and rel.parts[:2] == MODDIR.parts:
        return d / (".base-" + MOD.lower()) / Path(*rel.parts[2:])
    return d / ".base" / rel.relative_to("src")

def changed(name):
    d = lane(name); out = []
    for p in sorted((d / "src").iterdir()):
        if p.name == "MATCHED.txt" or not p.is_file(): continue
        b = d / ".base" / p.name
        if not b.exists() or not filecmp.cmp(p, b, shallow=False):
            out.append("src/" + p.name)
    for rel in _modfiles(d):
        p, b = d / rel, _base(d, rel)
        if not b.exists() or not filecmp.cmp(p, b, shallow=False):
            out.append(str(rel))
    return out

def _symmerge(mine, base, theirs):
    """Per-address 3-way merge of a symbols.txt into `mine`; returns the conflict count.
    A row is keyed by (address, kind).  A side that left a row as in base yields to the
    other; a real name beats a func_/data_ placeholder; two different real names are a
    conflict (main's is kept and both are printed)."""
    import re
    def rows(p):
        out = []
        for l in p.read_text().split("\n"):
            m = re.match(r"^(0x[0-9A-Fa-f]{8})\s+\[(\w+)", l)
            out.append(((m.group(1).upper(), m.group(2)) if m else None, l))
        return out
    ph = lambda l: bool(re.search(r"\]\s+(func|data)_[0-9A-Fa-f]{8}\b", l))
    M, B, L = rows(mine), rows(base), rows(theirs)
    bd = {k: l for k, l in B if k}; ld = {k: l for k, l in L if k}
    out, bad = [], 0
    for k, l in M:
        b, t = bd.get(k), ld.get(k)
        if not k or t is None or t == l or t == b: out.append(l); continue
        if l == b or (ph(l) and not ph(t)): out.append(t); continue
        if ph(t) and not ph(l): out.append(l); continue
        print(f"    {k[0]}: main `{l.strip()}` vs lane `{t.strip()}`"); bad += 1; out.append(l)
    mine.write_text("\n".join(out))
    return bad

def merge(name):
    d = lane(name); bad = []
    for f in changed(name):
        mine, base, theirs = HERE / f, _base(d, f), d / f
        if not mine.exists() or (base.exists() and filecmp.cmp(mine, base, shallow=False)):
            mine.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(theirs, mine); print(f"  take   {f}"); continue
        if filecmp.cmp(mine, theirs, shallow=False):
            print(f"  same   {f}"); continue
        if not base.exists():
            print(f"  CONFLICT {f}: new in lane and in main"); bad.append(f); continue
        if mine.name == "symbols.txt":
            # One row per address: merge per ADDRESS, so two lanes renaming
            # neighbouring rows never collide the way a line merge makes them.
            if _symmerge(mine, base, theirs):
                print(f"  CONFLICT {f}: per-address, see above"); bad.append(f)
            else:
                print(f"  merged {f} (per address)")
            continue
        r = subprocess.run(["git", "merge-file", "-L", "main", "-L", "base", "-L", name,
                            str(mine), str(base), str(theirs)])
        if r.returncode:
            print(f"  CONFLICT {f}: {r.returncode} hunk(s), markers left in {f}"); bad.append(f)
        else:
            print(f"  merged {f}")
    return 1 if bad else 0

def _caps(path):
    """The SPAN_CAP / SPAN_IGNORE entries a copy of eqdecomp.py holds."""
    import re
    t = path.read_text()
    return set(re.findall(r'^\s*"([^"]+)":\s*(\d+)', t, re.M)) | \
           set(re.findall(r'^\s*"([^"]+)",\s*$', t, re.M))

def sync(name, force=False):
    d = lane(name)
    # A lane's eqdecomp.py is copied over below, and the 3-way merge never
    # carries SPAN_CAPs back -- so refuse while the lane holds one main lacks.
    extra = _caps(d / "eqdecomp.py") - _caps(HERE / "eqdecomp.py")
    if extra and not force:
        raise SystemExit(f"lane {name} has SPAN entries main lacks -- port them "
                         f"first (or --force):\n  " + "\n  ".join(map(str, sorted(extra))))
    for sub in ("src", ".base"):
        shutil.rmtree(d / sub); shutil.copytree(HERE / "src", d / sub)
    for p in HERE.glob("*.py"):
        shutil.copy2(p, d / p.name)
    print(f"lane {d} synced")

if __name__ == "__main__":
    cmd, name = sys.argv[1], sys.argv[2]
    if cmd == "new": new(name)
    elif cmd == "diff": print("\n".join(changed(name)))
    elif cmd == "merge": sys.exit(merge(name))
    elif cmd == "sync": sync(name, "--force" in sys.argv)
    else: raise SystemExit(__doc__)
