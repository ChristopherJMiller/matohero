#!/usr/bin/env python3
"""Find functions whose code is identical to the original except for addresses reccmp
can't name.

reccmp maps an address to a symbol only when it falls inside one, so code that indexes
an array from 1 (`table[i - 1]`, compiled as `table - 4 + i * 4`) shows the raw original
address against our `other_symbol[n]`, and an identical function scores 90-97%. This tool
reads reccmp's JSON report (with diffs) and, for every function below 100%, checks each
differing instruction pair: same length and opcode, differing only in one 4-byte field,
whose two values sit at the same offset from the same annotated global (within 64 KB, or
inside the global up to one past its end) in the two builds. Functions where every difference passes are written to
build/offset_identical.json; tools/progress.py counts them as effective and tools/hook.py
hooks them.

  tools/offsetmatch.py [build/report.json]

Global addresses come from reccmp's own matching (the PDB and the GLOBAL markers),
cached in build/variables.json per PDB timestamp.
"""
import json
import os
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
ORIG = ROOT / "orig" / "Bionicle.exe"
RECOMP = BUILD / "Bionicle.exe"
PDB = BUILD / "Bionicle.pdb"
VARS = BUILD / "variables.json"
OUT = BUILD / "offset_identical.json"
MAX_DELTA = 0x10000


def variables():
    """[(orig, recomp, name)] for every global reccmp matched, cached per PDB."""
    stamp = PDB.stat().st_mtime
    if VARS.exists():
        cached = json.load(open(VARS))
        if cached.get("pdb_mtime") == stamp:
            return cached["vars"]
    os.environ.setdefault("WINEPREFIX", str(ROOT / "toolchain" / "wineprefix"))
    os.environ.setdefault("WINEDEBUG", "-all")
    os.chdir(ROOT)
    import argparse
    import logging
    from reccmp.compare import Compare
    from reccmp.project.detect import argparse_parse_project_target
    logging.disable(logging.CRITICAL)
    target = argparse_parse_project_target(argparse.Namespace(target="BIONICLE", paths_target=None))
    compare = Compare.from_target(target)
    out = [(m.orig_addr, m.recomp_addr, m.name) for m in compare.get_variables()]
    json.dump({"pdb_mtime": stamp, "vars": out}, open(VARS, "w"))
    return out


class Image:
    def __init__(self, path):
        import pefile
        self.pe = pefile.PE(str(path), fast_load=True)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase

    def read(self, va, n):
        return self.pe.get_data(va - self.base, n)


def decode(image, va):
    import capstone
    md = getattr(decode, "md", None)
    if md is None:
        md = decode.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    insn = next(md.disasm(image.read(va, 16), va), None)
    return insn


def field(a, b):
    """The 4-byte field where two equal-length encodings differ: (offset, a_val, b_val)."""
    diff = [i for i in range(len(a)) if a[i] != b[i]]
    if not diff or diff[-1] - diff[0] >= 4:
        return None
    for start in range(max(0, diff[-1] - 3), diff[0] + 1):
        if start + 4 <= len(a):
            return start, struct.unpack_from("<I", a, start)[0], struct.unpack_from("<I", b, start)[0]
    return None


def text_field(otext, rtext):
    """(None, a, b) when reccmp's two texts differ in exactly one token, a hex number."""
    ot = re.split(r"[\s,\[\]]+", otext.split("\t")[0].strip())
    rt = re.split(r"[\s,\[\]]+", rtext.split("\t")[0].strip())
    if len(ot) != len(rt):
        return None
    diff = [(a, b) for a, b in zip(ot, rt) if a != b]
    if len(diff) != 1 or not all(re.fullmatch(r"0x[0-9a-f]+", x) for x in diff[0]):
        return None
    return None, int(diff[0][0], 16), int(diff[0][1], 16)


def extents(gvars):
    """Each global's extent in both builds, as far as known: up to the next annotated
    global (the size of a big array, e.g. a material store of 1800 entries)."""
    def gaps(addrs):
        addrs = sorted(set(addrs))
        return {a: b - a for a, b in zip(addrs, addrs[1:])}
    go, gr = gaps(o for o, _, _ in gvars), gaps(r for _, r, _ in gvars)
    return {(o, r): min(go.get(o, 0), gr.get(r, 0)) for o, r, _ in gvars}


def equivalent(v_orig, v_ours, gvars, ext):
    """Some annotated global S has v_orig at the same offset from S's original address as
    v_ours from ours, the offset being within MAX_DELTA or inside S (up to one past its
    end, for loops over a big array). The two builds lay globals out differently, so an
    equal offset from the same symbol isn't a coincidence; the closest such S is
    reported."""
    best = None
    for orig, recomp, name in gvars:
        d = v_ours - recomp
        near = abs(d) < MAX_DELTA or 0 <= d <= ext[(orig, recomp)]
        if near and v_orig - orig == d and (best is None or abs(d) < abs(best[0])):
            best = (d, name)
    return best


def check(entry, orig_img, ours_img, gvars, ext):
    """None if not identical modulo offsets, else the list of explained differences."""
    explained = []
    for _, blocks in entry.get("diff", []):
        for block in blocks:
            if "both" in block:
                continue
            o, r = block.get("orig", []), block.get("recomp", [])
            if len(o) != len(r):
                return None
            for (oa, otext), (ra, rtext) in zip(o, r):
                oi, ri = decode(orig_img, int(oa, 16)), decode(ours_img, int(ra, 16))
                if oi is None or ri is None or oi.mnemonic != ri.mnemonic or oi.size != ri.size:
                    return None
                f = field(bytes(oi.bytes), bytes(ri.bytes))
                if f is None:
                    # Two fields differ (e.g. a store of `global + offset` to another
                    # global): reccmp has already named the other one, so its text differs
                    # only in the raw immediate.
                    f = text_field(otext, rtext)
                if f is None:
                    return None
                _, v_orig, v_ours = f
                hit = equivalent(v_orig, v_ours, gvars, ext)
                if hit is None:
                    return None
                explained.append(f"{oa}: {hit[1]}{hit[0]:+#x}")
    return explained or None


def find(report):
    """Orig addresses (as in the report, "0x...") of functions identical apart from
    addresses reccmp can't name, with the explanation of each difference. Also written to
    build/offset_identical.json."""
    entries = json.load(open(report))["data"]
    candidates = [e for e in entries if not e.get("stub") and e.get("matching", 0) < 1.0
                  and not e.get("effective") and e.get("diff")]
    gvars = variables()
    ext = extents(gvars)
    orig_img, ours_img = Image(ORIG), Image(RECOMP)
    found = {}
    for e in candidates:
        why = check(e, orig_img, ours_img, gvars, ext)
        if why:
            found[e["address"]] = {"name": e["name"], "matching": e["matching"], "offsets": why}
    json.dump(found, open(OUT, "w"), indent=1)
    return found


def main():
    report = Path(sys.argv[1]) if len(sys.argv) > 1 else BUILD / "report.json"
    found = find(report)
    for addr, info in found.items():
        print(f"{addr} {info['name']} ({info['matching']:.2%}): {', '.join(info['offsets'])}")
    print(f"{len(found)} functions identical apart from unnamed addresses")


if __name__ == "__main__":
    main()
