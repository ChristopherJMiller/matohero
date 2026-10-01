#include "nuvport.h"

/* Viewport state (PS2: nu3d/nuvport_gen.c). Only what the camera code needs so far; the
 * rest of the file (NuVpSetSize, ...) sits at 0x53a140-0x53a4d0. */

// GLOBAL: BIONICLE 0x3607268
float vport_hrdump_xoffset;

// GLOBAL: BIONICLE 0x3607264
float vport_hrdump_yoffset;

// GLOBAL: BIONICLE 0x360726c
float vport_hrdump_xscale;

// GLOBAL: BIONICLE 0x3607260
float vport_hrdump_yscale;

// GLOBAL: BIONICLE 0x6169b0
NUMTX vp_cmtx = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

// GLOBAL: BIONICLE 0x3607280
NUVIEWPORT vpCurrent;

// GLOBAL: BIONICLE 0x3607300
NUMTX vp_smtx;

// FUNCTION: BIONICLE 0x53a4d0
void NuVpGetClippingMtx(NUMTX *mtx) {
    if (mtx != NULL) {
        *mtx = vp_cmtx;
    }
}

// FUNCTION: BIONICLE 0x53a4f0
void NuVpGetScalingMtx(NUMTX *mtx) {
    if (mtx != NULL) {
        *mtx = vp_smtx;
    }
}

// FUNCTION: BIONICLE 0x53a510
NUVIEWPORT *NuVpGetCurrentViewport(void) {
    return &vpCurrent;
}
