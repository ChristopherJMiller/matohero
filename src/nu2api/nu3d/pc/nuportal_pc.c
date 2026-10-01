#include "nuportal.h"

#include <string.h>

/* PC counterpart of the PS2's nu3d/ps2/nuportal_ps2.c (file name ours): marks the
 * instances the portal walk finds visible. Scenes with a display list keep one bit per
 * instance in PortalVisiFlags; the others set the instances' visitest flags. */

// GLOBAL: BIONICLE 0x357d6c0
unsigned char PortalVisiFlags[625];

/* clipRoomAgainstFrustrum for scenes without a display list. The PS2 has an `instance`
 * pointer local; indexing gsc->instances each time is closer to PC's registers (46%: the
 * original keeps i in ebx and gsc in ebp, the other way round, and tests the flags as a
 * byte). */
// FUNCTION: BIONICLE 0x55b3c0
void clipRoomAgainstFrustrum_nodisplaylist(NUGSCN *gsc, NUROOM *room, NUFRUSTRUM *frust) {
    unsigned int i;
    unsigned int inst;
    int pvis;

    for (i = 0; i < room->ninstances; i++) {
        inst = room->instances[i];
        if (gsc->instances[inst].anim != NULL || gsc->instances[inst].flags.isspecial) {
            gsc->instances[inst].flags.visitest = 1;
        } else if (!gsc->instances[inst].flags.visitest) {
            pvis = clipTestSphere(&gsc->bounds.spheres[inst], frust);
            switch (pvis) {
            case 1:
                gsc->instances[inst].flags.visitest = 1;
                break;
            case 2:
                if (clipTestBox(&gsc->bounds.boxes[inst * 2], &gsc->bounds.boxes[inst * 2 + 1], frust->planes,
                                frust->nplanes)) {
                    gsc->instances[inst].flags.visitest = 1;
                }
                break;
            }
        }
    }
}

// FUNCTION: BIONICLE 0x55b470
void Initialise_PS(NUGSCN *gsc) {
    gsc->visibility.PortalVisiFlags = PortalVisiFlags;
}

/* The camera's own room: all its instances are visible. */
// FUNCTION: BIONICLE 0x55b480
void FlagRoomInstancesAsVisible(NUROOM *room, NUGSCN *gsc) {
    int i;

    if (gsc->display_list != NULL) {
        for (i = 0; i < room->ninstances; i++) {
            PortalVisiFlags[room->instances[i] >> 3] |= 1 << (room->instances[i] & 7);
        }
    } else {
        for (i = 0; i < room->ninstances; i++) {
            gsc->instances[room->instances[i]].flags.visitest = 1;
        }
    }
}

// FUNCTION: BIONICLE 0x55b500
void SetAllInstancesVisible(NUGSCN *gsc) {
    int i;

    if (gsc->display_list != NULL) {
        memset(PortalVisiFlags, 0xff, sizeof(PortalVisiFlags));
    } else {
        for (i = 0; i < gsc->numinstance; i++) {
            gsc->instances[i].flags.visitest = 1;
        }
    }
}

// FUNCTION: BIONICLE 0x55b550
void SetAllInstancesHidden(NUGSCN *gsc) {
    int i;

    if (gsc->display_list != NULL) {
        memset(PortalVisiFlags, 0, sizeof(PortalVisiFlags));
    } else {
        for (i = 0; i < gsc->numinstance; i++) {
            gsc->instances[i].flags.visitest = 0;
        }
    }
}

/* Marks the room's instances inside the frustum (sphere test, then the box when the
 * sphere straddles a plane). Instances flagged special in the display list always count. */
// FUNCTION: BIONICLE 0x55b5a0
void clipRoomAgainstFrustrum(NUGSCN *gsc, NUROOM *room, NUFRUSTRUM *frust) {
    unsigned int i;
    unsigned int inst;
    int pvis;

    if (gsc->display_list == NULL) {
        clipRoomAgainstFrustrum_nodisplaylist(gsc, room, frust);
        return;
    }
    for (i = 0; i < room->ninstances; i++) {
        inst = room->instances[i];
        if ((gsc->display_list->SpecialFlags[inst >> 3] >> (inst & 7)) & 1) {
            PortalVisiFlags[inst >> 3] |= 1 << (inst & 7);
        } else if (!((PortalVisiFlags[inst >> 3] >> (inst & 7)) & 1)) {
            pvis = clipTestSphere(&gsc->bounds.spheres[inst], frust);
            switch (pvis) {
            case 1:
                PortalVisiFlags[inst >> 3] |= 1 << (inst & 7);
                break;
            case 2:
                if (clipTestBox(&gsc->bounds.boxes[inst * 2], &gsc->bounds.boxes[inst * 2 + 1], frust->planes,
                                frust->nplanes)) {
                    PortalVisiFlags[inst >> 3] |= 1 << (inst & 7);
                }
                break;
            }
        }
    }
}
