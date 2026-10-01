#ifndef NUTIME_H
#define NUTIME_H

typedef struct nutime_s {
    unsigned int low;
    int high;
} NUTIME;

typedef NUTIME NUTIME_T;

float NuTimeGetFrameTime(void);
void NuTimeGet(NUTIME *t);
void NuTimeSub(NUTIME *t, NUTIME *a, NUTIME *b);
float NuTimeSeconds(NUTIME *t);

/* Platform layer (pc/nutime_pc.c). */
void NuTimeInitPS(void);
void NuTimeGetTicksPS(unsigned int *low, unsigned int *high);
void NuTimeGetTicksPerSecondPS(unsigned int *low, unsigned int *high);

#endif
