#ifndef NUFLOAT_H
#define NUFLOAT_H

#include <math.h>

float NuFsqrt(float f);

static __inline float NuFabs(float f) {
    return (float)fabs(f);
}

/* Division that yields 0 when either operand is 0 (both are tested, see NuQuatSlerp). */
static __inline float NuFdiv(float a, float b) {
    if (a != 0.0f && b != 0.0f) {
        return a / b;
    }
    return 0.0f;
}

#endif
