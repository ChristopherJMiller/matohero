#include "numemblk.h"

#include <string.h>

#include "nutypes.h"

/* Block pools: a header, then cnt blocks of size rounded up to the mask, chained into a
 * free list. Freed blocks are filled with 0xfe, allocated ones with 0xff. Names and
 * locals from the PS2 prototype; the PC exe only has CreateEx, Size, Alloc and Free. */

// FUNCTION: BIONICLE 0x56b070
NUMEMBLK *NuMemBlkCreateEx(int size, int cnt, int mask, void *buff) {
    NUMEMBLK *memblk;
    void **ptr;
    int i;
    int step;
    int blksize;
    int addr;

    if (size < sizeof(void *)) {
        size = sizeof(void *);
    }
    blksize = (size + mask) & ~mask;
    memblk = (NUMEMBLK *)buff;
    memblk->blksize = blksize;
    memblk->blkcnt = cnt;
    memblk->blkfree = cnt;
    memblk->flags = 1;
    if (cnt != 0) {
        addr = (sizeof(NUMEMBLK) + mask) & ~mask;
        step = blksize / sizeof(void *);
        ptr = (void **)((char *)memblk + addr);
        memblk->free = ptr;
        for (i = 0; i < cnt - 1; i++) {
            *ptr = ptr + step;
            ptr += step;
        }
        *ptr = NULL;
    } else {
        memblk->free = NULL;
    }
    return memblk;
}

// FUNCTION: BIONICLE 0x56b0e0
int NuMemBlkSize(int size, int cnt, int mask) {
    int hdrsize;
    int blksize;

    blksize = (size + mask) & ~mask;
    hdrsize = (sizeof(NUMEMBLK) + mask) & ~mask;
    return blksize * cnt + hdrsize;
}

// FUNCTION: BIONICLE 0x56b100
void *NuMemBlkAlloc(NUMEMBLK *memblk) {
    void *ptr;

    ptr = memblk->free;
    if (ptr > NULL) {
        memblk->free = *(void **)ptr;
        memblk->blkfree--;
        memset(ptr, 0xff, memblk->blksize);
    }
    return ptr;
}

// FUNCTION: BIONICLE 0x56b130
void NuMemBlkFree(NUMEMBLK *memblk, void *ptr) {
    memblk->blkfree++;
    memset(ptr, 0xfe, memblk->blksize);
    *(void **)ptr = memblk->free;
    memblk->free = ptr;
}
