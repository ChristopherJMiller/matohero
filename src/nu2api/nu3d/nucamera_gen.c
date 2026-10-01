#include "nucamera.h"

#include <string.h>

#include "folded.h"
#include "nufloat.h"
#include "numem.h"
#include "nurndr.h"
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

// GLOBAL: BIONICLE 0x96ce10
NUCAMFX camfx = NUCAMFX_NONE;

// GLOBAL: BIONICLE 0x96ce18
int prev_lock = 0;

// GLOBAL: BIONICLE 0x616990
float nucamera_farclip_hack = 0.1f;

// GLOBAL: BIONICLE 0x96ce1c
static int clip_scissor_to_viewport = 0;

// GLOBAL: BIONICLE 0x96ce20
static int current_clip_scissor_to_viewport = 0;

// GLOBAL: BIONICLE 0x96ce24
int num_cameras_allocated = 0;

// GLOBAL: BIONICLE 0x3607480
NUCAMERA global_camera;

// GLOBAL: BIONICLE 0x36073e0
NUREFLECT global_reflect;

// GLOBAL: BIONICLE 0x3607344
float zx;

// GLOBAL: BIONICLE 0x3607464
float zy;

// GLOBAL: BIONICLE 0x3607340
float zxs;

// GLOBAL: BIONICLE 0x3607348
float zys;

// GLOBAL: BIONICLE 0x96cc40
NUCLIPPLANES ClipPlanes;

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

