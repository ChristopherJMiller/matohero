#ifndef NUFILE_H
#define NUFILE_H

#include "nustring.h"

typedef int NUFILE;

typedef enum {
    NUFILE_READ = 0,
    NUFILE_WRITE = 1,
    NUFILE_APPEND = 2,
} NUFILEMODE;

int NuFileExists(char *fname);
NUFILE NuFileOpen(char *file, NUFILEMODE mode);
void NuFileClose(NUFILE fh);
int NuFileSeek(NUFILE fh, int offset, int origin);
int NuFileRead(NUFILE fh, void *data, int size);
char NuFileReadChar(NUFILE fh);
NUWCHAR NuFileReadWChar(NUFILE fh);
int NuFileOpenSize(NUFILE fh);
void *NuMemFileAddr(NUFILE fh);

#endif
