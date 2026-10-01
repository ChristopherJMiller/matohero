# Binary notes

Facts about the shipped executables that shape how this decomp is set up.

## Which binary we target

The retail disc ships `bionicle.exe` wrapped in SecuROM: its `.text` is encrypted
(entropy 7.99) and decrypted at runtime, and it carries extra sections
(`nunc`, `bibendum`, `est`, `.securom`).

The ViTALiTY release on the same rip (and the "No DRM" ISO, which is byte-identical)
contains a decrypted `Bionicle.exe`. Comparing it to retail:

| Section  | vs. retail |
|----------|------------|
| `.text`  | can't compare directly (retail is encrypted); same VA (`0x401000`) and size (`0x1c5d24`) |
| `.rdata` | identical except the first `0x2b0` bytes: the import address table, which the crack rebuilt at the standard MSVC location (`0x5c7000`) |
| `.data`  | identical |
| `.rsrc`  | identical |

A scan of the decrypted `.text` finds 368 `call/jmp [IAT]` sites, no indirect
references to unmapped addresses, and no calls into the stripped SecuROM sections, so
the API redirections appear to be fully undone. **The decrypted `Bionicle.exe`
(`sha1 8f24d5a2…`) is our reference binary.** If a function refuses to match for no
apparent reason, a crack patch is one possible cause.

The crack rewrote the PE timestamp to 2015 and zeroed the debug directory. The retail
header still has the originals:

- build timestamp: `2006-10-26 19:32:01 UTC`
- PDB path: `c:\Projects\Bionicle\Release\Bionicle.pdb` (the PDB itself never shipped)

`bionicle_win2k.exe` is a separate build of the same code without XInput (no
`XINPUT1_1.dll` import, `.text` 0x2000 smaller). It's secondary for now.

## Toolchain (from the Rich header)

| ProdID | Build | Count | Meaning |
|-------:|------:|------:|---------|
| 95 (0x5f) | 6030 | 275 | `cl` C, 13.10.6030 (Visual Studio .NET 2003 SP1) |
| 96 (0x60) | 6030 | 214 | `cl` C++, 13.10.6030 |
| 15 (0x0f) | 6030 | 37 | MASM 7.10 SP1 (mostly the static CRT) |
| 90 (0x5a) | 6030 | 1 | linker 7.10 SP1 |
| 93 (0x5d) | various | 27 | import libs (Windows/DirectX SDKs) |
| 95/96 | 4035, 2179 | 6 | objects from prebuilt static libs |
| 1 | 0 | 189 | import stubs |

The counts include the statically linked CRT, so the game itself is somewhat fewer
than ~490 objects. **To match, game code must be compiled with `cl` 13.10.6030**; the
free Visual C++ Toolkit 2003 is 13.10.3077 (RTM), which may produce slightly
different code.

## Engine

Assert/debug strings keep the original relative source paths, which give us part of
the source tree. It's Traveller's Tales' Nu2 engine (shared with LEGO Star Wars):

```
.\nufile.c  .\nupad.c  .\nutrig.c  .\nutexanm.c  .\nucamera_gen.c  .\nutimebar.c
.\pc\nurndr.c  .\pc\nutex.c  .\pc\nugobj.c  .\pc\nustream.c  .\pc\nusound.cpp
.\pc\Bink\dsplay_bink.cpp  .\d3dCalls.cpp  .\pcapi.cpp
.\gamelib\edgra.c  .\edtools\edui.c  .\rtl\rtl.c  .\listman.c
.\charsys\charbase.cpp  .\Geoms\MeshGeom.cpp
.\gui\gamemenu\apisave.c  .\soundmgr.cpp  .\main.cpp
```

No symbols, and RTTI exists only for standard library classes (`std::bad_alloc`,
`std::logic_error`, …), so game class and function names must be recovered by hand.

## Layout

| Section | VA | Virtual size | Notes |
|---------|----|--------------|-------|
| `.text`  | `0x401000`  | `0x1c5d24` | code |
| `.rdata` | `0x5c7000`  | `0x3176e`  | IAT at start, then constants/strings |
| `.data`  | `0x5f9000`  | `0x30326b8`| only `0x4127c` initialized; ~48 MB of BSS (big static pools) |
| `.shr`   | `0x362c000` | `0x4`      | shared section, empty |
| `.rsrc`  | `0x362d000` | `0xcc7e0`  | |

Imports: `d3d9`, `d3dx9_30`, `dsound`, `dinput8`, `xinput1_1`, `binkw32`, `winmm`,
plus the usual Win32 DLLs. No relocations, so the image always loads at `0x400000`.

Sizes above are from the retail header. The crack's rebuilt section headers round
virtual sizes up to the page (and swap `.text`'s virtual/raw sizes), so tools reading
the reference binary will see e.g. `.text` virtual size `0x1c6000`.

## Code generation fingerprint

From Ghidra's auto-analysis (5,507 functions, ~1.5 MB of function bodies) and byte
statistics over `.text`:

- **Optimized release build, frame pointers mostly omitted** (`/O2`-style): only ~950
  functions open with `push ebp; mov ebp, esp`.
- **`/GS` on**: 286 functions call `__security_check_cookie`. Confirmed by matching
  `NuSPrintfW` (cookie at `0x639088`); in the build flags since nustring.
- **`/Ob1`**: only functions marked `__inline` are inlined. Confirmed by `NuSPrintfW`;
  see `docs/matching.md`.
- **C++ exceptions** (`/EHsc`-style): ~130 functions set up an EH frame
  (`push -1; push handler; mov eax, fs:[0]`), plus compiler-generated `Unwind@` funclets
  at the end of `.text`.
- **x87 floating point, no SSE**: about 3,200 `fld dword [mem]` against 3 `movss`.
- Functions are 16-byte aligned (91%) and padded with `0xCC`.
- Median function is 109 bytes; 90% are under 700 bytes; the largest is ~9 KB.

These are starting guesses for compiler flags. The real flags get pinned down by
compiling candidate functions and comparing.

## Rough address map

| Range | Contents |
|-------|----------|
| `0x401000`–`~0x535000` | game code (`main.cpp` ≈ `0x4897c0`, `charsys\`, `edtools\edui.c`, `gui\`) |
| `~0x535000`–`~0x5b4a00` | Nu2 engine (`nucamera_gen.c`, `nufile.c`, `pc\nurndr.c`, `pc\nusound.cpp`, `d3dCalls.cpp`, `Geoms\MeshGeom.cpp`), probably linked as a static library |
| `0x5b4a34`–`0x5c4000` | statically linked MSVC 7.1 CRT (`operator new`, `_sprintf`, …; matched by Ghidra's function ID database) |
| `0x5c4000`–end | compiler-generated EH `Unwind@`/`Catch@` funclets |

Source-file hints come from `tools/ghidra_scripts/ExportOverview.java`, which writes
`ghidra/export/source_hints.csv`. Boundaries are approximate.
