#include <windows.h>

#include "nutime.h"

/* PC tick counter: QueryPerformanceCounter, read with the process pinned to the first
 * CPU (multi-core counters weren't guaranteed to agree in 2006). Names follow later Nu2
 * engines (opensagadev/saga); the file name and statics are ours. */

// GLOBAL: BIONICLE 0x34c5fc0
static LARGE_INTEGER pc_frequency;

// GLOBAL: BIONICLE 0x34c5fc8
static LARGE_INTEGER pc_base;

// FUNCTION: BIONICLE 0x56c820
unsigned int NuTimeGetTime(void) {
    NUTIME t;

    NuTimeGet(&t);
    return (unsigned int)(t.low / 1000i64);
}

// FUNCTION: BIONICLE 0x56c850
void NuTimeGetTicksPS(unsigned int *low, unsigned int *high) {
    DWORD_PTR process_mask = 1;
    DWORD_PTR system_mask = 1;
    LARGE_INTEGER now;
    __int64 ticks;

    if (!GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask)) {
        process_mask = 1;
    }
    SetProcessAffinityMask(GetCurrentProcess(), 1);
    QueryPerformanceCounter(&now);
    SetProcessAffinityMask(GetCurrentProcess(), process_mask);
    if (now.QuadPart > pc_base.QuadPart) {
        now.QuadPart -= pc_base.QuadPart;
    } else {
        now.QuadPart += 0x7fffffffffffffffi64 - pc_base.QuadPart;
    }
    ticks = now.QuadPart;
    *low = (unsigned int)ticks;
    *high = (unsigned int)(ticks >> 32);
}

// FUNCTION: BIONICLE 0x56c8f0
void NuTimeGetTicksPerSecondPS(unsigned int *low, unsigned int *high) {
    *low = (unsigned int)pc_frequency.QuadPart;
    *high = (unsigned int)(pc_frequency.QuadPart >> 32);
}

// FUNCTION: BIONICLE 0x56c910
void NuTimeInitPS(void) {
    DWORD_PTR process_mask = 1;
    DWORD_PTR system_mask = 1;

    if (!GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask)) {
        process_mask = 1;
    }
    SetProcessAffinityMask(GetCurrentProcess(), 1);
    QueryPerformanceCounter(&pc_base);
    QueryPerformanceFrequency(&pc_frequency);
    SetProcessAffinityMask(GetCurrentProcess(), process_mask);
}
