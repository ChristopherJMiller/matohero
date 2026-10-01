#include "nucamera.h"
#include "nudlist.h"
#include "nurndr.h"

/* The render state the display lists draw with (PS2: nu3d/ps2/nurndrstat.c, plus the
 * light setters of nu3d/ps2/nustream.c). Each setter stores its part, drops the part's
 * cached state and bumps the part's id and the global id; the PC builds no VU data here.
 * File name ours; in PC order. */

// GLOBAL: BIONICLE 0x35e92c0
NUGLOBALRNDRSTATE render_state;

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
