#include "numtx.h"

#include "nufloat.h"
#include "nutrig.h"
#include "nuvec.h"

/* 4x4 matrices (row vectors, translation in row 3). Ported from opensagadev/saga
 * src/nu2api/numath/numtx.cpp (GPL-3.0); names from the PS2 prototype. In PC order: the
 * PC exe keeps 35 of these. */

// GLOBAL: BIONICLE 0x6174e0
NUMTX numtx_identity = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

// GLOBAL: BIONICLE 0x34c60a0
NUMTX numtx_zero;

// FUNCTION: BIONICLE 0x56e250
void NuMtxSetZero(NUMTX *m) {
    *m = numtx_zero;
}

// FUNCTION: BIONICLE 0x56e270
void NuMtxSetIdentity(NUMTX *m) {
    *m = numtx_identity;
}

// FUNCTION: BIONICLE 0x56e290
void NuMtxSetTranslation(NUMTX *m, NUVEC *t) {
    m->_30 = t->x;
    m->_31 = t->y;
    m->_32 = t->z;
    m->_01 = m->_02 = m->_03 = m->_10 = m->_12 = m->_13 = m->_20 = m->_21 = m->_23 = 0.0f;
    m->_00 = m->_11 = m->_22 = m->_33 = 1.0f;
}

// FUNCTION: BIONICLE 0x56e2e0
void NuMtxSetRotationX(NUMTX *m, NUANG a) {
    m->_11 = m->_22 = NU_COS_LUT(a);
    m->_12 = NU_SIN_LUT(a);
    m->_21 = -m->_12;
    m->_00 = 1.0f;
    m->_01 = m->_02 = m->_03 = m->_23 = m->_10 = m->_20 = m->_13 = m->_30 = m->_31 = m->_32 = 0.0f;
    m->_33 = 1.0f;
}

// FUNCTION: BIONICLE 0x56e350
void NuMtxSetRotationY(NUMTX *m, NUANG a) {
    m->_00 = m->_22 = NU_COS_LUT(a);
    m->_20 = NU_SIN_LUT(a);
    m->_02 = -m->_20;
    m->_11 = 1.0f;
    m->_01 = m->_10 = m->_03 = m->_23 = m->_12 = m->_21 = m->_13 = m->_30 = m->_31 = m->_32 = 0.0f;
    m->_33 = 1.0f;
}

// FUNCTION: BIONICLE 0x56e3c0
void NuMtxSetRotationZ(NUMTX *m, NUANG a) {
    m->_00 = m->_11 = NU_COS_LUT(a);
    m->_01 = NU_SIN_LUT(a);
    m->_10 = -m->_01;
    m->_22 = 1.0;
    m->_02 = m->_12 = m->_03 = m->_23 = m->_20 = m->_21 = m->_13 = m->_30 = m->_31 = m->_32 = 0.0f;
    m->_33 = 1.0;
}

// FUNCTION: BIONICLE 0x56e430
void NuMtxTranslate(NUMTX *m, NUVEC *t) {
    m->_30 += t->x;
    m->_31 += t->y;
    m->_32 += t->z;
}

// FUNCTION: BIONICLE 0x56e460
void NuMtxPreTranslate(NUMTX *m, NUVEC *t) {
    m->_30 += t->x * m->_00 + t->y * m->_10 + t->z * m->_20;
    m->_31 += t->x * m->_01 + t->y * m->_11 + t->z * m->_21;
    m->_32 += t->x * m->_02 + t->y * m->_12 + t->z * m->_22;
}

// FUNCTION: BIONICLE 0x56e4c0
void NuMtxScale(NUMTX *m, NUVEC *s) {
    m->_00 *= s->x;
    m->_01 *= s->y;
    m->_02 *= s->z;
    m->_10 *= s->x;
    m->_11 *= s->y;
    m->_12 *= s->z;
    m->_20 *= s->x;
    m->_21 *= s->y;
    m->_22 *= s->z;
    m->_30 *= s->x;
    m->_31 *= s->y;
    m->_32 *= s->z;
}

// FUNCTION: BIONICLE 0x56e530
NUVEC NuMtxGetScale(NUMTX *m) {
    NUVEC scale;

    scale.x = NuFsqrt(m->_00 * m->_00 + m->_01 * m->_01 + m->_02 * m->_02);
    scale.y = NuFsqrt(m->_10 * m->_10 + m->_11 * m->_11 + m->_12 * m->_12);
    scale.z = NuFsqrt(m->_20 * m->_20 + m->_21 * m->_21 + m->_22 * m->_22);

    return scale;
}

