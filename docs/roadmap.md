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
2. Build and compare before committing: `tools/build.py && tools/reccmp reccmp --target BIONICLE
   --json build/report.json`, then `tools/regress.py <previous report> build/report.json`.
   Nothing that matched 100% may regress, and no other function may drop either.
   After reverting a source experiment, rebuild before comparing or generating progress
   (a stale `build/` once showed 5 phantom regressions).
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
- [x] `nucore/nulist.c`, `nutime.c`, `nuapi.c`, `nuptrblock.c` (saga) and
      `nucore/pc/nutime_pc.c` (QueryPerformanceCounter ticks): 16 functions, all exact,
      plus the `nuapi` global (`0x3607b60`, PS2 layout). Win32 imports compare by name,
      so platform code matches like anything else. Hook test: 159 functions, title
      reached, `NuTimeGetFrameTime` 373k calls.
- [x] `nucore/nurdpf.c` (saga): the float expression evaluator behind
      `NuFParGetFloatRDP` (`get_tok`, `prim`, `term`, `expr`, `NuRDPFVar`, `NuRDPF`,
      `0x56ca30–0x56cebe`): 6/6 exact on the first build, emission order as in the exe.
      Declarations in `nurdp.h`. Hook test: 241 functions, title reached, `NuRDPFVar`
      343 calls from the exe.
- [x] `nucore/nurdpi.c` (saga): the int evaluator behind `NuFParGetIntRDP`
      (`ishexnum`, `hexcodetoint`, `get_tok`, `prim`, `term`, `expr`, `NuRDPI`,
      `0x56d810–0x56e0de`): 7/7 exact. `NuRDPIVar` isn't in the PC exe. `get_tok`'s last
      4% was `curr_num *= 16` for saga's `<<= 4` (`docs/matching.md`); `tools/permute.py`
      now also takes a marker address, for statics whose names repeat across files.
      Hook test: 244 functions, title reached, `NuFParGetIntRDP` 725 calls.
- [x] Small nucore leftovers: `NuTimeGetTime` (`0x56c820`, `pc/nutime_pc.c`; ms from the
      tick counter via `_alldiv`), `NuVideoSetSwapMode` (`0x56c980`, new `nuvideo.c`,
      saga), and the PC critical sections (`0x56cec0`/`0x56cf00`/`0x56cf20`,
      `pc/nuthread.c`): 2 exact, the 3 critical sections identical but held at 90–93% by
      reccmp's `global - offset` limit (the table is indexed `[id - 1]`). The empty-function
      fold at `0x5af030` is `NuEmptyFolded` in `folded.h`/`pc/folded_pc.c` with the
      memory-card one. Hook test: 246 functions, title reached.
- [x] `nucore/nupad.c` part 1 (saga, BH-specific `NUPAD`): `NuPadOpen`, `CalcRamp`,
      `NuPadSetStatus`, `NuPadMapPortToPS2Port`, `NuPadGetFirstActivePad` (PC version:
      player 0's port if valid), `NuPadRead` (with the PC's d-pad ramps): 6/6 exact.
      Globals placed (16 scanned pads on PC). The PC platform layer's frame-pointer empty
      function `0x585070` is `NuPCEmptyFolded` (`folded.h`). Hook test: 252 functions,
      title reached and **input works through our `NuPadRead`** (keys now held in
      `rungame`; instant taps were missed).
- [x] `nucore/nupad.c` part 2: `NuPadProcessReadData` (`0x56bd90`, deadzones and
      normalised sticks) and `NuPadUpdatePads` (`0x56c130`, scanning and pad mapping):
      both exact; nupad complete (8/8). `0x56c3f0` renamed `NuPadReadMapped`: the real
      `NuPadRead` is the PC layer's wrapper at `0x57a540`. PC-layer stubs `NuPadReadPS`,
      `NuPadGetMaxDevicesPS` (`pc/nupad_pc_stubs.c`); `NuMcFolded` → `NuPCZeroFolded`.
      Found that reccmp drops markers on multi-line signatures. Hook test: 254
      functions, input works end to end through our pad code (title → menu → save
      prompt).
