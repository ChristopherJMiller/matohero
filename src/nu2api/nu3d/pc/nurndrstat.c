#include "nucamera.h"
#include "nudlist.h"
#include "nurndr.h"
#include "numtl.h"
#include "numtx.h"

/* The render state the display lists draw with (PS2: nu3d/ps2/nurndrstat.c, plus the
 * light setters of nu3d/ps2/nustream.c). Each setter stores its part, drops the part's
 * cached state and bumps the part's id and the global id; the PC builds no VU data here.
 * File name ours; in PC order. */

/* The light state a list's light item points at (PC layout, 0xac bytes; name ours). */
typedef struct rndrlightstate_s {
    NUCOLOUR3 ambient;
    NUCOLOUR3 colour[3];
    NUVEC4 direction[3];
    NUVEC specular;
    NUMTX specular_mtx;
} RNDRLIGHTSTATE;

// GLOBAL: BIONICLE 0x35e9280
static NUMTX specular_mtx_scaled;

// GLOBAL: BIONICLE 0x35e92c0
NUGLOBALRNDRSTATE render_state;

// GLOBAL: BIONICLE 0x35e9460
static NUMTX specular_mtx;

/* The specular power specular_mtx_scaled was made for. Not static: as a static, MSVC
 * knows NuRndrSetSpecularMtx's copy from *m can't read it and moves the store later. */
// GLOBAL: BIONICLE 0x616c40
float specular_power = 1.0f;

// FUNCTION: BIONICLE 0x5447c0
__inline void RndrStateClear(NURNDRSTATE *state) {
    state->mtl = NULL;
    state->tid = -1;
    state->global_id = -1;
    state->lights_id = -1;
    state->camera_id = -1;
    state->fog_id = -1;
    state->konst_id = -1;
}

/* Sets the ambient light, at half intensity as on PS2. */
// FUNCTION: BIONICLE 0x5447f0
int NuRndrSetAmbientLight(NUVEC *c0) {
    render_state.ambient_intensity.r = c0->x * 0.5f;
    render_state.ambient_intensity.g = c0->y * 0.5f;
    render_state.ambient_intensity.b = c0->z * 0.5f;
    render_state.light_state = NULL;
    render_state.state.global_id++;
    render_state.state.lights_id++;
    return 1;
}

/* Sets the three directional lights, their colours at half intensity. */
// FUNCTION: BIONICLE 0x544850
int NuRndrSetDirectionalLights(NUVEC *d0, NUCOLOUR3 *c0, NUVEC *d1, NUCOLOUR3 *c1, NUVEC *d2, NUCOLOUR3 *c2) {
    render_state.light_direction[0] = *d0;
    render_state.light_direction[1] = *d1;
    render_state.light_direction[2] = *d2;
    render_state.light_intensity[0].r = c0->r * 0.5f;
    render_state.light_intensity[0].g = c0->g * 0.5f;
    render_state.light_intensity[0].b = c0->b * 0.5f;
    render_state.light_intensity[1].r = c1->r * 0.5f;
    render_state.light_intensity[1].g = c1->g * 0.5f;
    render_state.light_intensity[1].b = c1->b * 0.5f;
    render_state.light_intensity[2].r = c2->r * 0.5f;
    render_state.light_intensity[2].g = c2->g * 0.5f;
    render_state.light_intensity[2].b = c2->b * 0.5f;
    render_state.light_state = NULL;
    render_state.state.global_id++;
    render_state.state.lights_id++;
    return 1;
}

/* Copies the camera's view matrix and the projection terms the renderer uses. */
// FUNCTION: BIONICLE 0x544960
int NuRndrStateUpdateCameraState(void) {
    NUMTX *proj;

    proj = NuCameraGetProjectionMtx();
    render_state.view = *NuCameraGetViewMtx();
    render_state.proj_00 = proj->_00;
    render_state.proj_11 = proj->_11;
    render_state.proj_22 = proj->_22;
    render_state.proj_23 = proj->_23;
    render_state.proj_32 = proj->_32;
    render_state.camera_state = NULL;
    render_state.state.global_id++;
    render_state.state.camera_id++;
    NuRndrUpdateCameraPlane();
    return 1;
}

