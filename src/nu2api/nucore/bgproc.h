#ifndef BGPROC_H
#define BGPROC_H

#include "nutypes.h"

/* Background requests: DoFn runs on the loader thread (or at frame end), AckFn after it
 * or on abort. */
typedef void bgprocdofn();
typedef void bgprocackfn();

typedef struct bgvar_s {
    int type;
    union {
        int s32;
        unsigned int u32;
        float f32;
        char *cp;
        void *pvoid;
        VARIPTR *pvariptr;
        VARIPTR variptr;
    } v;
} BGVAR;

typedef struct bgprocinfo_s {
    bgprocdofn *DoFn;
    bgprocackfn *AckFn;
    int posted : 1;
    int abort : 1;
    int pad;
    char wkspace[256];
    BGVAR *vars;
    int var_cnt;
} BGPROCINFO;

BGPROCINFO *bgPostRequest(bgprocdofn *dofn, bgprocackfn *ackfn, void *data, int datasize);
void bgProcInit(void);
void bgProcAbortAll(void);
void NuFrameEndBgLoadPS(int min_delay);

#endif