// FUNCTION: BIONICLE 0x56e5e0
void NuMtxPreScale(NUMTX *m, NUVEC *s) {
    m->_00 *= s->x;
    m->_01 *= s->x;
    m->_02 *= s->x;
    m->_10 *= s->y;
    m->_11 *= s->y;
    m->_12 *= s->y;
    m->_20 *= s->z;
    m->_21 *= s->z;
    m->_22 *= s->z;
}

// FUNCTION: BIONICLE 0x56e640
void NuMtxRotateX(NUMTX *m, NUANG a) {
    float cosx = NU_COS_LUT(a);
    float sinx = NU_SIN_LUT(a);
    float m01 = m->_01;
    float m11 = m->_11;
    float m21 = m->_21;
    float m31 = m->_31;

    m->_01 = m01 * cosx - m->_02 * sinx;
    m->_02 = m01 * sinx + m->_02 * cosx;
    m->_11 = m11 * cosx - m->_12 * sinx;
    m->_12 = m11 * sinx + m->_12 * cosx;
    m->_21 = m21 * cosx - m->_22 * sinx;
    m->_22 = m21 * sinx + m->_22 * cosx;
    m->_31 = m31 * cosx - m->_32 * sinx;
    m->_32 = m31 * sinx + m->_32 * cosx;
}

// FUNCTION: BIONICLE 0x56e700
void NuMtxPreRotateX(NUMTX *m, NUANG a) {
    float cosx = NU_COS_LUT(a);
    float sinx = NU_SIN_LUT(a);
    float m10 = m->_10;
    float m11 = m->_11;
    float m12 = m->_12;

    m->_10 = cosx * m10 + m->_20 * sinx;
    m->_11 = cosx * m11 + m->_21 * sinx;
    m->_12 = cosx * m12 + m->_22 * sinx;
    m->_20 = m->_20 * cosx - sinx * m10;
    m->_21 = m->_21 * cosx - sinx * m11;
    m->_22 = m->_22 * cosx - sinx * m12;
}

// FUNCTION: BIONICLE 0x56e7a0
void NuMtxRotateY(NUMTX *m, NUANG a) {
    float cosx = NU_COS_LUT(a);
    float sinx = NU_SIN_LUT(a);
    float m00 = m->_00;
    float m10 = m->_10;
    float m20 = m->_20;
    float m30 = m->_30;

    m->_00 = m00 * cosx + m->_02 * sinx;
    m->_02 = m->_02 * cosx - m00 * sinx;
    m->_10 = m10 * cosx + m->_12 * sinx;
    m->_12 = m->_12 * cosx - m10 * sinx;
    m->_20 = m20 * cosx + m->_22 * sinx;
    m->_22 = m->_22 * cosx - m20 * sinx;
    m->_30 = m30 * cosx + m->_32 * sinx;
    m->_32 = m->_32 * cosx - m30 * sinx;
}

// FUNCTION: BIONICLE 0x56e860
void NuMtxPreRotateY(NUMTX *m, NUANG a) {
    float cosx = NU_COS_LUT(a);
    float sinx = NU_SIN_LUT(a);
    float m00 = m->_00;
    float m01 = m->_01;
    float m02 = m->_02;

    m->_00 = cosx * m00 - m->_20 * sinx;
    m->_01 = cosx * m01 - m->_21 * sinx;
    m->_02 = cosx * m02 - m->_22 * sinx;
    m->_20 = sinx * m00 + m->_20 * cosx;
    m->_21 = sinx * m01 + m->_21 * cosx;
    m->_22 = sinx * m02 + m->_22 * cosx;
}

// FUNCTION: BIONICLE 0x56e900
void NuMtxPreRotateY180(NUMTX *m) {
    m->_00 = -m->_00;
    m->_01 = -m->_01;
    m->_02 = -m->_02;
    m->_20 = -m->_20;
    m->_21 = -m->_21;
    m->_22 = -m->_22;
}

