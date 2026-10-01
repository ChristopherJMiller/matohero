#ifndef NUMEM_H
#define NUMEM_H

#include "nutypes.h"

void *NuMemAllocFn(int size, char *file, int line);

#define NuMemAlloc(size) NuMemAllocFn((size), __FILE__, __LINE__)

/* NuMemAlloc as it was called from a given line of the original source file. Our files
 * don't keep the original line numbering, and #line would break reccmp's line mapping. */
#define NuMemAllocAt(size, line) NuMemAllocFn((size), __FILE__, (line))

#endif
