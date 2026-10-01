#!/usr/bin/env python3
"""Populate orig/ from a user-supplied copy of BIONICLE Heroes.

Accepts any of:
  * the retail PC disc as a .zip containing a MODE1/2352 .bin/.cue
  * a raw .bin (MODE1/2352) or a plain 2048-byte-sector .iso
  * the "No DRM" PC .iso (or the .zip around it)
  * the Sep 28 2006 PS2 prototype (.rar or .iso), for its symbols and linker map

Produces (all gitignored):
  orig/retail/       untouched SecuROM-wrapped exes + installer (when a retail disc is given)
  orig/game/         installed game tree (data, movies, DLLs) from the MSI/Main.cab
  orig/Bionicle.exe  the reference binary we decompile against (decrypted, see docs/binary-notes.md)
  orig/ps2proto/     prototype ELF with symbols/.mdebug, its linker map, and SLES/SYSTEM.CNF
  orig/nuconfig.txt  the UK controller config the installer puts next to the exe

Requires the dev shell (7z, msiextract, unar).
"""
import argparse
import hashlib
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ORIG = ROOT / "orig"

# sha1 of files we know about. The reference binary is what matching is measured against.
REFERENCE_SHA1 = "8f24d5a23af3e81f39fe733b05634eac690968c0"  # Bionicle.exe (decrypted)
KNOWN = {
    "8f24d5a23af3e81f39fe733b05634eac690968c0": "Bionicle.exe (decrypted, reference)",
    "193b43651052dd68a6e08d204baadd3de16c68fa": "bionicle_win2k.exe (decrypted)",
    "87c49d3e41903158bf58793a649febaac6d1f3e1": "bionicle.exe (retail, SecuROM)",
    "2128663ee5a121bd39343485da16824d390ec9dd": "bionicle_win2k.exe (retail, SecuROM)",
}

# Sep 28 2006 PS2 prototype (SLES_541.50 build): the debug ELF and its linker map.
PS2_FILES = {
    "BION~213.ELF": "dadcfab9fa07b169147179efdcdc753def0b7752",
    "BION~213.MAP": "5bad4b24929ab31d27ef59b01bf761702995296d",
    "SLES_541.50": "7c141981d88271804089d885efa6004ecc718e08",
    "SYSTEM.CNF": "f98ac0796a7ef1c0da0a456fb47dd5c7fb3a4a8f",
}

GAME_DIR_IN_ISO = "program files/Eidos/BIONICLE Heroes"


