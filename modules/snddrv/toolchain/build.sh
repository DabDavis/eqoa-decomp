#!/bin/bash
# The IOP compiler: gcc 2.95.3, built as a mipsel-elf cross cc1plus with one
# target constant changed, run with -fno-weak -fno-exceptions.
#
#   modules/snddrv/toolchain/build.sh [workdir]     # ~3 minutes; installs
#                                                    # toolchains/iop-gcc2.95.3/
#
# SNDDRV.IRX is C++ compiled at -O0 for the IOP's R3000 (mips1) and assembled by
# an SN Systems assembler (modelled by ./iop-as, on ps2eeas).  What picks each,
# measured on bgm.cpp and driver.cpp (49 functions, all byte-exact):
#   * C++, not C: every function ends `j L; nop` THREE times before its epilogue
#     -- the return, then cc1plus's cleanup and return labels at -O0.  cc1 (C)
#     emits one.
#   * gcc 2.95, without weak symbols (-fno-weak): driver.cpp's tail is the
#     synthesized XmSlot ctor and dtor, __static_initialization_and_destruction_0,
#     the un-inlined inline accessors in declaration order, then _GLOBAL_$I$/$D$,
#     all in .text.  With weak support they go to .gnu.linkonce.t.*; egcs 1.1.2
#     emits them in reverse and has no __static_initialization function; 2.8.1
#     and egcs 1.0.3a also get array-of-struct field addresses wrong
#     (`sw $0,4($2)` where the target folds the offset: `la $3,gXmSlot+4`).
#   * -fno-exceptions, as on the EE: with exceptions the XmSlot ctor carries a
#     cleanup region the target does not have.
#   * BIGGEST_ALIGNMENT 32, not mips.h's 64: a struct local's stack slot is NOT
#     rounded up to 8 (SdInitBgmSystem's iop_thread_t is 20 bytes and the int
#     after it sits at +36, not +40).  An R3000 has no 64-bit type; the IOP
#     build of the compiler evidently said so.  Every stock version pads.
# The compiler that built SNDDRV is not preserved; this is stock gcc 2.95.3
# with that one constant changed.
#
# Host fixes (as toolchains/src/build.sh needs for the EE rig): build for i686,
# gnu89, -fcommon, and obstack.h's cast-as-lvalue rewritten (gcc 4+ rejects it).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
DEST=$(cd "$HERE/../../.." && pwd)/toolchains/iop-gcc2.95.3
W=${1:-${XDG_CACHE_HOME:-$HOME/.cache}/iop-gcc2.95.3}
mkdir -p "$W" && cd "$W"
[ -f gcc-2.95.3.tar.gz ] || curl -sSLo gcc-2.95.3.tar.gz https://ftp.gnu.org/gnu/gcc/gcc-2.95.3.tar.gz
rm -rf gcc-2.95.3 && tar xzf gcc-2.95.3.tar.gz && chmod -R u+w gcc-2.95.3
cd gcc-2.95.3
python3 - include/obstack.h <<'EOF'
import sys, pathlib
for f in sys.argv[1:]:
    p = pathlib.Path(f)
    if not p.exists():
        continue
    s = p.read_text()
    for a, b in [("*((void **)__o->next_free)++ = ((void *)datum);",
                  "*((void **)__o->next_free) = ((void *)datum); __o->next_free += sizeof (void *);"),
                 ("*((int *)__o->next_free)++ = ((int)datum);",
                  "*((int *)__o->next_free) = ((int)datum); __o->next_free += sizeof (int);"),
                 ("(*((void **) (h)->next_free)++ = (void *)aptr)",
                  "(*((void **) (h)->next_free) = (void *)aptr, (h)->next_free += sizeof (void *), (void) 0)"),
                 ("(*((int *) (h)->next_free)++ = (int) aint)",
                  "(*((int *) (h)->next_free) = (int) aint, (h)->next_free += sizeof (int), (void) 0)")]:
        s = s.replace(a, b)
    p.write_text(s)
EOF
sed -i 's/^#define BIGGEST_ALIGNMENT 64/#define BIGGEST_ALIGNMENT 32/' gcc/config/mips/mips.h
grep -q '^#define BIGGEST_ALIGNMENT 32' gcc/config/mips/mips.h
H="gcc -m32"
F="-std=gnu89 -w -fcommon -O1"
(cd libiberty && CC="$H" CFLAGS="$F" ./configure --build=i686-pc-linux-gnu \
    --host=i686-pc-linux-gnu >configure.out 2>&1 && make CC="$H" CFLAGS="$F" >make.out 2>&1)
cd gcc
CC="$H" CFLAGS="$F" bash ./configure --build=i686-pc-linux-gnu --host=i686-pc-linux-gnu \
    --target=mipsel-unknown-elf --enable-languages=c++ --disable-nls >configure.out 2>&1 || true
# configure writes configure.lang only inside its language loop; re-run config.status
: > configure.lang && bash ./config.status >/dev/null 2>&1
grep -q 'mips/elfl.h' tm.h || { echo "tm.h is not mipsel-elf"; exit 1; }
touch c-parse.y && sleep 1 && touch c-parse.c c-parse.h c-gperf.h 2>/dev/null || true
make CC="$H" CFLAGS="$F" LDFLAGS="-m32" cc1 cc1plus cccp >make.out 2>&1 \
    || { tail -20 make.out; exit 1; }
mkdir -p "$DEST/lib" "$DEST/bin"
cp cc1 cc1plus cccp "$DEST/lib/"
cp "$HERE/iop-gcc" "$HERE/iop-as" "$DEST/bin/"
chmod +x "$DEST/bin/iop-gcc" "$DEST/bin/iop-as"
echo "installed $DEST"
