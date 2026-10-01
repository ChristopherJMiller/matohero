#ifndef LISTMAN_H
#define LISTMAN_H

#include "nutypes.h"

/* Pooled doubly linked lists: a fixed number of fixed-size elements allocated in one
 * block, each preceded by a link header. API functions take and return element pointers
 * (the memory just past the NULNKHDR). Names and layouts follow the LEGO Star Wars: The
 * Complete Saga decomp's nulst.h (https://github.com/opensagadev/saga). */

struct nulsthdr_s;

typedef struct nulnkhdr_s {
    struct nulsthdr_s *owner;
    struct nulnkhdr_s *next;
    struct nulnkhdr_s *prev;
    u16 id;
    u16 is_used : 1;
} NULNKHDR;

typedef struct nulsthdr_s {
    NULNKHDR *free;
    NULNKHDR *free_tail;
    NULNKHDR *head;
    NULNKHDR *tail;
    u16 element_count;
    u16 element_size;
    u16 element_size_total;
    u16 used_count;
    u32 safe_thread;
    u16 padding[2];
} NULSTHDR;

NULNKHDR *NuLstGetByIdx(NULSTHDR *list, i32 index);
NULNKHDR *NuLstGetNext(NULSTHDR *list, NULNKHDR *node);

#endif
