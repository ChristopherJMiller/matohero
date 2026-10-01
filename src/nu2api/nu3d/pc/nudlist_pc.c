#include "nudlist.h"

/* PC counterpart of the PS2's nu3d/ps2/nudlistPS2.c (file name ours). */

/* The PC item handlers, by type - 0x80 (name ours). The original's table holds the
 * addresses of handlers not decompiled yet; this is a placeholder of the same size. */
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
