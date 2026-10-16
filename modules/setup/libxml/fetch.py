#!/usr/bin/env python3
"""Put SETUP's libxml sources in place: upstream libxml2 2.3.6 plus the game's
own edits (`eqoa.patch`), in `modules/setup/src/libxml/`.

  ./fetch.py                      # download the release tarball and patch it
  ./fetch.py --tarball FILE       # use a tarball you already have

SETUP links libxml2 2.3.6 (`xmlParserVersion` is "20306"), built in the game's
tree as `C:/code-frontiers-gm/Patcher/libxml/`.  Upstream source compiled with
the decomp's toolchain matches the image once the PS2 build's configuration
(`src/libxml/ps2/config.h`) and four small source edits are in place; those are
this repository's.  The upstream files themselves are NOT in the repository
(libxml2 2.3.6 is "LGPL or the W3C IPR"), so this script fetches the official
release, checks its SHA-256, and extracts only the files the build compiles.
Nothing from the archive is run.
"""
import argparse, hashlib, io, subprocess, sys, tarfile, urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent
DEST = HERE.parent / "src" / "libxml"
URL = "https://download.gnome.org/sources/libxml2/2.3/libxml2-2.3.6.tar.gz"
SHA256 = "634663bb53c46ca3fde80bcfbbbc9b083eff14f6f41397e7c142002d2929acff"
TOP = "libxml2-2.3.6/"

# The files SETUP links, in no particular order (testXPath.c supplies the
# `main` the module carries), and every public header.
SOURCES = ("HTMLparser HTMLtree SAX debugXML encoding entities error hash list "
           "parser parserInternals testXPath tree uri valid xinclude xlink xmlIO "
           "xmlmemory xpath xpointer").split()


def fetch(tarball):
    if tarball:
        data = Path(tarball).read_bytes()
    else:
        print(f"downloading {URL}")
        with urllib.request.urlopen(URL, timeout=120) as r:
            data = r.read()
    got = hashlib.sha256(data).hexdigest()
    if got != SHA256:
        raise SystemExit(f"SHA-256 mismatch: {got}, expected {SHA256}")
    return data


def extract(data):
    want = {f"{TOP}{s}.c": DEST / f"{s}.c" for s in SOURCES}
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tf:
        for m in tf.getmembers():
            if not m.isfile():
                continue
            if m.name in want:
                out = want.pop(m.name)
            elif m.name.startswith(TOP + "include/libxml/") and m.name.endswith(".h") \
                    and "/" not in m.name[len(TOP + "include/libxml/"):]:
                out = DEST / "include" / "libxml" / m.name.rsplit("/", 1)[1]
            else:
                continue
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(tf.extractfile(m).read())
    if want:
        raise SystemExit(f"missing from the tarball: {sorted(want)}")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tarball", help="a local copy of libxml2-2.3.6.tar.gz")
    a = ap.parse_args()
    extract(fetch(a.tarball))
    r = subprocess.run(["patch", "-p1", "-s", "-d", str(DEST), "-i", str(HERE / "eqoa.patch")],
                       capture_output=True, text=True)
    if r.returncode:
        raise SystemExit("patch failed:\n" + r.stdout + r.stderr)
    print(f"libxml2 2.3.6 + eqoa.patch -> {DEST}")


if __name__ == "__main__":
    sys.exit(main())
