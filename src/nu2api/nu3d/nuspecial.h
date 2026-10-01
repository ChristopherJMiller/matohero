#ifndef NUSPECIAL_H
#define NUSPECIAL_H

#include "nugscn.h"
#include "numtl.h"

extern int nuspecial_draw_state;
extern int nuspecial_const_alpha_enabled;
extern float nuspecial_const_alpha;
extern int nuspecial_const_tint_enabled;
extern NUCOLOUR3 nuspecial_const_tint;
extern int nuspecial_clip_state;

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
int NuSpecialDrawAt(NUHSPECIAL *special, NUMTX *transform);
void NuSpecialSetVisibility(NUHSPECIAL *spec, int visibility);
void NuSpecialSetCollision(NUHSPECIAL *spec, int collision);
int NuSpecialGetCollision(NUHSPECIAL *spec);
int NuSpecialGetInstanceix(NUHSPECIAL *special);
int NuSpecialNumMtls(NUHSPECIAL *special);
NUMTL *NuSpecialGetMtl(NUHSPECIAL *special, int i);
void NuSpecialGetBounds(NUHSPECIAL *special, NUVEC *min, NUVEC *max);
void NuSpecialGetRadius(NUHSPECIAL *special, NUVEC *cntr, float *radius);
void NuSpecialSetDrawMtx(NUHSPECIAL *special, NUMTX *mtx);
void NuSpecialSetDrawPos(NUHSPECIAL *special, NUVEC *pos);

#endif
