#ifndef NUFLOAT_H
#define NUFLOAT_H

#include <math.h>

float NuFsqrt(float f);
float NuFsign(float f);
int NuEquiv(float f1, float f2);
void NuEquivTollerance(float f);
float NuFloor(float f);
float NuFmod(float a, float b);

/* Clears the sign bit in memory rather than using fabs (NuEquiv, NuMtxAlignZ). */
static __inline float NuFabs(float f) {
    *(int *)&f &= 0x7fffffff;
    return f;
}

/* Division that yields 0 when either operand is 0 (both are tested, see NuQuatSlerp). */
static __inline float NuFdiv(float a, float b) {
    if (a != 0.0f && b != 0.0f) {
        return a / b;
    }
    return 0.0f;
}

#endif
