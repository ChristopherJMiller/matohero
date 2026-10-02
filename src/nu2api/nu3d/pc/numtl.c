#include "numtl.h"

#include "listman.h"
#include "nutex.h"

/* PC materials (PS2: nu3d/ps2/numtl.c). Only the end of the file so far: the PC's
 * variants of a material, copies of it made for each vertex shader it is drawn with
 * (a forced material over scene geometry of another vertex format), and destruction,
 * which takes the variants along. In PC order. */

/* A variant (name and layout ours). */
typedef struct numtlvariant_s {
    NUMTL *mtl;
    NUMTL *forced;
    int shader;
} NUMTLVARIANT;

// GLOBAL: BIONICLE 0x348c978
static NULSTHDR *mtl_variants;

// FUNCTION: BIONICLE 0x542490
void NuMtlVariantsInit(int count, VARIPTR *buff, VARIPTR *buffend) {
    mtl_variants = NuLstCreateBuff(count, sizeof(NUMTLVARIANT), buff, *buffend, 16);
}

/* The variant of forced for a vertex shader, or NULL. */
// FUNCTION: BIONICLE 0x5424c0
NUMTL *NuMtlFindVariant(NUMTL *forced, int shader) {
    NUMTLVARIANT *v;

    for (v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, NULL); v != NULL; v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, (NULNKHDR *)v)) {
        if (v->forced == forced && v->mtl->unknown_1b8 == shader) {
            return v->mtl;
        }
    }
    return NULL;
}

/* Makes the variants of forced for n vertex shaders that it doesn't have yet. The last
 * argument isn't used. */
// FUNCTION: BIONICLE 0x542510
void NuMtlCreateVariants(NUMTL *forced, int *shaders, int n, void **lnk) {
    NUMTLVARIANT *v;
    NUMTL *snext;
    NUMTL *slast;

    for (; n > 0; n--, shaders++) {
        if (NuMtlFindVariant(forced, *shaders) == NULL) {
            v = (NUMTLVARIANT *)NuLstAlloc(mtl_variants);
            v->mtl = NuMtlCreate(1);
            v->forced = forced;
            v->shader = *shaders;
            slast = v->mtl->slast;
            snext = v->mtl->snext;
            *v->mtl = *forced;
            v->mtl->unknown_1bc = 0;
            v->mtl->slast = slast;
            v->mtl->snext = snext;
            v->mtl->used = 1;
            v->mtl->unknown_1b8 = *shaders;
            v->mtl->unknown_210 = NuPCShaderLookup(v->mtl->unknown_1b8);
            NuMtlUpdate(v->mtl);
        }
    }
}

/* The variant of forced for mtl's vertex shader, made if needed. 83%: the original stores
 * the loop variable to its stack slot after each NuLstGetNext, since its address is
 * passed to NuMtlCreateVariants afterwards; ours never stores it
 * (docs/matching.md, open questions). */
// FUNCTION: BIONICLE 0x5425e0
NUMTL *NuMtlGetForcedVariant(NUMTL *mtl, NUMTL *forced) {
    NUMTLVARIANT *v;

    for (v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, NULL); v != NULL; v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, (NULNKHDR *)v)) {
        if (v->forced == forced && v->mtl->unknown_1b8 == mtl->unknown_1b8) {
            return v->mtl;
        }
    }
    NuMtlCreateVariants(forced, &mtl->unknown_1b8, 1, (void **)&v);
    return NuMtlFindVariant(forced, mtl->unknown_1b8);
}

/* The variant of forced for a vertex shader, or forced itself. */
// FUNCTION: BIONICLE 0x542660
NUMTL *NuMtlGetVariant(NUMTL *forced, int shader) {
    NUMTLVARIANT *v;

    for (v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, NULL); v != NULL; v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, (NULNKHDR *)v)) {
        if (v->forced == forced && v->mtl->unknown_1b8 == shader) {
            return v->mtl;
        }
    }
    return forced;
}

/* Releases one material: its texture, its geometry and its place in the material list
 * (the PS2's NuMtlDestroy; name ours). */
// FUNCTION: BIONICLE 0x5426b0
static void MtlDestroy(NUMTL *mtl) {
    NUMTLGEOMREF *ref;

    if (mtl->tid != 0) {
        NuTexUnRef(mtl->tid);
    }
    if (mtl->unknown_08 != NULL) {
        NuPCGeomRelease(mtl->unknown_08->geom);
    }
    if (mtl->unknown_0c != NULL) {
        NuPCGeomRelease(mtl->unknown_0c->geom);
    }
    for (ref = mtl->unknown_10; ref != NULL; ref = ref->next) {
        NuPCGeomRelease(ref->geom);
    }
    NuMtlRemove(mtl);
    mtl->used = 0;
    NuMtlCountRelease();
}

/* Destroys a material and its variants. */
// FUNCTION: BIONICLE 0x542720
void NuMtlDestroy(NUMTL *mtl) {
    NUMTLVARIANT *v;
    NUMTLVARIANT *next;

    MtlDestroy(mtl);
    for (v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, NULL); v != NULL; v = next) {
        next = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, (NULNKHDR *)v);
        if (v->forced == mtl) {
            MtlDestroy(v->mtl);
            NuLstFree((NULNKHDR *)v);
        }
    }
}
