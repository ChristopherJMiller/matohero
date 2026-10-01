# Prior art and resources

Collected 2026-09-30. No decompilation of BIONICLE Heroes (PC) existed at that point.

## BIONICLE Heroes

- **PS2 prototype with debug symbols** (press build, 4 Sep 2006): same game code
  compiled for the EE (MIPS), so it should provide function and global names, and
  possibly types if DWARF type info is present (not yet checked). Likely our best
  source for names.
  - https://hiddenpalace.org/Bionicle_Heroes_(Sep_4,_2006_prototype)
  - https://www.maskofdestiny.com/news/bionicle-heroes-ps2-press-demo-2006-09-03
- BH Modding wiki, covering file formats (`.ghg`/`.hgp`, `.nup`) and how to extract
  `bionicle.dat` with QuickBMS `ttgames.bms`:
  https://bionicle-heroes-modding.fandom.com/wiki/File_Types
- HeroHookDLL, runtime ASM injection (no published addresses):
  https://github.com/0za0/HeroHookDLL
- Mod manager: https://github.com/0za0/BionicleHeroesModManager
- Asset loader: https://github.com/REDxEYE/pr_bh

## Nu2 engine (other TT games)

- **opensagadev/saga**: matching decomp of LEGO Star Wars: The Complete Saga (Android
  x86 `libTTapp.so`), with `src/nu2api/{nucore,nufile,numath,nu3d,nusound,…}` and
  symbol info. A later engine revision, but the richest source of nu2api names and
  struct layouts. https://github.com/opensagadev/saga
- Crash Bandicoot: The Wrath of Cortex, the first Nu2 game:
  - GameCube decomp with `symbols.txt`: https://github.com/denzi-gh/crashwoc-decomp
  - PS2 matching decomp: https://github.com/denzi-gh/crashwoc-decomp-ps2
  - OpenCrashWOC, includes a DWARF dump from an alpha GC build:
    https://github.com/Open-Travelers/OpenCrashWOC
- TTModding wiki list of executables that shipped with symbols (the LSW demo, LSW II
  for Mac, BH prototypes): https://ttmodding.fandom.com/wiki/EXE_file
- Asset tools: https://github.com/linterniGamer/Tt-Games-quickbms-scripts,
  https://github.com/IsaMorphic/DATOneArchiver,
  https://github.com/AcK77/TTGames-Explorer-Rebirth,
  https://github.com/leftmostcat/mortar-eng, https://github.com/codeshaunted/nu2debug

## Tooling

- reccmp (comparison harness): https://github.com/isledecomp/reccmp
  - Reads the recompiled build's PDB through Microsoft's `cvdump.exe`, run under Wine
    on Linux.
  - Known limitation with exes that have no relocation table (ours has none):
    https://github.com/isledecomp/reccmp/issues/235
  - Fork with MSVC 8.0 fixes for vtables, folding and magic statics:
    https://github.com/Asphaltian/reccmp/tree/msvc800
- decomp.me has `msvc7.1` (win32) compilers that run under
  [wibo](https://github.com/decompals/wibo). Check the exact `cl` build against
  13.10.6030 before trusting results.
- Projects using MSVC 7.x with Wine and reccmp: Touhou 6/7/8
  (https://github.com/GensokyoClub/th06, VS2002), NFS Most Wanted
  (https://github.com/Brawltendo/Most-Wanted-Vehicles-Decomp, VS .NET 2003).
- Visual C++ Toolkit 2003: v1.0 shipped `cl` 13.10.3052, v1.01 shipped 13.10.3077
  (RTM). It has no `lib.exe` (use `link /lib`). SP1 (13.10.6030) is a patch for an
  installed VS .NET 2003.
