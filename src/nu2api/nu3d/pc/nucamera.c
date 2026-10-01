#include "nucamera.h"

#include <string.h>

#include "nutrig.h"

/* PC counterpart of the PS2's nu3d/ps2/nucamera.c (file name ours). */

// FUNCTION: BIONICLE 0x53e350
void NuCameraSetProjectionMtx(NUMTX *mtx, float fFOV, float fAspect, float fNearPlane, float fFarPlane) {
    float w;
    float Q;
    float rtan;
    int ang = (int)((fFOV * 0.5f) * 10430.378f);

    rtan = NU_COS_LUT(ang) / NU_SIN_LUT(ang);
    memset(mtx, 0, sizeof(NUMTX));
    mtx->_23 = 1.0f;
    Q = fFarPlane / (fFarPlane - fNearPlane);
    w = rtan * fAspect;
    mtx->_00 = w;
    mtx->_11 = rtan;
    mtx->_22 = Q;
    mtx->_32 = -(Q * fNearPlane);
}
