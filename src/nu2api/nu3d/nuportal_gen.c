#include "nuportal.h"

#include "folded.h"
#include "nucamera.h"
#include "nuplane.h"
#include "nurndr.h"
#include "nutrig.h"
#include "nuvec.h"
#include "nuvec4.h"
#include "nuvu0.h"

#include <float.h>

/* Portal visibility: rooms joined by portals, walked from the camera's room through the
 * portals in view, narrowing a frustum at each step. Names, types and locals from the PS2
 * prototype; logic checked against opensagadev/saga src/nu2api/nu3d/nuportal.cpp
 * (GPL-3.0, a later engine). The PC scene embeds the NUPORTALINFO (at 0x9c). */

// GLOBAL: BIONICLE 0x616bb4
int portals_enabled = 1;

// GLOBAL: BIONICLE 0x616bb8
static int portal_inst = -1;

// GLOBAL: BIONICLE 0x337c8b4
static NUVEC *override_campos;

// GLOBAL: BIONICLE 0x337a3d0
static NUMTX local_inv_view_mtx;

// GLOBAL: BIONICLE 0x337c890
static NUVEC4 near_clip_plane;

// GLOBAL: BIONICLE 0x337c8a0
static NUVEC world_campos;

// GLOBAL: BIONICLE 0x337c8ac
static short camera_roomid;

// GLOBAL: BIONICLE 0x337a468
static NUCAMERA *cam;

// GLOBAL: BIONICLE 0x337c8b0
static int draw_portals;

// GLOBAL: BIONICLE 0x3604fd0
NUVEC4 cam_plane;

// GLOBAL: BIONICLE 0x3604fa0
short rooms_visited[18];

// GLOBAL: BIONICLE 0x337a4b0
static char fstackmem[9182];

// GLOBAL: BIONICLE 0x337a414
static VARIPTR fstack;

// GLOBAL: BIONICLE 0x337a464
static VARIPTR fstack_top;

// GLOBAL: BIONICLE 0x337a460
static VARIPTR fstack_limit;

// GLOBAL: BIONICLE 0x337a418
static int *nfrustra;

// GLOBAL: BIONICLE 0x337a410
static NUFRUSTRUM **frustra;

/* PS2 checks the stack limit and the frustum count here (NuError); PC doesn't. */
static __inline NUFRUSTRUM *NuFrustrumAllocate(int nplanes) {
    NUFRUSTRUM *f = (NUFRUSTRUM *)fstack_top.voidptr;

    fstack_top.intaddr = ((fstack_top.intaddr + 15) & ~15) + sizeof(NUFRUSTRUM);
    f->planes = (NUVEC4 *)fstack_top.voidptr;
    fstack_top.intaddr += nplanes * sizeof(NUVEC4);
    f->nplanes = nplanes;
    frustra[(*nfrustra)++] = f;
    return f;
}

// FUNCTION: BIONICLE 0x53ef60
int NuPortalEnabled(int on) {
    int old = portals_enabled;

    portals_enabled = on;
    return old;
}

// FUNCTION: BIONICLE 0x53ef70
void NuPortalInit(void) {
    fstack.s8 = fstackmem;
    fstack_top.intaddr = ((unsigned int)fstackmem + 15) & ~15;
    fstack_limit.s8 = fstackmem + sizeof(fstackmem);
}

/* Copies a frustum onto the stack (name from saga; inlined on PS2). The struct copy
 * overwrites the new planes pointer, so the plane loop copies the planes onto themselves. */
// FUNCTION: BIONICLE 0x53efa0
static NUFRUSTRUM *copyFrustrum(NUFRUSTRUM *from) {
    NUFRUSTRUM *to;
    int i;

    to = NuFrustrumAllocate(from->nplanes);
    *to = *from;
    for (i = 0; i < from->nplanes; i++) {
        to->planes[i] = from->planes[i];
    }
    return to;
}

/* Debug drawing, on when draw_portals is set (nothing sets it on PC). PS2 passes the
 * colours in; PC uses constants. */

/* The portal's outline, and a line from each corner back to the room wall the camera ray
 * through it hits first. */
