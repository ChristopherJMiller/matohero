#ifndef NUFILE_H
#define NUFILE_H

#include "folded.h"

#include "nustring.h"

typedef int NUFILE;

typedef enum {
    NUFILE_READ = 0,
    NUFILE_WRITE = 1,
    NUFILE_APPEND = 2,
} NUFILEMODE;

struct nufile_device_s;
typedef int nufiledevFormatName(struct nufile_device_s *dev, char *fullname, char *fname, int buffsize);
typedef int nufiledevInterrogate(struct nufile_device_s *dev);

typedef struct nufile_device_s {
    int id;
    int unitid;
    int attr;
    int status;
    char dirsep;
    int maxNameSize;
    int maxExtSize;
    int maxDirEntries;
    int freeSpace;
    int params[4];
    char name[16];
    int matchlen;
    int namelen;
    char root[32];
    char desc[64];
    char curdir[128];
    char sysdir[128];
    char dlldir[128];
    nufiledevFormatName *FormatName;
    nufiledevInterrogate *Interrogate;
} NUFILE_DEVICE;

typedef struct filebuff_s FILEBUFF;

typedef struct fileinfo_s {
    int fh;
    int read_pos;
    int file_pos;
    int file_length;
    int buff_start;
    int buff_length;
    int use_buff;
    int start_lsn;
    int open_mode;
    FILEBUFF *buff;
} FILEINFO;

struct filebuff_s {
    FILEINFO *info;
    int time;
    char data[1024];
};

typedef struct numemfile_s {
    char *start;
    char *end;
    char *curr;
    NUFILEMODE mode;
    int used;
} NUMEMFILE;

typedef struct nudatinfo_s {
    int foffset;
    int flen;
    int uplen;
    int ppack : 1;
} NUDATFINFO;

typedef struct nudfnode_s {
    short childix;
    short sibix;
    char *txt;
} NUDFNODE;

typedef struct nuiffhdr_s {
    int blk;
    int size;
} NUIFFHDR;

typedef struct blkinfo_s {
    NUIFFHDR hdr;
    int pos;
} BLKINFO;

typedef struct fileextinfo_s {
    char extension[13]; /* stored reversed */
    char type;
    char platform;
    char len;
} FILEEXTINFO;

typedef struct nudathdr_s {
    int ver;
    int nfiles;
    NUDATFINFO *finfo;
    int treecnt;
    NUDFNODE *filetree;
    int leafnamesize;
    char *leafnames;
    int dfhandle;
    NUFILE fh;
    short intalloc;
    short openmode;
    int start_lsn;
    char *name;
    int pos;
} NUDATHDR;

typedef struct nudatfile_s {
    NUDATHDR *ndh;
    int start;
    int pos;
    int len;
    int uplen;
    int fix;
    int used;
    int ppack : 1;
} NUDATFILE;

extern NUFILE_DEVICE host_device;
extern NUFILE_DEVICE *default_device;

void NuFileUpCase(NUFILE_DEVICE *dev, char *name);
void NuFileReldirFix(NUFILE_DEVICE *dev, char *name);
NUFILE_DEVICE *NuFileGetDeviceFromPath(char *path);
void NuFileGetCurrentDirectory(char *dir);
void NuFileSetCurrentDirectory(char *dir);
void NuFileGetCurrentPath(char *dir);
void NuFileSetCurrentSysDirectory(char *dir);
void NuFileSetCurrentDllDirectory(char *dir);
int NuFileFormatName(char *fullname, char *fname, int buffsize);
NUFILE NuMemFileOpen(void *start, int buffsize, NUFILEMODE mode);
void *NuMemFileAddr(NUFILE fh);
int NuMemFileWrite(NUFILE fh, void *data, int size);
int NuFileWrite(NUFILE fh, void *data, int size);
int NuFileWriteFloat(NUFILE fh, float data);
int NuFileWriteInt(NUFILE fh, int data);
int NuFileWriteChar(NUFILE fh, char data);
int NuFileWriteString(NUFILE fh, char *str);
int NuFileWriteStringV(NUFILE fh, char *fmt, ...);
int NuDatFileSeek(NUFILE fh, int offset, int origin);
int NuDatFileFindTree(NUDATHDR *ndh, char *fname);
void NuFileExtInit(void);
int NuFileExtConvert(char *out, char *in, int outsize);
int NuFileExtGetExt(char *out, int outsize, int type);
int NuFileExtRemove(char *out, char *in, int outsize);
int NuFileInitEx(int deviceid, int rebootiop, int eject);
void NuMemFileClose(NUFILE fh);
int NuMemFilePos(NUFILE fh);
int NuMemFileSeek(NUFILE fh, int offset, int origin);
int NuFilePos(NUFILE fh);
void NuFileEndBlkRead(NUFILE fh);
int NuFileAlign(NUFILE fh, int mask);
int NuDatGetFileInfo(NUDATHDR *ndh, char *fname, int *start, int *len);
int NuDatFileRead(NUFILE fh, void *data, int size);
NUFILE NuDatFileOpen(NUDATHDR *ndh, char *fname, NUFILEMODE mode);
int NuMemFileRead(NUFILE fh, void *data, int size);
float NuFileReadFloat(NUFILE fh);
int NuFileReadInt(NUFILE fh);
short NuFileReadShort(NUFILE fh);
int NuFileBeginBlkRead(NUFILE fh, int blk);
int NuDatFileLoadBufferLSN(NUDATHDR *ndh, char *fname, void *mem, int buffsize);
int NuDatFileLoadBuffer(NUDATHDR *ndh, char *fname, void *mem, int buffsize);
NUDATHDR *NuDatOpenEx(char *fname, VARIPTR *buff, int *req, short openmode);
NUDATHDR *NuDatOpen(char *fname, VARIPTR *buff, int *req);
void *NuFileLoad(char *fname);
int NuFileLoadBuffer(char *fname, void *mem, int buffsize);
int NuFileLoadBufferVP(char *fname, VARIPTR *ptr, VARIPTR *end);

/* RNC ("PP") packed files (rnc.c). */
int NuPPLoadBuffer(NUFILE fh, void *mem, int buffsize);
void NuPPUnpack(char *paddr, char *waddr);
NUDATHDR *NuDatSet(NUDATHDR *dh);
NUDATHDR *NuDatGet(void);

int NuFileExists(char *fname);
int NuFileSize(char *fname);
NUFILE NuFileOpen(char *file, NUFILEMODE mode);
void NuFileClose(NUFILE fh);
int NuFileSeek(NUFILE fh, int offset, int origin);
int NuFileRead(NUFILE fh, void *data, int size);
char NuFileReadChar(NUFILE fh);
NUWCHAR NuFileReadWChar(NUFILE fh);
int NuFileOpenSize(NUFILE fh);

/* PC file layer (pc/nufile_pc.c; PS2: ps2/nufile_ps2.c). */
int NuPSFileOpen(char *filename, int flag);
int NuPSFileClose(int fd);
int NuPSFileRead(int fd, void *buf, int nbyte);
int NuPSFileWrite(int fd, void *buf, int nbyte);
int NuPSFileLSeek(int fd, int offset, int where);
int NuFileRename(char *to, char *from);
int NuFileCreateDir(char *dir);

#endif
