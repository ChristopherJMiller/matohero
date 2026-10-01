#!/usr/bin/env python3
"""Search source variants of one function for a closer match to the original.

  tools/permute.py NuVecRotateX            report the best variant found
  tools/permute.py NuVecRotateX --apply    also write it into the source file

The function is found by name next to its `// FUNCTION: BIONICLE 0x...` marker. Variants are
written next to the source (permvar_N.c, removed afterwards) and compiled in batches by
one cl run each, since Wine startup dominates a compile. The function is compared
in-process: both versions are disassembled and operands that
the linker would fill in (relocations in our object, absolute addresses in the original)
are masked before a sequence match. The best candidate is then confirmed with reccmp.

Variant dimensions, from what mattered on earlier files:
  * operand order of each product `a * b` (MSVC canonicalizes sums, not products);
  * operand order of comparisons: `a < b` <-> `b > a`, `a == b` <-> `b == a` (the
    load order of x87 compares follows it);
  * order of the local variable declarations (affects register allocation).
Small spaces are searched exhaustively, larger ones by hill climbing.
"""
import argparse
import difflib
import itertools
import random
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

import capstone
import pefile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build  # noqa: E402
import coffpatch  # noqa: E402

ROOT = build.ROOT
EXE = ROOT / "orig" / "Bionicle.exe"
IMAGE_LO, IMAGE_HI = 0x400000, 0x3800000
TERM = r"[A-Za-z_][\w]*(?:(?:->|\.)[\w]+|\[[^\]]*\])*|\d+(?:\.\d+)?f?"
PRODUCT = re.compile(rf"({TERM}) \* ({TERM})")
COMPARE = re.compile(rf"({TERM}) (<=|>=|==|!=|<|>) ({TERM})")
FLIP = {"<": ">", ">": "<", "<=": ">=", ">=": "<=", "==": "==", "!=": "!="}
MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def find_function(name):
    for src in sorted(build.SRC.rglob("*")):
        if src.suffix.lower() not in (".c", ".cpp"):
            continue
        text = src.read_text()
        m = re.search(rf"// FUNCTION: BIONICLE (0x[0-9a-fA-F]+)\n([^\n]*\b{re.escape(name)}\s*\([^)]*\)\s*\{{)", text)
        if m:
            start = m.start(2)
            body_start = m.end(2)
            depth, i = 1, body_start
            while depth:
                depth += {"{": 1, "}": -1}.get(text[i], 0)
                i += 1
            return src, int(m.group(1), 16), text, body_start, i - 1
    sys.exit(f"{name}: no annotated definition in src/")


def original_size(addr):
    import csv
    for row in csv.DictReader(open(ROOT / "ghidra" / "export" / "functions.csv")):
        if int(row["address"], 16) == addr:
            return int(row["size"])
    return None


def original_bytes(addr):
    pe = pefile.PE(str(EXE), fast_load=True)
    data = pe.get_memory_mapped_image()[addr - 0x400000:addr - 0x400000 + 0x4000]
    return data


def obj_function(obj, name):
    """(code bytes, set of relocated byte offsets) for the function's COMDAT section."""
    data = obj.read_bytes()
    nsect = struct.unpack_from("<H", data, 2)[0]
    opt = struct.unpack_from("<H", data, 16)[0]
    for _, sym, value, section, _ in coffpatch.symbols(data):
        if section > 0 and (sym == "_" + name or sym == name or sym.startswith(f"?{name}@")):
            hdr = 20 + opt + (section - 1) * 40
            size, ptr, rptr = struct.unpack_from("<III", data, hdr + 16)[0], *struct.unpack_from("<II", data, hdr + 20)
            nrel = struct.unpack_from("<H", data, hdr + 32)[0]
            code = data[ptr + value:ptr + size]
            relocs = set()
            for r in range(nrel):
                off = struct.unpack_from("<I", data, rptr + r * 10)[0] - value
                relocs.update(range(off, off + 4))
            return code, relocs
    return None, None