- [x] `nucore/nufilepak.c` (saga; `CalcChecksum`, `LoadKey` with inlined decryption,
      `Load`, `Condense`, `GetItem`, `GetItemInfo`, `SetItemRequired`), `nucore/nulang.c`
      (`NuLanguageGet`/`Set`), `NuAPIInit` (`0x56c670`) and the PC file helpers
      `NuFileRename`/`NuFileCreateDir` (`0x56b760`/`0x56b7e0`, `pc/nufile_pc.c`): 12/12
      exact. `0x565bf0` is the engine's byte-wise memmove, stubbed as `NuMemMove`
      (`numem.c`, our name). Hook test: 266 functions, paks loaded through our code
      (`NuFilePakGetItem` 448 calls), input works.
- [x] `NuFrameBegin` (`0x56c6a0`) / `NuFrameEnd` (`0x56c6c0`): both exact on the first
      build. PC's `NuFrameEnd` also pauses sound and sleeps while the window is inactive,
      pumping messages, and checks for a lost D3D device. New stubs: nu3d callees in
      `nu3d/nu3d_stubs.c` (PS2 names), the PC layer in `pc/pcapi_stubs.c` (names ours:
      `PCFrameLock`/`Unlock`, `PCSoundPause`/`Resume`, `PCProcessMessages`,
      `PCCheckDevice`, `pc_app_active`). Hook test: 268 functions, the frame loop runs
      through our code (3.5k frames to the save prompt).
- [x] `nucore/nufpar.c` (saga): all 24 PC functions exact (BH has one command stack,
      saga two). Stubs for the nufile callees, `NuFileExists` (`pc/nufile_pc.c`) and the
      expression evaluators `NuRDPI`/`NuRDPF`. Hook test: 183 functions, title reached,
      the game's script files parsed by our code (`NuFParGetWord` 138k calls).
- [x] `nucore/nufile.c` part 1: devices and paths (`NuFileUpCase`, `ReldirFix`,
      `GetDeviceFromPath`, current/sys/dll directories, `FormatName`), memory files
      (`Open`/`Addr`/`Write`), `NuFileWrite*`, `NuDatFileSeek`/`Set`/`Get`, plus the PC
      layer `pc/nufile_pc.c` (`host_device`, `DEV_FormatName`, `DEVHOST_Interrogate`,
      `NuFileExists`, `NuPSFileOpen/Close/Read/Write/LSeek` on CRT stdio): 29 functions,
      28 exact (`NuPSFileOpen` 81%). Hook DLL now routes stdio to the exe's CRT
      (`docs/running.md`). Hook test: 211 functions, title reached, file reads through
      `NuPSFileRead`.
- [x] `nucore/nufile.c` part 2a: `NuDatFileFindTree`, the extension lookups
      (`NuFileExtFind` static, `ExtConvert`, `ExtGetExt`, `ExtRemove`), `NuFileInitEx`,
      `NuFileOpenSize`, `NuMemFileClose/Pos/Seek`, `NuFilePos`, `NuFileSeek`: 12
      functions, 9 exact + `OpenSize` identical (reccmp `global - offset` limit);
      `ExtFind` 95%, `NuFileSeek` 85%. Memory-card names alias one folded stub
      (`NuMcFolded`). Hook test: 220 functions, title reached.
- [x] `nucore/nufile.c` part 2b: `AquireFileBuffer`, `NuFileEndBlkRead`, `Align`,
      `NuDatGetFileInfo`, `NuDatFileRead`, `NuDatFileOpen`, `NuFileOpen`, `Close`,
      `NuMemFileRead`, `NuFileSize`, `NuFileRead` and the five `NuFileRead<Type>`: 17
      functions, 8 exact; near-misses (block layout / shrink-wrapping): `Align` 99%,
      `DatFileOpen` 97%, `DatFileRead` 85%, `Close` 87%, `Open` 83%, `EndBlkRead` 82%,
      `Read` 81%, `GetFileInfo` 61%. Hook test: 228 functions, title reached.
- [x] `nucore/nufile.c` part 2c: the extension table (`NuFileExtAdd`, register-argument
      static, and `NuFileExtInit` with its 56 registrations, 98.6%),
      `NuDatFileLoadBufferLSN` (79%; a dead path on PC), `NuFileBeginBlkRead`,
      `NuDatOpen`, `NuFileLoad`, `NuFileLoadBuffer` (83%), `NuFileLoadBufferVP`: 9
      functions, 5 exact. `rnc.c` is `NuPPUnpack` `0x56d320` + `NuPPLoadBuffer` `0x56d6b0`
      (stubs). Hook test: 233 functions, title reached.
