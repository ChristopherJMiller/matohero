#include "nuspecial.h"

#include "nurndr.h"
#include "nustring.h"

/* Specials: named objects in a scene, either instances flagged special or standalone
 * special objects, reached through NUHSPECIAL handles. Names and locals from the PS2
 * prototype; logic checked against opensagadev/saga src/nu2api/nu3d/nuspecial.cpp
 * (GPL-3.0, a later engine). The PC scene can carry a display list with its own
 * special objects; the finders handle that case in static helpers (inlined on PS2; names
 * ours). */

// GLOBAL: BIONICLE 0x337a3b4
int nuspecial_const_alpha_enabled;

// GLOBAL: BIONICLE 0x616ba0
float nuspecial_const_alpha = 1.0f;

// GLOBAL: BIONICLE 0x337a3b8
int nuspecial_const_tint_enabled;

// GLOBAL: BIONICLE 0x616ba4
NUCOLOUR3 nuspecial_const_tint = {1.0f, 1.0f, 1.0f};

/* The outcode specials are drawn with; -1 clip-tests them. */
// GLOBAL: BIONICLE 0x616bb0
int nuspecial_clip_state = -1;

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

// FUNCTION: BIONICLE 0x53e880
int NuSpecialDrawAt(NUHSPECIAL *special, NUMTX *transform) {
    if (special != NULL && special->scene != NULL) {
        if (special->special != NULL && special->special->instance != NULL) {
            return NuRndrGScnObj(special->scene->gobjs[special->special->instance->objid], transform);
        }
        if (special->special_obj != NULL) {
            return NuDisplayListRndrSpecial(special, transform, 0, NULL, NULL);
        }
    }
    return 0;
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

// FUNCTION: BIONICLE 0x53e960
void NuSpecialSetVisibility(NUHSPECIAL *spec, int visibility) {
    NUSPECIALOBJECT *sob;
    NUINSTANCE *inst;

    if (spec == NULL || spec->scene == NULL) {
        return;
    }
    if (spec->special != NULL) {
        inst = spec->special->instance;
        if (inst != NULL) {
            inst->flags.visible = visibility;
        }
        if (visibility) {
            spec->special->flags |= 0x200;
        } else {
            spec->special->flags &= ~0x200;
        }
        return;
    }
    sob = spec->special_obj;
    if (sob != NULL) {
        if (visibility) {
            sob->flags |= 0x202;
            if (spec->scene->display_list->scene_specials) {
                spec->scene->display_list->visibility_flags[sob->instance_ix] |= 1;
            }
        } else {
            spec->special_obj->flags &= ~2;
            spec->special_obj->flags &= ~0x200;
            if (spec->scene->display_list->scene_specials) {
                spec->scene->display_list->visibility_flags[sob->instance_ix] &= ~1;
            }
        }
    }
}

// FUNCTION: BIONICLE 0x53ea40
void NuSpecialSetCollision(NUHSPECIAL *spec, int collision) {
    if (spec != NULL && spec->scene != NULL) {
        if (spec->special != NULL) {
            if (collision) {
                spec->special->flags |= 0x200;
            } else {
                spec->special->flags &= ~0x200;
            }
        } else if (spec->special_obj != NULL) {
            if (collision) {
                spec->special_obj->flags |= 0x200;
            } else {
                spec->special_obj->flags &= ~0x200;
            }
        }
    }
}

// FUNCTION: BIONICLE 0x53eab0
int NuSpecialGetCollision(NUHSPECIAL *spec) {
    if (spec != NULL && spec->scene != NULL) {
        if (spec->special != NULL) {
            return spec->special->flags & 0x200;
        }
        if (spec->special_obj != NULL) {
            return spec->special_obj->flags & 0x200;
        }
    }
    return 0;
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

// FUNCTION: BIONICLE 0x53eb20
int NuSpecialGetInstanceix(NUHSPECIAL *special) {
    int i;

    if (special->special != NULL) {
        for (i = 0; i < special->scene->numinstance; i++) {
            if (&special->scene->instances[i] == special->special->instance) {
                return i;
            }
        }
        return -1;
    }
    if (special->special_obj != NULL) {
        return special->special_obj->instance_ix;
    }
    return -1;
}

// FUNCTION: BIONICLE 0x53eb60
int NuSpecialNumMtls(NUHSPECIAL *special) {
    NUGOBJ *gobj;
    NUGEOM *geom;
    int cnt;
    int n;

    cnt = 0;
    if (special->special != NULL) {
        gobj = special->scene->gobjs[special->special->instance->objid];
        while (gobj->next_gobj != NULL) {
            gobj = gobj->next_gobj;
        }
        for (geom = gobj->geom; geom != NULL; geom = geom->next) {
            cnt++;
        }
        return cnt;
    }
    if (special->special_obj != NULL) {
        for (n = 0; special->special_obj->clip_range[n] != 0.0f; n++) {
        }
        return special->special_obj->clip_objects[n].nclip_items;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x53ebf0
NUMTL *NuSpecialGetMtl(NUHSPECIAL *special, int i) {
    NUGOBJ *gobj;
    NUGEOM *geom;
    int n;

    if (special->special != NULL) {
        gobj = special->scene->gobjs[special->special->instance->objid];
        while (gobj->next_gobj != NULL) {
            gobj = gobj->next_gobj;
        }
        geom = gobj->geom;
        for (; i != 0; i--) {
            if (geom == NULL) {
                return NULL;
            }
            geom = geom->next;
        }
        return geom->mtl;
    }
    if (special->special_obj != NULL) {
        for (n = 0; special->special_obj->clip_range[n] != 0.0f; n++) {
        }
        return special->scene->display_list->mtls[special->special_obj->clip_objects[n].mtls[i]];
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x53eca0
void NuSpecialGetBounds(NUHSPECIAL *special, NUVEC *min, NUVEC *max) {
    NUGOBJ *gobj;

    if (special->special != NULL) {
        gobj = special->scene->gobjs[special->special->instance->objid];
        while (gobj->next_gobj != NULL) {
            gobj = gobj->next_gobj;
        }
        *min = gobj->min;
        *max = gobj->max;
    } else if (special->special_obj != NULL) {
        *min = *(NUVEC *)&special->special_obj->min;
        *max = *(NUVEC *)&special->special_obj->max;
    }
}

// FUNCTION: BIONICLE 0x53ed40
void NuSpecialGetRadius(NUHSPECIAL *special, NUVEC *cntr, float *radius) {
    if (special->special != NULL) {
        *radius = special->scene->gobjs[special->special->instance->objid]->cntr_r;
        *cntr = special->scene->gobjs[special->special->instance->objid]->cntr;
    } else {
        *cntr = *(NUVEC *)&special->special_obj->sphere;
        *radius = special->special_obj->sphere.w;
    }
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

// FUNCTION: BIONICLE 0x53ee00
void NuSpecialSetDrawMtx(NUHSPECIAL *special, NUMTX *mtx) {
    if (special != NULL && special->scene != NULL) {
        if (special->special != NULL) {
            special->special->instance->mtx = *mtx;
        } else {
            special->special_obj->draw_mtx = *mtx;
            if (special->scene->display_list->scene_specials) {
                NuDisplayListUpdateSpecial(special);
                special->special_obj->flags |= 0x400;
            }
        }
    }
}

// FUNCTION: BIONICLE 0x53ee60
void NuSpecialSetDrawPos(NUHSPECIAL *special, NUVEC *pos) {
    if (special != NULL && special->scene != NULL) {
        if (special->special != NULL) {
            special->special->instance->mtx._30 = pos->x;
            special->special->instance->mtx._31 = pos->y;
            special->special->instance->mtx._32 = pos->z;
        } else {
            special->special_obj->draw_mtx._30 = pos->x;
            special->special_obj->draw_mtx._31 = pos->y;
            special->special_obj->draw_mtx._32 = pos->z;
            if (special->scene->display_list->scene_specials) {
                NuDisplayListUpdateSpecial(special);
                special->special_obj->flags |= 0x400;
            }
        }
    }
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
