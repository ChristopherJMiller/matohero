#include "nuapi.h"

#include <string.h>

/* Engine frame loop and setup. Names from the PS2 prototype. NuFrameBegin/NuFrameEnd
 * call into nu3d and the PC layer and are still to do. */

// GLOBAL: BIONICLE 0x3607b60
NUAPI nuapi;

// GLOBAL: BIONICLE 0x34c5fbc
void (*nuapi_endframe_callbackfn)();

// FUNCTION: BIONICLE 0x56c670
void NuAPIInit(void) {
    memset(&nuapi, 0, sizeof(NUAPI));
    nuapi.native_FPS = 60.0f;
    nuapi.video_mode = 2;
    nuapi.video_swap_mode = NUVIDEO_SWAPMODE_ROLLING;
}

// FUNCTION: BIONICLE 0x56c810
void NuRegisterEndFrameCallBackFn(void (*fn)()) {
    nuapi_endframe_callbackfn = fn;
}