// FUNCTION: BIONICLE 0x53f040
static void drawPortal(NUPORTAL *p, NUROOM *room) {
    int i;
    int j;
    NUVEC v;
    NUVEC *s;
    NUVEC *e;
    NUVEC ip;
    NUVEC min_ip;
    float min_dist;
    float dist;

    if (draw_portals == 0) {
        return;
    }
    NuCameraUnlock();
    for (i = 0; i < p->npoints; i++) {
        s = &p->points[i];
        e = &p->points[(i + 1) % p->npoints];
        NuVecSub(&v, s, e);
        NuRndrLine3dDbg(s->x, s->y, s->z, e->x, e->y, e->z, 0xffff00ff);
    }
    for (i = 0; i < p->npoints; i++) {
        NuVecSub(&v, &p->points[i], &world_campos);
        NuVecNorm(&v, &v);
        min_dist = FLT_MAX;
        for (j = 0; j < room->nplanes; j++) {
            dist = NuPlnLine3((NUPLANE *)&room->plane_eqns[j], &p->points[i], &v, &ip);
            if (dist > 0.0f && dist < min_dist) {
                min_dist = dist;
                min_ip = ip;
            }
        }
        NuRndrLine3dDbg(min_ip.x, min_ip.y, min_ip.z, p->points[i].x, p->points[i].y, p->points[i].z, 0xffff00ff);
    }
    NuCameraRelock();
}

/* A screen-space rectangle, min/max in clip space (-1..1). */
// FUNCTION: BIONICLE 0x53f1d0
static void drawSquare(NUVEC *min, NUVEC *max, unsigned int colour) {
    NUVTX_TLTC1 v[8];

    v[0].pnt.x = (min->x + 1.0f) * 0.5f;
    v[0].pnt.y = 0.5f - min->y * 0.5f;
    v[0].pnt.z = 1.0f;
    v[0].diffuse = colour;
    v[1].pnt.x = (min->x + 1.0f) * 0.5f;
    v[1].pnt.y = 0.5f - max->y * 0.5f;
    v[1].pnt.z = 1.0f;
    v[1].diffuse = colour;
    v[2].pnt.x = (min->x + 1.0f) * 0.5f;
    v[2].pnt.y = 0.5f - max->y * 0.5f;
    v[2].pnt.z = 1.0f;
    v[2].diffuse = colour;
    v[3].pnt.x = (max->x + 1.0f) * 0.5f;
    v[3].pnt.y = 0.5f - max->y * 0.5f;
    v[3].pnt.z = 1.0f;
    v[3].diffuse = colour;
    v[4].pnt.x = (max->x + 1.0f) * 0.5f;
    v[4].pnt.y = 0.5f - max->y * 0.5f;
    v[4].pnt.z = 1.0f;
    v[4].diffuse = colour;
    v[5].pnt.x = (max->x + 1.0f) * 0.5f;
    v[5].pnt.y = 0.5f - min->y * 0.5f;
    v[5].pnt.z = 1.0f;
    v[5].diffuse = colour;
    v[6].pnt.x = (max->x + 1.0f) * 0.5f;
    v[6].pnt.y = 0.5f - min->y * 0.5f;
    v[6].pnt.z = 1.0f;
    v[6].diffuse = colour;
    v[7].pnt.x = (min->x + 1.0f) * 0.5f;
    v[7].pnt.y = 0.5f - min->y * 0.5f;
    v[7].pnt.z = 1.0f;
    v[7].diffuse = colour;
    NuRndrLineStrip2d(v, NULL, 8);
}

/* The view frustum's edges, 0.2 units out from the camera. */
// FUNCTION: BIONICLE 0x53f300
static void drawViewFrustrum(void) {
    NUVEC c[4];
    NUVEC *a;
    NUVEC *b;
    NUVEC p[4];
    int i;

    if (draw_portals == 0) {
        return;
    }
    NuCameraRayCast(&c[0], -1.0f, -1.0f);
    NuCameraRayCast(&c[1], -1.0f, 1.0f);
    NuCameraRayCast(&c[2], 1.0f, 1.0f);
    NuCameraRayCast(&c[3], 1.0f, -1.0f);
    for (i = 0; i < 4; i++) {
        NuVecScale(&p[i], &c[i], 0.2f);
        NuVecAdd(&p[i], &p[i], &world_campos);
    }
    NuCameraUnlock();
    for (i = 0; i < 4; i++) {
        a = &p[i];
        b = &p[(i + 1) % 4];
        NuRndrLine3dDbg(world_campos.x, world_campos.y, world_campos.z, a->x, a->y, a->z, 0xffffffff);
        NuRndrLine3dDbg(a->x, a->y, a->z, b->x, b->y, b->z, 0xffff0000);
    }
    NuCameraRelock();
}

/* The portal clipped to the frustum: on screen, on the portal's plane, and the room walls
 * behind it. */
