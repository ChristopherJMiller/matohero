#!/usr/bin/env python3
"""Build the hook DLL: matched decompiled code that runs inside the original game.

The DLL is named xinput1_1.dll (the game imports it; the real API is forwarded to
xinput1_3). On load it patches each hooked function in the exe with a jump to its
recompiled copy, so decompiled code gets exercised in-game long before the whole game
is decompiled.

What goes in:
  * every object from the matching build (tools/build.py) with at least one matched
    function (exact or effective per reccmp, or identical per tools/offsetmatch.py), as a patched copy (tools/coffpatch.py):
    its stubs, still-mismatching functions and annotated globals are unbound from the
    local definitions so they resolve to the exe's; static matched functions are made
    external so the hook table can name them;
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
  tools/hook.py --trace      count calls to each hooked function; the DLL rewrites
                             bhhook-trace.txt next to the exe every 5 s
  tools/hook.py --skip A,B   leave functions A and B (names or name prefixes) to the exe,
                             e.g. to bisect a hook that breaks the game
"""
import argparse
import collections
import csv
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

import coffpatch

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


# CRT functions with state shared with the exe (stdio streams, the heap). The DLL links its
# own CRT, so our code's calls must go to the exe's copies (src/library.h has their
# addresses): a FILE* from the exe's fopen used with the DLL's fseek hangs the game, and a
# block from the exe's malloc must go back to the exe's free. Only our objects are
# redirected; overriding the symbols DLL-wide crashed at load (the DLL's CRT startup
# mallocs before the exe's heap exists). Pure helpers (_ftol2, __security_check_cookie
# with the DLL's own cookie) stay local.
EXE_CRT = {"fopen", "fclose", "fread", "fwrite", "fseek", "ftell", "malloc", "free"}


def library_functions():
    """C name -> original address, from the LIBRARY markers in src/library.h."""
    out = {}
    lines = (ROOT / "src" / "library.h").read_text().splitlines()
    for i, line in enumerate(lines):
        m = re.match(r"// LIBRARY: BIONICLE (0x[0-9a-fA-F]+)", line)
        if m and i + 1 < len(lines):
            n = re.match(r"//\s*(\w+)", lines[i + 1])
            if n:
                out[n.group(1)] = int(m.group(1), 16)
    return out


