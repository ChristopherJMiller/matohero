#include "nudlist.h"

#include "folded.h"
#include "nufloat.h"
#include "nuhtml.h"
#include "numem.h"
#include "numtl.h"
#include "nuspecial.h"
#include "nuvu0.h"
#include "nustring.h"
#include "nuthread.h"

#include <stdio.h>
#include <string.h>

/* Display lists (PS2: nu3d/nudlist.c). Names, types and locals from the PS2 prototype; the
 * PC keeps the PS2's manager layout but has its own item format and an HTML debug dump.
 * In PC order. */

// GLOBAL: BIONICLE 0x34928bc
static int capture_dlist;

// GLOBAL: BIONICLE 0x34928c0
NUDISPLAYLISTMANAGER global_dlist_manager;

// GLOBAL: BIONICLE 0x616eb4
static int items_per_mtl = 8;

// GLOBAL: BIONICLE 0x616eb8
static int dyn_geom_pos = 6;

// GLOBAL: BIONICLE 0x616ebc
static int next_pos = 7;

// GLOBAL: BIONICLE 0x616eb0
int nudlist_debug_level = 2;

// GLOBAL: BIONICLE 0x3492e18
int do_capture;

// GLOBAL: BIONICLE 0x35bba74
NUFILE capture_fh;

/* A material's list: a clip header that jumps past the list when the material is
 * clipped, the state items, a jump to the dynamic geometry and the end. The item argument
 * isn't used on PC: the list is taken from the buffer. */
// FUNCTION: BIONICLE 0x553cf0
NUDISPLAYLISTITEM *NuDisplayListCreateMtlDlist(NUDISPLAYLISTITEM *item, NUMTL *mtl, VARIPTR *buff, VARIPTR buffend) {
    item = buff->item;
    buff->intaddr += 8 * sizeof(NUDISPLAYLISTITEM);
    item[0].type = 0x85;
    item[0].next = &item[items_per_mtl - 1];
    item[0].id = 1;
    NuDisplayListAddClut(&item[1], mtl->tid);
    NuDisplayListAddTexture(&item[2], mtl->tid);
    NuDisplayListAddMaterialState(&item[3], mtl);
    NuDisplayListAddMicrocode(&item[4], mtl);
    NuDisplayListAddLightState(&item[5], mtl);
    item[6].next = &item[items_per_mtl - 1];
    item[6].type = 0x8b;
    item[6].id = 1;
    item[7].type = 0x84;
    item[7].next = NULL;
    item[7].id = 4;
    return item;
}

/* Points each of the scene's sort priorities at its first material: the one whose render
 * plane and sort priority make up its key (name ours; the PS2 inlines it into
 * DisplayListLinkDynamicMtls). */
// FUNCTION: BIONICLE 0x553d90
void DisplayListFindFirstMtls(NUDISPLAYSCENE *ds) {
    int i;
    int j;
    int nmtls;
    NUMTL *mtl;

    nmtls = 0;
    for (i = 0; i < ds->nsort_pris; i++) {
        nmtls += ds->sort_pris[i].nmtls;
    }
    for (i = 0; i < ds->nsort_pris; i++) {
        if (ds->sort_pris[i].nmtls == 0) {
            continue;
        }
        for (j = 0; j < nmtls; j++) {
            mtl = ds->mtls[j];
            if ((mtl->renderplane << 17) + mtl->sort_pri == ds->sort_pris[i].sort_pri) {
                ds->sort_pris[i].mtl_first = j;
                break;
            }
        }
    }
}

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

/* Lays a scene's display list out in the buffer: the items, the material tables, the
 * per-material lists with their render states, the used-material bits for both buffers
 * and the sort priorities. alloc_mtls also allocates the materials. The PC doesn't use
 * the clip object, bounds and special counts here. */
