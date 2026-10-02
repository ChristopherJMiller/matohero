#include "nuqfnt.h"

#include "numtl.h"
#include "nufile.h"
#include "nuptrblock.h"
#include "nutex.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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

/* The built-in font: a pointer block (NuPtrBlockFix) holding a VUFNT and its texture.
 * The original's 18736 bytes of font data aren't reproduced here (game data). */
// GLOBAL: BIONICLE 0x611d90
unsigned char sysfont[18736] = {0};

// GLOBAL: BIONICLE 0x968b18
float qfnt_offscale_x = 0.0f;

// GLOBAL: BIONICLE 0x968b1c
float qfnt_offscale_y = 0.0f;

// GLOBAL: BIONICLE 0x968b20
float nuqfnt_space_width = 0.0f;

// GLOBAL: BIONICLE 0x968b24
NUQFNT *system_qfont = NULL;

// GLOBAL: BIONICLE 0x968b28
static int nuqfnt_init = 0;

// GLOBAL: BIONICLE 0x968b2c
NUQFNT_CSMODE NuQFntCSMode = NUQFNT_CSMODE_UNINITIALISED;

// GLOBAL: BIONICLE 0x968b30
NUQFNT_PRINTMODE NuQFntMode = 0;

// GLOBAL: BIONICLE 0x968b34
int NuQFntCSModeStackIndex = 0;

// GLOBAL: BIONICLE 0x968b38
int NuQFntModeStackIndex = 0;

// GLOBAL: BIONICLE 0x3607580
NUQFNT_CSMODE NuQFntCSModeStack[16];

// GLOBAL: BIONICLE 0x36075c0
NUQFNT_PRINTMODE NuQFntModeStack[16];

// FUNCTION: BIONICLE 0x536d70
__inline void NuQFntPushPrintMode(NUQFNT_PRINTMODE newmode) {
    if (NuQFntModeStackIndex < 16) {
        NuQFntModeStack[NuQFntModeStackIndex++] = NuQFntMode;
    }
    NuQFntMode = newmode;
}

// FUNCTION: BIONICLE 0x536db0
__inline void NuQFntPopPrintMode(void) {
    if (NuQFntModeStackIndex > 0) {
        NuQFntMode = NuQFntModeStack[--NuQFntModeStackIndex];
    }
}

/* Frees the font's material, and its texture unless flag 1 says it is shared. */
// FUNCTION: BIONICLE 0x536dd0
void NuQFntDestroy(NUQFNT *fv) {
    int tid;

    if (fv != NULL) {
        tid = ((VUFNT *)fv)->mtl->tid;
        NuMtlDestroy(((VUFNT *)fv)->mtl);
        if (!(((VUFNT *)fv)->flags & 1)) {
            NuTexDestroy(tid);
        }
    }
}

