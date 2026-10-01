#ifndef NUTEX_H
#define NUTEX_H

/* PS2: nu3d/ps2/nutex.c. Textures, by id. */

#include "nutypes.h"

typedef struct nutex_s NUTEX;

/* Not decompiled yet (nu3d_stubs.c). */
int NuTexCreate(NUTEX *tex);
int NuTexRead(char *fname, VARIPTR *buff, VARIPTR buffend);
void NuTexDestroy(int tid);
int NuTexWidth(int tid);
int NuTexHeight(int tid);

#endif
