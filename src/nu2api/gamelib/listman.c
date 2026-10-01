#include "listman.h"

#include "numem.h"
#include "nuthread.h"

/* Logic ported from opensagadev/saga src/nu2api/nucore/nulst.cpp (GPL-3.0); names and
 * types are the originals from the PS2 prototype's debug info. NuLstCreateBuff is
 * written from BH's own code, which differs from saga's later engine. */

// STRING: BIONICLE 0x5e07e8
// __FILE__, passed to NuMemAlloc: ".\\listman.c"

// FUNCTION: BIONICLE 0x5654c0
NULSTHDR *NuLstCreate(int elcnt, int elsize) {
    NULSTHDR *list;
    int eltotsize;
    int n;
    NULNKHDR *curr;
    NULNKHDR *next;

    eltotsize = elsize + sizeof(NULNKHDR);
    list = (NULSTHDR *)NuMemAllocAt(elcnt * eltotsize + sizeof(NULSTHDR), 97);

    if (list != NULL) {
        list->free = (NULNKHDR *)(list + 1);
        list->head = NULL;
        list->tail = NULL;

        list->elcnt = elcnt;
        list->elsize = elsize;
        list->eltotsize = eltotsize;
        list->usedcnt = 0;

        curr = list->free;
        next = (NULNKHDR *)((char *)curr + eltotsize);
        for (n = 1; n < elcnt; n++) {
            curr->succ = next;
            curr->id = n - 1;
            curr->owner = list;
            curr = next;
            next = (NULNKHDR *)((char *)next + eltotsize);
        }

        curr->succ = NULL;
        list->free_tail = curr;
        curr->id = n - 1;
        curr->owner = list;
        list->safe_thread = nu_current_thread_id;
    }

    return list;
}

/* Effective match: the original loads nu_current_thread_id into edx rather than ecx.
 * Local types and declaration order don't change it. */
// FUNCTION: BIONICLE 0x565550
NULSTHDR *NuLstCreateBuff(int elcnt, int elsize, VARIPTR *ptr, VARIPTR end, int align) {
    NULSTHDR *list;
    unsigned int eltotsize;
    unsigned int buffsize;
    int n;
    NULNKHDR *curr;
    NULNKHDR *next;

    list = (NULSTHDR *)((ptr->addr + align - 1) & ~(align - 1));
    eltotsize = (elsize + sizeof(NULNKHDR) - 1 + align) & -align;
    buffsize = elcnt * eltotsize + sizeof(NULSTHDR);
    ptr->void_ptr = list;
    if (buffsize < end.addr - (unsigned int)list) {
        ptr->addr = (unsigned int)list + buffsize;

        list->free = (NULNKHDR *)(list + 1);
        list->head = NULL;
        list->tail = NULL;

        list->elcnt = elcnt;
        list->elsize = elsize;
        list->eltotsize = eltotsize;
        list->usedcnt = 0;

        curr = list->free;
        next = (NULNKHDR *)((char *)curr + eltotsize);
        for (n = 1; n < elcnt; n++) {
            curr->succ = next;
            curr->id = n - 1;
            curr->owner = list;
            curr = next;
            next = (NULNKHDR *)((char *)next + eltotsize);
        }

        curr->succ = NULL;
        list->free_tail = curr;
        curr->id = n - 1;
        curr->owner = list;
        list->safe_thread = nu_current_thread_id;

        return list;
    }

    return NULL;
}

// FUNCTION: BIONICLE 0x565600
NULNKHDR *NuLstAllocHead(NULSTHDR *hdr) {
    NULNKHDR *rv;
    unsigned int threadid;

    if (hdr->free != NULL) {
        threadid = nu_current_thread_id;
        if (hdr->safe_thread != threadid) {
            (*NuThreadDisableThreadSwap)();
        }

        rv = hdr->free;
        hdr->free = hdr->free->succ;
        if (hdr->free == NULL) {
            hdr->free_tail = NULL;
        } else {
            hdr->free->prev = NULL;
        }

        rv->succ = hdr->head;
        if (hdr->head != NULL) {
            hdr->head->prev = rv;
        } else {
            hdr->tail = rv;
        }
        rv->prev = NULL;

        hdr->head = rv;
        hdr->usedcnt++;
        rv->used = 1;

        if (hdr->safe_thread != threadid) {
            (*NuThreadEnableThreadSwap)();
        }
        return rv + 1;
    }

    return NULL;
}

