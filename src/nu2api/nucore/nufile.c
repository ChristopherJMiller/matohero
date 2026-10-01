#include "nufile.h"

/* File system. Not decompiled yet; stubs give reccmp and the hook build addresses for
 * calls from decompiled code. */

// STUB: BIONICLE 0x566110
void *NuMemFileAddr(NUFILE fh) {
    return 0;
}

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