/* Fog on or off (an empty hook on PS2). */
// FUNCTION: BIONICLE 0x5449e0
void NuRndrFogMode(int enable) {
    render_state.fog_enabled = enable;
    render_state.fog_state = NULL;
    render_state.state.global_id++;
    render_state.state.fog_id++;
}

/* Fog distances, colour and density (PC only; the PS2 has NuRndrFogDistance and
 * NuRndrFogColour, both empty; name ours). */
// FUNCTION: BIONICLE 0x544a10
void NuRndrFogSet(float fognear, float fogfar, unsigned int colour, float density) {
    render_state.fog_near = fognear;
    render_state.fog_far = fogfar;
    render_state.fog_rgba = colour;
    render_state.fog_density = density;
    render_state.fog_state = NULL;
    render_state.state.global_id++;
    render_state.state.fog_id++;
}

/* Writes a state's lights to the display list buffer: per light its colour, the
 * specular terms and its direction. */
// FUNCTION: BIONICLE 0x544a60
void *RndrStateBuildLightState(NUGLOBALRNDRSTATE *local_state) {
    VARIPTR *buff;
    RNDRLIGHTSTATE *ls;
    int i;

    display_list_buffer->intaddr = (display_list_buffer->intaddr + 15) & ~15;
    buff = display_list_buffer;
    ls = buff->voidptr;
    ls->ambient = local_state->ambient_intensity;
    for (i = 0; i < 3; i++) {
        ls->colour[i] = local_state->light_intensity[i];
        ls->specular = local_state->specular_intensity;
        ls->specular_mtx = local_state->specular_mtx;
        ls->direction[i].x = local_state->light_direction[i].x;
        ls->direction[i].y = local_state->light_direction[i].y;
        ls->direction[i].z = local_state->light_direction[i].z;
        ls->direction[i].w = 0.0f;
    }
    buff->intaddr += sizeof(RNDRLIGHTSTATE);
    return ls;
}

/* Writes the constant alpha (1 when disabled) to the display list buffer. */
// FUNCTION: BIONICLE 0x544bb0
void *RndrStateBuildKonstState(NUGLOBALRNDRSTATE *local_state) {
    VARIPTR *buff;
    float *konst;

    display_list_buffer->intaddr = (display_list_buffer->intaddr + 15) & ~15;
    buff = display_list_buffer;
    konst = buff->f32;
    if (local_state->const_alpha_enabled) {
        *konst = local_state->const_alpha;
    } else {
        *konst = 1.0f;
    }
    buff->intaddr += sizeof(float);
    return konst;
}

/* Drops a state's cached parts and clears the shared state's ids (the PS2 also clears
 * all of render_state first). */
// FUNCTION: BIONICLE 0x544bf0
__inline void RndrStateResetGlobalState(NUGLOBALRNDRSTATE *state) {
    state->const_alpha_enabled = 0;
    state->const_tint_enabled = 0;
    state->fog_state = NULL;
    state->light_state = NULL;
    state->camera_state = NULL;
    state->konst_state = NULL;
    RndrStateClear(&render_state.state);
}

// FUNCTION: BIONICLE 0x544c50
void RndrStateResetSharedGlobalState(void) {
    RndrStateResetGlobalState(&render_state);
}

/* Name ours (PC only). */
// FUNCTION: BIONICLE 0x544ca0
NUGLOBALRNDRSTATE *RndrStateGetSharedGlobalState(void) {
    return &render_state;
}

/* Writes the view matrix and a projection matrix with the state's terms to the display
 * list buffer (inlined on PC). */
static __forceinline void *RndrStateBuildCameraState(NUGLOBALRNDRSTATE *local_state) {
    NUMTX proj = {0};
    VARIPTR *buff;

    proj._00 = local_state->proj_00;
    proj._11 = local_state->proj_11;
    proj._22 = local_state->proj_22;
    proj._23 = local_state->proj_23;
    proj._32 = local_state->proj_32;
    display_list_buffer->intaddr = (display_list_buffer->intaddr + 15) & ~15;
    buff = display_list_buffer;
    *buff->mtx44 = local_state->view;
    buff->intaddr += sizeof(NUMTX);
    *buff->mtx44 = proj;
    buff->intaddr += sizeof(NUMTX);
    return (void *)(buff->intaddr - 2 * sizeof(NUMTX));
}

/* The PC has no fog state to build (inlined). */
static __inline void *RndrStateBuildFogState(NUGLOBALRNDRSTATE *local_state) {
    return NULL;
}

