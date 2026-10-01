#include "nufile.h"

/* Memory-card file layer (PS2: nucore/ps2/ps2mc.c). On PC every entry point is an empty
 * function returning 0, and the linker folded them all into one copy at 0x57a600, so
 * this one stub stands for all of them. The file name is ours. */

// STUB: BIONICLE 0x57a600
int NuMcWrite() {
    return 0;
}
