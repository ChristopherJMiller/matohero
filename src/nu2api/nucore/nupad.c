#include "nupad.h"

#include <string.h>

#include "folded.h"
#include "numem.h"

/* Pads: the PC layer scans physical controllers into g_nupadScannedPads, game pads are
 * mapped onto them, and NuPadRead copies a mapped controller into the game's NUPAD.
 * Based on opensagadev/saga src/nu2api/nucore/nupad.cpp (GPL-3.0, a later engine with a
 * different NUPAD); names and locals from the PS2 prototype. PC differences: 16 scanned
 * pads, NuPadGetFirstActivePad reads the mapping, no NuPadInit/Close/SetMotors/ResetState
 * here, and NuPadRead is a PC-layer wrapper (0x57a540, like PS2's around NuPs2ReadPad)
 * around NuPadReadMapped (our name), which adds d-pad ramps. */

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Game pads (players). */
#define NUPAD_MAXGAMEPADS 2

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

static __inline void DeadZoneXY(int *dx, int *dy, int deadzone) {
    int x;
    int y;

    x = *dx;
    y = *dy;
    if (x > -deadzone && x < deadzone) {
        *dx = 0;
    }
    if (y > -deadzone && y < deadzone) {
        *dy = 0;
    }
}

// FUNCTION: BIONICLE 0x56bd90
void NuPadProcessReadData(NUGENERICPAD *pad) {
    float rate;

    pad->paddata_db = (pad->paddata ^ pad->oldpaddata) & pad->paddata;
    pad->paddata_dbr = (pad->paddata ^ pad->oldpaddata) & pad->oldpaddata;
    pad->oldpaddata = pad->paddata;
    pad->ldx = pad->l_alg_x - 0x80;
    pad->ldy = 0x80 - pad->l_alg_y;
    pad->rdx = pad->r_alg_x - 0x80;
    pad->rdy = 0x80 - pad->r_alg_y;
    if (pad->deadzone != 0) {
        DeadZoneXY(&pad->ldx, &pad->ldy, pad->deadzone);
        DeadZoneXY(&pad->rdx, &pad->rdy, pad->deadzone);
        pad->l_alg_x = pad->ldx + 0x80;
        pad->l_alg_y = 0x80 - pad->ldy;
        pad->r_alg_x = pad->rdx + 0x80;
        pad->r_alg_y = 0x80 - pad->rdy;
    }

    pad->l_nx = pad->ldx / 128.0f;
    pad->l_ny = pad->ldy / 128.0f;
    if (pad->l_nx > 0.0f) {
        pad->l_nx = MAX(0.0f, pad->l_nx - pad->l_dx);
    }
    if (pad->l_nx < 0.0f) {
        pad->l_nx = MIN(0.0f, pad->l_nx + pad->l_dx);
    }
    if (pad->l_ny > 0.0f) {
        pad->l_ny = MAX(0.0f, pad->l_ny - pad->l_dy);
    }
    if (pad->l_ny < 0.0f) {
        pad->l_ny = MIN(0.0f, pad->l_ny + pad->l_dy);
    }

    pad->r_nx = pad->rdx / 128.0f;
    pad->r_ny = pad->rdy / 128.0f;
    if (pad->r_nx > 0.0f) {
        pad->r_nx = MAX(0.0f, pad->r_nx - pad->r_dx);
    }
    if (pad->r_nx < 0.0f) {
        pad->r_nx = MIN(0.0f, pad->r_nx + pad->r_dx);
    }
    if (pad->r_ny > 0.0f) {
        pad->r_ny = MAX(0.0f, pad->r_ny - pad->r_dy);
    }
    if (pad->r_ny < 0.0f) {
        pad->r_ny = MIN(0.0f, pad->r_ny + pad->r_dy);
    }

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
}

/* Activity on a scanned pad: buttons held or a stick off centre. The original tests
 * r_alg_x twice (and never r_alg_y). */
#define PAD_ACTIVE(gp)                                                                   \
    ((gp)->paddata != 0 || (gp)->l_alg_x != 0x80 || (gp)->l_alg_y != 0x80 ||          \
     (gp)->r_alg_x != 0x80 || (gp)->r_alg_x != 0x80)

