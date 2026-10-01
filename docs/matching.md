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
- **MSVC picks the operand order of a sum itself; the source only fixes the grouping.**
  `p->x * m._03 + p->y * m._13` and `p->y * m._13 + p->x * m._03` give the same code (the
  `y` product first), and a plain `A + B + C` comes out as `C, A, +, B, +` while
  `C0 + (Z + (X + Y))` gives `Y, X, +, Z, +, C0`. The order of the leaves follows when they
  were first referenced in the function: a statement reading `p->y` beforehand puts the
  `x` product first. Parentheses, product operand order, array or union access and
  `__inline` helpers don't change it (tested in `msvctest` files).
- **Several "return 1" exits share one tail only if the source has one.** With
  `if (!(or)) return 1; … if (or2) return 2; return 1;` MSVC gave the early exit its own
  epilogue and turned the end into `setne al; inc eax`; the original's
  `mov eax, 2; jnz end; mov eax, 1` came from nesting the second half in
  `if (or) { … if (!and2 && or2) return 2; } return 1;` (`NuCameraClipTestExtentsGeneric`).
- **MSVC unrolls some constant-count loops.** Of two 8-iteration outcode loops in one
  function, the one with a store through a pointer (`*min_z`) stayed a loop; the other
  was fully unrolled (the PS2 build has both as loops). The first unrolled iteration
  turned `oc[0] |= 0x20` into a plain store, since `oc` had just been zeroed.
- **A struct copy keeps stores that field assignments lose.** `NuVpUpdate` fills a local
  viewport, overwrites its first four fields later, and passes it on; the original keeps
  all six initial stores, which only `vp = *(NUPCVIEWPORT *)&vpCurrent;` reproduced
  (field-by-field assignments were dead-store-eliminated, 40% → 89%).
- **`_ftol2` doesn't clobber memory.** Globals loaded before a run of `(int)` conversions
  stay in callee-saved registers and are reused after them; which ones depends on the
  order of the later assignments (`h, w, x, y` in `NuVpUpdate`, found by permuting them).
- **MSVC knows when a static callee is read-only.** A load like `p->rooms` in a loop that
  calls a static function which only reads memory gets hoisted out of the loop (into a
  spill slot if registers are short), and later uses reuse it. In `NuPortalWhichRoom` our
  build stopped hoisting once the loop also had an indexed store into a local array and a
  `break`; the original hoists anyway (unsolved).
- **A register copy before a store suggests a chained assignment.** `mov eax, edx; mov
  [cam_plane], eax` … `mov [near_clip_plane], edx` came from
  `cam_plane.x = near_clip_plane.x = m._20;` (written the other way round, the stores
  swap; `NuPortalVisibility`, 82% → effective).
