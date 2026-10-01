#ifndef NUDLIST_H
#define NUDLIST_H

#include "nugscn.h"

/* Display lists: the scenes' material lists, sorted by priority and drawn each frame. */

typedef struct nudisplaylistrenderscene_s NUDISPLAYLISTRENDERSCENE;
typedef struct numtlanimset_s NUMTLANIMSET;

/* PS2 layout; the PC's global_dlist_manager offsets agree with it. */
typedef struct nudisplaylistmanager_s {
    int ndisplay_lists;
    NUDISPLAYSCENE *dlists[256];
    NUDISPLAYSCENE dyn_mtl_dlist;
    int nnew_materials;
    NUMTL **new_materials;
    int ndel_materials;
    NUMTL **del_materials;
    unsigned char *material_used;
    NUMTL *temp_mtl;
    VARIPTR mtlbuff;
    VARIPTR mtlbuffend;
    unsigned char *mtl_buffers_used;
    NUDISPLAYLIST dlist_2d;
    NUDISPLAYLISTITEM dlist_2d_first;
    NUDISPLAYLIST *current_dlist;
    int nsort_pris;
    NUSORTPRI *sort_list;
    int nrender_scenes;
    NUDISPLAYLISTRENDERSCENE *render_scenes[8];
    int maxfx;
    char *fx_used;
    NUSORTPRI *fx_sort_pris;
    NUDISPLAYLISTITEM *fx_items;
    int loading_critical_section;
    NUMTLANIMSET *mtlanim_list;
} NUDISPLAYLISTMANAGER;

extern NUDISPLAYLISTMANAGER global_dlist_manager;

void NuDisplaySceneAdd(NUDISPLAYSCENE *ds);
void NuDisplayListSwapBuffersBeginFrame(void);
void NuDisplayListExecute(NUDISPLAYLISTITEM *item, NUDISPLAYLISTITEMFN *fntable);

/* pc/nudlist_pc.c (PS2: ps2/nudlistPS2.c). */
void NuDisplaySceneAddPS(NUDISPLAYSCENE *ds);

/* PS2: nu3d/ps2/nurndrstat.c, not decompiled yet. */
void RndrStateClear(NURNDRSTATE *state);
void RndrStateResetSharedGlobalState(void);

#endif