// FUNCTION: BIONICLE 0x56e940
void NuMtxRotateZ(NUMTX *m, NUANG a) {
    float cosx = NU_COS_LUT(a);
    float sinx = NU_SIN_LUT(a);
    float m00 = m->_00;
    float m10 = m->_10;
    float m20 = m->_20;
    float m30 = m->_30;

    m->_00 = m00 * cosx - m->_01 * sinx;
    m->_01 = m00 * sinx + m->_01 * cosx;
    m->_10 = m10 * cosx - m->_11 * sinx;
    m->_11 = m10 * sinx + m->_11 * cosx;
    m->_20 = m20 * cosx - m->_21 * sinx;
    m->_21 = m20 * sinx + m->_21 * cosx;
    m->_30 = m30 * cosx - m->_31 * sinx;
    m->_31 = m30 * sinx + m->_31 * cosx;
}

// FUNCTION: BIONICLE 0x56ea00
void NuMtxPreRotateZ(NUMTX *m, NUANG a) {
    float cosx = NU_COS_LUT(a);
    float sinx = NU_SIN_LUT(a);
    float m00 = m->_00;
    float m01 = m->_01;
    float m02 = m->_02;

    m->_00 = cosx * m00 + m->_10 * sinx;
    m->_01 = cosx * m01 + m->_11 * sinx;
    m->_02 = cosx * m02 + m->_12 * sinx;
    m->_10 = m->_10 * cosx - sinx * m00;
    m->_11 = m->_11 * cosx - sinx * m01;
    m->_12 = m->_12 * cosx - sinx * m02;
}

// FUNCTION: BIONICLE 0x56eaa0
void NuMtxSetRotateXYZ(NUMTX *m, NUANGVEC *a) {
    float cosx = NU_COS_LUT(a->x);
    float sinx = NU_SIN_LUT(a->x);
    float cosy = NU_COS_LUT(a->y);
    float siny = NU_SIN_LUT(a->y);
    float cosz = NU_COS_LUT(a->z);
    float sinz = NU_SIN_LUT(a->z);

    m->_00 = cosy * cosz;
    m->_01 = cosy * sinz;
    m->_02 = -siny;
    m->_03 = 0.0f;
    m->_10 = sinx * siny * cosz - cosx * sinz;
    m->_11 = sinx * siny * sinz + cosx * cosz;
    m->_12 = sinx * cosy;
    m->_13 = 0.0f;
    m->_20 = cosx * siny * cosz + sinx * sinz;
    m->_21 = cosx * siny * sinz - sinx * cosz;
    m->_22 = cosx * cosy;
    m->_23 = 0.0f;
    m->_30 = 0.0f;
    m->_31 = 0.0f;
    m->_32 = 0.0f;
    m->_33 = 1.0f;
}

// FUNCTION: BIONICLE 0x56ebb0
void NuMtxMul(NUMTX *m, NUMTX *m0, NUMTX *m1) {
    NUMTX gm;

    gm._00 = m0->_00 * m1->_00 + m0->_01 * m1->_10 + m0->_02 * m1->_20;
    gm._01 = m0->_00 * m1->_01 + m0->_01 * m1->_11 + m0->_02 * m1->_21;
    gm._02 = m0->_00 * m1->_02 + m0->_01 * m1->_12 + m0->_02 * m1->_22;
    gm._03 = 0.0f;
    gm._10 = m0->_10 * m1->_00 + m0->_11 * m1->_10 + m0->_12 * m1->_20;
    gm._11 = m0->_10 * m1->_01 + m0->_11 * m1->_11 + m0->_12 * m1->_21;
    gm._12 = m0->_10 * m1->_02 + m0->_11 * m1->_12 + m0->_12 * m1->_22;
    gm._13 = 0.0f;
    gm._20 = m0->_20 * m1->_00 + m0->_21 * m1->_10 + m0->_22 * m1->_20;
    gm._21 = m0->_20 * m1->_01 + m0->_21 * m1->_11 + m0->_22 * m1->_21;
    gm._22 = m0->_20 * m1->_02 + m0->_21 * m1->_12 + m0->_22 * m1->_22;
    gm._23 = 0.0f;
    gm._30 = m0->_30 * m1->_00 + m0->_31 * m1->_10 + m0->_32 * m1->_20 + m1->_30;
    gm._31 = m0->_30 * m1->_01 + m0->_31 * m1->_11 + m0->_32 * m1->_21 + m1->_31;
    gm._32 = m0->_30 * m1->_02 + m0->_31 * m1->_12 + m0->_32 * m1->_22 + m1->_32;
    gm._33 = 1.0f;
    *m = gm;
}

