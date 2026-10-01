# Roadmap

**End goal:** build the game from source for other platforms, starting with Linux via GCC.

The path: a matching decompilation against the shipped PC exe (MSVC 7.1 SP1), testable
in-game early through a hook DLL, then a second compiler (MinGW-GCC on Windows), then a
portable platform layer for Linux.

## Milestones

| | Milestone | Done when |
|---|---|---|
| M0 | Toolchain and harness | build + reccmp loop works ✅ |
| M1 | Groundwork | PS2 symbols mapped onto the PC exe, names in Ghidra, CRT excluded from scoring, progress page, hook DLL running decompiled code in-game |
| M2 | Nu2 engine core | `nu2api` modules (nucore, numath, nufile, …) matched |
| M3 | Game code | game-side objects matched (charsys, gui, gamelib, …) |
| M4 | Standalone exe | the recompiled exe runs the game without the original |
| M5 | Second compiler | same source builds with MinGW-GCC for Windows and runs |
| M6 | Linux | platform layer (D3D9, DirectInput, DirectSound, Bink, Win32) replaced; GCC build runs natively |

## Working rules (each loop iteration)

1. Read this file and `docs/progress.md`; take the first unchecked task in the queue
   (or the next file in the current milestone). Keep it to one focused unit of work.
2. Build and compare before committing: `tools/build.py && tools/reccmp reccmp --target BIONICLE`.
   Nothing that matched 100% may regress.
3. Commit locally with a descriptive message. Never push, never rewrite pushed history.
4. Update the queue below (check off, add follow-ups), run `tools/progress.py` to
   refresh `docs/progress.md`, and update docs for anything learned.
5. If blocked on something only Chris can provide or decide, add it to **Needs from
   Chris** and move on to another task.
6. Ask before: large downloads, deleting anything outside `build/`/scratch, license
   changes, publishing anything.

## Queue

### M1: Groundwork

- [x] **PS2 prototype ingestion.** `tools/extract_orig.py` pulls the ELF/MAP from the
      prototype `.rar`; `tools/ps2map.py` writes `orig/ps2proto/symbols.csv` (11,851
      functions in 560 objects, with C++ signatures). See `docs/ps2-prototype.md`.
- [x] **PS2 types.** ccc v2.1 packaged (`nix/ccc.nix`); `tools/ps2dump` writes types,
      functions (params/locals) and globals to `orig/ps2proto/stdump/`. `listman.c`
      renamed to the original identifiers, still 10/10.
- [x] **Name the PC exe from PS2 symbols.** `tools/ps2names.py` v1: 638/5,469 paired
      (309 by unique strings, the rest propagated through calls/object order, with
      sanity checks and a cluster filter).
- [x] **More naming coverage.** Global pairing + feature matching (strings, callees,
      callers, globals): 1,859/5,469 named (34%).
- [x] **Object regions.** `orig/ps2proto/pc_objects.csv`: 3,202/4,843 game+engine
      functions placed in an original source file; core-run filter cut propagated-pair
      isolation to ~2–3%.