- **MSVC unrolls variable-count inner loops 4x** with a remainder loop when the body is
  small (`checkPortalVisibility`'s per-point plane test); the source is a plain loop.
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
- **A `char` array anywhere in a local turns on the /GS cookie.** `NuQFntReadPS` gained
  `__security_cookie` code because a placeholder struct (`NUSHADERMTLDESC`) padded its
  unknown fields with `unsigned char` arrays; with `int` arrays it matched. Pad
  placeholder structs with `int`s unless the asm shows a cookie.
- **`add [mem], reg` is a read-modify-write through a pointer.** `NuQFntLoadPtr` rebases
  two pointers in place; `fv->chars = (VUCHAR *)((char *)fv->chars + (int)fv)` compiled
  to load/add/store, the PS2's inline `RemapAddr` (`*(int *)vptr += (int)vto -
  (int)vfrom`) to the single `add`.
- **A call nested in an argument list vs a temporary.** `NuQFntReadPS(rv,
  NuTexCreate(header[1]), …)` loaded the other arguments before `NuTexCreate` and popped
  its argument right away; `tid = NuTexCreate(header[1]);` first gave the original's
  deferred stack cleanup over three calls (`NuQFntInit` 68% → 100%).
- **`return n++` in two branches shares one tail; `n++; return n - 1;` keeps two.**
  `NuDisplayListAddRenderScene` ends both branches with `inc`/store/`dec`/epilogue;
  `return dlm->nrender_scenes++;` in each merged them into one `mov`/`inc` tail.
- **A global pointer read once, then used through a register, was copied to a local.**
  `NuDisplayListLinkItem` loads `display_list_buffer` once after aligning through it and
  keeps it in `eax` across byte stores that could alias it; writing
  `display_list_buffer->item->…` reloaded the global for every access (18%), a local
  `buff = display_list_buffer;` after the alignment matched.
- **A local pointer to a global struct (`dlm = &global_dlist_manager`) is not just
  style.** The PS2 locals of `NuDisplayListSwapBuffersBeginFrame` include `dlm`;
  writing `dlm->dlists[j]` made MSVC walk the array with a pointer and a separate
  counter as the original does, where `global_dlist_manager.dlists[j]` indexed it
  (79% → 100%). When the PS2 lists such a pointer, use it.
- **`|` between a test and a flag survives inlining; `||` folds.** `NuHtmlEnd` inlines
  `NuHtmlFlush(1)` and still computes `txt > flush` (`cmp`/`sbb`/`neg`/`or 1`) before an
  always-taken branch; with `txt > flush || force` our build dropped the test (29% →
  100% with `(txt > flush) | force`).
- **Toggling a 1-bit field.** `bf = !bf`, `bf ^= 1`, `bf = bf ^ 1` and `bf = bf == 0`
  all give the original's `not`/`xor`/`and`/`xor` insert; `bf = ~bf` extracts the bit
  first (`shr`/`shl`).
- **A store common to both branches can be written in both.** `NuGScnGetSpecial` stores
  `spec->scene` after loading and testing `display_list`; one `spec->scene = gsc;` before
  the `if` stored it first (43%), the same statement at the top of each branch matched.
- **`a = b->f = call()` keeps the result in a register for the rest of the function.**
  `ReadInstAnimBlockDlist` indexes the block it just read through a register while
  storing into other structs; reading `gscene->instanimblock` again reloaded it after
  every store, `instanim = gscene->instanimblock = NuMemFileAddr(fh)` matched (the
  opposite chain order and a separate statement were 85%, the original 100%).
- **A pointer temporary inside an unrolled loop gives a second induction variable.**
  `clipPortal`'s 4× unrolled dot loop reads `x` through one pointer and `y`/`z` through
  another; `p = &portal->points[i]; … p->x …` reproduced that, `portal->points[i].x`
  didn't (48% → 92%).
- **`dec eax; je` / `sub eax, 3; jne` on a call's result is a `switch`, and MSVC lays the
  case bodies out in reverse source order.** `roomRecursive`'s outer switch puts
  `default`, then `case 2`, then `case 1` after the dispatch; its inner `if (clip == 1)
  … else if (clip == 4)` had to be `switch (clip) { case 1: … case 4: … }`.
- **`xor reg, reg; mov reg16, [mem]` on the first of two short loads.** `next_room =
  left; if (next_room == roomid) next_room = right;` folded the address and loaded with a
  bare `mov si`; `if (left != roomid) next_room = left; else next_room = right;` gave the
  original's `xor`/`mov` pair (`roomRecursive` 96% → 100%).
- **A PS2 local list without a loop counter means no loop.** `buildFrustrum`'s four
  transform-and-`w` steps are written out (PS2 has no `i`); as a loop MSVC kept it rolled,
  since it calls a function (54% → 94%).
- **One field, compared signed in one function and unsigned in another.**
  `NUDISPLAYSCENE.nmtls` is `unsigned int` (PS2 type). `DisplayListBeforeFrame`'s
  `(nmtls + 7) / 128` and `DisplayListMaterialClipUpdate`'s `>> 3` need it unsigned, but
  `DisplayListLinkDynamicMtls` compares it to `int` counters with `jl`/`jle`, so it casts
  `(int)dlist->nmtls` at each use, as `MaterialClipUpdate` already does once.

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
- **An `__inline` function with no caller in its own file isn't emitted.**
  `NuFileSeek` is `__inline` in the original (it's inlined into `NuFileEndBlkRead`), but
  marking it so before its inlining callers exist dropped it from the object and left
  other files' calls unresolved. Add the `__inline` together with those callers. Once
  it has callers, even only inlined ones, the out-of-line copy is emitted as well
  (`NuQFntEncodeUnicodeChar`, inlined into `NuQFntUTF8toQCode` and `NuQFntPrintLenU`).
- **A loop in an inlined helper isn't unrolled; the same loop written inline is.**
  `NuPSFileOpen`'s free-handle search became a 4× unrolled loop until moved into a
  `static __inline` helper (58% → 81%).
- **`static` functions get register arguments, and ours match.** `NuFileExtAdd` takes its
  string in `edi` and `AquireFileBuffer` its `FILEINFO *` in `edi`; writing them as plain
  `static` functions reproduced that without help.
- **Which parameters of a static get registers depends on their order.** `clipExtents`
  takes four of its six pointers in registers; declared `(mina, maxa, minc, maxc, min,
  max)`, MSVC put `maxa`…`min` in registers and `mina` on the stack, while the original
  has the four inputs in registers. Output-first, `(min, max, mina, maxa, minc, maxc)`,
  matched exactly. When a static's register set is off by one parameter, try another
  parameter order.
- **A static's register arguments only match once its callees are real statics too.**
  With a plain `cdecl` stub for `drawClippedPortal`, `clipPortal` allocated registers
  differently, and `clipExtents` (called from `clipPortal`) got another convention.
- **A folded empty function called with floats needs a prototype.** `NuRndrLine3dDbg`
  is ICF-folded into the unprototyped `NuEmptyFolded`; calling it directly would promote
  the floats to `double`. `folded.h` defines it as a call through a cast function
  pointer, which MSVC still emits as a direct `call`.
- **A global re-read after its own store is `volatile`.** `NuThreadCreateCriticalSection`
  compares the counter in memory, loads it, increments, stores, and loads it again for each
  use; a plain `static int` keeps it in `eax` (35% → identical).
- `__FILE__`/`__LINE__` call sites: pass the original line explicitly (`NuMemAllocAt`);
  `#line` breaks reccmp's line mapping.
- **A big loop body can be a forced-inline helper, and the stack layout shows it.**
  `DisplayListLinkDynamicMtls` written as one function got the right instructions but the
  wrong spill slots and registers (79%). The PS2 prototype lists a second block of locals
  that repeats `dlm` and `dlist`, a sign of inlined helpers. Splitting each loop body into a
  per-material `static __forceinline` helper with its own `dlm`/`dlist` locals fixed the slots
  (94%); a plain `__inline` helper is too big to be inlined. Smaller pieces followed the same
  way: the sort-list unlink as an `__inline` helper taking the `NUSORTPRI *` (80% → 91%), and
  `DisplayListAddSortPri` marked `__inline`, which LinkDynamicMtls inlines while
  `NuDisplaySceneAdd` still calls it.
- **Storing a constant can reuse a register known to hold it.** `DisplayListAddSortPri`'s
  `if (sort_list == NULL) { sort_list = pri; pri->sys_next = p; }` stored `p`'s copy
  of the list head; the original stores the tested register itself, and its inlined copy
  stores a zeroed `ebx`. That came from `pri->sys_next = NULL` (97% → exact, and the
  inlined copy matched as well).
- **Load an argument into its variable first when the original loads it early.**
  `NuDisplayListCreateMtlDlist(mtl->dlist_mtl->mtl_item, …)` loaded `mtl_item` just before
  the push. The original loads it ahead of the buffer-offset arithmetic, which came from
  `item = mtl->dlist_mtl->mtl_item;` on its own line before the call (effective).

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
- **Uninitialised globals are commons, placed in the linker's own order; `= 0` keeps a
  global in its object's `.bss`, in source order.** The original's far `.bss` range
  (`0x36xxxxx`) holds the commons, the per-object `.bss` (`0x96xxxx`) the explicitly
  initialised ones; the PS2's `.data` placement of a zero global is a hint that it had an
  initialiser. It matters when code reads one global through another's address:
  `NuQFntPrintEx`'s inlined pop reads `NuQFntModeStack - 4`, which the original labels
  `NuQFntCSModeStack[15]`; with the `qfnt` scalars as commons ours put another global in
  between (98.85%), with `= 0` on the scalars the two arrays became neighbours (100%).
- A reference to `global - offset` (e.g. `NuTrigTable - 4` from a loop counting from 1)
  isn't mapped. The code is identical, but it shows as ~92% (`NuTrigInit`) or ~97%
  (`NuPow`, which writes its ring buffers through `pow_x - 4` etc.), ~95%
  (`NuFileOpenSize`'s `file_info[fh - 1]`) and ~90% (the PC critical sections,
  `criticalsections[id - 1]`). Candidate for an upstream reccmp improvement; meanwhile
  `tools/offsetmatch.py` proves these identical (same offset from the same global in
  both builds), and progress/hooks count them as effective.

## Open questions

- **Which parameters a static gets in registers, with one caller.**
  `DisplayListPrintItems` (63%) takes `item` in `ecx` and `flags` in `eax` in the
  original; ours puts `item` and `fh` in registers (the three end up in the same
  callee-saved registers, with `flags` and `fh` swapped). Parameter orders, a `follow`
  local, loop shapes and the permuter changed nothing; its caller
  `NuDisplayListDrawRenderScene` (69%) only differs because of it.

- **Base or index first in `[reg + reg]`.** In `DisplayListLinkDynamicMtls` (effective)
  the original writes some `dlist->sort_pris[sp]` addresses with the base register first
  (`[edx + ecx]`) where ours puts the `sp * 36` register first: every access on the add
  path, and the check and unlink after a sort priority's `nmtls--`. The instructions are
  otherwise the same. No luck so far with a `pri` local, `(sort_pris + sp)`,
  `sp[sort_pris]`, `dlm->dyn_mtl_dlist.sort_pris`, moving the `sp` declaration, a
  `(dlist, sp)` helper or the global written directly (the last two were much worse).
  The sum operand-order rule above (the leaf referenced first goes first) may apply.

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
