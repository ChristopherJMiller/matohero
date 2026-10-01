#include "nucamera.h"
#include "nurndr.h"
#include "nuspecial.h"
#include "nuvport.h"

/* The PC renderer front end (PS2: nu3d/ps2/nurndr.c), which drives Direct3D through the
 * PC layer's wrappers. Names and locals from the PS2 prototype. PC moved NuCameraSet here
 * from nucamera_gen.c, to hand the view and projection to Direct3D. */

// GLOBAL: BIONICLE 0x337a2f4
NUVEC curlight;

// GLOBAL: BIONICLE 0x337a300
unsigned int curlightcol;

// GLOBAL: BIONICLE 0x3492e54
float curlightfade;

// GLOBAL: BIONICLE 0x3492e58
float curlightfadefar;

// GLOBAL: BIONICLE 0x337a39c
int nurndr_shadowson;

/* Alpha for the next draw, or -1 for the material's own (name ours). */
// GLOBAL: BIONICLE 0x617118
float nurndr_alpha = -1.0f;

// FUNCTION: BIONICLE 0x53ac50
void NuRndrShadowDirCol(NUVEC *lightdir, unsigned int col, float fadedist, float fadefar) {
    curlight = *lightdir;
    curlightcol = ((col >> 1) & 0x7f7f7f) + ((col >> 24) << 25);
    curlightfade = fadedist;
    curlightfadefar = fadefar;
}

// FUNCTION: BIONICLE 0x53b480
int NuSpecialDrawAtAlpha(NUHSPECIAL *Gfx, NUMTX *Mtx, float AlphaF) {
    nurndr_alpha = AlphaF;
    NuSpecialDrawAt(Gfx, Mtx);
    nurndr_alpha = -1.0f;
    return 0;
}

// FUNCTION: BIONICLE 0x53b4b0
void NuRndrShadowOnOff(int flag) {
    nurndr_shadowson = flag;
}

// FUNCTION: BIONICLE 0x53cd80
int NuRndrGScnObj(NUGOBJ *gobj, NUMTX *wm) {
    int rv = NuRndrGScnObjClipFlag(gobj, wm, 0);

    if (rv > 1) {
        rv = 1;
    }
    return rv;
}

// FUNCTION: BIONICLE 0x53d2c0
int NuHGobjRndr(NUHGOBJ *hgobj, NUMTX *wm, int nlayers, short *layers) {
    // GLOBAL: BIONICLE 0xd5ded8
    static NUMTX mtx_array[256];

    NuHGobjEval(hgobj, 0, NULL, mtx_array);
    return NuHGobjRndrMtxDwa(hgobj, wm, nlayers, layers, mtx_array, NULL, 0);
}

// FUNCTION: BIONICLE 0x53d640
void NuCameraSet(NUCAMERA *camera) {
    NuCameraSetEx(camera, 0);
    NuVpGetClippingMtx(&cmtx);
    NuMtxMulH(&vpc_vport_mtx, &vpmtx, &cmtx);
    vpc_sci_mtx = vpc_vport_mtx;
    d3dSetTransform(2, &vmtx); /* D3DTS_VIEW */
    d3dSetTransform(3, &pmtx); /* D3DTS_PROJECTION */
}
