#include "nufilepak.h"

#include "nufile.h"
#include "numem.h"
#include "nustring.h"

/* Pak files: load (with checksums and optional decryption), look up items, and squeeze
 * out items that aren't required. Based on opensagadev/saga
 * src/nu2api/nufile/nufilepak.cpp (GPL-3.0); names and locals from the PS2 prototype.
 * Not in the PC exe (unreferenced): NuFilePakGetLastError, NuFilePakRelocate,
 * NuFilePakGetSize. */

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// GLOBAL: BIONICLE 0x34c57d8
static NUFILEPAK_ERROR fpk_err;

// FUNCTION: BIONICLE 0x56b810
static unsigned int CalcChecksum(void *data, int size) {
    unsigned int checkval;
    unsigned char *c;

    checkval = 0x12345678;
    c = (unsigned char *)data;
    while (size != 0) {
        size--;
        checkval += *c++;
    }
    return checkval;
}

static __inline void DecryptData(NUFILEPAKHDR *hdr, unsigned int size, unsigned char *bkey, int keysize) {
    int ix;
    unsigned char *ptr;

    size -= sizeof(NUFILEPAKHDR);
    ix = 0;
    ptr = (unsigned char *)(hdr + 1);
    while (size != 0) {
        size--;
        *ptr -= bkey[ix++];
        if (ix >= keysize) {
            ix = 0;
        }
        ptr++;
    }
    hdr->flags &= ~1;
}

// FUNCTION: BIONICLE 0x56b830
void *NuFilePakLoadKey(char *fname, VARIPTR *ptr, VARIPTR end, int alignment, unsigned char *key, unsigned int keysize) {
    NUFILEPAKHDR *hdr;
    int size;
    unsigned int checksum;

    fpk_err = NUFILEPAK_ERROR_NONE;
    if (alignment > 0x2000) {
        fpk_err = NUFILEPAK_ERROR_ALIGNMENT;
        return NULL;
    }
    alignment = MAX(alignment, 4);
    ptr->intaddr = (ptr->intaddr + alignment - 1) & ~(alignment - 1);
    hdr = (NUFILEPAKHDR *)ptr->voidptr;

    size = NuFileLoadBufferVP(fname, ptr, &end);
    if (size == 0) {
        fpk_err = NUFILEPAK_ERROR_FILENOTFOUND;
        return NULL;
    }
    if (size != hdr->filesize) {
        fpk_err = NUFILEPAK_ERROR_ALIGNMENT;
        return NULL;
    }

    switch (hdr->type) {
    case 0x12345679:
        checksum = hdr->checksum;
        hdr->checksum = 0;
        if (checksum != CalcChecksum(hdr, size)) {
            fpk_err = NUFILEPAK_ERROR_CHECKSUM;
            return NULL;
        }
        break;
    case 0x1234567a:
        checksum = hdr->checksum;
        hdr->checksum = 0;
        if (checksum != CalcChecksum(hdr, size)) {
            fpk_err = NUFILEPAK_ERROR_CHECKSUM;
            return NULL;
        }
        if (hdr->flags & 1) {
            if (key == NULL) {
                fpk_err = NUFILEPAK_ERROR_KEYMISMATCH;
                return NULL;
            }
            DecryptData(hdr, size, key, keysize);
            checksum = hdr->encsum;
            hdr->encsum = 0;
            if (checksum != CalcChecksum(hdr, size)) {
                fpk_err = NUFILEPAK_ERROR_KEYMISMATCH;
                return NULL;
            }
        }
        break;
    case 0x12345678:
        break;
    default:
        fpk_err = NUFILEPAK_ERROR_UNSUPPORTEDTYPE;
        return NULL;
    }
    return hdr;
}

// FUNCTION: BIONICLE 0x56b9a0
void *NuFilePakLoad(char *fname, VARIPTR *ptr, VARIPTR end, int alignment) {
    return NuFilePakLoadKey(fname, ptr, end, alignment, NULL, 0);
}

/* The item table follows the header, whose size depends on the version. */
static __inline NUFILEPAKITEM *GetItems(NUFILEPAKHDR *hdr) {
    switch (hdr->type) {
    case 0x12345678:
        return (NUFILEPAKITEM *)((NUFILEPAKHDR_V0 *)hdr + 1);
    default:
        return (NUFILEPAKITEM *)((NUFILEPAKHDR_V1 *)hdr + 1);
    case 0x1234567a:
        return (NUFILEPAKITEM *)(hdr + 1);
    }
}

// FUNCTION: BIONICLE 0x56b9d0
int NuFilePakCondense(void *fph) {
    NUFILEPAKHDR *hdr;
    NUFILEPAKITEM *items;
    int dataoffset;
    unsigned int n;

    fpk_err = NUFILEPAK_ERROR_NONE;
    hdr = (NUFILEPAKHDR *)fph;
    items = GetItems(hdr);
    dataoffset = 0;
    for (n = 0; n < hdr->numitems; n++) {
        if (items[n].attrib.removed) {
            continue;
        }
        if (items[n].attrib.required) {
            if (dataoffset != 0 && items[n].align != 0) {
                dataoffset &= ~(items[n].align - 1);
            }
            if (dataoffset != 0) {
                NuMemMove((char *)fph + items[n].doffset - dataoffset, (char *)fph + items[n].doffset, items[n].size);
                items[n].doffset -= dataoffset;
            }
        } else {
            items[n].attrib.removed = 1;
            dataoffset += items[n].size;
            dataoffset = (dataoffset + items[n].align - 1) & ~(items[n].align - 1);
        }
    }
    hdr->filesize -= dataoffset;
    return dataoffset;
}

// FUNCTION: BIONICLE 0x56ba80
int NuFilePakGetItem(void *fph, char *iname) {
    NUFILEPAKHDR *hdr;
    NUFILEPAKITEM *items;
    unsigned int n;

    fpk_err = NUFILEPAK_ERROR_NONE;
    hdr = (NUFILEPAKHDR *)fph;
    items = GetItems(hdr);
    for (n = 0; n < hdr->numitems; n++) {
        if (NuStrICmp((char *)fph + items[n].noffset, iname) == 0) {
            if (items[n].attrib.removed) {
                return 0;
            }
            return n + 1;
        }
    }
    return 0;
}

// FUNCTION: BIONICLE 0x56bb00
int NuFilePakGetItemInfo(void *fph, int ihandle, void **addr, int *size) {
    NUFILEPAKHDR *hdr;
    NUFILEPAKITEM *item;

    fpk_err = NUFILEPAK_ERROR_NONE;
    hdr = (NUFILEPAKHDR *)fph;
    ihandle--;
    if (ihandle >= 0 && ihandle < hdr->numitems) {
        item = GetItems(hdr);
        item = &item[ihandle];
        if (!item->attrib.removed) {
            if (addr != NULL) {
                *addr = (char *)fph + item->doffset;
            }
            if (size != NULL) {
                *size = item->size;
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: BIONICLE 0x56bb70
int NuFilePakSetItemRequired(void *fph, int ihandle, int required) {
    NUFILEPAKHDR *hdr;
    NUFILEPAKITEM *item;

    fpk_err = NUFILEPAK_ERROR_NONE;
    hdr = (NUFILEPAKHDR *)fph;
    ihandle--;
    if (ihandle < 0 || ihandle >= hdr->numitems) {
        return 0;
    }
    item = GetItems(hdr);
    item = &item[ihandle];
    if (!item->attrib.removed) {
        item->attrib.required = required;
    }
    return 1;
}
