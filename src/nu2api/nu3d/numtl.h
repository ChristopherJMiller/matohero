#ifndef NUMTL_H
#define NUMTL_H

#include "nugscn.h"

/* PS2: nu3d/ps2/numtl.c. Materials: render state plus a texture. */

typedef struct rndrstream_s RNDRSTREAM;

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

/* Something a PC material holds geometry through (layout ours; only next and the handle
 * that is released are known). */
typedef struct numtlgeomref_s {
    struct numtlgeomref_s *next;
    int unknown_04[7];
    void *geom;
} NUMTLGEOMREF;

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
    int rs; /* the font code passes this address as a RNDRSTREAM * (PS2: rs at 0x124) */
    NUMTLGEOMREF *unknown_08; /* PC geometry the material owns, released with it */
    NUMTLGEOMREF *unknown_0c;
    NUMTLGEOMREF *unknown_10; /* a list of them */
    int unknown_14[(0x28 - 0x14) / 4];
    struct numtl_s *snext; /* the list of materials (as on PS2) */
    struct numtl_s *slast;
    struct numtl_s *next; /* the next pass of a multi-pass material */
    int unknown_34[2];
    NUDISPLAYLIST *dlist_mtl; /* the material's list in a dynamic scene */
    NUMTLATTRIB attrib;
    unsigned char unknown_48[0x54 - 0x48];
    NUCOLOUR3 diffuse;
    float power; /* specular power (PS2: after the fx fields) */
    unsigned char unknown_64[0x74 - 0x64];
    float alpha;
    short tid; /* int on PS2 */
    short sort_pri;
    int unknown_7c[(0x9c - 0x7c) / 4];
    char context;
    unsigned char fxid; /* the shader effect: 0x81 and 0x85 are specular */
    unsigned char special_id; /* index into nurndr_forced_mtl_table */
    unsigned char unknown_9f;
    int unknown_a0[(0x1b8 - 0xa0) / 4];
    int unknown_1b8; /* the vertex shader: a forced material stands in directly only if
                      * this agrees */
    int unknown_1bc;
    int unknown_1c0[(0x210 - 0x1c0) / 4];
    void *unknown_210; /* looked up from unknown_1b8 (0x214 bytes in all: NuQFntCreate
                        * copies whole ones) */
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

/* pc/numtl.c: PC variants of a material, one per vertex shader it's drawn with (names
 * ours). */
void NuMtlVariantsInit(int count, VARIPTR *buff, VARIPTR *buffend);
NUMTL *NuMtlFindVariant(NUMTL *forced, int shader);
void NuMtlCreateVariants(NUMTL *forced, int *shaders, int n, void **lnk);
NUMTL *NuMtlGetForcedVariant(NUMTL *mtl, NUMTL *forced);
NUMTL *NuMtlGetVariant(NUMTL *forced, int shader);

/* Not decompiled yet (nu3d_stubs.c; names ours unless PS2). */
void NuMtlRemove(NUMTL *mtl);
void NuMtlCountRelease(void);
int NuTexUnRef(int tid);
void NuPCGeomRelease(void *geom);
void *NuPCShaderLookup(int shader);

#endif
