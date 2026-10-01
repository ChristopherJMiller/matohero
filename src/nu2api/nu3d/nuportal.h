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

/* nuportal_gen.c statics, stubbed in nu3d_stubs.c until decompiled. */
void drawPortal(NUPORTAL *p, NUROOM *room);
void drawViewFrustrum(void);
NUFRUSTRUM *buildFrustrum(NUVEC *min, NUVEC *max, short camera_roomid);
int clipPortal(NUPORTAL *portal, NUROOM *room, int roomid, NUFRUSTRUM *frust, NUFRUSTRUM **newfrust);
NUFRUSTRUM *createPortalFrustrum(NUPORTAL *portal, int room_id);

/* PS2: nu3d/ps2/nuportal_ps2.c, not decompiled yet. */
void Initialise_PS(NUGSCN *gsc);
void FlagRoomInstancesAsVisible(NUROOM *room, NUGSCN *gsc);
void SetAllInstancesVisible(NUGSCN *gsc);
void SetAllInstancesHidden(NUGSCN *gsc);
void clipRoomAgainstFrustrum(NUGSCN *gsc, NUROOM *room, NUFRUSTRUM *frust);
void NuRndrSphere(NUVEC *pos, float radius, unsigned int col, float yscale);

#endif
