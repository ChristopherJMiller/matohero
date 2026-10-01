#ifndef NUAPI_H
#define NUAPI_H

#include "nutime.h"
#include "nuvideo.h"

typedef struct nuapi_s {
    int video_mode;
    NUVIDEO_SWAPMODE video_swap_mode;
    int pal;
    float native_FPS;
    int nuframe_begin_cnt;
    int viewport_calls;
    NUTIME_T frame_start;
    NUTIME_T after_vsync;
    float last_frame_time;
    int frame_count;
} NUAPI;

extern NUAPI nuapi;

void NuAPIInit(void);
void NuFrameBegin(void);
float NuFrameEnd(void);
void NuRegisterEndFrameCallBackFn(void (*fn)());

#endif
