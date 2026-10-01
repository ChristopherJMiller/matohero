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

// FUNCTION: BIONICLE 0x53f670
short NuPortalNumRooms(NUGSCN *gsc) {
    if (gsc != NULL) {
        return gsc->portalinfo.nrooms;
    }
    return 0;
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