/* Name from saga (not in the PS2 prototype). */
// FUNCTION: BIONICLE 0x536e00
__inline void NuQFntSetICGap(NUQFNT *fv, float icgap) {
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

/* PC setup of a loaded font: its material, and the render state block psdata points to
 * (name from saga). flags and renderplane aren't used on PC. */
// FUNCTION: BIONICLE 0x5373a0
int NuQFntReadPS(VUFNT *fv, int tid, int flags, int renderplane, VARIPTR *ptr, VARIPTR buffend) {
    NUSHADERMTLDESC desc;

    fv->mtl = NuMtlCreate(1);
    fv->mtl->diffuse.r = 1.0f;
    fv->mtl->diffuse.g = 1.0f;
    fv->mtl->diffuse.b = 1.0f;
    fv->mtl->attrib.cull = 2;
    fv->mtl->attrib.zmode = 3;
    fv->mtl->alpha = 1.0f;
    fv->mtl->attrib.alpha = 1;
    fv->mtl->attrib.colour = 1;
    fv->mtl->tid = tid;
    fv->mtl->attrib.utc = 1;
    fv->mtl->attrib.vtc = 1;
    memset(&desc, 0, sizeof(desc));
    desc.flags = 0x1000;
    desc.unknown_004 = 1;
    desc.unknown_014 = 0xffffffff;
    desc.unknown_024 = 1.0f;
    desc.unknown_034 = 1.0f;
    desc.unknown_100 = (desc.unknown_100 & ~0x3000) | 0x40901;
    NuMtlSetShaderDescPS(fv->mtl, &desc);
    NuMtlUpdate(fv->mtl);
    fv->psdata = (void *)((ptr->intaddr + 15) & ~15);
    ptr->intaddr = ((ptr->intaddr + 15) & ~15) + sizeof(VUFNT_PC);
    fv->scale_x = &((VUFNT_PC *)fv->psdata)->scale_x;
    fv->scale_y = &((VUFNT_PC *)fv->psdata)->scale_y;
    fv->abgr = &((VUFNT_PC *)fv->psdata)->abgr;
    fv->header.voidptr = NULL;
    return 0;
}

/* A font saved as a pointer block: fixes it up, creates its texture and sets it up.
 * Inlined everywhere, so the linker dropped it from the PC executable. */
__inline NUQFNT *NuQFntReadBuffer(VARIPTR *font, VARIPTR *ptr, VARIPTR buffend) {
    void **header;
    VUFNT *rv;
    int tid;

    if (font->voidptr == NULL) {
        return NULL;
    }
    header = NuPtrBlockFix(font->voidptr);
    rv = header[0];
    tid = NuTexCreate(header[1]);
    NuQFntReadPS(rv, tid, 4, 0, ptr, buffend);
    return rv;
}

/* Loads "<fname>.fnt" (the PC variant of the name) into the buffer. */
// FUNCTION: BIONICLE 0x5374f0
NUQFNT *NuQFntRead(char *fname, VARIPTR *ptr, VARIPTR buffend) {
    char filename[1204];
    VARIPTR font;

    NuStrFixExtPlatform(filename, fname,
                        // STRING: BIONICLE 0x5e00ac
                        "fnt", 1024,
                        // STRING: BIONICLE 0x5e00b0
                        "pc");
    font.intaddr = (ptr->intaddr + 31) & ~31;
    *ptr = font;
    if (NuFileLoadBufferVP(filename, ptr, &buffend) == 0) {
        return NULL;
    }
    return NuQFntReadBuffer(&font, ptr, buffend);
}

// FUNCTION: BIONICLE 0x5375d0
__inline void NuQFntSet(NUQFNT *fv) {
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

/* Positions the next print, converting from the current coordinate system (name from
 * saga; the PS2 inlines it). The render stream isn't used on PC. */
/* Builds a quick font from an old-style NUFNT in the buffer: the VUFNT with its psdata,
 * a VUFNTHDR-like block (gap, height, 1/texture size, space width, then the scales),
 * the glyph rectangles in texels, the character map and a copy of the material. flags
 * pick the material's uvmode, alpha, zmode and filter; renderplane carries the sort
 * priority in its top half. Name from saga. */
// FUNCTION: BIONICLE 0x537650
VUFNT *NuQFntCreate(NUFNT *fnt, int flags, int renderplane, VARIPTR *ptr, VARIPTR buffend) {
    VUFNT *vf;
    float width;
    float height;
    int sortpri;
    int i;
    int ix;

    sortpri = renderplane >> 16;
    renderplane &= 0xffff;
    width = NuTexWidth(fnt->mtl->tid);
    height = NuTexHeight(fnt->mtl->tid);
    vf = (VUFNT *)((ptr->intaddr + 15) & ~15);
    ptr->intaddr = ((ptr->intaddr + 15) & ~15) + sizeof(VUFNT) + sizeof(VUFNT_PC);
    memset(vf, 0, sizeof(VUFNT) + sizeof(VUFNT_PC));
    vf->psdata = vf + 1;
    vf->numchars = fnt->nchars;
    vf->fnt = fnt;
    vf->height = fnt->hi;
    vf->baseline = fnt->bi;
    vf->icgap = fnt->icgap;
    vf->spacewidth = width * fnt->chars[0].x2 - width * fnt->chars[0].x1;
    vf->header = *ptr;
    *ptr->f32 = vf->icgap;
    ptr->f32++;
    *ptr->f32 = vf->height;
    ptr->f32++;
    *ptr->f32 = 1.0f / width;
    ptr->f32++;
    *ptr->f32 = 1.0f / height;
    ptr->f32++;
    *ptr->f32 = 0.0f;
    ptr->f32++;
    *ptr->f32 = 0.0f;
    ptr->f32++;
    *ptr->f32 = vf->spacewidth;
    ptr->f32++;
    *ptr->f32 = 0.0f;
    ptr->f32++;
    vf->scale_x = ptr->f32;
    *ptr->f32 = 1.0f;
    ptr->f32++;
    vf->scale_y = ptr->f32;
    *ptr->f32 = 0.5f;
    ptr->f32++;
    vf->chars = (VUCHAR *)ptr->voidptr;
    for (i = 0; i < (fnt->nchars < 256 ? fnt->nchars : 256); i++) {
        *ptr->f32 = width * fnt->chars[i].x1;
        ptr->f32++;
        *ptr->f32 = height * fnt->chars[i].y1;
        ptr->f32++;
        *ptr->f32 = width * fnt->chars[i].x2 - width * fnt->chars[i].x1;
        ptr->f32++;
    }
    vf->index_table.a = ptr->u8;
    for (i = 0; i < 256; i++) {
        ix = fnt->chrix[i];
        if (ix == 0xff) {
            ix = 0;
        }
        *ptr->u8 = (i == ' ') ? fnt->nchars : ix;
        ptr->u8++;
    }
    vf->mtl = (NUMTL *)((ptr->intaddr + 15) & ~15);
    ptr->intaddr = ((ptr->intaddr + 15) & ~15) + sizeof(NUMTL);
    *vf->mtl = *fnt->mtl;
    NuMtlAddEx(vf->mtl, renderplane);
    if (flags & 8) {
        vf->mtl->attrib.uvmode = 0;
    } else {
        vf->mtl->attrib.uvmode = 1;
    }
    vf->mtl->attrib.lighting = 2;
    if (flags & 2) {
        vf->mtl->attrib.alpha = 0;
    } else {
        vf->mtl->attrib.alpha = 1;
    }
    if (flags & 0x40) {
        vf->mtl->attrib.zmode = 0;
    } else if (flags & 0x20) {
        vf->mtl->attrib.zmode = 1;
    } else {
        vf->mtl->attrib.zmode = 3;
    }
    if (flags & 4) {
        vf->mtl->attrib.filter = 1;
    } else if (flags & 0x10) {
        vf->mtl->attrib.filter = 2;
    } else {
        vf->mtl->attrib.filter = 0;
    }
    vf->mtl->sort_pri = sortpri;
    NuMtlUpdate(vf->mtl);
    ((VUFNT_PC *)vf->psdata)->x = ((VUFNT_PC *)vf->psdata)->y = ((VUFNT_PC *)vf->psdata)->z = 0.0f;
    return vf;
}

// FUNCTION: BIONICLE 0x5379f0
void NuQFntMoveRS(RNDRSTREAM *rs, NUQFNT *fv, float x, float y, float z) {
    if (NuQFntCSMode != NUQFNT_CSMODE_ABSOLUTE) {
        x = qfnt_rezscale_w * x + qfnt_offscale_x;
        y = qfnt_rezscale_h * y + qfnt_offscale_y - *((VUFNT *)fv)->scale_y * ((VUFNT *)fv)->baseline;
    }
    ((VUFNT_PC *)((VUFNT *)fv)->psdata)->x = x;
    ((VUFNT_PC *)((VUFNT *)fv)->psdata)->y = y;
    ((VUFNT_PC *)((VUFNT *)fv)->psdata)->z = z;
}

/* Rebases a pointer in a block moved from vfrom to vto (inlined everywhere; dropped from
 * the PC executable). */
__inline void RemapAddr(void *vto, void *vfrom, void *vptr) {
    *(int *)vptr += (int)vto - (int)vfrom;
}

/* Loads a font: the pointer-block format, else "<name>.qfn" with the texture <texname>
 * (or <name>). The two ints aren't used. Name from saga. */
// FUNCTION: BIONICLE 0x537a50
NUQFNT *NuQFntLoadPtr(char *name, char *texname, int unknown3, int unknown4, VARIPTR *ptr, VARIPTR *end) {
    VUFNT *fv;
    VARIPTR oldptr;
    char filename[1024];
    int size;
    int tid;

    fv = NuQFntRead(name, ptr, *end);
    if (fv != NULL) {
        return fv;
    }
    oldptr = *ptr;
    ptr->intaddr = (ptr->intaddr + 15) & ~15;
    // STRING: BIONICLE 0x5e00b4
    sprintf(filename, "%s.qfn", name);
    size = NuFileLoadBuffer(filename, ptr->voidptr, end->intaddr - ptr->intaddr);
    if (size != 0) {
        fv = ptr->voidptr;
        RemapAddr(fv, NULL, &fv->chars);
        RemapAddr(fv, NULL, &fv->index_table);
        ptr->intaddr = (ptr->intaddr + size + 15) & ~15;
        fv->psdata = ptr->voidptr;
        ptr->intaddr += sizeof(VUFNT_PC);
        fv->scale_x = &((VUFNT_PC *)fv->psdata)->scale_x;
        fv->scale_y = &((VUFNT_PC *)fv->psdata)->scale_y;
        fv->abgr = &((VUFNT_PC *)fv->psdata)->abgr;
        if (texname == NULL) {
            // STRING: BIONICLE 0x5c72e0
            sprintf(filename, "%s", name);
        } else {
            sprintf(filename, "%s", texname);
        }
        tid = NuTexRead(filename, ptr, *end);
        if (tid != 0) {
            fv->mtl = NuMtlCreate(1);
            if (fv->mtl != NULL) {
                fv->mtl->diffuse.r = 1.0f;
                fv->mtl->diffuse.g = 1.0f;
                fv->mtl->diffuse.b = 1.0f;
                fv->mtl->attrib.cull = 2;
                fv->mtl->attrib.zmode = 3;
                fv->mtl->alpha = 1.0f;
                fv->mtl->attrib.alpha = 1;
                fv->mtl->attrib.colour = 1;
                fv->mtl->tid = tid;
                NuMtlUpdate(fv->mtl);
                return fv;
            }
            NuTexDestroy(tid);
        }
    }
    *ptr = oldptr;
    return NULL;
}

/* Loads the built-in font as the system font, once. Its slot in the PC name list
 * (NuQFntSet2d) was a guess; it takes the PS2 NuQFntInit's arguments and does its job. */
// FUNCTION: BIONICLE 0x538200
void NuQFntInit(VARIPTR *buff, VARIPTR buffend) {
    VARIPTR font;

    if (nuqfnt_init == 0) {
        nuqfnt_init = 1;
        font.voidptr = sysfont;
        system_qfont = NuQFntReadBuffer(&font, buff, buffend);
        if (system_qfont != NULL) {
            NuQFntSetICGap(system_qfont, 1.0f);
            NuQFntSet(system_qfont);
        }
    }
}

// FUNCTION: BIONICLE 0x538270
__inline void NuQFntMove(NUQFNT *fv, float x, float y, float z) {
    RNDRSTREAM *rs;

    if (fv == NULL) {
        fv = system_qfont;
    }
    if (fv != NULL) {
        rs = (NuQFntMode & 2) ? NULL : (RNDRSTREAM *)&((VUFNT *)fv)->mtl->rs;
        NuQFntMoveRS(rs, fv, x, y, z);
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

/* printf at (x, y); origin 1 right-aligns, 2 centres. */
// FUNCTION: BIONICLE 0x538350
void NuQFntPrintEx(NUQFNT *fnt, int x, int y, int origin, char *fmt, ...) {
    int len;
    char text[1024];
    int newx;
    va_list args;

    NuQFntPushPrintMode(2);
    va_start(args, fmt);
    vsprintf(text, fmt, args);
    len = (int)NuQFntPrintLenU(fnt, text);
    va_end(args);
    switch (origin) {
    case 0:
        newx = x;
        break;
    case 1:
        newx = x - len;
        break;
    case 2:
        newx = x - len / 2;
        break;
    default:
        newx = x;
        break;
    }
    NuQFntMove(fnt, newx, y, 0.0f);
    NuQFntPrintU(fnt, text);
    NuQFntPopPrintMode();
}
