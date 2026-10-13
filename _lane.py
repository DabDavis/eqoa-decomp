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
"""
import filecmp, shutil, subprocess, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
ROOT = HERE.parent

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
    print(f"lane {d}")

def changed(name):
    d = lane(name); out = []
    for p in sorted((d / "src").iterdir()):
        if p.name == "MATCHED.txt" or not p.is_file(): continue
        b = d / ".base" / p.name
        if not b.exists() or not filecmp.cmp(p, b, shallow=False):
            out.append(p.name)
    return out

def merge(name):
    d = lane(name); bad = []
    for f in changed(name):
        mine, base, theirs = HERE / "src" / f, d / ".base" / f, d / "src" / f
        if not mine.exists() or (base.exists() and filecmp.cmp(mine, base, shallow=False)):
            shutil.copy2(theirs, mine); print(f"  take   {f}"); continue
        if not base.exists():
            print(f"  CONFLICT {f}: new in lane and in main"); bad.append(f); continue
        r = subprocess.run(["git", "merge-file", "-L", "main", "-L", "base", "-L", name,
                            str(mine), str(base), str(theirs)])
        if r.returncode:
            print(f"  CONFLICT {f}: {r.returncode} hunk(s), markers left in src/{f}"); bad.append(f)
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
