#include "numtl.h"

#include "listman.h"
#include "nurndr.h"
#include "nutex.h"

#include <string.h>

/* PC materials (PS2: nu3d/ps2/numtl.c): a static store of materials, one list per render
 * plane sorted by sort priority and texture, the PC's variants of a material (copies made
 * for each set of shader flags it is drawn with, for a forced material over scene
 * geometry of another vertex format), and destruction, which takes the variants along.
 * In PC order; the animation and default-material code isn't decompiled yet. */

#define NUMTL_STORE 1800
#define NUMTL_RENDERPLANES 24
#define NUMTL_SORTPRIS 257

/* An entry of the per-frame render lists: something to draw with a material, sorted by
 * render plane and sort priority (PC; layout and name ours). */
typedef struct numtlrenderitem_s {
    struct numtlrenderitem_s *next;
    void *data;
    float dist;
    NUMTL *mtl;
} NUMTLRENDERITEM;

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

// GLOBAL: BIONICLE 0x616c10
static int mtl_animation_mask = 0xffffff;

// GLOBAL: BIONICLE 0x616c14
static float mtl_animation_speed_scale = 1.0f;

/* The render lists: first and last entry for each render plane and sort priority (the
 * sort priority clamped to -1..255, offset by one). */
// GLOBAL: BIONICLE 0x35f5040
static NUMTLRENDERITEM *rlist_head[NUMTL_RENDERPLANES][NUMTL_SORTPRIS];

// GLOBAL: BIONICLE 0x35fb0a0
static NUMTLRENDERITEM *rlist_tail[NUMTL_RENDERPLANES][NUMTL_SORTPRIS];

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

/* Appends an entry to its material's render list (name ours). */
// FUNCTION: BIONICLE 0x540fc0
void NuMtlRenderListAdd(NUMTLRENDERITEM *item) {
    NUMTL *mtl;
    int pri;

    mtl = item->mtl;
    pri = mtl->sort_pri + 1;
    if (pri >= NUMTL_SORTPRIS) {
        pri = NUMTL_SORTPRIS - 1;
    } else if (pri < 0) {
        pri = 0;
    }
    if (rlist_head[mtl->renderplane][pri] != NULL) {
        rlist_tail[mtl->renderplane][pri]->next = item;
    } else {
        rlist_head[mtl->renderplane][pri] = item;
    }
    rlist_tail[mtl->renderplane][pri] = item;
    item->next = NULL;}

// FUNCTION: BIONICLE 0x5414d0
void NuMtlAnimateSetMask(int mask) {
    mtl_animation_mask = mask;
}

// FUNCTION: BIONICLE 0x5414e0
void NuMtlAnimateSetSpeedScale(float scale) {
    mtl_animation_speed_scale = scale;
}

/* Unhooks the material chained after host (the PC doesn't free it). */
// FUNCTION: BIONICLE 0x5414f0
void NuMtlDetachMtl(NUMTL *host) {
    host->next = host->next->next;
}

/* Sets the render plane new materials go to; returns the old one. */
// FUNCTION: BIONICLE 0x541500
int NuMtlSetCurrentRenderPlane(int renderplane) {
    int rv;

    rv = numtl_renderplane;
    numtl_renderplane = renderplane;
    return rv;
}

/* Sets Direct3D's alpha test from the material's PS2 test (attrib.atst: never, always,
 * less, less-equal, equal, greater-equal, greater, not-equal) and reference value
 * (name ours). */
// FUNCTION: BIONICLE 0x541510
void NuMtlSetAlphaTest(NUMTL *mtl) {
    if (mtl->attrib.atst != 1) {
        d3dSetRenderState(15, 1); /* D3DRS_ALPHATESTENABLE */
        if (mtl->attrib.atst == 1) {
            d3dSetRenderState(25, 8); /* D3DRS_ALPHAFUNC, D3DCMP_ALWAYS */
        } else if (mtl->attrib.atst == 0) {
            d3dSetRenderState(25, 1);
        } else if (mtl->attrib.atst == 3) {
            d3dSetRenderState(25, 4);
        } else if (mtl->attrib.atst == 4) {
            d3dSetRenderState(25, 3);
        } else if (mtl->attrib.atst == 5) {
            d3dSetRenderState(25, 7);
        } else if (mtl->attrib.atst == 6) {
            d3dSetRenderState(25, 5);
        } else if (mtl->attrib.atst == 7) {
            d3dSetRenderState(25, 6);
        } else if (mtl->attrib.atst == 2) {
            d3dSetRenderState(25, 2);
        }
        d3dSetRenderState(24, mtl->attrib.aref * 2); /* D3DRS_ALPHAREF */
    } else {
        d3dSetRenderState(15, 0);
    }
}

/* Sets up the pass for the pixels that fail the alpha test: the inverse comparison, and
 * which buffers they still write (attrib.afail: keep, frame buffer only, z only, colour
 * only; name ours). */
// FUNCTION: BIONICLE 0x5415b0
void NuMtlSetAlphaFail(NUMTL *mtl) {
    if (mtl->attrib.atst == 1) {
        return;
    }
    if (mtl->attrib.atst == 0) {
        d3dSetRenderState(25, 8);
    } else if (mtl->attrib.atst == 3) {
        d3dSetRenderState(25, 5);
    } else if (mtl->attrib.atst == 4) {
        d3dSetRenderState(25, 6);
    } else if (mtl->attrib.atst == 5) {
        d3dSetRenderState(25, 2);
    } else if (mtl->attrib.atst == 6) {
        d3dSetRenderState(25, 4);
    } else if (mtl->attrib.atst == 7) {
        d3dSetRenderState(25, 3);
    } else if (mtl->attrib.atst == 2) {
        d3dSetRenderState(25, 7);
    }
    if (mtl->attrib.afail == 0) {
        d3dSetRenderState(168, 0); /* D3DRS_COLORWRITEENABLE */
        d3dSetRenderState(14, 0); /* D3DRS_ZWRITEENABLE */
    } else if (mtl->attrib.afail == 1) {
        d3dSetRenderState(14, 0);
    } else if (mtl->attrib.afail == 2) {
        d3dSetRenderState(168, 0);
        d3dSetRenderState(14, 1);
    } else if (mtl->attrib.afail == 3) {
        d3dSetRenderState(168, 7);
        d3dSetRenderState(14, 0);
    }
}

/* Replaces the material's shader description, if it has one. */
// FUNCTION: BIONICLE 0x5417a0
void NuMtlSetShaderDescPS(NUMTL *mtl, NUSHADERMTLDESC *desc) {
    if (mtl->unknown_b4) {
        *(NUSHADERMTLDESC *)mtl->desc = *desc;
    }
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
