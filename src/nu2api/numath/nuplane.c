#include "nuplane.h"

#include "nuvec.h"
#include "nutypes.h"

/* Planes and lines. Based on opensagadev/saga src/nu2api/numath/nuplane.cpp (GPL-3.0);
 * names and locals from the PS2 prototype. Only the functions below are in the PC exe
 * in this place; the rest of the PS2 file is still to be found. */

// FUNCTION: BIONICLE 0x571650
float NuPlnLine3(NUPLANE *p, NUVEC *p1, NUVEC *v1, NUVEC *iv) {
    float dist;

    NuVecNorm(v1, v1);
    dist = ((p->a * -p->d - p1->x) * p->a + (p->b * -p->d - p1->y) * p->b + (p->c * -p->d - p1->z) * p->c) / (v1->x * p->a + v1->y * p->b + v1->z * p->c);
    iv->x = dist * v1->x + p1->x;
    iv->y = dist * v1->y + p1->y;
    iv->z = dist * v1->z + p1->z;
    return dist;
}

// FUNCTION: BIONICLE 0x5716d0
float NuInfiniteLineToPointDistSqrEx(NUVEC *s, NUVEC *e, NUVEC *pnt, NUVEC *lp) {
    NUVEC v1;
    NUVEC v2;
    NUVEC nearp;
    NUVEC dp;
    float d1;

    NuVecSub(&v1, e, s);
    d1 = NuVecMag(&v1);
    NuVecScale(&v1, &v1, 1.0f / d1);
    NuVecSub(&v2, pnt, s);
    d1 = NuVecDot(&v1, &v2);
    NuVecScale(&nearp, &v1, d1);
    NuVecAdd(&nearp, &nearp, s);
    d1 = NuVecDistSqr(&nearp, pnt, &dp);
    if (lp != NULL) {
        *lp = nearp;
    }
    return d1;
}

// FUNCTION: BIONICLE 0x571790
float NuInfiniteLineToPointDistSqr(NUVEC *s, NUVEC *e, NUVEC *pnt) {
    return NuInfiniteLineToPointDistSqrEx(s, e, pnt, NULL);
}
