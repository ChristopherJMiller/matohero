#ifndef NUQUAT_H
#define NUQUAT_H

#include "numath.h"

typedef struct nuquat_s {
    float x;
    float y;
    float z;
    float w;
} NUQUAT;

void NuMtxToQuat(NUMTX *m, NUQUAT *q);
void NuQuatToMtx(NUQUAT *q, NUMTX *m);
void NuQuatNormalise(NUQUAT *q, NUQUAT *q0);
void NuQuatSlerp(NUQUAT *q, NUQUAT *from, NUQUAT *to, float t);

#endif
