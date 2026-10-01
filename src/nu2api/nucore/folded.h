#ifndef FOLDED_H
#define FOLDED_H

/* Functions that are empty on PC, which the linker folded (/OPT:ICF) into one copy per
 * body: one stub stands for each group (pc/folded_pc.c), and the original names alias it.
 * The stubs are unprototyped so every caller's arguments still compile. */

/* `return 0` in the PC platform layer (frame-pointer build, see below): the memory-card
 * file layer (PS2: nucore/ps2/ps2mc.c) and the pad deadzone query. */
int NuPCZeroFolded();
#define NuMcWrite NuPCZeroFolded
#define NuMcRead NuPCZeroFolded
#define NuMcSeek NuPCZeroFolded
#define NuMcClose NuPCZeroFolded
#define NuMcOpen NuPCZeroFolded
#define NuMcOpenSize NuPCZeroFolded
#define NuPadGetDeadzoneByPortPS NuPCZeroFolded

/* Empty `void` functions: PS2 hardware hooks with nothing to do on PC. */
void NuEmptyFolded();
#define NuPSFileInitDevices NuEmptyFolded
#define NuVideoSetSwapModePS NuEmptyFolded

/* Empty `void` functions of the PC platform layer, which was built with frame pointers
 * (`push ebp; mov ebp, esp; pop ebp; ret`), so they fold separately. */
void NuPCEmptyFolded();
#define NuPadOpenPS NuPCEmptyFolded
#define NuPadGetDeadzonePS NuPCEmptyFolded

#endif
