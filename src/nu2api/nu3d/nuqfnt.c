#include "nuqfnt.h"

/* Quick fonts. Names, types and locals from the PS2 prototype (nu3d/nuqfnt.c); logic
 * checked against opensagadev/saga src/nu2api/nu3d/nuqfnt.cpp (GPL-3.0, a later engine).
 * Most functions accept NULL for the system font. In PC order. */

// GLOBAL: BIONICLE 0x611d78
float qfnt_rezscale_w = 1.0f;

// GLOBAL: BIONICLE 0x611d7c
float qfnt_rezscale_h = 1.0f;

// GLOBAL: BIONICLE 0x611d80
float qfnt_len_scale = 1.0f;

// GLOBAL: BIONICLE 0x611d84
float qfnt_height_scale = 1.0f;

// GLOBAL: BIONICLE 0x611d88
static float justify_squash = 1.0f;

// GLOBAL: BIONICLE 0x611d8c
static float justify_stretch = 1.0f;

// GLOBAL: BIONICLE 0x968b18
float qfnt_offscale_x;

// GLOBAL: BIONICLE 0x968b1c
float qfnt_offscale_y;

// GLOBAL: BIONICLE 0x968b20
float nuqfnt_space_width;

// GLOBAL: BIONICLE 0x968b24
NUQFNT *system_qfont;

// GLOBAL: BIONICLE 0x968b28
static int nuqfnt_init;

// GLOBAL: BIONICLE 0x968b2c
NUQFNT_CSMODE NuQFntCSMode;

// GLOBAL: BIONICLE 0x968b30
NUQFNT_PRINTMODE NuQFntMode;

// GLOBAL: BIONICLE 0x968b34
int NuQFntCSModeStackIndex;

// GLOBAL: BIONICLE 0x968b38
int NuQFntModeStackIndex;

// GLOBAL: BIONICLE 0x3607580
NUQFNT_CSMODE NuQFntCSModeStack[16];

// GLOBAL: BIONICLE 0x36075c0
NUQFNT_PRINTMODE NuQFntModeStack[16];

// FUNCTION: BIONICLE 0x536d70
void NuQFntPushPrintMode(NUQFNT_PRINTMODE newmode) {
    if (NuQFntModeStackIndex < 16) {
        NuQFntModeStack[NuQFntModeStackIndex++] = NuQFntMode;
    }
    NuQFntMode = newmode;
}

// FUNCTION: BIONICLE 0x536db0
void NuQFntPopPrintMode(void) {
    if (NuQFntModeStackIndex > 0) {
        NuQFntMode = NuQFntModeStack[--NuQFntModeStackIndex];
    }
}

/* Name from saga (not in the PS2 prototype). */
// FUNCTION: BIONICLE 0x536e00
void NuQFntSetICGap(NUQFNT *fv, float icgap) {
    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv != NULL) {
        ((VUFNT *)fv)->icgap = icgap;
    }
}

/* 0 uses each font's own space width. */
// FUNCTION: BIONICLE 0x536e20
void NuQFntSetSpaceWidth(NUQFNT *fv, float width) {
    nuqfnt_space_width = width;
}

/* Sets the scales from text coordinates to the screen; returns the previous mode. */
// FUNCTION: BIONICLE 0x536e30
NUQFNT_CSMODE NuQFntSetCoordinateSystem(NUQFNT_CSMODE mode) {
    NUQFNT_CSMODE old = NuQFntCSMode;

    NuQFntCSMode = mode;
    switch (mode) {
    case NUQFNT_CSMODE_PIXEL:
        qfnt_rezscale_w = 1.0f;
        qfnt_rezscale_h = 0.5f;
        qfnt_offscale_x = 1.0f;
        qfnt_offscale_y = 1.0f;
        qfnt_len_scale = 1.0f;
        qfnt_height_scale = 1.0f;
        break;
    case NUQFNT_CSMODE_NORMALISED:
        qfnt_rezscale_w = 320.0f;
        qfnt_rezscale_h = -112.0f;
        qfnt_offscale_x = 320.0f;
        qfnt_offscale_y = 112.0f;
        qfnt_len_scale = 1.0f / 320.0f;
        qfnt_height_scale = -1.0f / 112.0f;
        break;
    case NUQFNT_CSMODE_ABSOLUTE:
        qfnt_rezscale_w = 1.0f;
        qfnt_rezscale_h = 1.0f;
        qfnt_offscale_x = 0.0f;
        qfnt_offscale_y = 0.0f;
        qfnt_len_scale = 1.0f;
        qfnt_height_scale = -1.0f;
        break;
    default:
        qfnt_rezscale_w = 0.0625f;
        qfnt_rezscale_h = 0.0625f;
        qfnt_offscale_x = 0.0f;
        qfnt_offscale_y = 0.0f;
        qfnt_len_scale = 16.0f;
        qfnt_height_scale = 16.0f;
        break;
    }
    return old;
}