// FUNCTION: BIONICLE 0x53f420
static void drawClippedPortal(NUPORTAL *portal, NUROOM *room, NUVEC *min, NUVEC *max) {
    NUVEC c[4];
    NUVEC s[4];
    NUVEC ip;
    NUVEC min_ip;
    NUVEC *a;
    NUVEC *b;
    int i;
    int j;
    float min_dist;
    float dist;

    if (draw_portals == 0) {
        return;
    }
    NuCameraRayCast(&c[0], min->x, min->y);
    NuCameraRayCast(&c[1], min->x, max->y);
    NuCameraRayCast(&c[2], max->x, max->y);
    NuCameraRayCast(&c[3], max->x, min->y);
    for (i = 0; i < 4; i++) {
        NuPlnLine3((NUPLANE *)&portal->plane_eqn, &world_campos, &c[i], &s[i]);
    }
    NuCameraUnlock();
    drawSquare(min, max, 0xff00);
    for (i = 0; i < 4; i++) {
        a = &s[i];
        b = &s[(i + 1) % 4];
        NuRndrLine3dDbg(a->x, a->y, a->z, b->x, b->y, b->z, 0xff0000ff);
    }
    for (i = 0; i < 4; i++) {
        NuVecNorm(&c[i], &c[i]);
        min_dist = FLT_MAX;
        for (j = 0; j < room->nplanes; j++) {
            dist = NuPlnLine3((NUPLANE *)&room->plane_eqns[j], &s[i], &c[i], &ip);
            if (dist > 0.0f && dist < min_dist) {
                min_dist = dist;
                min_ip = ip;
            }
        }
        NuRndrLine3dDbg(min_ip.x, min_ip.y, min_ip.z, s[i].x, s[i].y, s[i].z, 0xff0000ff);
    }
    NuCameraRelock();
}

