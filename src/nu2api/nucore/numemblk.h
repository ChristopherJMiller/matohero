#ifndef NUMEMBLK_H
#define NUMEMBLK_H

/* Fixed-size block pools carved from a caller's buffer. */
typedef struct numemblk_s {
    void **free;
    int blksize;
    int blkcnt;
    short flags;
    short blkfree;
} NUMEMBLK;

NUMEMBLK *NuMemBlkCreateEx(int size, int cnt, int mask, void *buff);
int NuMemBlkSize(int size, int cnt, int mask);
void *NuMemBlkAlloc(NUMEMBLK *memblk);
void NuMemBlkFree(NUMEMBLK *memblk, void *ptr);

#endif
