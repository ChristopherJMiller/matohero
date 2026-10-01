#ifndef NUSTRING_H
#define NUSTRING_H

#include "nutypes.h"

typedef u16 NUWCHAR;
typedef u8 NUWCHAR8;
typedef u16 NUWCHAR16;

typedef struct nustring_s {
    char *id;
    union {
        NUWCHAR *text;
        NUWCHAR8 *text_utf8;
    };
} NUSTRING;

typedef struct nustringbank_s {
    NUSTRING *Strings;
    int NumStrings;
    int MaxStrings;
} NUSTRINGBANK;

int NuStrICmpWC(char *a, char *b, char *WCMatch);
int NuSPrintfW(NUWCHAR *dst, NUWCHAR *src, ...);
int NuStrCpyWC(char *dst, char *src, char *WCMatch);
void NuStrCat(char *str, char *ext);
int NuStrCpy(char *dst, char *src);
int NuStrNCpy(char *dst, char *src, int n);
char *NuStrChr(char *src, char c);
char *NuStrRChr(char *src, char c);
int NuStrLen(char *str);
char *NuStrStr(char *str, char *sub);
char *NuStrIStr(char *str, char *sub);
int NuStrCmp(char *a, char *b);
int NuStrICmp(char *a, char *b);
int NuStrNCmp(char *a, char *b, int n);
int NuStrNICmp(char *a, char *b, int n);
void NuStrUpr(char *a, char *b);
void NuStrLwr(char *a, char *b);
int NuHexStringToI(char *a);
int NuIsAlNum(char a);
float NuAToF(char *string);
int NuAToI(char *string);
unsigned char NuToUpper(unsigned char c);
unsigned char NuToLower(unsigned char c);
void NuStrToLower(char *str);
void NuStrCatW(NUWCHAR *str, NUWCHAR *ext);
int NuStrCpyW(NUWCHAR *dst, NUWCHAR *src);
int NuStrLenW(NUWCHAR *str);
int NuStrICmpW(NUWCHAR *a, NUWCHAR *b);
int NuAToIW(NUWCHAR *string);
NUWCHAR NuToUpperW(NUWCHAR c);
void NuUnicodeToAscii(char *dest, NUWCHAR16 *src);
void NuUnicodeToUTF8(NUWCHAR8 *dest, NUWCHAR16 *src);
void NuAsciiToUnicode(NUWCHAR16 *dest, char *src);
void NuUTF8ToUnicode(NUWCHAR16 *dest, NUWCHAR8 *src);
NUWCHAR8 *NuUnicodeCharFromUTF8(NUWCHAR *unicode, NUWCHAR8 *utf8);
void NuStringTableSetFormat(int format);
int NuStringTableLoadCSV(char *filename, VARIPTR *ptr, VARIPTR end, char *label, char *language);
int NuStringTableLoadTXT(char *filename, VARIPTR *ptr, VARIPTR end);
int NuStringTableGetFormat(void);
void NuStringTableSetBank(int bank);
void NuStringTableUnload(void);
NUWCHAR *NuStringTableGetById(int id);
int NuStringTableGetIdByName(char *stringname);
NUWCHAR *NuStringTableGetByName(char *stringname);
void NuStrFixExtPlatform(char *dst, char *src, char *ext, int dst_size, char *platform_string);
void NuStrFixExt(char *dst, char *src, char *ext, int dst_size);
void NuStrGetFilenameNoExt(char *filename, char *filepath);
void NuStrGetPath(char *path, char *filepath);
void NuStrGetExt(char *ext, char *filepath);

#endif
