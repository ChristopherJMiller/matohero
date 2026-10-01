#ifndef NUVPORT_H
#define NUVPORT_H

#include "numath.h"

/* Viewport (layout from the PS2 prototype). */
typedef struct nuviewport_s {
    int x;
    int y;
    int width;
    int height;
    float zmin;
    float zmax;
    float centre_x;
    float centre_y;
    float clipmin_x;
    float clipmin_y;
    float clipmax_x;
    float clipmax_y;
    float clip_w;
    float clip_h;
    float scis_w;
    float scis_h;
} NUVIEWPORT;

extern NUVIEWPORT vpCurrent;

NUVIEWPORT *NuVpGetCurrentViewport(void);

#endif
