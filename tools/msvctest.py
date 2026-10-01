#!/usr/bin/env python3
"""Compile a small C file with the project's flags and show what MSVC made of it.

  tools/msvctest.py test.c [more.c ...]

For each file, prints the functions in the order the compiler emitted them, each with
the functions it still calls (anything else was inlined) and its size. Used to work out
compiler behaviour (deferral, inlining, register arguments) on minimal examples; see
docs/matching.md.
"""
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build  # noqa: E402

OUT = build.BUILD / "msvctest"


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    srcs = [Path(a).resolve() for a in sys.argv[1:]]
    flags = build.COMMON_FLAGS
    for src in srcs:
        obj = OUT / (src.stem + ".obj")
        res = subprocess.run([str(build.MSVC), "cl", *flags, f"/Fo{build.winpath(obj)}", build.winpath(src)],
                             cwd=src.parent, capture_output=True, text=True)
        errors = [l for l in res.stdout.splitlines() if "error" in l or "warning" in l]
        if errors:
            print("\n".join(errors))
        if not obj.exists():
            continue
        dis = subprocess.run(["llvm-objdump", "-dr", "--x86-asm-syntax=intel", str(obj)],
                             capture_output=True, text=True).stdout
        print(f"== {src.name}")
        cur, calls, size = None, [], 0
        rows = []
        for line in dis.splitlines():
            m = re.match(r"^[0-9a-f]+ <(_\w+|@\w+@\d+)>:", line)
            if m:
                if cur:
                    rows.append((cur, calls, size))
                cur, calls, size = m.group(1), [], 0
                continue
            m = re.search(r"IMAGE_REL_I386_REL32\s+(\S+)", line)
            if m and cur:
                calls.append(m.group(1))
            m = re.match(r"^\s+([0-9a-f]+):", line)
            if m and cur:
                size = max(size, int(m.group(1), 16))
        if cur:
            rows.append((cur, calls, size))
        for name, calls, size in rows:
            print(f"  {name:28} {size:4}  calls: {' '.join(calls) or '-'}")


if __name__ == "__main__":
    main()
