#include "nuspecial.h"

#include "nurndr.h"
#include "nustring.h"

/* Specials: named objects in a scene, either instances flagged special or standalone
 * special objects, reached through NUHSPECIAL handles. Names and locals from the PS2
 * prototype; logic checked against opensagadev/saga src/nu2api/nu3d/nuspecial.cpp
 * (GPL-3.0, a later engine). The PC scene can carry a display list with its own
 * special objects; the finders handle that case in static helpers (inlined on PS2; names
 * ours). */

// GLOBAL: BIONICLE 0x337a3bc
int nuspecial_draw_state;

// FUNCTION: BIONICLE 0x53e3d0
static int FindSpecialObj(NUGSCN *scene, NUHSPECIAL *special, char *name) {
    NUSPECIALOBJECT *sob;
    int n;

    if (scene == NULL) {
        return 0;
    }
    sob = scene->display_list->specials;
    for (n = 0; n < scene->display_list->nspecials; sob++, n++) {
        if (NuStrICmp(name, sob->name) == 0) {
            special->scene = scene;
            special->special = NULL;
            special->special_obj = sob;
            return 1;
        }
    }
    special->scene = NULL;
    special->special = NULL;
    special->special_obj = NULL;
    return 0;
}

// FUNCTION: BIONICLE 0x53e450
int NuSpecialFind(NUGSCN *scene, NUHSPECIAL *special, char *name, int ErrMsg) {
    NUSPECIAL *sp;
    int n;

    if (scene != NULL && name != NULL) {
        if (scene->display_list != NULL) {
            return FindSpecialObj(scene, special, name);
        }
        sp = scene->specials;
        for (n = 0; n < scene->numspecial; sp++, n++) {
            if (NuStrICmp(name, sp->name) == 0) {
                special->scene = scene;
                special->special = sp;
                special->special_obj = NULL;
                return 1;
            }
        }
        special->scene = NULL;
        special->special = NULL;
        special->special_obj = NULL;
        return 0;
    }
    special->scene = NULL;
    special->special = NULL;
    special->special_obj = NULL;
    return 0;
}

// FUNCTION: BIONICLE 0x53e4f0
static int FindMultiWCObj(NUGSCN *scene, NUHSPECIAL *special, char (*WCName)[20], char *name, int maxcnt) {
    NUSPECIALOBJECT *sob;
    int n;
    int cnt;

    cnt = 0;
    sob = scene->display_list->specials;
    for (n = 0; n < scene->display_list->nspecials; sob++, n++) {
        if (WCName != NULL) {
            if (NuStrICmpWC(name, sob->name, *WCName) != 0) {
                continue;
            }
        } else if (NuStrICmpWC(name, sob->name, NULL) != 0) {
            continue;
        }
        if (special != NULL) {
            special->scene = scene;
            special->special_obj = sob;
            special->special = NULL;
            special++;
        }
        if (WCName != NULL) {
            WCName++;
        }
        cnt++;
        if (cnt >= maxcnt) {
            return cnt;
        }
    }
    return cnt;
}

// FUNCTION: BIONICLE 0x53e590
int NuSpecialFindMultiWC(NUGSCN *scene, NUHSPECIAL *special, char (*WCName)[20], char *name, int maxcnt, int ErrMsg) {
    NUSPECIAL *sp;
    NUSPECIALOBJECT *sob;
    int n;
    int cnt;

    cnt = 0;
    sob = NULL;
    if (scene != NULL) {
        if (scene->display_list != NULL) {
            return FindMultiWCObj(scene, special, WCName, name, maxcnt);
        }
        sp = scene->specials;
        for (n = 0; n < scene->numspecial; sp++, sob++, n++) {
            if (WCName != NULL) {
                if (NuStrICmpWC(name, sp->name, *WCName) != 0) {
                    continue;
                }
            } else if (NuStrICmpWC(name, sp->name, NULL) != 0) {
                continue;
            }
            if (special != NULL) {
                special->scene = scene;
                special->special = sp;
                special->special_obj = scene->display_list != NULL ? sob : NULL;
                special++;
            }
            if (WCName != NULL) {
                WCName++;
            }
            cnt++;
            if (cnt >= maxcnt) {
                return cnt;
            }
        }
        return cnt;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x53e680
static int FindMultiObj(NUGSCN *scene, NUHSPECIAL *special, char *name, int maxcnt) {
    NUSPECIALOBJECT *sob;
    int n;
    int cnt;

    cnt = 0;
    sob = scene->display_list->specials;
    for (n = 0; n < scene->display_list->nspecials; sob++, n++) {
        if (NuStrIStr(sob->name, name) != NULL) {
            special->scene = scene;
            special->special_obj = sob;
            special->special = NULL;
            special++;
            if (++cnt >= maxcnt) {
                break;
            }
        }
    }
    return cnt;
}

// FUNCTION: BIONICLE 0x53e700
int NuSpecialFindMulti(NUGSCN *scene, NUHSPECIAL *special, char *name, int maxcnt, int ErrMsg) {
    NUSPECIAL *sp;
    NUSPECIALOBJECT *sob;
    int n;
    int cnt;

    cnt = 0;
    sob = NULL;
    if (scene != NULL) {
        if (scene->display_list != NULL) {
            return FindMultiObj(scene, special, name, maxcnt);
        }
        sp = scene->specials;
        for (n = 0; n < scene->numspecial; sp++, sob++, n++) {
            if (NuStrIStr(sp->name, name) != NULL) {
                special->scene = scene;
                special->special = sp;
                special->special_obj = scene->display_list != NULL ? sob : NULL;
                special++;
                cnt++;
                if (cnt >= maxcnt) {
                    return cnt;
                }
            }
        }
        return cnt;
    }
    return 0;
}

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