/* 1 if the point is inside all the room's planes (name ours; inlined on PS2). */
// FUNCTION: BIONICLE 0x53f600
static short PointInRoom(NUROOM *r, NUVEC *campos) {
    short j;
    float test_sign = 0.0f;

    for (j = 0; j < r->nplanes; j++) {
        test_sign = r->plane_eqns[j].x * campos->x + r->plane_eqns[j].y * campos->y +
                    r->plane_eqns[j].z * campos->z + r->plane_eqns[j].w;
        if (test_sign > 0.0f) {
            break;
        }
    }
    if (test_sign <= 0.0f) {
        return 1;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x53f670
short NuPortalNumRooms(NUGSCN *gsc) {
    if (gsc != NULL) {
        return gsc->portalinfo.nrooms;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x53f690
short NuPortalWhichRoom(NUGSCN *gsc, NUVEC *campos) {
    NUPORTALINFO *p;
    NUROOM *room0;
    NUROOM *room1;
    short i;
    short j;
    short k;
    short p1;
    short room_list[2];
    short room_cnt = 0;
    NUVEC4 *pln;
    float test_sign;

    if (gsc == NULL || campos == NULL) {
        return -1;
    }
    p = &gsc->portalinfo;
    room_list[0] = 0;
    room_list[1] = 0;
    for (i = 0; i < p->nrooms; i++) {
        if (PointInRoom(&p->rooms[i], campos)) {
            room0 = &p->rooms[room_list[0]];
            room1 = &p->rooms[room_list[1]];
            if (room_cnt == 2) {
                if (room0->depth < room1->depth) {
                    room_list[0] = i;
                } else {
                    room_list[1] = i;
                }
            }
            room_list[room_cnt++] = i;
            if (room_cnt == 2 && !room0->contains_room && !room1->contains_room) {
                break;
            }
        }
    }
    if (room_cnt == 1) {
        return room_list[0];
    }
    if (room_cnt == 0) {
        return -1;
    }

    /* In two rooms: decide by the side of their shared portal. */
    room0 = &p->rooms[room_list[0]];
    room1 = &p->rooms[room_list[1]];
    for (j = 0; j < room0->nportals; j++) {
        p1 = room0->portals[j];
        for (k = 0; k < room1->nportals; k++) {
            if (p1 == room1->portals[k]) {
                pln = &p->portals[p1].plane_eqn;
                test_sign = pln->x * campos->x + pln->y * campos->y + pln->z * campos->z + pln->w;
                if (test_sign < 0.0f) {
                    return p->portals[p1].left_room;
                }
                return p->portals[p1].right_room;
            }
        }
    }
    return -1;
}

// FUNCTION: BIONICLE 0x53f860
int clipTestSphere(NUVEC4 *sphere, NUFRUSTRUM *frust) {
    int i;
    int inside;
    float dot;

    inside = 0;
    for (i = 0; i < frust->nplanes; i++) {
        dot = frust->planes[i].x * sphere->x + frust->planes[i].y * sphere->y + frust->planes[i].z * sphere->z +
              frust->planes[i].w;
        if (dot < -sphere->w) {
            return 0;
        }
        if (dot > sphere->w) {
            inside++;
        }
    }
    dot = cam_plane.x * sphere->x + cam_plane.y * sphere->y + cam_plane.z * sphere->z + cam_plane.w;
    if (!(dot < -sphere->w)) {
        if (dot > sphere->w) {
            inside++;
        }
        return inside == frust->nplanes + 1 ? 1 : 2;
    }
    return 0;
}

/* One corner of the box against plane i (PS2 writes the eight tests out). */
#define CLIPTEST_CORNER(n)                                                                             \
    dot = frust[i].x * p[n].x + (frust[i].y * p[n].y + frust[i].z * p[n].z) + frust[i].w;          \
    if (dot < 0.0f) {                                                                                  \
        outside++;                                                                                     \
    } else {                                                                                           \
        inside++;                                                                                      \
    }

// FUNCTION: BIONICLE 0x53f920
int clipTestBox(NUVEC4 *min, NUVEC4 *max, NUVEC4 *frust, int nplanes) {
    int i;
    int inside;
    int outside;
    float dot;
    NUVEC p[8];

    p[0].x = min->x;
    p[0].y = min->y;
    p[0].z = min->z;
    p[1].x = max->x;
    p[1].y = min->y;
    p[1].z = min->z;
    p[2].x = min->x;
    p[2].y = max->y;
    p[2].z = min->z;
    p[3].x = max->x;
    p[3].y = max->y;
    p[3].z = min->z;
    p[4].x = min->x;
    p[4].y = min->y;
    p[4].z = max->z;
    p[5].x = max->x;
    p[5].y = min->y;
    p[5].z = max->z;
    p[6].x = min->x;
    p[6].y = max->y;
    p[6].z = max->z;
    p[7].x = max->x;
    p[7].y = max->y;
    p[7].z = max->z;
    inside = 0;
    for (i = 0; i < nplanes; i++) {
        outside = 0;
        CLIPTEST_CORNER(0)
        CLIPTEST_CORNER(1)
        CLIPTEST_CORNER(2)
        CLIPTEST_CORNER(3)
        CLIPTEST_CORNER(4)
        CLIPTEST_CORNER(5)
        CLIPTEST_CORNER(6)
        CLIPTEST_CORNER(7)
        if (outside == 8) {
            return 0;
        }
    }
    return inside == nplanes * 8 ? 1 : 2;
}

/* Writes the planes into the frustum's transposed form, four per matrix, padding the
 * unused slots with the camera plane. */
// FUNCTION: BIONICLE 0x53fb80
static void transposeClipPlanes(NUFRUSTRUM *f) {
    f->clip_planes[0]._03 = cam_plane.x;
    f->clip_planes[0]._13 = cam_plane.y;
    f->clip_planes[0]._23 = cam_plane.z;
    f->clip_planes[0]._33 = cam_plane.w;
    f->clip_planes[1]._00 = cam_plane.x;
    f->clip_planes[1]._10 = cam_plane.y;
    f->clip_planes[1]._20 = cam_plane.z;
    f->clip_planes[1]._30 = cam_plane.w;
    f->clip_planes[1]._01 = cam_plane.x;
    f->clip_planes[1]._11 = cam_plane.y;
    f->clip_planes[1]._21 = cam_plane.z;
    f->clip_planes[1]._31 = cam_plane.w;
    f->clip_planes[1]._02 = cam_plane.x;
    f->clip_planes[1]._12 = cam_plane.y;
    f->clip_planes[1]._22 = cam_plane.z;
    f->clip_planes[1]._32 = cam_plane.w;
    f->clip_planes[1]._03 = cam_plane.x;
    f->clip_planes[1]._13 = cam_plane.y;
    f->clip_planes[1]._23 = cam_plane.z;
    f->clip_planes[1]._33 = cam_plane.w;
    f->clip_planes[0]._00 = f->planes[0].x;
    f->clip_planes[0]._10 = f->planes[0].y;
    f->clip_planes[0]._20 = f->planes[0].z;
    f->clip_planes[0]._30 = f->planes[0].w;
    f->clip_planes[0]._01 = f->planes[1].x;
    f->clip_planes[0]._11 = f->planes[1].y;
    f->clip_planes[0]._21 = f->planes[1].z;
    f->clip_planes[0]._31 = f->planes[1].w;
    f->clip_planes[0]._02 = f->planes[2].x;
    f->clip_planes[0]._12 = f->planes[2].y;
    f->clip_planes[0]._22 = f->planes[2].z;
    f->clip_planes[0]._32 = f->planes[2].w;
    if (f->nplanes < 4) {
        return;
    }
    f->clip_planes[0]._03 = f->planes[3].x;
    f->clip_planes[0]._13 = f->planes[3].y;
    f->clip_planes[0]._23 = f->planes[3].z;
    f->clip_planes[0]._33 = f->planes[3].w;
    if (f->nplanes < 5) {
        return;
    }
    f->clip_planes[1]._00 = f->planes[4].x;
    f->clip_planes[1]._10 = f->planes[4].y;
    f->clip_planes[1]._20 = f->planes[4].z;
    f->clip_planes[1]._30 = f->planes[4].w;
    if (f->nplanes < 6) {
        return;
    }
    f->clip_planes[1]._01 = f->planes[5].x;
    f->clip_planes[1]._11 = f->planes[5].y;
    f->clip_planes[1]._21 = f->planes[5].z;
    f->clip_planes[1]._31 = f->planes[5].w;
    if (f->nplanes < 7) {
        return;
    }
    f->clip_planes[1]._02 = f->planes[6].x;
    f->clip_planes[1]._12 = f->planes[6].y;
    f->clip_planes[1]._22 = f->planes[6].z;
    f->clip_planes[1]._32 = f->planes[6].w;
    if (f->nplanes < 8) {
        return;
    }
    f->clip_planes[1]._03 = f->planes[7].x;
    f->clip_planes[1]._13 = f->planes[7].y;
    f->clip_planes[1]._23 = f->planes[7].z;
    f->clip_planes[1]._33 = f->planes[7].w;
}

/* The view frustum through a screen rectangle (min/max in clip space). */
// FUNCTION: BIONICLE 0x53fd30
static NUFRUSTRUM *buildFrustrum(NUVEC *min, NUVEC *max, short camera_roomid) {
    int ang;
    float zx;
    float zy;
    float l;
    float v;
    NUVEC4 p;
    NUVEC4 *f;
    NUFRUSTRUM *frust;

    frust = NuFrustrumAllocate(4);
    frust->min = *min;
    frust->max = *max;
    frust->roomid = camera_roomid;
    f = frust->planes;
    ang = (int)((cam->fov * 0.5f) * 10430.378f);
    zy = NU_SIN_LUT(ang) / NU_COS_LUT(ang);
    zx = zy / cam->aspect;

    v = zy * min->y;
    l = 1.0f / NuFsqrt(v * v + 1.0f);
    f[2].x = 0.0f;
    f[2].y = l;
    f[2].z = -(l * v);
    f[2].w = 0.0f;

    v = zy * max->y;
    l = 1.0f / NuFsqrt(v * v + 1.0f);
    f[3].x = 0.0f;
    f[3].y = -l;
    f[3].z = l * v;
    f[3].w = 0.0f;

    v = zx * max->x;
    l = 1.0f / NuFsqrt(v * v + 1.0f);
    f[0].x = -l;
    f[0].z = l * v;
    f[0].y = 0.0f;
    f[0].w = 0.0f;

    v = zx * min->x;
    l = 1.0f / NuFsqrt(v * v + 1.0f);
    f[1].y = 0.0f;
    f[1].x = l;
    f[1].w = 0.0f;
    f[1].z = -(l * v);

    /* Into world space. */
    p.x = local_inv_view_mtx._30;
    p.y = local_inv_view_mtx._31;
    p.z = local_inv_view_mtx._32;
    NuVec4MtxTransformH(&f[0], &f[0], &local_inv_view_mtx);
    f[0].w = -(p.x * f[0].x + p.y * f[0].y + p.z * f[0].z);
    NuVec4MtxTransformH(&f[1], &f[1], &local_inv_view_mtx);
    f[1].w = -(p.x * f[1].x + p.y * f[1].y + p.z * f[1].z);
    NuVec4MtxTransformH(&f[2], &f[2], &local_inv_view_mtx);
    f[2].w = -(p.x * f[2].x + p.y * f[2].y + p.z * f[2].z);
    NuVec4MtxTransformH(&f[3], &f[3], &local_inv_view_mtx);
    f[3].w = -(p.x * f[3].x + p.y * f[3].y + p.z * f[3].z);
    transposeClipPlanes(frust);
    return frust;
}

/* Debug: draws the bounding sphere of instance i (inlined on PS2; name ours). */
// FUNCTION: BIONICLE 0x53ffe0
static void drawInstanceSphere(int i, NUGSCN *gsc) {
    NUVEC4 *sphere;

    if (i != -1 && i < gsc->bounds.nspheres) {
        sphere = &gsc->bounds.spheres[i];
        NuRndrSphere((NUVEC *)sphere, sphere->w, 0x400000, 1.0f);
    }
}

/* 0 if the portal is out of view, 1 if inside all the frustum's planes, 2 if crossing. */
// FUNCTION: BIONICLE 0x540020
static int checkPortalVisibility(NUPORTAL *portal, NUFRUSTRUM *frust, NUGSCN *gsc, short roomid, short next_room) {
    int i;
    int j;
    int inside;
    int outside;
    float dot;
    float test_sgn;

    if (gsc->portalinfo.rooms[roomid].depth < gsc->portalinfo.rooms[next_room].depth) {
        test_sgn = portal->plane_eqn.x * world_campos.x + portal->plane_eqn.y * world_campos.y +
                   portal->plane_eqn.z * world_campos.z + portal->plane_eqn.w;
        if (portal->right_room == roomid && test_sgn < 0.0f) {
            return 0;
        }
        if (portal->left_room == roomid && test_sgn > 0.0f) {
            return 0;
        }
    }
    inside = 0;
    for (i = 0; i < frust->nplanes; i++) {
        outside = 0;
        for (j = 0; j < portal->npoints; j++) {
            dot = frust->planes[i].x * portal->points[j].x + frust->planes[i].y * portal->points[j].y +
                  frust->planes[i].z * portal->points[j].z + frust->planes[i].w;
            if (dot >= 0.0f) {
                inside++;
            } else {
                outside++;
            }
        }
        if (outside == portal->npoints) {
            return 0;
        }
    }
    return inside == frust->nplanes * portal->npoints ? 1 : 2;
}

/* The portal's bounding rectangle in clip space, clamped to the screen (name from saga;
 * inlined on PS2). */
// FUNCTION: BIONICLE 0x540280
static void computePortalExtents(NUPORTAL *portal, NUVEC *min, NUVEC *max) {
    // GLOBAL: BIONICLE 0x337a470
    static NUMTX inv_clip_mtx;
    // GLOBAL: BIONICLE 0x337a420
    static NUMTX clip;
    int i;
    NUVEC p;

    NuCameraGetClipMtx(&clip, NULL);
    NuMtxInvH(&inv_clip_mtx, &clip);
    NuVecMtxTransformH(max, &portal->points[0], &clip);
    *min = *max;
    for (i = 1; i < portal->npoints; i++) {
        NuVecMtxTransformH(&p, &portal->points[i], &clip);
        NuVecMax(max, max, &p);
        NuVecMin(min, min, &p);
    }
    NuVecMin(max, max, &nuvec_one);
    NuVecMax(max, max, &nuvec_minus_one);
    NuVecMin(min, min, &nuvec_one);
    NuVecMax(min, min, &nuvec_minus_one);
}

/* Intersects the rectangle mina..maxa with minc..maxc into min..max; 0 if they don't
 * overlap (name ours; inlined on PS2). */
// FUNCTION: BIONICLE 0x540370
static int clipExtents(NUVEC *min, NUVEC *max, NUVEC *mina, NUVEC *maxa, NUVEC *minc, NUVEC *maxc) {
    if (maxa->x <= minc->x || maxa->y <= minc->y || mina->x >= maxc->x || mina->y >= maxc->y) {
        return 0;
    }
    if (mina->x > minc->x) {
        min->x = mina->x;
    } else {
        min->x = minc->x;
    }
    if (mina->y > minc->y) {
        min->y = mina->y;
    } else {
        min->y = minc->y;
    }
    if (maxa->x < maxc->x) {
        max->x = maxa->x;
    } else {
        max->x = maxc->x;
    }
    if (maxa->y < maxc->y) {
        max->y = maxa->y;
    } else {
        max->y = maxc->y;
    }
    return 1;
}

/* 0: behind the camera; 4: crossing the camera plane (keep the frustum); 1: clipped to
 * the rectangle it covers, with a new frustum in *newfrust. */
// FUNCTION: BIONICLE 0x540430
static int clipPortal(NUPORTAL *portal, NUROOM *room, int roomid, NUFRUSTRUM *frust, NUFRUSTRUM **newfrust) {
    NUVEC max;
    NUVEC min;
    int i;
    int inside;
    float dot;
    NUVEC *p;

    inside = 0;
    for (i = 0; i < portal->npoints; i++) {
        p = &portal->points[i];
        dot = cam_plane.x * p->x + cam_plane.y * p->y + cam_plane.z * p->z + cam_plane.w;
        if (dot > 0.0f) {
            inside++;
        }
    }
    if (inside == 0) {
        return 0;
    }
    if (inside != portal->npoints) {
        return 4;
    }
    computePortalExtents(portal, &min, &max);
    if (!clipExtents(&min, &max, &min, &max, &frust->min, &frust->max)) {
        return 0;
    }
    drawClippedPortal(portal, room, &min, &max);
    *newfrust = buildFrustrum(&min, &max, roomid);
    return 1;
}

/* A frustum through the portal's edges from the camera (all of the portal in view). */
// FUNCTION: BIONICLE 0x540640
static NUFRUSTRUM *createPortalFrustrum(NUPORTAL *portal, int room_id) {
    NUVEC p;
    NUVEC ex;
    NUVEC ey;
    int i;
    float d;
    NUFRUSTRUM *frustrum;

    frustrum = NuFrustrumAllocate(portal->npoints);
    frustrum->roomid = room_id;
    p.x = local_inv_view_mtx._30;
    p.y = local_inv_view_mtx._31;
    p.z = local_inv_view_mtx._32;
    d = portal->plane_eqn.x * p.x + portal->plane_eqn.y * p.y + portal->plane_eqn.z * p.z + portal->plane_eqn.w;
    for (i = 0; i < portal->npoints; i++) {
        NuVecSub(&ex, &p, &portal->points[i]);
        NuVecSub(&ey, &p, &portal->points[(i + 1) % portal->npoints]);
        NuVecCross(&ex, &ex, &ey);
        NuVecNorm(&ex, &ex);
        if (d < 0.0f) {
            frustrum->planes[i].x = ex.x;
            frustrum->planes[i].y = ex.y;
            frustrum->planes[i].z = ex.z;
            frustrum->planes[i].w = -(ex.x * p.x + ex.y * p.y + ex.z * p.z);
        } else {
            frustrum->planes[i].x = -ex.x;
            frustrum->planes[i].y = -ex.y;
            frustrum->planes[i].z = -ex.z;
            frustrum->planes[i].w = ex.x * p.x + ex.y * p.y + ex.z * p.z;
        }
    }
    computePortalExtents(portal, &frustrum->min, &frustrum->max);
    transposeClipPlanes(frustrum);
    return frustrum;
}

// FUNCTION: BIONICLE 0x540840
void NuPortalRoomClipTestAll(NUGSCN *gsc, char *RoomList) {
    int i;
    int RoomId;

    if (gsc != NULL && gsc->portalinfo.nportals != 0) {
        for (i = 0; i < gsc->portalinfo.nrooms; i++) {
            RoomList[i] = 0;
        }
        RoomList[gsc->portalinfo.camera_roomid] = 1;
        for (i = 0; i < gsc->portalinfo.nfrustra; i++) {
            RoomId = gsc->portalinfo.frustra[i]->roomid;
            if (RoomId >= 0) {
                RoomList[RoomId] = 1;
            }
        }
    }
}

// FUNCTION: BIONICLE 0x5408c0
int NuPortalClipTest(NUGSCN *gsc, NUVEC *centre, float radius, short roomid) {
    NUPORTALINFO *p = &gsc->portalinfo;
    int i;
    int outcode;
    NUVEC4 v;

    if (p->nportals == 0) {
        return 1;
    }
    if (roomid == p->camera_roomid) {
        return 1;
    }
    for (i = 0; i < p->nfrustra; i++) {
        if (p->frustra[i] != NULL && p->frustra[i]->roomid == roomid) {
            v.x = centre->x;
            v.y = centre->y;
            v.z = centre->z;
            v.w = radius;
            outcode = clipTestSphere(&v, p->frustra[i]);
            if (outcode != 0) {
                return outcode;
            }
        }
    }
    return 0;
}

// FUNCTION: BIONICLE 0x540960
static void roomRecursive(NUGSCN *gsc, NUFRUSTRUM *frustrum, short roomid, short prev_room, int depth) {
    NUPORTAL *new_portal;
    NUROOM *room;
    unsigned int i;
    unsigned int pvis;
    int j;
    int clip;
    short next_room;
    NUFRUSTRUM *new_frustrum;

    room = &gsc->portalinfo.rooms[roomid];
    for (j = 0; j < depth - 1; j++) {
        if (rooms_visited[j] == roomid) {
            return;
        }
    }
    if (depth >= 16) {
        return;
    }
    rooms_visited[depth] = roomid;
    if (roomid == camera_roomid) {
        FlagRoomInstancesAsVisible(room, gsc);
    } else {
        clipRoomAgainstFrustrum(gsc, room, frustrum);
    }
    room->is_visible = 1;
    if (depth >= gsc->portalinfo.max_depth) {
        return;
    }
    for (i = 0; i < room->nportals; i++) {
        new_portal = &gsc->portalinfo.portals[room->portals[i]];
        if (new_portal->left_room != roomid) {
            next_room = new_portal->left_room;
        } else {
            next_room = new_portal->right_room;
        }
        if (next_room == prev_room) {
            continue;
        }
        if (new_portal->flags & 1) {
            pvis = checkPortalVisibility(new_portal, frustrum, gsc, roomid, next_room);
        } else {
            pvis = 0;
        }
        switch (pvis) {
        case 0:
            break;
        case 1:
            drawPortal(new_portal, &gsc->portalinfo.rooms[next_room]);
            new_frustrum = createPortalFrustrum(new_portal, next_room);
            roomRecursive(gsc, new_frustrum, next_room, roomid, depth + 1);
            break;
        case 2:
            clip = clipPortal(new_portal, &gsc->portalinfo.rooms[next_room], next_room, frustrum, &new_frustrum);
            switch (clip) {
            case 1:
                roomRecursive(gsc, new_frustrum, next_room, roomid, depth + 1);
                break;
            case 4:
                new_frustrum = copyFrustrum(frustrum);
                new_frustrum->roomid = next_room;
                roomRecursive(gsc, new_frustrum, next_room, roomid, depth + 1);
                break;
            }
            break;
        default:
            roomRecursive(gsc, frustrum, next_room, roomid, depth + 1);
            break;
        }
    }
}

// FUNCTION: BIONICLE 0x540b40
int NuPortalVisibility(NUGSCN *gsc) {
    unsigned int i;
    NUVEC p;
    NUVEC *pcampos;
    float nearclip;
    NUFRUSTRUM *view_frustrum;

    Initialise_PS(gsc);
    pcampos = override_campos != NULL ? override_campos : &world_campos;
    if (!portals_enabled) {
        SetAllInstancesVisible(gsc);
        return 0;
    }
    if (gsc->portalinfo.nportals == 0 || gsc->portalinfo.nrooms == 0 || gsc->bounds.nbounds == 0) {
        return 0;
    }
    nfrustra = &gsc->portalinfo.nfrustra;
    *nfrustra = 0;
    frustra = gsc->portalinfo.frustra;
    SetAllInstancesHidden(gsc);
    cam = NuCameraGetCam();
    local_inv_view_mtx = *NuCameraGetMtx();
    world_campos.x = global_camera.mtx._30;
    world_campos.y = global_camera.mtx._31;
    world_campos.z = global_camera.mtx._32;
    nearclip = cam->nearclip;
    camera_roomid = NuPortalWhichRoom(gsc, pcampos);
    gsc->portalinfo.camera_roomid = camera_roomid;
    if (camera_roomid == -1) {
        return 0;
    }

    /* The camera plane and the near clip plane, both facing along the view. */
    cam_plane.x = near_clip_plane.x = local_inv_view_mtx._20;
    cam_plane.y = near_clip_plane.y = local_inv_view_mtx._21;
    cam_plane.z = near_clip_plane.z = local_inv_view_mtx._22;
    p.x = local_inv_view_mtx._20 * nearclip + world_campos.x;
    p.y = local_inv_view_mtx._21 * nearclip + world_campos.y;
    p.z = local_inv_view_mtx._22 * nearclip + world_campos.z;
    near_clip_plane.w = -(local_inv_view_mtx._20 * p.x + local_inv_view_mtx._21 * p.y + local_inv_view_mtx._22 * p.z);
    cam_plane.w = -(local_inv_view_mtx._20 * world_campos.x + local_inv_view_mtx._21 * world_campos.y +
                    local_inv_view_mtx._22 * world_campos.z);
    view_frustrum = buildFrustrum(&nuvec_minus_one, &nuvec_one, -2);
    drawViewFrustrum();
    for (i = 0; i < gsc->portalinfo.nrooms; i++) {
        gsc->portalinfo.rooms[i].is_visible = 0;
    }
    roomRecursive(gsc, view_frustrum, camera_roomid, -1, 0);
    drawInstanceSphere(portal_inst, gsc);
    return 1;
}
