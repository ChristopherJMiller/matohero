#ifndef NULIST_H
#define NULIST_H

typedef struct nulistlnk_s {
    struct nulistlnk_s *next;
    struct nulistlnk_s *prev;
} NULISTLNK;

typedef struct nulisthdr_s {
    NULISTLNK *head;
    NULISTLNK *tail;
} NULISTHDR;

void NuListAppend(NULISTHDR *hdr, NULISTLNK *newlnk);
void NuListInsert(NULISTHDR *hdr, NULISTLNK *newlnk);
void NuListRemove(NULISTHDR *hdr, NULISTLNK *lnk);
NULISTLNK *NuListGetHead(NULISTHDR *hdr);
NULISTLNK *NuListGetTail(NULISTHDR *hdr);
NULISTLNK *NuListGetNext(NULISTHDR *hdr, NULISTLNK *lnk);

#endif
