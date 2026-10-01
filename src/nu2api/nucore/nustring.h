#ifndef NUSTRING_H
#define NUSTRING_H

#include "nutypes.h"

typedef u16 NUWCHAR;
typedef u8 NUWCHAR8;
typedef u16 NUWCHAR16;

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
void NuAsciiToUnicode(NUWCHAR16 *dest, char *src);

#endif
