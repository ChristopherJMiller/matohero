#ifndef NUFLOAT_H
#define NUFLOAT_H

float NuFsqrt(float f);

/* Division that yields 0 for a zero divisor. */
static __inline float NuFdiv(float a, float b) {
    if (b != 0.0f) {
        return a / b;
    }
    return 0.0f;
}

#endif
