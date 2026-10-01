#ifndef NURNDR_H
#define NURNDR_H

#include "nugscn.h"

/* nurndr_gen.c, not decompiled yet (globals defined in nu3d_stubs.c). */
extern NUMTL *nurndr_forced_mtl;
extern NUMTL **nurndr_forced_mtl_table;
extern int nurndr_nforced_mtls;

typedef struct nuhgobj_s NUHGOBJ;
typedef struct nujointanim_s NUJOINTANIM;
typedef struct deformerweightsarray_s DEFORMERWEIGHTSARRAY;

/* pc/nurndr.c */
extern NUVEC curlight;
extern unsigned int curlightcol;
extern float curlightfade;
extern float curlightfadefar;
extern int nurndr_shadowson;
extern float nurndr_alpha;
void NuRndrShadowDirCol(NUVEC *lightdir, unsigned int col, float fadedist, float fadefar);
int NuSpecialDrawAtAlpha(NUHSPECIAL *Gfx, NUMTX *Mtx, float AlphaF);
void NuRndrShadowOnOff(int flag);
int NuRndrGScnObj(NUGOBJ *gobj, NUMTX *wm);
int NuRndrGScnObjClipFlag(NUGOBJ *gobj, NUMTX *wm, int outcode);
int NuHGobjRndr(NUHGOBJ *hgobj, NUMTX *wm, int nlayers, short *layers);
int NuHGobjRndrMtxDwa(NUHGOBJ *hgobj, NUMTX *wm, int nlayers, short *layers, NUMTX *mtx_array,
                      DEFORMERWEIGHTSARRAY **array_dwa, int forceshadow);

/* NuHGobjEval.c, not decompiled yet. */
void NuHGobjEval(NUHGOBJ *hgobj, int njanims, NUJOINTANIM *janim, NUMTX *mtx_array);

/* The PC layer's Direct3D wrapper (named after its error-check string). */
void d3dSetTransform(int state, NUMTX *m);

/* Render target size in pixels (nurndr_gen.c). */
extern int nurndr_pixel_width;
extern int nurndr_pixel_height;

/* Virtual screen centre and size, and scissor resolution (PS2: nups2/ps2video.c; PC file
 * unknown). */
extern int PS2_VCNTR_X;
extern int PS2_VCNTR_Y;
extern int PS2_VREZ_W;
extern int PS2_VREZ_H;
extern int PS2_SREZ_W;
extern int PS2_SREZ_H;

typedef struct nucolour4_s {
    float r;
    float g;
    float b;
    float a;
} NUCOLOUR4;

/* Transformed, lit vertex with one texture coordinate (screen space). */
typedef struct nuvtx_tltc1_s {
    NUVEC pnt;
    float rhw;
    unsigned int diffuse;
    float tc[2];
} NUVTX_TLTC1;

/* PS2: nu3d/ps2/nurndr.c, not decompiled yet. */
int NuRndrLineStrip2d(NUVTX_TLTC1 *vtx, NUMTL *mtl, int cnt);

/* PS2: nu3d/ps2/nustream.c and nurndrstat.c, not decompiled yet. */
int NuRndrSetSpecularLight(NUVEC *dir, NUCOLOUR4 *intensity);
int NuRndrStateUpdateCameraState(void);

/* nudlist.c, not decompiled yet. */
void NuDisplayListUpdateSpecial(NUHSPECIAL *hspec);
int NuDisplayListRndrSpecial(NUHSPECIAL *special, NUMTX *mtx, int skinned, NUMTX *skin_mtx, void *blend_values);

#endif
