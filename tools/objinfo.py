#!/usr/bin/env python3
"""Show what's known about one original object before decompiling it.

  tools/objinfo.py numath/nuvec            PC functions in the object's region, in order,
                                           with PS2 names/signatures where paired
  tools/objinfo.py numath/nuvec --asm      plus each function's disassembly
  tools/objinfo.py numath/nuvec --ps2      plus the PS2 object's full function list
                                           (incl. ones the PC linker dropped)

The object is matched as a suffix of the PS2 object path (nu2api.bionicleps2/numath/
nuvec.o). Region: functions placed in it (named/inside) plus gap neighbours on each side.
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PS2 = ROOT / "orig" / "ps2proto"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("object")
    ap.add_argument("--asm", action="store_true")
    ap.add_argument("--ps2", action="store_true")
    args = ap.parse_args()

    want = args.object.removesuffix(".o").removesuffix(".c").removesuffix(".cpp") + ".o"
    objects = list(csv.DictReader(open(PS2 / "pc_objects.csv")))
    names = {r["pc_address"]: r for r in csv.DictReader(open(PS2 / "pc_names.csv"))}
    matches = sorted({r["object"] for r in objects if r["status"] in ("named", "inside")
                      and r["object"].endswith("/" + want)})
    if len(matches) != 1:
        sys.exit(f"{want}: {'ambiguous: ' + ', '.join(matches) if matches else 'no PC functions placed'}")
    obj = matches[0]

    idx = [i for i, r in enumerate(objects) if r["object"] == obj]
    lo, hi = idx[0], idx[-1]
    while lo > 0 and objects[lo - 1]["status"] == "gap" and obj in objects[lo - 1]["object"].split("|"):
        lo -= 1
    while hi + 1 < len(objects) and objects[hi + 1]["status"] == "gap" and obj in objects[hi + 1]["object"].split("|"):
        hi += 1

    sigs = {}
    stdump = PS2 / "stdump" / "functions.cpp"
    if stdump.exists():
        for line in open(stdump, errors="replace"):
            m = re.match(r"/\* ([0-9a-f]{8}) [0-9a-f]{8} \*/ (.*?)\s*\{", line)
            if m:
                sigs[int(m.group(1), 16)] = m.group(2)
    ps2_addr = {}
    ps2_rows = []
    for r in csv.DictReader(open(PS2 / "symbols.csv")):
        if r["object"] == obj and r["type"] == "FUNC":
            ps2_rows.append(r)
            ps2_addr[r["name"]] = int(r["address"], 16)

    print(f"{obj}: PC 0x{int(objects[lo]['pc_address'], 16):x}-0x{int(objects[hi]['pc_address'], 16):x}")
    for r in objects[lo:hi + 1]:
        a = r["pc_address"]
        n = names.get(a)
        tag = r["status"] if r["object"] == obj else "gap?"
        if n:
            sig = sigs.get(ps2_addr.get(n["name"], -1)) or n["signature"] or n["name"]
            print(f"  {a} {int(r['pc_size']):5}  {tag:6} {sig}   [{n['method']}]")
        else:
            print(f"  {a} {int(r['pc_size']):5}  {tag:6} ?")
        if args.asm:
            start = int(a, 16)
            out = subprocess.run(["llvm-objdump", "-d", "--no-show-raw-insn", "--x86-asm-syntax=intel",
                                  f"--start-address={start:#x}", f"--stop-address={start + int(r['pc_size']):#x}",
                                  str(ROOT / "orig" / "Bionicle.exe")], capture_output=True, text=True).stdout
            for line in out.splitlines()[6:]:
                if line.strip():
                    print("        " + line.strip())
    if args.ps2:
        print(f"\nPS2 {obj} ({len(ps2_rows)} functions, source order):")
        for r in ps2_rows:
            print(f"  {sigs.get(int(r['address'], 16), r['signature'] or r['name'])}")


if __name__ == "__main__":
    sys.exit(main())
