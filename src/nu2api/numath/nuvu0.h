#ifndef NUVU0_H
#define NUVU0_H

#include "numtx.h"

/* PC replacements for the PS2's VU0 routines (pc/nuvu0_pc.c). */

float NuFsqrt(float f);
void NuMtxSetRotateXYZVU0(NUMTX *m, NUANGVEC *a);
void NuMtxScaleVU0(NUMTX *m, NUVEC *s);
void NuMtxMulRVU0(NUMTX *m, NUMTX *m0, NUMTX *m1);
void NuVec4ScaleXYZVU0(NUVEC4 *v, NUVEC4 *v0, float k);
void NuMtxPreScaleVU0(NUMTX *m, NUVEC *s);
void NuVec4MtxRotateVU0(NUVEC4 *v, NUVEC4 *v0, NUMTX *m0);
void NuMtxMulVU0(NUMTX *m, NUMTX *m0, NUMTX *m1);
void NuMtxPreScaleUVU0(NUMTX *m, float s);
void NuVecMtxTransform(NUVEC *v, NUVEC *v0, NUMTX *m0);

#endif