/* Brings a material list's render state up to date with local_state: each part whose id
 * changed gets an item calling its (cached or newly built) state. */
// FUNCTION: BIONICLE 0x544cb0
void DisplayListUpdateRenderState(NUDISPLAYLIST *dlist, NUGLOBALRNDRSTATE *local_state) {
    if (local_state == NULL || dlist->state->global_id == local_state->state.global_id) {
        return;
    }
    if (dlist->state->lights_id != local_state->state.lights_id) {
        if (local_state->light_state == NULL) {
            local_state->light_state = RndrStateBuildLightState(local_state);
        }
        NuDisplayListLinkItem(dlist, 0x94, local_state->light_state);
        dlist->state->lights_id = local_state->state.lights_id;
    }
    if (dlist->state->camera_id != local_state->state.camera_id) {
        if (local_state->camera_state == NULL) {
            local_state->camera_state = RndrStateBuildCameraState(local_state);
        }
        NuDisplayListLinkItem(dlist, 0x9a, local_state->camera_state);
        dlist->state->camera_id = local_state->state.camera_id;
    }
    if (dlist->state->fog_id != local_state->state.fog_id) {
        if (local_state->fog_state == NULL) {
            local_state->fog_state = RndrStateBuildFogState(local_state);
        }
        NuDisplayListLinkItem(dlist, 0xa6, local_state->fog_state);
        dlist->state->fog_id = local_state->state.fog_id;
    }
    if (dlist->state->konst_id != local_state->state.konst_id) {
        if (local_state->konst_state == NULL) {
            local_state->konst_state = RndrStateBuildKonstState(local_state);
        }
        NuDisplayListLinkItem(dlist, 0xa5, local_state->konst_state);
        dlist->state->konst_id = local_state->state.konst_id;
    }
    dlist->state->global_id = local_state->state.global_id;
}

/* Sets the constant alpha a material is drawn with. The PC ignores the tint, and sets
 * the alpha even when both it and the current one are disabled. */
// FUNCTION: BIONICLE 0x544e80
void RndrStateSetConstAlphaTint(unsigned int const_alpha_enabled, unsigned int const_tint_enabled, float const_alpha, NUCOLOUR3 *const_tint, NUMTL *mtl) {
    render_state.const_alpha_mtl = mtl;
    render_state.const_alpha_enabled = const_alpha_enabled;
    render_state.const_alpha = const_alpha;
    render_state.konst_state = NULL;
    render_state.state.global_id++;
    render_state.state.konst_id++;
}

/* The specular matrix for a material (its second pass's, if it has one): for the
 * specular effects (fxid 0x81, 0x85) scaled by the material's specular power, cached
 * while the power doesn't change. PC only; name ours. */
// FUNCTION: BIONICLE 0x544ec0
void NuRndrGetSpecularMtx(NUMTX *out, NUMTL *mtl) {
    float power;
    NUVEC s;

    if (mtl->next != NULL) {
        mtl = mtl->next;
    }
    if (mtl->fxid == 0x81 || mtl->fxid == 0x85) {
        if (mtl->power != 0.0f) {
            power = mtl->power;
        } else {
            power = 1.0f;
        }
        if (power != specular_power) {
            specular_power = power;
            specular_mtx_scaled = specular_mtx;
            if (power != 1.0f) {
                s.x = s.y = s.z = power;
                NuMtxPreScale(&specular_mtx_scaled, &s);
            }
        }
        *out = specular_mtx_scaled;
    } else {
        *out = specular_mtx;
    }
}

/* Stores the specular matrix, its rotation transposed. PC only; name ours. */
// FUNCTION: BIONICLE 0x544f90
void NuRndrSetSpecularMtx(NUMTX *m) {
    float tmp;

    specular_power = 1.0f;
    specular_mtx = *m;
    tmp = specular_mtx._01;
    specular_mtx._01 = specular_mtx._10;
    specular_mtx._10 = tmp;
    tmp = specular_mtx._02;
    specular_mtx._02 = specular_mtx._20;
    specular_mtx._20 = tmp;
    tmp = specular_mtx._21;
    specular_mtx._21 = specular_mtx._12;
    specular_mtx._12 = tmp;
    specular_mtx_scaled = specular_mtx;
}
