#include "nuptrblock.h"

#include "nutypes.h"

/* Pointer blocks: data followed by a table of self-relative offsets to the pointers in
 * it, which are themselves stored relative to their own address. Based on
 * opensagadev/saga src/nu2api/nucore/nuptrblock.cpp (GPL-3.0); names and locals from
 * the PS2 prototype. */

// FUNCTION: BIONICLE 0x56d010
void *NuPtrBlockRead(NUFILE fh) {
    VARIPTR buff;
    VARIPTR start;
    int i;
    int npointers;
    unsigned int *ptr;

    buff.voidptr = NuMemFileAddr(fh);
    start.intaddr = buff.intaddr + 4;
    buff.intaddr += *buff.u32;
    npointers = *buff.s32++;
    for (i = 0; i < npointers; i++) {
        ptr = (unsigned int *)(buff.intaddr + *buff.u32);
        *ptr += (unsigned int)ptr;
        buff.u32++;
    }
    return start.voidptr;
}

// FUNCTION: BIONICLE 0x56d050
void *NuPtrBlockFix(void *data) {
    VARIPTR buff;
    VARIPTR start;
    int i;
    int npointers;
    unsigned int *ptr;

    buff.voidptr = data;
    start.intaddr = buff.intaddr + 4;
    buff.intaddr += *buff.u32;
    npointers = *buff.s32++;
    for (i = 0; i < npointers; i++) {
        ptr = (unsigned int *)(buff.intaddr + *buff.u32);
        if (*ptr != 0) {
            *ptr += (unsigned int)ptr;
        }
        buff.u32++;
    }
    return start.voidptr;
}
