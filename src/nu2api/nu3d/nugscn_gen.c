#include "nugscn.h"

#include "nufile.h"

/* Scene loading helpers shared by the platforms (PS2: nu3d/nugscn_gen.c). Names, types and
 * locals from the PS2 prototype. In PC order; the PS2's video memory hooks,
 * SetForceScenePlayBack, NuGScnFixupTID and NuGScnMtlLayerMask aren't in the PC exe. */

/* Instance animations of a scene with a display list: each special's anim holds an index
 * into the block (-1 for none) until here. */
// FUNCTION: BIONICLE 0x5536a0
void ReadInstAnimBlockDlist(NUFILE fh, NUGSCN *gscene) {
    int n;
    int animix;
    NUDISPLAYSCENE *ds;
    NUINSTANIM *instanim;

    ds = gscene->display_list;
    gscene->ninstanims = NuFileReadInt(fh);
    NuFileReadInt(fh);
    instanim = gscene->instanimblock = NuMemFileAddr(fh);
    if (instanim != NULL) {
        for (n = 0; n < ds->nspecials; n++) {
            animix = (int)ds->specials[n].anim;
            if (animix != -1) {
                ds->specials[n].anim = &instanim[animix];
                ds->specials[n].anim->mtx = ds->specials[n].mtx;
                ds->specials[n].anim->instance_ix = n;
            } else {
                ds->specials[n].anim = NULL;
            }
        }
    }
    if (gscene->ninstanims != 0) {
        gscene->prev_eval_mtx = NuGScnBufferAllocAligned(gscene->ninstanims * sizeof(NUMTX), 16);
    }
}

// FUNCTION: BIONICLE 0x553770
void ReadInstanceIDs(NUFILE fh, NUGSCN *gscene) {
    gscene->numinstanceixs = NuFileReadInt(fh);
    NuFileReadInt(fh);
    gscene->instanceixs = NuMemFileAddr(fh);
}

/* Instance animations of a scene without a display list. Version 0 files hand them out
 * in instance order to the instances with a (non-NULL) anim; later ones name each
 * animation's instance. */
// FUNCTION: BIONICLE 0x5537a0
void ReadInstAnimBlock(NUFILE fh, NUGSCN *gscene) {
    int n;
    int version;
    NUINSTANIM *instanim;

    gscene->ninstanims = NuFileReadInt(fh);
    version = NuFileReadInt(fh);
    gscene->instanimblock = NuMemFileAddr(fh);
    if (version == 0) {
        instanim = gscene->instanimblock;
        if (instanim != NULL) {
            for (n = 0; n < gscene->numinstance; n++) {
                if (gscene->instances[n].anim != NULL) {
                    gscene->instances[n].anim = instanim;
                    gscene->instances[n].anim->mtx = gscene->instances[n].mtx;
                    instanim++;
                    gscene->instances[n].anim->instance_ix = n;
                }
            }
        }
    } else if (gscene->instanimblock != NULL) {
        for (n = 0; n < gscene->ninstanims; n++) {
            instanim = &gscene->instanimblock[n];
            gscene->instances[instanim->instance_ix].anim = instanim;
            instanim->mtx = gscene->instances[instanim->instance_ix].mtx;
        }
    }
    if (gscene->ninstanims != 0) {
        gscene->prev_eval_mtx = NuGScnBufferAllocAligned(gscene->ninstanims * sizeof(NUMTX), 16);
    }
}

// FUNCTION: BIONICLE 0x553890
int NuGScnNumSpecials(NUGSCN *gsc) {
    if (gsc->display_list != NULL) {
        return gsc->display_list->nspecials;
    }
    return gsc->numspecial;
}

// FUNCTION: BIONICLE 0x5538b0
void NuGScnGetSpecial(NUHSPECIAL *spec, NUGSCN *gsc, int i) {
    if (gsc->display_list != NULL) {
        spec->scene = gsc;
        spec->special = NULL;
        spec->special_obj = &gsc->display_list->specials[i];
    } else {
        spec->scene = gsc;
        spec->special = &gsc->specials[i];
        spec->special_obj = NULL;
    }
}
