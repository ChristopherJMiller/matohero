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