// FUNCTION: BIONICLE 0x536f20
void NuQFntPushCoordinateSystem(NUQFNT_CSMODE newmode) {
    if (NuQFntCSModeStackIndex < 16) {
        NuQFntCSModeStack[NuQFntCSModeStackIndex++] = NuQFntCSMode;
    }
    NuQFntSetCoordinateSystem(newmode);
}

// FUNCTION: BIONICLE 0x536f50
void NuQFntPopCoordinateSystem(void) {
    if (NuQFntCSModeStackIndex > 0) {
        NuQFntSetCoordinateSystem(NuQFntCSModeStack[--NuQFntCSModeStackIndex]);
    }
}

/* Binary search of a font's sorted Unicode table; 0xffff if the character is missing. */
// FUNCTION: BIONICLE 0x536f70
NUWCHAR UnicodeToIndexFast(VUCHARIDX *idxtab, int tablesize, NUWCHAR unicode) {
    int lo;
    int hi;
    int mid;

    mid = tablesize / 2;
    if (unicode > idxtab[tablesize - 1].unicode) {
        return 0xffff;
    }
    hi = tablesize - 1;
    lo = 0;
    while (idxtab[mid].unicode != unicode) {
        if (hi == lo + 1) {
            if (idxtab[hi].unicode == unicode) {
                return idxtab[hi].index;
            }
            if (idxtab[lo].unicode == unicode) {
                return idxtab[lo].index;
            }
            return 0xffff;
        }
        if (idxtab[mid].unicode > unicode) {
            hi = mid;
            mid = (mid + lo) >> 1;
        } else if (idxtab[mid].unicode < unicode) {
            lo = mid;
            mid = (mid + hi) >> 1;
        } else if (hi == lo) {
            return 0xffff;
        }
    }
    return idxtab[mid].index;
}

// FUNCTION: BIONICLE 0x537010
__inline NUWCHAR NuQFntEncodeUnicodeChar(NUQFNT *fv, NUWCHAR c) {
    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv == NULL) {
        return 0xffff;
    }
    return UnicodeToIndexFast(((VUFNT *)fv)->index_table.w, ((VUFNT *)fv)->unicodetablesize, c);
}

/* In place; characters the font lacks become '?'. */
// FUNCTION: BIONICLE 0x537040
void NuQFntEncodeUnicodeString(NUQFNT *fv, NUWCHAR *txt) {
    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv == NULL) {
        return;
    }
    while (*txt != 0) {
        *txt = UnicodeToIndexFast(((VUFNT *)fv)->index_table.w, ((VUFNT *)fv)->unicodetablesize, *txt);
        if (*txt == 0xffff) {
            *txt = '?';
        }
        txt++;
    }
}

// FUNCTION: BIONICLE 0x537090
void NuQFntUTF8toQCode(NUQFNT *fv, char *txtin, NUWCHAR *txtout) {
    NUWCHAR w;

    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv == NULL) {
        return;
    }
    while (*txtin != 0) {
        txtin = (char *)NuUnicodeCharFromUTF8(&w, (NUWCHAR8 *)txtin);
        *txtout = NuQFntEncodeUnicodeChar(fv, w);
        if (*txtout == 0xffff) {
            *txtout = NuQFntEncodeUnicodeChar(fv, '?');
        }
        txtout++;
    }
    *txtout = 0;
}

