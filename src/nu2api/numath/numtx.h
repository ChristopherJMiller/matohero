#ifndef NUMTX_H
#define NUMTX_H

#include "numath.h"

typedef struct nuangvec_s {
    NUANG x;
    NUANG y;
    NUANG z;
} NUANGVEC;

/* Row r of a matrix as a vector (its first three elements). */
#define NUMTX_GET_ROW_VEC(mtx, row) ((NUVEC *)&(mtx)->_##row##0)

extern NUMTX numtx_identity;
extern NUMTX numtx_zero;

void NuMtxSetZero(NUMTX *m);
void NuMtxSetIdentity(NUMTX *m);
void NuMtxSetTranslation(NUMTX *m, NUVEC *t);
void NuMtxSetRotationX(NUMTX *m, NUANG a);
void NuMtxSetRotationY(NUMTX *m, NUANG a);
void NuMtxSetRotationZ(NUMTX *m, NUANG a);
void NuMtxTranslate(NUMTX *m, NUVEC *t);
void NuMtxPreTranslate(NUMTX *m, NUVEC *t);
void NuMtxScale(NUMTX *m, NUVEC *s);
NUVEC NuMtxGetScale(NUMTX *m);
void NuMtxPreScale(NUMTX *m, NUVEC *s);
void NuMtxRotateX(NUMTX *m, NUANG a);
void NuMtxPreRotateX(NUMTX *m, NUANG a);
void NuMtxRotateY(NUMTX *m, NUANG a);
void NuMtxPreRotateY(NUMTX *m, NUANG a);
void NuMtxPreRotateY180(NUMTX *m);
void NuMtxRotateZ(NUMTX *m, NUANG a);
void NuMtxPreRotateZ(NUMTX *m, NUANG a);
void NuMtxSetRotateXYZ(NUMTX *m, NUANGVEC *a);
void NuMtxMul(NUMTX *m, NUMTX *m0, NUMTX *m1);
void NuMtxMulH(NUMTX *m, NUMTX *m0, NUMTX *m1);
void NuMtxMulR(NUMTX *m, NUMTX *m0, NUMTX *m1);
void NuMtxTransposeR(NUMTX *m, NUMTX *m0);
void NuMtxTranspose(NUMTX *m, NUMTX *m0);
void NuMtxInv(NUMTX *m, NUMTX *m0);
void NuMtxInvR(NUMTX *m, NUMTX *m0);
void NuMtxInvRSS(NUMTX *inv, NUMTX *T);
void NuMtxInvH(NUMTX *mi, NUMTX *m0);
void NuMtxAlignY(NUMTX *m, NUVEC *v);
void NuMtxAlignZ(NUMTX *m, NUVEC *v);
void NuMtxLookAtY(NUMTX *m, NUVEC *pnt);
void NuMtxLookAtZ(NUMTX *m, NUVEC *pnt);
void NuMtxGetTranslation(NUMTX *m, NUVEC *t);
float NuMtxDet3(NUMTX *m);
void NuMtxGetPerspectiveD3D(NUMTX *mtx, float *fovy, float *aspect, float *zNear, float *zFar);

#endif
