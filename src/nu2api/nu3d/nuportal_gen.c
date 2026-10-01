#include "nuportal.h"

/* Portal visibility: rooms joined by portals, walked from the camera's room through the
 * portals in view, narrowing a frustum at each step. Names, types and locals from the PS2
 * prototype; logic checked against opensagadev/saga src/nu2api/nu3d/nuportal.cpp
 * (GPL-3.0, a later engine). The PC scene embeds the NUPORTALINFO (at 0x9c). */

// GLOBAL: BIONICLE 0x616bb4
int portals_enabled = 1;

// GLOBAL: BIONICLE 0x3604fd0
NUVEC4 cam_plane;

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
