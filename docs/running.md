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