def norm_ours(code, relocs):
    out = []
    for insn in MD.disasm(code, 0):
        text = f"{insn.mnemonic} {insn.op_str}"
        if any(o in relocs for o in range(insn.address, insn.address + insn.size)):
            text = re.sub(r"0x[0-9a-f]+", "R", text)
        out.append(text)
        if insn.mnemonic == "ret" and insn.address + insn.size >= len(code):
            break
    return out


def norm_orig(code, addr, length):
    out = []
    for insn in MD.disasm(code[:length], addr):
        text = f"{insn.mnemonic} {insn.op_str}"
        if insn.mnemonic in ("call", "jmp") and insn.op_str.startswith("0x") and insn.size == 5:
            text = f"{insn.mnemonic} R"
        else:
            text = re.sub(r"0x[0-9a-f]{6,}", lambda m: "R" if IMAGE_LO <= int(m.group(0), 16) < IMAGE_HI else m.group(0), text)
        out.append(text)
    return out


def relative_jumps(lines):
    # Short/near jumps inside the function: compare by shape, not absolute target.
    return [re.sub(r"^(j\w+) 0x[0-9a-f]+$", r"\1 L", l) for l in lines]


def score(ours, orig):
    return difflib.SequenceMatcher(None, relative_jumps(ours), relative_jumps(orig), autojunk=False).ratio()


def products(body):
    return [m for m in PRODUCT.finditer(body)]


def apply_flips(body, matches, flips, swap):
    out, last = [], 0
    for m, f in zip(matches, flips):
        out.append(body[last:m.start()])
        out.append(swap(m) if f else m.group(0))
        last = m.end()
    out.append(body[last:])
    return "".join(out)


def swap_product(m):
    return f"{m.group(2)} * {m.group(1)}"


def swap_compare(m):
    return f"{m.group(3)} {FLIP[m.group(2)]} {m.group(1)}"


DECL = re.compile(r"^    [A-Za-z_][\w ]*[\w*] \**(\w+)(\[\w*\])?( = (.*))?;$")


