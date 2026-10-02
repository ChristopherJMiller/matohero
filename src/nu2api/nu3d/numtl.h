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
    unsigned int fb_mask; /* PS2 name: 0xff000000, or 0 for attrib.fill */
    int unknown_a4[(0xb4 - 0xa4) / 4];
    int unknown_b4; /* set when the material has a shader description */
    /* 0xb8: the material's NUSHADERMTLDESC, kept as ints here because the struct's double
     * would pad NUMTL past its 0x214 bytes */
    int desc[(0x1b8 - 0xb8) / 4];
    unsigned int shader_flags; /* the description's unknown_100; a forced material stands
                                * in directly only if these agree */
    int shader; /* the description's unknown_104: the shader built from it */
    int desc_1c0[(0x210 - 0x1c0) / 4];
    void *unknown_210; /* looked up from shader_flags (0x214 bytes in all: NuQFntCreate
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

/* pc/numtl.c */
void NuMtlRelease(void);
void NuMtlInsert(NUMTL *sm, int renderplane);
void NuMtlRemove(NUMTL *mtl);
void NuMtlCountRelease(void);
void NuMtlAdd(NUMTL *mtl);
void NuMtlAnimateSetMask(int mask);
void NuMtlAnimateSetSpeedScale(float scale);
void NuMtlDetachMtl(NUMTL *host);
int NuMtlSetCurrentRenderPlane(int renderplane);
void NuMtlSetAlphaTest(NUMTL *mtl);
void NuMtlSetAlphaFail(NUMTL *mtl);
NUMTL *NuMtlCreateEx(int cnt, int renderplane);
NUMTL *NuMtlCreate(int cnt);

/* Not decompiled yet (nu3d_stubs.c). */
void NuMtlSetShaderDescPS(NUMTL *mtl, NUSHADERMTLDESC *desc);
void NuMtlUpdate(NUMTL *mtl);
void NuMtlAddEx(NUMTL *mtl, int renderplane);
void NuMtlDestroy(NUMTL *mtl);

/* pc/numtl.c: PC variants of a material, one per vertex shader it's drawn with (names
 * ours). */
void NuMtlVariantsInit(int count, VARIPTR *buff, VARIPTR *buffend);
NUMTL *NuMtlFindVariant(NUMTL *forced, unsigned int shader);
void NuMtlCreateVariants(NUMTL *forced, unsigned int *shaders, int n, void **lnk);
NUMTL *NuMtlGetForcedVariant(NUMTL *mtl, NUMTL *forced);
NUMTL *NuMtlGetVariant(NUMTL *forced, unsigned int shader);

/* Not decompiled yet (nu3d_stubs.c; names ours unless PS2). */
void NuMtlShaderDescInit(NUMTL *mtl, int flags);
void NuMtlShaderUpdate(NUMTL *mtl);
int NuTexUnRef(int tid);
void NuPCGeomRelease(void *geom);
void *NuPCShaderLookup(unsigned int flags);

#endif