// FUNCTION: BIONICLE 0x56ed30
void NuMtxMulH(NUMTX *m, NUMTX *m0, NUMTX *m1) {
    NUMTX gm;

    gm._00 = m0->_00 * m1->_00 + m0->_01 * m1->_10 + m0->_02 * m1->_20 + m0->_03 * m1->_30;
    gm._01 = m0->_00 * m1->_01 + m0->_01 * m1->_11 + m0->_02 * m1->_21 + m0->_03 * m1->_31;
    gm._02 = m0->_00 * m1->_02 + m0->_01 * m1->_12 + m0->_02 * m1->_22 + m0->_03 * m1->_32;
    gm._03 = m0->_00 * m1->_03 + m0->_01 * m1->_13 + m0->_02 * m1->_23 + m0->_03 * m1->_33;
    gm._10 = m0->_10 * m1->_00 + m0->_11 * m1->_10 + m0->_12 * m1->_20 + m0->_13 * m1->_30;
    gm._11 = m0->_10 * m1->_01 + m0->_11 * m1->_11 + m0->_12 * m1->_21 + m0->_13 * m1->_31;
    gm._12 = m0->_10 * m1->_02 + m0->_11 * m1->_12 + m1->_22 * m0->_12 + m0->_13 * m1->_32;
    gm._13 = m0->_10 * m1->_03 + m0->_11 * m1->_13 + m0->_12 * m1->_23 + m0->_13 * m1->_33;
    gm._20 = m0->_20 * m1->_00 + m0->_21 * m1->_10 + m0->_22 * m1->_20 + m0->_23 * m1->_30;
    gm._21 = m0->_20 * m1->_01 + m0->_21 * m1->_11 + m0->_22 * m1->_21 + m0->_23 * m1->_31;
    gm._22 = m0->_20 * m1->_02 + m0->_21 * m1->_12 + m0->_22 * m1->_22 + m0->_23 * m1->_32;
    gm._23 = m0->_20 * m1->_03 + m0->_21 * m1->_13 + m0->_22 * m1->_23 + m0->_23 * m1->_33;
    gm._30 = m0->_30 * m1->_00 + m0->_31 * m1->_10 + m0->_32 * m1->_20 + m0->_33 * m1->_30;
    gm._31 = m0->_30 * m1->_01 + m0->_31 * m1->_11 + m0->_32 * m1->_21 + m0->_33 * m1->_31;
    gm._32 = m0->_30 * m1->_02 + m0->_31 * m1->_12 + m0->_32 * m1->_22 + m0->_33 * m1->_32;
    gm._33 = m0->_30 * m1->_03 + m0->_31 * m1->_13 + m0->_32 * m1->_23 + m0->_33 * m1->_33;
    *m = gm;
}

// FUNCTION: BIONICLE 0x56ef70
void NuMtxMulR(NUMTX *m, NUMTX *m0, NUMTX *m1) {
    NUMTX gm;

    gm._00 = m0->_00 * m1->_00 + m0->_01 * m1->_10 + m0->_02 * m1->_20;
    gm._01 = m0->_00 * m1->_01 + m0->_01 * m1->_11 + m0->_02 * m1->_21;
    gm._02 = m0->_00 * m1->_02 + m0->_01 * m1->_12 + m0->_02 * m1->_22;
    gm._03 = 0.0;
    gm._10 = m0->_10 * m1->_00 + m0->_11 * m1->_10 + m0->_12 * m1->_20;
    gm._11 = m0->_10 * m1->_01 + m0->_11 * m1->_11 + m0->_12 * m1->_21;
    gm._12 = m0->_10 * m1->_02 + m0->_11 * m1->_12 + m0->_12 * m1->_22;
    gm._13 = 0.0;
    gm._20 = m0->_20 * m1->_00 + m0->_21 * m1->_10 + m0->_22 * m1->_20;
    gm._21 = m0->_20 * m1->_01 + m0->_21 * m1->_11 + m0->_22 * m1->_21;
    gm._22 = m0->_20 * m1->_02 + m0->_21 * m1->_12 + m0->_22 * m1->_22;
    gm._23 = 0.0;
    gm._30 = gm._31 = gm._32 = 0.0f;
    gm._33 = 1.0;
    *m = gm;
}

