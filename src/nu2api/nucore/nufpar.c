#include "nufpar.h"

#include <string.h>

/* Text file parser: lines, words, numbers and command dispatch over a small pool of
 * parsers. Based on opensagadev/saga src/nu2api/nufile/nufpar.cpp (GPL-3.0); names and
 * locals from the PS2 prototype. BH has a single command stack (saga's later engine has
 * two). Not in the PC exe: NuFParGetPos/SetPos, NuFParPushComCTX,
 * NuFParSetInterpreterErrorHandler, NuFParCreateGivenFH/CreateMem. */

// GLOBAL: BIONICLE 0x34c4278
static NUFPAR *fpars;

// GLOBAL: BIONICLE 0x34c427c
static char *fparbuffers;

// GLOBAL: BIONICLE 0x34c2ce8
static NUFPAR _fpars[4];

// GLOBAL: BIONICLE 0x34bece0
static char _fparbuffers[16384];

// GLOBAL: BIONICLE 0x34c2ce0
static int old_line_pos;

// GLOBAL: BIONICLE 0x34c4280
nufpcomfn *fnInterpreterError;

// FUNCTION: BIONICLE 0x569fe0
char NuGetChar(NUFPAR *fp) {
    int bytesread;
    int be;
    char c;

    be = fp->buffend >= 0 ? fp->buffend : 0;
    if (fp->cpos > fp->buffend) {
        if (fp->buffend + 1 > fp->size) {
            return 0;
        }
        bytesread = NuFileRead(fp->fh, fp->fbuff, fp->size - be <= 0x1000 ? fp->size - be : 0x1000);
        fp->buffstart = fp->buffend + 1;
        fp->buffend += bytesread;
        if (bytesread == 0) {
            return 0;
        }
    }
    c = fp->fbuff[fp->cpos - fp->buffstart];
    fp->cpos++;
    return c;
}

// FUNCTION: BIONICLE 0x56a070
__inline NUWCHAR NuGetWChar(NUFPAR *fp) {
    unsigned char a;
    NUWCHAR c;

    a = NuGetChar(fp);
    c = (unsigned char)NuGetChar(fp);
    c <<= 8;
    c += a;
    return c;
}

// FUNCTION: BIONICLE 0x56a0a0
int NuFParGetLineW(NUFPAR *fp) {
    NUWCHAR chr;
    int len;
    int quit;
    int nws;
    int inquotes;
    NUWCHAR *wline;

    inquotes = 0;
    wline = (NUWCHAR *)fp->lbuff;
    len = 0;
    fp->line_pos = 0;
    fp->line_num++;
    nws = 0;
    for (;;) {
        chr = NuGetWChar(fp);
        switch (chr) {
        case '\r':
            NuGetWChar(fp);
        case '\n':
            if (len == 0 || !nws) {
                fp->line_num++;
                nws = 0;
                break;
            }
        case '\0':
            wline[len] = 0;
            return len;
        case '"':
            inquotes = 1 - inquotes;
            nws = 1;
            wline[len & ((fp->wBuffSize >> 1) - 1)] = chr;
            len++;
            break;
        case ';':
            if (!inquotes) {
                if (len == 0 || !nws) {
                    quit = 0;
                    do {
                        chr = NuGetWChar(fp);
                        switch (chr) {
                        case '\r':
                            NuGetWChar(fp);
                        case '\n':
                        case '\0':
                            quit = 1;
                            break;
                        }
                    } while (!quit);
                    len = 0;
                    fp->line_pos = 0;
                    fp->line_num++;
                    nws = 0;
                } else {
                    for (;;) {
                        chr = NuGetWChar(fp);
                        switch (chr) {
                        case '\r':
                            NuGetWChar(fp);
                        case '\n':
                        case '\0':
                            wline[len] = 0;
                            return len;
                        }
                    }
                }
                break;
            }
        default:
            nws = 1;
        case '\t':
        case ' ':
            wline[len & ((fp->wBuffSize >> 1) - 1)] = chr;
            len++;
            break;
        }
    }
}

