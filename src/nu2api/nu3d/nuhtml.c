#include "nuhtml.h"

#include "nustring.h"

#include <stdarg.h>
#include <stdio.h>

/* HTML debug dumps (PS2: nu3d/nuhtml.c): text collects in a buffer that is written out
 * once it passes 3 KB. Names from the PS2 prototype (buffer, txt and flush are globals
 * there; static here, as the names are generic). The PS2's graphs, bitmaps and other
 * headings aren't in the PC exe. In PC order. */

// GLOBAL: BIONICLE 0x6170c4
char unknown[8] = "unknown";

// GLOBAL: BIONICLE 0x34b6bcc
NUFILE hfh;

// GLOBAL: BIONICLE 0x357c6c0
static char buffer[4096];

// GLOBAL: BIONICLE 0x357c6a0
static char *txt;

// GLOBAL: BIONICLE 0x357c6a4
static char *flush;

/* Inlined everywhere, so the linker dropped it from the PC exe. */
__inline void NuHtmlFlush(int force) {
    if ((txt > flush) | force) {
        NuFileWriteString(hfh, buffer);
        txt = buffer;
        flush = buffer + 3072;
    }
}

// FUNCTION: BIONICLE 0x55b910
void NuHtmlBegin(NUFILE fh) {
    hfh = fh;
    txt = buffer;
    flush = buffer + 3072;
}

// FUNCTION: BIONICLE 0x55b930
void NuHtmlWrite(char *msg, ...) {
    va_list args;

    if (msg == NULL || *msg == '\0') {
        msg = unknown;
    }
    va_start(args, msg);
    vsprintf(txt, msg, args);
    txt += NuStrLen(txt);
    va_end(args);
    NuHtmlFlush(0);
}

// FUNCTION: BIONICLE 0x55b9b0
void NuHtmlHeading1(char *msg, ...) {
    va_list args;

    if (msg == NULL || *msg == '\0') {
        msg = unknown;
    }
    // STRING: BIONICLE 0x5e06b0
    NuHtmlWrite("<table width=100%c bgcolor=#CFCFE5><tr><td><font face=arial size=+3> ", '%');
    va_start(args, msg);
    vsprintf(txt, msg, args);
    txt += NuStrLen(txt);
    va_end(args);
    // STRING: BIONICLE 0x5e069c
    NuHtmlWrite("</font></table>\n");
}

// FUNCTION: BIONICLE 0x55ba10
void NuHtmlBanner(void) {
    // STRING: BIONICLE 0x5e06f8
    NuHtmlWrite("<table   width=100%c height=20 bgcolor=#FF0000><tr><td><font face=arial size=+2></font></table>", '%');
}

// FUNCTION: BIONICLE 0x55ba20
void NuHtmlEnd(void) {
    NuHtmlFlush(1);
    hfh = 0;
}
