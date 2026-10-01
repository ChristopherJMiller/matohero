#include "nurand.h"

#include "nutypes.h"

/* Random numbers: a 32-bit LCG (fseed) for floats/ints, Park-Miller "minimal standard"
 * (Numerical Recipes ran0, masked) for NURAND states, and a 16-bit LCG for wind.
 * Based on opensagadev/saga src/nu2api/numath/nurand.cpp (GPL-3.0); original names
 * from the PS2 prototype. Functions are in PC order; NuRandGetSeed isn't in the PC exe. */

#define IA 16807
#define IM 2147483647
#define IQ 127773
#define IR 2836
#define MASK 123459876

// GLOBAL: BIONICLE 0x34c6098
unsigned int fseed;

// GLOBAL: BIONICLE 0x6174d8
int NuWindQS = 4965;

// GLOBAL: BIONICLE 0x6174dc
static NURAND global_rand = {1};

// FUNCTION: BIONICLE 0x56e0e0
void NuRandSeed(unsigned int seed) {
    fseed = seed;
}

// FUNCTION: BIONICLE 0x56e0f0
float NuRandFloatSeeded(unsigned int *seed) {
    union {
        unsigned int i;
        float f;
    } value;

    *seed = *seed * 0x19660d + 0x3c6ef35f;
    value.i = *seed & 0x7fffff | 0x3f800000;
    return value.f - 1.0f;
}

// FUNCTION: BIONICLE 0x56e120
float NuRandFloat(void) {
    return NuRandFloatSeeded(&fseed);
}

// FUNCTION: BIONICLE 0x56e150
unsigned int NuRandIntSeeded(unsigned int *seed) {
    *seed = *seed * 0x19660d + 0x3c6ef35f;
    return *seed;
}

// FUNCTION: BIONICLE 0x56e170
unsigned int NuRandInt(void) {
    return NuRandIntSeeded(&fseed);
}

// FUNCTION: BIONICLE 0x56e190
int NuWindRand(void) {
    NuWindQS = (NuWindQS * 9421 + 1) & 0xffff;
    return NuWindQS;
}

// FUNCTION: BIONICLE 0x56e1b0
void NuRandSetSeed(NURAND *nrand, int seed) {
    if (nrand == NULL) {
        nrand = &global_rand;
    }
    nrand->idum = seed;
}

// FUNCTION: BIONICLE 0x56e1d0
int NuRand(NURAND *nrand) {
    int k;

    if (nrand != NULL) {
        if (nrand->idum == 0) {
            nrand->idum = 1;
        }
    } else {
        nrand = &global_rand;
    }

    nrand->idum ^= MASK;
    k = nrand->idum / IQ;
    nrand->idum = IA * (nrand->idum - k * IQ) - IR * k;
    if (nrand->idum < 0) {
        nrand->idum += IM;
    }
    nrand->idum ^= MASK;
    return nrand->idum;
}

// FUNCTION: BIONICLE 0x56e230
float NuFloatRand(NURAND *nrand) {
    return NuRand(nrand) / 2147483648.0f;
}
