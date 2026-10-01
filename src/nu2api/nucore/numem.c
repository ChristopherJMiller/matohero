#include "numem.h"

#include <stdlib.h>
#include <string.h>

/* Memory allocation: from a caller's buffer (external), a discardable buffer, the
 * engine heap or malloc, with a debug header and tail mark around each heap block, plus
 * the PC's C versions of the PS2 assembly helpers (NuMemMove, our name, and NuMemSet128).
 * Names and locals from the PS2 prototype; the PC build has no NuMemAlloc (non-Fn),
 * NuMemReAllocFn or validation/dump functions. */

typedef union {
    char *s8;
    int *i32;
    int intval;
    void *vptr;
    MEMBLK_HEAD *head;
    MEMBLK_TAIL *tail;
} MEMPTR;

// GLOBAL: BIONICLE 0x6171f8
static char blk_hdr[13] = "MEMBLKHEADER";

// GLOBAL: BIONICLE 0x617208
static char blk_end[13] = "MEMBLKTAIL01";

// GLOBAL: BIONICLE 0x34b72f8
static MEMEXTERNAL memext;

// GLOBAL: BIONICLE 0x34b7300
static MEMBLK_HEAD *alloc_list;

// GLOBAL: BIONICLE 0x34b7304
static int totalloc;

// GLOBAL: BIONICLE 0x34b7308
static MEMDISCARDABLE *discardbuff;

// GLOBAL: BIONICLE 0x34b730c
static int alloccnt;

// GLOBAL: BIONICLE 0x34b7310
static MEMEXTERNAL *memexternal;

// GLOBAL: BIONICLE 0x34b7314
NUHEAP NuMem_Heap;

// GLOBAL: BIONICLE 0x34b7318
int highallocaddr;

// GLOBAL: BIONICLE 0x34b731c
static int peakallocaddr;

// FUNCTION: BIONICLE 0x565960
void NuMemSetExternal(VARIPTR *ptr, VARIPTR *end) {
    if (ptr != NULL) {
        memext.ptr = ptr;
        memexternal = &memext;
        if (end != NULL) {
            memext.end = *end;
        }
    } else {
        memexternal = NULL;
    }
}

// FUNCTION: BIONICLE 0x5659a0
void *NuMemAllocFn(int size, char *file, int line) {
    MEMPTR rv;
    int endaddr;
    void *ptr;
    MEMBLK_TAIL *tail;

    if (memexternal != NULL) {
        memexternal->ptr->intaddr = (memexternal->ptr->intaddr + 0xf) & ~0xf;
        if (memexternal->end.intaddr - memexternal->ptr->intaddr >= size) {
            ptr = memexternal->ptr->voidptr;
            memexternal->ptr->intaddr += size;
            return ptr;
        }
        return NULL;
    }

    if (discardbuff != NULL) {
        size = (size + 0xf) & ~0xf;
        if (size < discardbuff->bufffree) {
            ptr = discardbuff->freeptr;
            discardbuff->freeptr += size;
            discardbuff->bufffree -= size;
            return ptr;
        }
        return NULL;
    }

    if (NuMem_Heap != NULL) {
        rv.vptr = NuHeapAllocAligned(NuMem_Heap, size + strlen(file) + 0x2d, 4);
    } else {
        rv.vptr = malloc(size + strlen(file) + 0x2d);
        if (rv.vptr != NULL) {
            memset(rv.vptr, 0, size + strlen(file) + 0x2d);
        }
    }

    if (rv.vptr != NULL) {
        memset(rv.vptr, 0, size + strlen(file) + 0x2d);
        totalloc += size;
        rv.head->size = size;
        tail = (MEMBLK_TAIL *)(rv.s8 + sizeof(MEMBLK_HEAD) + size);
        memcpy(rv.head->mark, blk_hdr, 12);
        memcpy(tail->mark, blk_end, 12);
        strcpy((char *)(tail + 1), file);
        rv.head->cnt = alloccnt++;
        rv.head->line = line;
        rv.head->succ = alloc_list;
        if (alloc_list != NULL) {
            alloc_list->prev = rv.head;
        }
        rv.head->prev = NULL;
        alloc_list = rv.head;
        rv.s8 += sizeof(MEMBLK_HEAD);
    }

    endaddr = rv.intval + size;
    if (endaddr > highallocaddr) {
        highallocaddr = endaddr;
    }
    if (endaddr > peakallocaddr) {
        peakallocaddr = endaddr;
    }
    return rv.vptr;
}

// FUNCTION: BIONICLE 0x565b80
void NuMemFreeFn(void *addr, char *file, int line) {
    MEMPTR head;

    if (addr != NULL) {
        head.vptr = addr;
        head.head--;
        if (head.head->succ != NULL) {
            head.head->succ->prev = head.head->prev;
        }
        if (head.head->prev != NULL) {
            head.head->prev->succ = head.head->succ;
        } else {
            alloc_list = head.head->succ;
        }
        head.head->succ = NULL;
        head.head->prev = NULL;
        totalloc -= head.head->size;
        if (NuMem_Heap != NULL) {
            NuHeapFree(NuMem_Heap, head.vptr);
        } else {
            free(head.vptr);
        }
    }
}

// FUNCTION: BIONICLE 0x565bf0
void NuMemMove(void *dest, void *src, int size) {
    char *d;
    char *s;

    d = (char *)dest;
    s = (char *)src;
    if (d < s) {
        while (size != 0) {
            *d = *s;
            d++;
            s++;
            size--;
        }
    } else {
        d += size;
        s += size;
        while (size != 0) {
            d--;
            s--;
            *d = *s;
            size--;
        }
    }
}

/* PS2: nucore/ps2/asmmem.s, which also had NuMemCopy128. Fills count 16-byte units. */
// FUNCTION: BIONICLE 0x565c40
void NuMemSet128(void *dest, unsigned char value, int count) {
    memset(dest, value, count * 16);
}
