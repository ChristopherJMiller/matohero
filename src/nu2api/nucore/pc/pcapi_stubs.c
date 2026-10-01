/* PC platform layer functions the engine core calls, not decompiled yet. The PC layer has
 * no surviving names: apart from NuFrameEndBgLoadPS (the PS2 hook it implements), these
 * names are ours, from what the code does. */

// GLOBAL: BIONICLE 0x6189d8
int pc_app_active;

// STUB: BIONICLE 0x56b3f0
int NuFrameEndBgLoadPS(int min_delay) {
    return 0;
}

// STUB: BIONICLE 0x57b940
void PCFrameLock(void) {
}

// STUB: BIONICLE 0x57b960
void PCFrameUnlock(void) {
}

// STUB: BIONICLE 0x573030
void PCSoundPause(void) {
}

// STUB: BIONICLE 0x5717b0
void PCSoundResume(void) {
}

// STUB: BIONICLE 0x57b490
int PCProcessMessages(void) {
    return 0;
}

// STUB: BIONICLE 0x57dd80
void PCCheckDevice(void) {
}
