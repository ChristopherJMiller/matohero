#ifndef LISTMAN_H
#define LISTMAN_H

#include "nutypes.h"

/* Pooled doubly linked lists: a fixed number of fixed-size elements allocated in one
 * block, each preceded by a link header. The API takes and returns element pointers
 * (the memory just past the NULNKHDR). Names are the originals, from the PS2 prototype's
 * debug info. */

typedef struct nulsthdr_s NULSTHDR_T;

typedef struct nulnkhdr_s {
    NULSTHDR_T *owner;
    struct nulnkhdr_s *succ;
    struct nulnkhdr_s *prev;
    unsigned short id;
    unsigned short used : 1;
} NULNKHDR;

typedef struct nulsthdr_s {
    NULNKHDR *free;
    NULNKHDR *free_tail;
    NULNKHDR *head;
    NULNKHDR *tail;
    short elcnt;
    short elsize;
    short eltotsize;
    short usedcnt;
    unsigned int safe_thread;
    short pad[2];
} NULSTHDR;

NULSTHDR *NuLstCreate(int elcnt, int elsize);
NULSTHDR *NuLstCreateBuff(int elcnt, int elsize, VARIPTR *ptr, VARIPTR end, int align);
NULNKHDR *NuLstAlloc(NULSTHDR *hdr);
NULNKHDR *NuLstAllocHead(NULSTHDR *hdr);
NULNKHDR *NuLstAllocTail(NULSTHDR *hdr);
NULNKHDR *NuLstAllocFree(NULSTHDR *hdr);
void NuLstAttachTail(NULSTHDR *hdr, NULNKHDR *rv);
void NuLstFree(NULNKHDR *lnk);
NULNKHDR *NuLstGetByIdx(NULSTHDR *hdr, int idx);
NULNKHDR *NuLstGetNext(NULSTHDR *hdr, NULNKHDR *lnk);

#endif
