#include "nufile.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* File system: devices and paths, buffered file handles, memory files and .dat
 * archives on top of the platform layer. Based on opensagadev/saga
 * src/nu2api/nufile/nufile.cpp (GPL-3.0); names and locals from the PS2 prototype.
 * Functions are in PC order; the rest are stubs until decompiled. */

// GLOBAL: BIONICLE 0x61722c
NUFILE_DEVICE *default_device = &host_device;

// GLOBAL: BIONICLE 0x34bec78
int DontDoFileUpperCaseHack;

// GLOBAL: BIONICLE 0x34bec7c
static NUDATHDR *curr_dat;

// GLOBAL: BIONICLE 0x34bec80
static int file_time_count;

// GLOBAL: BIONICLE 0x34bec98
int NuFile_SwapEndianOnWrite;

// GLOBAL: BIONICLE 0x34bac50
static FILEBUFF file_buff[4];

// GLOBAL: BIONICLE 0x3607b94
int numdevices;

// GLOBAL: BIONICLE 0x3607ba0
NUFILE_DEVICE devices[16];

// GLOBAL: BIONICLE 0x3609ee0
NUMEMFILE memfiles[20];

// GLOBAL: BIONICLE 0x360a080
NUDATFILE datfiles[20];

// GLOBAL: BIONICLE 0x617224
// STRING: BIONICLE 0x5e07f8
char *iop_image_name = "ioprp300.img";

// FUNCTION: BIONICLE 0x565c70
void NuFileUpCase(NUFILE_DEVICE *dev, char *name) {
    char sep;

    if (dev != NULL) {
        sep = dev->dirsep;
    } else {
        sep = '\\';
    }
    if (DontDoFileUpperCaseHack) {
        for (; *name != '\0'; name++) {
            if (*name == '\\' || *name == '/') {
                *name = sep;
            }
        }
    } else {
        for (; *name != '\0'; name++) {
            switch (*name) {
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
            case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
            case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
            case 'y': case 'z':
                *name -= 0x20;
                break;
            case '/':
            case '\\':
                *name = sep;
                break;
            }
        }
    }
}

// FUNCTION: BIONICLE 0x565d40
void NuFileReldirFix(NUFILE_DEVICE *dev, char *name) {
    char *src;
    char *dst;

    src = dst = name;
    while (*src != '\0') {
        if (*src == dev->dirsep && src[1] == '.' && src[2] == '.' && src[3] == dev->dirsep) {
            dst = src;
            while (dst > name) {
                dst--;
                if (*dst == dev->dirsep) {
                    src += 3;
                    break;
                }
                if (*dst == ':') {
                    dst++;
                    src += 4;
                    break;
                }
            }
        }
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0';
}

// FUNCTION: BIONICLE 0x565db0
NUFILE_DEVICE *NuFileGetDeviceFromPath(char *path) {
    NUFILE_DEVICE *dev;
    int i;

    dev = NULL;
    for (i = 0; i < 8; i++) {
        if (path[i] == ':') {
            break;
        }
    }
    if (i < 8) {
        for (i = 0; i < numdevices; i++) {
            if (NuStrNICmp(path, devices[i].name, devices[i].matchlen) == 0) {
                dev = &devices[i];
                break;
            }
        }
    }
    return dev;
}

// FUNCTION: BIONICLE 0x565e30
void NuFileGetCurrentDirectory(char *dir) {
    strcpy(dir, default_device->curdir);
}

// FUNCTION: BIONICLE 0x565e50
void NuFileSetCurrentDirectory(char *dir) {
    char sep[2] = "\\";
    int len;

    if (*dir != '\0') {
        len = NuStrCpy(default_device->curdir, dir);
        if (len != 0 && dir[len - 1] != '\\' && dir[len - 1] != '/') {
            sep[0] = default_device->dirsep;
            NuStrCat(default_device->curdir, sep);
        }
    } else {
        default_device->curdir[0] = '\0';
    }
}

// FUNCTION: BIONICLE 0x565ec0
void NuFileGetCurrentPath(char *dir) {
    // STRING: BIONICLE 0x5c86d9
    default_device->FormatName(default_device, dir, "", 0x7fff);
}

// FUNCTION: BIONICLE 0x565ee0
void NuFileSetCurrentSysDirectory(char *dir) {
    char *dst;
    char *src;

    if (*dir != '\0') {
        strcpy(default_device->sysdir, dir);
        // STRING: BIONICLE 0x5cf770
        NuStrCat(default_device->sysdir, "\\");
    } else {
        strcpy(default_device->sysdir, "sys");
        src = iop_image_name + 5;
        dst = default_device->sysdir + strlen(default_device->sysdir);
        while (*src != '.') {
            *dst++ = *src++;
        }
        *dst++ = '\\';
        *dst = '\0';
    }
    NuFileUpCase(default_device, default_device->sysdir);
}

// FUNCTION: BIONICLE 0x565f90
void NuFileSetCurrentDllDirectory(char *dir) {
    if (*dir != '\0') {
        strcpy(default_device->dlldir, dir);
        NuStrCat(default_device->dlldir, "\\");
    } else {
        default_device->dlldir[0] = '\0';
    }
}

// FUNCTION: BIONICLE 0x565fe0
int NuFileFormatName(char *fullname, char *fname, int buffsize) {
    return default_device->FormatName(default_device, fullname, fname, buffsize);
}

/* 0x566000; marker added once its callers (NuFileRead etc.) are in, since an unused
 * static is dropped. */
static void AquireFileBuffer(FILEINFO *info) {
    int i;
    int n;
    int time;
    FILEBUFF *buff;

    n = 0;
    time = file_time_count;
    for (i = 0; i < 4; i++) {
        if (file_buff[i].time < time) {
            time = file_buff[i].time;
            n = i;
        }
    }
    buff = &file_buff[n];
    if (buff->info != NULL) {
        buff->info->buff = NULL;
    }
    buff->time = file_time_count++;
    buff->info = info;
    info->buff = buff;
    if (info->buff_length != 0) {
        NuPSFileLSeek(info->fh, -info->buff_length, 1);
        NuPSFileRead(info->fh, buff->data, info->buff_length);
    }
}

// FUNCTION: BIONICLE 0x5660a0
NUFILE NuMemFileOpen(void *start, int buffsize, NUFILEMODE mode) {
    int i;

    if (buffsize > 0 && (mode == NUFILE_READ || mode == NUFILE_WRITE)) {
        for (i = 0; i < 20; i++) {
            if (!memfiles[i].used) {
                memfiles[i].start = (char *)start;
                memfiles[i].end = (char *)start + buffsize - 1;
                memfiles[i].curr = memfiles[i].start;
                memfiles[i].mode = mode;
                memfiles[i].used = 1;
                return i + 0x400;
            }
        }
    }
    return 0;
}

// FUNCTION: BIONICLE 0x566110
void *NuMemFileAddr(NUFILE fh) {
    return memfiles[fh - 0x400].curr;
}

// FUNCTION: BIONICLE 0x566130
int NuMemFileWrite(NUFILE fh, void *data, int size) {
    int left;

    fh -= 0x400;
    left = memfiles[fh].end - memfiles[fh].curr + 1;
    if (left < size) {
        size = left;
    }
    if (size != 0) {
        memcpy(memfiles[fh].curr, data, size);
        memfiles[fh].curr += size;
    }
    return size;
}

// FUNCTION: BIONICLE 0x566190
__inline int NuFileWrite(NUFILE fh, void *data, int size) {
    if (fh >= 0x1000) {
        return NuMcWrite(fh - 0x1000, data, size, 0);
    }
    if (fh >= 0x400) {
        return NuMemFileWrite(fh, data, size);
    }
    return NuPSFileWrite(fh - 1, data, size);
}

static __inline void NuFileEndianSwap32(void *data) {
    unsigned char *src;
    unsigned int swapped;
    unsigned char *dst;

    src = (unsigned char *)data;
    dst = (unsigned char *)&swapped;
    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];
    *(unsigned int *)data = swapped;
}