/* Width of a string of glyph codes. In print mode 1 digits are all as wide as '0'. */
// FUNCTION: BIONICLE 0x537140
float NuQFntPrintLenW(NUQFNT *fv, NUWCHAR *txt) {
    float len;
    float scale_x;
    NUWCHAR t;
    float w;

    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv == NULL) {
        return 0.0f;
    }
    len = 0.0f;
    scale_x = *((VUFNT *)fv)->scale_x;
    while ((t = *txt++) != 0) {
        switch (t) {
        case ' ':
            if (nuqfnt_space_width != 0.0f) {
                w = nuqfnt_space_width;
            } else {
                w = ((VUFNT *)fv)->chars[t].w;
            }
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (NuQFntMode & 1) {
                w = ((VUFNT *)fv)->chars[UnicodeToIndexFast(((VUFNT *)fv)->index_table.w,
                                                            ((VUFNT *)fv)->unicodetablesize, '0')]
                        .w;
            } else {
                w = ((VUFNT *)fv)->chars[t].w;
            }
            break;
        default:
            w = ((VUFNT *)fv)->chars[t].w;
            break;
        }
        len += w + ((VUFNT *)fv)->icgap;
    }
    return qfnt_len_scale * scale_x * len;
}

/* Width of a UTF-8 string. */
// FUNCTION: BIONICLE 0x537210
float NuQFntPrintLenU(NUQFNT *fv, char *txt) {
    float len;
    float scale_x;
    NUWCHAR t;
    NUWCHAR qt;
    float w;

    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv == NULL) {
        return 0.0f;
    }
    len = 0.0f;
    scale_x = *((VUFNT *)fv)->scale_x;
    while (*txt != 0) {
        txt = (char *)NuUnicodeCharFromUTF8(&t, (NUWCHAR8 *)txt);
        qt = NuQFntEncodeUnicodeChar(fv, t);
        if (qt == 0xffff) {
            qt = NuQFntEncodeUnicodeChar(fv, '?');
        }
        switch (t) {
        case ' ':
            if (nuqfnt_space_width != 0.0f) {
                w = nuqfnt_space_width;
            } else {
                w = ((VUFNT *)fv)->chars[qt].w;
            }
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (NuQFntMode & 1) {
                w = ((VUFNT *)fv)->chars[NuQFntEncodeUnicodeChar(fv, '0')].w;
            } else {
                w = ((VUFNT *)fv)->chars[qt].w;
            }
            break;
        default:
            w = ((VUFNT *)fv)->chars[qt].w;
            break;
        }
        len += w + ((VUFNT *)fv)->icgap;
    }
    return qfnt_len_scale * scale_x * len;
}

// FUNCTION: BIONICLE 0x537340
float NuQFntHeight(NUQFNT *fv) {
    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv == NULL) {
        return 0.0f;
    }
    return *((VUFNT *)fv)->scale_y * ((VUFNT *)fv)->height * qfnt_height_scale;
}

// FUNCTION: BIONICLE 0x537370
float NuQFntBaseline(NUQFNT *fv) {
    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv == NULL) {
        return 0.0f;
    }
    return *((VUFNT *)fv)->scale_y * ((VUFNT *)fv)->baseline * qfnt_height_scale;
}

// FUNCTION: BIONICLE 0x5375d0
void NuQFntSet(NUQFNT *fv) {
    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv != NULL) {
        ((VUFNT *)fv)->sendid = 1;
        nuqfnt_space_width = ((VUFNT *)fv)->spacewidth;
    }
}

// FUNCTION: BIONICLE 0x537600
void NuQFntSetScale(NUQFNT *fv, float xs, float ys) {
    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv != NULL) {
        *((VUFNT *)fv)->scale_x = xs;
        *((VUFNT *)fv)->scale_y = ys * 0.5f;
    }
}

// FUNCTION: BIONICLE 0x537630
void NuQFntSetColour(NUQFNT *fv, unsigned int abgr) {
    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv != NULL) {
        ((VUFNT_PC *)((VUFNT *)fv)->psdata)->abgr = abgr;
    }
}

// FUNCTION: BIONICLE 0x5382b0
void NuQFntPrintW(NUQFNT *fv, NUWCHAR *txt) {
    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv != NULL) {
        NuQFntPrintCharW(fv, txt, NuQFntMode);
    }
}

// FUNCTION: BIONICLE 0x5382e0
void NuQFntPrintU(NUQFNT *fv, char *txt) {
    NUWCHAR wtxt[1024];

    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv != NULL) {
        NuQFntUTF8toQCode(fv, txt, wtxt);
        NuQFntPrintCharW(fv, wtxt, NuQFntMode);
    }
}
