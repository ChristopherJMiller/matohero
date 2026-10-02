#include "numtl.h"

#include "listman.h"
#include "nutex.h"

#include <string.h>

/* PC materials (PS2: nu3d/ps2/numtl.c): a static store of materials, one list per render
 * plane sorted by sort priority and texture, the PC's variants of a material (copies made
 * for each set of shader flags it is drawn with, for a forced material over scene
 * geometry of another vertex format), and destruction, which takes the variants along.
 * In PC order; the animation and default-material code isn't decompiled yet. */

#define NUMTL_STORE 1800
#define NUMTL_RENDERPLANES 24

// GLOBAL: BIONICLE 0x33a2be0
static NUMTL mtl_store[NUMTL_STORE];

// GLOBAL: BIONICLE 0x337d330
static NUMTL *smlist[NUMTL_RENDERPLANES];

// GLOBAL: BIONICLE 0x348c884
int numtl_renderplane;

/* Materials in use (two counts on PC; names ours). */
// GLOBAL: BIONICLE 0x348c88c
int numtl_count;

// GLOBAL: BIONICLE 0x348c948
int numtl_nmtls;

/* The highest store index used so far. */
// GLOBAL: BIONICLE 0x348c958
static int mtl_allocmax;

// FUNCTION: BIONICLE 0x540d70
void NuMtlCountRelease(void) {
    numtl_nmtls--;
    numtl_count--;
}

/* Destroys every material in the render plane lists. */
// FUNCTION: BIONICLE 0x540d80
void NuMtlRelease(void) {
    NUMTL *next;
    NUMTL *sm;
    int n;

    for (n = 0; n < NUMTL_RENDERPLANES; n++) {
        sm = smlist[n];
        smlist[n] = NULL;
        while (sm != NULL) {
            next = sm->snext;
            NuMtlDestroy(sm);
            sm = next;
        }
    }
}

/* Puts a material into a render plane's list, before the first one with a higher sort
 * priority and texture. */
// FUNCTION: BIONICLE 0x540dc0
void NuMtlInsert(NUMTL *sm, int renderplane) {
    NUMTL *list;
    NUMTL *last;
    int sid;
    int lsid;

    list = smlist[renderplane];
    last = NULL;
    sm->renderplane = renderplane;
    sm->snext = NULL;
    sm->slast = NULL;
    sid = (sm->sort_pri << 16) + (sm->tid & 0x7fff);
    if (list != NULL) {
        do {
            lsid = (list->sort_pri << 16) + (list->tid & 0x7fff);
            if (lsid >= sid) {
                break;
            }
            last = list;
            list = list->snext;
        } while (list != NULL);
        if (last != NULL) {
            sm->slast = last;
            sm->snext = last->snext;
            last->snext = sm;
            goto linked;
        }
    }
    sm->snext = smlist[renderplane];
    smlist[renderplane] = sm;
linked:
    if (sm->snext != NULL) {
        sm->snext->slast = sm;
    }
}

/* Takes a material out of its render plane's list. */
// FUNCTION: BIONICLE 0x540e60
void NuMtlRemove(NUMTL *mtl) {
    int i;

    if (mtl->slast != NULL) {
        mtl->slast->snext = mtl->snext;
        if (mtl->snext != NULL) {
            mtl->snext->slast = mtl->slast;
        }
    } else {
        for (i = 0; i < NUMTL_RENDERPLANES; i++) {
            if (smlist[i] == mtl) {
                smlist[i] = smlist[i]->snext;
                if (smlist[i] != NULL) {
                    smlist[i]->slast = NULL;
                }
            }
        }
    }
}

/* Adds a material to render plane 0 (the PS2 uses numtl_renderplane). */
// FUNCTION: BIONICLE 0x540f60
void NuMtlAdd(NUMTL *mtl) {
    mtl->next = NULL;
    NuMtlInsert(mtl, 0);
}

// FUNCTION: BIONICLE 0x540f80
void NuMtlAddEx(NUMTL *mtl, int renderplane) {
    mtl->next = NULL;
    mtl->fb_mask = mtl->attrib.fill ? 0 : 0xff000000;
    NuMtlInsert(mtl, renderplane);
}

/* Takes cnt materials from the store, each linked to the one made before it through
 * next, and returns the last. */
// FUNCTION: BIONICLE 0x5417d0
NUMTL *NuMtlCreateEx(int cnt, int renderplane) {
    NUMTL *sm;
    NUMTL *prev;
    int i;
    int ix;

    sm = NULL;
    prev = NULL;
    for (i = 0; i < cnt; i++) {
        for (ix = 0; ix < NUMTL_STORE; ix++) {
            if (!mtl_store[ix].used) {
                if (ix > mtl_allocmax) {
                    mtl_allocmax = ix;
                }
                goto init;
            }
        }
        ix = NUMTL_STORE;
    init:
        sm = &mtl_store[ix];
        memset(sm, 0, sizeof(NUMTL));
        sm->used = 1;
        sm->alpha = 1.0f;
        sm->attrib.filter = 2;
        sm->attrib.atst = 5;
        sm->attrib.aref = 127;
        sm->attrib.afail = 3;
        NuMtlInsert(sm, renderplane);
        numtl_count++;
        numtl_nmtls++;
        sm->next = prev;
        prev = sm;
        NuMtlShaderDescInit(sm, 0);
    }
    return sm;
}

// FUNCTION: BIONICLE 0x5418b0
NUMTL *NuMtlCreate(int cnt) {
    return NuMtlCreateEx(cnt, numtl_renderplane);
}

/* Re-sorts a material into its render plane's list and rebuilds its shader. */
// FUNCTION: BIONICLE 0x5418d0
void NuMtlUpdate(NUMTL *mtl) {
    NuMtlRemove(mtl);
    NuMtlInsert(mtl, mtl->renderplane);
    NuMtlShaderUpdate(mtl);
}

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
NUMTL *NuMtlFindVariant(NUMTL *forced, unsigned int shader) {
    NUMTLVARIANT *v;

    for (v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, NULL); v != NULL; v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, (NULNKHDR *)v)) {
        if (v->forced == forced && v->mtl->shader_flags == shader) {
            return v->mtl;
        }
    }
    return NULL;
}

/* Makes the variants of forced for n vertex shaders that it doesn't have yet. The last
 * argument isn't used. */
// FUNCTION: BIONICLE 0x542510
void NuMtlCreateVariants(NUMTL *forced, unsigned int *shaders, int n, void **lnk) {
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
            v->mtl->shader = 0;
            v->mtl->slast = slast;
            v->mtl->snext = snext;
            v->mtl->used = 1;
            v->mtl->shader_flags = *shaders;
            v->mtl->unknown_210 = NuPCShaderLookup(v->mtl->shader_flags);
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
        if (v->forced == forced && v->mtl->shader_flags == mtl->shader_flags) {
            return v->mtl;
        }
    }
    NuMtlCreateVariants(forced, &mtl->shader_flags, 1, (void **)&v);
    return NuMtlFindVariant(forced, mtl->shader_flags);
}

/* The variant of forced for a vertex shader, or forced itself. */
// FUNCTION: BIONICLE 0x542660
NUMTL *NuMtlGetVariant(NUMTL *forced, unsigned int shader) {
    NUMTLVARIANT *v;

    for (v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, NULL); v != NULL; v = (NUMTLVARIANT *)NuLstGetNext(mtl_variants, (NULNKHDR *)v)) {
        if (v->forced == forced && v->mtl->shader_flags == shader) {
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