// FUNCTION: BIONICLE 0x538740
void NuCameraBuildClipPlanes(void) {
    float NearDist;
    float NearDistSqr;
    float HalfWidth;
    float HalfHeight;
    float LR_InvLen;
    float TB_InvLen;
    _VUMTX CamSpaceFrustrumPlanes;
    _VUMTX CamSpaceScissorPlanes;
    float fDot;

    NearDist = global_camera.nearclip;
    NearDistSqr = NearDist * NearDist;

    HalfWidth = zx * NearDist;
    HalfHeight = zy * NearDist;
    LR_InvLen = 1.0f / NuFsqrt(HalfWidth * HalfWidth + NearDistSqr);
    TB_InvLen = 1.0f / NuFsqrt(HalfHeight * HalfHeight + NearDistSqr);
    NuMtxSetZero(&CamSpaceFrustrumPlanes.m);
    CamSpaceFrustrumPlanes.m._00 = -(LR_InvLen * NearDist);
    CamSpaceFrustrumPlanes.m._20 = LR_InvLen * HalfWidth;
    CamSpaceFrustrumPlanes.m._01 = -CamSpaceFrustrumPlanes.m._00;
    CamSpaceFrustrumPlanes.m._21 = CamSpaceFrustrumPlanes.m._20;
    CamSpaceFrustrumPlanes.m._12 = -(TB_InvLen * NearDist);
    CamSpaceFrustrumPlanes.m._22 = TB_InvLen * HalfHeight;
    CamSpaceFrustrumPlanes.m._13 = -CamSpaceFrustrumPlanes.m._12;
    CamSpaceFrustrumPlanes.m._23 = CamSpaceFrustrumPlanes.m._22;

    HalfWidth = zxs * NearDist;
    HalfHeight = zys * NearDist;
    LR_InvLen = 1.0f / NuFsqrt(HalfWidth * HalfWidth + NearDistSqr);
    TB_InvLen = 1.0f / NuFsqrt(HalfHeight * HalfHeight + NearDistSqr);
    NuMtxSetZero(&CamSpaceScissorPlanes.m);
    CamSpaceScissorPlanes.m._00 = -(LR_InvLen * NearDist);
    CamSpaceScissorPlanes.m._20 = LR_InvLen * HalfWidth;
    CamSpaceScissorPlanes.m._01 = -CamSpaceScissorPlanes.m._00;
    CamSpaceScissorPlanes.m._21 = CamSpaceScissorPlanes.m._20;
    CamSpaceScissorPlanes.m._12 = -(TB_InvLen * NearDist);
    CamSpaceScissorPlanes.m._22 = TB_InvLen * HalfHeight;
    CamSpaceScissorPlanes.m._13 = -CamSpaceScissorPlanes.m._12;
    CamSpaceScissorPlanes.m._23 = CamSpaceScissorPlanes.m._22;

    NuMtxMulH(&ClipPlanes.FrustrumPlanes.m, &vmtx, &CamSpaceFrustrumPlanes.m);
    NuMtxMulH(&ClipPlanes.ScissorPlanes.m, &vmtx, &CamSpaceScissorPlanes.m);

    fDot = global_camera.mtx._20 * global_camera.mtx._30 + global_camera.mtx._21 * global_camera.mtx._31 +
           global_camera.mtx._22 * global_camera.mtx._32;
    ClipPlanes.NearPlane.v4.x = global_camera.mtx._20;
    ClipPlanes.NearPlane.v4.y = global_camera.mtx._21;
    ClipPlanes.NearPlane.v4.z = global_camera.mtx._22;
    ClipPlanes.NearPlane.v4.w = -fDot - global_camera.nearclip;

    ClipPlanes.NearFarPlanes.m._00 = -global_camera.mtx._20;
    ClipPlanes.NearFarPlanes.m._10 = -global_camera.mtx._21;
    ClipPlanes.NearFarPlanes.m._20 = -global_camera.mtx._22;
    ClipPlanes.NearFarPlanes.m._30 = fDot + global_camera.farclip;
    ClipPlanes.NearFarPlanes.m._01 = ClipPlanes.NearPlane.v4.x;
    ClipPlanes.NearFarPlanes.m._11 = ClipPlanes.NearPlane.v4.y;
    ClipPlanes.NearFarPlanes.m._21 = ClipPlanes.NearPlane.v4.z;
    ClipPlanes.NearFarPlanes.m._31 = ClipPlanes.NearPlane.v4.w;
    ClipPlanes.NearFarPlanes.m._02 = NuFabs(ClipPlanes.NearFarPlanes.m._00);
    ClipPlanes.NearFarPlanes.m._12 = NuFabs(ClipPlanes.NearFarPlanes.m._10);
    ClipPlanes.NearFarPlanes.m._22 = NuFabs(ClipPlanes.NearFarPlanes.m._20);
    ClipPlanes.NearFarPlanes.m._32 = NuFabs(ClipPlanes.NearFarPlanes.m._30);
    ClipPlanes.NearFarPlanes.m._03 = NuFabs(ClipPlanes.NearFarPlanes.m._01);
    ClipPlanes.NearFarPlanes.m._13 = NuFabs(ClipPlanes.NearFarPlanes.m._11);
    ClipPlanes.NearFarPlanes.m._23 = NuFabs(ClipPlanes.NearFarPlanes.m._21);
    ClipPlanes.NearFarPlanes.m._33 = NuFabs(ClipPlanes.NearFarPlanes.m._31);

    ClipPlanes.AbsFrustrumPlanes.m._00 = NuFabs(ClipPlanes.FrustrumPlanes.m._00);
    ClipPlanes.AbsFrustrumPlanes.m._01 = NuFabs(ClipPlanes.FrustrumPlanes.m._01);
    ClipPlanes.AbsFrustrumPlanes.m._02 = NuFabs(ClipPlanes.FrustrumPlanes.m._02);
    ClipPlanes.AbsFrustrumPlanes.m._03 = NuFabs(ClipPlanes.FrustrumPlanes.m._03);
    ClipPlanes.AbsFrustrumPlanes.m._10 = NuFabs(ClipPlanes.FrustrumPlanes.m._10);
    ClipPlanes.AbsFrustrumPlanes.m._11 = NuFabs(ClipPlanes.FrustrumPlanes.m._11);
    ClipPlanes.AbsFrustrumPlanes.m._12 = NuFabs(ClipPlanes.FrustrumPlanes.m._12);
    ClipPlanes.AbsFrustrumPlanes.m._13 = NuFabs(ClipPlanes.FrustrumPlanes.m._13);
    ClipPlanes.AbsFrustrumPlanes.m._20 = NuFabs(ClipPlanes.FrustrumPlanes.m._20);
    ClipPlanes.AbsFrustrumPlanes.m._21 = NuFabs(ClipPlanes.FrustrumPlanes.m._21);
    ClipPlanes.AbsFrustrumPlanes.m._22 = NuFabs(ClipPlanes.FrustrumPlanes.m._22);
    ClipPlanes.AbsFrustrumPlanes.m._23 = NuFabs(ClipPlanes.FrustrumPlanes.m._23);
    ClipPlanes.AbsFrustrumPlanes.m._30 = 0.0f;
    ClipPlanes.AbsFrustrumPlanes.m._31 = 0.0f;
    ClipPlanes.AbsFrustrumPlanes.m._32 = 0.0f;
    ClipPlanes.AbsFrustrumPlanes.m._33 = 0.0f;

    ClipPlanes.AbsScissorPlanes.m._00 = NuFabs(ClipPlanes.ScissorPlanes.m._00);
    ClipPlanes.AbsScissorPlanes.m._01 = NuFabs(ClipPlanes.ScissorPlanes.m._01);
    ClipPlanes.AbsScissorPlanes.m._02 = NuFabs(ClipPlanes.ScissorPlanes.m._02);
    ClipPlanes.AbsScissorPlanes.m._03 = NuFabs(ClipPlanes.ScissorPlanes.m._03);
    ClipPlanes.AbsScissorPlanes.m._10 = NuFabs(ClipPlanes.ScissorPlanes.m._10);
    ClipPlanes.AbsScissorPlanes.m._11 = NuFabs(ClipPlanes.ScissorPlanes.m._11);
    ClipPlanes.AbsScissorPlanes.m._12 = NuFabs(ClipPlanes.ScissorPlanes.m._12);
    ClipPlanes.AbsScissorPlanes.m._13 = NuFabs(ClipPlanes.ScissorPlanes.m._13);
    ClipPlanes.AbsScissorPlanes.m._20 = NuFabs(ClipPlanes.ScissorPlanes.m._20);
    ClipPlanes.AbsScissorPlanes.m._21 = NuFabs(ClipPlanes.ScissorPlanes.m._21);
    ClipPlanes.AbsScissorPlanes.m._22 = NuFabs(ClipPlanes.ScissorPlanes.m._22);
    ClipPlanes.AbsScissorPlanes.m._23 = NuFabs(ClipPlanes.ScissorPlanes.m._23);
    ClipPlanes.AbsScissorPlanes.m._30 = 0.0f;
    ClipPlanes.AbsScissorPlanes.m._31 = 0.0f;
    ClipPlanes.AbsScissorPlanes.m._32 = 0.0f;
    ClipPlanes.AbsScissorPlanes.m._33 = 0.0f;
}

