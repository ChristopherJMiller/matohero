#ifndef NUVEC_H
#define NUVEC_H

#include "numath.h"

extern NUVEC nuvec_one;
extern NUVEC nuvec_minus_one;

float NuVecMtxTransformH(NUVEC *v, NUVEC *v0, NUMTX *m0);
void NuVecMtxRotate(NUVEC *v, NUVEC *v0, NUMTX *m0);
void NuVecInvMtxTransform(NUVEC *v, NUVEC *v0, NUMTX *m0);
void NuVecInvMtxRotate(NUVEC *v, NUVEC *v0, NUMTX *m0);
void NuVecRotateX(NUVEC *v, NUVEC *v0, NUANG a);
void NuVecRotateY(NUVEC *v, NUVEC *v0, NUANG a);
void NuVecRotateZ(NUVEC *v, NUVEC *v0, NUANG a);
void NuVecAdd(NUVEC *v, NUVEC *v0, NUVEC *v1);
void NuVecSub(NUVEC *v, NUVEC *v0, NUVEC *v1);
void NuVecScale(NUVEC *v, NUVEC *v0, float k);
void NuVecAddScale(NUVEC *v, NUVEC *v0, NUVEC *v1, float k);
void NuVecCross(NUVEC *v, NUVEC *v0, NUVEC *v1);
float NuVecDot(NUVEC *v0, NUVEC *v1);
void NuVecMax(NUVEC *v0, NUVEC *v1, NUVEC *v2);
void NuVecMin(NUVEC *v0, NUVEC *v1, NUVEC *v2);
float NuVecMagSqr(NUVEC *v0);
float NuVecNorm(NUVEC *v, NUVEC *v0);
float NuVecDistSqr(NUVEC *v0, NUVEC *v1, NUVEC *d);
float NuVecXZDistSqr(NUVEC *v0, NUVEC *v1, NUVEC *d);
void NuVecLerp(NUVEC *vt, NUVEC *v1, NUVEC *v0, float t);
float NuVecMag(NUVEC *v0);
float NuVecDist(NUVEC *v0, NUVEC *v1, NUVEC *d);
float NuVecXZDist(NUVEC *v0, NUVEC *v1, NUVEC *d);
int NuLineLineIntersect(NUVEC *p0, NUVEC *v0, NUVEC *p1, NUVEC *v1, float *s, float *t);

#endif
