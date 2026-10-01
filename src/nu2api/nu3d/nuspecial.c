#include "nuspecial.h"

#include "nurndr.h"

/* Specials: named objects in a scene, either instances flagged special or standalone
 * special objects, reached through NUHSPECIAL handles. Names and locals from the PS2
 * prototype; logic checked against opensagadev/saga src/nu2api/nu3d/nuspecial.cpp
 * (GPL-3.0, a later engine). Part 1: accessors. */

// GLOBAL: BIONICLE 0x337a3bc
int nuspecial_draw_state;

// FUNCTION: BIONICLE 0x53e7c0
NUVEC *NuSpecialGetPos(NUHSPECIAL *sph) {
    if (sph->special_obj != NULL) {
        return (NUVEC *)&sph->special_obj->mtx._30;
    } else if (sph->special != NULL) {
        return (NUVEC *)&sph->special->mtx._30;
    } else {
        return NULL;
    }
}

// FUNCTION: BIONICLE 0x53e7e0
NUVEC *NuSpecialGetDrawPos(NUHSPECIAL *sph) {
    if (sph->special_obj != NULL) {
        if (sph->special_obj->anim != (NUINSTANIM *)-1 && sph->special_obj->anim != NULL) {
            return (NUVEC *)&sph->special_obj->anim->mtx._30;
        }
        return (NUVEC *)&sph->special_obj->draw_mtx._30;
    }
    if (sph->special != NULL) {
        if (sph->special->instance->anim != NULL) {
            return (NUVEC *)&sph->special->instance->anim->mtx._30;
        }
        return (NUVEC *)&sph->special->instance->mtx._30;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x53e820
NUMTX *NuSpecialGetMtx(NUHSPECIAL *sph) {
    if (sph->special_obj != NULL) {
        return &sph->special_obj->mtx;
    }
    return &sph->special->mtx;
}

// FUNCTION: BIONICLE 0x53e830
void NuSpecialSetMtx(NUHSPECIAL *sph, NUMTX *mtx) {
    if (sph->special_obj != NULL) {
        sph->special_obj->mtx = *mtx;
    } else {
        sph->special->mtx = *mtx;
    }
}

// FUNCTION: BIONICLE 0x53e850
char *NuSpecialGetName(NUHSPECIAL *sph) {
    if (sph != NULL) {
        if (sph->special_obj != NULL) {
            return sph->special_obj->name;
        }
        if (sph->special != NULL) {
            return sph->special->name;
        }
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x53e8d0
int NuSpecialExistsFn(NUHSPECIAL *sph) {
    if (sph != NULL && (sph->special != NULL || sph->special_obj != NULL)) {
        return 1;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x53e8f0
void NuSpecialMtlMap(int nmtls, NUMTL **mtls) {
    if (nmtls == 0) {
        nurndr_forced_mtl_table = NULL;
        nuspecial_draw_state &= ~4;
    } else {
        nurndr_nforced_mtls = nmtls;
        nurndr_forced_mtl_table = mtls;
        nuspecial_draw_state |= 4;
    }
}

// FUNCTION: BIONICLE 0x53e930
void NuSpecialForceMtl(NUMTL *mtl) {
    if (mtl != NULL) {
        nuspecial_draw_state |= 8;
        nurndr_forced_mtl = mtl;
    } else {
        nuspecial_draw_state &= ~8;
        nurndr_forced_mtl = NULL;
    }
}

// FUNCTION: BIONICLE 0x53eaf0
int NuSpecialGetVisibilityFn(NUHSPECIAL *spec) {
    if (spec->scene != NULL) {
        if (spec->special != NULL) {
            return spec->special->instance->flags.visible;
        }
        if (spec->special_obj != NULL) {
            return (spec->special_obj->flags >> 1) & 1;
        }
    }
    return 0;
}

// FUNCTION: BIONICLE 0x53edc0
NUMTX *NuSpecialGetDrawMtx(NUHSPECIAL *special) {
    if (special->special != NULL) {
        if (special->special->instance->anim != NULL) {
            return &special->special->instance->anim->mtx;
        }
        return &special->special->instance->mtx;
    }
    if (special->special_obj != NULL) {
        if (special->special_obj->anim != (NUINSTANIM *)-1 && special->special_obj->anim != NULL) {
            return &special->special_obj->anim->mtx;
        }
        return &special->special_obj->draw_mtx;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x53eef0
NUINSTANIM *NuSpecialGetInstAnim(NUHSPECIAL *special) {
    if (special->special != NULL) {
        return special->special->instance->anim;
    }
    if (special->special_obj != NULL && special->special_obj->anim != (NUINSTANIM *)-1 &&
        special->special_obj->anim != NULL) {
        return special->special_obj->anim;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x53ef20
int NuSpecialCompare(NUHSPECIAL *special1, NUHSPECIAL *special2) {
    if ((special1->special != NULL && special1->special == special2->special) ||
        (special1->special_obj != NULL && special1->special_obj == special2->special_obj)) {
        return 1;
    }
    return 0;
}