def global_name(code_line):
    """'unsigned int nu_current_thread_id;' -> 'nu_current_thread_id'."""
    m = re.search(r"(\w+)\s*(\[[^\]]*\])?\s*(=|;)", code_line)
    return m.group(1) if m else None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--install", action="store_true", help="copy the DLL into run/game")
    ap.add_argument("--trace", action="store_true", help="count calls per hooked function")
    ap.add_argument("--skip", default="", help="comma-separated function names/prefixes to leave to the exe")
    args = ap.parse_args()
    skip = [p for p in args.skip.split(",") if p]

    subprocess.run([str(ROOT / "tools" / "build.py")], check=True)
    report = BUILD / "report.json"
    subprocess.run([str(ROOT / "tools" / "reccmp"), "reccmp", "--target", "BIONICLE", "--silent",
                    "--json", str(report)], check=True, capture_output=True)
    entries = json.load(open(report))["data"]
    # Also hook functions identical apart from addresses reccmp can't name.
    import offsetmatch
    identical = offsetmatch.find(report)
    publics = parse_map(BUILD / "Bionicle.map")
    by_recomp = {addr: (name, is_func, obj) for name, (addr, is_func, obj) in publics.items()}
    sizes = {int(r["address"], 16): int(r["size"]) for r in csv.DictReader(open(FUNCS))}

    # Every function the matching build knows: mangled -> (original address, status, object).
    funcs = {}
    display = {}
    for e in entries:
        recomp = int(e["recomp"], 16) if e.get("recomp") else None
        if recomp not in by_recomp:
            continue
        name, _, obj = by_recomp[recomp]
        if e.get("stub"):
            status = "stub"
        elif e["matching"] == 1.0 or e.get("effective") or e["address"] in identical:
            status = "matched"
        else:
            status = "partial"
        if status == "matched" and any(e["name"].startswith(p) for p in skip):
            status = "partial"
        funcs[name] = (int(e["address"], 16), status, obj)
        display[name] = e["name"]
    orig_of = {n: f[0] for n, f in funcs.items()}
    obj_paths = {p.name: p for p in (BUILD / "obj").rglob("*.obj")}

    # Annotated globals -> the symbol their object defines (static or not).
    globals_by_obj = collections.defaultdict(dict)  # object -> {symbol: (address, static)}
    for kind, addr, src, code in annotations():
        name = global_name(code) if kind == "GLOBAL" else None
        path = obj_paths.get(src.with_suffix(".obj").name)
        if not name or not path:
            continue
        for _, sym, value, section, storage in coffpatch.symbols(path.read_bytes()):
            defined = section > 0 or (section == 0 and value > 0)  # value>0: communal
            if defined and (sym == "_" + name or sym.startswith(f"?{name}@@3")):
                globals_by_obj[path.name][sym] = (addr, storage == 3)
                break
    data_names = set()
    for syms in globals_by_obj.values():
        for sym, (addr, static) in syms.items():
            if not static:
                orig_of[sym] = addr
                data_names.add(sym)

    # Any object with a matched function goes in. Within it, matched functions get hooked;
    # stubs, partial matches and annotated globals are unbound from their local copies and
    # bound to the exe's (statics get a per-object name).
    include = sorted({obj for _, status, obj in funcs.values()
                      if status == "matched" and not obj.startswith("LIBCMT")})
    hooks, objs, unbound = [], [], 0
    lib = library_functions()
    (OUT / "obj").mkdir(parents=True, exist_ok=True)
    for k, objname in enumerate(include):
        path = obj_paths[objname]
        symbols = list(coffpatch.symbols(path.read_bytes()))
        storage = {sym: stor for _, sym, _, _, stor in symbols}
        changes = {}
        # Stateful CRT functions our code calls go to the exe's CRT under a private name
        # (glue.asm), so the DLL's own CRT (whose startup mallocs before the exe's heap
        # exists) keeps its copies.
        referenced = {sym for _, sym, value, section, stor in symbols
                      if section == 0 and value == 0 and stor == 2}
        for cname in EXE_CRT:
            if "_" + cname in referenced and cname in lib:
                changes["_" + cname] = ("unbind", "_bh_crt_" + cname)
                orig_of["_bh_crt_" + cname] = lib[cname]
        for name, (orig, status, obj) in funcs.items():
            if obj != objname:
                continue
            static = storage.get(name) == 3
            local = f"{name}$h{k}" if static else name
            if status == "matched":
                if sizes.get(orig, MIN_PATCH) >= MIN_PATCH:
                    if static:
                        changes[name] = ("export", local)
                        display[local] = display[name]
                    hooks.append((orig, local))
            else:
                changes[name] = ("unbind", local)
                orig_of[local] = orig
                unbound += 1
        for sym, (addr, static) in globals_by_obj.get(objname, {}).items():
            local = f"{sym}$h{k}" if static else sym
            changes[sym] = ("unbind", local)
            orig_of[local] = addr
            data_names.add(local)
        dst = OUT / "obj" / objname
        coffpatch.patch(path, dst, changes)
        objs.append(dst)

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
    unknown = sorted(u for u in unresolved if u not in orig_of and not u.startswith("_bh_"))  # glue defines _bh_*
    if unknown:
        sys.exit("no original address for: " + ", ".join(unknown))

    lines = [".386", ".model flat", ""]
    data_syms = sorted(u for u in unresolved if u in orig_of and u in data_names)
    func_syms = sorted(u for u in unresolved if u in orig_of and u not in data_names)
    for s in data_syms:
        lines += [f"PUBLIC {s}", f"{s} EQU 0{orig_of[s]:X}h"]
    lines += ["", "_DATA SEGMENT"]
    for s in func_syms:
        lines += [f"slot{i} DD 0{orig_of[s]:X}h" for i in [func_syms.index(s)]]
    hooks.sort()
    lines += ["PUBLIC _bh_hooks", "_bh_hooks LABEL DWORD"]
    for i, (addr, name) in enumerate(hooks):
        lines.append(f"    DD 0{addr:X}h, OFFSET {f'trace{i}' if args.trace else name}")
    lines += ["    DD 0, 0"]
    n_trace = len(hooks) if args.trace else 0
    lines += ["PUBLIC _bh_trace_n", f"_bh_trace_n DD {n_trace}",
              "PUBLIC _bh_counts", f"_bh_counts DD {max(n_trace, 1)} DUP (0)",
              "PUBLIC _bh_names", "_bh_names LABEL DWORD"]
    lines += [f"    DD OFFSET tname{i}" for i in range(n_trace)]
    for i in range(n_trace):
        text = display.get(hooks[i][1], hooks[i][1]).replace('"', "'")
        lines.append(f'tname{i} DB "{text}", 0')
    lines += ["_DATA ENDS", "", "_TEXT SEGMENT"]
    for i, s in enumerate(func_syms):
        lines += [f"PUBLIC {s}", f"{s} PROC", f"    jmp DWORD PTR slot{i}", f"{s} ENDP"]
    for i in range(n_trace):  # lock inc touches only flags, which no call relies on
        lines += [f"trace{i} PROC", f"    lock inc DWORD PTR _bh_counts+{4 * i}",
                  f"    jmp {hooks[i][1]}", f"trace{i} ENDP"]
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
    print(f"built {dll.relative_to(ROOT)}{' (trace)' if args.trace else ''}: {len(hooks)} hooked functions from "
          f"{len(include)} objects ({unbound} unmatched functions left to the exe); "
          f"{len(data_syms)} globals and {len(func_syms)} functions bound to the exe")
    if args.install:
        game = ROOT / "run" / "game"
        if not game.exists():
            sys.exit("run/game missing; run tools/rungame setup")
        shutil.copy2(dll, game / "xinput1_1.dll")
        print(f"installed to {(game / 'xinput1_1.dll').relative_to(ROOT)}")


if __name__ == "__main__":
    sys.exit(main())