def declarations(body):
    """Leading declarations at the top of the body that can be reordered freely: plain
    ones, and initialized ones whose initializer doesn't use another of them."""
    lines = body.split("\n")
    idx, names, inits = [], [], []
    for i, line in enumerate(lines):
        m = DECL.match(line)
        if m:
            idx.append(i)
            names.append(m.group(1))
            inits.append(m.group(4) or "")
        elif line.strip() and idx:
            break
    for init in inits:
        # A name after -> or . is a member, not one of these locals.
        if any(re.search(rf"(?<![\w>.]){n}\b", init) for n in names):
            return lines, []
    return lines, idx


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("function")
    ap.add_argument("--apply", action="store_true", help="write the best variant into the source")
    ap.add_argument("--budget", type=int, default=200, help="max variants to compile")
    args = ap.parse_args()

    src, addr, text, b0, b1 = find_function(args.function)
    body = text[b0:b1]
    orig_code = original_bytes(addr)
    size = original_size(addr)
    tmp = Path(tempfile.mkdtemp(dir=build.BUILD))
    seen = {}
    orig_norm = norm_orig(orig_code, addr, size) if size else None
    written = []

    def evaluate_many(candidates):
        """Score candidates (bodies), compiling the new ones in one batch."""
        nonlocal orig_norm
        new = [c for c in dict.fromkeys(candidates) if c not in seen]
        for i in range(0, len(new), 40):
            chunk = new[i:i + 40]
            paths = []
            for j, cand in enumerate(chunk):
                path = src.parent / f"permvar_{j}{src.suffix}"
                path.write_text(text[:b0] + cand + text[b1:])
                paths.append(path)
                if path not in written:
                    written.append(path)
            build.compile_many(paths, tmp)
            for path, cand in zip(paths, chunk):
                obj = tmp / (path.stem + ".obj")
                code, relocs = obj_function(obj, args.function) if obj.exists() else (None, None)
                if code is None:
                    seen[cand] = -1.0
                    continue
                ours = norm_ours(code, relocs)
                if orig_norm is None:
                    orig_norm = norm_orig(orig_code, addr, len(code) + 16)
                seen[cand] = score(ours, orig_norm)
                obj.unlink()
        return [seen[c] for c in candidates]

    def evaluate(candidate):
        return evaluate_many([candidate])[0]

    try:
        best_body, best = body, evaluate(body)
        print(f"{args.function} @ {addr:#x}: baseline {best:.4f}")
        prods = products(body)
        comps = [m for m in COMPARE.finditer(body)]
        dims = len(prods) + len(comps)

        def variant(pflips, cflips, order):
            b = apply_flips(body, prods, pflips, swap_product)
            # Comparisons are matched on the product-flipped text.
            cm = [m for m in COMPARE.finditer(b)]
            if len(cm) == len(cflips):
                b = apply_flips(b, cm, cflips, swap_compare)
            lines, idx = declarations(b)
            if order and idx:
                decl = [lines[i] for i in idx]
                for i, j in zip(idx, order):
                    lines[i] = decl[j]
            return "\n".join(lines)

        _, decl_idx = declarations(body)
        orders = list(itertools.permutations(range(len(decl_idx)))) if len(decl_idx) <= 4 else [tuple(range(len(decl_idx)))]
        space = (2 ** dims) * len(orders)
        print(f"  {len(prods)} products, {len(comps)} comparisons, {len(decl_idx)} declarations: {space} variants")
        if space <= args.budget:
            cands = [variant(f[:len(prods)], f[len(prods):], o)
                     for f in itertools.product([0, 1], repeat=dims) for o in orders]
            for cand, sc in zip(cands, evaluate_many(cands)):
                if sc > best:
                    best, best_body = sc, cand
            print(f"  exhaustive: best {best:.4f}")
        else:
            state = [0] * dims
            order = tuple(range(len(decl_idx)))
            improved = True
            while improved and len(seen) < args.budget and best < 1.0:
                improved = False
                neighbours = []
                for mv in list(range(dims)) + [("swap", i) for i in range(len(decl_idx) - 1)]:
                    if isinstance(mv, int):
                        st, o = state[:], order
                        st[mv] ^= 1
                    else:
                        st, o = state, list(order)
                        o[mv[1]], o[mv[1] + 1] = o[mv[1] + 1], o[mv[1]]
                        o = tuple(o)
                    neighbours.append((st, o, variant(st[:len(prods)], st[len(prods):], o)))
                scores = evaluate_many([n[2] for n in neighbours])
                top = max(range(len(neighbours)), key=lambda k: scores[k])
                if scores[top] > best:
                    best = scores[top]
                    state, order, best_body = neighbours[top]
                    improved = True
                    print(f"  {best:.4f} after {len(seen)} variants")
        print(f"best {best:.4f} after {len(seen)} variants")
        if best_body != body:
            diff = difflib.unified_diff(body.splitlines(), best_body.splitlines(), lineterm="", n=1)
            print("\n".join(list(diff)[2:]))
    finally:
        for path in written:
            path.unlink(missing_ok=True)
        shutil.rmtree(tmp, ignore_errors=True)

    if args.apply and best_body != body:
        src.write_text(text[:b0] + best_body + text[b1:])
        print(f"applied to {src.relative_to(ROOT)}; confirming with reccmp...")
        subprocess.run([str(ROOT / "tools" / "build.py")], check=True, capture_output=True)
        out = subprocess.run([str(ROOT / "tools" / "reccmp"), "reccmp", "--target", "BIONICLE"],
                             capture_output=True, text=True).stdout
        for line in out.splitlines():
            if f" {args.function} (" in line:
                print(re.sub(r"\x1b\[[0-9;]*m", "", line).strip())


if __name__ == "__main__":
    sys.exit(main())
