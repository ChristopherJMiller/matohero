# BIONICLE Heroes decomp

A work-in-progress matching decompilation of **BIONICLE Heroes** (PC, 2006, Traveller's
Tales / Eidos). The goal is C/C++ source that compiles with the original toolchain
(MSVC 7.1 / Visual Studio .NET 2003 SP1) into code identical to the shipped
executable, function by function. Later goals: build with modern compilers (GCC/Clang)
and make it practical to modify or replace engine modules.

This repository contains **no game code or assets**. You need your own copy of the
game.

Licensed under GPL-3.0. Parts of the Nu2 engine code are ported from
[opensagadev/saga](https://github.com/opensagadev/saga) (GPL-3.0), a matching decomp of
LEGO Star Wars: The Complete Saga, which shares the engine.

## Setup

Requires Nix with flakes. `nix develop` (or `direnv allow`) provides Ghidra, rizin,
ImHex, binutils/LLVM tools, Python (pefile, capstone, lief), unpackers and Wine.

Populate `orig/` (gitignored) from your disc image:

```sh
tools/extract_orig.py path/to/retail.zip-or-.bin-or-.iso [path/to/nodrm.iso]
```

This produces:

- `orig/Bionicle.exe`: the reference binary (decrypted; see
  [docs/binary-notes.md](docs/binary-notes.md) for why this one)
- `orig/retail/`: the untouched SecuROM executables
- `orig/game/`: the installed game (data, movies, DLLs), runnable with the reference exe

The script verifies the reference binary by SHA-1.

### Compiler

The game was built with MSVC 7.1 SP1 (`cl` 13.10.6030). Download `VisualStudio2003.zip`
and `VisualStudio2003-sp1.zip` from
[archive.org/details/visual-studio-2003-unpacked](https://archive.org/details/visual-studio-2003-unpacked),
then:

```sh
tools/setup_msvc.py path/to/VisualStudio2003.zip path/to/VisualStudio2003-sp1.zip
tools/msvc cl /nologo /c foo.c     # any MSVC tool, run under Wine
```

This assembles a portable toolchain in `toolchain/msvc71` (gitignored): the RTM
compiler, CRT and Platform SDK, with SP1's replacement files overlaid from the
patch's embedded cabinet, so no installer runs. `tools/msvc` uses a private Wine prefix
in `toolchain/wineprefix`.

## Building and comparing

```sh
tools/build.py                           # compile src/ under Wine, link build/Bionicle.exe + PDB
tools/reccmp project detect --what recompiled --search-path build   # once
tools/reccmp reccmp --target BIONICLE    # per-function similarity report
tools/reccmp reccmp --target BIONICLE --verbose 0x565550            # diff one function
```

Source lives in `src/<library>/`, mirroring the original tree (`src/nu2/listman.c` was
`.\listman.c` in the engine project). Annotate each decompiled function with its
original address:

```c
// FUNCTION: BIONICLE 0x565840
NULNKHDR *NuLstGetByIdx(NULSTHDR *list, i32 index) {
```

Use the original names and types from the PS2 prototype (`orig/ps2proto/stdump/`,
see [docs/ps2-prototype.md](docs/ps2-prototype.md)).

Globals use `// GLOBAL:`, string literals `// STRING:`, and placeholders for
not-yet-decompiled callees `// STUB:`. See the
[reccmp annotation docs](https://github.com/isledecomp/reccmp/blob/master/docs/annotations.md).

Gotchas:

- Put notes *above* the marker. A comment between `// FUNCTION:` and the definition makes
  reccmp silently skip the function.
- Don't use `#line` to reproduce an original `__LINE__`: it shifts every later line and
  reccmp pairs functions with the wrong addresses. Pass the number explicitly (see
  `NuMemAllocAt`).
- Code that touches an undefined global or function compares as a mismatch. Define the
  global with `// GLOBAL:` or add a `// STUB:` so reccmp can map the address.

## Ghidra

```sh
tools/ghidra import                 # create ghidra/BionicleHeroes and auto-analyze (~90 s)
tools/ghidra export                 # function list + source hints -> ghidra/export/
tools/ps2names.py && tools/ghidra names   # name functions from the PS2 prototype
tools/reccmp ghidra-import --target BIONICLE --local-project-name BionicleHeroes \
    --local-project-dir "$PWD/ghidra" --file /Bionicle.exe   # names/signatures of matched code
tools/ghidra decompile 0x565840     # print decompiled C
```

The project in `ghidra/` is gitignored; every step above can be rerun. PS2 names carry
a plate comment with the PS2 signature, source object and how the pairing was found.

## Docs

- [docs/roadmap.md](docs/roadmap.md): milestones toward a Linux build, and the work queue
- [docs/ps2-prototype.md](docs/ps2-prototype.md): the PS2 prototype's symbols, map and source tree
- [docs/progress.md](docs/progress.md): matching progress
- [docs/prior-art.md](docs/prior-art.md): existing BH/Nu2 reverse-engineering work and tooling
- [docs/binary-notes.md](docs/binary-notes.md): the target binary, toolchain
  fingerprint, engine and section layout
