#ifndef NURNDR_H
#define NURNDR_H

#include "nugscn.h"

/* nurndr_gen.c, not decompiled yet (globals defined in nu3d_stubs.c). */
extern NUMTL *nurndr_forced_mtl;
extern NUMTL **nurndr_forced_mtl_table;
extern int nurndr_nforced_mtls;

int NuRndrGScnObj(NUGOBJ *gobj, NUMTX *wm);

/* nudlist.c, not decompiled yet. */
void NuDisplayListUpdateSpecial(NUHSPECIAL *hspec);
int NuDisplayListRndrSpecial(NUHSPECIAL *special, NUMTX *mtx, int skinned, NUMTX *skin_mtx, void *blend_values);

#endif
