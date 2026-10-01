#include "nuvport.h"

#include <string.h>

#include "nufloat.h"
#include "nurndr.h"
#include "numtx.h"

/* Viewport state (PS2: nu3d/nuvport_gen.c, with NuVpSetScalingMtx from ps2/nuvport.c).
 * Names from the PS2 prototype; logic checked against opensagadev/saga
 * src/nu2api/nu3d/nuvport.cpp (GPL-3.0, a later engine). Positions and sizes are kept in
 * 1/16 pixels. Not in the PC exe (unreferenced): NuVpSetPosition, NuVpSetZRange,
 * NuVpSetCentre, NuVpSetClipping, NuVpSetCurrent, NuVpGetCurrent, NuVpVirtualWidth,
 * NuVpVirtualHeight, NuVpPixelWidth, NuVpPixelHeight, NuViewPortSet; NuVpSetSize2 and
 * NuVpGetSize2 only exist inlined. */

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

// GLOBAL: BIONICLE 0x36072c0
NUVIEWPORT vpDevice;

// GLOBAL: BIONICLE 0x3607280
NUVIEWPORT vpCurrent;

// GLOBAL: BIONICLE 0x3607300
NUMTX vp_smtx;

// GLOBAL: BIONICLE 0x3607270
int vport_inval;

// FUNCTION: BIONICLE 0x53a140
void NuVpSetSize(float w, float h) {
    vpCurrent.width = (int)w;
    vpCurrent.height = (int)h;
    vport_inval = 1;
}

// FUNCTION: BIONICLE 0x53a170
void NuVpSetPosition2(float x, float y) {
    vpCurrent.x = (int)((nurndr_pixel_width * x / PS2_VREZ_W + ((float)PS2_VCNTR_X - (nurndr_pixel_width >> 1))) * 16.0f);
    vpCurrent.y = (int)((nurndr_pixel_height * y / PS2_VREZ_H + ((float)PS2_VCNTR_Y - (nurndr_pixel_height >> 1))) * 16.0f);
    vport_inval = 1;
}

// FUNCTION: BIONICLE 0x53a1f0
void NuVpGetPosition2(float *x, float *y) {
    *x = NuFdiv(vpCurrent.x * 0.0625f - ((float)PS2_VCNTR_X - (nurndr_pixel_width >> 1)),
                NuFdiv((float)nurndr_pixel_width, (float)PS2_VREZ_W));
    *y = NuFdiv(vpCurrent.y * 0.0625f - ((float)PS2_VCNTR_Y - (nurndr_pixel_height >> 1)),
                NuFdiv((float)nurndr_pixel_height, (float)PS2_VREZ_H));
}

static __inline void NuVpSetSize2(float w, float h) {
    vpCurrent.width = (int)(nurndr_pixel_width * w / PS2_VREZ_W * 16.0f);
    vpCurrent.height = (int)(nurndr_pixel_height * h / PS2_VREZ_H * 16.0f);
    vport_inval = 1;
}

static __inline void NuVpGetSize2(float *w, float *h) {
    *w = vpCurrent.width * 0.0625f / ((float)nurndr_pixel_width / PS2_VREZ_W);
    *h = vpCurrent.height * 0.0625f / ((float)nurndr_pixel_height / PS2_VREZ_H);
}

// FUNCTION: BIONICLE 0x53a350
void NuVpSetCurrent2(NUVIEWPORT2 *vp) {
    NuVpSetPosition2(vp->x, vp->y);
    NuVpSetSize2(vp->width, vp->height);
    vpCurrent.centre_x = vp->centre_x;
    vpCurrent.centre_y = vp->centre_y;
    vpCurrent.clipmin_x = vp->clipmin_x;
    vpCurrent.clipmin_y = vp->clipmin_y;
    vpCurrent.clipmax_x = vp->clipmax_x;
    vpCurrent.clipmax_y = vp->clipmax_y;
    vpCurrent.clip_w = vp->clip_w;
    vpCurrent.clip_h = vp->clip_h;
    vpCurrent.scis_w = vp->scis_w;
    vpCurrent.scis_h = vp->scis_h;
    vport_inval = 1;
}

// FUNCTION: BIONICLE 0x53a420
void NuVpGetCurrent2(NUVIEWPORT2 *vp) {
    NuVpGetPosition2(&vp->x, &vp->y);
    NuVpGetSize2(&vp->width, &vp->height);
    vp->centre_x = vpCurrent.centre_x;
    vp->centre_y = vpCurrent.centre_y;
    vp->clipmin_x = vpCurrent.clipmin_x;
    vp->clipmin_y = vpCurrent.clipmin_y;
    vp->clipmax_x = vpCurrent.clipmax_x;
    vp->clipmax_y = vpCurrent.clipmax_y;
    vp->clip_w = vpCurrent.clip_w;
    vp->clip_h = vpCurrent.clip_h;
    vp->scis_w = vpCurrent.scis_w;
    vp->scis_h = vpCurrent.scis_h;
}

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

// FUNCTION: BIONICLE 0x53a520
void NuVpRestore(void) {
    NUPCVIEWPORT vp;

    vpCurrent = vpDevice;
    vp.X = vpCurrent.x;
    vp.Y = vpCurrent.y;
    vp.Width = vpCurrent.width;
    vp.Height = vpCurrent.height;
    vp.MinZ = vpCurrent.zmin;
    vp.MaxZ = vpCurrent.zmax;
    vp.Width = vp.Width * 0.0625f;
    vp.Height = vp.Height * 0.0625f;
    d3dSetViewport(&vp);
    vport_inval = 1;
}

