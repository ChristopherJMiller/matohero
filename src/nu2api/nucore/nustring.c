#include "nustring.h"

#include <stdarg.h>
#include <stdio.h>

/* String helpers: ASCII, UTF-16 ("W") and UTF-8, plus the string tables. Based on
 * opensagadev/saga src/nu2api/nucore/nustring_c.cpp (GPL-3.0); names and locals from the
 * PS2 prototype. The PC source order differs from PS2's. MSVC emits a function that
 * calls one defined later in the file at the end of the object, which is why the
 * case-insensitive functions (calling NuToUpper/NuToLower) come last in the exe. */

// FUNCTION: BIONICLE 0x569aa0
int NuStrICmpWC(char *a, char *b, char *WCMatch) {
    char ua;
    char ub;
    int i;
    int LenA;
    int Delta;

    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    while (*a != '\0' && *a != '*') {
        ua = NuToUpper(*a);
        ub = NuToUpper(*b);
        if (ua > ub) {
            return 1;
        }
        if (ua < ub) {
            return -1;
        }
        a++;
        b++;
    }
    if (*a == '*') {
        a++;
        LenA = NuStrLen(a);
        Delta = NuStrLen(b) - LenA;
        if (Delta < 0) {
            return 1;
        }
        if (WCMatch != NULL) {
            for (i = 0; i < Delta; i++) {
                WCMatch[i] = b[i];
            }
            WCMatch[i] = '\0';
        }
        b += Delta;
    } else if (WCMatch != NULL) {
        *WCMatch = '\0';
    }
    while (*a != '\0') {
        ua = NuToUpper(*a);
        ub = NuToUpper(*b);
        if (ua > ub) {
            return 1;
        }
        if (ua < ub) {
            return -1;
        }
        a++;
        b++;
    }
    return 0;
}

// FUNCTION: BIONICLE 0x569e20
int NuSPrintfW(NUWCHAR *dst, NUWCHAR *src, ...) {
    char temp_src_buff[256];
    char temp_dst_buff[256];
    va_list args;

    NuUnicodeToAscii(temp_src_buff, src);
    va_start(args, src);
    vsprintf(temp_dst_buff, temp_src_buff, args);
    va_end(args);
    NuAsciiToUnicode(dst, temp_dst_buff);
    return NuStrLenW(dst);
}

// FUNCTION: BIONICLE 0x5687d0
int NuStrCpyWC(char *dst, char *src, char *WCMatch) {
    int cnt = 0;
    char *Temp;

    if (src == NULL) {
        *dst = '\0';
    } else {
        do {
            if (*src != '*') {
                *dst = *src;
                dst++;
                cnt++;
            } else if ((Temp = WCMatch) != NULL) {
                while (Temp != NULL && *Temp != '\0') {
                    *dst = *Temp;
                    dst++;
                    Temp++;
                    cnt++;
                }
            }
        } while (*src++ != '\0');
    }
    return cnt;
}

// FUNCTION: BIONICLE 0x568820
void NuStrCat(char *str, char *ext) {
    while (*str != '\0') {
        str++;
    }
    if (ext != NULL) {
        do {
            *str = *ext;
            str++;
        } while (*ext++ != '\0');
    }
}

// FUNCTION: BIONICLE 0x568850
int NuStrCpy(char *dst, char *src) {
    char *dst0 = dst;

    if (src != NULL) {
        while (*src != '\0') {
            *dst = *src;
            dst++;
            src++;
        }
    }
    *dst = '\0';
    return dst - dst0;
}

// FUNCTION: BIONICLE 0x568880
int NuStrNCpy(char *dst, char *src, int n) {
    int cnt = 0;

    if (src == NULL) {
        *dst = '\0';
        return 0;
    }
    do {
        n--;
        if (n <= 0) {
            *dst = '\0';
            cnt++;
            break;
        }
        *dst = *src;
        dst++;
        cnt++;
    } while (*src++ != '\0');
    return cnt - 1;
}

