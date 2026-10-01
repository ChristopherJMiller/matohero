#include "nuthread.h"

/* Placeholder home for the thread globals until their original file is identified. */

// GLOBAL: BIONICLE 0x6174cc
nuthreaddisableswapfn *NuThreadDisableThreadSwap;

// GLOBAL: BIONICLE 0x6174d0
nuthreaddisableswapfn *NuThreadEnableThreadSwap;

// GLOBAL: BIONICLE 0x3517ef0
unsigned int nu_current_thread_id;

/* Critical sections over a table of Win32 CRITICAL_SECTIONs; not decompiled yet. */

// STUB: BIONICLE 0x56cec0
int NuThreadCreateCriticalSection(void) {
    return 0;
}

// STUB: BIONICLE 0x56cf00
void NuThreadCriticalSectionBegin(int SectionID) {
}

// STUB: BIONICLE 0x56cf20
void NuThreadCriticalSectionEnd(int SectionID) {
}
