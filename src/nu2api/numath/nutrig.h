#ifndef NUTRIG_H
#define NUTRIG_H

#include "numath.h"

/* Sine over a full turn at NUANG/2 resolution; cosine reads a quarter turn ahead. */
extern float NuTrigTable[32768];

#define NU_SIN_LUT(a) NuTrigTable[((a) >> 1) & 0x7fff]
#define NU_COS_LUT(a) NuTrigTable[(((a) + 0x4000) >> 1) & 0x7fff]

#endif