// FUNCTION: BIONICLE 0x56f0b0
void NuMtxTransposeR(NUMTX *m, NUMTX *m0) {
    float t;

    t = m0->_01;
    m->_01 = m0->_10;
    m->_10 = t;
    t = m0->_02;
    m->_02 = m0->_20;
    m->_20 = t;
    t = m0->_12;
    m->_12 = m0->_21;
    m->_21 = t;
    m->_00 = m0->_00;
    m->_11 = m0->_11;
    m->_22 = m0->_22;
    m->_30 = m0->_30;
    m->_31 = m0->_31;
    m->_32 = m0->_32;
    m->_33 = m0->_33;
}

// FUNCTION: BIONICLE 0x56f110
void NuMtxTranspose(NUMTX *m, NUMTX *m0) {
    float t;

    t = m0->_01;
    m->_01 = m0->_10;
    m->_10 = t;
    t = m0->_02;
    m->_02 = m0->_20;
    m->_20 = t;
    t = m0->_03;
    m->_03 = m0->_30;
    m->_30 = t;
    t = m0->_12;
    m->_12 = m0->_21;
    m->_21 = t;
    t = m0->_13;
    m->_13 = m0->_31;
    m->_31 = t;
    t = m0->_23;
    m->_23 = m0->_32;
    m->_32 = t;
    m->_00 = m0->_00;
    m->_11 = m0->_11;
    m->_22 = m0->_22;
    m->_33 = m0->_33;
}

// FUNCTION: BIONICLE 0x56f180
void NuMtxInv(NUMTX *m, NUMTX *m0) {
    float t;

    float tx = -m0->_30;
    float ty = -m0->_31;
    float tz = -m0->_32;

    t = m0->_01;
    m->_01 = m0->_10;
    m->_10 = t;
    t = m0->_02;
    m->_02 = m0->_20;
    m->_20 = t;
    t = m0->_12;
    m->_12 = m0->_21;
    m->_21 = t;
    m->_00 = m0->_00;
    m->_11 = m0->_11;
    m->_22 = m0->_22;
    m->_30 = m->_00 * tx + m->_10 * ty + m->_20 * tz;
    m->_31 = m->_01 * tx + m->_11 * ty + m->_21 * tz;
    m->_32 = m->_02 * tx + m->_12 * ty + m->_22 * tz;
    m->_23 = 0.0f;
    m->_13 = m->_23;
    m->_03 = m->_13;
    m->_33 = 1.0f;
}

// FUNCTION: BIONICLE 0x56f220
void NuMtxInvR(NUMTX *m, NUMTX *m0) {
    float t;

    t = m0->_01;
    m->_01 = m0->_10;
    m->_10 = t;
    t = m0->_02;
    m->_02 = m0->_20;
    m->_20 = t;
    t = m0->_12;
    m->_12 = m0->_21;
    m->_21 = t;
    m->_00 = m0->_00;
    m->_11 = m0->_11;
    m->_22 = m0->_22;
    m->_23 = 0.0f;
    m->_13 = m->_23;
    m->_03 = m->_13;
    m->_32 = 0.0f;
    m->_31 = m->_32;
    m->_30 = m->_31;
    m->_33 = 1.0f;
}

// FUNCTION: BIONICLE 0x56f280
void NuMtxInvRSS(NUMTX *inv, NUMTX *T) {
    NUMTX gm;
    float det = T->_00 * (T->_11 * T->_22 - T->_12 * T->_21) - T->_01 * (T->_10 * T->_22 - T->_12 * T->_20) +
                T->_02 * (T->_10 * T->_21 - T->_11 * T->_20);
    float invdet = 1.0f / det;

    gm._00 = (T->_11 * T->_22 - T->_12 * T->_21) * invdet;
    gm._10 = -((T->_10 * T->_22 - T->_12 * T->_20) * invdet);
    gm._20 = (T->_10 * T->_21 - T->_11 * T->_20) * invdet;
    gm._01 = -((T->_01 * T->_22 - T->_02 * T->_21) * invdet);
    gm._11 = (T->_00 * T->_22 - T->_02 * T->_20) * invdet;
    gm._21 = -((T->_00 * T->_21 - T->_01 * T->_20) * invdet);
    gm._02 = (T->_01 * T->_12 - T->_02 * T->_11) * invdet;
    gm._12 = -((T->_00 * T->_12 - T->_02 * T->_10) * invdet);
    gm._22 = (T->_00 * T->_11 - T->_01 * T->_10) * invdet;
    gm._03 = 0.0f;
    gm._13 = 0.0f;
    gm._23 = 0.0f;
    gm._33 = 1.0f;
    gm._30 = 0.0f;
    gm._31 = 0.0f;
    gm._32 = 0.0f;
    *inv = gm;
}