// FUNCTION: BIONICLE 0x5688d0
char *NuStrChr(char *src, char c) {
    while (*src != '\0') {
        if (*src == c) {
            return src;
        }
        src++;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x5688f0
char *NuStrRChr(char *src, char c) {
    char *ptr = src;

    while (*ptr != '\0') {
        ptr++;
    }
    while (ptr >= src) {
        if (*ptr == c) {
            return ptr;
        }
        ptr--;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x568920
int NuStrLen(char *str) {
    int cnt = 0;

    while (*str != '\0') {
        cnt++;
        str++;
    }
    return cnt;
}

// FUNCTION: BIONICLE 0x568940
char *NuStrStr(char *str, char *sub) {
    char *str2;
    char *sub2;

    while (*str != '\0') {
        str2 = str;
        sub2 = sub;
        while (*sub2 != '\0' && *str2 != '\0' && *str2 == *sub2) {
            str2++;
            sub2++;
        }
        if (*sub2 == '\0') {
            return str;
        }
        str++;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x569bd0
char *NuStrIStr(char *str, char *sub) {
    char *str2;
    char *sub2;

    while (*str != '\0') {
        str2 = str;
        sub2 = sub;
        while (*sub2 != '\0' && *str2 != '\0' && NuToUpper(*str2) == NuToUpper(*sub2)) {
            str2++;
            sub2++;
        }
        if (*sub2 == '\0') {
            return str;
        }
        str++;
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x568990
int NuStrCmp(char *a, char *b) {
    char ua;
    char ub;

    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    do {
        ua = *a;
        ub = *b;
        if (ua > ub) {
            return 1;
        }
        if (ua < ub) {
            return -1;
        }
        a++;
        b++;
    } while (ua != '\0' && ub != '\0');
    return 0;
}

// FUNCTION: BIONICLE 0x569c50
int NuStrICmp(char *a, char *b) {
    char ua;
    char ub;

    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    do {
        ua = NuToUpper(*a);
        ub = NuToUpper(*b);
        if (ua > ub) {
            return 1;
        }
        if (ua < ub) {
            return -1;
        }
        a++;
        b++;
    } while (ua != '\0' && ub != '\0');
    return 0;
}

// FUNCTION: BIONICLE 0x5689d0
int NuStrNCmp(char *a, char *b, int n) {
    char ua;
    char ub;

    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    if (n == 0) {
        return 0;
    }
    if (n == -1) {
        n = NuStrLen(a);
    } else if (n == -2) {
        n = NuStrLen(b);
    }
    do {
        ua = *a;
        ub = *b;
        if (ua > ub) {
            return 1;
        }
        if (ua < ub) {
            return -1;
        }
        a++;
        b++;
        n--;
    } while (ua != '\0' && ub != '\0' && n != 0);
    return 0;
}

// FUNCTION: BIONICLE 0x569cb0
int NuStrNICmp(char *a, char *b, int n) {
    char ua;
    char ub;

    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    if (n == 0) {
        return 0;
    }
    if (n == -1) {
        n = NuStrLen(a);
    } else if (n == -2) {
        n = NuStrLen(b);
    }
    do {
        ua = NuToUpper(*a);
        ub = NuToUpper(*b);
        if (ua > ub) {
            return 1;
        }
        if (ua < ub) {
            return -1;
        }
        a++;
        b++;
        n--;
    } while (ua != '\0' && ub != '\0' && n != 0);
    return 0;
}

// FUNCTION: BIONICLE 0x569d40
void NuStrUpr(char *a, char *b) {
    while (*b != '\0') {
        *a = NuToUpper(*b);
        a++;
        b++;
    }
    *a = *b;
}

// FUNCTION: BIONICLE 0x569d80
void NuStrLwr(char *a, char *b) {
    while (*b != '\0') {
        *a = NuToLower(*b);
        a++;
        b++;
    }
    *a = *b;
}

// FUNCTION: BIONICLE 0x569dc0
int NuHexStringToI(char *a) {
    int rv = 0;
    int ua;

    while (*a != '\0') {
        rv <<= 4;
        ua = NuToUpper(*a);
        if (ua <= '9' && ua >= '0') {
            rv |= ua - '0';
        } else if (ua <= 'F' && ua >= 'A') {
            rv |= ua - 'A' + 10;
        } else {
            return 0;
        }
        a++;
    }
    return rv;
}

// FUNCTION: BIONICLE 0x569eb0
void NuStrToLower(char *str) {
    while (*str != '\0') {
        *str = NuToLower(*str);
        str++;
    }
}

// FUNCTION: BIONICLE 0x568a40
void NuStrCatW(NUWCHAR *str, NUWCHAR *ext) {
    while (*str != 0) {
        str++;
    }
    if (ext != NULL) {
        do {
            *str = *ext;
            str++;
        } while (*ext++ != 0);
    }
}

// FUNCTION: BIONICLE 0x568a80
int NuStrCpyW(NUWCHAR *dst, NUWCHAR *src) {
    int cnt = 0;

    if (src == NULL) {
        *dst = 0;
    } else {
        do {
            *dst = *src;
            dst++;
            cnt++;
        } while (*src++ != 0);
    }
    return cnt;
}

// FUNCTION: BIONICLE 0x568ab0
int NuStrLenW(NUWCHAR *str) {
    int cnt = 0;

    while (*str != 0) {
        cnt++;
        str++;
    }
    return cnt;
}

NUWCHAR NuToUpperW(NUWCHAR c) {
    if (c >= 'a' && c <= 'z') {
        c -= 0x20;
    } else if (c >= 0xe0 && c <= 0xff) {
        c -= 0x20;
    }
    return c;
}

// FUNCTION: BIONICLE 0x568ad0
int NuStrICmpW(NUWCHAR *a, NUWCHAR *b) {
    NUWCHAR ua;
    NUWCHAR ub;

    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    do {
        ua = NuToUpperW(*a);
        ub = NuToUpperW(*b);
        if (ua > ub) {
            return 1;
        }
        if (ua < ub) {
            return -1;
        }
        a++;
        b++;
    } while (ua != 0 && ub != 0);
    return 0;
}

// FUNCTION: BIONICLE 0x568b50
int NuIsAlNum(char a) {
    switch (a) {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
    case '8': case '9':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
    case '_':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
        return 1;
    default:
        return 0;
    }
}

// FUNCTION: BIONICLE 0x568bd0
float NuAToF(char *string) {
    float v = 0.0f;
    float sign = 1.0f;
    char c = *string++;

    if (c == '-') {
        sign = -1.0f;
        c = *string++;
    }
    while (c >= '0' && c <= '9') {
        v = v * 10.0f + (float)(c - '0');
        c = *string++;
    }
    if (c == '.') {
        c = *string++;
        while (c >= '0' && c <= '9') {
            sign *= 10.0f;
            v = v * 10.0f + (float)(c - '0');
            c = *string++;
        }
    }
    v /= sign;
    return v;
}

// FUNCTION: BIONICLE 0x568c70
int NuAToI(char *string) {
    int v = 0;
    int sign = 0;
    char c = *string++;

    if (c == '-') {
        sign = -1;
        c = *string++;
    }
    while (c >= '0' && c <= '9') {
        v = v * 10 + c - '0';
        c = *string++;
    }
    if (sign != 0) {
        return v * sign;
    }
    return v;
}

// FUNCTION: BIONICLE 0x568cc0
int NuAToIW(NUWCHAR *string) {
    int v = 0;
    int sign = 0;
    NUWCHAR c = *string++;

    if (c == '-') {
        sign = -1;
        c = *string++;
    }
    while (c >= '0' && c <= '9') {
        v = v * 10 + c - '0';
        c = *string++;
    }
    if (sign != 0) {
        return v * sign;
    }
    return v;
}

unsigned char NuToUpper(unsigned char c) {
    if (c >= 'a' && c <= 'z') {
        c -= 0x20;
    } else if (c >= 0xe0) {
        c -= 0x20;
    }
    return c;
}

// FUNCTION: BIONICLE 0x568d10
unsigned char NuToLower(unsigned char c) {
    if (c >= 'A' && c <= 'Z') {
        c += 0x20;
    } else if (c >= 0xc0 && c <= 0xdf) {
        c += 0x20;
    }
    return c;
}
