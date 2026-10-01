#include "nuportal.h"

#include "nucamera.h"
#include "nuvec.h"

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

/* 1 if the point is inside all the room's planes (name ours; inlined on PS2). */
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
        next_room = new_portal->left_room;
        if (next_room == roomid) {
            next_room = new_portal->right_room;
        }
        if (next_room == prev_room) {
            continue;
        }
        switch (new_portal->flags & 1 ? checkPortalVisibility(new_portal, frustrum, gsc, roomid, next_room) : 0) {
        case 0:
            break;
        case 1:
            drawPortal(new_portal, &gsc->portalinfo.rooms[next_room]);
            new_frustrum = createPortalFrustrum(new_portal, next_room);
            roomRecursive(gsc, new_frustrum, next_room, roomid, depth + 1);
            break;
        case 2:
            clip = clipPortal(new_portal, &gsc->portalinfo.rooms[next_room], next_room, frustrum, &new_frustrum);
            if (clip == 1) {
                roomRecursive(gsc, new_frustrum, next_room, roomid, depth + 1);
            } else if (clip == 4) {
                new_frustrum = copyFrustrum(frustrum);
                new_frustrum->roomid = next_room;
                roomRecursive(gsc, new_frustrum, next_room, roomid, depth + 1);
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
