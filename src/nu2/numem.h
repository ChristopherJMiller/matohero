#ifndef NUMEM_H
#define NUMEM_H

#include "nutypes.h"

/* Name is a guess: Crash WoC's Nu2 has NuMemAlloc(size); BH's build passes the call
 * site's file and line, as in debug-aware TT allocators. */
void *NuMemAllocFn(i32 size, char *file, i32 line);

#define NuMemAlloc(size) NuMemAllocFn((size), __FILE__, __LINE__)

/* NuMemAlloc as it was called from a given line of the original source file. Our files
 * don't keep the original line numbering, and #line would break reccmp's line mapping. */
#define NuMemAllocAt(size, line) NuMemAllocFn((size), __FILE__, (line))

#endif
