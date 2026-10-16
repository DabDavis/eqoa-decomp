#!/usr/bin/env python3
"""Put STATION's crypto library sources in place, in `modules/station/src/crypto/`:

  ./fetch.py                  # download the pinned upstream files, verify, patch
  ./fetch.py --from DIR       # use copies you already have (DIR/libdes, DIR/ltc)

STATION links three C libraries (0x002D1F68 .. 0x002DC7F0):

* **libdes 3.24, from SSLeay 0.6.3** (Eric Young; data: "libdes v 3.24 - 20-Apr-1996
  - eay", "DES part of SSLeay 0.6.3 06-Aug-1996").  No SSLeay 0.6.3 tarball is online;
  FreeBSD imported exactly that DES as secure/lib/libdes (freebsd-src commit
  43e844cb021988dbc2482cca450042d3c7cd9c15, 1996-08-10, "Next version from the author -
  from SSLeay.0.6.3").  Used UNMODIFIED.
* **LibTomCrypt's rijndael and MPI**, from a release before v0.67 (Sept 2002): the image
  keeps the per-file ID_TAGs v0.67 removed and the string error API.  No release before
  0.75 survives, so this fetches 0.75 (github.com/libtom/libtomcrypt commit
  a6a5fc648b43b8d4e2685fab4c50753faee365fc, "added libtomcrypt-0.75") and applies
  `station-ltc.patch`, which is ours: the older API and options the image measures
  (string errors and CRYPT_ERROR, no ARGCHK, endian-neutral, the options in
  mycrypt_cfg.h, MPI with MP_MACRO/MP_LOGTAB/MP_IOFUNC 0) and the 32-bit key and table
  types SOE's copy had (`unsigned int`; the `UL` literals stay 64-bit, as the image's
  64-bit multiply in imix_col shows).

The upstream files themselves are NOT in the repository; this script fetches them, checks
each one's SHA-256, writes only the files the build compiles or includes, and patches.
Nothing downloaded is run.  Our own files beside them (des_soe.c, aes_soe.c, ps2/) are
committed and left alone.
"""
import argparse, hashlib, subprocess, sys, urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent
DEST = HERE.parent / "src" / "crypto"

LIBDES_URL = ("https://raw.githubusercontent.com/freebsd/freebsd-src/"
              "43e844cb021988dbc2482cca450042d3c7cd9c15/secure/lib/libdes/")
LTC_URL = ("https://raw.githubusercontent.com/libtom/libtomcrypt/"
           "a6a5fc648b43b8d4e2685fab4c50753faee365fc/")

