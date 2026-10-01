#!/usr/bin/env python3
"""Build the recompiled Bionicle.exe (and its PDB) with MSVC 7.1 under Wine.

Every .c/.cpp under src/ is compiled with the flags of the library it belongs to, then
everything is linked into build/Bionicle.exe. Unresolved externals are expected while
the decomp is partial, so the link uses /FORCE:UNRESOLVED; the result is for comparing
against the original with reccmp, not for running.

  tools/build.py            incremental build
  tools/build.py --clean    rebuild everything
"""
import argparse
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
BUILD = ROOT / "build"
MSVC = ROOT / "tools" / "msvc"

# Settings recovered so far (see docs/binary-notes.md): optimized, frame pointers
# omitted, static multithreaded CRT. /Z7 keeps debug info in each .obj, so parallel
# compiles never contend for a shared PDB.
COMMON_FLAGS = ["/nologo", "/c", "/O2", "/MT", "/W3", "/Z7"]
CXX_FLAGS = ["/EHsc"]

# Original PC projects: each was compiled from its own directory, which is what the
# __FILE__ strings in asserts are relative to (".\\listman.c" from nu2api/gamelib,
# ".\\pc\\nurndr.c" from nu2api/nu3d, ".\\charsys\\charbase.cpp" from gameapi). A source
# belongs to the deepest project root containing it. Layout follows the PS2 prototype's
# tree (docs/ps2-prototype.md), with pc/ where PS2 has ps2/.
PROJECTS = {
    "nu2api/nucore": {"flags": []},
    "nu2api/numath": {"flags": []},
    "nu2api/nu3d": {"flags": []},
    "nu2api/nusound3": {"flags": []},
    "nu2api/nufmv": {"flags": []},
    "nu2api/gamelib": {"flags": []},
    "gameapi": {"flags": []},
    "gameapi/DynoSDK": {"flags": []},
    "game": {"flags": []},
}

LINK_FLAGS = [
    "/nologo", "/DEBUG", "/OPT:NOREF", "/OPT:NOICF", "/FORCE:UNRESOLVED",
    "/SUBSYSTEM:WINDOWS", "/BASE:0x400000", "/FIXED",
]


def winpath(p):
    return "Z:" + str(p).replace("/", "\\")


def project_of(src):
    rel = src.relative_to(SRC).as_posix()
    roots = [p for p in PROJECTS if rel.startswith(p + "/")]
    if not roots:
        sys.exit(f"{src.relative_to(ROOT)} is not under any project root in tools/build.py")
    return max(roots, key=len)


def sources():
    for src in sorted(SRC.rglob("*")):
        # Hidden directories hold scratch files (tools/permute.py variants).
        if any(part.startswith(".") for part in src.relative_to(SRC).parts):
            continue
        if src.suffix.lower() in (".c", ".cpp"):
            proj = project_of(src)
            yield proj, PROJECTS[proj], src


def headers_mtime():
    return max((h.stat().st_mtime for h in SRC.rglob("*.h")), default=0)


def compile_source(src, obj):
    """Compile one source to obj as the original build would. Returns (ok, compiler output)."""
    lib = project_of(src)
    cfg = PROJECTS[lib]
    obj.parent.mkdir(parents=True, exist_ok=True)
    flags = COMMON_FLAGS + cfg["flags"] + (CXX_FLAGS if src.suffix.lower() == ".cpp" else [])
    # Compile from the library root with a relative path, as the original build did
    # (asserts embed __FILE__ as e.g. ".\\listman.c").
    rel = ".\\" + str(src.relative_to(SRC / lib)).replace("/", "\\")
    # Every project root is on the include path, as the original builds pointed at the
    # engine's directories.
    includes = [f"/I{winpath(SRC / p)}" for p in [lib] + [p for p in PROJECTS if p != lib] if (SRC / p).exists()]
    cmd = [str(MSVC), "cl", *flags, *includes, f"/Fo{winpath(obj)}", rel]
    res = subprocess.run(cmd, cwd=SRC / lib, capture_output=True, text=True)
    out = "\n".join(l for l in res.stdout.splitlines() if l.strip() and l.strip() != src.name)
    return res.returncode == 0, out