// FUNCTION: BIONICLE 0x5539f0
void NuDisplayListCreate(NUDISPLAYSCENE *dlist, VARIPTR *buff, VARIPTR buffend, int nitems, int nmtls, int nclip_objects, int nbounds, int nsort_pris, int nspecials, int alloc_mtls) {
    int nbytes;
    int i;
    unsigned int j;
    NUDISPLAYLIST *dmtl;
    NUMTL *mtl;
    NUDISPLAYLISTITEM *firsts;
    NUDISPLAYLISTITEM *item;

    buff->intaddr = (buff->intaddr + 15) & ~15;
    dlist->nitems = nitems;
    dlist->items = (NUDISPLAYLISTITEM *)((buff->intaddr + 15) & ~15);
    buff->intaddr = ((buff->intaddr + 15) & ~15) + nitems * sizeof(NUDISPLAYLISTITEM);
    dlist->nmtls = nmtls;
    buff->intaddr = (buff->intaddr + 15) & ~15;
    dlist->mtls = (NUMTL **)((buff->intaddr + 15) & ~15);
    buff->intaddr = ((buff->intaddr + 15) & ~15) + nmtls * sizeof(NUMTL *);
    dlist->dlist_mtls = (NUDISPLAYLIST **)((buff->intaddr + 15) & ~15);
    buff->intaddr = ((buff->intaddr + 15) & ~15) + nmtls * sizeof(NUDISPLAYLIST *);
    memset(dlist->mtls, 0, nmtls * sizeof(NUMTL *));
    memset(dlist->dlist_mtls, 0, nmtls * sizeof(NUDISPLAYLIST *));
    if (alloc_mtls) {
        mtl = (NUMTL *)((buff->intaddr + 15) & ~15);
        buff->intaddr = ((buff->intaddr + 15) & ~15) + nmtls * sizeof(NUMTL);
        for (i = 0; i < nmtls; i++) {
            dlist->mtls[i] = mtl++;
        }
    }
    dmtl = (NUDISPLAYLIST *)((buff->intaddr + 15) & ~15);
    buff->intaddr = ((buff->intaddr + 15) & ~15) + nmtls * sizeof(NUDISPLAYLIST);
    memset(dmtl, 0, nmtls * sizeof(NUDISPLAYLIST));
    firsts = (NUDISPLAYLISTITEM *)((buff->intaddr + 15) & ~15);
    buff->intaddr = ((buff->intaddr + 15) & ~15) + nmtls * sizeof(NUDISPLAYLISTITEM);
    memset(firsts, 0, nmtls * sizeof(NUDISPLAYLISTITEM));
    for (i = 0; i < nmtls; i++) {
        dlist->dlist_mtls[i] = dmtl;
        dmtl->state = (NURNDRSTATE *)((buff->intaddr + 3) & ~3);
        buff->intaddr = ((buff->intaddr + 3) & ~3) + sizeof(NURNDRSTATE);
        dmtl->first = &firsts[i];
        RndrStateClear(dmtl->state);
        dmtl++;
    }
    dlist->local_state = NULL;
    for (j = 0; j < dlist->nmtls; j++) {
        item = dlist->dlist_mtls[j]->first;
        item->type = 0x8d;
        item->next = NULL;
        item->id = 1;
    }
    nbytes = ((nmtls + 7) / 8 / 16 + 1) * 16;
    dlist->mtl_used[0] = (unsigned char *)((buff->intaddr + 15) & ~15);
    buff->intaddr = ((buff->intaddr + 15) & ~15) + nbytes;
    dlist->mtl_used[1] = (unsigned char *)((buff->intaddr + 15) & ~15);
    buff->intaddr = ((buff->intaddr + 15) & ~15) + nbytes;
    dlist->nsort_pris = nsort_pris;
    dlist->sort_pris = (NUSORTPRI *)buff->voidptr;
    buff->intaddr += nsort_pris * sizeof(NUSORTPRI);
    memset(dlist->sort_pris, 0, nsort_pris * sizeof(NUSORTPRI));
    for (i = 0; i < nsort_pris; i++) {
        dlist->sort_pris[i].display_scene = dlist;
        dlist->sort_pris[i].nmtls = 0;
        dlist->sort_pris[i].mtl_first = 0;
    }
    DisplayListCreatePS(dlist, buff, buffend);
    DisplayListBeforeFrame(dlist, 1);
}

/* Links a sort priority into the global list, which is kept in sort_pri order (name
 * ours). Inlined into DisplayListLinkDynamicMtls (as on PS2, where it has no copy of its
 * own); NuDisplaySceneAdd calls it. */
