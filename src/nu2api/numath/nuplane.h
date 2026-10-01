#ifndef NUPLANE_H
#define NUPLANE_H

#include "numath.h"

float NuPlnLine3(NUPLANE *p, NUVEC *p1, NUVEC *v1, NUVEC *iv);
float NuInfiniteLineToPointDistSqrEx(NUVEC *s, NUVEC *e, NUVEC *pnt, NUVEC *lp);
float NuInfiniteLineToPointDistSqr(NUVEC *s, NUVEC *e, NUVEC *pnt);

#endif