// FUNCTION: BIONICLE 0x56f3d0
void NuMtxInvH(NUMTX *mi, NUMTX *m0) {
    // there's some weird stack alignment stuff going on here...
    // also not sure if this semantically matches the original implementation
    int p[4];
    int j;
    int k;
    int i;
    float a[4][4];
    int n;
    float local_1c;
    float localm20;
    float localm28;
    float localm30;

    n = 4;

    a[0][0] = m0->_00;
    a[0][1] = m0->_01;
    a[0][2] = m0->_02;
    a[0][3] = m0->_03;
    a[1][0] = m0->_10;
    a[1][1] = m0->_11;
    a[1][2] = m0->_12;
    a[1][3] = m0->_13;
    a[2][0] = m0->_20;
    a[2][1] = m0->_21;
    a[2][2] = m0->_22;
    a[2][3] = m0->_23;
    a[3][0] = m0->_30;
    a[3][1] = m0->_31;
    a[3][2] = m0->_32;
    a[3][3] = m0->_33;

    for (i = 0; i < n; i++) {
        localm20 = 0.0f;
        p[i] = 0;

        for (j = i; j < n; j++) {
            local_1c = 0.0f;

            for (k = i; k < n; k++) {
                local_1c += NuFabs(a[j][k]);
            }

            localm28 = NuFdiv(NuFabs(a[j][i]), local_1c);

            if (localm20 < localm28) {
                localm20 = localm28;
                p[i] = j;
            }
        }

        if (localm20 == 0.0f) {
            return;
        }

        if (p[i] != i) {
            for (k = 0; k < n; k++) {
                float f;

                f = a[i][k];
                a[i][k] = a[p[i]][k];
                a[p[i]][k] = f;
            }
        }

        localm30 = a[i][i];

        for (k = 0; k < n; k++) {
            if (k != i) {
                a[i][k] = -a[i][k] / localm30;
                for (j = 0; j < n; j++) {
                    if (j != i) {
                        a[j][k] += a[j][i] * a[i][k];
                    }
                }
            }
        }

        for (j = 0; j < n; j++) {
            a[j][i] = a[j][i] / localm30;
        }

        a[i][i] = 1.0f / localm30;
    }

    for (i = n - 1; i >= 0; i--) {
        if (p[i] != i) {
            for (j = 0; j < n; j++) {
                float f;

                f = a[j][i];
                a[j][i] = a[j][p[i]];
                a[j][p[i]] = f;
            }
        }
    }

    mi->_00 = a[0][0];
    mi->_01 = a[0][1];
    mi->_02 = a[0][2];
    mi->_03 = a[0][3];
    mi->_10 = a[1][0];
    mi->_11 = a[1][1];
    mi->_12 = a[1][2];
    mi->_13 = a[1][3];
    mi->_20 = a[2][0];
    mi->_21 = a[2][1];
    mi->_22 = a[2][2];
    mi->_23 = a[2][3];
    mi->_30 = a[3][0];
    mi->_31 = a[3][1];
    mi->_32 = a[3][2];
    mi->_33 = a[3][3];
}

// FUNCTION: BIONICLE 0x56f7c0
void NuMtxAlignY(NUMTX *m, NUVEC *v) {
    float s;

    m->_10 = v->x;
    m->_11 = v->y;
    m->_12 = v->z;
    m->_00 = m->_11 * m->_22 - m->_12 * m->_21;
    m->_01 = m->_12 * m->_20 - m->_10 * m->_22;
    m->_02 = m->_10 * m->_21 - m->_11 * m->_20;

    s = NuFsqrt(m->_00 * m->_00 + m->_01 * m->_01 + m->_02 * m->_02);
    s = NuFdiv(1.0f, s);

    m->_00 = m->_00 * s;
    m->_01 = m->_01 * s;
    m->_02 = m->_02 * s;
    m->_20 = m->_01 * m->_12 - m->_02 * m->_11;
    m->_21 = m->_02 * m->_10 - m->_00 * m->_12;
    m->_22 = m->_00 * m->_11 - m->_01 * m->_10;
}