- [x] `nucore/nufile.c` part 2d: `NuDatOpenEx` (98.4%). `NuDatFileLoadBuffer` is
      written but lives in `nufile_pending.c` (27% there): in nufile.c it changes the
      compiler's inlining of `NuDatFileSeek` (see `docs/matching.md`). Hook test: 233
      functions, title reached.
- [ ] nufile near-miss pass (`GetFileInfo` 61%, `Read` 81%, `EndBlkRead` 82%, `Open`
      83%, `LoadBuffer` 83%, `Close` 87%, `DatFileRead` 85%, `FileSeek` 85%, `ExtFind`
      95%, `PSFileOpen` 81%, `DatFileLoadBuffer` 35%). Unblocked: the compile-order
      question is answered (see the tooling backlog). First round (no change, all
      reverted): `ExtFind` loop shapes (`goto next`, `for(;;)` with early return,
      combined condition, `do`/`while`): the original threads the inner loop's exit
      straight to `return si`, ours re-tests `*fext`; `ExtInit` statement order of the
      two zero stores (the original stores `ext_init_needed` as an immediate while `eax`
      is 0); `DatFileOpen` splitting `start = pos = …` (worse; one remaining diff is reccmp
      naming the end-of-`datfiles` address after different neighbours); `DatOpenEx` keeps
      0 in `ebx` around the LSN code, so that part probably has more source than ours.
- [x] `nucore/nustring.c` part 3: `NuStringTableLoadCSV` (`0x568ff0`, 1148 bytes) and
      `NuStringTableLoadTXT` (`0x569480`): both exact. `LoadCSV` inlines a copy of
      `NuAsciiToUnicode` that `NuSPrintfW` calls, so it uses a `static __inline` copy
      (`docs/matching.md`). Hook test: 235 functions, title reached, the game's CSV
      string tables loaded by our code.
- [x] `nucore/numem.c`: `NuMemSetExternal`, `NuMemAllocFn` (external buffer,
      discardable buffer, heap or `malloc`, with debug head/tail marks) and
      `NuMemFreeFn`: 3/3 exact, globals placed (`0x34b72f8–0x34b731c`, marks at
      `0x6171f8`). Heap calls stubbed (`nuheap_stubs.c`: `NuHeapFree` `0x56d210`,
      `NuHeapAllocAligned` `0x56d300`). The hook DLL now sends our `malloc`/`free` to the
      exe's CRT per object (DLL-wide crashed it, `docs/running.md`). Hook test: 271
      functions, every engine allocation through our code, input works.
- [x] `nucore/nuheap.c` (saga): `RemoveFromFreeList` (`0x56d080`), `SplitFreeBlock`
      (`0x56d0a0`, register-argument static), `NuHeapAllocAlignedNamed`, `NuHeapFree`,
      `NuHeapAllocAligned`: 2 exact; near-misses `Split` 63%, `AllocAlignedNamed` 66%
      (`eax`/`ecx` roles swapped throughout; loop shape, declaration order, parameter
      order and macro-vs-inline accessors don't change it), `Free` 80% (the original
      reloads `block->size` around `SetSize`). No `NuHeapCreate` on PC, so the engine
      heap is unused there. Hook test: 273 functions, input works.
- [x] `nucore/numemblk.c` (`NuMemBlkCreateEx`, `Size`, `Alloc`, `Free`; 3 exact,
      `CreateEx` 65%: the original decrements `cnt` in place for its chain loop) and the PC
      background loader `pc/bgproc_pc.c` (`bgPostRequest`, `bgThreadMain`, `bgProcInit`,
      `bgProcAbortAll`, `NuFrameEndBgLoadPS`; Win32 thread + event + critical section; 4
      exact, `AbortAll` 83%, `edi` saved at entry in the original). Found that the active
      request pointer is `volatile BGPROCINFO *` (double field loads). Added
      `tools/regress.py`, which also flags drops below 100% (the old check only compared
      exact sets). Hook test: 280 functions, our loader thread runs, input works.
- [x] The PC's C versions of PS2's `asmmem.s`, in `numem.c`: `NuMemMove` (`0x565bf0`,
      byte-wise memmove, our name; callers pass byte counts, so not `NuMemCopy128`) and
      `NuMemSet128` (`0x565c40`): 2/2 exact. Hook test: 282 functions, `NuMemMove` 31k
      calls, input works.
- [ ] Leftovers: `0x565930` (index lookup in 0x50-byte records, called from gamelib's PC
      nuobject; maybe glutils), `0x56b490` (initializes a critical section at
      `0x34c56f8` just before `fhandles`; its flag at `0x34c5790` lies inside
      `fhandles[64]`, though `NuPSFileOpen` scans 64 entries).

