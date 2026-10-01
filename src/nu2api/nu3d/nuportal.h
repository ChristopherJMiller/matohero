#ifndef NUPORTAL_H
#define NUPORTAL_H

#include "nugscn.h"

extern int portals_enabled;
extern NUVEC4 cam_plane;

void NuPortalInit(void);
int NuPortalEnabled(int on);
short NuPortalNumRooms(NUGSCN *gsc);
short NuPortalWhichRoom(NUGSCN *gsc, NUVEC *campos);
int clipTestSphere(NUVEC4 *sphere, NUFRUSTRUM *frust);
void NuPortalRoomClipTestAll(NUGSCN *gsc, char *RoomList);
int NuPortalClipTest(NUGSCN *gsc, NUVEC *centre, float radius, short roomid);
int NuPortalVisibility(NUGSCN *gsc);
int clipTestBox(NUVEC4 *min, NUVEC4 *max, NUVEC4 *frust, int nplanes);

/* PS2: nu3d/ps2/nuportal_ps2.c, not decompiled yet. */
void Initialise_PS(NUGSCN *gsc);
void FlagRoomInstancesAsVisible(NUROOM *room, NUGSCN *gsc);
void SetAllInstancesVisible(NUGSCN *gsc);
void SetAllInstancesHidden(NUGSCN *gsc);
void clipRoomAgainstFrustrum(NUGSCN *gsc, NUROOM *room, NUFRUSTRUM *frust);
void NuRndrSphere(NUVEC *pos, float radius, unsigned int col, float yscale);

#endif
