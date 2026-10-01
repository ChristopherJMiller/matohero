#ifndef NUTRIG_H
#define NUTRIG_H

#include "numath.h"

/* Sine over a full turn at NUANG/2 resolution; cosine reads a quarter turn ahead. */
extern float NuTrigTable[32768];

#define NU_SIN_LUT(a) NuTrigTable[((a) >> 1) & 0x7fff]
#define NU_COS_LUT(a) NuTrigTable[(((a) + 0x4000) >> 1) & 0x7fff]

void NuTrigInit(void);
int NuAtan2D(float dx, float dy);
NUANG NuAngAdd(NUANG ang1, NUANG ang2);
NUANG NuAngSub(NUANG ang1, NUANG ang2);
int NuPower2(int v);
short NuASin(float Sin);
short NuACos(float Cos);
float NuPow(float x, float y);
float NuLog10(float x);
float NuExp10(float x);
float NuAtan2(float dx, float dy);
float NuPowFast(float x, float y);

#endif
