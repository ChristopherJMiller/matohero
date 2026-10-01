#ifndef NUGSCN_H
#define NUGSCN_H

#include "numath.h"
#include "nutypes.h"

/* Scene types (PS2 prototype layouts). Only what the decompiled code uses so far; the
 * scene itself (NUGSCN, 0x1e8 bytes) is still opaque. */

typedef struct numtl_s NUMTL;
typedef struct nugscn_s NUGSCN;
typedef struct nuclipobject_s NUCLIPOBJECT;

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

/* Handle to a special: one of special/special_obj is set. */
typedef struct nuhspecial_s {
    NUGSCN *scene;
    NUSPECIAL *special;
    NUSPECIALOBJECT *special_obj;
} NUHSPECIAL;

#endif
