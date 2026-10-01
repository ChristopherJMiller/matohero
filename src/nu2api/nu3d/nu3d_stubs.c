#include "nucamera.h"
#include "nurndr.h"
#include "nuvport.h"

/* nu3d functions and globals the decompiled code uses, not decompiled yet. */

// GLOBAL: BIONICLE 0x348f754
NUMTL *nurndr_forced_mtl;

// GLOBAL: BIONICLE 0x348f758
NUMTL **nurndr_forced_mtl_table;

// GLOBAL: BIONICLE 0x348f75c
int nurndr_nforced_mtls;

// GLOBAL: BIONICLE 0x348f764
int nurndr_pixel_width;

// GLOBAL: BIONICLE 0x348f768
int nurndr_pixel_height;

// GLOBAL: BIONICLE 0x617110
int PS2_VCNTR_X = 320;

// GLOBAL: BIONICLE 0x617114
int PS2_VCNTR_Y = 240;

// GLOBAL: BIONICLE 0x616bd8
int PS2_VREZ_W = 640;

// GLOBAL: BIONICLE 0x616bdc
int PS2_VREZ_H = 224;

// GLOBAL: BIONICLE 0x616be0
int PS2_SREZ_W = 4096;

// GLOBAL: BIONICLE 0x616be4
int PS2_SREZ_H = 4096;

// STUB: BIONICLE 0x53ef70
void NuPortalInit(void) {
}

// STUB: BIONICLE 0x54c450
void NuWaterReset(void) {
}

// STUB: BIONICLE 0x5410b0
void NuMtlAnimate(float timestep) {
}

// STUB: BIONICLE 0x54f420
void NuTexAnimProcess(float frametime) {
}

// STUB: BIONICLE 0x550310
void NuTimeBarSetRender(int tbh) {
}

// STUB: BIONICLE 0x53ae40
int NuRndrSwapScreenEx(int hRT, void (*fn)()) {
    return 0;
}

// STUB: BIONICLE 0x53cd80
int NuRndrGScnObj(NUGOBJ *gobj, NUMTX *wm) {
    return 0;
}

// STUB: BIONICLE 0x554610
void NuDisplayListUpdateSpecial(NUHSPECIAL *hspec) {
}

// STUB: BIONICLE 0x554be0
int NuDisplayListRndrSpecial(NUHSPECIAL *special, NUMTX *mtx, int skinned, NUMTX *skin_mtx, void *blend_values) {
    return 0;
}

// STUB: BIONICLE 0x53d640
void NuCameraSet(NUCAMERA *camera) {
}

// STUB: BIONICLE 0x581070
void d3dGetViewport(NUVIEWPORT *vp) {
}

// STUB: BIONICLE 0x581540
void d3dSetViewport(NUPCVIEWPORT *vp) {
}

// STUB: BIONICLE 0x53b090
int NuRndrSetSpecularLight(NUVEC *dir, NUCOLOUR4 *intensity) {
    return 0;
}

// STUB: BIONICLE 0x53e350
void NuCameraSetProjectionMtx(NUMTX *mtx, float fFOV, float fAspect, float fNearPlane, float fFarPlane) {
}

// STUB: BIONICLE 0x544960
int NuRndrStateUpdateCameraState(void) {
    return 0;
}
