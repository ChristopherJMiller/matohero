#include "nurndr.h"

#include "nurand.h"
#include "nutrig.h"

/* Generic renderer helpers (PS2: nu3d/nurndr_gen.c): the post-effect parameters, the
 * render-pass flags and the wind shear of specials so far; in PC order. The PC has no
 * reflection pass setters (nothing writes global_GobjIsReflectedGeometry). */

// GLOBAL: BIONICLE 0x348f744
int NuRndr_ShadowScene;

// GLOBAL: BIONICLE 0x348f774
int global_GobjIsReflectedGeometry;

// GLOBAL: BIONICLE 0x348f778
int global_GobjIsShadowReceive;

// GLOBAL: BIONICLE 0x35bba80
NUCAMERAMOTIONBLURPARAMETERS NuRndr_cameraMotionBlur;

// GLOBAL: BIONICLE 0x35bbb50
NUDOFPARAMETERS NuRndr_depthOfField;

// GLOBAL: BIONICLE 0x616d68
float global_windscale = 1.0f;

// GLOBAL: BIONICLE 0x616d6c
float global_windspeed = 1.0f;

// GLOBAL: BIONICLE 0x348f76c
unsigned int global_frame_count;

/* Sine and cosine of an angle in radians through NuTrigTable (65536 units a turn). */
#define WIND_SIN(x) NuTrigTable[((int)((x) * 10430.378f) >> 1) & 0x7fff]
#define WIND_COS(x) NuTrigTable[((0x4000 - (int)((x) * -10430.378f)) >> 1) & 0x7fff]

// FUNCTION: BIONICLE 0x54fc00
int NuRndIsReflectionGobj(void) {
    return global_GobjIsReflectedGeometry;
}

// FUNCTION: BIONICLE 0x54fc10
void NuRndrStartShadowReceiveRender(void) {
    global_GobjIsShadowReceive = 1;
}

// FUNCTION: BIONICLE 0x54fc20
void NuRndrEndShadowReceiveRender(void) {
    global_GobjIsShadowReceive = 0;
}

// FUNCTION: BIONICLE 0x54fc30
void NuDepthOfFieldEffect(float aperture, float focalLength, float planeInFocus) {
    NuRndr_depthOfField.hasDepthOfField = 1;
    NuRndr_depthOfField.aperture = aperture;
    NuRndr_depthOfField.focalLength = focalLength;
    NuRndr_depthOfField.focalPlane = planeInFocus;
}

// FUNCTION: BIONICLE 0x54fc60
void NuCameraMotionBlurEffect(NUMTX *viewProj, NUMTX *preViewProj, float timeElapsed, float shutterSpeed, float threshold) {
    NuRndr_cameraMotionBlur.applyMotion = 1;
    NuRndr_cameraMotionBlur.viewProj = *viewProj;
    NuRndr_cameraMotionBlur.preViewProj = *preViewProj;
    NuRndr_cameraMotionBlur.timeElapsed = timeElapsed;
    NuRndr_cameraMotionBlur.shutterSpeed = shutterSpeed;
    NuRndr_cameraMotionBlur.threshold = threshold;
}

// FUNCTION: BIONICLE 0x54fcb0
void NuDynamicLightingRenderShadows(void) {
    NuRndr_ShadowScene = 1;
}

/* The wind's sideways push on an object: a sum of three waves per axis over the frame
 * count, phase-shifted by a random offset seeded with the object's wind speed. */
// FUNCTION: BIONICLE 0x54fcc0
static void WindDirection(float *sx, float *sz, int wind_scale, int wind_speed) {
    unsigned int frame;
    float theta;
    float t1;
    float t2;
    float s1;
    float s2;
    float windscale;
    float windspeed;
    float offset;
    unsigned int seed;

    windscale = wind_scale * global_windscale * (1.0f / 65535.0f);
    windspeed = wind_speed * global_windspeed * (1.0f / 65535.0f);
    seed = wind_speed;
    offset = NuRandFloatSeeded(&seed) * 6.284f;
    frame = global_frame_count;
    theta = frame * windspeed + offset;
    t1 = WIND_COS(2.3f * theta) * 0.5f;
    t2 = WIND_COS(4.4f * theta) * 0.25f;
    s1 = WIND_SIN(theta * 2.1f) * 0.5f;
    s2 = WIND_SIN(4.2f * theta) * 0.25f;
    *sx = (WIND_SIN(theta) + s1 + s2) * windscale;
    *sz = (WIND_COS(theta * 1.1f) + t1 + t2) * windscale;
}

/* A special's draw matrix bent by the wind: its y axis leans along the x and z axes. */
// FUNCTION: BIONICLE 0x54fe40
void WindShear(NUMTX *windmtx, NUMTX *instmtx, int wind_scale, int wind_speed) {
    float sx;
    float sz;

    WindDirection(&sx, &sz, wind_scale, wind_speed);
    *windmtx = *instmtx;
    windmtx->_10 = instmtx->_10 + sx * instmtx->_00 + sz * instmtx->_20;
    windmtx->_11 = instmtx->_11 + sx * instmtx->_01 + sz * instmtx->_21;
    windmtx->_12 = instmtx->_12 + sx * instmtx->_02 + sz * instmtx->_22;
    windmtx->_13 = instmtx->_13 + sx * instmtx->_03 + sz * instmtx->_23;
}
