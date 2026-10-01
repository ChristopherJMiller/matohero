#include "nuvideo.h"

#include "nuapi.h"

/* Video mode settings. Based on opensagadev/saga src/nu2api/nucore/nuvideo.cpp
 * (GPL-3.0); names and locals from the PS2 prototype. On PC the platform half,
 * NuVideoSetSwapModePS, is empty (folded.h). */

// FUNCTION: BIONICLE 0x56c980
NUVIDEO_SWAPMODE NuVideoSetSwapMode(NUVIDEO_SWAPMODE video_swap_mode) {
    NUVIDEO_SWAPMODE previous;

    previous = nuapi.video_swap_mode;
    nuapi.video_swap_mode = video_swap_mode;
    switch (nuapi.video_swap_mode) {
    case NUVIDEO_SWAPMODE_ASYNC:
        if (nuapi.pal) {
            nuapi.native_FPS = 50.0f;
        } else {
            nuapi.native_FPS = 60.0f;
        }
        break;
    case NUVIDEO_SWAPMODE_FIELDSYNC:
        if (nuapi.pal) {
            nuapi.native_FPS = 50.0f;
        } else {
            nuapi.native_FPS = 60.0f;
        }
        break;
    case NUVIDEO_SWAPMODE_FRAMESYNC:
        if (nuapi.pal) {
            nuapi.native_FPS = 25.0f;
        } else {
            nuapi.native_FPS = 30.0f;
        }
        break;
    case NUVIDEO_SWAPMODE_MIN2FIELD:
        if (nuapi.pal) {
            nuapi.native_FPS = 25.0f;
        } else {
            nuapi.native_FPS = 30.0f;
        }
        break;
    case NUVIDEO_SWAPMODE_ROLLING:
        if (nuapi.pal) {
            nuapi.native_FPS = 50.0f;
        } else {
            nuapi.native_FPS = 60.0f;
        }
        break;
    case NUVIDEO_SWAPMODE_ROLLING2FRAMES:
        if (nuapi.pal) {
            nuapi.native_FPS = 25.0f;
        } else {
            nuapi.native_FPS = 30.0f;
        }
        break;
    default:
        break;
    }
    NuVideoSetSwapModePS(video_swap_mode);
    return previous;
}
