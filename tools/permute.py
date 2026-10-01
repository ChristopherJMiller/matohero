#!/usr/bin/env python3
"""Search source variants of one function for a closer match to the original.

  tools/permute.py NuVecRotateX            report the best variant found
  tools/permute.py NuVecRotateX --apply    also write it into the source file
  tools/permute.py NuVecRotateX --try f    score hand-written bodies from file f instead
                                           (bodies between the braces, separated by
                                           lines of ----)

The function is found by name next to its `// FUNCTION: BIONICLE 0x...` marker, or by that
address (`tools/permute.py 0x56d8f0`) when the name is ambiguous (static functions). Variants are
written to a scratch directory under build/ (so neither the build nor reccmp sees them) and compiled in batches by
one cl run each, since Wine startup dominates a compile. The function is compared
in-process: both versions are disassembled and operands that
the linker would fill in (relocations in our object, absolute addresses in the original)
are masked before a sequence match. The best candidate is then confirmed with reccmp.

Moves, from what mattered on earlier files (docs/matching.md):
  * flip the operands of one product `a * b`;
  * flip a comparison: `a < b` <-> `b > a`, `a == b` <-> `b == a`;
  * swap two adjacent simple statements in one block that don't touch each other's
    targets, or two adjacent leading declarations (register allocation and x87 stack
    order follow source order).
Search: batches of random one- or two-move neighbours; the best is taken even on a tie,
to cross plateaus; after a few rounds without a new best it restarts near the best.
"""
import argparse
import difflib
import random
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
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
    """Returns (source, address, name, text, body start, body end) for a function name or
    marker address."""
    if re.fullmatch(r"0x[0-9a-fA-F]+", name):
        pat = rf"// FUNCTION: BIONICLE ({re.escape(name)})\n([^\n]*?\b(\w+)\s*\(.*\)\s*\{{)"
    else:
        pat = rf"// FUNCTION: BIONICLE (0x[0-9a-fA-F]+)\n([^\n]*\b({re.escape(name)})\s*\(.*\)\s*\{{)"
    for src in sorted(build.SRC.rglob("*")):
        if src.suffix.lower() not in (".c", ".cpp"):
            continue
        text = src.read_text()
        m = re.search(pat, text)
        if m:
            body_start = m.end(2)
            depth, i = 1, body_start
            while depth:
                depth += {"{": 1, "}": -1}.get(text[i], 0)
                i += 1
            return src, int(m.group(1), 16), m.group(3), text, body_start, i - 1
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
    """Sequence similarity, plus a small bonus for instructions already in the same
    position (separates variants the ratio alone rates equal). 1.0 means identical."""
    a, b = relative_jumps(ours), relative_jumps(orig)
    ratio = difflib.SequenceMatcher(None, a, b, autojunk=False).ratio()
    same = sum(1 for x, y in zip(a, b) if x == y) / max(len(a), len(b), 1)
    return 0.98 * ratio + 0.02 * same


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


STMT = re.compile(r"^(\s+)([^;{}()=!<>]*?[\w\]])\s*(=|\+=|-=|\*=|/=)\s*([^;]+);$")


