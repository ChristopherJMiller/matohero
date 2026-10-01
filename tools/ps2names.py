#!/usr/bin/env python3
"""Pair PC functions with their PS2 prototype counterparts to recover original names.

Evidence, strongest first:
  strings  a string literal referenced by exactly one function on each side
  calls    callees of an already-paired function, aligned by call order
  order    MSVC keeps each object's functions together and in source order, as the PS2
           map does, so unpaired runs between two pairs of the same object line up

Inputs: orig/Bionicle.exe, ghidra/export/functions.csv (ExportOverview.java),
orig/ps2proto/BION~213.ELF and symbols.csv (tools/ps2map.py).
Output: orig/ps2proto/pc_names.csv
  pc_address,pc_size,name,object,signature,method,evidence

  tools/ps2names.py            write the mapping and print a summary
  tools/ps2names.py --check    also compare against reccmp-annotated functions in src/
"""
import argparse
import bisect
import collections
import csv
import re
import struct
import sys
from pathlib import Path

import capstone
import lief
import pefile

ROOT = Path(__file__).resolve().parent.parent
PC_EXE = ROOT / "orig" / "Bionicle.exe"
PC_FUNCS = ROOT / "ghidra" / "export" / "functions.csv"
PS2_ELF = ROOT / "orig" / "ps2proto" / "BION~213.ELF"
PS2_SYMS = ROOT / "orig" / "ps2proto" / "symbols.csv"
OUT = ROOT / "orig" / "ps2proto" / "pc_names.csv"

MIN_STRING = 4


class Binary:
    """Functions with their ordered callees and referenced strings."""

    def __init__(self):
        self.funcs = {}       # start -> size
        self.calls = {}       # start -> [callee starts, in order, deduplicated]
        self.strings = {}     # start -> set(str)
        self.callers = collections.defaultdict(set)

    def finish(self):
        self.starts = sorted(self.funcs)
        for f, cs in self.calls.items():
            for c in cs:
                self.callers[c].add(f)


def read_cstring(data_at, addr):
    raw = data_at(addr, 256)
    if not raw:
        return None
    end = raw.find(b"\0")
    if end < MIN_STRING:
        return None
    s = raw[:end]
    if not all(32 <= b < 127 or b in (9, 10, 13) for b in s):
        return None
    return s.decode("ascii")


def normalize(s):
    """Source paths differ per platform; compare by basename."""
    if re.search(r"\.(c|cpp|h)$", s, re.I) and ("\\" in s or "/" in s):
        return "FILE:" + re.split(r"[\\/]", s)[-1].lower()
    return s


# --- PC (x86, MSVC) --------------------------------------------------------------

def load_pc():
    pe = pefile.PE(str(PC_EXE), fast_load=True)
    base = pe.OPTIONAL_HEADER.ImageBase
    image = pe.get_memory_mapped_image()
    data_ranges = [(base + s.VirtualAddress, base + s.VirtualAddress + s.Misc_VirtualSize)
                   for s in pe.sections if s.Name.rstrip(b"\0") in (b".rdata", b".data")]

    def data_at(addr, n):
        off = addr - base
        return image[off:off + n] if 0 <= off < len(image) else b""

    def is_data(a):
        return any(lo <= a < hi for lo, hi in data_ranges)

    b = Binary()
    for row in csv.DictReader(open(PC_FUNCS)):
        if row["thunk"] == "true":
            continue
        b.funcs[int(row["address"], 16)] = int(row["size"])
    starts = sorted(b.funcs)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    for i, f in enumerate(starts):
        end = min(f + b.funcs[f], starts[i + 1] if i + 1 < len(starts) else f + b.funcs[f])
        code = data_at(f, end - f)
        calls, strs = [], set()
        for insn in md.disasm(code, f):
            for op in insn.operands:
                val = None
                if op.type == capstone.x86.X86_OP_IMM:
                    val = op.imm & 0xFFFFFFFF
                elif op.type == capstone.x86.X86_OP_MEM and op.mem.base == 0 and op.mem.index == 0:
                    val = op.mem.disp & 0xFFFFFFFF
                if val is None:
                    continue
                if insn.mnemonic in ("call", "jmp") and val in b.funcs and val != f:
                    if val not in calls:
                        calls.append(val)
                elif is_data(val):
                    s = read_cstring(data_at, val)
                    if s:
                        strs.add(normalize(s))
        b.calls[f] = calls
        b.strings[f] = strs
    b.finish()
    return b


# --- PS2 (MIPS, ee-gcc) ----------------------------------------------------------

