# Running the game

`tools/rungame` runs the original game (or any build of it) under Wine from a working
copy in `run/` (gitignored).

```sh
tools/rungame setup        # copy orig/game -> run/game and finish the install
tools/rungame              # play it (needs a display)
tools/rungame --headless=90 --keys "20:Escape 40:Return"   # Xvfb, screenshots in run/shots/
                                     # (keys are held 0.3 s; the game samples once a frame)
```

## What setup does

The MSI only lays down files. The InstallShield script finishes the install in two
steps, which `setup` repeats:

- **Music:** `Audio/Ogg2Wav.exe /recursive /quiet <Audio dir>` runs the bundled
  `oggdec.exe` on every `.ogg` under `_Music/` and `_Cutscenes/`, writing `.wgt` files
  (the extension the exe loads) and deleting the originals: 170 files.
- **Controller config:** the disc's per-language `files_<LANG>/nuconfig.txt` goes next to
  the exe. We use the UK one (`orig/nuconfig.txt`, saved by `tools/extract_orig.py`).

The game uses its own Wine prefix, `run/prefix`. Wine's builtin `d3d9`, `d3dx9_30`,
`dsound`, `dinput8` and `xinput1_1` are enough; the bundled DirectX redistributable
isn't needed.

## Headless runs

`--headless` runs the game in Xvfb (1024×768) inside a Wine **virtual desktop**, saving
a screenshot every 5 seconds to `run/shots/`. The virtual desktop is required: the game
switches to fullscreen at startup, Xvfb can't change display modes, and without the
desktop Direct3D device creation fails ("d3d device unavailable").

Rendering is software (Mesa llvmpipe through WineD3D). The original exe reaches the
**"Press any button" title screen at about 80 s** after the intro movies (Bink), TT logo
and loading screen. This is the baseline smoke test for modified builds (hook DLL).

Log: `run/game.log`. Wine debug channels go through `WINEDEBUG`, e.g.
`WINEDEBUG=err+all,warn+d3d tools/rungame --headless=20`.

## Hook DLL: decompiled code in the original game

```sh
tools/hook.py --install          # build build/hook/xinput1_1.dll and copy it into run/game
tools/rungame --headless=85      # run/game/bhhook.log: "bhhook: redirected N functions"
rm run/game/xinput1_1.dll        # back to the unmodified game
```

Input check: the title screen appears at ~80 s, so
`tools/rungame --headless=115 --keys "90:Return 92:Return 100:Return"` should end past
"Press any button" (in the main menu or a new game's save prompt). With the pad code
hooked (`NuPadRead`), this exercises our input path.

The game imports `xinput1_1.dll`, so the hook DLL takes that name (`rungame` adds the
`xinput1_1=n,b` override when it's present). Its `XInputGetState`/`GetCapabilities`/…
pass through to `xinput1_3`. On load, `DllMain` writes a `jmp` over the first five bytes
of each hooked function in the exe, pointing at the recompiled copy.

`tools/hook.py` assembles the DLL from the matching build:

- **Objects:** every object with at least one matched (exact or effective) function, as a
  patched copy (`tools/coffpatch.py` rewrites its COFF symbol table). Matched functions
  get hooked; static ones are made external under a per-object name so the hook table
  can reference them. Stubs, still-mismatching functions and annotated `GLOBAL`s are
  *unbound*: references to them are redirected to new undefined externals, so the
  object's code uses the exe's copies, not its own. (The local definition survives as
  `<name>$dead`, because `/O2` puts every function in a COMDAT section that needs its
  defining symbol.) A file can therefore be hook-tested while partly done.
- **Everything else they touch stays in the exe.** Unresolved references from a first
  link pass are bound to their original addresses in a generated `glue.asm`. Globals
  become absolute symbols (`_nu_current_thread_id EQU 03517EF0h`); functions become
  `jmp [slot]` thunks, because `link` treats absolute symbols as RVAs for `call rel32`.
  The DLL is `/FIXED` at `0x10000000`, so none of this needs relocating.
- Functions shorter than 5 bytes aren't patched.
- **The C runtime is split.** The DLL statically links its own CRT. Pure helpers
  (`_ftol2`, `strlen`, `__security_check_cookie` with the DLL's own cookie) can stay
  local, but anything with state shared with the exe must be the exe's copy: a `FILE*`
  from the exe's `fopen` passed to the DLL's `fseek` hung the game at boot. `hook.py`
  renames our objects' references to the functions in its `EXE_CRT` list (stdio,
  `malloc`/`free`) to `_bh_crt_<name>` and thunks those to the exe's copies (addresses
  from `src/library.h`); add to it when decompiled code starts calling `rand`, … Only our
  objects are redirected: overriding `malloc` DLL-wide crashed the game before it showed
  anything, since the DLL's CRT startup allocates before the exe's heap exists.
- A hook that breaks the game can be bisected with
  `tools/hook.py --install --skip NuPSFile,DEV_` (names or prefixes left to the exe).

### Trace mode

`tools/hook.py --trace --install` routes each hook through a trampoline
(`lock inc [counter]; jmp function`), and a DLL thread rewrites `run/game/bhhook-trace.txt`
every 5 s. Calls between functions inside the DLL bypass the trampolines, so only calls
coming from the exe are counted.

Booting to the title screen with `listman.c` and `numath/nurand.c` redirected (19
functions; `nurand`'s `fseed`, `NuWindQS` and static `global_rand` bound to the exe's):
`NuRandFloatSeeded` ran 3,444 times, `NuRandIntSeeded` 3,338, `NuLstGetNext` ~35,000.
The listman-only run:

```
         1 005654c0 NuLstCreate
         3 00565550 NuLstCreateBuff
         0 00565600 NuLstAllocHead      (reached via NuLstAlloc inside the DLL)
         1 00565680 NuLstAllocTail
        20 00565700 NuLstAllocFree
        20 00565760 NuLstAttachTail
         1 005657b0 NuLstFree
         0 00565840 NuLstGetByIdx
     35780 00565860 NuLstGetNext
       259 00565890 NuLstAlloc
```
