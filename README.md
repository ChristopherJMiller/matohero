# BIONICLE Heroes decomp

A work-in-progress matching decompilation of **BIONICLE Heroes** (PC, 2006, Traveller's
Tales / Eidos). The goal is C/C++ source that compiles with the original toolchain
(MSVC 7.1 / Visual Studio .NET 2003 SP1) into code identical to the shipped
executable, function by function. Later goals: build with modern compilers (GCC/Clang)
and make it practical to modify or replace engine modules.

This repository contains **no game code or assets**. You need your own copy of the
game.

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

## Ghidra

```sh
"$GHIDRA_INSTALL_DIR/support/analyzeHeadless" ghidra BionicleHeroes \
    -import orig/Bionicle.exe -overwrite
```

The resulting project in `ghidra/` is gitignored.

## Docs

- [docs/prior-art.md](docs/prior-art.md): existing BH/Nu2 reverse-engineering work and tooling
- [docs/binary-notes.md](docs/binary-notes.md): the target binary, toolchain
  fingerprint, engine and section layout
