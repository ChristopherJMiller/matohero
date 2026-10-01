#include "nufile.h"

/* nufile functions not decompiled yet. Kept out of nufile.c: functions there that call
 * these are compiled after them and would rely on the stubs' register usage. */

// STUB: BIONICLE 0x566b60
int NuFileOpenSize(NUFILE fh) {
    return 0;
}

// STUB: BIONICLE 0x566d40
int NuFileSeek(NUFILE fh, int offset, int origin) {
    return 0;
}

// STUB: BIONICLE 0x567470
NUFILE NuFileOpen(char *file, NUFILEMODE mode) {
    return 0;
}

// STUB: BIONICLE 0x5677a0
void NuFileClose(NUFILE fh) {
}

// STUB: BIONICLE 0x5678a0
int NuFileSize(char *fname) {
    return 0;
}

// STUB: BIONICLE 0x5679f0
int NuFileRead(NUFILE fh, void *data, int size) {
    return 0;
}

// STUB: BIONICLE 0x567ba0
char NuFileReadChar(NUFILE fh) {
    return 0;
}

// STUB: BIONICLE 0x567bc0
NUWCHAR NuFileReadWChar(NUFILE fh) {
    return 0;
}
