#ifndef NUCAMERA_H
#define NUCAMERA_H

#include "numath.h"

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
extern float zx;
extern float zy;
extern NUCAMERA cam_copy;
extern NUCAMERA locked_camera;

NUCAMERA *NuCameraCreate(void);
void NuCameraDestroy(NUCAMERA *camera);
void NuCameraSetVPortClipMtx(NUMTX *vcmtx, NUMTX *vmtx, NUCAMERA *cam, int fast);
void NuCameraSetScissorClipMtx(NUMTX *vcmtx, NUMTX *vmtx, NUCAMERA *cam, int fast);
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
int NuCameraClipTestSphere(NUVEC *pnt, float radius, NUMTX *wm);
float NuCameraDistSqr(NUVEC *p);
void NuCameraUnlock(void);
void NuCameraRelock(void);

#endif
