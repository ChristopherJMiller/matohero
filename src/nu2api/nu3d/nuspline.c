#include "nuspline.h"

#include "nustring.h"

/* Finding a scene's splines by name. Names and locals from the PS2 prototype; logic
 * checked against opensagadev/saga src/nu2api/nu3d/nuspline.c (GPL-3.0, a later engine).
 * Not in the PC exe (unreferenced): NuSplineList, NuSplineGetRandomPoint. */

// FUNCTION: BIONICLE 0x54c340
NUGSPLINE *NuSplineFind(NUGSCN *scene, char *name) {
    NUGSPLINE *sp;
    int n;

    if (scene != NULL) {
        sp = scene->splines;
        for (n = 0; n < scene->numsplines; sp++, n++) {
            if (NuStrICmp(name, sp->name) == 0) {
                return sp;
            }
        }
    }
    return NULL;
}

// FUNCTION: BIONICLE 0x54c390
int NuSplineFindAllSub(NUGSCN *scene, char *name, NUGSPLINE **tab, int maxcount) {
    NUGSPLINE *sp;
    int n;
    int count;

    if (maxcount <= 0 || scene == NULL) {
        return 0;
    }
    count = 0;
    sp = scene->splines;
    for (n = 0; n < scene->numsplines; sp++, n++) {
        if (NuStrIStr(sp->name, name) != NULL) {
            tab[count++] = sp;
            if (count >= maxcount) {
                break;
            }
        }
    }
    return count;
}

// FUNCTION: BIONICLE 0x54c3f0
int NuSplineFindAllBeg(NUGSCN *scene, char *name, NUGSPLINE **tab, int maxcount) {
    NUGSPLINE *sp;
    int n;
    int count;

    if (maxcount <= 0 || scene == NULL) {
        return 0;
    }
    count = 0;
    sp = scene->splines;
    for (n = 0; n < scene->numsplines; sp++, n++) {
        if (NuStrNICmp(name, sp->name, -1) == 0) {
            tab[count++] = sp;
            if (count >= maxcount) {
                break;
            }
        }
    }
    return count;
}
