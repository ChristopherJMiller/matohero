#include "nuheap.h"

#include <stddef.h>

/* Heap over a caller's buffer: a free list headed by a dummy block just after the heap
 * header, first fit, blocks split from the top of a free block and merged with free
 * neighbours on release. Based on opensagadev/saga src/nu2api/nucore/nuheap.cpp
 * (GPL-3.0); names and locals from the PS2 prototype. The PC exe only has the allocator
 * and NuHeapFree; no NuHeapCreate, so the engine heap is never set up on PC. */

#define INUSE 0x80000000
#define OVERHEAD (sizeof(NUHEAPBLOCK) + sizeof(NUHEAPBLOCK *))

static __inline unsigned int GetSize(NUHEAPBLOCK *hb) {
    return hb->size & 0x7fffffff;
}

static __inline void SetUsed(NUHEAPBLOCK *hb) {
    hb->size |= INUSE;
}

static __inline void SetFree(NUHEAPBLOCK *hb) {
    hb->size &= 0x7fffffff;
}

static __inline void SetSize(NUHEAPBLOCK *hb, unsigned int size) {
    hb->size = (hb->size & INUSE) | size;
}

static __inline void SetFooter(NUHEAPBLOCK *hb) {
    *(NUHEAPBLOCK **)((char *)hb + sizeof(NUHEAPBLOCK) + GetSize(hb)) = hb;
}

// FUNCTION: BIONICLE 0x56d080
static __inline void RemoveFromFreeList(NUHEAPBLOCK *hb) {
    if (hb->prevfree != NULL) {
        hb->prevfree->nextfree = hb->nextfree;
    }
    if (hb->nextfree != NULL) {
        hb->nextfree->prevfree = hb->prevfree;
    }
}

// FUNCTION: BIONICLE 0x56d0a0
static void SplitFreeBlock(NUHEAPBLOCK *block, unsigned int *size, NUHEAPBLOCK **used, NUHEAPBLOCK **remainder, unsigned int align) {
    unsigned int freesize;
    unsigned int original;
    unsigned int aligned;

    freesize = GetSize(block) - *size - OVERHEAD;
    *remainder = block;
    *used = (NUHEAPBLOCK *)((char *)block + freesize + OVERHEAD);
    if (align > 0) {
        original = (unsigned int)(*used + 1);
        aligned = (original + align - 1) & ~(align - 1);
        *used = (NUHEAPBLOCK *)((char *)*used + aligned - original);
        freesize += aligned - original;
        *size += original - aligned;
    }
    SetUsed(*used);
    SetSize(*used, *size);
    (*used)->nextfree = NULL;
    (*used)->prevfree = NULL;
    SetFooter(*used);
    SetFree(*remainder);
    SetSize(*remainder, freesize);
    (*remainder)->nextfree = NULL;
    (*remainder)->prevfree = NULL;
    SetFooter(*remainder);
}

// FUNCTION: BIONICLE 0x56d150
void *NuHeapAllocAlignedNamed(NUHEAP heap, unsigned int size, unsigned int align, char *name) {
    NUHEAPBLOCK *list = (NUHEAPBLOCK *)((char *)((_NUHEAP *)heap)->buffer + sizeof(_NUHEAP));
    NUHEAPBLOCK *block;
    unsigned int reqsize;
    NUHEAPBLOCK *used;
    NUHEAPBLOCK *remainder;

    if (size == 0) {
        return NULL;
    }
    size += align;
    reqsize = size + OVERHEAD;
    for (block = list; block != NULL; block = block->nextfree) {
        if (GetSize(block) > reqsize) {
            break;
        }
    }
    if (block == NULL) {
        return NULL;
    }
    RemoveFromFreeList(block);
    SplitFreeBlock(block, &size, &used, &remainder, align);
    if (list->nextfree != NULL) {
        list->nextfree->prevfree = remainder;
    }
    remainder->nextfree = list->nextfree;
    remainder->prevfree = list;
    list->nextfree = remainder;
    ((_NUHEAP *)heap)->alloc_count++;
    ((_NUHEAP *)heap)->total_allocated += GetSize(used) + OVERHEAD;
    return used + 1;
}

// FUNCTION: BIONICLE 0x56d210
void NuHeapFree(NUHEAP heap, void *mem) {
    NUHEAPBLOCK *list = (NUHEAPBLOCK *)((char *)((_NUHEAP *)heap)->buffer + sizeof(_NUHEAP));
    NUHEAPBLOCK *block;
    NUHEAPBLOCK *next;
    NUHEAPBLOCK *prev;

    if (mem == NULL) {
        return;
    }
    if ((char *)mem < (char *)((_NUHEAP *)heap)->buffer ||
        (char *)mem > (char *)((_NUHEAP *)heap)->buffer + ((_NUHEAP *)heap)->buffer_size) {
        return;
    }
    block = (NUHEAPBLOCK *)((char *)mem - sizeof(NUHEAPBLOCK));
    SetFree(block);
    ((_NUHEAP *)heap)->alloc_count--;
    ((_NUHEAP *)heap)->total_allocated -= GetSize(block);
    ((_NUHEAP *)heap)->total_allocated -= sizeof(NUHEAPBLOCK);
    ((_NUHEAP *)heap)->total_allocated -= sizeof(NUHEAPBLOCK *);
    if (list->nextfree != NULL) {
        list->nextfree->prevfree = block;
    }
    block->nextfree = list->nextfree;
    block->prevfree = list;
    list->nextfree = block;

    next = (NUHEAPBLOCK *)((char *)block + GetSize(block) + OVERHEAD);
    if (!(next->size & INUSE)) {
        RemoveFromFreeList(next);
        SetSize(block, GetSize(block) + GetSize(next) + OVERHEAD);
        SetFooter(block);
    }
    prev = *(NUHEAPBLOCK **)((char *)block - sizeof(NUHEAPBLOCK *));
    if (!(prev->size & INUSE)) {
        RemoveFromFreeList(block);
        SetSize(prev, GetSize(prev) + GetSize(block) + OVERHEAD);
        SetFooter(prev);
    }
}

// FUNCTION: BIONICLE 0x56d300
void *NuHeapAllocAligned(NUHEAP heap, unsigned int size, unsigned int align) {
    return NuHeapAllocAlignedNamed(heap, size, align, NULL);
}
