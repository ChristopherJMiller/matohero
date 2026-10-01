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
int clipTestBox(NUVEC4 *min, NUVEC4 *max, NUVEC4 *frust, int nplanes);

#endif
