# Running the game

`tools/rungame` runs the original game (or any build of it) under Wine from a working
copy in `run/` (gitignored).

```sh
tools/rungame setup        # copy orig/game -> run/game and finish the install
tools/rungame              # play it (needs a display)
tools/rungame --headless=90 --keys "20:Escape 40:Return"   # Xvfb, screenshots in run/shots/
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

The game imports `xinput1_1.dll`, so the hook DLL takes that name (`rungame` adds the
`xinput1_1=n,b` override when it's present). Its `XInputGetState`/`GetCapabilities`/…
pass through to `xinput1_3`. On load, `DllMain` writes a `jmp` over the first five bytes
of each hooked function in the exe, pointing at the recompiled copy.

`tools/hook.py` assembles the DLL from the matching build:

- **Objects:** those whose annotated functions all match (exact or effective) and that
  define no `GLOBAL`s or `STUB`s. Mixed objects are listed as "not hooked" until they
  match.
- **Everything else they touch stays in the exe.** Unresolved references from a first
  link pass are bound to their original addresses in a generated `glue.asm`. Globals
  become absolute symbols (`_nu_current_thread_id EQU 03517EF0h`); functions become
  `jmp [slot]` thunks, because `link` treats absolute symbols as RVAs for `call rel32`.
  The DLL is `/FIXED` at `0x10000000`, so none of this needs relocating.
- Functions shorter than 5 bytes aren't patched.

### Trace mode

`tools/hook.py --trace --install` routes each hook through a trampoline
(`lock inc [counter]; jmp function`), and a DLL thread rewrites `run/game/bhhook-trace.txt`
every 5 s. Calls between functions inside the DLL bypass the trampolines, so only calls
coming from the exe are counted.

First result (2026-09-30), booting to the title screen with all 10 `listman.c`
functions redirected:

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
