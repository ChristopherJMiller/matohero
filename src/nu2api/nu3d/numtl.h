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
    unsigned int unknown_00;
    unsigned char rs[0x38];
    unsigned int unknown_3c;
    NUMTLATTRIB attrib;
    unsigned char unknown_48[0x54 - 0x48];
    NUCOLOUR3 diffuse;
    unsigned char unknown_60[0x74 - 0x60];
    float alpha;
    short tid; /* int on PS2 */
    short sort_pri;
};

/* Not decompiled yet (nu3d_stubs.c). */
void NuMtlDestroy(NUMTL *mtl);

#endif