// FUNCTION: BIONICLE 0x565680
NULNKHDR *NuLstAllocTail(NULSTHDR *hdr) {
    NULNKHDR *rv;
    unsigned int threadid;

    if (hdr->free != NULL) {
        threadid = nu_current_thread_id;
        if (hdr->safe_thread != threadid) {
            (*NuThreadDisableThreadSwap)();
        }

        rv = hdr->free;
        hdr->free = hdr->free->succ;
        if (hdr->free == NULL) {
            hdr->free_tail = NULL;
        } else {
            hdr->free->prev = NULL;
        }

        rv->prev = hdr->tail;
        if (hdr->tail != NULL) {
            hdr->tail->succ = rv;
        } else {
            hdr->head = rv;
        }
        rv->succ = NULL;

        hdr->tail = rv;
        rv->used = 1;
        hdr->usedcnt++;

        if (hdr->safe_thread != threadid) {
            (*NuThreadEnableThreadSwap)();
        }
        return rv + 1;
    }

    return NULL;
}

// FUNCTION: BIONICLE 0x565700
NULNKHDR *NuLstAllocFree(NULSTHDR *hdr) {
    NULNKHDR *rv;
    unsigned int threadid;

    if (hdr->free != NULL) {
        threadid = nu_current_thread_id;
        if (hdr->safe_thread != threadid) {
            (*NuThreadDisableThreadSwap)();
        }

        rv = hdr->free;
        hdr->free = hdr->free->succ;
        if (hdr->free == NULL) {
            hdr->free_tail = NULL;
        } else {
            hdr->free->prev = NULL;
        }

        rv->prev = NULL;
        rv->used = 1;
        ++hdr->usedcnt;

        if (hdr->safe_thread != threadid) {
            (*NuThreadEnableThreadSwap)();
        }
        return rv + 1;
    }

    return NULL;
}

// FUNCTION: BIONICLE 0x565760
void NuLstAttachTail(NULSTHDR *hdr, NULNKHDR *rv) {
    unsigned int threadid = nu_current_thread_id;

    if (hdr->safe_thread != threadid) {
        (*NuThreadDisableThreadSwap)();
    }

    rv->prev = hdr->tail;
    rv->succ = NULL;
    if (hdr->tail != NULL) {
        hdr->tail->succ = rv;
    } else {
        hdr->head = rv;
    }
    hdr->tail = rv;

    if (hdr->safe_thread != threadid) {
        (*NuThreadEnableThreadSwap)();
    }
}

// FUNCTION: BIONICLE 0x5657b0
void NuLstFree(NULNKHDR *lnk) {
    NULSTHDR *hdr;
    unsigned int threadid;

    lnk--;
    hdr = lnk->owner;

    threadid = nu_current_thread_id;
    if (hdr->safe_thread != threadid) {
        (*NuThreadDisableThreadSwap)();
    }

    if (lnk->succ != NULL) {
        lnk->succ->prev = lnk->prev;
    } else {
        hdr->tail = lnk->prev;
    }

    if (lnk->prev != NULL) {
        lnk->prev->succ = lnk->succ;
    } else {
        hdr->head = lnk->succ;
    }

    lnk->prev = hdr->free_tail;
    lnk->succ = NULL;
    if (hdr->free_tail != NULL) {
        hdr->free_tail->succ = lnk;
    } else {
        hdr->free_tail = hdr->free = lnk;
    }
    hdr->free_tail = lnk;

    hdr->usedcnt--;
    lnk->used = 0;

    if (hdr->safe_thread != threadid) {
        (*NuThreadEnableThreadSwap)();
    }
}

// FUNCTION: BIONICLE 0x565840
NULNKHDR *NuLstGetByIdx(NULSTHDR *hdr, int idx) {
    NULNKHDR *rv = (NULNKHDR *)((char *)(hdr + 1) + hdr->eltotsize * idx);

    if (rv->used) {
        return rv + 1;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x565860
NULNKHDR *NuLstGetNext(NULSTHDR *hdr, NULNKHDR *lnk) {
    if (lnk != NULL) {
        --lnk;
        if (lnk->succ != NULL) {
            return lnk->succ + 1;
        }
    } else if (hdr->head != NULL) {
        return hdr->head + 1;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x565890
NULNKHDR *NuLstAlloc(NULSTHDR *hdr) {
    return NuLstAllocHead(hdr);
}
