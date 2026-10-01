#!/usr/bin/env python3
"""List float constants in the original exe for reccmp (build/orig-floats.csv).

reccmp finds float constants by scanning for x87 instructions with a memory operand,
but only accepts operands that sit on a relocation, and our exe has no relocation table
(isledecomp/reccmp#235). Without these entries every fld/fmul of a constant compares as
a mismatch. Same scan, minus the relocation check: the operand must point into .rdata.
Names are str(value), the way reccmp names the recompiled side's floats.
"""
import struct
import sys
from pathlib import Path

import pefile
from reccmp.analysis.float_const import (
    DOUBLE_PRECISION_OPCODES,
    SINGLE_PRECISION_OPCODES,
    find_float_instructions_in_buffer,
)

ROOT = Path(__file__).resolve().parent.parent
EXE = ROOT / "orig" / "Bionicle.exe"
OUT = ROOT / "build" / "orig-floats.csv"


def main():
    if OUT.exists() and OUT.stat().st_mtime >= EXE.stat().st_mtime:
        return
    pe = pefile.PE(str(EXE), fast_load=True)
    base = pe.OPTIONAL_HEADER.ImageBase
    sect = {s.Name.rstrip(b"\0").decode(): s for s in pe.sections}
    text, rdata = sect[".text"], sect[".rdata"]
    lo, hi = base + rdata.VirtualAddress, base + rdata.VirtualAddress + rdata.Misc_VirtualSize
    code = pe.get_data(text.VirtualAddress, text.SizeOfRawData)
    seen = {}
    for inst in find_float_instructions_in_buffer(code, base + text.VirtualAddress):
        if inst.pointer in seen or not lo <= inst.pointer < hi:
            continue
        if inst.opcode in SINGLE_PRECISION_OPCODES:
            (value,) = struct.unpack("<f", pe.get_data(inst.pointer - base, 4))
            seen[inst.pointer] = (value, 4)
        elif inst.opcode in DOUBLE_PRECISION_OPCODES:
            (value,) = struct.unpack("<d", pe.get_data(inst.pointer - base, 8))
            seen[inst.pointer] = (value, 8)
    OUT.parent.mkdir(exist_ok=True)
    with open(OUT, "w") as f:
        f.write("address|type|name|size\n")
        for addr in sorted(seen):
            value, size = seen[addr]
            f.write(f"{addr:#x}|float|{value}|{size}\n")
    print(f"wrote {OUT.relative_to(ROOT)}: {len(seen)} float constants")


if __name__ == "__main__":
    sys.exit(main())