def compile_many(srcs, outdir, extra_includes=(), project=None):
    """Compile several sources of one project in a single cl run (Wine startup dominates
    a compile, so batching matters). Objects land in outdir as <stem>.obj. Returns the
    compiler output."""
    lib = project or project_of(srcs[0])
    cfg = PROJECTS[lib]
    outdir.mkdir(parents=True, exist_ok=True)
    flags = COMMON_FLAGS + cfg["flags"] + (CXX_FLAGS if srcs[0].suffix.lower() == ".cpp" else [])
    # Sources outside the project (scratch variants) compile by absolute path.
    rels = [".\\" + str(s.relative_to(SRC / lib)).replace("/", "\\") if s.is_relative_to(SRC / lib)
            else winpath(s) for s in srcs]
    includes = [f"/I{winpath(d)}" for d in extra_includes]
    includes += [f"/I{winpath(SRC / p)}" for p in [lib] + [p for p in PROJECTS if p != lib] if (SRC / p).exists()]
    # Arguments go in a response file: Wine crashes at startup (exit -11, no output) on
    # long command lines.
    rsp = outdir / "cl.rsp"
    rsp.write_text("\n".join([*flags, *includes, f"/Fo{winpath(outdir)}\\"] + [f'"{r}"' for r in rels]) + "\n")
    cmd = [str(MSVC), "cl", f"@{winpath(rsp)}"]
    return subprocess.run(cmd, cwd=SRC / lib, capture_output=True, text=True).stdout


def compile_one(lib, cfg, src, force):
    obj = BUILD / "obj" / src.relative_to(SRC).with_suffix(".obj")
    newest_input = max(src.stat().st_mtime, headers_mtime())
    if not force and obj.exists() and obj.stat().st_mtime >= newest_input:
        return obj, None
    ok, out = compile_source(src, obj)
    if not ok:
        return None, f"{src.relative_to(ROOT)}:\n{out}"
    return obj, (out or None)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--clean", action="store_true", help="remove build/ first")
    ap.add_argument("-j", type=int, default=os.cpu_count(), help="parallel compiles")
    args = ap.parse_args()

    if args.clean and BUILD.exists():
        shutil.rmtree(BUILD)
    BUILD.mkdir(exist_ok=True)

    jobs = list(sources())
    with ThreadPoolExecutor(args.j) as pool:
        results = list(pool.map(lambda j: compile_one(*j, args.clean), jobs))
    failed = False
    for obj, msg in results:
        if msg:
            print(msg)
        failed |= obj is None
    if failed:
        sys.exit("compile failed")

    objs = [obj for obj, _ in results]
    exe = BUILD / "Bionicle.exe"
    rsp = BUILD / "link.rsp"
    rsp.write_text("\n".join(
        LINK_FLAGS
        + [f"/OUT:{winpath(exe)}", f"/PDB:{winpath(BUILD / 'Bionicle.pdb')}", f"/MAP:{winpath(BUILD / 'Bionicle.map')}"]
        + [f'"{winpath(o)}"' for o in objs]
        + ["kernel32.lib", "user32.lib", "gdi32.lib", "advapi32.lib"]
    ) + "\n")
    res = subprocess.run([str(MSVC), "link", f"@{winpath(rsp)}"], capture_output=True, text=True)
    # Unresolved externals are expected (LNK4088/LNK2001/LNK2019 under /FORCE).
    noise = ("LNK2001", "LNK2019", "LNK4088")
    lines = [l for l in res.stdout.splitlines() if l.strip() and not any(n in l for n in noise)]
    unresolved = sum(1 for l in res.stdout.splitlines() if "LNK2001" in l or "LNK2019" in l)
    if lines:
        print("\n".join(lines))
    if not exe.exists() or res.returncode not in (0, 1):
        sys.exit("link failed")
    print(f"built {exe.relative_to(ROOT)} from {len(objs)} objects ({unresolved} unresolved externals)")
    # Float constants in the original, for reccmp (see tools/floatconsts.py).
    subprocess.run([sys.executable, str(ROOT / "tools" / "floatconsts.py")], check=True)


if __name__ == "__main__":
    main()
