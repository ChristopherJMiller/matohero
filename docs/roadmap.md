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
4. Update the queue below (check off, add follow-ups), `docs/progress.md`, and docs for
   anything learned.
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
- [ ] **More naming coverage.** Pair globals (PS2 `.data`/`.bss` symbols ↔ PC data
      refs) and use them as anchors; add caller-side propagation; assign unpaired PC
      functions to objects by region (MSVC keeps objects contiguous) to get file
      boundaries even where names are unknown.
- [x] **Names into Ghidra.** `tools/ghidra names` applies the PS2 mapping (namespaces,
      plate comments); `tools/reccmp ghidra-import` applies matched functions' names and
      signatures (struct members come through empty, and unions aren't supported).
- [ ] **PS2 types into Ghidra.** Turn `orig/ps2proto/stdump/types.h` into a Ghidra data
      type archive (or apply it through a script) so decompiler output gets real struct
      members.
- [ ] **Library code out of the score**: mark statically linked CRT/third-party code
      (`// LIBRARY:` or report ignores) so accuracy reflects game code.
- [ ] **Progress page**: `tools/progress.py` → `docs/progress.md` (functions and bytes
      matched, per object/module).
- [ ] **Run the original game under Wine** (D3D9 via WineD3D/DXVK); document how.
- [ ] **Hook DLL**: build `src/` into a DLL; a loader patches each 100%-matched
      function in the original exe with a jump into the DLL, so decompiled code runs
      in-game. Smoke test: boots to the title screen.
- [ ] **Re-layout `src/`** to mirror the original tree from the PS2 map
      (`nu2api/gamelib/listman.c`, …), keeping PC-only files (`pc\…`) in place.

### M2 onward

Filled in as M1 lands: one queue entry per original object file, ordered by
dependency (core first) and saga coverage.

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
