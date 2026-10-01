#include "listman.h"

// FUNCTION: BIONICLE 0x565840
NULNKHDR *NuLstGetByIdx(NULSTHDR *list, i32 index) {
    NULNKHDR *node = (NULNKHDR *)((u8 *)(list + 1) + (i16)list->element_size_total * index);
    if (node->is_used) {
        return node + 1;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x565860
NULNKHDR *NuLstGetNext(NULSTHDR *list, NULNKHDR *node) {
    if (node != NULL) {
        --node;
        if (node->next != NULL) {
            return node->next + 1;
        }
    } else if (list->head != NULL) {
        return list->head + 1;
    }
    return NULL;
}
