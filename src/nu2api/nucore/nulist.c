#include "nulist.h"

#include "nutypes.h"

/* Intrusive doubly linked lists. Based on opensagadev/saga src/nu2api/nucore/nulist.cpp
 * (GPL-3.0); names from the PS2 prototype. NuListCheck, NuListInsertBefore/After and
 * NuListGetPrev aren't in the PC exe. */

// FUNCTION: BIONICLE 0x56bbd0
void NuListAppend(NULISTHDR *hdr, NULISTLNK *newlnk) {
    newlnk->next = NULL;
    newlnk->prev = hdr->tail;
    if (hdr->tail != NULL) {
        hdr->tail->next = newlnk;
    }
    hdr->tail = newlnk;
    if (hdr->head == NULL) {
        hdr->head = newlnk;
    }
}

// FUNCTION: BIONICLE 0x56bc00
void NuListInsert(NULISTHDR *hdr, NULISTLNK *newlnk) {
    newlnk->next = hdr->head;
    newlnk->prev = NULL;
    if (hdr->head != NULL) {
        hdr->head->prev = newlnk;
    }
    hdr->head = newlnk;
    if (hdr->tail == NULL) {
        hdr->tail = newlnk;
    }
}

// FUNCTION: BIONICLE 0x56bc30
void NuListRemove(NULISTHDR *hdr, NULISTLNK *lnk) {
    if (lnk->next != NULL) {
        lnk->next->prev = lnk->prev;
    } else {
        hdr->tail = lnk->prev;
    }
    if (lnk->prev != NULL) {
        lnk->prev->next = lnk->next;
    } else {
        hdr->head = lnk->next;
    }
}

// FUNCTION: BIONICLE 0x56bc60
NULISTLNK *NuListGetHead(NULISTHDR *hdr) {
    return hdr->head;
}

// FUNCTION: BIONICLE 0x56bc70
NULISTLNK *NuListGetTail(NULISTHDR *hdr) {
    return hdr->tail;
}

// FUNCTION: BIONICLE 0x56bc80
NULISTLNK *NuListGetNext(NULISTHDR *hdr, NULISTLNK *lnk) {
    if (lnk != NULL) {
        return lnk->next;
    }
    return hdr->head;
}
