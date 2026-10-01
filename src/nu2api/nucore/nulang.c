#include "nulang.h"

/* Current language. Names from the PS2 prototype. */

// GLOBAL: BIONICLE 0x6174c4
static int nuapi_language = 1;

// FUNCTION: BIONICLE 0x56c650
int NuLanguageGet(void) {
    return nuapi_language;
}

// FUNCTION: BIONICLE 0x56c660
void NuLanguageSet(int language) {
    nuapi_language = language;
}