// FUNCTION: BIONICLE 0x56a2c0
int NuFParGetLine(NUFPAR *fp) {
    char chr;
    int len;
    int quit;
    int nws;
    int inquotes;

    inquotes = 0;
    if (fp->unicode) {
        return NuFParGetLineW(fp);
    }
    len = 0;
    fp->line_pos = 0;
    fp->line_num++;
    nws = 0;
    for (;;) {
        chr = NuGetChar(fp);
        switch (chr) {
        case '\r':
            NuGetChar(fp);
        case '\n':
            if (len == 0 || !nws) {
                fp->line_num++;
                nws = 0;
                break;
            }
        case '\0':
            fp->lbuff[len] = '\0';
            return len;
        case '"':
            inquotes = 1 - inquotes;
            nws = 1;
            fp->lbuff[len & (fp->lBuffSize - 1)] = chr;
            len++;
            break;
        case ';':
            if (!inquotes) {
                if (len == 0 || !nws) {
                    quit = 0;
                    do {
                        chr = NuGetChar(fp);
                        switch (chr) {
                        case '\r':
                            NuGetChar(fp);
                        case '\n':
                        case '\0':
                            quit = 1;
                            break;
                        }
                    } while (!quit);
                    len = 0;
                    fp->line_pos = 0;
                    fp->line_num++;
                    nws = 0;
                } else {
                    for (;;) {
                        chr = NuGetChar(fp);
                        switch (chr) {
                        case '\r':
                            NuGetChar(fp);
                        case '\n':
                        case '\0':
                            fp->lbuff[len] = '\0';
                            return len;
                        }
                    }
                }
                break;
            }
        default:
            nws = 1;
        case '\t':
        case ' ':
            fp->lbuff[len & (fp->lBuffSize - 1)] = chr;
            len++;
            break;
        }
    }
}

// FUNCTION: BIONICLE 0x56a440
__inline void NuFParUnGetWord(NUFPAR *fp) {
    fp->line_pos = old_line_pos;
}

// FUNCTION: BIONICLE 0x56a460
int NuFParGetWordW(NUFPAR *fp) {
    int len;
    int inquotes;
    NUWCHAR chr;
    NUWCHAR *wline;
    NUWCHAR *wword;

    wline = (NUWCHAR *)fp->lbuff;
    wword = (NUWCHAR *)fp->wbuff;
    len = 0;
    inquotes = 0;
    old_line_pos = fp->line_pos;
    while (wline[fp->line_pos & ((fp->lBuffSize >> 1) - 1)] != 0) {
        chr = wline[fp->line_pos];
        if (fp->seperator_tokens != NULL && !inquotes && NuStrChr(fp->seperator_tokens, (char)chr) != NULL) {
            if (len == 0) {
                wword[len] = chr;
                len++;
                fp->line_pos++;
            }
            wword[len & ((fp->wBuffSize >> 1) - 1)] = 0;
            return len;
        }
        if (fp->SeperatorList != NULL && !inquotes && NuStrChr(fp->SeperatorList, (char)chr) != NULL) {
            chr = ' ';
        }
        switch (chr & 0xff) {
        case ' ':
        case ',':
        case '\t':
            if (!inquotes) {
                if (len != 0) {
                    wword[len & ((fp->wBuffSize >> 1) - 1)] = 0;
                    return len;
                }
                break;
            }
        default:
            if (chr == '"') {
                if (inquotes && wline[fp->line_pos + 1] == chr) {
                    wword[len & ((fp->wBuffSize >> 1) - 1)] = chr;
                    len++;
                    fp->line_pos++;
                } else {
                    inquotes = 1 - inquotes;
                }
            } else {
                wword[len & ((fp->wBuffSize >> 1) - 1)] = chr;
                len++;
            }
            break;
        }
        fp->line_pos++;
    }
    wword[len & ((fp->wBuffSize >> 1) - 1)] = 0;
    return len;
}