// FUNCTION: BIONICLE 0x5661d0
int NuFileWriteFloat(NUFILE fh, float data) {
    float tmp = data;

    if (NuFile_SwapEndianOnWrite) {
        NuFileEndianSwap32(&tmp);
    }
    return NuFileWrite(fh, &tmp, 4);
}

// FUNCTION: BIONICLE 0x566260
int NuFileWriteInt(NUFILE fh, int data) {
    int tmp = data;

    if (NuFile_SwapEndianOnWrite) {
        NuFileEndianSwap32(&tmp);
    }
    return NuFileWrite(fh, &tmp, 4);
}

// FUNCTION: BIONICLE 0x5662e0
int NuFileWriteChar(NUFILE fh, char data) {
    return NuFileWrite(fh, &data, 1);
}

// FUNCTION: BIONICLE 0x566330
int NuFileWriteString(NUFILE fh, char *str) {
    return NuFileWrite(fh, str, strlen(str));
}

// FUNCTION: BIONICLE 0x566390
int NuFileWriteStringV(NUFILE fh, char *fmt, ...) {
    char buff[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buff, fmt, args);
    va_end(args);
    return NuFileWrite(fh, buff, strlen(buff));
}

// FUNCTION: BIONICLE 0x566460
int NuDatFileSeek(NUFILE fh, int offset, int origin) {
    NUDATFILE *df = &datfiles[fh - 0x800];
    int pos;

    switch (origin) {
    case 1:
        pos = df->pos + offset;
        break;
    case 2:
        pos = df->len + df->start - offset;
        break;
    default:
        pos = df->start + offset;
        break;
    }
    df->ndh->pos = NuFileSeek(df->ndh->fh, pos, 0);
    df->pos = df->ndh->pos;
    return df->pos;
}

// FUNCTION: BIONICLE 0x5665a0
NUDATHDR *NuDatSet(NUDATHDR *dh) {
    NUDATHDR *old = curr_dat;

    curr_dat = dh;
    return old;
}

// FUNCTION: BIONICLE 0x5665b0
NUDATHDR *NuDatGet(void) {
    return curr_dat;
}
