#ifndef NUAPI_H
#define NUAPI_H

#include "nutime.h"

typedef struct nuapi_s {
    int video_mode;
    int video_swap_mode;
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

void NuRegisterEndFrameCallBackFn(void (*fn)());

#endif
