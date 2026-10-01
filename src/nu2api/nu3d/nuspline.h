#ifndef NUSPLINE_H
#define NUSPLINE_H

#include "nugscn.h"

NUGSPLINE *NuSplineFind(NUGSCN *scene, char *name);
int NuSplineFindAllSub(NUGSCN *scene, char *name, NUGSPLINE **tab, int maxcount);
int NuSplineFindAllBeg(NUGSCN *scene, char *name, NUGSPLINE **tab, int maxcount);

#endif
