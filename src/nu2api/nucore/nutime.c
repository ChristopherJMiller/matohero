#include "nutime.h"

#include "nuapi.h"

/* Frame timing on top of the platform tick counter. Based on opensagadev/saga
 * src/nu2api/nucore/nutime.cpp (GPL-3.0); names from the PS2 prototype. */

// FUNCTION: BIONICLE 0x56cf40
float NuTimeGetFrameTime(void) {
    return nuapi.last_frame_time;
}

// FUNCTION: BIONICLE 0x56cf50
void NuTimeGet(NUTIME *t) {
    NuTimeGetTicksPS(&t->low, (unsigned int *)&t->high);
}

// FUNCTION: BIONICLE 0x56cf70
void NuTimeSub(NUTIME *t, NUTIME *a, NUTIME *b) {
    if (a->low >= b->low) {
        t->low = a->low - b->low;
        t->high = a->high - b->high;
    } else {
        t->low = a->low - b->low;
        t->high = a->high - b->high - 1;
    }
}

// FUNCTION: BIONICLE 0x56cfb0
float NuTimeSeconds(NUTIME *t) {
    unsigned int low;
    unsigned int high;
    double ticks;
    double ticks_per_second;

    NuTimeGetTicksPerSecondPS(&low, &high);
    ticks = t->high * 4294967295.0 + t->low;
    ticks_per_second = high * 4294967295.0 + low;
    return ticks / ticks_per_second;
}