/* Outcodes of a point against the frustum (or the scissor region) in view space. */
#define NUCAMERA_OUT_LEFT 0x01
#define NUCAMERA_OUT_RIGHT 0x02
#define NUCAMERA_OUT_BOTTOM 0x04
#define NUCAMERA_OUT_TOP 0x08
#define NUCAMERA_OUT_FAR 0x10
#define NUCAMERA_OUT_NEAR 0x20

// FUNCTION: BIONICLE 0x538cf0
int NuCameraClipTestExtentsGeneric(NUVEC *min, NUVEC *max, NUMTX *wm, float farclip, int flags, float *min_z) {
    NUVEC src[8];
    NUVEC dest[8];
    unsigned char oc[8];
    unsigned int *erase_outcode;
    NUMTX mtx;
    int i;

    if (farclip == 0.0f) {
        farclip = global_camera.farclip;
    }
    if (wm != NULL && wm != &numtx_identity) {
        NuMtxMul(&mtx, wm, &vmtx);
    } else {
        mtx = vmtx;
    }

    src[0].x = min->x;
    src[0].y = min->y;
    src[0].z = min->z;
    src[1].x = max->x;
    src[1].y = max->y;
    src[1].z = max->z;
    src[2].x = min->x;
    src[2].y = min->y;
    src[2].z = max->z;
    src[3].x = max->x;
    src[3].y = max->y;
    src[3].z = min->z;
    src[4].x = min->x;
    src[4].y = max->y;
    src[4].z = min->z;
    src[5].x = min->x;
    src[5].y = max->y;
    src[5].z = max->z;
    src[6].x = max->x;
    src[6].y = min->y;
    src[6].z = min->z;
    src[7].x = max->x;
    src[7].y = min->y;
    src[7].z = max->z;
    NuVecMtxTransform(&dest[0], &src[0], &mtx);
    NuVecMtxTransform(&dest[1], &src[1], &mtx);
    NuVecMtxTransform(&dest[2], &src[2], &mtx);
    NuVecMtxTransform(&dest[3], &src[3], &mtx);
    NuVecMtxTransform(&dest[4], &src[4], &mtx);
    NuVecMtxTransform(&dest[5], &src[5], &mtx);
    NuVecMtxTransform(&dest[6], &src[6], &mtx);
    NuVecMtxTransform(&dest[7], &src[7], &mtx);

    erase_outcode = (unsigned int *)oc;
    erase_outcode[0] = 0;
    erase_outcode[1] = 0;
    *min_z = 3.402823466e+38f;
    for (i = 0; i < 8; i++) {
        if (dest[i].z < *min_z) {
            *min_z = dest[i].z;
        }
        if (dest[i].z < global_camera.nearclip && !(flags & 2)) {
            oc[i] |= NUCAMERA_OUT_NEAR;
        }
        if (dest[i].z > farclip) {
            oc[i] |= NUCAMERA_OUT_FAR;
        }
        if (dest[i].x < -(zx * dest[i].z)) {
            oc[i] |= NUCAMERA_OUT_LEFT;
        }
        if (dest[i].x > zx * dest[i].z) {
            oc[i] |= NUCAMERA_OUT_RIGHT;
        }
        if (dest[i].y < -(zy * dest[i].z)) {
            oc[i] |= NUCAMERA_OUT_TOP;
        }
        if (dest[i].y > zy * dest[i].z) {
            oc[i] |= NUCAMERA_OUT_BOTTOM;
        }
    }
    if (oc[0] & oc[1] & oc[2] & oc[3] & oc[4] & oc[5] & oc[6] & oc[7]) {
        return 0;
    }
    if (oc[0] | oc[1] | oc[2] | oc[3] | oc[4] | oc[5] | oc[6] | oc[7]) {
        if (flags & 1) {
            return 2;
        }

        /* Partly outside the frustum: test against the scissor region. */
        erase_outcode[0] = 0;
        erase_outcode[1] = 0;
        for (i = 0; i < 8; i++) {
            if (dest[i].z < global_camera.nearclip && !(flags & 2)) {
                oc[i] |= NUCAMERA_OUT_NEAR;
            }
            if (dest[i].z > farclip) {
                oc[i] |= NUCAMERA_OUT_FAR;
            }
            if (dest[i].x < -(dest[i].z * zxs)) {
                oc[i] |= NUCAMERA_OUT_LEFT;
            }
            if (dest[i].x > dest[i].z * zxs) {
                oc[i] |= NUCAMERA_OUT_RIGHT;
            }
            if (dest[i].y < -(dest[i].z * zys)) {
                oc[i] |= NUCAMERA_OUT_TOP;
            }
            if (dest[i].y > dest[i].z * zys) {
                oc[i] |= NUCAMERA_OUT_BOTTOM;
            }
        }
        if (!(oc[0] & oc[1] & oc[2] & oc[3] & oc[4] & oc[5] & oc[6] & oc[7]) &&
            (oc[0] | oc[1] | oc[2] | oc[3] | oc[4] | oc[5] | oc[6] | oc[7])) {
            return 2;
        }
    }
    return 1;
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

// FUNCTION: BIONICLE 0x5399b0
int NuCameraClipTestExtentsAxisAligned(NUVEC4 *Pos, NUVEC4 *Extents, float FarClip) {
    _VUVEC Distances[2];
    _VUVEC Radii[2];
    float OldFarClip;

    OldFarClip = ClipPlanes.NearFarPlanes.m._30;
    if (FarClip != 0.0f) {
        ClipPlanes.NearFarPlanes.m._30 = ClipPlanes.NearFarPlanes.m._30 - global_camera.farclip + FarClip;
    }

    NuVecMtxTransform(&Distances[0].v, (NUVEC *)Pos, &ClipPlanes.FrustrumPlanes.m);
    Distances[0].v4.w = ClipPlanes.FrustrumPlanes.m._33 + (Pos->z * ClipPlanes.FrustrumPlanes.m._23 + (Pos->x * ClipPlanes.FrustrumPlanes.m._03 + Pos->y * ClipPlanes.FrustrumPlanes.m._13));
    Distances[1].v4.x = ClipPlanes.NearFarPlanes.m._30 + (Pos->z * ClipPlanes.NearFarPlanes.m._20 + (Pos->x * ClipPlanes.NearFarPlanes.m._00 + Pos->y * ClipPlanes.NearFarPlanes.m._10));
    Distances[1].v4.y = ClipPlanes.NearFarPlanes.m._31 + (Pos->z * ClipPlanes.NearFarPlanes.m._21 + (Pos->x * ClipPlanes.NearFarPlanes.m._01 + Pos->y * ClipPlanes.NearFarPlanes.m._11));
    NuVecMtxTransform(&Radii[0].v, (NUVEC *)Extents, &ClipPlanes.AbsFrustrumPlanes.m);
    Radii[0].v4.w = Extents->x * ClipPlanes.AbsFrustrumPlanes.m._03 + Extents->y * ClipPlanes.AbsFrustrumPlanes.m._13 + Extents->z * ClipPlanes.AbsFrustrumPlanes.m._23;
    Radii[1].v4.x = Extents->x * ClipPlanes.NearFarPlanes.m._02 + Extents->y * ClipPlanes.NearFarPlanes.m._12 + Extents->z * ClipPlanes.NearFarPlanes.m._22;
    Radii[1].v4.y = Extents->x * ClipPlanes.NearFarPlanes.m._03 + Extents->y * ClipPlanes.NearFarPlanes.m._13 + Extents->z * ClipPlanes.NearFarPlanes.m._23;
    ClipPlanes.NearFarPlanes.m._30 = OldFarClip;

    if (Distances[0].v4.x < -Radii[0].v4.x || Distances[0].v4.y < -Radii[0].v4.y ||
        Distances[0].v4.z < -Radii[0].v4.z || Distances[0].v4.w < -Radii[0].v4.w ||
        Distances[1].v4.x < -Radii[1].v4.x || Distances[1].v4.y < -Radii[1].v4.y) {
        return 0;
    }
    if (Distances[0].v4.x < Radii[0].v4.x || Distances[0].v4.y < Radii[0].v4.y || Distances[0].v4.z < Radii[0].v4.z ||
        Distances[0].v4.w < Radii[0].v4.w) {
        NuVecMtxTransform(&Distances[0].v, (NUVEC *)Pos, &ClipPlanes.ScissorPlanes.m);
        Distances[0].v4.w = ClipPlanes.ScissorPlanes.m._33 + (Pos->z * ClipPlanes.ScissorPlanes.m._23 + (Pos->x * ClipPlanes.ScissorPlanes.m._03 + Pos->y * ClipPlanes.ScissorPlanes.m._13));
        NuVecMtxTransform(&Radii[0].v, (NUVEC *)Extents, &ClipPlanes.AbsScissorPlanes.m);
        Radii[0].v4.w = Extents->x * ClipPlanes.AbsScissorPlanes.m._03 + Extents->y * ClipPlanes.AbsScissorPlanes.m._13 + Extents->z * ClipPlanes.AbsScissorPlanes.m._23 + ClipPlanes.AbsScissorPlanes.m._33;
        if (Distances[0].v4.x < Radii[0].v4.x || Distances[0].v4.y < Radii[0].v4.y ||
            Distances[0].v4.z < Radii[0].v4.z || Distances[0].v4.w < Radii[0].v4.w) {
            return 2;
        }
    }
    return 1;
}

// FUNCTION: BIONICLE 0x539c90
void NuCameraSetEx(NUCAMERA *camera, int fast) {
    NUMTX mci;
    NUMTX mri;
    NUMTX minv;
    int ang;
    float l;
    float la;

    global_camera = *camera;
    FaceYDirStream(NuAtan2D(-global_camera.mtx._20, -global_camera.mtx._22));
    if (!fast) {
        NuVpUpdate();
    }
    if (camfx == NUCAMFX_NONE) {
        NuMtxInvVU0(&vmtx, &global_camera.mtx);
        NuMtxScaleVU0(&vmtx, &global_camera.scale);
    } else {
        NuMtxCopy(&minv, &numtx_identity);
        minv._11 = -1.0f;
        NuMtxInv(&mci, &global_camera.mtx);
        NuMtxInv(&mri, &global_reflect.mtx);
        vmtx = mri;
        if (camfx == NUCAMFX_REFLECT) {
            NuMtxMul(&vmtx, &vmtx, &minv);
        }
        NuMtxMul(&vmtx, &vmtx, &global_reflect.mtx);
        NuMtxMul(&vmtx, &vmtx, &mci);
    }
    if (!fast) {
        NuCameraSetProjectionMtx(&pmtx, global_camera.fov, global_camera.aspect, global_camera.nearclip,
                                 global_camera.farclip);
        pmtx._00 *= vport_hrdump_xscale;
        pmtx._20 = vport_hrdump_xoffset;
        pmtx._21 = vport_hrdump_yoffset;
        pmtx._11 *= vport_hrdump_yscale;
    }
    NuCameraSetVPortClipMtx(&vpc_vport_mtx, &vmtx, &global_camera, fast);
    if (clip_scissor_to_viewport != current_clip_scissor_to_viewport) {
        fast = 0;
        current_clip_scissor_to_viewport = clip_scissor_to_viewport;
    }
    if (clip_scissor_to_viewport) {
        NuCameraSetVPortClipMtx(&vpc_sci_mtx, &vmtx, &global_camera, fast);
    } else {
        NuCameraSetScissorClipMtx(&vpc_sci_mtx, &vmtx, &global_camera, fast);
    }
    NuVpGetScalingMtx(&smtx);
    NuMtxMulH(&vpmtx, &vmtx, &pmtx);
    NuMtxMulH(&vpsmtx, &vpmtx, &smtx);
    NuMtxMulH(&psmtx, &pmtx, &smtx);
    if (!fast) {
        NuCameraBuildClipPlanes();
        ang = (int)((global_camera.fov * 0.5f) * 10430.378f);
        zy = NU_SIN_LUT(ang) / NU_COS_LUT(ang);
        zx = zy / global_camera.aspect;
        clip_planes._12 = NU_COS_LUT(ang);
        clip_planes._22 = NU_SIN_LUT(ang);
        clip_planes._02 = 0.0f;
        clip_planes._32 = 0.0f;
        clip_planes._03 = 0.0f;
        clip_planes._23 = clip_planes._22;
        clip_planes._33 = 0.0f;
        clip_planes._13 = -clip_planes._12;
        la = 1.0f / clip_planes._12 / camera->aspect * clip_planes._22;
        l = 1.0f / NuFsqrt(la * la + 1.0f);
        clip_planes._10 = 0.0f;
        clip_planes._30 = 0.0f;
        clip_planes._11 = 0.0f;
        clip_planes._31 = 0.0f;
        clip_planes._00 = -l;
        clip_planes._20 = la * l;
        clip_planes._01 = l;
        clip_planes._21 = la * l;
        zxs = PS2_SREZ_W * zx / PS2_REZ_W;
        zys = PS2_SREZ_H * zy / PS2_REZ_H;
    }
    NuRndrSetViewMtx(&vpsmtx, &vpc_vport_mtx, &vpc_sci_mtx);
    NuRndrSetSpecularLight(NULL, NULL);
    NuRndrStateUpdateCameraState();
}

// FUNCTION: BIONICLE 0x53a030
void NuMtxCalcCheapFaceOn(NUMTX *mtx, NUVEC *pos) {
    NUMTX *viewmtx = &vmtx;

    mtx->_00 = -viewmtx->_00;
    mtx->_10 = viewmtx->_01;
    mtx->_20 = -viewmtx->_02;
    mtx->_01 = -viewmtx->_10;
    mtx->_11 = viewmtx->_11;
    mtx->_21 = -viewmtx->_12;
    mtx->_02 = -viewmtx->_20;
    mtx->_12 = viewmtx->_21;
    mtx->_22 = -viewmtx->_22;
    mtx->_23 = 0.0f;
    mtx->_13 = 0.0f;
    mtx->_03 = 0.0f;
    mtx->_33 = 1.0f;
    mtx->_30 = pos->x;
    mtx->_31 = pos->y;
    mtx->_32 = pos->z;
}

// FUNCTION: BIONICLE 0x53a0c0
void NuMtxCalcDebrisFaceOn(NUMTX *mtx) {
    NUMTX *viewmtx = &vmtx;

    mtx->_00 = -viewmtx->_00;
    mtx->_10 = viewmtx->_01;
    mtx->_20 = -viewmtx->_02;
    mtx->_01 = -viewmtx->_10;
    mtx->_11 = viewmtx->_11;
    mtx->_21 = -viewmtx->_12;
    mtx->_02 = -viewmtx->_20;
    mtx->_12 = viewmtx->_21;
    mtx->_22 = -viewmtx->_22;
    mtx->_23 = 0.0f;
    mtx->_13 = 0.0f;
    mtx->_03 = 0.0f;
    mtx->_33 = 1.0f;
}
