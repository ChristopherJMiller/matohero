#include "nudlist.h"

#include "folded.h"
#include "nuhtml.h"
#include "numem.h"
#include "numtl.h"
#include "nustring.h"
#include "nuthread.h"

#include <stdio.h>

/* Display lists (PS2: nu3d/nudlist.c). Names, types and locals from the PS2 prototype; the
 * PC keeps the PS2's manager layout but has its own item format and an HTML debug dump.
 * In PC order. */

// GLOBAL: BIONICLE 0x34928bc
static int capture_dlist;

// GLOBAL: BIONICLE 0x34928c0
NUDISPLAYLISTMANAGER global_dlist_manager;

// GLOBAL: BIONICLE 0x616eb0
int nudlist_debug_level = 2;

// GLOBAL: BIONICLE 0x3492e18
int do_capture;

// GLOBAL: BIONICLE 0x35bba74
NUFILE capture_fh;

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

/* One line of the HTML dump for an item (PS2 name). */
// FUNCTION: BIONICLE 0x555140
void DisplayListPrintItem(NUDISPLAYLISTITEM *item, int id, int nfilter, int *filter, NUFILE fh) {
    int i;
    int do_write;
    char buffer1[256];
    char buffer2[256];

    buffer2[0] = '\0';
    if (fh) {
        if (item->id == 0) {
            // STRING: BIONICLE 0x5e05a0
            sprintf(buffer2, "<font color = \"#00a000\">");
        } else {
            // STRING: BIONICLE 0x5e0584
            sprintf(buffer2, "<font color = \"#0000a0\">");
        }
    }
    // STRING: BIONICLE 0x5e057c
    sprintf(buffer1, "%d: ", id);
    NuStrCat(buffer2, buffer1);
    switch (item->type) {
    case 0x87:
        // STRING: BIONICLE 0x5e0568
        sprintf(buffer1, "DUMMY            ");
        break;
    case 0x80:
        // STRING: BIONICLE 0x5e0554
        sprintf(buffer1, "MTL              ");
        break;
    case 0x81:
        // STRING: BIONICLE 0x5e0540
        sprintf(buffer1, "TXTRLOAD         ");
        break;
    case 0x82:
        // STRING: BIONICLE 0x5e052c
        sprintf(buffer1, "GEOMCALL         ");
        break;
    case 0x93:
        // STRING: BIONICLE 0x5e0518
        sprintf(buffer1, "GEOMCALL2D       ");
        break;
    case 0x97:
        // STRING: BIONICLE 0x5e0504
        sprintf(buffer1, "LINECALL2D       ");
        break;
    case 0x9f:
        // STRING: BIONICLE 0x5e04f0
        sprintf(buffer1, "LINEBUFFER       ");
        break;
    case 0x8f:
        // STRING: BIONICLE 0x5e04dc
        sprintf(buffer1, "FACEON           ");
        break;
    case 0x83:
        // STRING: BIONICLE 0x5e04c8
        sprintf(buffer1, "MTXLOAD          ");
        break;
    case 0x90:
        // STRING: BIONICLE 0x5e04b4
        sprintf(buffer1, "FACEON_TRANSFORM ");
        break;
    case 0x84:
        // STRING: BIONICLE 0x5e04a0
        sprintf(buffer1, "TERMINATE        ");
        break;
    case 0x85:
        // STRING: BIONICLE 0x5e048c
        sprintf(buffer1, "MTL_CLIP         ");
        break;
    case 0x86:
        // STRING: BIONICLE 0x5e0478
        sprintf(buffer1, "CLUT             ");
        break;
    case 0x88:
        // STRING: BIONICLE 0x5e0464
        sprintf(buffer1, "DLIST            ");
        break;
    case 0x8c:
        // STRING: BIONICLE 0x5e0450
        sprintf(buffer1, "TRANSFORM_REF    ");
        break;
    case 0x8d:
        // STRING: BIONICLE 0x5e043c
        sprintf(buffer1, "NEXT             ");
        break;
    case 0x8b:
        // STRING: BIONICLE 0x5e0428
        sprintf(buffer1, "DYNAMIC GEOMETRY ");
        break;
    case 0x94:
        // STRING: BIONICLE 0x5e0414
        sprintf(buffer1, "LIGHTS           ");
        break;
    case 0x9a:
        // STRING: BIONICLE 0x5e0400
        sprintf(buffer1, "CAMERA           ");
        break;
    case 0x95:
        // STRING: BIONICLE 0x5e03ec
        sprintf(buffer1, "DEBUG GEOMCALL   ");
        break;
    case 0x9b:
        // STRING: BIONICLE 0x5e03d8
        sprintf(buffer1, "FX SCREEN COPY   ");
        break;
    case 0x9c:
        // STRING: BIONICLE 0x5e03c4
        sprintf(buffer1, "FX SUN FLARE     ");
        break;
    case 0x9d:
        // STRING: BIONICLE 0x5e03b0
        sprintf(buffer1, "FX SCREEN FILTER ");
        break;
    case 0x9e:
        // STRING: BIONICLE 0x5e039c
        sprintf(buffer1, "FX SPEED BLUR    ");
        break;
    case 0xa0:
        // STRING: BIONICLE 0x5e0388
        sprintf(buffer1, "GEOM CLIPPED     ");
        break;
    case 0x8e:
        // STRING: BIONICLE 0x5e0374
        sprintf(buffer1, "END ITEM         ");
        break;
    default:
        if (DisplayListPrintItemPS(item, buffer1)) {
            // STRING: BIONICLE 0x5e035c
            sprintf(buffer1, "ERROR: Unknown item.");
        }
        break;
    }
    NuStrCat(buffer2, buffer1);
    // STRING: BIONICLE 0x5e0354
    sprintf(buffer1, "(%x)", item->next);
    NuStrCat(buffer2, buffer1);
    if (nfilter == 0) {
        do_write = 1;
    } else {
        do_write = 0;
        for (i = 0; i < nfilter; i++) {
            if (filter[i] == item->type) {
                do_write = 1;
                break;
            }
        }
    }
    if (nudlist_debug_level >= 1 && do_write && fh) {
        NuHtmlWrite(buffer2);
        // STRING: BIONICLE 0x5e034c
        NuHtmlWrite("</br>\n");
        if (item->id == 0) {
            // STRING: BIONICLE 0x5e0330
            NuHtmlWrite("<font color = \"#000000\">");
        }
    }
    if (fh) {
        NuDisplayListDebugToFile(item, fh);
    }
}

