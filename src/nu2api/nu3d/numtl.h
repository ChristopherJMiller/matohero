#ifndef NUMTL_H
#define NUMTL_H

#include "nugscn.h"

/* PS2: nu3d/ps2/numtl.c. Materials: render state plus a texture. */

typedef struct rndrstream_s RNDRSTREAM;

typedef struct nucolour3_s {
    float r;
    float g;
    float b;
} NUCOLOUR3;

/* PS2 layout; the PC's masks on NUMTL+0x40 use the same bits. */
typedef struct numtlattrib_s {
    unsigned int alpha : 4;
    unsigned int filter : 2;
    unsigned int fx : 2;
    unsigned int utc : 2;
    unsigned int vtc : 2;
    unsigned int cull : 2;
    unsigned int zmode : 2;
    unsigned int lighting : 2;
    unsigned int colour : 1;
    unsigned int fill : 1;
    unsigned int atst : 3;
    unsigned int aref : 8;
    unsigned int afail : 2;
    unsigned int uvmode : 1;
    unsigned int ot : 1;
    unsigned int shade : 1;
    unsigned int date : 1;
    unsigned int datm : 1;
    unsigned int trackskin : 1;
    unsigned int only2d : 1;
    unsigned int fastblend : 1;
    unsigned int sortpri : 1;
    unsigned int stencil_shadows : 2;
    unsigned int sticky : 1;
    unsigned int mtl_layers : 8;
    unsigned int pad : 10;
} NUMTLATTRIB;

/* PC layout, only the fields placed so far (from the font code). The PS2 has a
 * RNDRSTREAM rs at 0x124; on PC the font code passes the address 4 bytes in where it
 * passes rs, and nothing reads through it, so it is kept as bytes here. */
struct numtl_s {
    short used : 1; /* the PS2 also has sbexists before renderplane */
    short fromblock : 1;
    short intalloc : 1;
    short zupd : 1;
    short renderplane : 8;
    short pad : 4;
    short unknown_02;
    unsigned char rs[0x38];
    NUDISPLAYLIST *dlist_mtl; /* the material's list in a dynamic scene */
    NUMTLATTRIB attrib;
    unsigned char unknown_48[0x54 - 0x48];
    NUCOLOUR3 diffuse;
    unsigned char unknown_60[0x74 - 0x60];
    float alpha;
    short tid; /* int on PS2 */
    short sort_pri;
    int unknown_7c[(0x214 - 0x7c) / 4]; /* 0x214 bytes: NuQFntCreate copies whole ones */
};

/* The PC's shader description (0x158 bytes; differs from saga's). Only the fields the
 * font code sets are placed, by offset. */
typedef struct nushadermtldesc_s {
    unsigned int flags;
    int unknown_004;
    int unknown_008[3];
    unsigned int unknown_014;
    int unknown_018[3];
    float unknown_024;
    int unknown_028[3];
    float unknown_034;
    int unknown_038[(0x100 - 0x38) / 4]; /* not chars: /GS would guard the frame */
    unsigned int unknown_100; /* bits */
    int unknown_104[(0x150 - 0x104) / 4];
    double unknown_150; /* something 8-byte aligned: users align their frames */
} NUSHADERMTLDESC;

extern int numtl_renderplane;

/* Not decompiled yet (nu3d_stubs.c). */
NUMTL *NuMtlCreate(int cnt);
void NuMtlSetShaderDescPS(NUMTL *mtl, NUSHADERMTLDESC *desc);
void NuMtlUpdate(NUMTL *mtl);
void NuMtlSetRenderPlane(NUMTL *mtl, int newplane);
void NuMtlDestroy(NUMTL *mtl);

#endif
