#ifndef NURNDR_H
#define NURNDR_H

#include "nugscn.h"

/* nurndr_gen.c, not decompiled yet (globals defined in nu3d_stubs.c). */
extern NUMTL *nurndr_forced_mtl;
extern NUMTL **nurndr_forced_mtl_table;
extern int nurndr_nforced_mtls;

int NuRndrGScnObj(NUGOBJ *gobj, NUMTX *wm);

/* Render target size in pixels (nurndr_gen.c). */
extern int nurndr_pixel_width;
extern int nurndr_pixel_height;

/* Virtual screen centre and size, and scissor resolution (PS2: nups2/ps2video.c; PC file
 * unknown). */
extern int PS2_VCNTR_X;
extern int PS2_VCNTR_Y;
extern int PS2_VREZ_W;
extern int PS2_VREZ_H;
extern int PS2_SREZ_W;
extern int PS2_SREZ_H;

typedef struct nucolour4_s {
    float r;
    float g;
    float b;
    float a;
} NUCOLOUR4;

/* PS2: nu3d/ps2/nustream.c and nurndrstat.c, not decompiled yet. */
int NuRndrSetSpecularLight(NUVEC *dir, NUCOLOUR4 *intensity);
int NuRndrStateUpdateCameraState(void);

/* nudlist.c, not decompiled yet. */
void NuDisplayListUpdateSpecial(NUHSPECIAL *hspec);
int NuDisplayListRndrSpecial(NUHSPECIAL *special, NUMTX *mtx, int skinned, NUMTX *skin_mtx, void *blend_values);

#endif
