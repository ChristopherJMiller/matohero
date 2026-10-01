#ifndef FOLDED_H
#define FOLDED_H

/* Functions that are empty on PC, which the linker folded (/OPT:ICF) into one copy per
 * body: one stub stands for each group (pc/folded_pc.c), and the original names alias it.
 * The stubs are unprototyped so every caller's arguments still compile. */

/* `return 0`: the memory-card file layer (PS2: nucore/ps2/ps2mc.c). */
int NuMcFolded();
#define NuMcWrite NuMcFolded
#define NuMcRead NuMcFolded
#define NuMcSeek NuMcFolded
#define NuMcClose NuMcFolded
#define NuMcOpen NuMcFolded
#define NuMcOpenSize NuMcFolded

/* Empty `void` functions: PS2 hardware hooks with nothing to do on PC. */
void NuEmptyFolded();
#define NuPSFileInitDevices NuEmptyFolded
#define NuVideoSetSwapModePS NuEmptyFolded

#endif
