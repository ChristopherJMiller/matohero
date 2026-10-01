#include <windows.h>

#include "nuthread.h"

/* PC threading layer (PS2: nucore/ps2/nuthread.c). The file name is ours. */

// GLOBAL: BIONICLE 0x6174cc
nuthreaddisableswapfn *NuThreadDisableThreadSwap;

// GLOBAL: BIONICLE 0x6174d0
nuthreaddisableswapfn *NuThreadEnableThreadSwap;

// GLOBAL: BIONICLE 0x3517ef0
unsigned int nu_current_thread_id;

/* Critical sections: a fixed table of Win32 CRITICAL_SECTIONs, handed out in order with
 * IDs from 1 (indexed as `[id - 1]`). Never destroyed. */

// GLOBAL: BIONICLE 0x34c5fd0
static CRITICAL_SECTION criticalsections[8];

// GLOBAL: BIONICLE 0x34c6090
static volatile int numcriticalsections;

// FUNCTION: BIONICLE 0x56cec0
int NuThreadCreateCriticalSection(void) {
    if (numcriticalsections <= 8) {
        numcriticalsections++;
        InitializeCriticalSection(&criticalsections[numcriticalsections - 1]);
        return numcriticalsections;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x56cf00
void NuThreadCriticalSectionBegin(int SectionID) {
    if (SectionID > 0 && SectionID <= 8) {
        EnterCriticalSection(&criticalsections[SectionID - 1]);
    }
}

// FUNCTION: BIONICLE 0x56cf20
void NuThreadCriticalSectionEnd(int SectionID) {
    if (SectionID > 0 && SectionID <= 8) {
        LeaveCriticalSection(&criticalsections[SectionID - 1]);
    }
}