LIBDES = {
    "cbc_cksm.c": "264807fc8dad4f5769490bf6caa927fba50a971ee3f1bf07cd7de8f7da558224",
    "des.h": "eed716e8be343eace445990acf039c0396ffec0ca04cdcedaf103cde6573af20",
    "des_locl.h": "d59e164e4d21f4fb69a0085443458d419240ef8f26b44e46223a7fe8b10e1550",
    "ecb_enc.c": "1eab401e09cbd1e79016dd914bd7cb837c82cfb922822d385736ef78e4a3e7b2",
    "ncbc_enc.c": "452a4fccaf069941deceaac293d161417b60c572610dc740256435c157c622bd",
    "podd.h": "a662cefea931b6aa822fbf878db58439bd13bb53f2a289efae033c144a9d8106",
    "set_key.c": "85335a1fe24dce8dbc8e733826d9a5ba0337b50ca3073bcb831f46943d7ed908",
    "sk.h": "ba424877f9081993355cd1426cd806c90a972f8419652f171fbe36d4cd9b2834",
    "spr.h": "7566277566efeefdc43c0350dca988d4295451d87e4eac1423c09c128e0e7797",
    "str2key.c": "7a49df10a8cda37afdc3e7b5090595176479ea7e9208e1dbc1eaf50877bdd6e0",
    "supp.c": "6ea3bf87efa755800f3eab2d66f1d6e10b1beddf3d34637a9e824a097636c450",
}
LTC = {
    "aes.c": "2e5635088d6a90b4779e5b565721cf3e1e3e7c923d8640474b995692eb8bafd6",
    "aes_tab.c": "f544e102201dbbda7507ffd499091243ea9b623d025457fe11d21265f9d1708f",
    "mpi.c": "14d61c6fea11ee3191da4af1de28879d03bf90442e7c8888167c2ec81ecfe64f",
    "mpi-config.h": "2333a7be3daa433746a975cf46cb0ecefc18c11027c327df25852468e3cf6ff6",
    "mpi.h": "0873cde434ab7d2a103324a494136a231ae3477589a331ef595a5a99a7e36f4d",
    "mpi-types.h": "4a0531eae7eee2a200fa7a7e3fe2ee98663eb7b421fa698671d7345048e4074f",
    "mycrypt_argchk.h": "abe1cd6e6ac2e56a19f00ab3e7d0eae25a8976ef34688fb20d8efc3803a5d1a2",
    "mycrypt_cfg.h": "b78fa0465828c532aa942e3af73a28a7ab4368a8f45d53bfa3b79afc75477b8f",
    "mycrypt_cipher.h": "f41936d4f9eeea5211ee7bb0566d0497adca7fa812964e7b7cfd9231b0225cd9",
    "mycrypt_gf.h": "b16c2a44dbb72f6759fa50d98afea56b0994d11095a816802a2a38599eb615c6",
    "mycrypt.h": "21dae909f9297cf9cd404264c5fea3457abe3773002d42e5928449536f89a1f5",
    "mycrypt_hash.h": "22795e0c81356f3a1b61a3c9ca85504be2e41d0efa4c4134079ea36866d52036",
    "mycrypt_kr.h": "e0543caa4a688a187aff9bd3cc68568ebd5731f4029ce8e1792a8456f44144cb",
    "mycrypt_macros.h": "7ae050a6be706e928c398512d5799c518576b8656e206ca4f1dd4e6e6b5ea463",
    "mycrypt_misc.h": "632720d8d813939e486c4d7e805c871a81e261283adac30c5064ec6e96451601",
    "mycrypt_pk.h": "af80d7e34e6ae27b35db22be2374153deec4b4edf0ee713c9a0d00c49ddb28c5",
    "mycrypt_prng.h": "85142dad33c55a739e371c39c9dec9207efe227302589fdd1bfbd00f106d73c3",
}

# Every file this script writes: what _publish.py must leave out of the export.
FILES = sorted(LIBDES) + sorted(LTC)


def get(name, url, local):
    if local:
        return local.read_bytes()
    with urllib.request.urlopen(url, timeout=120) as r:
        return r.read()


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--from", dest="src", help="a directory holding libdes/ and ltc/ copies")
    a = ap.parse_args()
    src = Path(a.src) if a.src else None
    DEST.mkdir(parents=True, exist_ok=True)
    for table, url, sub in ((LIBDES, LIBDES_URL, "libdes"), (LTC, LTC_URL, "ltc")):
        for name, want in table.items():
            data = get(name, url + name, src / sub / name if src else None)
            got = hashlib.sha256(data).hexdigest()
            if got != want:
                raise SystemExit(f"{name}: SHA-256 {got}, expected {want}")
            (DEST / name).write_bytes(data)
    r = subprocess.run(["patch", "-p1", "-s", "-d", str(DEST), "-i", str(HERE / "station-ltc.patch")],
                       capture_output=True, text=True)
    if r.returncode:
        raise SystemExit("patch failed:\n" + r.stdout + r.stderr)
    print(f"libdes 3.24 (SSLeay 0.6.3) + LibTomCrypt 0.75 + station-ltc.patch -> {DEST}")


if __name__ == "__main__":
    sys.exit(main())
