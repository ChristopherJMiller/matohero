#ifndef NUFPAR_H
#define NUFPAR_H

#include "nufile.h"
#include "nustring.h"

struct nufpar_s;

typedef void nufpcomfn(struct nufpar_s *fp);
typedef void nufpcomctxfn(struct nufpar_s *fp, void *ctx);

typedef struct nufpcomjmp_s {
    char *fname;
    nufpcomfn *func;
} NUFPCOMJMP;

typedef struct nufpcomjmpctx_s {
    char *fname;
    nufpcomctxfn *func;
} NUFPCOMJMPCTX;

typedef struct nufpar_s {
    char *fbuff;
    NUFILE fh;
    char fname[256];
    char _lbuff[514];
    char _wbuff[514];
    char *lbuff;
    char *wbuff;
    int lBuffSize;
    int wBuffSize;
    int line_num;
    int line_pos;
    int cpos;
    int buffstart;
    int buffend;
    union {
        NUFPCOMJMP *jmp[8];
        NUFPCOMJMPCTX *jmp_ctx[8];
    } comstack;
    int compos;
    int size;
    char unicode;
    char utf8;
    char *SeperatorList;
    char *seperator_tokens;
} NUFPAR;

char NuGetChar(NUFPAR *fp);
NUWCHAR NuGetWChar(NUFPAR *fp);
int NuFParGetLineW(NUFPAR *fp);
int NuFParGetLine(NUFPAR *fp);
void NuFParUnGetWord(NUFPAR *fp);
int NuFParGetWordW(NUFPAR *fp);
int NuFParGetWord(NUFPAR *fp);
int NuFParGetInt(NUFPAR *fp);
float NuFParGetFloat(NUFPAR *fp);
void NuFParGetOptionalFloat(NUFPAR *fp, float *f);
void NuFParGetOptionalInt(NUFPAR *fp, int *i);
int NuFParGetIntRDP(NUFPAR *fp);
float NuFParGetFloatRDP(NUFPAR *fp);
int NuFParPushCom(NUFPAR *fp, NUFPCOMJMP *jmp);
void NuFParPopCom(NUFPAR *fp);
int NuFParInterpretWord(NUFPAR *fp);
int NuFParInterpretWordCTX(NUFPAR *fp, void *ctx);
void NuFParSuspend(NUFPAR *fp);
void NuFParResume(NUFPAR *fp);
void NuFParInit(int max_fpars, int buffsize);
void NuFParClose(NUFPAR *fp);
NUFPAR *NuFParOpen(NUFILE fh);
void NuFParDestroy(NUFPAR *fp);
NUFPAR *NuFParCreate(char *fname);

/* Recursive-descent expression evaluators (nurdpi.c, nurdpf.c). */
int NuRDPI(char *expr);
float NuRDPF(char *expr);

#endif
