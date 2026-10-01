#include "nufloat.h"
#include "numtx.h"
#include "nuvec4.h"

/* PC replacements for the PS2's VU0 routines (numath/ps2/numtxvu0.c, nuvecvu0.c):
 * mostly tail calls to the plain versions, plus NuFsqrt and NuVecMtxTransform, which
 * PC moved here out of nuvec.c. Names from the PS2 prototype, matched through callers;
 * the original file name isn't known. */

// FUNCTION: BIONICLE 0x571290
float NuFsqrt(float f) {
    if (f < 0.0f) {
        return 0.0f;
    }
    return (float)sqrt(f);
}

// FUNCTION: BIONICLE 0x5712b0
void NuMtxSetRotateXYZVU0(NUMTX *m, NUANGVEC *a) {
    NuMtxSetRotateXYZ(m, a);
}

// FUNCTION: BIONICLE 0x5712c0
void NuMtxScaleVU0(NUMTX *m, NUVEC *s) {
    NuMtxScale(m, s);
}

// FUNCTION: BIONICLE 0x5712d0
void NuMtxMulRVU0(NUMTX *m, NUMTX *m0, NUMTX *m1) {
    NuMtxMulR(m, m0, m1);
}

// FUNCTION: BIONICLE 0x5712e0
void NuVec4ScaleXYZVU0(NUVEC4 *v, NUVEC4 *v0, float k) {
    NuVec4Scale(v, v0, k);
}

// FUNCTION: BIONICLE 0x5712f0
void NuMtxPreScaleVU0(NUMTX *m, NUVEC *s) {
    NuMtxPreScale(m, s);
}

// FUNCTION: BIONICLE 0x571300
void NuVec4MtxRotateVU0(NUVEC4 *v, NUVEC4 *v0, NUMTX *m0) {
    float y = m0->_01 * v0->x + m0->_11 * v0->y + m0->_21 * v0->z;
    float z = m0->_02 * v0->x + m0->_12 * v0->y + m0->_22 * v0->z;

    v->x = m0->_00 * v0->x + m0->_10 * v0->y + m0->_20 * v0->z;
    v->y = y;
    v->z = z;
    v->w = v0->w;
}

// FUNCTION: BIONICLE 0x571360
void NuMtxMulVU0(NUMTX *m, NUMTX *m0, NUMTX *m1) {
    NuMtxMulH(m, m0, m1);
}

// FUNCTION: BIONICLE 0x571370
void NuMtxPreScaleUVU0(NUMTX *m, float s) {
    m->_00 *= s;
    m->_01 *= s;
    m->_02 *= s;
    m->_10 *= s;
    m->_11 *= s;
    m->_12 *= s;
    m->_20 *= s;
    m->_21 *= s;
    m->_22 *= s;
}

// FUNCTION: BIONICLE 0x5713d0
void NuVecMtxTransform(NUVEC *v, NUVEC *v0, NUMTX *m0) {
    float y = m0->_01 * v0->x + m0->_11 * v0->y + m0->_21 * v0->z + m0->_31;
    float z = m0->_02 * v0->x + m0->_12 * v0->y + m0->_22 * v0->z + m0->_32;

    v->x = m0->_00 * v0->x + m0->_10 * v0->y + m0->_20 * v0->z + m0->_30;
    v->y = y;
    v->z = z;
}