/* Dumps a run of items up to its END ITEM (or TERMINATE with flags 2), following jumps
 * with flags 1 (inlined into NuDisplaySceneDebug on PS2; name ours). */
// FUNCTION: BIONICLE 0x5554a0
static void DisplayListPrintItems(NUDISPLAYLISTITEM *item, int flags, int nfilter, int *filter, NUFILE fh) {
    int type;
    int id;

    id = 0;
    if (item == NULL) {
        return;
    }
    type = (flags & 2) ? 0x84 : 0x8e;
    do {
        DisplayListPrintItem(item, id, nfilter, filter, fh);
        if ((flags & 1) && item->id == 1) {
            item = item->next;
        } else {
            item++;
        }
        id++;
    } while (item->type != type);
    DisplayListPrintItem(item, id, nfilter, filter, fh);
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

// FUNCTION: BIONICLE 0x555eb0
void NuDisplayListDrawRenderScene(int scene_id) {
    NUDISPLAYLISTMANAGER *dlm;
    int j;
    int nsort_pris;
    NUSORTPRI *sp;

    dlm = &global_dlist_manager;
    NuThreadCriticalSectionBegin(dlm->loading_critical_section);
    nsort_pris = dlm->render_scenes[scene_id]->nsort_pris;
    for (j = 0; j < nsort_pris; j++) {
        sp = dlm->render_scenes[scene_id]->sort_pris[j];
        if (do_capture) {
            if (sp->display_scene != NULL && sp->display_scene->name != NULL) {
                // STRING: BIONICLE 0x5e05fc
                NuHtmlHeading1("Display Scene : %s (sort: %d)", sp->display_scene->name, sp->sort_pri);
            } else {
                // STRING: BIONICLE 0x5e05d8
                NuHtmlHeading1("Display Scene : UNKNOWN (sort: %d)", sp->sort_pri);
            }
            // STRING: BIONICLE 0x5e05bc
            NuHtmlWrite("<font face=courier new>\n");
            DisplayListPrintItems(sp->items, 3, 0, NULL, capture_fh);
        }
        DisplayListExecutePS(sp->items);
    }
    DisplayListExecutePS(&dlm->render_scenes[scene_id]->render_2d_first);
    NuThreadCriticalSectionEnd(dlm->loading_critical_section);
}

/* Draws every render scene; a set capture_dlist dumps this frame's lists to dlist.htm. */
// FUNCTION: BIONICLE 0x555f80
void NuDisplayListDrawAll(void) {
    int i;

    if (capture_dlist != 0) {
        nudlist_debug_level = capture_dlist - 1;
        capture_dlist = 0;
        do_capture = 1;
        // STRING: BIONICLE 0x5e0324
        capture_fh = NuFileOpen("dlist.htm", NUFILE_WRITE);
        NuHtmlBegin(capture_fh);
        // STRING: BIONICLE 0x5e030c
        NuHtmlWrite("<body bgcolor=#C0C0C0>");
    }
    if (global_dlist_manager.nrender_scenes == 0) {
        NuDisplayListAddRenderScene();
    }
    for (i = 0; i < global_dlist_manager.nrender_scenes; i++) {
        NuDisplayListDrawRenderScene(i);
    }
    if (do_capture) {
        NuHtmlBanner();
        NuHtmlEnd();
        NuFileClose(capture_fh);
        do_capture = 0;
    }
}
