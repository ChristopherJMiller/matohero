#ifndef NUCAMERA_H
#define NUCAMERA_H

#include "numath.h"
#include "nuvu0.h"

/* Camera (layout from the PS2 prototype). */
typedef struct nucamera_s {
    NUMTX mtx;
    float fov;
    float aspect;
    float nearclip;
    float farclip;
    float cnearclip; /* overrides nearclip for the clip matrices when non-zero */
    float cfarclip;  /* likewise farclip */
    float portalnearclip;
    NUVEC scale;
} NUCAMERA;

typedef enum nucamfxmode_s {
    NUCAMFX_NONE = 0,
    NUCAMFX_REFLECT = 1,
    NUCAMFX_REFRACT = 2
} NUCAMFX;

typedef struct nureflect_s {
    float distortion;
    NUMTX mtx;
    NUMTX uvmtx;
} NUREFLECT;

/* World-space clip planes, one per column (built by NuCameraBuildClipPlanes). */
typedef struct nuclipplanes_s {
    _VUMTX FrustrumPlanes;
    _VUMTX ScissorPlanes;
    _VUMTX NearFarPlanes;
    _VUVEC NearPlane;
    _VUMTX AbsFrustrumPlanes;
    _VUMTX AbsScissorPlanes;
} NUCLIPPLANES;

extern NUCAMFX camfx;

extern NUVEC cam_axes;
extern NUMTX vmtx;          /* view */
extern NUMTX pmtx;          /* projection */
extern NUMTX cmtx;          /* clipping */
extern NUMTX smtx;          /* scaling */
extern NUMTX vpmtx;         /* view * projection */
extern NUMTX vpc_sci_mtx;   /* view * projection * clipping, scissor */
extern NUMTX vpc_vport_mtx; /* view * projection * clipping, viewport */
extern NUMTX pc_vport_mtx;
extern NUMTX vpsmtx;
extern NUMTX psmtx;
extern NUMTX clip_planes;
extern float nucamera_farclip_hack;
extern int prev_lock;
extern int num_cameras_allocated;
extern NUCAMERA global_camera;
extern NUREFLECT global_reflect;
extern float zx;
extern float zy;
extern float zxs;
extern float zys;
extern NUCLIPPLANES ClipPlanes;
extern NUCAMERA cam_copy;
extern NUCAMERA locked_camera;

NUCAMERA *NuCameraCreate(void);
void NuCameraDestroy(NUCAMERA *camera);
void NuCameraSetVPortClipMtx(NUMTX *vcmtx, NUMTX *vmtx, NUCAMERA *cam, int fast);
void NuCameraSetScissorClipMtx(NUMTX *vcmtx, NUMTX *vmtx, NUCAMERA *cam, int fast);
void NuCameraBuildClipPlanes(void);
/* PS2: nu3d/ps2/nucamera.c; PC has it in the renderer (0x53e350, stubbed). */
void NuCameraSetProjectionMtx(NUMTX *mtx, float fFOV, float fAspect, float fNearPlane, float fFarPlane);
void NuCameraSet(NUCAMERA *camera);
void NuCameraSetEx(NUCAMERA *camera, int fast);
void NuCameraGet(NUCAMERA *camera);
NUCAMERA *NuCameraGetCam(void);
/* Returned &global_camera.mtx, the same address as NuCameraGetCam, so the linker folded
 * the two (/OPT:ICF). */
#define NuCameraGetMtx() (&NuCameraGetCam()->mtx)
NUMTX *NuCameraGetViewMtx(void);
NUMTX *NuCameraGetProjectionMtx(void);
NUMTX *NuCameraGetVPCSMtx(void);
NUMTX *NuCameraGetVPCMtx(void);
NUMTX *NuCameraGetVPMtx(void);
void NuCameraGetClipMtx(NUMTX *vport, NUMTX *sci);
void NuCameraRayCast(NUVEC *p, float x, float y);
void NuCameraTransformScreenClip(NUVEC *dest, NUVEC *src, int n, NUMTX *w);
void NuCameraTransformScreen(NUVEC *dest, NUVEC *src, int n, NUMTX *w);
int NuCameraClipTestExtentsGeneric(NUVEC *min, NUVEC *max, NUMTX *wm, float farclip, int flags, float *min_z);
int NuCameraClipTestSphere(NUVEC *pnt, float radius, NUMTX *wm);
float NuCameraDistSqr(NUVEC *p);
int NuCameraClipTestExtentsAxisAligned(NUVEC4 *Pos, NUVEC4 *Extents, float FarClip);
void NuCameraUnlock(void);
void NuMtxCalcCheapFaceOn(NUMTX *mtx, NUVEC *pos);
void NuMtxCalcDebrisFaceOn(NUMTX *mtx);
void NuCameraRelock(void);

#endif