// FUNCTION: BIONICLE 0x553c90
static __inline void DisplayListAddSortPri(NUSORTPRI *pri) {
    NUSORTPRI *plast;
    NUSORTPRI *p;

    plast = NULL;
    p = global_dlist_manager.sort_list;
    if (global_dlist_manager.sort_list == NULL) {
        global_dlist_manager.sort_list = pri;
        pri->sys_next = NULL;
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

/* Unlinks a sort priority from the global list (name ours; inlined, as on PS2). */
static __inline void DisplayListRemoveSortPri(NUSORTPRI *pri) {
    NUSORTPRI *plast;
    NUSORTPRI *p;

    plast = NULL;
    p = global_dlist_manager.sort_list;
    while (p != pri) {
        plast = p;
        p = p->sys_next;
    }
    if (plast == NULL) {
        global_dlist_manager.sort_list = p->sys_next;
    } else {
        plast->sys_next = p->sys_next;
    }
    global_dlist_manager.nsort_pris--;
}

/* Releases one use of a material in the dynamic scene. With its last use it leaves its
 * sort priority's chain (and the sort priority the global list, with its last material),
 * gives back its list buffer, and the materials after it close up. Name ours (the PS2
 * prototype lists these locals in a block of their own). `__inline` alone is too big to
 * be inlined, and the code written straight into the loop spills to other stack slots. */
static __forceinline void DisplayListDelDynamicMtl(NUMTL *mtl) {
    NUDISPLAYLISTMANAGER *dlm;
    NUDISPLAYSCENE *dlist;
    int sp;
    int j;
    int sort_pri;
    int idx;
    NUDISPLAYLISTITEM *item;
    NUDISPLAYLISTITEM *prev;
    NUDISPLAYLIST *lost;

    dlm = &global_dlist_manager;
    dlist = &dlm->dyn_mtl_dlist;
    if (mtl->dlist_mtl == NULL) {
        return;
    }
    idx = -1;
    for (j = 0; j < (int)dlist->nmtls; j++) {
        if (dlist->mtls[j] == mtl) {
            idx = j;
            break;
        }
    }
    if (idx == -1) {
        return;
    }
    dlm->material_used[idx]--;
    if (dlm->material_used[idx] > 0) {
        return;
    }
    sort_pri = (mtl->renderplane << 17) + mtl->sort_pri;
    sp = -1;
    for (j = 0; j < dlist->nsort_pris; j++) {
        if (dlist->sort_pris[j].sort_pri == sort_pri) {
            sp = j;
        }
    }
    item = mtl->dlist_mtl->mtl_item;
    if (dlist->sort_pris[sp].items == item) {
        dlist->sort_pris[sp].items = item[next_pos].next;
        item[next_pos].id = 4;
        item[next_pos].next = NULL;
        item[next_pos].type = 0x84;
    } else {
        for (j = 0; j < dlist->sort_pris[sp].nmtls; j++) {
            prev = dlist->mtls[dlist->sort_pris[sp].mtl_first + j]->dlist_mtl->mtl_item;
            if (prev[next_pos].next == item) {
                prev[next_pos].next = item[next_pos].next;
                item[next_pos].id = 4;
                item[next_pos].next = NULL;
                item[next_pos].type = 0x84;
                break;
            }
        }
    }
    for (j = 0; j < (int)dlist->nmtls; j++) {
        if (dlm->mtl_buffers_used[j] && item == (NUDISPLAYLISTITEM *)(dlm->mtlbuff.intaddr + j * 0x80)) {
            dlm->mtl_buffers_used[j] = 0;
            break;
        }
    }
    dlist->sort_pris[sp].nmtls--;
    if (dlist->sort_pris[sp].nmtls == 0) {
        DisplayListRemoveSortPri(&dlist->sort_pris[sp]);
        for (j = sp; j < dlist->nsort_pris - 1; j++) {
            /* Only the first 16 bytes move up: the sort key, the items and the global
             * links (four dword moves in the original). */
            memcpy(&dlist->sort_pris[j], &dlist->sort_pris[j + 1], 16);
        }
        dlist->nsort_pris--;
    }
    lost = dlist->dlist_mtls[idx];
    for (j = idx; j < (int)dlist->nmtls - 1; j++) {
        dlist->mtls[j] = dlist->mtls[j + 1];
        dlist->dlist_mtls[j] = dlist->dlist_mtls[j + 1];
        dlm->material_used[j] = dlm->material_used[j + 1];
    }
    dlist->dlist_mtls[(int)dlist->nmtls - 1] = lost;
    dlm->material_used[(int)dlist->nmtls - 1] = 0;
    for (j = 0; j < (int)dlist->nmtls; j++) {
        if (dlm->material_used[j]) {
            dlist->mtls[j]->dlist_mtl->mtl_id = j;
        }
    }
    DisplayListFindFirstMtls(dlist);
    mtl->dlist_mtl = NULL;
}

/* Adds one use of a material to the dynamic scene. A new material gets a free slot,
 * sorted by render plane and sort priority, a list built in a free buffer, and a place in
 * its sort priority's chain, or a new sort priority. Name ours, inlined like the above. */
static __forceinline void DisplayListAddDynamicMtl(NUMTL *mtl) {
    NUDISPLAYLISTMANAGER *dlm;
    NUDISPLAYSCENE *dlist;
    int sp;
    int j;
    int sort_pri;
    int idx;
    int buffidx;
    int insert;
    NUDISPLAYLIST *last;
    VARIPTR mtlbuff;
    int sort_pri2;
    NUDISPLAYLISTITEM *item;
    NUDISPLAYLISTITEM *prev;
    NUDISPLAYLISTITEM *next;

    dlm = &global_dlist_manager;
    dlist = &dlm->dyn_mtl_dlist;
    idx = -1;
    for (j = 0; j < (int)dlist->nmtls; j++) {
        if (dlist->mtls[j] == mtl) {
            idx = j;
            break;
        }
    }
    if (idx != -1) {
        dlm->material_used[idx]++;
        return;
    }
    for (j = 0; j < (int)dlist->nmtls; j++) {
        if (!dlm->material_used[j]) {
            idx = j;
            break;
        }
    }
    if (idx == -1) {
        return;
    }
    buffidx = -1;
    for (j = 0; j < (int)dlist->nmtls; j++) {
        if (!dlm->mtl_buffers_used[j]) {
            dlm->mtl_buffers_used[j] = 1;
            buffidx = j;
            break;
        }
    }
    sort_pri = (mtl->renderplane << 17) + mtl->sort_pri;
    sp = -1;
    for (j = 0; j < dlist->nsort_pris; j++) {
        if (dlist->sort_pris[j].sort_pri == sort_pri) {
            sp = j;
        }
    }
    insert = -1;
    for (j = 0; j < (int)dlist->nmtls; j++) {
        if (dlm->material_used[j]) {
            sort_pri2 = (dlist->mtls[j]->renderplane << 17) + dlist->mtls[j]->sort_pri;
            if (sort_pri <= sort_pri2) {
                insert = j;
                break;
            }
        }
    }
    if (insert == -1) {
        insert = idx;
    }
    last = dlist->dlist_mtls[(int)dlist->nmtls - 1];
    for (j = (int)dlist->nmtls - 2; j >= insert; j--) {
        dlist->mtls[j + 1] = dlist->mtls[j];
        dlist->dlist_mtls[j + 1] = dlist->dlist_mtls[j];
        dlm->material_used[j + 1] = dlm->material_used[j];
    }
    dlist->dlist_mtls[insert] = last;
    dlm->material_used[insert] = 0;
    for (j = 0; j < (int)dlist->nmtls; j++) {
        if (dlm->material_used[j]) {
            dlist->mtls[j]->dlist_mtl->mtl_id = j;
        }
    }
    dlm->material_used[insert] = 1;
    mtl->dlist_mtl = dlist->dlist_mtls[insert];
    item = mtl->dlist_mtl->mtl_item;
    mtlbuff.intaddr = dlm->mtlbuff.intaddr + buffidx * 0x80;
    item = NuDisplayListCreateMtlDlist(item, mtl, &mtlbuff, dlm->mtlbuffend);
    dlist->mtls[insert] = mtl;
    mtl->dlist_mtl->mtl_item = item;
    mtl->dlist_mtl->dyn_geom = &item[dyn_geom_pos];
    mtl->dlist_mtl->dlist = dlist;
    mtl->dlist_mtl->mtl_id = insert;
    if (sp != -1) {
        if (insert != 0 && (dlist->mtls[insert - 1]->renderplane << 17) + dlist->mtls[insert - 1]->sort_pri == sort_pri) {
            prev = dlist->mtls[insert - 1]->dlist_mtl->mtl_item;
            next = prev[next_pos].next;
            item[next_pos].type = 0x8d;
            item[next_pos].next = next;
            item[next_pos].id = 1;
            prev[next_pos].next = item;
        } else {
            next = dlist->sort_pris[sp].items;
            item[next_pos].type = 0x8d;
            item[next_pos].next = next;
            item[next_pos].id = 1;
            dlist->sort_pris[sp].items = item;
        }
    } else {
        sp = dlist->nsort_pris;
        dlist->sort_pris[sp].sort_pri = sort_pri;
        dlist->sort_pris[sp].items = item;
        dlist->sort_pris[sp].mtl = dlm->temp_mtl;
        dlist->sort_pris[sp].nmtls = 0;
        DisplayListAddSortPri(&dlist->sort_pris[sp]);
        dlist->nsort_pris++;
    }
    dlist->sort_pris[sp].nmtls++;
    DisplayListFindFirstMtls(dlist);
}

/* Brings the dynamic material scene up to date with the materials released and added
 * since the last frame. Effective match: some `sort_pris[sp]` addresses come out as
 * index + base instead of base + index. */
// FUNCTION: BIONICLE 0x553e40
static void DisplayListLinkDynamicMtls(void) {
    int i;

    if (global_dlist_manager.nnew_materials == 0 && global_dlist_manager.ndel_materials == 0) {
        return;
    }
    for (i = 0; i < global_dlist_manager.ndel_materials; i++) {
        DisplayListDelDynamicMtl(global_dlist_manager.del_materials[i]);
    }
    global_dlist_manager.ndel_materials = 0;
    for (i = 0; i < global_dlist_manager.nnew_materials; i++) {
        DisplayListAddDynamicMtl(global_dlist_manager.new_materials[i]);
    }
    global_dlist_manager.nnew_materials = 0;
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

/* Refits a special object's box in its scene's bounds to the object's current draw
 * matrix: the box's centre goes through the matrix and its half-extents through the
 * matrix's absolute rotation. Version 1 bounds keep centre and extents (the centre's w
 * gets the root of the extents' sum), version 0 the corners. 79%: the stack frame differs
 * (docs/matching.md, open questions). */
// FUNCTION: BIONICLE 0x554610
void NuDisplayListUpdateSpecial(NUHSPECIAL *hspec) {
    _VUMTX mtx_a;
    NUSPECIALOBJECT *spec;
    NUDISPLAYSCENE *ds;
    _VUVEC centre;
    _VUVEC extents;
    _VUMTX FabsTransform;
    float result;
    _VUVEC wmin;
    _VUVEC wmax;
    float x;
    float y;

    ds = hspec->scene->display_list;
    spec = hspec->special_obj;
    mtx_a.m = *NuSpecialGetDrawMtx(hspec);
    extents.v.x = spec->max.x - spec->min.x;
    extents.v.y = spec->max.y - spec->min.y;
    extents.v.z = spec->max.z - spec->min.z;
    extents.v.x *= 0.5f;
    extents.v.y *= 0.5f;
    extents.v.z *= 0.5f;
    centre.v.x = spec->min.x + spec->max.x;
    centre.v.y = spec->min.y + spec->max.y;
    centre.v.z = spec->min.z + spec->max.z;
    centre.v.x *= 0.5f;
    centre.v.y *= 0.5f;
    centre.v.z *= 0.5f;
    FabsTransform.m._00 = NuFabs(mtx_a.m._00);
    FabsTransform.m._01 = NuFabs(mtx_a.m._01);
    FabsTransform.m._02 = NuFabs(mtx_a.m._02);
    FabsTransform.m._10 = NuFabs(mtx_a.m._10);
    FabsTransform.m._11 = NuFabs(mtx_a.m._11);
    FabsTransform.m._12 = NuFabs(mtx_a.m._12);
    FabsTransform.m._20 = NuFabs(mtx_a.m._20);
    FabsTransform.m._21 = NuFabs(mtx_a.m._21);
    FabsTransform.m._22 = NuFabs(mtx_a.m._22);
    x = FabsTransform.m._00 * extents.v.x + FabsTransform.m._10 * extents.v.y + FabsTransform.m._20 * extents.v.z;
    y = FabsTransform.m._01 * extents.v.x + FabsTransform.m._11 * extents.v.y + FabsTransform.m._21 * extents.v.z;
    extents.v.z = FabsTransform.m._02 * extents.v.x + FabsTransform.m._12 * extents.v.y + FabsTransform.m._22 * extents.v.z;
    extents.v.x = x;
    extents.v.y = y;
    NuVecMtxTransform(&centre.v, &centre.v, &mtx_a.m);
    result = NuFsqrt(extents.v.x + extents.v.y + extents.v.z);
    centre.f[3] = result;
    if (ds->bounds_version) {
        ds->bounds.boxes[spec->instance_ix * 2] = centre.v4;
        ds->bounds.boxes[spec->instance_ix * 2 + 1] = extents.v4;
    } else {
        wmax.v.x = centre.v.x + extents.v.x;
        wmax.v.y = centre.v.y + extents.v.y;
        wmax.v.z = centre.v.z + extents.v.z;
        wmin.v.x = centre.v.x - extents.v.x;
        wmin.v.y = centre.v.y - extents.v.y;
        wmin.v.z = centre.v.z - extents.v.z;
        ds->bounds.boxes[spec->instance_ix * 2] = wmin.v4;
        ds->bounds.boxes[spec->instance_ix * 2 + 1] = wmax.v4;
    }
    DisplayListUpdateSpecialTransformPS(hspec, &mtx_a);
}

/* Sets the items of clip objects whose visibility changed since the other buffer: two
 * bits per clip object, sixteen to a word. Items of hidden objects (outcode 0) are
 * skipped, the others run (the PS2 inlines a helper per case that sets the item and the
 * tag before it). */
// FUNCTION: BIONICLE 0x554980
static void DisplayListClipUpdate(NUDISPLAYSCENE *ds) {
    unsigned int *buffer[2];
    int c;
    int i;
    int j;
    int k;
    int cidx;
    int outcode;
    unsigned short *nclip_items;
    int *clip_obj_items;
    unsigned char clipbuffer;
    int ntests;
    NUDISPLAYLISTITEM *item;

    buffer[0] = (unsigned int *)ds->clip_used[ds->buffer];
    nclip_items = ds->nclip_items;
    buffer[1] = (unsigned int *)ds->clip_used[!ds->buffer];
    cidx = 0;
    ntests = (ds->nclip_objects + 15) / 16;
    for (i = 0; i < ntests; i++) {
        if (buffer[0][i] == buffer[1][i]) {
            cidx += 16;
            continue;
        }
        for (k = 0; k < 4; k++) {
            clipbuffer = ((unsigned char *)&buffer[0][i])[k];
            for (c = 0; c < 8; c += 2) {
                clip_obj_items = ds->clip_objects[cidx].items;
                outcode = (clipbuffer >> c) & 3;
                if (outcode == 0) {
                    for (j = 0; j < nclip_items[cidx]; j++) {
                        item = &ds->items[clip_obj_items[j]];
                        item->id = 0;
                        (item - 1)->id = 0;
                    }
                } else if (outcode == 1) {
                    for (j = 0; j < nclip_items[cidx]; j++) {
                        item = &ds->items[clip_obj_items[j]];
                        item->id = 3;
                        (item - 1)->id = 3;
                    }
                } else {
                    for (j = 0; j < nclip_items[cidx]; j++) {
                        item = &ds->items[clip_obj_items[j]];
                        item->id = 3;
                        (item - 1)->id = 3;
                    }
                }
                cidx++;
                if (cidx >= ds->nclip_objects) {
                    return;
                }
            }
        }
    }
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

/* Skips (id 1) or runs (id 0) each material's list by its used bit, for the bytes that
 * changed since the other buffer. */
// FUNCTION: BIONICLE 0x555520
static void DisplayListMaterialClipUpdate(NUDISPLAYSCENE *ds) {
    unsigned char *buffer[2];
    unsigned char usedbuffer;
    int i;
    int k;
    int c;
    NUDISPLAYLIST **dlist_mtls;
    int mtl_used;
    int ntests;

    if (ds->mtls[0] == NULL) {
        return;
    }
    dlist_mtls = ds->dlist_mtls;
    buffer[0] = ds->mtl_used[ds->buffer];
    buffer[1] = ds->mtl_used[!ds->buffer];
    ntests = (ds->nmtls + 7) >> 3;
    k = 0;
    for (i = 0; i < ntests; i++) {
        usedbuffer = buffer[0][i];
        if (usedbuffer == buffer[1][i]) {
            k += 8;
            continue;
        }
        for (c = 0; c < 8; c++) {
            mtl_used = (usedbuffer >> c) & 1;
            if (mtl_used) {
                dlist_mtls[k]->mtl_item->id = 0;
            } else {
                dlist_mtls[k]->mtl_item->id = 1;
            }
            k++;
            if (k >= (int)ds->nmtls) {
                return;
            }
        }
    }
}

/* Links each used material list for drawing: the dynamic geometry first, then the
 * scene items of the buffer being drawn, back to the material's end; updates the render
 * states in sort order and drops materials on hidden layers. */
// FUNCTION: BIONICLE 0x5555e0
static void DisplayListFixMaterials(void) {
    NUDISPLAYLISTMANAGER *dlm;
    NUSORTPRI *sp;
    int j;
    NURNDRSTATE state;
    NUGLOBALRNDRSTATE *rolling_state;
    NUDISPLAYSCENE *ds;
    NUDISPLAYLIST *dlist_mtl;
    NUMTL *mtl;

    dlm = &global_dlist_manager;
    rolling_state = NULL;
    RndrStateClear(&state);
    for (sp = dlm->sort_list; sp != NULL; sp = sp->sys_next) {
        ds = sp->display_scene;
        if (ds == NULL) {
            RndrStateUpdateFx(&state, sp->items);
            continue;
        }
        if (!ds->dynamic_used_this_frame && !ds->scene_used_this_frame) {
            continue;
        }
        for (j = 0; j < sp->nmtls; j++) {
            dlist_mtl = ds->dlist_mtls[sp->mtl_first + j];
            if (dlist_mtl->mtl_item->id != 0) {
                continue;
            }
            mtl = ds->mtls[sp->mtl_first + j];
            if (ds->local_state != NULL && ds->scene_used_this_frame) {
                if (dlist_mtl->mtl_last != dlist_mtl->first) {
                    DisplayListUpdateRenderState(dlist_mtl, ds->local_state);
                    rolling_state = ds->local_state;
                } else if (rolling_state != ds->local_state) {
                    DisplayListUpdateRenderState(dlist_mtl, ds->local_state);
                    rolling_state = ds->local_state;
                }
            } else {
                rolling_state = NULL;
            }
            if (dlist_mtl->mtl_last != dlist_mtl->first) {
                *dlist_mtl->dyn_geom = *dlist_mtl->first;
                if (!ds->scene_used_this_frame) {
                    dlist_mtl->mtl_last->next = dlist_mtl->mtl_item->next;
                    dlist_mtl->mtl_last = dlist_mtl->first;
                } else {
                    if (dlist_mtl->scene_first[dlist_mtl->scene_buffer] != dlist_mtl->scene_next) {
                        dlist_mtl->mtl_last->next = dlist_mtl->scene_first[dlist_mtl->scene_buffer];
                        dlist_mtl->scene_next->type = 0x8d;
                        dlist_mtl->scene_next->id = 1;
                        dlist_mtl->scene_next->next = dlist_mtl->dyn_geom + 1;
                    }
                    dlist_mtl->mtl_last = dlist_mtl->first;
                }
            } else if (dlist_mtl->scene_first[dlist_mtl->scene_buffer] != dlist_mtl->scene_next) {
                dlist_mtl->dyn_geom->next = dlist_mtl->scene_first[dlist_mtl->scene_buffer];
                dlist_mtl->scene_next->type = 0x8d;
                dlist_mtl->scene_next->id = 1;
                dlist_mtl->scene_next->next = dlist_mtl->dyn_geom + 1;
            } else {
                dlist_mtl->dyn_geom->next = dlist_mtl->dyn_geom + 1;
            }
            RndrStateUpdate(&state, mtl, dlist_mtl->mtl_item);
            if (ds->mtl_layer_mask && (ds->mtl_layer_mask & mtl->attrib.mtl_layers)) {
                dlist_mtl->mtl_item->id = 1;
                ds->mtl_used[ds->buffer][dlist_mtl->mtl_id / 8] &= ~(1 << (dlist_mtl->mtl_id % 8));
            }
        }
    }
    for (j = 0; j < dlm->ndisplay_lists; j++) {
        if (dlm->dlists[j]->local_state != NULL) {
            RndrStateResetGlobalState(dlm->dlists[j]->local_state);
        }
    }
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

/* The dynamic material scene: 64 material lists for geometry added at run time, then the
 * manager's material bookkeeping, its material buffer and the 2D list. */
// FUNCTION: BIONICLE 0x555880
void DisplayListCreateDynMtlList(VARIPTR *buff, VARIPTR buffend) {
    NUDISPLAYSCENE *dlist;
    NUDISPLAYLIST *rs;
    int nmaterials;
    int i;
    // GLOBAL: BIONICLE 0x616ec0
    // STRING: BIONICLE 0x5e02ec
    static char *name = "Dynamic Material Display Scene";
    NUDISPLAYLISTITEM *item;

    dlist = &global_dlist_manager.dyn_mtl_dlist;
    nmaterials = 64;
    NuDisplayListCreate(dlist, buff, buffend, nmaterials * items_per_mtl, nmaterials, 0, 0, 64, 0, 0);
    dlist->nsort_pris = 0;
    dlist->name = name;
    for (i = 0; i < nmaterials; i++) {
        dlist->dlist_mtls[i]->mtl_item = &dlist->items[i * items_per_mtl];
        dlist->dlist_mtls[i]->dyn_geom = &dlist->items[i * items_per_mtl + dyn_geom_pos];
        dlist->dlist_mtls[i]->dlist = dlist;
        dlist->dlist_mtls[i]->mtl_id = i;
    }
    buff->intaddr = (buff->intaddr + 15) & ~15;
    global_dlist_manager.nnew_materials = 0;
    global_dlist_manager.new_materials = (NUMTL **)buff->voidptr;
    buff->intaddr += 0x100;
    global_dlist_manager.ndel_materials = 0;
    global_dlist_manager.del_materials = (NUMTL **)buff->voidptr;
    buff->intaddr += 0x100;
    global_dlist_manager.material_used = buff->u8;
    buff->intaddr += 0x40;
    global_dlist_manager.mtl_buffers_used = buff->u8;
    buff->intaddr += 0x40;
    memset(global_dlist_manager.material_used, 0, 0x40);
    memset(global_dlist_manager.mtl_buffers_used, 0, 0x40);
    global_dlist_manager.mtlbuff.intaddr = (buff->intaddr + 15) & ~15;
    buff->intaddr = ((buff->intaddr + 15) & ~15) + 0x2000;
    global_dlist_manager.mtlbuffend = *buff;
    rs = &global_dlist_manager.dlist_2d;
    item = rs->first;
    item->next = NULL;
    item->type = 0x8d;
    item->id = 1;
    rs->mtl_last = rs->first;
    rs->state = (NURNDRSTATE *)((buff->intaddr + 3) & ~3);
    buff->intaddr = ((buff->intaddr + 3) & ~3) + sizeof(NURNDRSTATE);
    RndrStateClear(rs->state);
    rs->mtl_last = rs->first;
    dlist->scene_supports_instancing = 0;
}

/* Collects this frame's sort priorities into a new render scene with the 2D list, and
 * returns its index (-1 if there is nothing to draw). Scenes that support instancing get
 * copies of their dynamic sort priorities, so the lists can be reused. */
// FUNCTION: BIONICLE 0x555bc0
int NuDisplayListAddRenderScene(void) {
    NUDISPLAYLISTMANAGER *dlm;
    NUSORTPRI *sp;
    int nsort_pris;
    int i;
    int j;
    NUDISPLAYLIST *rs2d;
    NUSORTPRI *sp2;
    NUDISPLAYLIST *dlist_mtl;
    NUDISPLAYLISTITEM *item;
    VARIPTR *buff;
    NUSORTPRI **sort_pris;
    NUDISPLAYSCENE *ds;

    dlm = &global_dlist_manager;
    rs2d = &dlm->dlist_2d;
    display_list_buffer->intaddr = (display_list_buffer->intaddr + 15) & ~15;
    buff = display_list_buffer;
    sort_pris = (NUSORTPRI **)buff->voidptr;
    buff->voidptr = &sort_pris[dlm->nsort_pris + 1];
    nsort_pris = 0;
    for (sp = dlm->sort_list; sp != NULL; sp = sp->sys_next) {
        ds = sp->display_scene;
        if (ds != NULL && ds->scene_supports_instancing) {
            if (!ds->dynamic_used_this_frame) {
                continue;
            }
            sp2 = (NUSORTPRI *)((buff->intaddr + 3) & ~3);
            buff->intaddr = ((buff->intaddr + 3) & ~3) + sizeof(NUSORTPRI);
            sp2->sort_pri = sp->sort_pri;
            sp2->items = buff->item;
            sp2->display_scene = sp->display_scene;
            sp2->nmtls = sp->nmtls;
            sp2->mtl_first = sp->mtl_first;
            j = 0;
            for (i = 0; i < sp->nmtls; i++) {
                dlist_mtl = sp->display_scene->dlist_mtls[sp->mtl_first + i];
                if (dlist_mtl->mtl_last == dlist_mtl->first) {
                    continue;
                }
                item = dlist_mtl->mtl_item + 1;
                if (item->type == 0x80) {
                    sp2->items[j] = *item;
                } else {
                    sp2->items[j] = dlist_mtl->mtl_item[3];
                }
                j++;
                sp2->items[j] = *dlist_mtl->first;
                j++;
                dlist_mtl->mtl_last->next = &sp2->items[j];
                dlist_mtl->mtl_last = dlist_mtl->first;
                RndrStateClear(dlist_mtl->state);
            }
            sp2->items[j].id = 4;
            sp2->items[j].next = NULL;
            sp2->items[j].type = 0x84;
            buff->intaddr += (j + 1) * sizeof(NUDISPLAYLISTITEM);
            sort_pris[nsort_pris++] = sp2;
            sp2->rendered_this_frame = 1;
        } else if (!sp->rendered_this_frame &&
                   (ds == NULL || ds->dynamic_used_this_frame || ds->scene_used_this_frame)) {
            sort_pris[nsort_pris++] = sp;
            sp->rendered_this_frame = 1;
        }
    }
    if (nsort_pris == 0 && rs2d->mtl_last == rs2d->first) {
        return -1;
    }
    dlm->render_scenes[dlm->nrender_scenes] = (NUDISPLAYLISTRENDERSCENE *)((buff->intaddr + 3) & ~3);
    buff->intaddr = ((buff->intaddr + 3) & ~3) + sizeof(NUDISPLAYLISTRENDERSCENE);
    dlm->render_scenes[dlm->nrender_scenes]->nsort_pris = nsort_pris;
    dlm->render_scenes[dlm->nrender_scenes]->sort_pris = sort_pris;
    item = rs2d->mtl_last;
    if (item != rs2d->first) {
        item->type = 0x84;
        item->next = NULL;
        item->id = 4;
        dlm->render_scenes[dlm->nrender_scenes]->render_2d_first = *rs2d->first;
        RndrStateClear(rs2d->state);
        rs2d->mtl_last = rs2d->first;
        dlm->nrender_scenes++;
        return dlm->nrender_scenes - 1;
    }
    item = &dlm->render_scenes[dlm->nrender_scenes]->render_2d_first;
    item->type = 0x84;
    item->next = NULL;
    item->id = 4;
    dlm->nrender_scenes++;
    return dlm->nrender_scenes - 1;
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

/* Finishes the frame's lists: links the dynamic materials, updates the clip and
 * material states of the scenes used this frame, and fixes up the material chains. */
// FUNCTION: BIONICLE 0x556030
void NuDisplayListSwapBuffersEndFrame(void) {
    int j;
    NUDISPLAYLISTMANAGER *dlm;
    NUDISPLAYSCENE *ds;

    dlm = &global_dlist_manager;
    // STRING: BIONICLE 0x5e061c
    _NuTimeBarSlotBegin(-1, 2, "DLFIXUP");
    NuThreadCriticalSectionBegin(dlm->loading_critical_section);
    DisplayListLinkDynamicMtls();
    DisplayListSwapBuffersPS();
    DisplayListMaterialClipUpdate(&dlm->dyn_mtl_dlist);
    for (j = 0; j < dlm->ndisplay_lists; j++) {
        ds = dlm->dlists[j];
        if (ds->scene_used_this_frame) {
            DisplayListClipUpdate(ds);
        }
        if (ds->dynamic_used_this_frame || ds->scene_used_this_frame) {
            DisplayListMaterialClipUpdate(ds);
        }
    }
    DisplayListFixMaterials();
    NuThreadCriticalSectionEnd(dlm->loading_critical_section);
    _NuTimeBarSlotEnd(-1, 2);
}
