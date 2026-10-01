#include "nufile.h"

/* Functions from nufile.c kept in their own file for now: inside nufile.c they change
 * which functions the compiler defers and inlines, and matched neighbours regress (see
 * docs/matching.md, "Inlining depends on what else is in the file"). Here they can't
 * inline nufile.c's __inline helpers, so they don't match, but the code is complete. */

extern NUDATFILE datfiles[20];
extern int nufile_lsn_allowed;

int NuDatFileSeek(NUFILE fh, int offset, int origin);

// FUNCTION: BIONICLE 0x567c80
int NuDatFileLoadBuffer(NUDATHDR *ndh, char *fname, void *mem, int buffsize) {
    char *loadaddr;
    int size;
    int psize;
    int rv;
    NUFILE fh;

    if (nufile_lsn_allowed && ndh->start_lsn) {
        rv = NuDatFileLoadBufferLSN(ndh, fname, mem, buffsize);
        if (rv >= 0) {
            return rv;
        }
    }
    fh = NuDatFileOpen(ndh, fname, NUFILE_READ);
    if (fh != 0) {
        psize = datfiles[fh - 0x800].len;
        if (datfiles[fh - 0x800].ppack) {
            size = datfiles[fh - 0x800].uplen;
            loadaddr = (char *)mem + size - psize + 0x100;
            if (size + 0x100 <= buffsize && size != 0) {
                while (NuDatFileRead(fh, loadaddr, psize) < 0) {
                    while (NuDatFileSeek(fh, 0, 0) < 0) {
                    }
                }
                NuFileClose(fh);
                NuPPUnpack(loadaddr, (char *)mem);
                return size;
            }
        } else if (psize <= buffsize && psize != 0) {
            while (NuDatFileRead(fh, mem, psize) < 0) {
                while (NuDatFileSeek(fh, 0, 0) < 0) {
                }
            }
            NuFileClose(fh);
            return psize;
        }
        NuFileClose(fh);
    }
    return 0;
}
