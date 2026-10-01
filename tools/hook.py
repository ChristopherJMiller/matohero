#!/usr/bin/env python3
"""Build the hook DLL: matched decompiled code that runs inside the original game.

The DLL is named xinput1_1.dll (the game imports it; the real API is forwarded to
xinput1_3). On load it patches each hooked function in the exe with a jump to its
recompiled copy, so decompiled code gets exercised in-game long before the whole game
is decompiled.

What goes in:
  * objects from the matching build (tools/build.py) whose annotated functions all match
    (exact or effective, per reccmp) and that define no annotated globals or stubs;
  * glue.asm (generated): everything those objects reference that still lives in the
    exe, at its original address. Data via absolute symbols (link resolves DIR32 to
    them as-is); functions via `jmp [slot]` thunks, because link treats an absolute
    symbol as an RVA for REL32 calls;
  * tools/hook/hookmain.c: DllMain applies the bh_hooks table from glue.asm, and the
    XInput entry points the game imports pass through to xinput1_3.

The DLL is linked /FIXED at 0x10000000 so those absolute references never need
relocating.

  tools/hook.py              build build/hook/xinput1_1.dll
  tools/hook.py --install    also copy it into run/game (see tools/rungame)
"""
import argparse
import csv
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
OUT = BUILD / "hook"
MSVC = ROOT / "tools" / "msvc"
HOOK_SRC = ROOT / "tools" / "hook"
FUNCS = ROOT / "ghidra" / "export" / "functions.csv"
DLL_BASE = 0x10000000
MIN_PATCH = 5  # a jmp rel32; shorter functions are left alone