def load_ps2():
    elf = lief.parse(str(PS2_ELF))
    sections = [(s.virtual_address, s.virtual_address + s.size, bytes(s.content))
                for s in elf.sections if s.virtual_address and s.size and s.name in
                (".text", ".rodata", ".data", ".rdata", ".sdata")]

    def data_at(addr, n):
        for lo, hi, content in sections:
            if lo <= addr < hi:
                return content[addr - lo:addr - lo + n]
        return b""

    rodata = [(lo, hi) for lo, hi, _ in sections]

    b = Binary()
    b.info = {}
    for row in csv.DictReader(open(PS2_SYMS)):
        if row["type"] == "FUNC" and row["section"] == ".text":
            a = int(row["address"], 16)
            b.funcs[a] = int(row["size"])
            b.info[a] = row
    for f, size in b.funcs.items():
        code = data_at(f, size)
        hi = {}
        calls, strs = [], set()
        for off in range(0, len(code) - 3, 4):
            w = struct.unpack_from("<I", code, off)[0]
            op, rs, rt, imm = w >> 26, (w >> 21) & 31, (w >> 16) & 31, w & 0xFFFF
            simm = imm - 0x10000 if imm & 0x8000 else imm
            pc = f + off
            if op in (2, 3):  # j / jal
                tgt = ((pc + 4) & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
                if tgt in b.funcs and tgt != f and tgt not in calls:
                    calls.append(tgt)
                continue
            if op == 0x0F:  # lui
                hi[rt] = imm << 16
                continue
            addr = None
            if op == 0x09 and rs in hi:  # addiu
                addr = (hi[rs] + simm) & 0xFFFFFFFF
                hi[rt] = addr if rt != rs else hi.get(rt)
            elif op == 0x0D and rs in hi:  # ori
                addr = hi[rs] | imm
            elif 0x20 <= op <= 0x3F and rs in hi:  # loads/stores off a lui base
                addr = (hi[rs] + simm) & 0xFFFFFFFF
            if addr is not None and any(lo <= addr < hi_ for lo, hi_ in rodata):
                s = read_cstring(data_at, addr)
                if s:
                    strs.add(normalize(s))
        b.calls[f] = calls
        b.strings[f] = strs
    b.name_count = collections.Counter(r["name"] for r in b.info.values())
    b.finish()
    return b


# --- Pairing ---------------------------------------------------------------------

def compatible(pc, ps2, p, q):
    """Cheap sanity checks for a pairing proposed by calls/order propagation."""
    # Per-file static helpers (NuError, NuDebugMsg, ...) exist many times over on PS2
    # and are usually compiled out or inlined on PC.
    if ps2.name_count[ps2.info[q]["name"]] > 1:
        return False
    # x86 is denser than MIPS.
    if not 0.15 <= pc.funcs[p] / max(ps2.funcs[q], 1) <= 2.5:
        return False
    # A widely used helper (NuError, NuStrCmp, ...) must pair with a widely used one.
    a, b = len(pc.callers[p]), len(ps2.callers[q])
    if max(a, b) > 8 and max(a, b) > 4 * max(min(a, b), 1):
        return False
    # Literal strings should agree; file-name strings must name the PS2 object's file.
    sp = {x for x in pc.strings[p] if not x.startswith("FILE:")}
    sq = {x for x in ps2.strings[q] if not x.startswith("FILE:")}
    if (sp and sq and not sp & sq) or (len(sp) >= 2 and not sq) or (len(sq) >= 2 and not sp):
        return False
    files = {x[5:].rsplit(".", 1)[0] for x in pc.strings[p] if x.startswith("FILE:")}
    obj = ps2.info[q]["object"].rsplit("/", 1)[-1].rsplit(".", 1)[0].lower()
    if files and obj not in files:
        return False
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true", help="validate against annotated src/")
    args = ap.parse_args()

    pc, ps2 = load_pc(), load_ps2()
    pairs, method, evidence = {}, {}, {}  # pc -> ps2
    rev = {}

    def add(p, q, how, why):
        if p in pairs or q in rev:
            return False
        pairs[p], rev[q], method[p], evidence[p] = q, p, how, why
        return True

    # 1. Unique strings. A file-name string isn't unique (every asserting function in the
    # file shares it), so it only counts as object evidence later.
    str_pc, str_ps2 = collections.defaultdict(set), collections.defaultdict(set)
    for f, ss in pc.strings.items():
        for s in ss:
            str_pc[s].add(f)
    for f, ss in ps2.strings.items():
        for s in ss:
            str_ps2[s].add(f)
    votes = collections.Counter()
    for s in str_pc.keys() & str_ps2.keys():
        if s.startswith("FILE:") or len(str_pc[s]) != 1 or len(str_ps2[s]) != 1:
            continue
        votes[(next(iter(str_pc[s])), next(iter(str_ps2[s])))] += 1
    best_pc, best_ps2 = {}, {}
    for (p, q), n in votes.items():
        if n > best_pc.get(p, (0, None))[0]:
            best_pc[p] = (n, q)
        if n > best_ps2.get(q, (0, None))[0]:
            best_ps2[q] = (n, p)
    for p, (n, q) in best_pc.items():
        if best_ps2[q][1] == p:
            add(p, q, "strings", f"{n} unique string(s)")

    # 2/3. Propagate through call order and object order until nothing changes.
    obj_of = {q: ps2.info[q]["object"] for q in ps2.funcs}
    obj_funcs = collections.defaultdict(list)
    for q in sorted(ps2.funcs):
        obj_funcs[obj_of[q]].append(q)

    def align(ps, qs, how, why):
        """Pair two equal-length runs of unpaired functions positionally."""
        n = 0
        if len(ps) == len(qs) and ps and all(p not in pairs for p in ps) and all(q not in rev for q in qs):
            if all(compatible(pc, ps2, p, q) and (p, q) not in banned for p, q in zip(ps, qs)):
                for p, q in zip(ps, qs):
                    n += add(p, q, how, why)
        return n

    banned = set()

    def propagate():
        changed = True
        while changed:
            changed = False
            for p, q in list(pairs.items()):
                cp = [c for c in pc.calls[p]]
                cq = [c for c in ps2.calls[q]]
                # Split both callee lists at callees already paired with each other.
                anchors = [(i, cq.index(pairs[c])) for i, c in enumerate(cp) if c in pairs and pairs[c] in cq]
                prev_i, prev_j = -1, -1
                for i, j in anchors + [(len(cp), len(cq))]:
                    if i > prev_i and j > prev_j:
                        changed |= bool(align(cp[prev_i + 1:i], cq[prev_j + 1:j], "calls",
                                              f"callee of {ps2.info[q]['name']}"))
                        prev_i, prev_j = i, j
            for obj, qs in obj_funcs.items():
                known = [(q, rev[q]) for q in qs if q in rev]
                for (q1, p1), (q2, p2) in zip(known, known[1:]):
                    if p2 <= p1:
                        continue  # order disagrees; leave this object alone
                    ps = [p for p in pc.starts if p1 < p < p2]
                    between = qs[qs.index(q1) + 1:qs.index(q2)]
                    changed |= bool(align(ps, between, "order", f"between pairs in {obj}"))


    # MSVC emits each object contiguously, so a propagated pair sitting between two
    # functions of one other object is wrong. Ban it and propagate again until stable.
    propagate()
    while True:
        ordered = sorted(pairs)
        objs = [obj_of[pairs[p]] for p in ordered]
        bad = [p for i, p in enumerate(ordered)
               if method[p] != "strings" and 0 < i < len(ordered) - 1
               and objs[i - 1] == objs[i + 1] != objs[i]]
        if not bad:
            break
        for p in bad:
            q = pairs.pop(p)
            del rev[q], method[p], evidence[p]
            banned.add((p, q))
        propagate()

    # Write the mapping.
    with open(OUT, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["pc_address", "pc_size", "name", "object", "signature", "method", "evidence"])
        for p in sorted(pairs):
            q = pairs[p]
            i = ps2.info[q]
            w.writerow([f"0x{p:08x}", pc.funcs[p], i["name"], i["object"], i["signature"], method[p], evidence[p]])

    total = len(pc.funcs)
    by = collections.Counter(method.values())
    print(f"paired {len(pairs)}/{total} PC functions -> {OUT.relative_to(ROOT)}")
    for m in ("strings", "calls", "order"):
        print(f"  {m:8} {by[m]}")

    if args.check:
        known = {}
        for src in (ROOT / "src").rglob("*.c*"):
            lines = src.read_text().splitlines()
            for k, line in enumerate(lines):
                m = re.match(r"// FUNCTION: BIONICLE (0x[0-9a-f]+)", line)
                if m and k + 1 < len(lines):
                    name = re.search(r"(\w+)\s*\(", lines[k + 1])
                    if name:
                        known[int(m.group(1), 16)] = name.group(1)
        ok = sum(1 for a, n in known.items() if a in pairs and ps2.info[pairs[a]]["name"] == n)
        wrong = [(hex(a), n, ps2.info[pairs[a]]["name"]) for a, n in known.items()
                 if a in pairs and ps2.info[pairs[a]]["name"] != n]
        print(f"check: {ok} correct, {len(wrong)} wrong, {len(known) - ok - len(wrong)} unpaired "
              f"of {len(known)} annotated")
        for w_ in wrong:
            print("  wrong:", *w_)


if __name__ == "__main__":
    sys.exit(main())
