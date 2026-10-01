#ifndef NUHTML_H
#define NUHTML_H

#include "nufile.h"

/* HTML debug dumps written through a small buffer (PS2: nu3d/nuhtml.c). */

void NuHtmlBegin(NUFILE fh);
void NuHtmlWrite(char *msg, ...);
void NuHtmlHeading1(char *msg, ...);
void NuHtmlBanner(void);
void NuHtmlEnd(void);

#endif