MAP_LINE = re.compile(r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s+(f\s+)?(?:i\s+)?(\S+)\s*$")
MARKER = re.compile(r"^\s*// (FUNCTION|STUB|GLOBAL|LIBRARY|TEMPLATE|SYNTHETIC): BIONICLE (0x[0-9a-fA-F]+)")


def winpath(p):
    return "Z:" + str(p).replace("/", "\\")


def msvc(*args, cwd=None):
    return subprocess.run([str(MSVC), *args], cwd=cwd, capture_output=True, text=True)


def parse_map(path):
    """Publics from a link /MAP: mangled name -> (address, is_function, object)."""
    out = {}
    for line in open(path, encoding="latin-1"):
        m = MAP_LINE.match(line)
        if m and m.group(1) != "0000":
            out[m.group(3)] = (int(m.group(4), 16), bool(m.group(5)), m.group(6))
    return out


def annotations():
    """(kind, address, source file, following code line) for every marker in src/."""
    out = []
    for src in sorted((ROOT / "src").rglob("*")):
        if src.suffix.lower() not in (".c", ".cpp"):
            continue
        lines = src.read_text(errors="replace").splitlines()
        for i, line in enumerate(lines):
            m = MARKER.match(line)
            if m:
                nxt = next((l for l in lines[i + 1:] if l.strip() and not MARKER.match(l)), "")
                out.append((m.group(1), int(m.group(2), 16), src, nxt))
    return out


def global_name(code_line):
    """'unsigned int nu_current_thread_id;' -> 'nu_current_thread_id'."""
    m = re.search(r"(\w+)\s*(\[[^\]]*\])?\s*(=|;)", code_line)
    return m.group(1) if m else None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--install", action="store_true", help="copy the DLL into run/game")
    args = ap.parse_args()

    subprocess.run([str(ROOT / "tools" / "build.py")], check=True)
    report = BUILD / "report.json"
    subprocess.run([str(ROOT / "tools" / "reccmp"), "reccmp", "--target", "BIONICLE", "--silent",
                    "--json", str(report)], check=True, capture_output=True)
    entries = json.load(open(report))["data"]
    publics = parse_map(BUILD / "Bionicle.map")
    by_recomp = {addr: (name, is_func, obj) for name, (addr, is_func, obj) in publics.items()}
    sizes = {int(r["address"], 16): int(r["size"]) for r in csv.DictReader(open(FUNCS))}

    # Original address of every function the matching build knows (incl. stubs).
    orig_of = {}      # mangled -> original address
    matched = {}      # mangled -> bool (exact/effective)
    obj_funcs = {}    # object -> [mangled]
    for e in entries:
        recomp = int(e["recomp"], 16) if e.get("recomp") else None
        if recomp not in by_recomp:
            continue
        name, _, obj = by_recomp[recomp]
        orig_of[name] = int(e["address"], 16)
        matched[name] = not e.get("stub") and (e["matching"] == 1.0 or bool(e.get("effective")))
        if not e.get("stub"):
            obj_funcs.setdefault(obj, []).append(name)

    # Globals: annotated definitions, matched to their mangled public by name.
    global_objs = set()
    stub_objs = set()
    for kind, addr, src, code in annotations():
        if kind == "GLOBAL":
            name = global_name(code)
            for mangled, (_, is_func, obj) in publics.items():
                if not is_func and name and (mangled == "_" + name or mangled.startswith(f"?{name}@@3")):
                    orig_of[mangled] = addr
                    global_objs.add(obj)
    for name, ok in matched.items():
        if name in orig_of and not ok and name not in sum(obj_funcs.values(), []):
            stub_objs.add(publics[name][2])

    # Objects that can run in-game: every annotated function matches, no globals/stubs.
    include, skipped = [], []
    for obj, names in sorted(obj_funcs.items()):
        if obj.startswith("LIBCMT") or obj in global_objs or obj in stub_objs:
            continue
        if all(matched[n] for n in names):
            include.append(obj)
        else:
            skipped.append(obj)
    hooks = [(orig_of[n], n) for obj in include for n in obj_funcs[obj]
             if sizes.get(orig_of[n], MIN_PATCH) >= MIN_PATCH]
    objs = [p for p in (BUILD / "obj").rglob("*.obj") if p.name in include]

    OUT.mkdir(parents=True, exist_ok=True)
    r = msvc("cl", "/nologo", "/c", "/O2", "/MT", "/W3", f"/Fo{winpath(OUT / 'hookmain.obj')}",
             winpath(HOOK_SRC / "hookmain.c"))
    if r.returncode:
        sys.exit(r.stdout)

    def link(extra):
        rsp = OUT / "link.rsp"
        rsp.write_text("\n".join(
            ["/nologo", "/DLL", "/FIXED", f"/BASE:{DLL_BASE:#x}", "/OPT:NOREF", "/OPT:NOICF",
             f"/DEF:{winpath(HOOK_SRC / 'xinput1_1.def')}", f"/OUT:{winpath(OUT / 'xinput1_1.dll')}",
             f"/MAP:{winpath(OUT / 'xinput1_1.map')}", f'"{winpath(OUT / "hookmain.obj")}"']
            + [f'"{winpath(o)}"' for o in objs + extra]
            + ["libcmt.lib", "kernel32.lib", "user32.lib"]) + "\n")
        return msvc("link", f"@{winpath(rsp)}")

    # Pass 1 finds what the objects need from the exe; pass 2 links it in via glue.asm.
    glue = OUT / "glue.asm"
    unresolved = set()
    first = link([])
    for line in first.stdout.splitlines():
        m = re.search(r"unresolved external symbol (\S+)", line)
        if m:
            unresolved.add(m.group(1))
    unresolved.add("_bh_hooks")
    unknown = sorted(u for u in unresolved if u not in orig_of and u != "_bh_hooks")
    if unknown:
        sys.exit("no original address for: " + ", ".join(unknown))

    lines = [".386", ".model flat", ""]
    data_syms = sorted(u for u in unresolved if u in orig_of and not publics[u][1])
    func_syms = sorted(u for u in unresolved if u in orig_of and publics[u][1])
    for s in data_syms:
        lines += [f"PUBLIC {s}", f"{s} EQU 0{orig_of[s]:X}h"]
    lines += ["", "_DATA SEGMENT"]
    for s in func_syms:
        lines += [f"slot{i} DD 0{orig_of[s]:X}h" for i in [func_syms.index(s)]]
    lines += ["PUBLIC _bh_hooks", "_bh_hooks LABEL DWORD"]
    for addr, name in sorted(hooks):
        lines.append(f"    DD 0{addr:X}h, OFFSET {name}")
    lines += ["    DD 0, 0", "_DATA ENDS", "", "_TEXT SEGMENT"]
    for i, s in enumerate(func_syms):
        lines += [f"PUBLIC {s}", f"{s} PROC", f"    jmp DWORD PTR slot{i}", f"{s} ENDP"]
    for _, name in hooks:
        lines.append(f"EXTERN {name}:PROC")
    lines += ["_TEXT ENDS", "END", ""]
    glue.write_text("\n".join(lines))
    r = msvc("ml", "/nologo", "/c", "/coff", f"/Fo{winpath(OUT / 'glue.obj')}", winpath(glue))
    if r.returncode:
        sys.exit(r.stdout)
    second = link([OUT / "glue.obj"])
    if second.returncode:
        sys.exit(second.stdout)

    dll = OUT / "xinput1_1.dll"
    print(f"built {dll.relative_to(ROOT)}: {len(hooks)} hooked functions from {len(include)} objects; "
          f"{len(data_syms)} globals and {len(func_syms)} functions bound to the exe")
    for obj in skipped:
        print(f"  not hooked (has non-matching functions): {obj}")
    if args.install:
        game = ROOT / "run" / "game"
        if not game.exists():
            sys.exit("run/game missing; run tools/rungame setup")
        shutil.copy2(dll, game / "xinput1_1.dll")
        print(f"installed to {(game / 'xinput1_1.dll').relative_to(ROOT)}")


if __name__ == "__main__":
    sys.exit(main())
