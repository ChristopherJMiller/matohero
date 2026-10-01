#include "nucamera.h"

#include <string.h>

#include "numem.h"
#include "nutrig.h"
#include "nuvec.h"
#include "nuvec4.h"
#include "nuvport.h"
#include "nuvu0.h"

/* The camera: global_camera and the view, projection and clip matrices derived from it.
 * Names, locals and data order from the PS2 prototype; logic checked against
 * opensagadev/saga src/nu2api/nu3d/generic/nucamera_gen.cpp (GPL-3.0, a later engine).
 * PC orders the functions differently from PS2 and moved NuCameraSet into the renderer
 * (0x53d640). Not in the PC exe (unreferenced): NuCameraLock, NuCameraDist,
 * NuCameraGetClippingMtx, NuCameraGetScalingMtx, NuCameraGetPCMtx, NuCameraGetPCSMtx,
 * NuCameraGetTrans, NuCameraGetClipPlanes, ... */

// STRING: BIONICLE 0x5e00bc
// __FILE__, passed to NuMemAlloc: ".\\nucamera_gen.c"

// GLOBAL: BIONICLE 0x6166c0
NUVEC cam_axes = {1.0f, 1.0f, 1.0f};

#define NUMTX_IDENTITY {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}

// GLOBAL: BIONICLE 0x6166d0
NUMTX vmtx = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x616710
NUMTX pmtx = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x616750
NUMTX cmtx = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x616790
NUMTX smtx = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x6167d0
NUMTX vpmtx = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x616810
NUMTX vpc_sci_mtx = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x616850
NUMTX vpc_vport_mtx = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x616890
NUMTX pc_vport_mtx = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x6168d0
NUMTX vpsmtx = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x616910
NUMTX psmtx = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x616950
NUMTX clip_planes = NUMTX_IDENTITY;

// GLOBAL: BIONICLE 0x96ce18
int prev_lock = 0;

// GLOBAL: BIONICLE 0x616990
float nucamera_farclip_hack = 0.1f;

// GLOBAL: BIONICLE 0x96ce24
int num_cameras_allocated = 0;

// GLOBAL: BIONICLE 0x3607480
NUCAMERA global_camera;

// GLOBAL: BIONICLE 0x3607344
float zx;

// GLOBAL: BIONICLE 0x3607464
float zy;

// GLOBAL: BIONICLE 0x3607500
NUCAMERA cam_copy;

// GLOBAL: BIONICLE 0x3607360
NUCAMERA locked_camera;

// FUNCTION: BIONICLE 0x538490
NUCAMERA *NuCameraCreate(void) {
    NUCAMERA *camera = NuMemAllocAt(sizeof(NUCAMERA), 239);

    NuMtxSetIdentity(&camera->mtx);
    camera->fov = 0.75f;
    camera->aspect = 0.75f;
    camera->nearclip = 0.15f;
    camera->portalnearclip = 0.0f;
    camera->scale.x = camera->scale.y = camera->scale.z = 1.0f;
    camera->farclip = 10000.0f;
    camera->cfarclip = 0.0f;
    camera->cnearclip = 0.0f;
    num_cameras_allocated++;
    return camera;
}

// FUNCTION: BIONICLE 0x5384f0
void NuCameraDestroy(NUCAMERA *camera) {
    if (camera != NULL) {
        NuMemFreeAt(camera, 267);
        num_cameras_allocated--;
    }
}

// FUNCTION: BIONICLE 0x538520
void NuCameraSetVPortClipMtx(NUMTX *vcmtx, NUMTX *vmtx, NUCAMERA *cam, int fast) {
    float fc;
    float nc;
    NUVIEWPORT *vp;
    float w;
    float h;
    float fFOV;
    float fAspect;
    float cotfov;
    int ang;

    if (!fast) {
        vp = NuVpGetCurrentViewport();
        fc = (cam->cfarclip == 0.0f ? cam->farclip : cam->cfarclip) - nucamera_farclip_hack;
        nc = cam->cnearclip == 0.0f ? cam->nearclip : cam->cnearclip;
        w = vp->clip_w;
        h = vp->clip_h;
        fFOV = cam->fov;
        fAspect = cam->aspect;
        ang = (int)((fFOV * 0.5f) * 10430.378f);
        cotfov = NU_COS_LUT(ang) / NU_SIN_LUT(ang);
        w = w * (fAspect * cotfov);
        h = h * cotfov;
        memset(&pc_vport_mtx, 0, sizeof(NUMTX));
        pc_vport_mtx._00 = w;
        pc_vport_mtx._11 = h;
        pc_vport_mtx._23 = 1.0f;
        pc_vport_mtx._33 = 0.0f;
        pc_vport_mtx._22 = (fc + nc) / (fc - nc);
        pc_vport_mtx._32 = -2.0f * nc * fc / (fc - nc);
    }
    NuMtxMulVU0(vcmtx, vmtx, &pc_vport_mtx);
}

