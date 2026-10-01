#include "nufloat.h"

/* Float helpers. Based on opensagadev/saga src/nu2api/numath/nufloat.c (GPL-3.0);
 * names and locals from the PS2 prototype. NuFnabs, NuFneg and the NuFpException
 * functions aren't in the PC exe. */

// GLOBAL: BIONICLE 0x6179b8
static float fetol = 0.01f;

// FUNCTION: BIONICLE 0x571430
float NuFsign(float f) {
    if (*(unsigned int *)&f & 0x80000000) {
        return -1.0f;
    } else {
        return 1.0f;
    }
}

// FUNCTION: BIONICLE 0x571450
int NuEquiv(float f1, float f2) {
    float result = NuFabs(f1 - f2);
    return result < fetol;
}

// FUNCTION: BIONICLE 0x571490
void NuEquivTollerance(float f) {
    fetol = f;
}

// FUNCTION: BIONICLE 0x5714a0
float NuFloor(float f) {
    int fi;
    float t;

    fi = (int)f;
    t = (float)fi;
    if (t > f) {
        t -= 1.0f;
    }
    return t;
}

// FUNCTION: BIONICLE 0x5714d0
float NuFmod(float a, float b) {
    return a - (int)(a / b) * b;
}
