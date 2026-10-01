#include "listman.h"

#include "numem.h"
#include "nuthread.h"

/* Ported from opensagadev/saga src/nu2api/nucore/nulst.cpp (GPL-3.0). Most functions
 * match unchanged; NuLstCreateBuff is rewritten from BH's code, which predates saga's. */

// STRING: BIONICLE 0x5e07e8
// __FILE__, passed to NuMemAlloc: ".\\listman.c"

// FUNCTION: BIONICLE 0x5654c0
NULSTHDR *NuLstCreate(i32 element_count, i32 element_size) {
    NULSTHDR *list;
    i32 element_size_total;
    i32 i;
    NULNKHDR *curr;
    NULNKHDR *next;

    element_size_total = element_size + sizeof(NULNKHDR);
    list = (NULSTHDR *)NuMemAllocAt(element_count * element_size_total + sizeof(NULSTHDR), 97);

    if (list != NULL) {
        list->free = (NULNKHDR *)(list + 1);
        list->head = NULL;
        list->tail = NULL;

        list->element_count = element_count;
        list->element_size = element_size;
        list->element_size_total = element_size_total;
        list->used_count = 0;

        curr = list->free;
        next = (NULNKHDR *)((u8 *)curr + element_size_total);
        for (i = 1; i < element_count; i++) {
            curr->next = next;
            curr->id = i - 1;
            curr->owner = list;
            curr = next;
            next = (NULNKHDR *)((u8 *)next + element_size_total);
        }

        curr->next = NULL;
        list->free_tail = curr;
        curr->id = i - 1;
        curr->owner = list;
        list->safe_thread = nu_current_thread_id;
    }

    return list;
}

/* Effective match: the original loads nu_current_thread_id into edx rather than ecx.
 * Local types and declaration order don't change it. */
// FUNCTION: BIONICLE 0x565550
NULSTHDR *NuLstCreateBuff(i32 element_count, i32 element_size, VARIPTR *buffer, VARIPTR end, i32 alignment) {
    NULSTHDR *list;
    u32 element_size_total;
    u32 total;
    i32 i;
    NULNKHDR *curr;
    NULNKHDR *next;

    list = (NULSTHDR *)((buffer->addr + alignment - 1) & ~(alignment - 1));
    element_size_total = (element_size + sizeof(NULNKHDR) - 1 + alignment) & -alignment;
    total = element_count * element_size_total + sizeof(NULSTHDR);
    buffer->void_ptr = list;
    if (total < end.addr - (u32)list) {
        buffer->addr = (u32)list + total;

        list->free = (NULNKHDR *)(list + 1);
        list->head = NULL;
        list->tail = NULL;

        list->element_count = element_count;
        list->element_size = element_size;
        list->element_size_total = element_size_total;
        list->used_count = 0;

        curr = list->free;
        next = (NULNKHDR *)((u8 *)curr + element_size_total);
        for (i = 1; i < element_count; i++) {
            curr->next = next;
            curr->id = i - 1;
            curr->owner = list;
            curr = next;
            next = (NULNKHDR *)((u8 *)next + element_size_total);
        }

        curr->next = NULL;
        list->free_tail = curr;
        curr->id = i - 1;
        curr->owner = list;
        list->safe_thread = nu_current_thread_id;

        return list;
    }

    return NULL;
}

// FUNCTION: BIONICLE 0x565600
NULNKHDR *NuLstAllocHead(NULSTHDR *list) {
    u32 current_thread;
    NULNKHDR *node;

    if (list->free != NULL) {
        current_thread = nu_current_thread_id;

        if (list->safe_thread != current_thread) {
            (*NuThreadDisableThreadSwap)();
        }

        node = list->free;
        list->free = list->free->next;

        if (list->free == NULL) {
            list->free_tail = NULL;
        } else {
            list->free->prev = NULL;
        }

        node->next = list->head;

        if (list->head != NULL) {
            list->head->prev = node;
        } else {
            list->tail = node;
        }

        node->prev = NULL;

        list->head = node;
        list->used_count++;

        node->is_used = 1;

        if (list->safe_thread != current_thread) {
            (*NuThreadEnableThreadSwap)();
        }

        return node + 1;
    }

    return NULL;
}

// FUNCTION: BIONICLE 0x565680
NULNKHDR *NuLstAllocTail(NULSTHDR *list) {
    u32 current_thread;
    NULNKHDR *node;

    if (list->free != NULL) {
        current_thread = nu_current_thread_id;

        if (list->safe_thread != current_thread) {
            (*NuThreadDisableThreadSwap)();
        }

        node = list->free;
        list->free = list->free->next;

        if (list->free == NULL) {
            list->free_tail = NULL;
        } else {
            list->free->prev = NULL;
        }

        node->prev = list->tail;

        if (list->tail != NULL) {
            list->tail->next = node;
        } else {
            list->head = node;
        }

        node->next = NULL;

        list->tail = node;

        node->is_used = 1;
        list->used_count++;

        if (list->safe_thread != current_thread) {
            (*NuThreadEnableThreadSwap)();
        }

        return node + 1;
    }

    return NULL;
}

// FUNCTION: BIONICLE 0x565700
NULNKHDR *NuLstAllocFree(NULSTHDR *list) {
    u32 current_thread;
    NULNKHDR *node;
    if (list->free != NULL) {
        current_thread = nu_current_thread_id;
        if (list->safe_thread != current_thread)
            (*NuThreadDisableThreadSwap)();
        node = list->free;
        list->free = list->free->next;
        if (list->free == NULL)
            list->free_tail = NULL;
        else
            list->free->prev = NULL;
        node->prev = NULL;
        node->is_used = 1;
        ++list->used_count;
        if (list->safe_thread != current_thread)
            (*NuThreadEnableThreadSwap)();
        return node + 1;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x565760
void NuLstAttachTail(NULSTHDR *list, NULNKHDR *node) {
    u32 current_thread = nu_current_thread_id;
    if (list->safe_thread != current_thread)
        (*NuThreadDisableThreadSwap)();
    node->prev = list->tail;
    node->next = NULL;
    if (list->tail != NULL)
        list->tail->next = node;
    else
        list->head = node;
    list->tail = node;
    if (list->safe_thread != current_thread)
        (*NuThreadEnableThreadSwap)();
}

// FUNCTION: BIONICLE 0x5657b0
void NuLstFree(NULNKHDR *node) {
    u32 current_thread;
    NULSTHDR *list;

    node--;
    list = node->owner;

    current_thread = nu_current_thread_id;

    if (list->safe_thread != current_thread) {
        (*NuThreadDisableThreadSwap)();
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }

    node->prev = list->free_tail;
    node->next = NULL;

    if (list->free_tail != NULL) {
        list->free_tail->next = node;
    } else {
        list->free_tail = list->free = node;
    }

    list->free_tail = node;

    list->used_count--;

    node->is_used = 0;

    if (list->safe_thread != current_thread) {
        (*NuThreadEnableThreadSwap)();
    }
}

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

// FUNCTION: BIONICLE 0x565890
NULNKHDR *NuLstAlloc(NULSTHDR *list) {
    return NuLstAllocHead(list);
}