- [x] **Names into Ghidra.** `tools/ghidra names` applies the PS2 mapping (namespaces,
      plate comments); `tools/reccmp ghidra-import` applies matched functions' names and
      signatures (struct members come through empty, and unions aren't supported).
- [x] **Library code out of the score.** `tools/progress.py` counts only code below
      the CRT (`0x5b4a34`); `WinMainCRTStartup` is in reccmp's report ignore list.
- [x] **Progress page.** `tools/progress.py` regenerates `docs/progress.md`: functions
      and bytes matched, per original module, with a dated history.
- [x] **Run the original game under Wine.** `tools/rungame` (setup redoes the
      installer's Ogg2Wav music conversion; headless mode uses Xvfb + a Wine virtual
      desktop). Reaches the title screen in ~80 s with software rendering. See
      `docs/running.md`.
- [x] **Hook DLL.** `tools/hook.py --install` builds `xinput1_1.dll` (matched objects +
      generated glue binding everything else to original addresses); the game loads it
      and runs our code. Smoke test passes: 10 functions redirected, title screen reached.
- [x] **Hook trace mode.** `tools/hook.py --trace`: per-hook call counters in
      `run/game/bhhook-trace.txt`. Boot to title: `NuLstGetNext` ran 35,780 times.
- [x] **Re-layout `src/`.** `src/nu2api/{gamelib,nucore,…}`, `src/gameapi`, `src/game`;
      `tools/build.py` compiles each file from its original project root (PC `__FILE__`
      strings show the engine was one project per module directory).

M1 polish (after the hook DLL works):

- [ ] **Name the PC-only files.** The big gaps (`0x5717b0–0x57c8d0`, `0x5809e0–0x594060`,
      …) sit where PS2 has `nups2`/`*/ps2` code: they're the `pc\` renderer/platform
      files (`.\pc\nurndr.c`, `.\d3dCalls.cpp`, `.\pcapi.cpp`). Map them from PC
      assert strings and saga's PC-side names.
- [ ] **PS2 types into Ghidra.** Turn `orig/ps2proto/stdump/types.h` into a Ghidra data
      type archive (or apply it through a script) so decompiler output gets real struct
      members.

### M2: Nu2 engine core

One entry per original object, PC address range from `orig/ps2proto/pc_objects.csv`.
Order: self-contained math first, then core services. "saga" = opensagadev/saga has the
file. Each object: decompile → match → hook-test (`tools/hook.py --trace --install`,
headless boot to title) → commit.

- [x] **Hook objects that define globals.** `tools/coffpatch.py` unbinds annotated
      globals, stubs and mismatching functions in hook-build copies of objects (statics
      renamed per object), so any object with a matched function can run in-game.
      `nurand` + `listman`: 19 hooks, title screen reached.
- [x] `numath/nurand.c` (saga): 9/9 exact. Needed float constants in reccmp
      (`tools/floatconsts.py`, since the exe has no relocations).
- [x] `numath/nuvec.c` (saga): 24 functions, 18 exact + 4 effective; `NuVecRotateX/Z`
      (91%) and `NuVecDist`/`XZDist` (98%, `pop ecx` vs `add esp,4` cleanup) to revisit.
      Lesson: MSVC canonicalizes the order of `+` terms, but each product's operand order
      shows (`m0->_01 * v0->x` vs `v0->x * m0->_01`). Hook test: 39 functions in-game,
      `NuVecNorm` 93,840 calls during boot.
- [x] **Permuter.** `tools/permute.py <function> [--apply]`: product operand order,
      comparison direction, independent declaration order; batches of 40 variants per
      `cl` run (~1 s/variant). Recovers the known InvMtxRotate fix. Future dimensions:
      safe statement reordering, temporaries, if/else vs ?:.
- [x] `numath/nutrig.c` part 1 (saga): 11 functions, 10 exact (`NuTrigInit` identical but
      ~92% from a reccmp limitation, see `docs/matching.md`); `NuPowFast` 93% (`pop ecx`).
- [x] `numath/nutrig.c` part 2: `NuLog10`, `NuExp10` exact; `NuPow` identical code at 97%
      (reccmp's `array - 4` limitation). nutrig complete: 14 functions, all but
      `NuPowFast` (`pop ecx` question) identical. Hook test: title reached, tables built
      in-game through the exe's allocator.
- [x] `numath/numtx.c` (saga): 35 functions, 21 exact + 4 effective, generated from
      saga's text in PC order. Near misses: `RotateX/Z`, `PreRotateX/Y` (87–95%, x87
      `fxch` placement), `Inv` (93%), `InvRSS` (85%), `Mul/MulH/MulR` (56–63%), `InvH`
      (46%), `AlignZ` (63%). Hook test: 73 functions in-game, `NuMtxTranslate` 69k calls.
- [x] **Permuter v2.** Statement swaps, plateau-crossing random search with restarts,
      per-line flips, a positional tie-break in the score, exhaustion detection and a
      `--time` limit; scratch variants live under `build/`. Solves the known nuvec case in
      ~20 variants but found nothing for the numtx near-misses (~300 variants each), so
      their differences lie outside these moves.
- [x] `numath/nuquat.c`: 4/4 (3 exact, 1 effective). `NuMtxToQuat` (Shoemake) and
      `NuQuatSlerp` (older than saga's) written from the asm. Found the original
      `NuFdiv` (tests both operands for zero), which also lifted `NuMtxAlignZ` 63% → 94.5%.
- [x] `numath/nufloat.c`, `numath/nuplane.c` (saga): 8 functions, 7 exact + 1 effective
      (`NuPlnLine3`, denominator term order). `NuFabs` is a sign-bit mask through memory,
      not `fabs` (lifted `NuMtxAlignZ` to 96%, `NuMtxInvH` to 53%). Only three nuplane
      functions sit here in the PC exe. Permuter gained `--try` (score hand-written
      bodies) and `cl` batches now go through a response file (Wine crashed on long
      command lines, which had silently failed big batches). Hook test: 85 functions,
      title reached, `NuFloor` 284k calls.
- [x] PC-only numath around nufloat: `numath/pc/nuvu0_pc.c` (`NuFsqrt`, six `…VU0`
      tail-call thunks, `NuVec4MtxRotateVU0`, `NuMtxPreScaleUVU0`, `NuVecMtxTransform`)
      and `numath/nuvec4.c` (`NuVec4Scale`, `NuVec4MtxTransform`, `NuVec4MtxTransformH`):
      13/13 exact. Named by matching PC callers to PS2 callers. **numath is complete:**
      every function in `0x56e0e0–0x5717a0` is placed (98 matched functions overall); the
      near-misses are listed above. Hook test: 98 functions, title reached, `NuFsqrt`
      67k calls.
- [x] `nucore/nustring.c` part 1 (saga): the ASCII/UTF-16 helpers, 28 functions, 27
      exact; `NuSPrintfW` 97% until its callees are in. Found `/GS` (stack cookies) in
      the original flags and the MSVC rule that defers functions calling later-defined
      ones to the end of the object (`docs/matching.md`). Hook test: 126 functions,
      title reached, `NuStrICmp` 2.4M calls.
- [x] `nucore/nustring.c` part 2: the UTF-8/Unicode conversions, string-table accessors,
      and the path helpers (`NuStrFixExt[Platform]`, `NuStrGetPath/Ext/FilenameNoExt`):
      18 functions, 17 exact + 1 effective; `NuSPrintfW` now exact too. Found that the
      original builds with **`/Ob1`** (only `__inline` functions inline), now global, with
      `__inline` on the helpers that get inlined. Hook test: 143 functions, title reached.
- [ ] `nucore/nustring.c` part 3: `NuStringTableLoadCSV` (`0x568ff0`, 1148 bytes) and
      `NuStringTableLoadTXT` (`0x569480`). They use `NUFPAR`/`NuFile`, so do them after
      `nufpar` and `nufile`.
- [x] `nucore/nulist.c`, `nutime.c`, `nuapi.c`, `nuptrblock.c` (saga) and
      `nucore/pc/nutime_pc.c` (QueryPerformanceCounter ticks): 16 functions, all exact,
      plus the `nuapi` global (`0x3607b60`, PS2 layout). Win32 imports compare by name,
      so platform code matches like anything else. Hook test: 159 functions, title
      reached, `NuTimeGetFrameTime` 373k calls.
- [ ] Rest of `0x56c3a0–0x56cf3e`: `NuFrameBegin`/`NuFrameEnd` (need nu3d/PC callees
      stubbed), `0x56c820` (ms timer, `NuTimeGetTime`?), `0x56c980` (sets
      `nuapi.video_swap_mode` and `native_FPS`), `0x56ca30–0x56ce90` (`nurdpf.c`, the float
      expression evaluator behind `NuFParGetFloatRDP`; `NuRDPF` = `0x56ce90`; the int
      one, `NuRDPI` = `0x56e0b0`, sits after rnc), `0x56cec0`/`0x56cf00`/`0x56cf20` (critical-section table at
      `0x34c5fb8`), `0x56c3a0`/`0x56c3d0` (tables at `0x34c57e4`, maybe nulang).
- [x] `nucore/nufpar.c` (saga): all 24 PC functions exact (BH has one command stack,
      saga two). Stubs for the nufile callees, `NuFileExists` (`pc/nufile_pc.c`) and the
      expression evaluators `NuRDPI`/`NuRDPF`. Hook test: 183 functions, title reached,
      the game's script files parsed by our code (`NuFParGetWord` 138k calls).
- [ ] `nucore/nufile.c` (saga), ~52
- [ ] `nucore/numem.c` (`NuMemAllocFn` stub today) and `nucore/pc/nuthread.c`

Tooling backlog (pick up when near-misses pile up):

- [ ] **Permuter v3 ideas.** Inline-vs-local subexpression rewrites (what moved
      `InvRSS` and `NuPlnLine3`), `x * -y` ↔ `-(x * y)`, temporaries for repeated
      loads. `--try` covers hand-written variants meanwhile.
- [ ] **Re-run permuter v2 on the numtx near-misses.** Before the response-file fix,
      Wine crashed on some long `cl` command lines and those batches scored as
      "doesn't compile", so the earlier ~300-variant runs may have searched less than
      reported.

After that: `nu3d` (renderer-independent parts first), `gamelib`, then `gameapi`.

## Needs from Chris

- **DirectX SDK (April 2006)**, the release that shipped `d3dx9_30.dll`. The game uses
  `ID3DXEffect`/`ID3DXEffectPool`/`ID3DXFont`, whose method layouts changed after the
  Dec 2002 SDK in `~/Downloads`. Needed once renderer code is reached; archive.org has it.

## Decisions

- 2026-09-30: GPL-3.0, so code can be ported from opensagadev/saga.
- 2026-09-30: hook-DLL approach for in-game testing before the decomp is complete.
- 2026-09-30: commits stay local (no remote yet).
- 2026-09-30: identifiers (functions, struct fields, params, locals) follow the PS2
  prototype's debug info; saga and others are code sources, not naming sources.
