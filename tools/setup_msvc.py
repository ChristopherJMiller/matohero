#!/usr/bin/env python3
"""Assemble a portable MSVC 7.1 SP1 (Visual Studio .NET 2003 SP1) toolchain in toolchain/msvc71.

Inputs are the two archives from https://archive.org/details/visual-studio-2003-unpacked:
  VisualStudio2003.zip      unpacked RTM install media (cl 13.10.3077)
  VisualStudio2003-sp1.zip  the official SP1 patch, VS7.1sp1-KB918007-X86.exe

The SP1 patch carries full replacement files (not deltas) in a cabinet embedded in the
exe, so we can overlay them on the RTM tree without running any installer.

Output layout (gitignored):
  toolchain/msvc71/bin            cl, c1, c1xx, c2, link, lib, ml, cvtres + mspdb71/msobj71
  toolchain/msvc71/include        CRT/STL headers
  toolchain/msvc71/lib            static CRT and import libs
  toolchain/msvc71/PlatformSDK/{Include,Lib}

Requires the dev shell (7z, msiextract, cabextract).
"""
import argparse
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

import pefile

ROOT = Path(__file__).resolve().parent.parent
DEST = ROOT / "toolchain" / "msvc71"

VS = "VisualStudio2003/Program Files/Microsoft Visual Studio .NET 2003"
RTM_DIRS = {f"{VS}/Vc7/bin": "bin", f"{VS}/Vc7/include": "include", f"{VS}/Vc7/lib": "lib"}
# Runtime DLLs the compiler and linker load, which live outside Vc7/bin.
RTM_IDE_DLLS = ["mspdb71.dll", "msobj71.dll", "msdis140.dll", "msvcr71.dll", "msvcp71.dll", "mspdbsrv.exe"]

EXPECTED_CL = "13.10.6030"


def run(*args, **kw):
    subprocess.run(args, check=True, stdout=subprocess.DEVNULL, **kw)