def sha1(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def run(*args):
    subprocess.run(args, check=True, stdout=subprocess.DEVNULL)


def bin_to_iso(src, dst):
    """Strip MODE1/2352 raw sectors down to their 2048 bytes of user data."""
    with open(src, "rb") as f, open(dst, "wb") as o:
        while sector := f.read(2352):
            if sector[15] != 1:
                sys.exit(f"{src}: non-MODE1 sector, unsupported image")
            o.write(sector[16:16 + 2048])


def to_iso(src, tmp):
    """Return a path to a 2048-byte-sector ISO for whatever we were given."""
    if src.suffix.lower() == ".rar":
        run("unar", "-q", "-f", "-o", str(tmp), str(src))
        candidates = sorted(tmp.rglob("*.iso"))
        if not candidates:
            sys.exit(f"{src}: no .iso inside")
        return candidates[0]
    if src.suffix.lower() == ".zip":
        run("7z", "e", "-y", f"-o{tmp}", str(src))
        candidates = sorted(tmp.glob("*.bin")) + sorted(tmp.glob("*.iso"))
        if not candidates:
            sys.exit(f"{src}: no .bin or .iso inside")
        src = candidates[0]
    if src.suffix.lower() == ".bin":
        iso = tmp / (src.stem + ".iso")
        bin_to_iso(src, iso)
        return iso
    return src


def list_iso(iso):
    out = subprocess.run(["7z", "l", "-slt", str(iso)], check=True, capture_output=True, text=True).stdout
    return [line[7:] for line in out.splitlines() if line.startswith("Path = ")]


def extract(iso, member, dest):
    dest.mkdir(parents=True, exist_ok=True)
    run("7z", "e", "-y", f"-o{dest}", str(iso), member)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("images", nargs="+", type=Path, help="disc images / zips (retail and/or No-DRM)")
    args = ap.parse_args()

    ORIG.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=ORIG) as t:
        tmp = Path(t)
        for i, image in enumerate(args.images):
            work = tmp / str(i)
            work.mkdir()
            iso = to_iso(image.resolve(), work)
            members = list_iso(iso)
            lower = {m.lower(): m for m in members}

            if "bion~213.elf" in lower:
                dest = ORIG / "ps2proto"
                for name, digest in PS2_FILES.items():
                    extract(iso, lower[name.lower()], dest)
                    got = sha1(dest / name)
                    note = "ok" if got == digest else f"UNEXPECTED sha1 {got}"
                    print(f"ps2 prototype {name}: {note}")
                continue

            # Decrypted exes: ViTALiTY/ on the retail rip, or in place on the No-DRM image.
            for name in ("Bionicle.exe", "bionicle_win2k.exe"):
                for cand in (f"vitality/{name}".lower(), f"{GAME_DIR_IN_ISO}/{name}".lower()):
                    if cand in lower:
                        extract(iso, lower[cand], work / "exe")
                        got = work / "exe" / Path(lower[cand]).name
                        digest = sha1(got)
                        print(f"{lower[cand]}: {KNOWN.get(digest, 'unknown')} [{digest}]")
                        if digest == REFERENCE_SHA1:
                            shutil.copy2(got, ORIG / "Bionicle.exe")
                        elif "win2k" in name and digest in KNOWN and "SecuROM" not in KNOWN[digest]:
                            shutil.copy2(got, ORIG / "bionicle_win2k.exe")
                        elif "SecuROM" in KNOWN.get(digest, ""):
                            (ORIG / "retail").mkdir(exist_ok=True)
                            shutil.copy2(got, ORIG / "retail" / name.lower())
                        got.unlink()

            # Per-language files the installer copies next to the exe; we use the UK set.
            cfg = f"{GAME_DIR_IN_ISO}/files_UK/nuconfig.txt".lower()
            if cfg in lower and not (ORIG / "nuconfig.txt").exists():
                extract(iso, lower[cfg], ORIG)
                print("nuconfig.txt (UK) -> orig/nuconfig.txt")

            # Installed game tree: needs the MSI and Main.cab side by side.
            if "bionicle heroes.msi" in lower and "main.cab" in lower and not (ORIG / "game").exists():
                inst = work / "inst"
                extract(iso, lower["bionicle heroes.msi"], inst)
                extract(iso, lower["main.cab"], inst)
                unpacked = work / "unpacked"
                unpacked.mkdir()
                subprocess.run(["msiextract", "-C", str(unpacked), str(inst / "BIONICLE Heroes.msi")],
                               check=True, stdout=subprocess.DEVNULL)
                tree = next(unpacked.glob("Program Files/Eidos/BIONICLE Heroes"))
                shutil.move(str(tree), ORIG / "game")
                print("installed game tree -> orig/game")

    ref = ORIG / "Bionicle.exe"
    if not ref.exists():
        sys.exit("reference Bionicle.exe not found in the given images")
    if (ORIG / "game").exists():
        # The installed tree ships the SecuROM exes; swap in the decrypted ones so it runs.
        for name in ("bionicle.exe", "bionicle_win2k.exe"):
            dec = ORIG / ("Bionicle.exe" if name == "bionicle.exe" else "bionicle_win2k.exe")
            if dec.exists():
                for old in (ORIG / "game").glob(name):
                    old.unlink()
                shutil.copy2(dec, ORIG / "game" / dec.name)
    print("ok: orig/Bionicle.exe verified")


if __name__ == "__main__":
    main()