Tooling backlog (pick up when near-misses pile up):

- [x] **Model MSVC 7.1's compile order and inlining.** Solved for nufile: the
      "deferral" was the out-of-line copy of an `__inline` function in a recursive inline
      cycle being emitted at its first out-of-line call, which moves where MSVC cuts the
      cycle (`docs/matching.md`). `NuDatFileLoadBuffer` (35%, from 27%) now lives in
      `nufile.c` calling `NuFileSeek` as the original does; `nufile_pending.c` is gone.

- [x] **Count `global - offset` matches.** `tools/offsetmatch.py` checks every
      function below 100%: each differing instruction pair must have the same opcode and
      length and differ only in one 4-byte field whose two values are the same offset
      from the same annotated global (reccmp's variable matches, within 4 KB). The 6 such
      functions (`NuTrigInit`, `NuPow`, `NuFileOpenSize`, the 3 critical sections) now
      count as effective in `progress.py` and are hooked by `hook.py` (critical sections
      621k calls, `NuPow` 90k in the boot test, input works).
- [ ] **Permuter v3 ideas.** Inline-vs-local subexpression rewrites (what moved
      `InvRSS` and `NuPlnLine3`), `x * -y` ↔ `-(x * y)`, temporaries for repeated
      loads. `--try` covers hand-written variants meanwhile.
- [ ] **Re-run permuter v2 on the numtx near-misses.** Before the response-file fix,
      Wine crashed on some long `cl` command lines and those batches scored as
      "doesn't compile", so the earlier ~300-variant runs may have searched less than
      reported.

After that: `nu3d` (renderer-independent parts first), `gamelib`, then `gameapi`.

### nu3d

- [x] `nu3d/nuspecial.c` part 1: the accessors (`GetPos`, `GetDrawPos`, `GetMtx`,
      `SetMtx`, `GetName`, `ExistsFn`, `MtlMap`, `ForceMtl`, `GetVisibilityFn`,
      `GetDrawMtx`, `GetInstAnim`, `Compare`): 11 exact, `MtlMap` 67% (the original
      splits `nuspecial_draw_state |= 4` into load/`or`/store around another store;
      branch orders and statement orders tried). First nu3d types in `nu3d/nugscn.h`
      (PS2 layouts: `NUHSPECIAL`, `NUSPECIAL`, `NUSPECIALOBJECT`, `NUINSTANCE`,
      `NUINSTANIM`); `nurndr_gen` globals stubbed in `nu3d_stubs.c`. Hook test: 299
      functions, `NuSpecialGetDrawMtx` 63k calls, input works.
- [x] `nu3d/nuspecial.c` part 2, the finders: `NuSpecialFind`, `FindMulti`,
      `FindMultiWC` and their display-list helpers (statics, names ours): 4 exact
      (`Find` and 3 helpers), `FindMulti` 81% and `FindMultiWC` 77% (the original gives
      the loop counter `n` a register and keeps `cnt`/`sob` on the stack; ours swaps
      them; declaration order, init order and loop shapes don't change it). The PC
      `NUGSCN` has a display list at `0x1b4` (PS2: `0x1ac`) with its own special objects;
      `nugscn.h` places the fields used. `permute.py` now parses signatures with nested
      parentheses (`char (*WCName)[20]`). Hook test: 303 functions, `NuSpecialFind` 641
      calls, input works.
- [x] `nu3d/nuspecial.c` part 3: `DrawAt`, `SetCollision`, `GetCollision`,
      `GetInstanceix`, `NumMtls`, `GetMtl`, `GetBounds`, `GetRadius`, `SetDrawMtx`,
      `SetDrawPos`: 10/10 exact on the first build; `SetVisibility` 89% (the original
      tests `instance` and reloads it, yet keeps `edi` saved at entry as with a local).
      nuspecial is complete on PC: 31 functions, 27 exact. `nugscn.h` now has `NUGOBJ`,
      `NUGEOM`, `NUCLIPOBJECT` and the PS2 `NUDISPLAYSCENE`; `NuRndrGScnObj`,
      `NuDisplayListUpdateSpecial`, `NuDisplayListRndrSpecial` stubbed. Hook test: 313
      functions, `NuSpecialDrawAt` 104k calls, input works. (`0x53e350`, before
      `NuSpecialFind`, builds a perspective matrix: nurndr, not nuspecial.)
- [x] `nu3d/nucamera_gen.c` part 1: `Create`, `Destroy`, `SetVPortClipMtx`,
      `SetScissorClipMtx`, `Get`, the getters, `GetClipMtx`, `RayCast`,
      `TransformScreen(Clip)`, `ClipTestSphere`, `DistSqr`, `Unlock`, `Relock`, plus
      `NuVpGetCurrentViewport` (`nu3d/nuvport_gen.c`): 20/20 exact. `NuCameraGetMtx` is
      folded into `NuCameraGetCam` (a macro now); `NuCameraSet` moved to the renderer on
      PC (`0x53d640`, stubbed). Camera data follows the PS2 order (`vmtx` `0x6166d0` …
      `clip_planes` `0x616950`); zero-initialised data went to `.bss` (`prev_lock`
      `0x96ce18`, `num_cameras_allocated` `0x96ce24`). New `numath/nuvu0.h` declares the
      VU0 replacements. Hook test: 333 functions, `GetViewMtx` 500k calls, input works.
- [x] `nu3d/nucamera_gen.c` part 2: `SetEx` exact (98.8% until `zx = zy / aspect` moved
      up next to `zy`), `BuildClipPlanes` 63% (see below), plus `NuVpGetClippingMtx`/
      `NuVpGetScalingMtx` and the matrix copy at `0x414140` (a `static` header function,
      `NuMtxCopy` in `numtx.h`, ICF-folded with ~100 game call sites): 4 new exact.
      `FaceYDirStream` folds into the empty stub and `NuRndrSetViewMtx` into a new
      `return 0` group (`NuZeroFolded`, `0x4b44a0`); `NuVpUpdate`, `NuRndrSetSpecularLight`,
      `NuRndrStateUpdateCameraState`, `NuCameraSetProjectionMtx` stubbed. The projection
      tweak globals are `nuvport_gen`'s `vport_hrdump_*` (hi-res dump tiling). Hook test:
      337 functions, `SetEx` 21k calls, `NuMtxCopy` 106k, input works.
- [ ] `NuCameraBuildClipPlanes` (63%): the plane-building half matches apart from where
      the first `NuMtxMulH`'s pushes are scheduled; the near/far tail differs because the
      original gives `fDot` a stack home (`fst [esp+8]`, in `NearDistSqr`'s slot) and keeps
      `NearPlane.w` and `-_20` on the x87 stack, while ours keeps `fDot` there. Tried:
      declaration orders, reusing a live-across-calls local, the dot written out twice, an
      `__inline` dot helper, chained/duplicated copies; `fDot = _20*_30 + (_21*_31 +
      _22*_32)` gets the original's add grouping but scores lower.
- [ ] `nu3d/nucamera_gen.c` part 3: `0x538cf0` (2451 bytes, called from `0x597f50`:
      `ClipTestExtentsGeneric`?), `0x5399b0` (726 bytes, from `0x4e5cd0`). After it, the
      rest of `nuvport_gen` (`0x53a140`–`0x53a4d0`, `NuVpUpdate` `0x53a6a0`) and the PC
      `NuCameraSet` (`0x53d640`).
- [ ] Other nu3d candidates with saga sources: `nuspline` (3 PC functions),
      `nugscn_gen`, `nuportal_gen` (12), `nuqfnt` (30), `nudlist` (14).

Notes for later units:

- `gameapi/gamelib/edbits.c` on PC (`0x51bb40–0x51c5de`, 11 functions):
  `edbitsRegisterSaveFormat` (`0x51bb40`, six `NuStrCpy`s into the save path globals at
  `0x6c82f0`+`0x100*i`), three 10-byte setters (`0x51bc00` `RegisterBaseScene`, then
  `0x6c88f4`/`0x6c88f8`), `DrawCube`? (`0x51bc30`), `0x51c0d0`, `DrawOvalTilted`
  (`0x51c1d0`: 10 segments, `NuVecRotateZ`/`Y`, lines through the empty folded
  `0x5af030`, so editor debug drawing is a no-op on PC), `DrawCircleTilted` (`0x51c340`,
  calls it with rx = rz), `DrawCross`? (`0x51c370`), `DrawBBox`? (`0x51c4d0`, 12 lines),
  `edbitsLookupInstance` (`0x51c570`: `NuGScnNumSpecials`/`GetSpecial`,
  `NuSpecialGetName`, `NuStrNICmp`).

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
