#ifndef NUGSCN_H
#define NUGSCN_H

#include "numath.h"
#include "nutypes.h"

/* Scene types (PS2 prototype layouts). Only what the decompiled code uses so far; the
 * scene itself (NUGSCN, 0x1e8 bytes) is still opaque. */

typedef struct numtl_s NUMTL;
typedef struct nugscn_s NUGSCN;

/* Clip object of a display list special: materials and items it covers. */
typedef struct nuclipobject_s {
    unsigned int nclip_items;
    unsigned int *mtls;
    int *items;
} NUCLIPOBJECT;

/* Geometry of a graphics object (only the material chain so far). */
typedef struct nugeom_s {
    struct nugeom_s *next;
    NUMTL *mtl;
} NUGEOM;

/* A graphics object; next_gobj chains the levels of detail. */
typedef struct nugobj_s {
    struct nugobj_s *sysnext;
    struct nugobj_s *syslast;
    int type;
    NUGEOM *geom;
    void *faceon_geom;
    float r;
    float r2;
    NUVEC min;
    NUVEC max;
    NUVEC cntr;
    float cntr_r;
    float cntr_r2;
    unsigned int flags;
    struct nugobj_s *next_gobj;
    char exdata[12];
    void *colourref;
    int colourref_size;
    void *focolourref;
    int focolourref_size;
} NUGOBJ;

typedef struct nuinstanim_s {
    NUMTX mtx;
    float tfactor;
    float tfirst;
    float tinterval;
    float ltime;
    unsigned int playing : 1;
    unsigned int backwards : 1;
    unsigned int waiting : 1;
    unsigned int repeating : 1;
    unsigned int oscillate : 1;
    unsigned int unused : 27;
    float fparam1;
    float prev_eval_time;
    unsigned short anim_ix;
    unsigned short instance_ix;
} NUINSTANIM;

typedef struct nuinstflags_s {
    unsigned int visible : 1;
    unsigned int onscreen : 1;
    unsigned int visitest : 1;
    unsigned int novisitest : 1;
    unsigned int nolospan : 1;
    unsigned int isspecial : 1;
    unsigned int polycliptest : 1;
    unsigned int occlusion : 3;
    unsigned int receives_shadows : 1;
    unsigned int floor_object : 1;
    unsigned int does_not_move : 1;
    unsigned int pad : 19;
} NUINSTFLAGS;

typedef struct nuinstance_s {
    NUMTX mtx;
    short objid;
    short drawdist;
    NUINSTFLAGS flags;
    NUINSTANIM *anim;
    short windscale;
    short windspeed;
} NUINSTANCE;

typedef struct nuspecial_s {
    NUMTX mtx;
    NUINSTANCE *instance;
    char *name;
    unsigned int flags;
} NUSPECIAL;

/* Specials outside the instance list (anim is (NUINSTANIM *)-1 when unset). */
typedef struct nuspecialobject_s {
    NUMTX mtx;
    NUMTX draw_mtx;
    NUVEC4 min;
    NUVEC4 max;
    NUVEC4 sphere;
    NUCLIPOBJECT *clip_objects;
    char *name;
    unsigned int flags;
    float *clip_range;
    int instance_ix;
    NUINSTANIM *anim;
    short wind_speed;
    short wind_scale;
    void *RigidBody;
} NUSPECIALOBJECT;

/* Display list of a scene (PS2 layout). */
typedef struct nudisplayscene_s {
    char *name;
    int nitems;
    void *items;
    int CRC;
    int nclip_objects;
    NUCLIPOBJECT *clip_objects;
    unsigned short *nclip_items;
    unsigned short *cliptags;
    int *tagoffset;
    unsigned int *mtl_clip;
    float *clip_range;
    float *farclip;
    unsigned char *clip_used[2];
    char bounds[16];
    char *visibility_flags;
    unsigned int nmtls;
    NUMTL **mtls;
    void **dlist_mtls;
    unsigned char *mtl_used[2];
    int nsort_pris;
    void *sort_pris;
    unsigned int nstaticinstances;
    int nspecials;
    NUSPECIALOBJECT *specials;
    unsigned int used_this_frame : 1;
    unsigned int dynamic_used_this_frame : 1;
    unsigned int scene_used_this_frame : 1;
    unsigned int sorted_this_frame : 1;
    unsigned int scene_supports_instancing : 1;
    unsigned int mtl_layer_mask : 8;
    unsigned int is_a_clone : 1;
    unsigned int bounds_version : 1;
    unsigned int buffer : 1;
    unsigned int scene_specials : 1;
    unsigned int pad : 15;
    NUGSCN *gscene;
    void *ps;
    void *local_state;
    unsigned int NumInstances;
    unsigned char *SpecialFlags;
    void *mtl_anims;
} NUDISPLAYSCENE;

/* A named path of points. */
typedef struct nugspline_s {
    short len;
    short ptsize;
    char *name;
    char *pts;
} NUGSPLINE;

/* A loaded scene. The PC layout differs from the PS2's (display_list is at 0x1b4 on PC,
 * 0x1ac on PS2); only the fields used so far are placed. */
struct nugscn_s {
    int *tids;
    int numtid;
    NUMTL **mtls;
    int nummtl;
    int numgobj;
    NUGOBJ **gobjs;
    int numinstance;
    NUINSTANCE *instances;
    int numspecial;
    NUSPECIAL *specials;
    int numsplines; /* PS2 has splinedata before these */
    NUGSPLINE *splines;
    char unknown_030[0x1b4 - 0x30];
    NUDISPLAYSCENE *display_list;
};

/* Handle to a special: one of special/special_obj is set. */
typedef struct nuhspecial_s {
    NUGSCN *scene;
    NUSPECIAL *special;
    NUSPECIALOBJECT *special_obj;
} NUHSPECIAL;

#endif
