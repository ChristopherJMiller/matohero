#include "nudlist.h"

#include "nuthread.h"

/* PC counterpart of the PS2's nu3d/ps2/nudlistPS2.c (file name ours). */

/* The PC item handlers, by type - 0x80 (name ours). The original's table holds the
 * addresses of handlers not decompiled yet; this is a placeholder of the same size. */
// GLOBAL: BIONICLE 0x35bba7c
VARIPTR *display_list_buffer;

// GLOBAL: BIONICLE 0x35bba78
VARIPTR display_list_buffer_end;

/* The PC's own item buffer and its end (names ours). */
// GLOBAL: BIONICLE 0x3606ca0
VARIPTR render_buffer;

// GLOBAL: BIONICLE 0x35bba70
VARIPTR render_buffer_end;

// GLOBAL: BIONICLE 0x617020
NUDISPLAYLISTITEMFN DisplayListFnTable[33];

/* Resets the scene's material items to their unclipped type. On PC the two types share
 * the value 0x82, so the test and the store look redundant. */
// FUNCTION: BIONICLE 0x55b8e0
void NuDisplaySceneAddPS(NUDISPLAYSCENE *ds) {
    int i;
    NUDISPLAYLISTITEM *item;

    for (i = 0; i < ds->nitems; i++) {
        item = &ds->items[i];
        if (item->type == 0x82) {
            item->type = 0x82;
        }
    }
}

/* Runs a display list with the PC handlers (name ours). */
// FUNCTION: BIONICLE 0x55b700
void DisplayListExecutePS(NUDISPLAYLISTITEM *item) {
    NuDisplayListExecute(item, DisplayListFnTable);
}

/* PC: links items into the PC's own render buffer rather than the caller's. */
// FUNCTION: BIONICLE 0x55b6c0
void NuDisplayListInit(VARIPTR *buff, VARIPTR buffend) {
    global_dlist_manager.dlist_2d.first = &global_dlist_manager.dlist_2d_first;
    DisplayListCreateDynMtlList(buff, buffend);
    display_list_buffer = &render_buffer;
    display_list_buffer_end = render_buffer_end;
    global_dlist_manager.loading_critical_section = NuThreadCreateCriticalSection();
}

/* Also NuDisplayListAddTexture, AddMicrocode and AddLightState (see nudlist.h). */
// FUNCTION: BIONICLE 0x55b720
void NuDisplayListAddClut(NUDISPLAYLISTITEM *item, int tid) {
    item->type = 0x87;
    item->next = NULL;
    item->id = 0;
}

// FUNCTION: BIONICLE 0x55b730
void NuDisplayListAddMaterialState(NUDISPLAYLISTITEM *item, NUMTL *mtl) {
    item->type = 0x80;
    item->next = mtl;
    item->id = 3;
}

/* Appends a call item and a jump back to the dynamic geometry to a material's list.
 * Without a call address the data follows in the buffer, which is returned. */
// FUNCTION: BIONICLE 0x55b750
VARIPTR *NuDisplayListLinkItem(NUDISPLAYLIST *dl, unsigned char type, void *call_addr) {
    VARIPTR *buff;

    display_list_buffer->intaddr = (display_list_buffer->intaddr + 15) & ~15;
    buff = display_list_buffer;
    dl->mtl_last->next = buff->item;
    buff->item->type = type;
    buff->item->id = 3;
    if (call_addr != NULL) {
        buff->item->next = call_addr;
    } else {
        buff->item->next = buff->item + 2;
    }
    buff->item[1].type = 0x8d;
    buff->item[1].id = 1;
    buff->item[1].next = dl->dyn_geom + 1;
    dl->mtl_last = buff->item + 1;
    buff->intaddr = buff->intaddr + 0x20;
    return call_addr != NULL ? NULL : buff;
}

/* Appends room for n items to a material's list, then a jump back to the dynamic
 * geometry. */
// FUNCTION: BIONICLE 0x55b7d0
VARIPTR *NuDisplayListLinkItems(NUDISPLAYLIST *dlist, int n) {
    VARIPTR *buff;

    display_list_buffer->intaddr = (display_list_buffer->intaddr + 15) & ~15;
    buff = display_list_buffer;
    dlist->mtl_last->next = buff->item;
    dlist->add_item = buff->item;
    buff->item += n;
    buff->item->type = 0x8d;
    buff->item->id = 1;
    buff->item->next = dlist->dyn_geom + 1;
    dlist->mtl_last = buff->item;
    buff->intaddr += 0x10;
    return buff;
}

// FUNCTION: BIONICLE 0x55b830
void *DisplayListCreateRigidSkinTransformPS(VARIPTR *buff, NUMTX *mtx) {
    NUMTX *rv;

    buff->intaddr = (buff->intaddr + 3) & ~3;
    rv = buff->mtx44;
    *rv = *mtx;
    buff->intaddr += sizeof(NUMTX);
    return rv;
}

/* Copies the matrices a skinned geometry uses (its index list at +8, ended by 0xff, at
 * most 8) after a count, and returns where they start. */
// FUNCTION: BIONICLE 0x55b860
void *DisplayListCreateSkinTransformPS(VARIPTR *buff, NUMTX *mtx, DEFORMERWEIGHTSARRAY *blend, void *skingeom, void **geom, void *return_item) {
    VARIPTR skhdr;
    int mtxix;
    unsigned char *src;
    VARIPTR start;

    start = *buff;
    *geom = skingeom;
    skhdr = *buff;
    buff->intaddr += 4;
    src = (unsigned char *)skingeom + 8;
    *skhdr.s32 = 0;
    for (mtxix = 0; src[mtxix] != 0xff && mtxix < 8; mtxix++) {
        *buff->mtx44 = mtx[src[mtxix]];
        buff->intaddr += sizeof(NUMTX);
        (*skhdr.s32)++;
    }
    return start.voidptr;
}
