#include "nudlist.h"

#include "numem.h"
#include "numtl.h"
#include "nuthread.h"

/* Display lists (PS2: nu3d/nudlist.c). Names, types and locals from the PS2 prototype; the
 * PC keeps the PS2's manager layout but has its own item format and an HTML debug dump.
 * In PC order. */

// GLOBAL: BIONICLE 0x34928c0
NUDISPLAYLISTMANAGER global_dlist_manager;

/* Starts a scene's frame: clears the used-material and used-clip-object bits of the
 * buffer being filled, and rewinds every material list. force_reset also marks all clip
 * objects of the other buffer used and restarts the lists on buffer 0. */
// FUNCTION: BIONICLE 0x553900
static void DisplayListBeforeFrame(NUDISPLAYSCENE *ds, int force_reset) {
    unsigned int i;
    int nbytes;
    int nblocks;
    NUDISPLAYLIST *dlist_mtl;

    if (!force_reset && !ds->dynamic_used_this_frame && !ds->scene_used_this_frame) {
        return;
    }
    if (ds->nmtls != 0) {
        NuMemSet128(ds->mtl_used[ds->buffer], 0, (ds->nmtls + 7) / 128 + 1);
    }
    if (ds->nclip_objects != 0) {
        nbytes = (ds->nclip_objects + 7) / 8 * 2;
        nblocks = nbytes / 16 + 1;
        NuMemSet128(ds->clip_used[ds->buffer], 0, nblocks);
        if (force_reset) {
            NuMemSet128(ds->clip_used[!ds->buffer], 0xff, nblocks);
        }
    }
    for (i = 0; i < ds->nmtls; i++) {
        dlist_mtl = ds->dlist_mtls[i];
        dlist_mtl->mtl_last = dlist_mtl->first;
        if (force_reset) {
            dlist_mtl->scene_buffer = 0;
        } else {
            dlist_mtl->scene_buffer = !dlist_mtl->scene_buffer;
        }
        dlist_mtl->scene_next = dlist_mtl->scene_first[dlist_mtl->scene_buffer];
        RndrStateClear(dlist_mtl->state);
    }
    ds->dynamic_used_this_frame = 0;
    ds->scene_used_this_frame = 0;
    ds->sorted_this_frame = 0;
}

/* Links a sort priority into the global list, which is kept in sort_pri order (inlined
 * on PS2; name ours). */
// FUNCTION: BIONICLE 0x553c90
static void DisplayListAddSortPri(NUSORTPRI *pri) {
    NUSORTPRI *plast;
    NUSORTPRI *p;

    plast = NULL;
    p = global_dlist_manager.sort_list;
    if (global_dlist_manager.sort_list == NULL) {
        global_dlist_manager.sort_list = pri;
        pri->sys_next = p;
    } else {
        while (p != NULL && p->sort_pri < pri->sort_pri) {
            plast = p;
            p = p->sys_next;
        }
        if (plast == NULL) {
            pri->sys_next = global_dlist_manager.sort_list;
            global_dlist_manager.sort_list = pri;
        } else {
            pri->sys_next = p;
            plast->sys_next = pri;
        }
    }
    global_dlist_manager.nsort_pris++;
}

/* Registers a scene's display list for drawing. */
// FUNCTION: BIONICLE 0x554560
void NuDisplaySceneAdd(NUDISPLAYSCENE *ds) {
    int i;

    NuThreadCriticalSectionBegin(global_dlist_manager.loading_critical_section);
    global_dlist_manager.dlists[global_dlist_manager.ndisplay_lists++] = ds;
    DisplayListBeforeFrame(ds, 1);
    for (i = 0; i < ds->nsort_pris; i++) {
        if (numtl_renderplane != 0) {
            ds->sort_pris[i].sort_pri += numtl_renderplane << 17;
        }
        DisplayListAddSortPri(&ds->sort_pris[i]);
    }
    ds->scene_supports_instancing = 0;
    ds->is_a_clone = 0;
    ds->mtl_anims = NULL;
    NuDisplaySceneAddPS(ds);
    NuThreadCriticalSectionEnd(global_dlist_manager.loading_critical_section);
}

/* Flips every scene to its other buffer for the new frame. */
// FUNCTION: BIONICLE 0x554b20
void NuDisplayListSwapBuffersBeginFrame(void) {
    int j;
    NUDISPLAYLISTMANAGER *dlm;
    NUDISPLAYSCENE *ds;
    NUSORTPRI *sp;

    dlm = &global_dlist_manager;
    NuThreadCriticalSectionBegin(dlm->loading_critical_section);
    dlm->dyn_mtl_dlist.buffer = !dlm->dyn_mtl_dlist.buffer;
    DisplayListBeforeFrame(&dlm->dyn_mtl_dlist, 0);
    for (j = 0; j < dlm->ndisplay_lists; j++) {
        ds = dlm->dlists[j];
        ds->buffer = !ds->buffer;
        DisplayListBeforeFrame(ds, 0);
        if (ds->gscene != NULL) {
            ds->gscene->multirender_current = 0;
        }
    }
    dlm->nrender_scenes = 0;
    for (sp = dlm->sort_list; sp != NULL; sp = sp->sys_next) {
        sp->rendered_this_frame = 0;
    }
    RndrStateResetSharedGlobalState();
    NuThreadCriticalSectionEnd(dlm->loading_critical_section);
}

/* Runs a display list: id 0 steps to the next item, 1 jumps, 3 calls the handler for
 * the item's type with its data, 4 ends. */
// FUNCTION: BIONICLE 0x555820
void NuDisplayListExecute(NUDISPLAYLISTITEM *item, NUDISPLAYLISTITEMFN *fntable) {
    int bContinue;
    NUDISPLAYLISTITEMFN pFunc;

    bContinue = 1;
    while (bContinue) {
        switch (item->id) {
        case 3:
            pFunc = fntable[item->type - 0x80];
            if (pFunc != NULL) {
                pFunc(item->next);
            }
        case 0:
            item++;
            break;
        case 1:
            item = item->next;
            break;
        case 4:
            bContinue = 0;
            break;
        }
    }
}
