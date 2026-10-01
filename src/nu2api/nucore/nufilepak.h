#ifndef NUFILEPAK_H
#define NUFILEPAK_H

#include "nutypes.h"

/* Pak files: a header, an item table, then the items' data, all loaded as one block. */
typedef struct nufilepakhdrv0_s {
    unsigned int type;
    unsigned int numitems;
    unsigned int filesize;
} NUFILEPAKHDR_V0;

typedef struct nufilepakhdrv1_s {
    unsigned int type;
    unsigned int numitems;
    unsigned int filesize;
    unsigned int checksum;
    unsigned int flags;
} NUFILEPAKHDR_V1;

typedef struct nufilepakhdrv2_s {
    unsigned int type;
    unsigned int numitems;
    unsigned int filesize;
    unsigned int checksum;
    unsigned int flags;
    unsigned int encsum;
} NUFILEPAKHDR;

typedef struct nufilepakitemattr_s {
    unsigned int required : 1;
    unsigned int removed : 1;
} NUFILEPAKITEMATTR;

typedef int NuFilePakMemMoveFn();

typedef struct nufilepakitem_s {
    int noffset;
    int doffset;
    int size;
    int align;
    NUFILEPAKITEMATTR attrib;
    NuFilePakMemMoveFn *PreMove;
    NuFilePakMemMoveFn *PostMove;
} NUFILEPAKITEM;

typedef enum {
    NUFILEPAK_ERROR_NONE = 0,
    NUFILEPAK_ERROR_FILENOTFOUND = 1,
    NUFILEPAK_ERROR_FILEIO = 2,
    NUFILEPAK_ERROR_CHECKSUM = 3,
    NUFILEPAK_ERROR_UNSUPPORTEDTYPE = 4,
    NUFILEPAK_ERROR_ALIGNMENT = 5,
    NUFILEPAK_ERROR_KEYMISMATCH = 6,
    NUFILEPAK_ERROR_CNT = 7
} NUFILEPAK_ERROR;

void *NuFilePakLoadKey(char *fname, VARIPTR *ptr, VARIPTR end, int alignment, unsigned char *key, unsigned int keysize);
void *NuFilePakLoad(char *fname, VARIPTR *ptr, VARIPTR end, int alignment);
int NuFilePakCondense(void *fph);
int NuFilePakGetItem(void *fph, char *iname);
int NuFilePakGetItemInfo(void *fph, int ihandle, void **addr, int *size);
int NuFilePakSetItemRequired(void *fph, int ihandle, int required);

#endif
