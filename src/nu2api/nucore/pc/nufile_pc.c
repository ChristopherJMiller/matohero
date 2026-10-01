#include <stdio.h>
#include <windows.h>

#include "nufile.h"

/* PC file layer (PS2: nucore/ps2/nufile_ps2.c): the host device and CRT stdio file
 * handles. Names and locals from the PS2 prototype; the file name is ours. */

int DEV_FormatName(NUFILE_DEVICE *dev, char *fullname, char *fname, int buffsize);
int DEVHOST_Interrogate(NUFILE_DEVICE *dev);

// GLOBAL: BIONICLE 0x617260
NUFILE_DEVICE host_device = {
    0, 0, 0xe, 0, '\\', 0x400, -1, -1, -1, {0, 0, 0, 0},
    "", 4, 6, "", "Host pc filesystem", "", "", "",
    DEV_FormatName, DEVHOST_Interrogate,
};

// GLOBAL: BIONICLE 0x34c5710
static FILE *fhandles[64];

// FUNCTION: BIONICLE 0x56b4b0
int NuFileExists(char *fname) {
    return NuFileSize(fname) > 0;
}

// FUNCTION: BIONICLE 0x56b4d0
int DEV_FormatName(NUFILE_DEVICE *dev, char *fullname, char *fname, int buffsize) {
    char buff[512];
    char *chr;

    if (NuStrNICmp(fname, dev->name, NuStrLen(dev->name)) == 0) {
        chr = fname + NuStrLen(dev->name);
        if (*chr == '\\' || *chr == '/') {
            NuStrCpy(buff, fname);
        } else {
            NuStrCpy(buff, dev->root);
            NuStrCat(buff, dev->curdir);
            NuStrCat(buff, chr);
        }
    } else {
        NuStrCpy(buff, dev->root);
        NuStrCat(buff, dev->curdir);
        NuStrCat(buff, fname);
    }
    NuFileUpCase(dev, buff + NuStrLen(dev->root));
    NuFileReldirFix(dev, buff);
    if (NuStrLen(buff) < buffsize) {
        NuStrCpy(fullname, buff);
        return 1;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x56b600
int DEVHOST_Interrogate(NUFILE_DEVICE *dev) {
    return dev->status = 1;
}

static __inline int GetFreeHandle(void) {
    int i;

    for (i = 0; i < 64; i++) {
        if (fhandles[i] == NULL) {
            return i;
        }
    }
    return -1;
}

// FUNCTION: BIONICLE 0x56b610
int NuPSFileOpen(char *filename, int flag) {
    char *mode;
    int fd;

    switch (flag) {
    case 0:
        // STRING: BIONICLE 0x5df464
        mode = "rb";
        break;
    case 1:
        // STRING: BIONICLE 0x5e0920
        mode = "wb+";
        break;
    case 2:
        // STRING: BIONICLE 0x5e0924
        mode = "ab+";
        break;
    default:
        mode = (char *)flag;
        break;
    }
    fd = GetFreeHandle();
    fhandles[fd] = fopen(filename, mode);
    if (fhandles[fd] == NULL) {
        return -1;
    }
    return fd;
}

// FUNCTION: BIONICLE 0x56b680
int NuPSFileClose(int fd) {
    int rv = fclose(fhandles[fd]);

    fhandles[fd] = NULL;
    return rv;
}

// FUNCTION: BIONICLE 0x56b6b0
int NuPSFileRead(int fd, void *buf, int nbyte) {
    return fread(buf, 1, nbyte, fhandles[fd]);
}

// FUNCTION: BIONICLE 0x56b6e0
int NuPSFileWrite(int fd, void *buf, int nbyte) {
    return fwrite(buf, 1, nbyte, fhandles[fd]);
}

// FUNCTION: BIONICLE 0x56b710
int NuPSFileLSeek(int fd, int offset, int where) {
    if (where == 0) {
        fseek(fhandles[fd], offset, SEEK_SET);
    } else if (where == 1) {
        fseek(fhandles[fd], offset, SEEK_CUR);
    } else {
        fseek(fhandles[fd], offset, SEEK_END);
    }
    return ftell(fhandles[fd]);
}

// FUNCTION: BIONICLE 0x56b760
int NuFileRename(char *to, char *from) {
    char lto[256];
    char lfrom[256];

    NuFileFormatName(lfrom, from, 0x100);
    NuFileFormatName(lto, to, 0x100);
    DeleteFileA(lto);
    return MoveFileA(lfrom, lto);
}

// FUNCTION: BIONICLE 0x56b7e0
int NuFileCreateDir(char *dir) {
    SECURITY_ATTRIBUTES sa;

    sa.bInheritHandle = TRUE;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    return CreateDirectoryA(dir, &sa);
}
