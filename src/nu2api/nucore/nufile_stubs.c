#include "nufile.h"
#include "nuthread.h"

/* nufile functions not decompiled yet. Kept out of nufile.c: functions there that call
 * these are compiled after them and would rely on the stubs' register usage. */

/* Decompiled (see docs/roadmap.md), but in nufile.c it changes how the compiler inlines
 * NuDatFileSeek there, which costs that function its match. */
// STUB: BIONICLE 0x567c80
int NuDatFileLoadBuffer(NUDATHDR *ndh, char *fname, void *mem, int buffsize) {
    return 0;
}
