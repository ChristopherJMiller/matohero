#include "nuapi.h"

#include <string.h>
#include <windows.h>

#include "nupad.h"

/* Engine frame loop and setup. Names from the PS2 prototype. On PC NuFrameEnd also runs
 * the Windows side: while the window is inactive it pauses sound and sleeps, pumping
 * messages. */

/* Not decompiled yet: nu3d (nu3d/nu3d_stubs.c) and the PC layer (pc/pcapi_stubs.c,
 * names ours). */
void NuPortalInit(void);
void NuWaterReset(void);
void NuMtlAnimate(float timestep);
void NuTexAnimProcess(float frametime);
void NuTimeBarSetRender(int tbh);
int NuRndrSwapScreenEx(int hRT, void (*fn)());
int NuFrameEndBgLoadPS(int min_delay);
extern int pc_app_active;
void PCFrameLock(void);
void PCFrameUnlock(void);
void PCSoundPause(void);
void PCSoundResume(void);
int PCProcessMessages(void);
void PCCheckDevice(void);

// GLOBAL: BIONICLE 0x3607b60
NUAPI nuapi;

// GLOBAL: BIONICLE 0x34c5fbc
void (*nuapi_endframe_callbackfn)();

// GLOBAL: BIONICLE 0x6174c8
static int min_delay = 20;

// GLOBAL: BIONICLE 0x3607b90
float elapsedShaderTimeAPI;

// FUNCTION: BIONICLE 0x56c670
void NuAPIInit(void) {
    memset(&nuapi, 0, sizeof(NUAPI));
    nuapi.native_FPS = 60.0f;
    nuapi.video_mode = 2;
    nuapi.video_swap_mode = NUVIDEO_SWAPMODE_ROLLING;
}

// FUNCTION: BIONICLE 0x56c6a0
void NuFrameBegin(void) {
    nuapi.nuframe_begin_cnt++;
    NuPortalInit();
    NuWaterReset();
    PCFrameLock();
}

// FUNCTION: BIONICLE 0x56c6c0
float NuFrameEnd(void) {
    NUTIME current_time;
    NUTIME time_diff;

    nuapi.nuframe_begin_cnt--;
    nuapi.frame_count++;
    NuMtlAnimate(nuapi.last_frame_time);
    NuTexAnimProcess(nuapi.last_frame_time);
    NuTimeGet(&current_time);
    NuTimeSub(&time_diff, &current_time, &nuapi.frame_start);
    nuapi.last_frame_time = NuTimeSeconds(&time_diff);
    nuapi.frame_start = current_time;

    if (!pc_app_active) {
        PCSoundPause();
        while (!pc_app_active) {
            Sleep(1);
            if (!PCProcessMessages()) {
                break;
            }
        }
        PCCheckDevice();
        PCSoundResume();
    } else {
        PCProcessMessages();
        PCCheckDevice();
    }

    NuTimeGet(&current_time);
    nuapi.frame_start = current_time;
    if (nuapi.last_frame_time < 0.0f) {
        nuapi.last_frame_time = 0.0f;
    }
    NuTimeBarSetRender(-1);
    NuRndrSwapScreenEx(1, nuapi_endframe_callbackfn);
    elapsedShaderTimeAPI += nuapi.last_frame_time;
    PCFrameUnlock();
    NuFrameEndBgLoadPS(min_delay);
    NuPadUpdatePads();
    NuTimeGet(&nuapi.after_vsync);
    return nuapi.last_frame_time;
}

// FUNCTION: BIONICLE 0x56c810
void NuRegisterEndFrameCallBackFn(void (*fn)()) {
    nuapi_endframe_callbackfn = fn;
}
