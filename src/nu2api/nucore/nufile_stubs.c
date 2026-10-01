#include "nufile.h"
#include "nuthread.h"

/* nufile functions not decompiled yet. Kept out of nufile.c: functions there that call
 * these are compiled after them and would rely on the stubs' register usage. */

// STUB: BIONICLE 0x567c80
int NuDatFileLoadBuffer(NUDATHDR *ndh, char *fname, void *mem, int buffsize) {
    return 0;
}

// STUB: BIONICLE 0x567f40
NUDATHDR *NuDatOpenEx(char *fname, VARIPTR *buff, int *req, short openmode) {
    return 0;
}