def file_version(path):
    pe = pefile.PE(str(path), fast_load=True)
    pe.parse_data_directories([pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_RESOURCE"]])
    for fi in getattr(pe, "VS_FIXEDFILEINFO", []):
        return "%d.%d.%d" % (fi.FileVersionMS >> 16, fi.FileVersionMS & 0xFFFF, fi.FileVersionLS >> 16)
    return None


def extract_rtm(rtm_zip, tmp):
    print("extracting RTM tree...")
    wanted = [f"{d}/*" for d in RTM_DIRS] + [f"{VS}/Common7/IDE/{n}" for n in RTM_IDE_DLLS] + ["VisualStudio2003/vs_setup.msi"]
    run("7z", "x", "-y", f"-o{tmp}", str(rtm_zip), *wanted)
    for src, dst in RTM_DIRS.items():
        shutil.copytree(tmp / src, DEST / dst, dirs_exist_ok=True)
    for name in RTM_IDE_DLLS:
        p = tmp / VS / "Common7/IDE" / name
        if p.exists():
            shutil.copy2(p, DEST / "bin" / name)


def extract_platform_sdk(tmp):
    """The Platform SDK is only in vs_setup.msi's embedded cabinets.

    msiextract mangles MSI DefaultDir syntax into names like
    'Vc7:Vc7|Vc7/PlatformSDK:PSDK|PlatformSDK/.:common/include/...'; the part after
    'PlatformSDK/.:' is the source layout ({common,x86}/{include,Lib,...}), which we fold
    into the installed layout (PlatformSDK/Include, PlatformSDK/Lib).
    """
    print("extracting Platform SDK from vs_setup.msi...")
    out = tmp / "msi"
    out.mkdir()
    run("msiextract", "-C", str(out), str(tmp / "VisualStudio2003/vs_setup.msi"))
    folds = {"include": "Include", "lib": "Lib", "bin": "Bin"}
    count = 0
    for f in out.rglob("*"):
        if not f.is_file():
            continue
        s = f.as_posix()
        if "PlatformSDK/.:" not in s:
            continue
        rel = s.split("PlatformSDK/.:", 1)[1].split("/")  # e.g. common/include/gl/GL.h
        if len(rel) < 3 or rel[0] not in ("common", "x86") or rel[1].lower() not in folds:
            continue
        if rel[1].lower() == "lib" and len(rel) > 3 and rel[2].upper() == "IA64":
            continue
        target = DEST / "PlatformSDK" / folds[rel[1].lower()] / Path(*rel[2:])
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(f, target)
        count += 1
    print(f"  {count} Platform SDK files")


def overlay_sp1(sp1_zip, tmp):
    print("extracting SP1 payload...")
    run("7z", "x", "-y", f"-o{tmp}", str(sp1_zip))
    exe = next(tmp.rglob("VS7.1sp1-KB918007-X86.exe"))
    data = exe.read_bytes()
    # The payload is the largest of the MS cabinets embedded in the exe.
    cabs = []
    for m in re.finditer(rb"MSCF\0\0\0\0", data):
        size = struct.unpack_from("<I", data, m.start() + 8)[0]
        cabs.append((size, m.start()))
    size, off = max(cabs)
    cab = tmp / "sp1.cab"
    cab.write_bytes(data[off:off + size])
    payload = tmp / "sp1"
    payload.mkdir()
    run("cabextract", "-q", "-d", str(payload), str(cab))

    # Payload files are named FL_<name with . -> _>[_N]_<ENU|___>_X86.<GUID>.
    entries = {}
    for p in payload.iterdir():
        key = re.sub(r"\.[0-9A-F_]{36}$", "", p.name, flags=re.I)
        entries.setdefault(key.lower(), []).append(p)

    replaced, ambiguous = [], []
    for f in sorted(DEST.rglob("*")):
        if not f.is_file() or f.is_relative_to(DEST / "PlatformSDK"):
            continue
        hits = []
        for stem in {f.name.replace(".", "_"), f.name}:
            pat = re.compile(rf"^fl_{re.escape(stem.lower())}(_\d+)?_(enu|___)_x86$")
            hits += [p for k, ps in entries.items() if pat.match(k) for p in ps]
        if not hits:
            continue
        # Prefer the unsuffixed (primary) entry; a lone suffixed variant is fine too.
        primary = [p for p in hits if not re.search(r"_\d+_(enu|___)_x86\.", p.name, re.I)]
        pick = primary if primary else hits
        if len(pick) > 1 and len({p.read_bytes() for p in pick}) > 1:
            ambiguous.append(f.relative_to(DEST))
            continue
        shutil.copy2(pick[0], f)
        replaced.append(f.relative_to(DEST))
    print(f"  SP1 replaced {len(replaced)} files: {', '.join(str(r) for r in replaced)}")
    for a in ambiguous:
        print(f"  warning: ambiguous SP1 variants for {a}, kept RTM")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("rtm_zip", type=Path, help="VisualStudio2003.zip")
    ap.add_argument("sp1_zip", type=Path, help="VisualStudio2003-sp1.zip")
    args = ap.parse_args()

    if DEST.exists():
        shutil.rmtree(DEST)
    DEST.mkdir(parents=True)
    with tempfile.TemporaryDirectory(dir=DEST.parent) as t:
        tmp = Path(t)
        (tmp / "rtm").mkdir()
        (tmp / "sp1").mkdir()
        extract_rtm(args.rtm_zip.resolve(), tmp / "rtm")
        extract_platform_sdk(tmp / "rtm")
        overlay_sp1(args.sp1_zip.resolve(), tmp / "sp1")

    versions = {n: file_version(DEST / "bin" / n) for n in ("cl.exe", "c1.dll", "c1xx.dll", "c2.dll", "link.exe")}
    for n, v in versions.items():
        print(f"  {n:10} {v}")
    if versions["cl.exe"] != EXPECTED_CL or versions["c2.dll"] != EXPECTED_CL:
        sys.exit(f"expected cl/c2 {EXPECTED_CL}; SP1 overlay incomplete")
    print(f"ok: toolchain/msvc71 is MSVC {EXPECTED_CL}")


if __name__ == "__main__":
    main()
