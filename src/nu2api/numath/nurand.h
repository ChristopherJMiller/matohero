#ifndef NURAND_H
#define NURAND_H

typedef struct nunrand_s {
    int idum;
} NURAND;

extern unsigned int fseed;
extern int NuWindQS;

void NuRandSeed(unsigned int seed);
float NuRandFloatSeeded(unsigned int *seed);
float NuRandFloat(void);
unsigned int NuRandIntSeeded(unsigned int *seed);
unsigned int NuRandInt(void);
int NuWindRand(void);
void NuRandSetSeed(NURAND *nrand, int seed);
int NuRand(NURAND *nrand);
float NuFloatRand(NURAND *nrand);

#endif
