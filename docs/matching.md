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
- **…but grouping shows.** A four-term sum compiles the same for any term order, but
  parentheses change the evaluation order: `NuVec4MtxTransformH`'s `w, z, y, x` order
  came from `a + b + (c + d)` (83% → 100%). Several groupings give the same code.
- **…but not for `__int64`.** `now + (MAX - base)` was reassociated into
  `now - base + MAX`; the original's order came from updating in place,
  `now.QuadPart += MAX - base` (`NuTimeGetTicksPS`).
- **Parentheses stop reassociation.** `dy * 512.0f / dx` became `dy / dx * -512` with a
  negated table index; `(dy * 512.0f) / dx` matched (`fxyd` in nutrig).
- **…and constant folding.** `fov * 0.5f * 10430.378f` (or `fov / 2.0f * …`) compiles to
  one `fmul` by 5215.19; the original's two `fmul`s came from
  `(fov * 0.5f) * 10430.378f` (`NuCameraSetVPortClipMtx`).
- **A `static` function in a header is ICF-folded across the whole exe.** `NuCameraSetEx`
  calls a 64-byte matrix copy at `0x414140` with `src` in `eax` and `dst` in `ecx`; the
  same function serves ~100 call sites in game code. MSVC gives static functions register
  arguments, and a struct assignment would have been an inline `rep movsd`, so the source
  has an element-by-element copy as a `static` function in a shared header (`NuMtxCopy` in
  `numtx.h`, name ours). The marker sits in the header.
- **A local `__declspec(align(16))` matrix gives `push ebp; mov ebp, esp; and esp, -16`.**
  `NuCameraBuildClipPlanes` keeps its plane matrices in the PS2's `_VUMTX` (now a 16-byte
  aligned union in `numath/nuvu0.h`).
- **Repeated constant stores are grouped.** A float constant stored more than once goes
  into a register, and all its stores are emitted together where its group starts;
  single stores stay `mov [mem], imm32` in source order. Which register a constant gets
  shows where its first use was: in `NuCameraCreate`, zero in `ecx` (loaded before
  `1.0f` in `eax`) and zero stores interleaved with the immediates meant a zero store
  came before the `scale` stores in the source. Variants were compiled as hundreds of
  functions in one file (one `cl` run) and ranked by their store sequences.
- **A local vs. the same expression inline changes term order**, even when the asm
  evaluates it at the same point. `NuPlnLine3` with `dot = …; dist = … / dot;` was 76%;
  inlining the dot product into the division made it an effective match (96%).
- **A call result kept on the x87 stack and popped later (`fstp st(0)`) was a local.**
  `fld 1.0; fdiv st, st(1)` … `fstp st(0)` is `d1 = NuVecMag(&v1); … 1.0f / d1`;
  `1.0f / NuVecMag(&v1)` gives `fdivr [1.0]` (`NuInfiniteLineToPointDistSqrEx`).
- **`fxch; fdiv st, st(1); fxch; fstp st(0)` at the end is a store before the return.**
  `v /= sign; return v;` gives it; `return v / sign;` gives `fdivp` (`NuAToF`).
