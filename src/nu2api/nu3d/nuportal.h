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

#endif
