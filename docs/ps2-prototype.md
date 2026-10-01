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
| `strings` | a literal referenced by exactly one function on each side | 307 | 3.6% |
| `calls` | callees of a paired function, aligned by call order between already-paired callees | 645 | 5% |
| `order` | equal-length unpaired runs between two pairs of one object | 101 | 0% |
| `features` | mutual-best, rarity-weighted overlap of strings, paired callees, paired callers and paired globals | 806 | 6% |

\* share of pairs whose PC neighbors both come from other objects, a rough upper bound on
the error rate. Known weak spot: near-identical sibling files (the Rahkshi creature
variants `Creature_Lehrak`/`Vorahk`/`Panrahk`) get confused.

Globals pair when a PC data address and a PS2 data symbol keep co-occurring across
paired functions (≥2 times, ≥60% of each one's uses, unambiguous). PS2 refs include
`$gp`-relative small data.

Propagated pairs have to pass sanity checks (size ratio, caller count, string sets,
file-name hint). After each round, a cluster filter bans any pair sitting between two
functions of another object, and propagation reruns. Per-file static helpers that recur
under one name on PS2 (`NuError`, `NuDebugMsg`) are never propagated.

Current coverage: 1,877 of 5,469 PC functions (34%). Treat anything but `strings` as a
strong hint until reccmp confirms a match. The `--check` flag compares against
annotated `src/` (currently 5/5 correct where paired).

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
- Compiled with GCC 2.95 (ee-gcc), so codegen tells us nothing about MSVC. Names,
  signatures, file grouping and order transfer.