// FUNCTION: BIONICLE 0x56a610
int NuFParGetWord(NUFPAR *fp) {
    int len;
    int inquotes;
    int foundquotes;
    char chr;

    foundquotes = 0;
    if (fp->unicode) {
        return NuFParGetWordW(fp);
    }
    len = 0;
    inquotes = 0;
    old_line_pos = fp->line_pos;
    while (fp->lbuff[fp->line_pos & (fp->lBuffSize - 1)] != '\0') {
        chr = fp->lbuff[fp->line_pos];
        if (fp->seperator_tokens != NULL && !inquotes && NuStrChr(fp->seperator_tokens, chr) != NULL) {
            if (len == 0) {
                fp->wbuff[len] = chr;
                len++;
                fp->line_pos++;
            }
            fp->wbuff[len & (fp->wBuffSize - 1)] = '\0';
            return len;
        }
        if (fp->SeperatorList != NULL && !inquotes && NuStrChr(fp->SeperatorList, chr) != NULL) {
            chr = ' ';
        }
        switch (chr) {
        case ' ':
        case ',':
        case '\t':
            if (!inquotes) {
                if (len != 0) {
                    fp->wbuff[len & (fp->wBuffSize - 1)] = '\0';
                    return len;
                }
                break;
            }
        default:
            if (chr == '"') {
                inquotes = 1 - inquotes;
                foundquotes = 1;
            } else {
                fp->wbuff[len & (fp->wBuffSize - 1)] = chr;
                len++;
            }
            break;
        }
        fp->line_pos++;
        if (foundquotes && !inquotes && len == 0) {
            break;
        }
    }
    fp->wbuff[len & (fp->wBuffSize - 1)] = '\0';
    return len;
}

// FUNCTION: BIONICLE 0x56a7d0
int NuFParGetInt(NUFPAR *fp) {
    char twbuff[64];

    NuFParGetWord(fp);
    if (fp->unicode) {
        NuUnicodeToAscii(twbuff, (NUWCHAR16 *)fp->wbuff);
    } else {
        NuStrCpy(twbuff, fp->wbuff);
    }
    if (twbuff[0] != '\0') {
        if (twbuff[0] == '$') {
            return NuHexStringToI(twbuff + 1);
        } else if (twbuff[0] == '0' && (twbuff[1] == 'x' || twbuff[1] == 'X')) {
            return NuHexStringToI(twbuff + 2);
        } else {
            return NuAToI(twbuff);
        }
    } else {
        return 0;
    }
}

// FUNCTION: BIONICLE 0x56a8a0
float NuFParGetFloat(NUFPAR *fp) {
    char twbuff[64];

    NuFParGetWord(fp);
    if (fp->unicode) {
        NuUnicodeToAscii(twbuff, (NUWCHAR16 *)fp->wbuff);
    } else {
        NuStrCpy(twbuff, fp->wbuff);
    }
    if (twbuff[0] != '\0') {
        return NuAToF(twbuff);
    }
    return 0.0f;
}

// FUNCTION: BIONICLE 0x56a930
void NuFParGetOptionalFloat(NUFPAR *fp, float *f) {
    char twbuff[64];

    NuFParGetWord(fp);
    if (fp->unicode) {
        NuUnicodeToAscii(twbuff, (NUWCHAR16 *)fp->wbuff);
    } else {
        NuStrCpy(twbuff, fp->wbuff);
    }
    if (f != NULL && twbuff[0] != '\0') {
        *f = NuAToF(twbuff);
        if (*f == 0.0f) {
            NuFParUnGetWord(fp);
        }
    }
}

