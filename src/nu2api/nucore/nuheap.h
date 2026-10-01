#ifndef NUHEAP_H
#define NUHEAP_H

/* A heap inside a caller's buffer: blocks with a size word (top bit: in use), free-list
 * links and a footer pointing back at the block. */
typedef void *NUHEAP;

typedef struct nuheapblock_s {
    unsigned int size;
    struct nuheapblock_s *nextfree;
    struct nuheapblock_s *prevfree;
} NUHEAPBLOCK;

typedef struct {
    void *buffer;
    unsigned int buffer_size;
    unsigned int alloc_count;
    unsigned int total_allocated;
} _NUHEAP;

void *NuHeapAllocAlignedNamed(NUHEAP heap, unsigned int size, unsigned int align, char *name);
void NuHeapFree(NUHEAP heap, void *mem);
void *NuHeapAllocAligned(NUHEAP heap, unsigned int size, unsigned int align);

#endif