// FUNCTION: BIONICLE 0x56c130
void NuPadUpdatePads(void) {
    int i;
    int j;
    int numpads;

    numpads = NuPadGetMaxDevicesPS();
    for (i = 0; i < numpads; i++) {
        g_nupadScannedPads[i].valid = NuPadReadPS(
            i, &g_nupadScannedPads[i].l_alg_x, &g_nupadScannedPads[i].l_alg_y,
            &g_nupadScannedPads[i].r_alg_x, &g_nupadScannedPads[i].r_alg_y,
            &g_nupadScannedPads[i].l1_alg, &g_nupadScannedPads[i].l2_alg,
            &g_nupadScannedPads[i].r1_alg, &g_nupadScannedPads[i].r2_alg,
            &g_nupadScannedPads[i].paddata);
        if (g_nupadScannedPads[i].valid) {
            g_nupadScannedPads[i].deadzone = NuPadGetDeadzoneByPortPS(i);
        }
        NuPadProcessReadData(&g_nupadScannedPads[i]);
    }

    /* Unmap scanned pads no active game pad uses. */
    for (i = 0; i < numpads; i++) {
        if (!((g_nupadMapping[0].active && i == g_nupadMapping[0].port) ||
              (g_nupadMapping[1].active && i == g_nupadMapping[1].port))) {
            g_nupadScannedPads[i].mappedToPad = -1;
        }
    }

    /* Before anyone has pressed start, game pad 0 follows whichever pad is used. */
    if (!g_atLeastOnePadBeenActivated) {
        g_nupadMapping[0].port = -1;
        for (j = 0; j < numpads; j++) {
            if (g_nupadScannedPads[j].valid) {
                g_nupadMapping[0].port = j;
                g_nupadScannedPads[j].mappedToPad = 0;
                if (PAD_ACTIVE(&g_nupadScannedPads[j])) {
                    g_nupadMapping[0].port = j;
                    g_nupadScannedPads[j].mappedToPad = 0;
                    if (g_nupadScannedPads[j].paddata_db & 0x840) {
                        return;
                    }
                }
            }
        }
    }

    if (g_atLeastOnePadBeenActivated) {
        for (i = 0; i < NUPAD_MAXGAMEPADS; i++) {
            /* An active game pad whose controller went away takes the first free one
             * that presses start or select. */
            if (g_nupadMapping[i].active == 1 && !g_nupadScannedPads[g_nupadMapping[i].port].valid) {
                for (j = 0; j < numpads; j++) {
                    if (g_nupadScannedPads[j].valid && (g_nupadScannedPads[j].paddata_db & 0x840) &&
                        (g_nupadScannedPads[j].mappedToPad == -1 ||
                         !g_nupadMapping[g_nupadScannedPads[j].mappedToPad].active)) {
                        goto found;
                    }
                }
            }

            /* An inactive one follows any unclaimed pad, preferring one in use. */
            if (!g_nupadMapping[i].active) {
                g_nupadMapping[i].port = -1;
                if (g_nupadMapping[0].active + g_nupadMapping[1].active == 0) {
                    if (i == 0) {
                        for (j = 0; j < numpads; j++) {
                            if (g_nupadScannedPads[j].mappedToPad == -1 ||
                                g_nupadMapping[g_nupadScannedPads[j].mappedToPad].active != 1) {
                                g_nupadScannedPads[j].mappedToPad = -1;
                            }
                        }
                    }
                } else {
                    for (j = 0; j < numpads; j++) {
                        if (g_nupadScannedPads[j].mappedToPad == -1 ||
                            g_nupadMapping[g_nupadScannedPads[j].mappedToPad].active != 1) {
                            g_nupadScannedPads[j].mappedToPad = -1;
                        }
                    }
                }
                for (j = 0; j < numpads; j++) {
                    if (g_nupadScannedPads[j].mappedToPad == -1 && g_nupadScannedPads[j].valid) {
                        g_nupadMapping[i].port = j;
                        g_nupadScannedPads[j].mappedToPad = i;
                        if (PAD_ACTIVE(&g_nupadScannedPads[j]) || (g_nupadScannedPads[j].paddata_db & 0x840)) {
                            break;
                        }
                    }
                }
            }
            continue;
        found:
            g_nupadMapping[i].port = j;
            g_nupadScannedPads[j].mappedToPad = i;
            g_nupadScannedPads[j].paddata_db = 0;
        }
    }
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
int NuPadReadMapped(NUPAD *pad) {
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
