#ifndef NUVPORT_H
#define NUVPORT_H

#include "numath.h"
#include "nutypes.h"

/* Viewport (layout from the PS2 prototype). */
typedef struct nuviewport_s {
    int x;
    int y;
    int width;
    int height;
    float zmin;
    float zmax;
    float centre_x;
    float centre_y;
    float clipmin_x;
    float clipmin_y;
    float clipmax_x;
    float clipmax_y;
    float clip_w;
    float clip_h;
    float scis_w;
    float scis_h;
} NUVIEWPORT;

extern NUMTX vp_cmtx;
extern NUVIEWPORT vpCurrent;
extern NUMTX vp_smtx;
/* Offset and scale applied to the projection, for tiled hi-res screen dumps. */
extern float vport_hrdump_xoffset;
extern float vport_hrdump_yoffset;
extern float vport_hrdump_xscale;
extern float vport_hrdump_yscale;

void NuVpGetClippingMtx(NUMTX *mtx);
void NuVpGetScalingMtx(NUMTX *mtx);
NUVIEWPORT *NuVpGetCurrentViewport(void);
/* PS2: nu3d/ps2/nuvport.c, not decompiled yet (nu3d_stubs.c). */
void NuVpUpdate(void);

#endif
