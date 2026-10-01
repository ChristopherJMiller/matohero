#ifndef NUVPORT_H
#define NUVPORT_H

#include "numath.h"
#include "nutypes.h"

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

/* The same viewport in floats (PS2 layout). */
typedef struct nuviewport2_s {
    float x;
    float y;
    float width;
    float height;
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
} NUVIEWPORT2;

/* Direct3D's viewport (D3DVIEWPORT9's layout; the name is ours until the SDK headers are
 * in). NUVIEWPORT starts with the same six fields, and NuVpInit fills vpDevice through
 * d3dGetViewport. */
typedef struct nupcviewport_s {
    unsigned int X;
    unsigned int Y;
    unsigned int Width;
    unsigned int Height;
    float MinZ;
    float MaxZ;
} NUPCVIEWPORT;

extern int vport_inval;
extern NUVIEWPORT vpDevice;
extern NUMTX vp_cmtx;
extern NUVIEWPORT vpCurrent;
extern NUMTX vp_smtx;
/* Offset and scale applied to the projection, for tiled hi-res screen dumps. */
extern float vport_hrdump_xoffset;
extern float vport_hrdump_yoffset;
extern float vport_hrdump_xscale;
extern float vport_hrdump_yscale;

void NuVpSetSize(float w, float h);
void NuVpSetPosition2(float x, float y);
void NuVpGetPosition2(float *x, float *y);
void NuVpSetCurrent2(NUVIEWPORT2 *vp);
void NuVpGetCurrent2(NUVIEWPORT2 *vp);
void NuVpRestore(void);
void NuVpSetScalingMtx(void);
void NuVpInit(void);
void NuVpGetClippingMtx(NUMTX *mtx);
void NuVpGetScalingMtx(NUMTX *mtx);
NUVIEWPORT *NuVpGetCurrentViewport(void);
void NuVpUpdate(void);

/* The PC layer's Direct3D wrappers (named after their error-check strings). */
void d3dGetViewport(NUVIEWPORT *vp);
void d3dSetViewport(NUPCVIEWPORT *vp);

#endif