- **…including inside `MIN`/`MAX`.** `MIN(x + r, 1.0f)` compares the sum against memory
  (`fcom [1.0]`); the original's `fld [1.0]; fcomp st(1)` was `MIN(1.0f, x + r)`
  (`NuPadRead`'s d-pad ramps, 90% → 100%).
- **Comparison operand order is visible too.** `pow_x[n] == x` loads `x` first, and
  `x == pow_x[n]` loads `pow_x[n]` first (`NuPow`). The permuter flips these.
- **Early returns keep their own epilogue.** `if (a) return 0; if (b) return 0;`
  compiles to two returns; `if (a || b) return 0;` shares one (`NuFParInterpretWord`).
  Likewise `if (x) {…return…} else { return 0; }` vs. a leading `if (!x) return 0;`
  decides where the zero return lands (`NuFParGetInt`).
- **Registers pushed before an early `return 0` mean the source had no early return.**
  MSVC shrink-wraps early returns (they leave before the pushes); the original's
  functions often push first and jump to one shared `xor eax, eax` tail, which comes from
  wrapping the body in `if (ok) { … }` and returning 0 at the end (`NuDatFileOpen` 69% →
  97%). Two failure cases sharing one cleanup statement can be a single condition:
  `if (f == -1 || (ix = Find(…)) < 0) datfiles[f].used = 0;`.
- **A condition evaluated twice is a macro.** `NuDatFileRead` computes a clamp with
  `sets/dec/and`, then re-tests the sign in the other branch: `MIN(size, MAX(0, rem))`
  with function-like macros expanding their arguments twice.
- **Branch order follows the `if`.** `test ah, 1; jne` comes from
  `if (x >= 0.0f) {…} else {…}`, while `if (x < 0.0f)` gives `test ah, 5; jp`
  (`NuASin`).
- The float-to-int helper the original calls at `0x5b4d0c` is `_ftol2` (Ghidra's
  function ID calls it `__ftol`). It's named for reccmp in `src/library.h`.
- `sin`/`log`/`exp` compile to inline x87 (`fsin`, `fyl2x`, `f2xm1`…) under `/Oi`,
  which `/O2` implies.

## Integers and control flow

- `dec edi; test edi, edi; jle` is `if (--n <= 0)`; `n < 1` gives `cmp edi, 1; jl`
  (`NuStrNCpy`).
- **`x *= 16` and `x <<= 4` both compile to `shl`, but allocate registers differently.**
  `curr_num <<= 4; curr_num += hexcodetoint(…);` shifted in `edx` and copied to `ebx`
  for the call; `*= 16` shifts in `ebx` directly, as the original does (nurdpi
  `get_tok`, 96% → 100%). Try the arithmetic spelling when a shift's register is off.
- **`xor`/`and` pairs are written with `^`.** Button edges compiled as `c = new ^ old;
  released = c & old; pressed = c & new` came from `(new ^ old) & old` and
  `(new ^ old) & new`; `new & ~old` gives `not`/`and` (`NuPadProcessReadData`). The two
  stores came out in the opposite order to the statements.
- **A block placed far from its condition can be a `goto`.** In `NuPadUpdatePads` the
  "found a pad" assignments sit after the rest of the loop body, reached by a `je` out of
  the inner search loop; `break` puts them right after it, a `goto found;` to a label at
  the end of the outer loop body (after a `continue;`) matches (57% → 100% with the
  next point).
- `cmp eax, edi` with the loop counter in `eax` was `i == mapping.port`, not
  `mapping.port == i`: equality operands keep the source order too.
- `dec edx; js` after `ihandle--` is `ihandle >= 0` (or `< 0`); `ihandle > -1` gives
  `cmp eax, -1; jle` (`NuFilePakGetItemInfo`).
- **Splitting a pointer computation into two assignments changes register allocation.**
  `item = GetItems(hdr); item = &item[ihandle];` matched where
  `item = &GetItems(hdr)[ihandle];` (same code, other registers) was 52%
  (`NuFilePakGetItemInfo`, `NuFilePakSetItemRequired`).
- Stores to a local struct follow the source order: `SECURITY_ATTRIBUTES` filled
  `bInheritHandle` first in the original (`NuFileCreateDir`).
- **Statement order moves register choices even between independent statements.**
  `NuMemAllocFn` computes the tail pointer once (`tail = …`) before copying the head mark;
  writing it after the head copy, or inline twice, gave other registers (87% → 100%).
  `NuMemFreeFn`'s `totalloc -= size` came after the `succ`/`prev = NULL` stores.
- `test edx, edx; jbe` on a pointer is a relational test, `if (ptr > NULL)`; `ptr != NULL`
  gives `je` (`NuMemBlkAlloc`).
- **A struct field read twice in a row (`cmp [eax], 0` then `mov edx, [eax]`) and a
  bitfield set as load/`or`/store means a `volatile` pointee.** The background loader's
  `volatile BGPROCINFO *bg_active` took `bgThreadMain` and `NuFrameEndBgLoadPS` from
  74–84% to exact.
- **`*d++ = *s++` vs separate statements.** `NuMemMove`'s copy loops matched as
  `*d = *s; d++; s++;` (and `d--; s--; *d = *s;` backwards); the combined forms put the
  pointers in other registers (48% → 100%).
- A byte parameter loaded with `movzx` was `unsigned char`; plain `char`/`int` gave
  `mov al, [esp+8]` (`NuMemSet128`'s fill value).
- **`if (a) return x; else if (b) return y; else return NULL;` vs separate `if`s.** The
  chained form put the second test in its own block that falls back on the zeroed
  register for `NULL` (`NuSpecialGetPos` 67% → 100%); separate `if`s merged the two
  returns and added a `xor eax, eax`.
- **Pointer increments before the counter.** `for (...; sp++, n++)` matched where
  `n++, sp++` left only an `inc` in the wrong place (the nuspecial finders).
- **A test that picks an argument may be two calls.** `NuStrICmpWC(name, x, wc ? *wc :
  NULL)` compiled to one call with a select; the original has two call sites, from
  `if (wc) { if (Cmp(name, x, *wc)) continue; } else if (Cmp(name, x, NULL)) continue;`
  (`FindMultiWCObj` 43% → 100%).
- **Repeating a cleanup tail per path matters.** `NuSpecialFind`'s "not found" stores
  appear twice in the original, once for the early exit and once after the loop; one
  shared tail after `if (scene && name) {...}` merged them (64% → 100%).
- Compare against the constant the asm shows: `cmp eax, 0x8000; jl` is
  `if (a >= 0x8000)`, not `if (a > 0x7fff)` (`NuAngAdd`).
- `test eax, eax; jns` on a float's bits is `if (*(unsigned int *)&f & 0x80000000)`;
  `*(int *)&f < 0` gives `jge` (`NuFsign`).
- MSVC reuses a dead parameter's stack slot for temporaries (`NuEquiv` stores `f1 - f2`
  over `f1`), so a write to `[esp+4]` isn't necessarily an assignment to the parameter.
- Branchless selects come from `?:`, and the asm's `setl`/`setge` shows which way it
  was written: `v >= 128 ? 128 : 1` vs `v < 128 ? 1 : 128` (`NuPower2`).
- `if/else` assignments of floats compile to integer moves; `?:` doesn't
  (`NuVecMax`/`Min`).

## Structure

- **Only `__inline` functions are inlined (`/Ob1`).** The original was built with
  `/O2 /Ob1`, the setting VS .NET 2003's Release configs use (as far as I know), not
  `/O2`'s implied `/Ob2`. The proof is `NuSPrintfW`: it inlines `NuStrLenW` but
  calls `NuAsciiToUnicode`, and under `/Ob2` no source order or shape stopped
  `NuAsciiToUnicode` from being inlined. When the original inlines a helper,
  mark the helper `__inline` (it still gets an out-of-line copy if something calls it
  that way): `NuStrLen`, `NuStrLenW`, `NuStrRChr`, `NuToUpper`, `NuToLower`,
  `NuToUpperW`, `NuRandFloatSeeded`, `NuRandIntSeeded`, `NuVecSub`, `NuVecCross`,
  `NuVecMagSqr`, `NuVecMag`, `NuGetWChar`, `NuFParUnGetWord`, `NuFParClose`,
  `NuFParInit`, `NuFreeFPar` so far. Whether the compiler then inlines a given call
  is still its own choice (`NuStrNCmp` calls `NuStrLen`, `NuStrGetExt` inlines it).
- **One helper, inlined in one caller and called by another: a `static __inline` copy.**
  `NuStringTableLoadCSV` inlines `NuAsciiToUnicode`, but `NuSPrintfW` calls it; marking
  `NuAsciiToUnicode` `__inline` got it inlined into `NuSPrintfW` too (exact → regressed).
  A file-local `static __inline AsciiToUnicode` used only by `LoadCSV` matches both. The
  original probably had a macro or a private copy there.
- **Function order in an object.** `/O2` implies `/Gy`, so each function is its own
  COMDAT, emitted in source order, except that a function calling one defined *later* in
  the file (i.e. not compiled yet) is deferred to the end of the object, whether or not
  the call ends up inlined. In
  nustring the case-insensitive functions, which call `NuToUpper`/`NuToLower`, come
  last in the exe for that reason. reccmp doesn't care about order, but it explains
  layouts that look shuffled against the PS2 file, and a later-defined callee still
  inlines. A `static` function also gets a custom register calling convention.

- **Inline helpers have exact semantics.** BH's `NuFdiv(a, b)` returns 0 if *either*
  operand is 0 (two compares in `NuQuatSlerp`). With a constant numerator the extra
  test folds away, so `NuVecMtxTransformH` matched with the wrong definition. Getting
  it right lifted `NuMtxAlignZ` from 63% to 94.5%. Likewise `NuFabs` clears the sign
  bit through memory (`mov eax, [x]; and eax, 0x7fffffff; mov [x], eax; fld [x]`), not
  `fabs` (`NuEquiv`, `NuMtxAlignZ`).
- **Check saga's structure against the asm.** saga targets a later engine, which added
  things BH doesn't have: `NuMtxMul/MulH/MulR` branch on `m == m0 || m == m1`, while BH
  always builds into a local and copies (`sub esp, 0x40` … `rep movsd`); `NuMtxInvRSS`
  checks for a zero determinant, BH doesn't. Fixing structure first took these from
  14–38% to 55–70%.
- A wrapper that only calls another function with the same arguments compiles to a bare
  `jmp` (the PC `…VU0` thunks).
- **Inlining depends on what else is in the file: recursive `__inline` cycles.**
  `NuDatFileSeek` → `NuFileSeek` → `NuMemFileSeek` → `NuDatFileSeek` is a cycle of
  `__inline` functions, and MSVC has to stop inlining somewhere. An `__inline`
  function's out-of-line copy is emitted when something first calls it out of line, and
  where it lands decides where the cycle is cut. The original cuts at `NuDatFileSeek`'s
  own call to `NuFileSeek` (its copy is emitted in place and calls `NuFileSeek`). A first
  version of `NuDatFileLoadBuffer` that called `NuDatFileSeek` directly moved that copy to
  the end of the object, after `NuFileSeek`; it then inlined the whole chain, and every
  other caller's inlining changed (`NuDatFileSeek` 100% → 49%, `NuDatOpenEx` 98% → 62%).
  The original `NuDatFileLoadBuffer` seeks through `NuFileSeek` (its asm calls the
  inlined chain's `NuDatFileSeek`, not `NuDatFileSeek` directly from the source); written
  that way it sits in `nufile.c` without disturbing anything. Found by delta-debugging
  `nufile.c` down to the five functions that still showed the effect (compile, list
  emission order and remaining calls as `tools/msvctest.py` does, drop function blocks
  while the effect persists). When a new function makes matched neighbours regress, look
  for an `__inline` cycle and for which entry point the new code calls.
- **Keep stubs in a separate file anyway.** Under `/Ob1` they aren't inlined, but a
  caller compiled after a callee in the same file uses the callee's actual register
  usage: `NuDatFileSeek` (deferred behind the `NuFileSeek` stub) kept a pointer in `ecx`
  across the call because the stub didn't touch it (30% → 100% once the stubs moved to
  `nufile_stubs.c`).
- **Identical functions are folded (`/OPT:ICF`).** The PC's memory-card layer is a set of
  `return 0` functions, all folded into one copy at `0x57a600`; one stub stands for all of
  them. Likewise every empty `void` PS2 hardware hook (`NuPSFileInitDevices`,
  `NuVideoSetSwapModePS`, …) is one function at `0x5af030` with 247 callers.
  `nucore/folded.h` maps the original names onto unprototyped stubs (`pc/folded_pc.c`). An unreferenced `static` function is dropped entirely, so its
  marker can only go in once something calls it (`AquireFileBuffer`).
- **`add reg, 0x10; jne` loops over a static array are `for (p = array; p != NULL; p++)`.**
  The pointer never becomes NULL, so the loop only ends via a match; the compiler keeps
  the useless test (the extension table in nufile).
- **An `__inline` function with no out-of-line call in its own file isn't emitted.**
  `NuFileSeek` is `__inline` in the original (it's inlined into `NuFileEndBlkRead`), but
  marking it so before its inlining callers exist dropped it from the object and left
  other files' calls unresolved. Add the `__inline` together with those callers.
- **A loop in an inlined helper isn't unrolled; the same loop written inline is.**
  `NuPSFileOpen`'s free-handle search became a 4× unrolled loop until moved into a
  `static __inline` helper (58% → 81%).
- **`static` functions get register arguments, and ours match.** `NuFileExtAdd` takes its
  string in `edi` and `AquireFileBuffer` its `FILEINFO *` in `edi`; writing them as plain
  `static` functions reproduced that without help.
- **A global re-read after its own store is `volatile`.** `NuThreadCreateCriticalSection`
  compares the counter in memory, loads it, increments, stores, and loads it again for each
  use; a plain `static int` keeps it in `eax` (35% → identical).
- `__FILE__`/`__LINE__` call sites: pass the original line explicitly (`NuMemAllocAt`);
  `#line` breaks reccmp's line mapping.

## reccmp limits

- **reccmp scans every source file under `src/`, hidden ones included** (the build skips
  dot-files). A scratch copy of `nufile.c` left there as `.min_nufile.c` duplicated
  markers, and reccmp dropped the real ones ("Dropped duplicate address"). Keep scratch
  files out of `src/`.

- **A function signature split over several lines after a marker silently drops the
  marker**, and the parser loses the next marker in the file too (no warning; the
  addresses just show as `<OFFSET>` in callers). Keep marked signatures on one line.

- A `STRING` marker must be followed by a line with the literal; one in front of a macro
  that expands to `__FILE__` made reccmp drop the whole function. Put those at file
  scope with a comment instead (`listman.c`, `nufile.c`).
- Float constants in the original are only recognized via `build/orig-floats.csv`
  (`tools/floatconsts.py`), since the exe has no relocations.
- A reference to `global - offset` (e.g. `NuTrigTable - 4` from a loop counting from 1)
  isn't mapped. The code is identical, but it shows as ~92% (`NuTrigInit`) or ~97%
  (`NuPow`, which writes its ring buffers through `pow_x - 4` etc.), ~95%
  (`NuFileOpenSize`'s `file_info[fh - 1]`) and ~90% (the PC critical sections,
  `criticalsections[id - 1]`). Candidate for an upstream reccmp improvement; meanwhile
  `tools/offsetmatch.py` proves these identical (same offset from the same global in
  both builds), and progress/hooks count them as effective.

## Open questions

- **`NuUTF8ToUnicode` (33%)**: identical code with `ecx`/`edx` swapped for the source
  pointer and the byte temporary. The permuter's moves and hand variants (loop forms,
  check order, post-increments) don't change it; it was never exact (not a regression).

- **`nuheap.c`'s swapped registers.** `NuHeapAllocAlignedNamed` keeps `reqsize` in `eax`
  and the loop's block in `ecx`; ours swaps them, and the register-argument static
  `SplitFreeBlock` follows the caller (its parameter order changes nothing). Untried:
  other spellings of `size += align`, a `do`/`while` search loop.

- **`pop ecx` vs `add esp, 4`.** After a final call whose float result is returned, the
  original cleans the 4-byte argument with `add esp, 4` and we emit `pop ecx`
  (`NuPowFast`, `NuVecDist`, `NuVecXZDist`). `/G5`–`/G7`, `/Ox`, `/Gy`, `/GF` and
  `/Ob1` don't change it. Probably source-level, not found yet.
- `NuVecRotateX/Z`: x87 stack order at the end; not product order or declaration order.
