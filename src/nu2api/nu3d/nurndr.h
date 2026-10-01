#ifndef NURNDR_H
#define NURNDR_H

#include "nugscn.h"
#include "numtl.h"

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

/* PS2: nu3d/ps2/nustream.c, not decompiled yet. */
int NuRndrSetSpecularLight(NUVEC *dir, NUCOLOUR4 *intensity);

/* pc/nurndrstat.c (PS2: nu3d/ps2/nurndrstat.c and nustream.c's light setters). */
extern NUGLOBALRNDRSTATE render_state;
void RndrStateClear(NURNDRSTATE *state);
int NuRndrSetAmbientLight(NUVEC *c0);
int NuRndrSetDirectionalLights(NUVEC *d0, NUCOLOUR3 *c0, NUVEC *d1, NUCOLOUR3 *c1, NUVEC *d2, NUCOLOUR3 *c2);
int NuRndrStateUpdateCameraState(void);
void NuRndrFogMode(int enable);
void NuRndrFogSet(float fognear, float fogfar, unsigned int colour, float density);
void RndrStateResetGlobalState(NUGLOBALRNDRSTATE *state);
void RndrStateResetSharedGlobalState(void);
NUGLOBALRNDRSTATE *RndrStateGetSharedGlobalState(void);
void RndrStateSetConstAlphaTint(unsigned int const_alpha_enabled, unsigned int const_tint_enabled, float const_alpha, NUCOLOUR3 *const_tint, NUMTL *mtl);
void NuRndrUpdateCameraPlane(void);
void NuRndrGetSpecularMtx(NUMTX *out, NUMTL *mtl);
void NuRndrSetSpecularMtx(NUMTX *m);

/* Post-effect parameters (PS2 layouts). */
typedef struct nudofparameters {
    int hasDepthOfField;
    float aperture;
    float focalLength;
    float focalPlane;
} NUDOFPARAMETERS;

typedef struct nucameramotionblurparameters {
    int hasMotion;
    int applyMotion;
    NUMTX viewProj;
    NUMTX preViewProj;
    float timeElapsed;
    float shutterSpeed;
    float threshold;
} NUCAMERAMOTIONBLURPARAMETERS;

/* nurndr_gen.c */
extern int NuRndr_ShadowScene;
extern int global_GobjIsReflectedGeometry;
extern int global_GobjIsShadowReceive;
extern NUCAMERAMOTIONBLURPARAMETERS NuRndr_cameraMotionBlur;
extern NUDOFPARAMETERS NuRndr_depthOfField;
int NuRndIsReflectionGobj(void);
void NuRndrStartShadowReceiveRender(void);
void NuRndrEndShadowReceiveRender(void);
void NuDepthOfFieldEffect(float aperture, float focalLength, float planeInFocus);
void NuCameraMotionBlurEffect(NUMTX *viewProj, NUMTX *preViewProj, float timeElapsed, float shutterSpeed, float threshold);
void NuDynamicLightingRenderShadows(void);
extern float global_windscale;
extern float global_windspeed;
extern unsigned int global_frame_count;
void WindShear(NUMTX *windmtx, NUMTX *instmtx, int wind_scale, int wind_speed);

/* nudlist.c */
void NuDisplayListUpdateSpecial(NUHSPECIAL *hspec);
int NuDisplayListRndrSpecial(NUHSPECIAL *special, NUMTX *mtx, int skinned, NUMTX *skin_mtx, DEFORMERWEIGHTSARRAY *blend_vals);

/* A light that casts shadows: its matrices, then the list its shadow casters are drawn
 * into (PS2 layout up to there; the rest isn't placed). */
typedef struct nudynamiclight_s {
    NUMTX view;
    NUMTX projection;
    NUMTX warpTransform;
    NUMTX warpTransformInverse;
    NUMTX shadowTransform;
    NUVEC4 projectionParams;
    NUDISPLAYLIST geometryDList;
} NUDYNAMICLIGHT;

/* nurndr_gen.c globals and PS2 nurndr/nustream functions, not decompiled yet
 * (nu3d_stubs.c). */
extern NUDYNAMICLIGHT *NuRndr_shadowLight;
extern int nurndr_force_lod;

#endif
