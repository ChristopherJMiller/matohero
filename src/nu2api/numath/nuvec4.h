#ifndef NUVEC4_H
#define NUVEC4_H

#include "numath.h"

void NuVec4Scale(NUVEC4 *v, NUVEC4 *v0, float k);
void NuVec4MtxTransform(NUVEC4 *v, NUVEC *v0, NUMTX *m0);
void NuVec4MtxTransformH(NUVEC4 *v, NUVEC4 *v0, NUMTX *m0);

#endif
