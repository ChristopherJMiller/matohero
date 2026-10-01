#ifndef NUSPECIAL_H
#define NUSPECIAL_H

#include "nugscn.h"

extern int nuspecial_draw_state;

int NuSpecialFind(NUGSCN *scene, NUHSPECIAL *special, char *name, int ErrMsg);
int NuSpecialFindMultiWC(NUGSCN *scene, NUHSPECIAL *special, char (*WCName)[20], char *name, int maxcnt, int ErrMsg);
int NuSpecialFindMulti(NUGSCN *scene, NUHSPECIAL *special, char *name, int maxcnt, int ErrMsg);
NUVEC *NuSpecialGetPos(NUHSPECIAL *sph);
NUVEC *NuSpecialGetDrawPos(NUHSPECIAL *sph);
NUMTX *NuSpecialGetMtx(NUHSPECIAL *sph);
void NuSpecialSetMtx(NUHSPECIAL *sph, NUMTX *mtx);
char *NuSpecialGetName(NUHSPECIAL *sph);
int NuSpecialExistsFn(NUHSPECIAL *sph);
void NuSpecialMtlMap(int nmtls, NUMTL **mtls);
void NuSpecialForceMtl(NUMTL *mtl);
int NuSpecialGetVisibilityFn(NUHSPECIAL *spec);
NUMTX *NuSpecialGetDrawMtx(NUHSPECIAL *special);
NUINSTANIM *NuSpecialGetInstAnim(NUHSPECIAL *special);
int NuSpecialCompare(NUHSPECIAL *special1, NUHSPECIAL *special2);

#endif
