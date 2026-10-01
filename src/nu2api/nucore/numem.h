#ifndef NUMEM_H
#define NUMEM_H

#include "nutypes.h"
#include "nuheap.h"

/* Debug block header in front of every heap allocation, and the tail mark after it
 * (followed by the allocating file's name). */
typedef struct memblk_head_s {
    struct memblk_head_s *succ;
    struct memblk_head_s *prev;
    int line;
    int cnt;
    int size;
    char mark[12];
} MEMBLK_HEAD;

typedef struct memblk_tail_s {
    char mark[12];
} MEMBLK_TAIL;

/* Allocations from a caller's buffer instead of the heap (NuMemSetExternal). */
typedef struct memexternal_s {
    VARIPTR *ptr;
    VARIPTR end;
} MEMEXTERNAL;

typedef struct memdiscardable_s {
    int buffsize;
    int bufffree;
    char *freeptr;
    int pad;
} MEMDISCARDABLE;

extern NUHEAP NuMem_Heap;
extern int highallocaddr;

void NuMemSetExternal(VARIPTR *ptr, VARIPTR *end);
void *NuMemAllocFn(int size, char *file, int line);
void NuMemFreeFn(void *addr, char *file, int line);

/* Byte-wise memmove (the name is ours). */
void NuMemMove(void *dest, void *src, int size);
void NuMemSet128(void *dest, unsigned char value, int count);

#define NuMemAlloc(size) NuMemAllocFn((size), __FILE__, __LINE__)
#define NuMemFree(addr) NuMemFreeFn((addr), __FILE__, __LINE__)

/* NuMemAlloc as it was called from a given line of the original source file. Our files
 * don't keep the original line numbering, and #line would break reccmp's line mapping. */
#define NuMemAllocAt(size, line) NuMemAllocFn((size), __FILE__, (line))
#define NuMemFreeAt(addr, line) NuMemFreeFn((addr), __FILE__, (line))

#endif
