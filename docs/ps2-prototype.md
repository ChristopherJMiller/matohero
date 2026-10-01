# PS2 prototype (Sep 28 2006)

A PS2 build of BIONICLE Heroes from four weeks before the PC exe (Oct 26 2006),
preserved by Hidden Palace / BioMedia Project. It shipped with its debug ELF and linker
map, which makes it our main source of original names, source layout and types.

`tools/extract_orig.py <prototype .rar or .iso>` puts these in `orig/ps2proto/`:

| File | What it is |
|------|------------|
| `BION~213.ELF` | unstripped EE (MIPS) executable: `.symtab` with 11,851 functions and ~5,100 data symbols; `.mdebug` (16 MB of ECOFF debug info: types, locals, line numbers) |
| `BION~213.MAP` | ProDG linker map: every global symbol under the object file it came from |
| `SLES_541.50` | the stripped boot executable |

`tools/ps2map.py` merges the map and ELF into `orig/ps2proto/symbols.csv`
(`address,size,type,bind,section,object,name,signature,mangled`). Rows are in address
order, so each object's functions appear in their original source order. C++ entries
carry the demangled signature from the map, e.g.
`charbaseRelocateCharDef(chardef_s *, variptr_u *, variptr_u)`.

## Types, parameters and locals

`tools/ps2dump` runs ccc's `stdump` (packaged in `nix/ccc.nix`) over the `.mdebug`
section and writes C++ into `orig/ps2proto/stdump/`:

- `types.h`: about 27,800 lines of structs, unions, enums and typedefs with field offsets,
  e.g. `struct nulsthdr_s { /* 0x10 */ short int elcnt; … }`
- `functions.cpp`: every function with parameter names and types and its locals
- `globals.cpp`: globals with addresses and types

These are the original TT names and types, so they're the naming authority for this
project. They differ from the names other decomps invented (saga calls `elcnt`
`element_count`). Types matter for matching too: `elcnt` and friends are signed `short`,
which is why the PC code uses `movsx`.

## Naming PC functions

`tools/ps2names.py` pairs PC functions with PS2 ones and writes
`orig/ps2proto/pc_names.csv` (`pc_address,pc_size,name,object,signature,method,evidence`).
It needs `ghidra/export/functions.csv` from `ExportOverview.java`. Methods, by
reliability:

| Method | How | Pairs | Isolated* |
|--------|-----|------:|----------:|
| `strings` | a literal referenced by exactly one function on each side | 306 | 3% |
| `calls` | callees of a paired function, aligned by call order between already-paired callees | 724 | ~3% |
| `order` | equal-length unpaired runs between two pairs of one object | 91 | 0% |
| `features` | mutual-best, rarity-weighted overlap of strings, paired callees, paired callers and paired globals | 742 | ~2% |

\* share of pairs whose PC neighbors both come from other objects, a rough upper bound on
the error rate. Library code never takes part: the PC's statically linked MSVC CRT
(from `0x5b4a34`) and the PS2's Sony SDK and GCC runtime objects.

Globals pair when a PC data address and a PS2 data symbol keep co-occurring across
paired functions (≥2 times, ≥60% of each one's uses, unambiguous). PS2 refs include
`$gp`-relative small data.

Propagated pairs have to pass sanity checks (size ratio, caller count, string sets,
file-name hint). Per-file static helpers that recur under one name on PS2 (`NuError`,
`NuDebugMsg`) are never propagated.

### Object regions

MSVC emits each object's functions contiguously. For every PS2 object, its *core* is
the maximum-sum run over the address-sorted pairs, scoring +1 for its own names and −2
for others'. After each round, propagated pairs outside their object's core are banned
and propagation reruns. Every PC function then gets a region in
`orig/ps2proto/pc_objects.csv`:

| Status | Meaning |
|--------|---------|
| `named` | paired with a PS2 function |
| `inside` | unpaired, but within one object's core, so very likely from that file |
| `gap` | between two cores (`before|after`); often PC-only code (`pc\…`) or a file with no anchors yet |

Current coverage (game + engine, 4,843 functions): **1,863 named (38%)**, 1,339 more
placed inside a core, so **3,202 (66%) have a known source file**; 163 objects
located. `--check` compares against annotated `src/` (5/5 correct where paired).

## Source tree

Objects live under `h:\Bionicle\Ps2\Source\`, in three projects:

| Project | Functions | What |
|---------|----------:|------|
| `nu2api.bionicleps2/` | ~2,700 | the Nu2 engine: `nucore`, `numath`, `nu3d`, `nusound3`, `nufmv`, `gamelib` (incl. `listman.o`), plus PS2-only `nups2`, `*/ps2` |
| `gameapi.bionicleps2/` | ~4,000 | TT's shared game framework: `charsys`, `ai/aisys`, `gui`, `edlevel`, `edtools`, `DynoSDK` (physics), `effects`, `vfx`, `rtl` |
| `Bionicle.ps2/` | ~3,500 | the game itself (53 objects) |

The rest is the Sony SDK and GCC 2.95.3 runtime. The PC build mirrors this split: PC
assert strings such as `.\charsys\charbase.cpp` and `.\listman.c` line up with
`gameapi…/charsys/charbase.o` and `nu2api…/gamelib/listman.o`.

## Differences to expect from PC

- PS2-only code (`nups2`, `*/ps2/`, VU microcode in `.vutext`) has PC counterparts under
  `pc\` instead (`.\pc\nurndr.c`, `.\d3dCalls.cpp`).
- The PS2 build keeps functions the PC linker drops as unreferenced (`NuLstAllocBefore`,
  `NuLstGetPrev`, … exist here but not on PC).
- VU0 routines (`numath/ps2/numtxvu0.o`, `nuvecvu0.o`) become plain C on PC: most are
  tail calls to the generic function (`NuMtxMulVU0` → `NuMtxMulH`,
  `NuVec4ScaleXYZVU0` → `NuVec4Scale`), and some `…VU0` names are macros for the generic
  function (`NuVecMtxTransformVU0` call sites call `NuVecMtxTransform`). PC moved
  `NuVecMtxTransform` out of `nuvec.c` into that PC file. Callers that PS2 inlined
  (VU0 asm) show up on PC as calls, so cross-referencing callers needs both names.
- Compiled with GCC 2.95 (ee-gcc), so codegen tells us nothing about MSVC. Names,
  signatures, file grouping and order transfer.