// FUNCTION: BIONICLE 0x56a9d0
void NuFParGetOptionalInt(NUFPAR *fp, int *i) {
    char twbuff[64];

    NuFParGetWord(fp);
    if (fp->unicode) {
        NuUnicodeToAscii(twbuff, (NUWCHAR16 *)fp->wbuff);
    } else {
        NuStrCpy(twbuff, fp->wbuff);
    }
    if (i != NULL && twbuff[0] != '\0') {
        *i = NuAToI(twbuff);
        if (*i == 0) {
            NuFParUnGetWord(fp);
        }
    }
}

// FUNCTION: BIONICLE 0x56aa60
int NuFParGetIntRDP(NUFPAR *fp) {
    char twbuff[64];

    NuFParGetWord(fp);
    if (fp->unicode) {
        NuUnicodeToAscii(twbuff, (NUWCHAR16 *)fp->wbuff);
    } else {
        NuStrCpy(twbuff, fp->wbuff);
    }
    if (twbuff[0] != '\0') {
        return NuRDPI(twbuff);
    }
    return 0;
}

// FUNCTION: BIONICLE 0x56aaf0
float NuFParGetFloatRDP(NUFPAR *fp) {
    char twbuff[64];

    NuFParGetWord(fp);
    if (fp->unicode) {
        NuUnicodeToAscii(twbuff, (NUWCHAR16 *)fp->wbuff);
    } else {
        NuStrCpy(twbuff, fp->wbuff);
    }
    if (twbuff[0] != '\0') {
        return NuRDPF(twbuff);
    }
    return 0.0f;
}

// FUNCTION: BIONICLE 0x56ab80
int NuFParPushCom(NUFPAR *fp, NUFPCOMJMP *jmp) {
    if (fp->compos >= 7) {
        return -1;
    }
    fp->compos++;
    fp->comstack.jmp[fp->compos] = jmp;
    return fp->compos;
}

// FUNCTION: BIONICLE 0x56abb0
void NuFParPopCom(NUFPAR *fp) {
    if (fp->compos >= 0) {
        fp->compos--;
    }
}

// FUNCTION: BIONICLE 0x56abd0
int NuFParInterpretWord(NUFPAR *fp) {
    int ix;
    char twbuff[64];

    if (fp->unicode) {
        NuUnicodeToAscii(twbuff, (NUWCHAR16 *)fp->wbuff);
    } else {
        NuStrCpy(twbuff, fp->wbuff);
    }
    if (twbuff[0] == '\0') {
        return 0;
    }
    if (twbuff[0] == ';') {
        return 0;
    }
    if (fp->compos >= 0) {
        for (ix = 0; fp->comstack.jmp[fp->compos][ix].fname != NULL; ix++) {
            if (NuStrICmp(fp->comstack.jmp[fp->compos][ix].fname, twbuff) == 0) {
                fp->comstack.jmp[fp->compos][ix].func(fp);
                return 1;
            }
        }
    }
    if (fnInterpreterError != NULL) {
        fnInterpreterError(fp);
    }
    return 0;
}

// FUNCTION: BIONICLE 0x56acd0
int NuFParInterpretWordCTX(NUFPAR *fp, void *ctx) {
    int ix;
    char twbuff[64];

    if (fp->unicode) {
        NuUnicodeToAscii(twbuff, (NUWCHAR16 *)fp->wbuff);
    } else {
        NuStrCpy(twbuff, fp->wbuff);
    }
    if (twbuff[0] == '\0') {
        return 0;
    }
    if (twbuff[0] == ';') {
        return 0;
    }
    if (fp->compos >= 0) {
        for (ix = 0; fp->comstack.jmp_ctx[fp->compos][ix].fname != NULL; ix++) {
            if (NuStrICmp(fp->comstack.jmp_ctx[fp->compos][ix].fname, twbuff) == 0) {
                fp->comstack.jmp_ctx[fp->compos][ix].func(fp, ctx);
                return 1;
            }
        }
    }
    if (fnInterpreterError != NULL) {
        fnInterpreterError(fp);
    }
    return 0;
}