// FUNCTION: BIONICLE 0x538630
void NuCameraSetScissorClipMtx(NUMTX *vcmtx, NUMTX *vmtx, NUCAMERA *cam, int fast) {
    // GLOBAL: BIONICLE 0x96cd90
    static NUMTX cmtx;
    float fc;
    float nc;
    NUVIEWPORT *vp;
    float w;
    float h;
    float fFOV;
    float fAspect;
    float rtan;
    int ang;

    if (!fast) {
        vp = NuVpGetCurrentViewport();
        fc = (cam->cfarclip == 0.0f ? cam->farclip : cam->cfarclip) - nucamera_farclip_hack;
        nc = cam->cnearclip == 0.0f ? cam->nearclip : cam->cnearclip;
        w = vp->scis_w;
        h = vp->scis_h;
        fFOV = cam->fov;
        fAspect = cam->aspect;
        ang = (int)((fFOV * 0.5f) * 10430.378f);
        rtan = NU_COS_LUT(ang) / NU_SIN_LUT(ang);
        w = w * fAspect * rtan;
        h = rtan * h;
        memset(&cmtx, 0, sizeof(NUMTX));
        cmtx._00 = w;
        cmtx._11 = h;
        cmtx._23 = 1.0f;
        cmtx._22 = (fc + nc) / (fc - nc);
        cmtx._32 = -2.0f * nc * fc / (fc - nc);
    }
    NuMtxMulVU0(vcmtx, vmtx, &cmtx);
}

// FUNCTION: BIONICLE 0x539690
void NuCameraGet(NUCAMERA *camera) {
    *camera = global_camera;
}

// FUNCTION: BIONICLE 0x5396b0
NUCAMERA *NuCameraGetCam(void) {
    return &global_camera;
}

// FUNCTION: BIONICLE 0x5396c0
NUMTX *NuCameraGetViewMtx(void) {
    return &vmtx;
}

// FUNCTION: BIONICLE 0x5396d0
NUMTX *NuCameraGetProjectionMtx(void) {
    return &pmtx;
}

// FUNCTION: BIONICLE 0x5396e0
NUMTX *NuCameraGetVPCSMtx(void) {
    return &vpsmtx;
}

// FUNCTION: BIONICLE 0x5396f0
NUMTX *NuCameraGetVPCMtx(void) {
    return &vpc_vport_mtx;
}

// FUNCTION: BIONICLE 0x539700
NUMTX *NuCameraGetVPMtx(void) {
    return &vpmtx;
}

// FUNCTION: BIONICLE 0x539710
void NuCameraGetClipMtx(NUMTX *vport, NUMTX *sci) {
    if (vport != NULL) {
        *vport = vpc_vport_mtx;
    }
    if (sci != NULL) {
        *sci = vpc_sci_mtx;
    }
}

// FUNCTION: BIONICLE 0x539740
void NuCameraRayCast(NUVEC *p, float x, float y) {
    p->x = zx * x;
    p->y = zy * y;
    p->z = 1.0f;
    NuVecMtxRotate(p, p, &global_camera.mtx);
}

// FUNCTION: BIONICLE 0x539780
void NuCameraTransformScreenClip(NUVEC *dest, NUVEC *src, int n, NUMTX *w) {
    NUVEC *end = src + n;
    NUMTX m;

    if (w != NULL) {
        NuMtxMulH(&m, w, &vpc_vport_mtx);
    } else {
        m = vpc_vport_mtx;
    }
    for (; src < end; src++, dest++) {
        NuVecMtxTransformH(dest, src, &m);
    }
}

// FUNCTION: BIONICLE 0x539800
void NuCameraTransformScreen(NUVEC *dest, NUVEC *src, int n, NUMTX *w) {
    NUVEC *end = src + n;
    NUMTX m;

    if (w != NULL) {
        NuMtxMulH(&m, w, &vpsmtx);
    } else {
        m = vpsmtx;
    }
    for (; src < end; src++, dest++) {
        NuVecMtxTransformH(dest, src, &m);
    }
}

// FUNCTION: BIONICLE 0x539880
int NuCameraClipTestSphere(NUVEC *pnt, float radius, NUMTX *wm) {
    NUVEC v;
    NUVEC4 dot;

    NuVecMtxTransform(&v, pnt, wm);
    NuVecMtxTransform(&v, &v, &vmtx);
    if (v.z - global_camera.nearclip + radius < 0.0f) {
        return 1;
    }
    if (global_camera.farclip - v.z + radius < 0.0f) {
        return 1;
    }
    NuVec4MtxTransform(&dot, &v, &clip_planes);
    radius = -radius;
    if (dot.x < radius || dot.y < radius || dot.z < radius || dot.w < radius) {
        return 1;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x539950
float NuCameraDistSqr(NUVEC *p) {
    return NuVecDistSqr(p, (NUVEC *)&global_camera.mtx._30, NULL);
}

// FUNCTION: BIONICLE 0x539970
void NuCameraUnlock(void) {
    if (prev_lock == 1) {
        NuCameraSet(&cam_copy);
    }
}

// FUNCTION: BIONICLE 0x539990
void NuCameraRelock(void) {
    if (prev_lock == 1) {
        NuCameraSet(&locked_camera);
    }
}