class Model:
    """A function body as lines, with the moves the search can make: flip one product or
    comparison (tracked per original line), or swap two adjacent statements or leading
    declarations that don't touch each other's targets."""

    def __init__(self, body):
        self.lines = body.split("\n")
        self.prods = [list(PRODUCT.finditer(l)) for l in self.lines]
        self.comps = [list(COMPARE.finditer(l)) for l in self.lines]
        _, decl = declarations(body)
        self.decl = set(decl)
        self.stmt = {}
        for i, line in enumerate(self.lines):
            m = STMT.match(line)
            if m and i not in self.decl and not line.strip().startswith(("return", "if", "for", "while")):
                self.stmt[i] = (m.group(1), m.group(2).strip())
        self.flippable = [(i, "p", k) for i, ms in enumerate(self.prods) for k in range(len(ms))]
        self.flippable += [(i, "c", k) for i, ms in enumerate(self.comps) for k in range(len(ms))]

    def initial(self):
        return (frozenset(), tuple(range(len(self.lines))))

    def render(self, state):
        flips, order = state
        out = []
        for i in order:
            line = self.lines[i]
            for kind, matches, swap in (("p", self.prods[i], swap_product), ("c", self.comps[i], swap_compare)):
                for k in sorted((k for (j, t, k) in flips if j == i and t == kind), reverse=True):
                    m = matches[k]
                    line = line[:m.start()] + swap(m) + line[m.end():]
            out.append(line)
        return "\n".join(out)

    def swappable(self, a, b):
        """Adjacent lines a, b (original indices) may trade places."""
        if a in self.decl and b in self.decl:
            return True
        if a in self.stmt and b in self.stmt and self.stmt[a][0] == self.stmt[b][0]:
            la, lb = self.stmt[a][1], self.stmt[b][1]

            def uses(target, line):
                return re.search(rf"(?<![\w>.]){re.escape(target)}(?![\w])", line)

            return la != lb and not uses(la, self.lines[b]) and not uses(lb, self.lines[a])
        return False

    def moves(self, state):
        flips, order = state
        out = [("flip", f) for f in self.flippable]
        out += [("swap", j) for j in range(len(order) - 1) if self.swappable(order[j], order[j + 1])]
        return out

    def apply(self, state, move):
        flips, order = state
        if move[0] == "flip":
            return (flips ^ {move[1]}, order)
        j = move[1]
        o = list(order)
        o[j], o[j + 1] = o[j + 1], o[j]
        return (flips, tuple(o))

    def random_step(self, state, rng, n=1):
        for _ in range(n):
            ms = self.moves(state)
            if not ms:
                break
            state = self.apply(state, rng.choice(ms))
        return state


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("function")
    ap.add_argument("--apply", action="store_true", help="write the best variant into the source")
    ap.add_argument("--budget", type=int, default=300, help="max variants to compile")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--time", type=int, default=900, help="wall-clock limit in seconds")
    ap.add_argument("--try", dest="try_file", type=Path, help="score the bodies in this file")
    args = ap.parse_args()
    rng = random.Random(args.seed)

    src, addr, args.function, text, b0, b1 = find_function(args.function)
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
                path = tmp / "src" / f"permvar_{j}{src.suffix}"
                path.parent.mkdir(exist_ok=True)
                path.write_text(text[:b0] + cand + text[b1:])
                paths.append(path)
                if path not in written:
                    written.append(path)
            build.compile_many(paths, tmp, extra_includes=[src.parent], project=build.project_of(src))
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

    if args.try_file:
        bodies = [b.strip("\n") for b in re.split(r"(?m)^----$", args.try_file.read_text())]
        bodies = ["\n" + b + "\n" for b in bodies if b.strip()]
        try:
            scores = evaluate_many([body] + bodies)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
        print(f"{args.function} @ {addr:#x}: current source {scores[0]:.4f}")
        for i, (b, sc) in enumerate(zip(bodies, scores[1:])):
            print(f"  variant {i}: {sc:.4f}" + ("  (doesn't compile)" if sc < 0 else ""))
        return 0

    model = Model(body)
    try:
        cur = model.initial()
        cur_score = evaluate(model.render(cur))
        best, best_score = cur, cur_score
        nmoves = len(model.moves(cur))
        print(f"{args.function} @ {addr:#x}: baseline {best_score:.4f}; "
              f"{len(model.flippable)} flippable, {nmoves} moves from the start")
        stale = 0
        empty = 0
        deadline = time.time() + args.time
        while len(seen) < args.budget and best_score < 1.0 and nmoves and time.time() < deadline:
            # A batch of random neighbours (one or two moves each), compiled together.
            batch = []
            for _ in range(24):
                st = model.random_step(cur, rng, 1 if rng.random() < 0.7 else 2)
                if model.render(st) not in seen:
                    batch.append(st)
            if not batch:
                # Everything near here was tried; after enough misses the space is exhausted.
                empty += 1
                if empty >= 20:
                    print("  search space exhausted")
                    break
                cur = model.random_step(best, rng, 3)
                continue
            empty = 0
            scores = evaluate_many([model.render(st) for st in batch])
            k = max(range(len(batch)), key=lambda i: scores[i])
            if scores[k] > best_score:
                best, best_score = batch[k], scores[k]
                stale = 0
                print(f"  {best_score:.4f} after {len(seen)} variants")
            else:
                stale += 1
            if scores[k] >= cur_score:
                cur, cur_score = batch[k], scores[k]  # ties accepted: crosses plateaus
            if stale >= 4:
                cur = model.random_step(best, rng, 3)  # restart near the best with a kick
                cur_score = evaluate(model.render(cur))
                stale = 0
        best_body = model.render(best)
        print(f"best {best_score:.4f} after {len(seen)} variants")
        if best_body != body:
            diff = difflib.unified_diff(body.splitlines(), best_body.splitlines(), lineterm="", n=0)
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
            if f"({addr:#x})" in line:
                print(re.sub(r"\x1b\[[0-9;]*m", "", line).strip())


if __name__ == "__main__":
    sys.exit(main())