// FUNCTION: BIONICLE 0x56f8a0
void NuMtxAlignZ(NUMTX *m, NUVEC *v) {
    float xLenSq = m->_00 * m->_00 + m->_01 * m->_01 + m->_02 * m->_02;
    float yLenSq = m->_10 * m->_10 + m->_11 * m->_11 + m->_12 * m->_12;
    float zLenSq = m->_20 * m->_20 + m->_21 * m->_21 + m->_22 * m->_22;

    float len = NuFsqrt(NuFdiv(zLenSq, v->x * v->x + v->y * v->y + v->z * v->z));
    m->_20 = v->x * len;
    m->_21 = v->y * len;
    m->_22 = v->z * len;

    if (NuFabs(NuVecDot(NUMTX_GET_ROW_VEC(m, 1), NUMTX_GET_ROW_VEC(m, 2))) > 0.86602540378f /* cos(30 deg) */) {
        NuVecCross(NUMTX_GET_ROW_VEC(m, 1), NUMTX_GET_ROW_VEC(m, 2), NUMTX_GET_ROW_VEC(m, 0));
        len = NuFsqrt(NuFdiv(yLenSq, m->_10 * m->_10 + m->_11 * m->_11 + m->_12 * m->_12));
        m->_10 = m->_10 * len;
        m->_11 = m->_11 * len;
        m->_12 = m->_12 * len;
        NuVecCross(NUMTX_GET_ROW_VEC(m, 0), NUMTX_GET_ROW_VEC(m, 1), NUMTX_GET_ROW_VEC(m, 2));
        len = NuFsqrt(NuFdiv(xLenSq, m->_00 * m->_00 + m->_01 * m->_01 + m->_02 * m->_02));
        m->_00 = m->_00 * len;
        m->_01 = m->_01 * len;
        m->_02 = m->_02 * len;
    } else {
        NuVecCross(NUMTX_GET_ROW_VEC(m, 0), NUMTX_GET_ROW_VEC(m, 1), NUMTX_GET_ROW_VEC(m, 2));
        len = NuFsqrt(NuFdiv(xLenSq, m->_00 * m->_00 + m->_01 * m->_01 + m->_02 * m->_02));
        m->_00 = m->_00 * len;
        m->_01 = m->_01 * len;
        m->_02 = m->_02 * len;
        NuVecCross(NUMTX_GET_ROW_VEC(m, 1), NUMTX_GET_ROW_VEC(m, 2), NUMTX_GET_ROW_VEC(m, 0));
        len = NuFsqrt(NuFdiv(yLenSq, m->_10 * m->_10 + m->_11 * m->_11 + m->_12 * m->_12));
        m->_10 = m->_10 * len;
        m->_11 = m->_11 * len;
        m->_12 = m->_12 * len;
    }
}

// FUNCTION: BIONICLE 0x56fc00
void NuMtxLookAtY(NUMTX *m, NUVEC *pnt) {
    NUVEC v;

    v.x = pnt->x - m->_30;
    v.y = pnt->y - m->_31;
    v.z = pnt->z - m->_32;

    NuVecNorm(&v, &v);
    NuMtxAlignY(m, &v);
}

// FUNCTION: BIONICLE 0x56fc50
void NuMtxLookAtZ(NUMTX *m, NUVEC *pnt) {
    NUVEC v;

    v.x = pnt->x - m->_30;
    v.y = pnt->y - m->_31;
    v.z = pnt->z - m->_32;

    NuVecNorm(&v, &v);
    NuMtxAlignZ(m, &v);
}

// FUNCTION: BIONICLE 0x56fca0
void NuMtxGetTranslation(NUMTX *m, NUVEC *t) {
    t->x = m->_30;
    t->y = m->_31;
    t->z = m->_32;
}

// FUNCTION: BIONICLE 0x56fcc0
float NuMtxDet3(NUMTX *m) {
    return m->_00 * (m->_11 * m->_22 - m->_12 * m->_21) - m->_01 * (m->_10 * m->_22 - m->_12 * m->_20) +
           m->_02 * (m->_10 * m->_21 - m->_11 * m->_20);
}

// FUNCTION: BIONICLE 0x56fd00
void NuMtxGetPerspectiveD3D(NUMTX *mtx, float *fovy, float *aspect, float *zNear, float *zFar) {
    float Q = mtx->_22;
    *zNear = -mtx->_32 / Q;
    *zFar = (*zNear * Q) / (Q - 1.0f);
    *aspect = mtx->_11 / mtx->_00;
    *fovy = NuAtan2(1.0f / mtx->_11, 1.0f) * 360.0f / (float)3.14159265358979323846;
}

