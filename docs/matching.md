# Matching notes (MSVC 7.1 SP1)

Patterns learned while matching. Check these first when a function is close but not
exact; `tools/permute.py` automates some of them.

## Floating point (x87)

- **Sums are canonicalized; products aren't.** The order of `a + b + c` terms in the
  source makes no difference, but each product's operand order shows up in the code:
  `m0->_01 * v0->x` and `v0->x * m0->_01` compile differently (nuvec matrix functions,
  sometimes mixed within one expression). The permuter flips products.
- **…except when MSVC canonicalizes them itself.** For a product of two memory
  operands off the same pointer (`T->_22 * T->_10`), flipping the source changes
  nothing; the compiler picks the order. What does change it is expression structure:
  the inline nested-determinant form of `NuMtxInvRSS` got closer than precomputed
  cofactor locals (70% → 85%), presumably because canonical order follows where a
  subexpression first appears.
- **Parentheses stop reassociation.** `dy * 512.0f / dx` became `dy / dx * -512` with a
  negated table index; `(dy * 512.0f) / dx` matched (`fxyd` in nutrig).
- **Comparison operand order is visible too.** `pow_x[n] == x` loads `x` first, and
  `x == pow_x[n]` loads `pow_x[n]` first (`NuPow`). The permuter flips these.
- **Branch order follows the `if`.** `test ah, 1; jne` comes from
  `if (x >= 0.0f) {…} else {…}`, while `if (x < 0.0f)` gives `test ah, 5; jp`
  (`NuASin`).
- The float-to-int helper the original calls at `0x5b4d0c` is `_ftol2` (Ghidra's
  function ID calls it `__ftol`). It's named for reccmp in `src/library.h`.
- `sin`/`log`/`exp` compile to inline x87 (`fsin`, `fyl2x`, `f2xm1`…) under `/Oi`,
  which `/O2` implies.

## Integers and control flow

- Compare against the constant the asm shows: `cmp eax, 0x8000; jl` is
  `if (a >= 0x8000)`, not `if (a > 0x7fff)` (`NuAngAdd`).
- Branchless selects come from `?:`, and the asm's `setl`/`setge` shows which way it
  was written: `v >= 128 ? 128 : 1` vs `v < 128 ? 1 : 128` (`NuPower2`).
- `if/else` assignments of floats compile to integer moves; `?:` doesn't
  (`NuVecMax`/`Min`).

## Structure

- **Check saga's structure against the asm.** saga targets a later engine, which added
  things BH doesn't have: `NuMtxMul/MulH/MulR` branch on `m == m0 || m == m1`, while BH
  always builds into a local and copies (`sub esp, 0x40` … `rep movsd`); `NuMtxInvRSS`
  checks for a zero determinant, BH doesn't. Fixing structure first took these from
  14–38% to 55–70%.
- Inlining is on (`/Ob2`). A stub in the same file as its caller gets inlined into it and
  collapses the caller, so keep stubs in a separate file until decompiled
  (`numath/nutrig_stubs.c`).
- `__FILE__`/`__LINE__` call sites: pass the original line explicitly (`NuMemAllocAt`);
  `#line` breaks reccmp's line mapping.

## reccmp limits

- Float constants in the original are only recognized via `build/orig-floats.csv`
  (`tools/floatconsts.py`), since the exe has no relocations.
- A reference to `global - offset` (e.g. `NuTrigTable - 4` from a loop counting from 1)
  isn't mapped. The code is identical, but it shows as ~92% (`NuTrigInit`) or ~97%
  (`NuPow`, which writes its ring buffers through `pow_x - 4` etc.). Candidate for an
  upstream reccmp improvement.

## Open questions

- **`pop ecx` vs `add esp, 4`.** After a final call whose float result is returned, the
  original cleans the 4-byte argument with `add esp, 4` and we emit `pop ecx`
  (`NuPowFast`, `NuVecDist`, `NuVecXZDist`). `/G5`–`/G7`, `/Ox`, `/Gy`, `/GF` and
  `/Ob1` don't change it. Probably source-level, not found yet.
- `NuVecRotateX/Z`: x87 stack order at the end; not product order or declaration order.
