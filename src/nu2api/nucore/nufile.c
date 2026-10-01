#include "nufile.h"
#include "numem.h"
#include "nuthread.h"

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

// GLOBAL: BIONICLE 0x34b7324
static int file_criticalsection;

// GLOBAL: BIONICLE 0x34b7328
static FILEINFO file_info[33];

// GLOBAL: BIONICLE 0x34b7320
static int num_extensions;

// GLOBAL: BIONICLE 0x34b7850
static FILEEXTINFO extensions[64];

// GLOBAL: BIONICLE 0x61723c
static int ext_init_needed = 1;

// GLOBAL: BIONICLE 0x34b7c50
static BLKINFO nufile_blkinfo[1024];

// GLOBAL: BIONICLE 0x34bec84
int nufile_buffering_enabled;

// GLOBAL: BIONICLE 0x34bec88
int nufile_try_packed;

// GLOBAL: BIONICLE 0x34bec9c
int nufileloadbuffer_error;

// GLOBAL: BIONICLE 0x34becb0
int datsys_offset;

// GLOBAL: BIONICLE 0x617218
static int blkcnt = 1;

// GLOBAL: BIONICLE 0x617228
int nufile_lsn_allowed = 1;

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

// FUNCTION: BIONICLE 0x566000
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
__inline int NuMemFileWrite(NUFILE fh, void *data, int size) {
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
__inline int NuDatFileSeek(NUFILE fh, int offset, int origin) {
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

// FUNCTION: BIONICLE 0x5664c0
int NuDatFileFindTree(NUDATHDR *ndh, char *fname) {
    char *slash;
    int pix;
    char name[256];
    char *n;

    if (*fname == '@') {
        fname += 4;
    }
    NuStrCpy(name, fname);
    NuFileUpCase(NULL, name);
    pix = ndh->filetree[0].childix;
    n = name;
    slash = NuStrStr(n, "\\");
    if (slash != NULL) {
        *slash = '\0';
    }
    do {
        if (NuStrICmp(ndh->filetree[pix].txt, n) == 0) {
            if (slash == NULL) {
                if (ndh->filetree[pix].childix > 0) {
                    return -1;
                }
                return -ndh->filetree[pix].childix;
            }
            pix = ndh->filetree[pix].childix;
            n = slash + 1;
            slash = NuStrStr(n, "\\");
            if (slash != NULL) {
                *slash = '\0';
            }
        } else {
            pix = ndh->filetree[pix].sibix;
        }
    } while (pix != 0);
    return -1;
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

// FUNCTION: BIONICLE 0x5665c0
static void NuFileExtAdd(char *ext, char type, char platform) {
    FILEEXTINFO *di;
    char *c;
    int len;

    di = &extensions[num_extensions];
    len = NuStrLen(ext);
    di->len = len;
    c = di->extension;
    while (len != 0) {
        *c++ = ext[--len];
    }
    *c = '\0';
    di->type = type;
    di->platform = platform;
    num_extensions++;
}

/* Extensions per NUFILETYPE (anim, bsanim, character, scene, texture, cutscene, sfx,
 * soundstream) and platform; lookups convert to the platform-4 set. */
// FUNCTION: BIONICLE 0x566610
void NuFileExtInit(void) {
    if (ext_init_needed) {
        memset(extensions, 0, sizeof(extensions));
        ext_init_needed = 0;
        num_extensions = 0;
        // STRING: BIONICLE 0x5e0908
        NuFileExtAdd(".ca3", 0, 6);
        // STRING: BIONICLE 0x5e0900
        NuFileExtAdd(".cbs", 1, 6);
        // STRING: BIONICLE 0x5e08f4
        NuFileExtAdd("_360.ghg", 2, 6);
        // STRING: BIONICLE 0x5e08e8
        NuFileExtAdd("_360.gsc", 3, 6);
        // STRING: BIONICLE 0x5e08dc
        NuFileExtAdd("_360.tex", 4, 6);
        // STRING: BIONICLE 0x5e08d4
        NuFileExtAdd(".cc2", 5, 6);
        // STRING: BIONICLE 0x5e08cc
        NuFileExtAdd(".wavx", 6, 6);
        // STRING: BIONICLE 0x5e08c4
        NuFileExtAdd(".wavm", 7, 6);

        // STRING: BIONICLE 0x5e08bc
        NuFileExtAdd(".an3", 0, 0);
        // STRING: BIONICLE 0x5c940c
        NuFileExtAdd(".bsa", 1, 0);
        // STRING: BIONICLE 0x5c90d4
        NuFileExtAdd(".ghg", 2, 0);
        // STRING: BIONICLE 0x5c90dc
        NuFileExtAdd(".gsc", 3, 0);
        // STRING: BIONICLE 0x5cc8e0
        NuFileExtAdd(".pnt", 4, 0);
        // STRING: BIONICLE 0x5c9404
        NuFileExtAdd(".cu2", 5, 0);
        // STRING: BIONICLE 0x5e08b4
        NuFileExtAdd(".vag", 6, 0);
        // STRING: BIONICLE 0x5e08ac
        NuFileExtAdd(".mib", 7, 0);

        NuFileExtAdd(".ca3", 0, 2);
        NuFileExtAdd(".cbs", 1, 2);
        // STRING: BIONICLE 0x5e08a4
        NuFileExtAdd(".chg", 2, 2);
        // STRING: BIONICLE 0x5e089c
        NuFileExtAdd(".csc", 3, 2);
        // STRING: BIONICLE 0x5e0894
        NuFileExtAdd(".ctx", 4, 2);
        NuFileExtAdd(".cc2", 5, 2);
        // STRING: BIONICLE 0x5e088c
        NuFileExtAdd(".dsp", 6, 2);
        // STRING: BIONICLE 0x5e0884
        NuFileExtAdd(".gcm", 7, 2);

        NuFileExtAdd(".an3", 0, 1);
        NuFileExtAdd(".bsa", 1, 1);
        // STRING: BIONICLE 0x5e087c
        NuFileExtAdd(".hx2", 2, 1);
        // STRING: BIONICLE 0x5e0874
        NuFileExtAdd(".nx2", 3, 1);
        // STRING: BIONICLE 0x5e086c
        NuFileExtAdd(".dds", 4, 1);
        NuFileExtAdd(".cu2", 5, 1);
        NuFileExtAdd(".wavx", 6, 1);
        NuFileExtAdd(".wavm", 7, 1);

        NuFileExtAdd(".ca3", 0, 5);
        NuFileExtAdd(".cbs", 1, 5);
        // STRING: BIONICLE 0x5e0860
        NuFileExtAdd("_ps3.ghg", 2, 5);
        // STRING: BIONICLE 0x5e0854
        NuFileExtAdd("_ps3.gsc", 3, 5);
        // STRING: BIONICLE 0x5e0848
        NuFileExtAdd(".ps3.tex", 4, 5);
        NuFileExtAdd(".cc2", 5, 5);
        NuFileExtAdd(".vag", 6, 5);
        NuFileExtAdd(".mib", 7, 5);

        NuFileExtAdd(".an3", 0, 4);
        NuFileExtAdd(".bsa", 1, 4);
        // STRING: BIONICLE 0x5e0840
        NuFileExtAdd(".hgp", 2, 4);
        // STRING: BIONICLE 0x5e0838
        NuFileExtAdd(".nup", 3, 4);
        NuFileExtAdd(".dds", 4, 4);
        NuFileExtAdd(".cu2", 5, 4);
        // STRING: BIONICLE 0x5e0830
        NuFileExtAdd(".wav", 6, 4);
        NuFileExtAdd(".mib", 7, 4);

        NuFileExtAdd(".an3", 0, 3);
        NuFileExtAdd(".bsa", 1, 3);
        // STRING: BIONICLE 0x5e0828
        NuFileExtAdd(".phg", 2, 3);
        // STRING: BIONICLE 0x5e0820
        NuFileExtAdd(".psc", 3, 3);
        NuFileExtAdd(".pnt", 4, 3);
        NuFileExtAdd(".cu2", 5, 3);
        NuFileExtAdd(".vag", 6, 3);
        // STRING: BIONICLE 0x5e0818
        NuFileExtAdd(".at3", 7, 3);
    }
}

// FUNCTION: BIONICLE 0x566970
static FILEEXTINFO *NuFileExtFind(char *in, int len) {
    char *endptr;
    FILEEXTINFO *si;
    int flen;
    char *fext;
    char *ext;

    if (len == 0) {
        len = NuStrLen(in);
    }
    endptr = in + len;
    for (si = extensions; si != NULL; si++) {
        fext = si->extension;
        flen = len;
        ext = endptr;
        while (*fext != '\0') {
            ext--;
            if (flen == 0 || *fext != (char)NuToLower(*ext)) {
                break;
            }
            fext++;
            flen--;
        }
        if (*fext == '\0') {
            return si;
        }
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x5669f0
int NuFileExtConvert(char *out, char *in, int outsize) {
    char *c;
    char *d;
    FILEEXTINFO *di;
    FILEEXTINFO *si;
    int len;
    int type;

    len = NuStrCpy(out, in);
    si = NuFileExtFind(in, len);
    if (si != NULL) {
        type = si->type;
        if (si->platform == 4) {
            return 1;
        }
        for (di = extensions; di != NULL; di++) {
            if (di->platform == 4 && di->type == type) {
                d = out + len + (di->len - si->len);
                *d = '\0';
                for (c = di->extension; *c != '\0'; c++) {
                    *--d = *c;
                }
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: BIONICLE 0x566a70
int NuFileExtGetExt(char *out, int outsize, int type) {
    FILEEXTINFO *di;
    int n;
    char *c;

    for (di = extensions; di != NULL; di++) {
        if (di->platform == 4 && di->type == type) {
            if (outsize < di->len) {
                return 0;
            }
            c = di->extension + di->len;
            for (n = 0; n < di->len; n++) {
                out[n] = *--c;
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: BIONICLE 0x566ad0
int NuFileExtRemove(char *out, char *in, int outsize) {
    int len;
    FILEEXTINFO *si;

    len = NuStrCpy(out, in);
    si = NuFileExtFind(in, len);
    if (si != NULL) {
        out[len - si->len] = '\0';
        return si->type;
    }
    return -1;
}

// FUNCTION: BIONICLE 0x566b10
int NuFileInitEx(int deviceid, int rebootiop, int eject) {
    NuPSFileInitDevices(deviceid, rebootiop, eject);
    memset(memfiles, 0, sizeof(memfiles));
    memset(datfiles, 0, sizeof(datfiles));
    file_criticalsection = NuThreadCreateCriticalSection();
    NuFileExtInit();
    return deviceid;
}

// FUNCTION: BIONICLE 0x566b60
int NuFileOpenSize(NUFILE fh) {
    if (fh >= 0x1000) {
        return NuMcOpenSize(fh - 0x1000);
    }
    if (fh >= 0x800) {
        return datfiles[fh - 0x800].len;
    }
    if (fh >= 0x400) {
        return memfiles[fh - 0x400].end - memfiles[fh - 0x400].start;
    }
    return file_info[fh - 1].file_length;
}

// FUNCTION: BIONICLE 0x566bc0
void NuMemFileClose(NUFILE fh) {
    if (fh >= 0x800) {
        datfiles[fh - 0x800].ndh->dfhandle = 0;
        datfiles[fh - 0x800].used = 0;
    } else {
        memfiles[fh - 0x400].used = 0;
    }
}

// FUNCTION: BIONICLE 0x566c00
int NuMemFilePos(NUFILE fh) {
    NUDATFILE *info;

    if (fh >= 0x800) {
        info = &datfiles[fh - 0x800];
        return info->pos - info->start;
    }
    fh -= 0x400;
    return memfiles[fh].curr - memfiles[fh].start;
}

// FUNCTION: BIONICLE 0x566c40
__inline int NuMemFileSeek(NUFILE fh, int offset, int origin) {
    if (fh >= 0x800) {
        return NuDatFileSeek(fh, offset, origin);
    }
    fh -= 0x400;
    switch (origin) {
    case 1:
        memfiles[fh].curr += offset;
        break;
    case 2:
        memfiles[fh].curr = memfiles[fh].end - offset;
        break;
    default:
        memfiles[fh].curr = memfiles[fh].start + offset;
        break;
    }
    return memfiles[fh].curr - memfiles[fh].start;
}

// FUNCTION: BIONICLE 0x566cc0
int NuFilePos(NUFILE fh) {
    FILEINFO *info;
    int rv;

    if (fh >= 0x1000) {
        return NuMcSeek(fh - 0x1000, 0, 2, 0);
    }
    if (fh >= 0x400) {
        return NuMemFilePos(fh);
    }
    fh--;
    info = &file_info[fh];
    if (info->use_buff) {
        return info->read_pos;
    }
    rv = NuPSFileLSeek(fh, 0, 1);
    while (rv < 0) {
        rv = NuPSFileLSeek(fh, 0, 1);
    }
    return rv;
}

// FUNCTION: BIONICLE 0x566d40
__inline int NuFileSeek(NUFILE fh, int offset, int origin) {
    FILEINFO *info;

    if (fh >= 0x1000) {
        return NuMcSeek(fh - 0x1000, offset, origin, 0);
    }
    if (fh >= 0x400) {
        return NuMemFileSeek(fh, offset, origin);
    }
    fh--;
    info = &file_info[fh];
    if (info->use_buff) {
        switch (origin) {
        case 1:
            info->read_pos += offset;
            return info->read_pos;
        case 2:
            info->read_pos = info->file_length - offset;
            return info->read_pos;
        default:
            info->read_pos = offset;
            return offset;
        }
    }
    return NuPSFileLSeek(fh, offset, origin);
}

// FUNCTION: BIONICLE 0x566dd0
void NuFileEndBlkRead(NUFILE fh) {
    int pos;

    blkcnt--;
    pos = NuFilePos(fh);
    if (pos != nufile_blkinfo[blkcnt].pos + nufile_blkinfo[blkcnt].hdr.size) {
        NuFileSeek(fh, nufile_blkinfo[blkcnt].pos + nufile_blkinfo[blkcnt].hdr.size, 0);
    }
}

// FUNCTION: BIONICLE 0x566ec0
int NuFileAlign(NUFILE fh, int mask) {
    unsigned char pad;
    int pos;
    int cnt;

    pad = 0xfe;
    cnt = 0;
    pos = NuFilePos(fh);
    while (pos & mask) {
        pos++;
        cnt++;
        NuFileWrite(fh, &pad, 1);
    }
    return cnt;
}

// FUNCTION: BIONICLE 0x566fb0
int NuDatGetFileInfo(NUDATHDR *ndh, char *fname, int *start, int *len) {
    int ix;

    if (ndh != NULL) {
        ix = NuDatFileFindTree(ndh, fname);
        if (ix >= 0) {
            if (nufile_lsn_allowed && ndh->start_lsn) {
                if (start != NULL) {
                    *start = ndh->finfo[ix].foffset / 2048 + ndh->start_lsn;
                }
            } else {
                if (start != NULL) {
                    *start = ndh->finfo[ix].foffset;
                }
            }
            if (len != NULL) {
                *len = ndh->finfo[ix].flen;
            }
            return ix;
        }
    }
    return -1;
}

// FUNCTION: BIONICLE 0x567050
int NuDatFileLoadBufferLSN(NUDATHDR *ndh, char *fname, void *mem, int buffsize) {
    int ix;
    int flen;
    int uplen;
    int ofs;

    if (nufile_lsn_allowed && ndh->start_lsn && buffsize >= 0x800 && ndh->dfhandle == 0) {
        ix = NuDatFileFindTree(ndh, fname);
        if (ix >= 0) {
            if (ndh->finfo[ix].ppack) {
                flen = ndh->finfo[ix].flen;
                uplen = ndh->finfo[ix].uplen;
                ofs = uplen - flen + 0x100;
                ofs += -(ofs + (int)mem) & 0x3f;
                if (((flen + 0x7ff) & ~0x7ff) + ofs > buffsize) {
                    return -1;
                }
                NuPPUnpack((char *)mem + ofs, (char *)mem);
                return uplen;
            }
            if (((ndh->finfo[ix].flen + 0x7ff) & ~0x7ff) > buffsize) {
                return -1;
            }
            return ndh->finfo[ix].flen;
        }
    }
    return 0;
}

// FUNCTION: BIONICLE 0x567120
int NuDatFileRead(NUFILE fh, void *data, int size) {
    NUDATFILE *info;
    int bytes;

    NuThreadCriticalSectionBegin(file_criticalsection);
    info = &datfiles[fh - 0x800];
    if (info->pos != info->ndh->pos) {
        NuFileSeek(info->ndh->fh, info->pos, 0);
    }
    bytes = info->len + info->start - info->pos;
    size = size < (0 > bytes ? 0 : bytes) ? size : (0 > bytes ? 0 : bytes);
    if (size != 0) {
        bytes = NuFileRead(info->ndh->fh, data, size);
        if (bytes >= 0) {
            info->pos += bytes;
        }
        NuThreadCriticalSectionEnd(file_criticalsection);
        return bytes;
    }
    NuThreadCriticalSectionEnd(file_criticalsection);
    return 0;
}

// FUNCTION: BIONICLE 0x567270
NUFILE NuDatFileOpen(NUDATHDR *ndh, char *fname, NUFILEMODE mode) {
    int n;
    int ix;
    int f;
    NUFILE fh;

    if (mode == NUFILE_READ && ndh->dfhandle == 0) {
        NuThreadCriticalSectionBegin(file_criticalsection);
        f = -1;
        for (n = 0; n < 20; n++) {
            if (!datfiles[n].used) {
                datfiles[n].used = 1;
                f = n;
                break;
            }
        }
        NuThreadCriticalSectionEnd(file_criticalsection);
        if (f == -1 || (ix = NuDatFileFindTree(ndh, fname)) < 0) {
            datfiles[f].used = 0;
        } else {
            datfiles[f].ndh = ndh;
            datfiles[f].start = datfiles[f].pos = ndh->finfo[ix].foffset;
            datfiles[f].len = ndh->finfo[ix].flen;
            if (datfiles[f].len == 0) {
                datfiles[f].used = 0;
                return 0;
            }
            datfiles[f].uplen = ndh->finfo[ix].uplen;
            datfiles[f].ppack = ndh->finfo[ix].ppack;
            while (NuFileSeek(ndh->fh, ndh->finfo[ix].foffset, 0) < 0) {
            }
            ndh->pos = datfiles[f].pos;
            fh = f + 0x800;
            ndh->dfhandle = fh;
            return fh;
        }
    }
    return 0;
}

// FUNCTION: BIONICLE 0x567470
NUFILE NuFileOpen(char *file, NUFILEMODE mode) {
    NUFILE_DEVICE *device;
    int fp;
    char lfile[256];
    int rv;
    int len;

    device = NuFileGetDeviceFromPath(file);
    if (device == NULL) {
        if (curr_dat != NULL && mode != NUFILE_WRITE && mode != NUFILE_APPEND) {
            return NuDatFileOpen(curr_dat, file, mode);
        }
        device = default_device;
    }
    device->FormatName(device, lfile, file, 256);
    if (device->id == 2) {
        // STRING: BIONICLE 0x5e0910
        strcat(lfile, ";1");
    }
    if (device->id == 1) {
        rv = NuMcOpen(lfile[2] - '0', lfile[3] - '0', lfile + device->namelen, mode, 0);
        if (rv < 0) {
            return 0;
        }
        return rv + 0x1000;
    }
    fp = NuPSFileOpen(lfile, mode);
    if (fp < 0) {
        return 0;
    }
    memset(&file_info[fp], 0, sizeof(FILEINFO));
    file_info[fp].fh = fp;
    file_info[fp].open_mode = mode;
    if (mode == NUFILE_WRITE) {
        file_info[fp].file_length = 0;
    } else {
        if (fp <= 0x10) {
            while ((len = NuFileSeek(fp + 1, 0, 2)) < 0) {
            }
            file_info[fp].file_length = len;
            while (NuFileSeek(fp + 1, 0, 0) < 0) {
            }
        }
        if ((fp > 0x10 || mode != 3) && mode == NUFILE_READ) {
            file_info[fp].use_buff = nufile_buffering_enabled;
        }
    }
    return fp + 1;
}

// FUNCTION: BIONICLE 0x5677a0
void NuFileClose(NUFILE fh) {
    FILEINFO *info;

    if (fh >= 0x1000) {
        NuMcClose(fh - 0x1000, 0);
        return;
    }
    if (fh >= 0x400) {
        NuMemFileClose(fh);
        return;
    }
    fh--;
    while (NuPSFileClose(fh) < 0) {
    }
    info = &file_info[fh];
    if (info->buff != NULL) {
        info->buff->info = NULL;
    }
    memset(info, 0, sizeof(FILEINFO));
}

// FUNCTION: BIONICLE 0x567830
int NuMemFileRead(NUFILE fh, void *data, int size) {
    int left;

    if (fh >= 0x800) {
        return NuDatFileRead(fh, data, size);
    }
    fh -= 0x400;
    left = memfiles[fh].end - memfiles[fh].curr + 1;
    if (left < size) {
        size = left;
    }
    if (size > 0) {
        memcpy(data, memfiles[fh].curr, size);
        memfiles[fh].curr += size;
    }
    return size;
}

// FUNCTION: BIONICLE 0x5678a0
int NuFileSize(char *fname) {
    NUFILE fh;
    int rv;
    int be;
    int ix;

    be = nufile_buffering_enabled;
    nufile_buffering_enabled = 0;
    if (curr_dat != NULL) {
        ix = NuDatFileFindTree(curr_dat, fname);
        if (ix != 0) {
            return curr_dat->finfo[ix].uplen;
        }
    }
    rv = -1;
    if (fname != NULL && *fname != '\0') {
        fh = NuFileOpen(fname, NUFILE_READ);
        if (fh != 0) {
            while (NuFileSeek(fh, 0, 2) < 0) {
            }
            rv = NuFilePos(fh);
            NuFileClose(fh);
        }
    }
    nufile_buffering_enabled = be;
    return rv;
}

// FUNCTION: BIONICLE 0x5679f0
int NuFileRead(NUFILE fh, void *data, int size) {
    FILEINFO *info;
    int bytes;
    int blocksize;
    char *datac;

    if (fh >= 0x1000) {
        return NuMcRead(fh - 0x1000, data, size, 0);
    }
    if (fh >= 0x400) {
        return NuMemFileRead(fh, data, size);
    }
    fh--;
    info = &file_info[fh];
    if (!info->use_buff) {
        return NuPSFileRead(fh, data, size);
    }
    if (info->buff == NULL) {
        AquireFileBuffer(info);
    }
    datac = (char *)data;
    bytes = 0;
    while (size > 0 && info->read_pos < info->file_length) {
        if (info->read_pos < info->buff_start || info->read_pos >= info->buff_start + info->buff_length) {
            if (info->read_pos != info->file_pos) {
                NuPSFileLSeek(info->fh, info->read_pos, 0);
                info->file_pos = info->read_pos;
            }
            blocksize = NuPSFileRead(info->fh, info->buff->data, 0x400);
            info->buff_start = info->file_pos;
            info->buff_length = blocksize;
            info->file_pos += blocksize;
        }
        blocksize = info->buff_length - info->read_pos + info->buff_start;
        if (blocksize >= size) {
            blocksize = size;
        }
        if (blocksize != 0) {
            memcpy(datac, info->buff->data + info->read_pos - info->buff_start, blocksize);
        }
        datac += blocksize;
        bytes += blocksize;
        size -= blocksize;
        info->read_pos += blocksize;
    }
    return bytes;
}

// FUNCTION: BIONICLE 0x567b40
float NuFileReadFloat(NUFILE fh) {
    float data;

    NuFileRead(fh, &data, 4);
    return data;
}

// FUNCTION: BIONICLE 0x567b60
int NuFileReadInt(NUFILE fh) {
    int data;

    NuFileRead(fh, &data, 4);
    return data;
}

// FUNCTION: BIONICLE 0x567b80
short NuFileReadShort(NUFILE fh) {
    short data;

    NuFileRead(fh, &data, 2);
    return data;
}

// FUNCTION: BIONICLE 0x567ba0
char NuFileReadChar(NUFILE fh) {
    char data = 0;

    NuFileRead(fh, &data, 1);
    return data;
}

// FUNCTION: BIONICLE 0x567bc0
NUWCHAR NuFileReadWChar(NUFILE fh) {
    NUWCHAR data = 0;

    NuFileRead(fh, &data, 2);
    return data;
}

// FUNCTION: BIONICLE 0x567bf0
int NuFileBeginBlkRead(NUFILE fh, int blk) {
    int i;
    int pos;

    i = blkcnt++;
    pos = NuFilePos(fh);
    if (i > 1 && pos >= nufile_blkinfo[i - 1].pos + nufile_blkinfo[i - 1].hdr.size) {
        blkcnt--;
        return 0;
    }
    nufile_blkinfo[i].pos = NuFilePos(fh);
    NuFileRead(fh, &nufile_blkinfo[i].hdr, sizeof(NUIFFHDR));
    if (nufile_blkinfo[i].hdr.size < 0) {
        nufile_blkinfo[i].hdr.size = -nufile_blkinfo[i].hdr.size;
    }
    return nufile_blkinfo[i].hdr.blk;
}

// FUNCTION: BIONICLE 0x567f40
NUDATHDR *NuDatOpenEx(char *fname, VARIPTR *buff, int *req, short openmode) {
    NUDATHDR *ndh;
    NUFILE fh;
    int prehdr[2];
    VARIPTR hdr;
    int n;
    int size;
    int be;
    char lfile[256];
    int namesize;

    be = nufile_buffering_enabled;
    fh = NuFileOpen(fname, openmode);
    if (fh != 0) {
        if (datsys_offset) {
            NuFileSeek(fh, datsys_offset, 0);
        }
        while (NuFileRead(fh, prehdr, 8) < 0) {
            while (NuFileSeek(fh, datsys_offset, 0) < 0) {
            }
        }
        prehdr[0] += datsys_offset;
        while (NuFileSeek(fh, prehdr[0], 0) < 0) {
        }
        default_device->FormatName(default_device, lfile, fname, 256);
        namesize = (NuStrLen(lfile) + 0x10) & ~0xf;
        size = prehdr[1] + namesize + sizeof(NUDATHDR);
        if (buff != NULL) {
            ndh = (NUDATHDR *)buff->voidptr;
            buff->intaddr = (unsigned int)ndh + size;
            if (req != NULL) {
                *req = size;
            }
            ndh->intalloc = 0;
        } else {
            ndh = (NUDATHDR *)NuMemAllocAt(size, 3889);
            ndh->intalloc = 1;
        }
        ndh->name = (char *)(ndh + 1);
        hdr.intaddr = (unsigned int)(ndh + 1) + namesize;
        NuStrCpy(ndh->name, lfile);
        while (NuFileRead(fh, hdr.voidptr, prehdr[1]) < 0) {
            while (NuFileSeek(fh, prehdr[0], 0) < 0) {
            }
        }
        ndh->ver = hdr.s32[0];
        ndh->nfiles = hdr.s32[1];
        hdr.s32 += 2;
        ndh->finfo = (NUDATFINFO *)hdr.voidptr;
        hdr.intaddr += ndh->nfiles * sizeof(NUDATFINFO);
        ndh->treecnt = *hdr.s32++;
        ndh->filetree = (NUDFNODE *)hdr.voidptr;
        hdr.intaddr += ndh->treecnt * sizeof(NUDFNODE);
        ndh->leafnamesize = *hdr.s32++;
        ndh->leafnames = hdr.s8;
        for (n = 0; n < ndh->treecnt; n++) {
            ndh->filetree[n].txt += (int)hdr.s8;
        }
        ndh->filetree[0].txt = NULL;
        ndh->dfhandle = 0;
        ndh->fh = fh;
        ndh->openmode = openmode;
        ndh->pos = 0;
        if (datsys_offset) {
            for (n = 0; n < ndh->nfiles; n++) {
                ndh->finfo[n].foffset += datsys_offset;
            }
        }
        if (nufile_lsn_allowed && default_device->id == 2) {
            strcpy(lfile, "\\");
            strcat(lfile, default_device->curdir);
            strcat(lfile, fname);
            strcat(lfile, ";1");
        }
        ndh->start_lsn = 0;
        nufile_buffering_enabled = be;
        return ndh;
    }
    nufile_buffering_enabled = be;
    return NULL;
}

// FUNCTION: BIONICLE 0x568500
NUDATHDR *NuDatOpen(char *fname, VARIPTR *buff, int *req) {
    return NuDatOpenEx(fname, buff, req, 0);
}

// STRING: BIONICLE 0x5e080c
// __FILE__, passed to NuMemAlloc: ".\\nufile.c"

// FUNCTION: BIONICLE 0x568520
void *NuFileLoad(char *fname) {
    NUFILE fh;
    int size;
    void *mem;
    int be;

    be = nufile_buffering_enabled;
    mem = NULL;
    nufile_buffering_enabled = 0;
    fh = NuFileOpen(fname, NUFILE_READ);
    if (fh != 0) {
        size = NuFileOpenSize(fh);
        if (size > 0) {
            mem = NuMemAllocAt(size, 2134);
            if (mem != NULL) {
                NuFileRead(fh, mem, size);
            }
        }
        NuFileClose(fh);
    }
    nufile_buffering_enabled = be;
    return mem;
}

// FUNCTION: BIONICLE 0x567c80
int NuDatFileLoadBuffer(NUDATHDR *ndh, char *fname, void *mem, int buffsize) {
    char *loadaddr;
    int size;
    int psize;
    int rv;
    NUFILE fh;

    if (nufile_lsn_allowed && ndh->start_lsn) {
        rv = NuDatFileLoadBufferLSN(ndh, fname, mem, buffsize);
        if (rv >= 0) {
            return rv;
        }
    }
    fh = NuDatFileOpen(ndh, fname, NUFILE_READ);
    if (fh != 0) {
        psize = datfiles[fh - 0x800].len;
        if (datfiles[fh - 0x800].ppack) {
            size = datfiles[fh - 0x800].uplen;
            loadaddr = (char *)mem + size - psize + 0x100;
            if (size + 0x100 <= buffsize && size != 0) {
                while (NuDatFileRead(fh, loadaddr, psize) < 0) {
                    while (NuFileSeek(fh, 0, 0) < 0) {
                    }
                }
                NuFileClose(fh);
                NuPPUnpack(loadaddr, (char *)mem);
                return size;
            }
        } else if (psize <= buffsize && psize != 0) {
            while (NuDatFileRead(fh, mem, psize) < 0) {
                while (NuFileSeek(fh, 0, 0) < 0) {
                }
            }
            NuFileClose(fh);
            return psize;
        }
        NuFileClose(fh);
    }
    return 0;
}

// FUNCTION: BIONICLE 0x568590
int NuFileLoadBuffer(char *fname, void *mem, int buffsize) {
    NUFILE fh;
    int size;
    int rv;

    rv = 0;
    nufileloadbuffer_error = 0;
    if (curr_dat != NULL) {
        rv = NuDatFileLoadBuffer(curr_dat, fname, mem, buffsize);
        nufileloadbuffer_error = 100;
        if (rv != 0) {
            return rv;
        }
    }
    nufileloadbuffer_error = 1;
    if (!NuFileExists(fname)) {
        return rv;
    }
    nufileloadbuffer_error = 2;
    fh = NuFileOpen(fname, NUFILE_READ);
    if (fh == 0) {
        return rv;
    }
    if (nufile_try_packed) {
        nufileloadbuffer_error = 4;
        size = NuPPLoadBuffer(fh, mem, buffsize);
        NuFileClose(fh);
        return size;
    }
    nufileloadbuffer_error = 5;
    size = NuFileOpenSize(fh);
    if (size > buffsize || size == 0) {
        nufileloadbuffer_error = 8;
        size = 0;
    } else {
        nufileloadbuffer_error = 6;
        while (NuFileRead(fh, mem, size) < 0) {
            nufileloadbuffer_error = 7;
            while (NuFileSeek(fh, 0, 0) < 0) {
            }
        }
    }
    NuFileClose(fh);
    return size;
}

// FUNCTION: BIONICLE 0x5687a0
int NuFileLoadBufferVP(char *fname, VARIPTR *ptr, VARIPTR *end) {
    int size;

    size = NuFileLoadBuffer(fname, ptr->voidptr, end->intaddr - ptr->intaddr);
    ptr->intaddr += size;
    return size;
}
