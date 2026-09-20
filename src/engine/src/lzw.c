/* ags/lzw.h's own implementation. See that header and Common/lzw.cpp/
 * Common/compress.cpp (both still present in this repo) for the exact
 * algorithms being ported.
 */
#include "ags/lzw.h"

#include <stdlib.h>

/* Common/lzw.cpp's own constants -- unchanged. */
#define AGS_LZW_N 4096
#define AGS_LZW_F 16

unsigned char *ags_lzw_expand_to_mem(FILE *f, long maxsize)
{
    unsigned char *membuff;
    unsigned char *outp;
    long putbytes = 0;
    char *lzbuffer;
    int bits, ch, i, j, len, mask;

    membuff = (unsigned char *)malloc((size_t)maxsize + 10);
    if (!membuff) {
        return NULL;
    }
    outp = membuff;

    lzbuffer = (char *)malloc(AGS_LZW_N);
    if (!lzbuffer) {
        free(membuff);
        return NULL;
    }
    i = AGS_LZW_N - AGS_LZW_F;

    /* Common/lzw.cpp:228-259's own outer/inner loop -- "this end
     * condition just checks for EOF, which is no good to us" (the
     * inner putbytes>=maxsize check is the real terminator). */
    while ((bits = getc(f)) != -1) {
        for (mask = 0x01; mask & 0xFF; mask <<= 1) {
            if (bits & mask) {
                short jshort = 0;
                if (fread(&jshort, sizeof(short), 1, f) != 1) {
                    free(lzbuffer);
                    free(membuff);
                    return NULL;
                }
                j = jshort;

                len = ((j >> 12) & 15) + 3;
                j = (i - j - 1) & (AGS_LZW_N - 1);

                while (len--) {
                    lzbuffer[i] = lzbuffer[j];
                    if (putbytes < maxsize) {
                        *outp++ = (unsigned char)lzbuffer[i];
                    }
                    putbytes++;
                    j = (j + 1) & (AGS_LZW_N - 1);
                    i = (i + 1) & (AGS_LZW_N - 1);
                }
            } else {
                ch = getc(f);
                lzbuffer[i] = (char)ch;
                if (putbytes < maxsize) {
                    *outp++ = (unsigned char)lzbuffer[i];
                }
                putbytes++;
                i = (i + 1) & (AGS_LZW_N - 1);
            }

            if (putbytes >= maxsize) {
                break;
            }
        }

        if (putbytes >= maxsize) {
            break;
        }
    }

    free(lzbuffer);
    return membuff;
}

int ags_cunpackbitl(unsigned char *line, int size, FILE *infile)
{
    int n = 0; /* number of bytes decoded */

    while (n < size) {
        int ix = fgetc(infile);
        if (ferror(infile)) {
            break;
        }

        {
            signed char cx = (signed char)ix;
            if (cx == -128) {
                cx = 0;
            }

            if (cx < 0) {
                /* run */
                int cnt = 1 - cx;
                int ch = fgetc(infile);
                while (cnt--) {
                    if (n >= size) {
                        return -1;
                    }
                    line[n++] = (unsigned char)ch;
                }
            } else {
                /* literal sequence */
                int cnt = cx + 1;
                while (cnt--) {
                    if (n >= size) {
                        return -1;
                    }
                    line[n++] = (unsigned char)fgetc(infile);
                }
            }
        }
    }

    return ferror(infile);
}