// FUNCTION: BIONICLE 0x53a5d0
void NuVpSetScalingMtx(void) {
    float vp_w = (float)nurndr_pixel_width;
    float vp_h = (float)nurndr_pixel_height;
    float vp_minz = vpCurrent.zmin;

    vp_smtx._00 = vp_w * 0.5f;
    vp_smtx._01 = 0.0f;
    vp_smtx._02 = 0.0f;
    vp_smtx._03 = 0.0f;
    vp_smtx._10 = 0.0f;
    vp_smtx._11 = vp_h * -0.5f;
    vp_smtx._12 = 0.0f;
    vp_smtx._13 = 0.0f;
    vp_smtx._20 = 0.0f;
    vp_smtx._21 = 0.0f;
    vp_smtx._22 = 16777215.0f - vp_minz;
    vp_smtx._23 = 0.0f;
    vp_smtx._30 = vp_w * 0.5f;
    vp_smtx._31 = vp_h * 0.5f;
    vp_smtx._32 = vp_minz;
    vp_smtx._33 = 1.0f;
}

// FUNCTION: BIONICLE 0x53a6a0
void NuVpUpdate(void) {
    NUPCVIEWPORT vp;
    int xflip;
    int yflip;
    int sw;
    int sh;
    int x;
    int y;
    int w;
    int h;
    int right;
    int bottom;

    if (vport_inval) {
        vp = *(NUPCVIEWPORT *)&vpCurrent;
        xflip = 0;
        yflip = 0;
        vport_hrdump_xscale = 1.0f;
        vport_hrdump_xoffset = 0.0f;
        vport_hrdump_yscale = 1.0f;
        vport_hrdump_yoffset = 0.0f;
        sw = (int)(nurndr_pixel_width * 16.0f);
        sh = (int)(nurndr_pixel_height * 16.0f);
        h = vpCurrent.height;
        w = vpCurrent.width;
        x = vpCurrent.x;
        y = vpCurrent.y;
        right = x + w;
        bottom = y + h;

        /* Clip the viewport to the render target; for a hi-res dump tile the projection is
         * scaled and offset to show just the visible part. */
        if (right < 0 || x >= sw || bottom < 0 || y >= sh) {
            x = y = w = h = 0;
        } else {
            if (x < 0) {
                w = right;
                x = 0;
                xflip = 1;
            }
            if (right > sw) {
                w += sw - right;
            }
            if (y < 0) {
                h = bottom;
                y = 0;
                yflip = 1;
            }
            if (bottom > sh) {
                h += sh - bottom;
            }
            if (w > 0 && h > 0) {
                vport_hrdump_xscale = (float)vpCurrent.width / w;
                vport_hrdump_yscale = (float)vpCurrent.height / h;
                vport_hrdump_xoffset = (float)(vpCurrent.width - w) / w;
                vport_hrdump_yoffset = -((float)(vpCurrent.height - h) / h);
                if (xflip) {
                    vport_hrdump_xoffset = -vport_hrdump_xoffset;
                }
                if (yflip) {
                    vport_hrdump_yoffset = -vport_hrdump_yoffset;
                }
            } else {
                x = y = w = h = 0;
            }
        }
        vp.X = x * 0.0625f;
        vp.Y = y * 0.0625f;
        vp.Width = w * 0.0625f;
        vp.Height = h * 0.0625f;
        vport_inval = 0;
        d3dSetViewport(&vp);
        NuMtxSetIdentity(&vp_cmtx);
        NuVpSetScalingMtx();
        if (vpCurrent.clipmin_x == 0.0f && vpCurrent.clipmax_x == 1.0f) {
            vpCurrent.clip_w = 1.0f;
            vpCurrent.scis_w = vpCurrent.width / 65536.0f;
        } else {
            vpCurrent.clip_w = vpCurrent.scis_w = 1.0f / (vpCurrent.clipmax_x - vpCurrent.clipmin_x);
        }
        if (vpCurrent.clipmin_y == 0.0f && vpCurrent.clipmax_y == 1.0f) {
            vpCurrent.clip_h = 1.0f;
            vpCurrent.scis_h = vpCurrent.height / 65536.0f;
        } else {
            vpCurrent.clip_h = vpCurrent.scis_h = 1.0f / (vpCurrent.clipmax_y - vpCurrent.clipmin_y);
        }
    }
}

// FUNCTION: BIONICLE 0x53a9e0
void NuVpInit(void) {
    d3dGetViewport(&vpDevice);
    vpDevice.centre_x = 0.5f;
    vpDevice.centre_y = 0.5f;
    vpDevice.clipmin_x = 0.0f;
    vpDevice.clipmin_y = 0.0f;
    vpDevice.clipmax_x = 1.0f;
    vpDevice.clipmax_y = 1.0f;
    vpDevice.height = (int)(vpDevice.height * 16.0f);
    vpDevice.width = (int)(vpDevice.width * 16.0f);
    vpDevice.x = (int)(vpDevice.x * 16.0f);
    vpDevice.y = (int)(vpDevice.y * 16.0f);
    vpCurrent = vpDevice;
    vport_inval = 1;
    NuVpUpdate();
}
