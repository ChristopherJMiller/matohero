#ifndef NUPAD_H
#define NUPAD_H

/* Pad state as the game sees it. The PC layout (0xac bytes) drops the PS2 DMA buffer,
 * adds the d-pad ramps, and widens the flag bits to an int. */
typedef struct nupad_s {
    int port;
    int slot;
    int stage;
    int id;
    int ldx;
    int ldy;
    int rdx;
    int rdy;
    float l_nx;
    float l_ny;
    float l_rx;
    float l_ry;
    float l_dx;
    float l_dy;
    float r_nx;
    float r_ny;
    float r_rx;
    float r_ry;
    float r_dx;
    float r_dy;
    float l_up_ramp;
    float l_down_ramp;
    float l_left_ramp;
    float l_right_ramp;
    unsigned char act_direct[6];
    unsigned char act_align[6];
    int unknown_6c;
    unsigned int paddata;
    unsigned int oldpaddata;
    unsigned int paddata_db;
    unsigned int paddata_dbr;
    int analogue_sticks : 1;
    int analogue_buttons : 1;
    int valid : 1;
    int must_have_analogue_sticks : 1;
    char deadloffx;
    char deadloffy;
    char deadroffx;
    char deadroffy;
    char deadzone;
    char deadrange;
    char deadseek;
    unsigned char old_read_id;
    unsigned char ok;
    unsigned char read_id;
    unsigned char buttons_hi;
    unsigned char buttons_lo;
    unsigned char r_alg_x;
    unsigned char r_alg_y;
    unsigned char l_alg_x;
    unsigned char l_alg_y;
    unsigned char l_algpad_r;
    unsigned char l_algpad_l;
    unsigned char l_algpad_u;
    unsigned char l_algpad_d;
    unsigned char r_algpad_u;
    unsigned char r_algpad_r;
    unsigned char r_algpad_d;
    unsigned char r_algpad_l;
    unsigned char l1_alg;
    unsigned char r1_alg;
    unsigned char l2_alg;
    unsigned char r2_alg;
    unsigned char pad[12];
} NUPAD;

/* One physical controller's processed input (PS2 layout, 0x7c bytes). */
typedef struct _nugenericpad_s {
    unsigned char valid;
    int mappedToPad;
    unsigned char l_alg_x;
    unsigned char l_alg_y;
    unsigned char r_alg_x;
    unsigned char r_alg_y;
    unsigned char l1_alg;
    unsigned char l2_alg;
    unsigned char r1_alg;
    unsigned char r2_alg;
    unsigned int paddata;
    unsigned int paddata_db;
    unsigned int paddata_dbr;
    unsigned int oldpaddata;
    int analogue_buttons;
    int ldx;
    int ldy;
    int rdx;
    int rdy;
    int deadzone;
    float l_nx;
    float l_ny;
    float l_rx;
    float l_ry;
    float l_dx;
    float l_dy;
    float r_dx;
    float r_dy;
    float r_nx;
    float r_ny;
    float r_rx;
    float r_ry;
    float l_up_ramp;
    float l_down_ramp;
    float l_left_ramp;
    float l_right_ramp;
    unsigned char l_algpad_l;
    unsigned char l_algpad_r;
    unsigned char l_algpad_u;
    unsigned char l_algpad_d;
} NUGENERICPAD;

/* Game pad (player) -> physical port. */
typedef struct _nupadmapping_s {
    int gamepad;
    int port;
    int active;
} NUPADMAPPING;

extern int g_profilePlayerPad;

NUPAD *NuPadOpen(int port, int slot);
void NuPadSetStatus(int gamepad, int active);
void NuPadUpdatePads(void);
void NuPadMapPortToPS2Port(int gamepad, int port);
int NuPadGetFirstActivePad(void);
int NuPadReadMapped(NUPAD *pad);
void NuPadProcessReadData(NUGENERICPAD *pad);

/* PC platform layer (not decompiled; pc/nupad_pc_stubs.c). */
int NuPadGetMaxDevicesPS(void);
int NuPadReadPS(int port, unsigned char *l_alg_x, unsigned char *l_alg_y, unsigned char *r_alg_x,
                unsigned char *r_alg_y, unsigned char *l1_alg, unsigned char *l2_alg,
                unsigned char *r1_alg, unsigned char *r2_alg, unsigned int *paddata);

#endif