// FUNCTION: BIONICLE 0x56ade0
void NuFParSuspend(NUFPAR *fp) {
    if (fp->fh > 0x400 && fp->fh < 0x800) {
        return;
    }
    NuFileClose(fp->fh);
    fp->fh = 0;
}

// FUNCTION: BIONICLE 0x56ae10
void NuFParResume(NUFPAR *fp) {
    if (fp->fh > 0x400 && fp->fh < 0x800) {
        return;
    }
    fp->fh = NuFileOpen(fp->fname, NUFILE_READ);
    NuFileSeek(fp->fh, fp->cpos, 0);
    fp->buffend = fp->cpos - 1;
}

__inline void NuFParInit(int max_fpars, int buffsize) {
    fpars = _fpars;
    memset(fpars, 0, sizeof(_fpars));
    fparbuffers = _fparbuffers;
}

// FUNCTION: BIONICLE 0x56ae60
static NUFPAR *NuAllocFPar(void) {
    int n;

    if (fpars == NULL) {
        NuFParInit(4, 0x1000);
    }
    for (n = 0; n < 4; n++) {
        if (fpars[n].fbuff == NULL) {
            memset(&fpars[n], 0, sizeof(NUFPAR));
            fpars[n].fbuff = &fparbuffers[n * 0x1000];
            memset(fpars[n].fbuff, 0, 0x1000);
            return &fpars[n];
        }
    }
    return NULL;
}

static __inline void NuFreeFPar(NUFPAR *fp) {
    fp->fbuff = NULL;
}

// FUNCTION: BIONICLE 0x56aee0
__inline void NuFParClose(NUFPAR *fp) {
    NuFreeFPar(fp);
}

// FUNCTION: BIONICLE 0x56aef0
NUFPAR *NuFParOpen(NUFILE fh) {
    NUWCHAR ch;
    unsigned char cha;
    NUFPAR *fp;

    fp = NuAllocFPar();
    if (fp != NULL) {
        fp->buffend = -1;
        fp->line_num = -1;
        fp->compos = -1;
        fp->lbuff = fp->_lbuff;
        fp->fh = fh;
        fp->SeperatorList = NULL;
        fp->seperator_tokens = NULL;
        fp->wbuff = fp->_wbuff;
        fp->lBuffSize = 0x200;
        fp->wBuffSize = 0x200;
        ch = NuFileReadWChar(fh);
        fp->unicode = 0;
        fp->utf8 = 0;
        if (ch == 0xfffe) {
            fp->unicode = 2;
        } else if (ch == 0xfeff) {
            fp->unicode = 1;
        } else if (ch == 0xbbef) {
            cha = NuFileReadChar(fh);
            if (cha == 0xbf) {
                fp->utf8 = 1;
            }
        } else if (ch == 0xefbb) {
            cha = NuFileReadChar(fh);
            if (cha == 0xbf) {
                fp->utf8 = 2;
            }
        }
        if (!fp->unicode && !fp->utf8) {
            NuFileSeek(fh, 0, 0);
        }
        fp->size = NuFileOpenSize(fh);
    }
    return fp;
}

// FUNCTION: BIONICLE 0x56aff0
void NuFParDestroy(NUFPAR *fp) {
    NUFILE fh = fp->fh;

    NuFParClose(fp);
    NuFileClose(fh);
}

// FUNCTION: BIONICLE 0x56b010
NUFPAR *NuFParCreate(char *fname) {
    NUFILE fh;
    NUFPAR *fp;

    if (NuFileExists(fname)) {
        fh = NuFileOpen(fname, NUFILE_READ);
        if (fh != 0) {
            fp = NuFParOpen(fh);
            if (fp != NULL) {
                NuStrCpy(fp->fname, fname);
                return fp;
            }
            NuFileClose(fh);
        }
    }
    return NULL;
}
