#include "nupad.h"

#include <string.h>

#include "folded.h"
#include "numem.h"

/* Pads: the PC layer scans physical controllers into g_nupadScannedPads, game pads are
 * mapped onto them, and NuPadRead copies a mapped controller into the game's NUPAD.
 * Based on opensagadev/saga src/nu2api/nucore/nupad.cpp (GPL-3.0, a later engine with a
 * different NUPAD); names and locals from the PS2 prototype. PC differences: 16 scanned
 * pads, d-pad ramps in NuPadRead, NuPadGetFirstActivePad reads the mapping, and no
 * NuPadInit/Close/SetMotors/ResetState here. */

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// STRING: BIONICLE 0x5e0928
// __FILE__, passed to NuMemAlloc: ".\\nupad.c"

// GLOBAL: BIONICLE 0x617494
static float ramp_rate = 0.1f;

// GLOBAL: BIONICLE 0x617498
static float ramp_vals[5][2] = {{0.0f, 0.0f}, {0.5f, 0.1f}, {0.75f, 0.25f}, {0.85f, 0.5f}, {1.0f, 1.0f}};

// GLOBAL: BIONICLE 0x6174c0
int g_profilePlayerPad = -1;

// GLOBAL: BIONICLE 0x34c5fb8
static int g_atLeastOnePadBeenActivated;

// GLOBAL: BIONICLE 0x34c57e0
static NUPADMAPPING g_nupadMapping[2];

// GLOBAL: BIONICLE 0x34c57f8
static NUGENERICPAD g_nupadScannedPads[16];

// FUNCTION: BIONICLE 0x56bc90
NUPAD *NuPadOpen(int port, int slot) {
    NUPAD *pad;

    pad = (NUPAD *)NuMemAllocAt(sizeof(NUPAD), 148);
    if (pad != NULL) {
        memset(pad, 0, sizeof(NUPAD));
        pad->port = port;
        pad->slot = slot;
        pad->stage = 0;
        pad->r_alg_x = pad->r_alg_y = pad->l_alg_x = pad->l_alg_y = 0x80;
    }
    NuPadOpenPS(pad);
    NuPadGetDeadzonePS(pad);
    return pad;
}

// FUNCTION: BIONICLE 0x56bd00
static float CalcRamp(float ramp) {
    int i;

    ramp = MIN(ramp, 1.0f);
    for (i = 0; ramp > ramp_vals[i][0]; i++) {
    }
    return ramp_vals[i][1];
}

// FUNCTION: BIONICLE 0x56bd50
void NuPadSetStatus(int gamepad, int active) {
    if (active == 1) {
        if (g_atLeastOnePadBeenActivated == 0) {
            g_profilePlayerPad = g_nupadMapping[gamepad].port;
        }
        g_atLeastOnePadBeenActivated = 1;
    }
    g_nupadMapping[gamepad].active = active;
}

// FUNCTION: BIONICLE 0x56c3a0
void NuPadMapPortToPS2Port(int gamepad, int port) {
    g_nupadMapping[gamepad].port = port;
    g_nupadScannedPads[port].mappedToPad = gamepad;
    g_nupadScannedPads[port].paddata_db = 0;
}

// FUNCTION: BIONICLE 0x56c3d0
int NuPadGetFirstActivePad(void) {
    int port;

    port = g_nupadMapping[0].port;
    if (port >= 0 && g_nupadScannedPads[port].valid) {
        return port;
    }
    return -1;
}

// FUNCTION: BIONICLE 0x56c3f0
int NuPadRead(NUPAD *pad) {
    int port;
    NUGENERICPAD *gp;
    float rate;

    port = g_nupadMapping[pad->port].port;
    if (port != -1) {
        gp = &g_nupadScannedPads[port];
        pad->l_alg_x = gp->l_alg_x;
        pad->l_alg_y = gp->l_alg_y;
        pad->r_alg_x = gp->r_alg_x;
        pad->r_alg_y = gp->r_alg_y;
        pad->l1_alg = gp->l1_alg;
        pad->l2_alg = gp->l2_alg;
        pad->r1_alg = gp->r1_alg;
        pad->r2_alg = gp->r2_alg;
        pad->paddata = gp->paddata;
        pad->paddata_db = gp->paddata_db;
        pad->paddata_dbr = gp->paddata_dbr;
        pad->oldpaddata = gp->oldpaddata;
        pad->analogue_buttons = gp->analogue_buttons;
        pad->deadzone = gp->deadzone;
        pad->ldx = gp->ldx;
        pad->ldy = gp->ldy;
        pad->rdx = gp->rdx;
        pad->rdy = gp->rdy;
        pad->l_nx = gp->l_nx;
        pad->l_ny = gp->l_ny;
        pad->l_rx = gp->l_rx;
        pad->l_ry = gp->l_ry;
        pad->l_dx = gp->l_dx;
        pad->l_dy = gp->l_dy;
        pad->r_dx = gp->r_dx;
        pad->r_dy = gp->r_dy;
        pad->r_nx = gp->r_nx;
        pad->r_ny = gp->r_ny;
        pad->r_rx = gp->r_rx;
        pad->r_ry = gp->r_ry;
        pad->valid = gp->valid;

        /* No analogue d-pad: ramp the digital directions up over a few frames. */
        if (!pad->analogue_buttons) {
            rate = ramp_rate * 0.033333335f;
            if (pad->paddata & 0x1000) {
                pad->l_up_ramp = MIN(1.0f, pad->l_up_ramp + rate);
            } else {
                pad->l_up_ramp = 0.0f;
            }
            if (pad->paddata & 0x4000) {
                pad->l_down_ramp = MIN(1.0f, pad->l_down_ramp + rate);
            } else {
                pad->l_down_ramp = 0.0f;
            }
            if (pad->paddata & 0x8000) {
                pad->l_left_ramp = MIN(1.0f, pad->l_left_ramp + rate);
            } else {
                pad->l_left_ramp = 0.0f;
            }
            if (pad->paddata & 0x2000) {
                pad->l_right_ramp = MIN(1.0f, pad->l_right_ramp + rate);
            } else {
                pad->l_right_ramp = 0.0f;
            }
            pad->l_algpad_l = (unsigned char)(CalcRamp(pad->l_left_ramp) * 255.0f);
            pad->l_algpad_r = (unsigned char)(CalcRamp(pad->l_right_ramp) * 255.0f);
            pad->l_algpad_u = (unsigned char)(CalcRamp(pad->l_up_ramp) * 255.0f);
            pad->l_algpad_d = (unsigned char)(CalcRamp(pad->l_down_ramp) * 255.0f);
        }
        return pad->valid;
    }
    pad->valid = 0;
    return 0;
}
