#include "nuvport.h"

/* Viewport state (PS2: nu3d/nuvport_gen.c). Only the accessor the camera code needs so
 * far; the rest of the file (NuVpSetSize, NuVpGetClippingMtx, ...) sits at
 * 0x53a140-0x53a520. */

// GLOBAL: BIONICLE 0x3607280
NUVIEWPORT vpCurrent;

// FUNCTION: BIONICLE 0x53a510
NUVIEWPORT *NuVpGetCurrentViewport(void) {
    return &vpCurrent;
}
