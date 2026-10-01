#!/usr/bin/env python3
"""Turn the PS2 prototype's linker map + ELF symbols into one symbol table.

Reads orig/ps2proto/BION~213.MAP (object file for every global symbol) and the ELF
.symtab (adds static functions/data, which the map omits; they're assigned to the object
whose address range contains them). Writes orig/ps2proto/symbols.csv:

  address,size,type,bind,section,object,name,signature,mangled

For C++ the map shows demangled names with parameter types (`signature`) and the ELF the
GCC 2.95 mangled ones (`mangled`); both are joined by address. `name` is the bare
identifier (with any Class:: qualifier).

`object` is the path under h:\\Bionicle\\Ps2\\Source\\ (or the SDK/library archive member),
e.g. nu2api.bionicleps2/gamelib/listman.o. Rows are sorted by address, so each object's
functions appear in their original source order.

  tools/ps2map.py              write symbols.csv and print a per-directory summary
"""
import bisect
import collections
import csv
import re
import sys
from pathlib import Path

import lief

ROOT = Path(__file__).resolve().parent.parent
PROTO = ROOT / "orig" / "ps2proto"
MAP = PROTO / "BION~213.MAP"
ELF = PROTO / "BION~213.ELF"
OUT = PROTO / "symbols.csv"

SOURCE_PREFIX = "h:\\bionicle\\ps2\\source\\"
LINE = re.compile(r"^([0-9a-f]{8}) ([0-9a-f]{8}) +(\d+) (.*)$")


def short_object(path):
    p = path.strip()
    if p.lower().startswith(SOURCE_PREFIX):
        p = p[len(SOURCE_PREFIX):]
    # Objects are built into ps2_final_obj\ next to their sources; drop that component.
    p = p.replace("\\ps2_final_obj\\", "\\")
    return p.replace("\\", "/")


def parse_map():
    """Yield (address, size, section, object, symbol-or-None) for objects and symbols."""
    section = obj = None
    with open(MAP, encoding="latin-1") as f:
        for raw in f:
            line = raw.rstrip("\r\n")
            m = LINE.match(line)
            if not m:
                continue
            addr, size = int(m.group(1), 16), int(m.group(2), 16)
            rest = line[24:]
            indent = len(rest) - len(rest.lstrip(" "))
            text = rest.strip()
            if indent == 0:
                section, obj = text, None  # output section
            elif indent == 8:
                pass  # input section name, same as output for our purposes
            elif indent == 16:
                obj = short_object(text)
                yield addr, size, section, obj, None
            elif indent >= 24 and obj is not None:
                yield addr, size, section, obj, text


def bare_name(sig):
    """'charbaseAddAnimFlag(char *)' -> 'charbaseAddAnimFlag'; plain names pass through."""
    depth = 0
    for i, ch in enumerate(sig):
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth -= 1
        elif ch == "(" and depth == 0 and i > 0:
            return sig[:i]
    return sig


def main():
    objects = []  # (start, end, section, object)
    by_addr = {}  # address -> row; one row per address (first FUNC/OBJECT symbol wins)
    for addr, size, section, obj, sym in parse_map():
        if sym is None:
            objects.append((addr, addr + size, section, obj))
            continue
        row = by_addr.setdefault(addr, {"address": addr, "size": size, "type": "", "bind": "GLOBAL",
                                        "section": section, "object": obj, "name": bare_name(sym),
                                        "signature": "", "mangled": ""})
        if "(" in sym and not row["signature"]:
            row["signature"] = sym
    objects.sort()
    starts = [o[0] for o in objects]

    def containing(addr):
        i = bisect.bisect_right(starts, addr) - 1
        # Several objects can start at one address (empty ones); prefer the last non-empty.
        while i >= 0 and objects[i][0] <= addr:
            if addr < objects[i][1]:
                return objects[i]
            i -= 1
        return None

    elf = lief.parse(str(ELF))
    types = {lief.ELF.Symbol.TYPE.FUNC: "FUNC", lief.ELF.Symbol.TYPE.OBJECT: "OBJECT"}
    for s in elf.symtab_symbols:
        kind = types.get(s.type)
        if kind is None or not s.name or s.value == 0:
            continue
        row = by_addr.get(s.value)
        if row is None:
            o = containing(s.value)
            row = by_addr[s.value] = {
                "address": s.value, "size": s.size, "type": "",
                "bind": "LOCAL" if s.binding == lief.ELF.Symbol.BINDING.LOCAL else "GLOBAL",
                "section": o[2] if o else "", "object": o[3] if o else "", "name": s.name,
                "signature": "", "mangled": ""}
        if row["type"]:
            continue  # an alias at the same address; keep the first
        row["type"] = kind
        row["size"] = row["size"] or s.size
        if s.name != row["name"]:
            row["mangled"] = s.name

    ordered = sorted(by_addr.values(), key=lambda r: r["address"])
    with open(OUT, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["address", "size", "type", "bind", "section", "object",
                                          "name", "signature", "mangled"])
        w.writeheader()
        for r in ordered:
            w.writerow({**r, "address": f"0x{r['address']:08x}"})

    funcs = [r for r in ordered if r["section"] == ".text" and r["type"] != "OBJECT"]
    by_dir = collections.Counter()
    objs_by_dir = collections.defaultdict(set)
    for r in funcs:
        d = r["object"].rsplit("/", 1)[0] if "/" in r["object"] else (r["object"] or "?")
        by_dir[d] += 1
        objs_by_dir[d].add(r["object"])
    print(f"wrote {OUT.relative_to(ROOT)}: {len(ordered)} symbols, {len(funcs)} functions, "
          f"{len({o[3] for o in objects})} objects")
    for d, n in by_dir.most_common():
        print(f"  {n:6} funcs  {len(objs_by_dir[d]):4} objs  {d}")


if __name__ == "__main__":
    sys.exit(main())
