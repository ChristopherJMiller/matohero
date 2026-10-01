#ifndef NUQFNT_H
#define NUQFNT_H

#include "nugscn.h"
#include "nustring.h"

/* Quick fonts: textured bitmap fonts with Unicode lookup tables. NUQFNT is opaque to
 * callers (void in the PS2 debug info); the functions cast it to VUFNT. */
typedef void NUQFNT;
typedef struct nufnt_s NUFNT;

typedef enum {
    NUQFNT_CSMODE_UNINITIALISED = 0,
    NUQFNT_CSMODE_PS2 = 1,
    NUQFNT_CSMODE_PIXEL = 2,
    NUQFNT_CSMODE_NORMALISED = 3,
    NUQFNT_CSMODE_ABSOLUTE = 4,
    NUQFNT_CSMODE_CNT = 5
} NUQFNT_CSMODE;

typedef int NUQFNT_PRINTMODE;

typedef struct vuchar_s {
    float u;
    float v;
    float w;
} VUCHAR;

typedef struct vucharidx_s {
    NUWCHAR unicode;
    NUWCHAR index;
} VUCHARIDX;

typedef struct vufnt_s {
    int version;
    short type;
    short flags;
    int size;
    int numchars;
    int unicodetablesize;
    float height;
    float baseline;
    float spacewidth;
    int sendid;
    float icgap;
    NUFNT *fnt;
    float *scale_x;
    float *scale_y;
    VUCHAR *chars;
    union {
        unsigned char *a;
        VUCHARIDX *w;
    } index_table;
    VARIPTR header;
    NUMTL *mtl;
    unsigned int *abgr;
    void *psdata;
} VUFNT;

/* PC's per-font state behind psdata (name ours; the layout of saga's VUFNT_ANDROID).
 * abgr, scale_x and scale_y point into it. */
typedef struct vufnt_pc_s {
    unsigned int abgr;
    float x;
    float y;
    float z;
    NUMTX mtx;
    float scale_x;
    float scale_y;
} VUFNT_PC;

extern float qfnt_rezscale_w;
extern float qfnt_rezscale_h;
extern float qfnt_offscale_x;
extern float qfnt_offscale_y;
extern float qfnt_len_scale;
extern float qfnt_height_scale;
extern float nuqfnt_space_width;
extern NUQFNT *system_qfont;
extern unsigned char sysfont[18736];
extern NUQFNT_CSMODE NuQFntCSMode;
extern NUQFNT_PRINTMODE NuQFntMode;
extern int NuQFntCSModeStackIndex;
extern int NuQFntModeStackIndex;
extern NUQFNT_CSMODE NuQFntCSModeStack[16];
extern NUQFNT_PRINTMODE NuQFntModeStack[16];

void NuQFntPushPrintMode(NUQFNT_PRINTMODE newmode);
void NuQFntPopPrintMode(void);
void NuQFntSetICGap(NUQFNT *fv, float icgap);
void NuQFntSetSpaceWidth(NUQFNT *fv, float width);
NUQFNT_CSMODE NuQFntSetCoordinateSystem(NUQFNT_CSMODE mode);
void NuQFntPushCoordinateSystem(NUQFNT_CSMODE newmode);
void NuQFntPopCoordinateSystem(void);
NUWCHAR UnicodeToIndexFast(VUCHARIDX *idxtab, int tablesize, NUWCHAR unicode);
NUWCHAR NuQFntEncodeUnicodeChar(NUQFNT *fv, NUWCHAR c);
void NuQFntEncodeUnicodeString(NUQFNT *fv, NUWCHAR *txt);
void NuQFntUTF8toQCode(NUQFNT *fv, char *txtin, NUWCHAR *txtout);
float NuQFntPrintLenW(NUQFNT *fv, NUWCHAR *txt);
float NuQFntPrintLenU(NUQFNT *fv, char *txt);
float NuQFntHeight(NUQFNT *fv);
float NuQFntBaseline(NUQFNT *fv);
void NuQFntSet(NUQFNT *fv);
void NuQFntSetScale(NUQFNT *fv, float xs, float ys);
void NuQFntSetColour(NUQFNT *fv, unsigned int abgr);
void NuQFntDestroy(NUQFNT *fv);
int NuQFntReadPS(VUFNT *fv, int tid, int flags, int renderplane, VARIPTR *ptr, VARIPTR buffend);
NUQFNT *NuQFntRead(char *fname, VARIPTR *ptr, VARIPTR buffend);
NUQFNT *NuQFntLoadPtr(char *name, char *texname, int unknown3, int unknown4, VARIPTR *ptr, VARIPTR *end);
void NuQFntInit(VARIPTR *buff, VARIPTR buffend);
void NuQFntMove(NUQFNT *fv, float x, float y, float z);
void NuQFntPrintW(NUQFNT *fv, NUWCHAR *txt);
void NuQFntPrintU(NUQFNT *fv, char *txt);
void NuQFntPrintEx(NUQFNT *fnt, int x, int y, int origin, char *fmt, ...);

/* Not decompiled yet (nu3d_stubs.c). */
void NuQFntPrintCharW(NUQFNT *fv, NUWCHAR *txt, NUQFNT_PRINTMODE mode);

#endif
