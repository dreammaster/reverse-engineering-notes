/* ags/script_loader.h's own implementation. See that header and
 * Common/CSRUN.CPP:2005-2122 (freadstring/fread_script, both still
 * present in this repo) for the exact format being ported.
 */
#include "ags/script_loader.h"

#include <stdlib.h>
#include <string.h>

static int read_le32(FILE *f, int *out)
{
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) {
        return -1;
    }
    *out = (int)((unsigned int)b[0] | ((unsigned int)b[1] << 8) |
                 ((unsigned int)b[2] << 16) | ((unsigned int)b[3] << 24));
    return 0;
}

/* Common/CSRUN.CPP:2005-2020's own freadstring(): reads byte by byte
 * until a NUL terminator (bounded to a 300-byte local buffer, matching
 * source's own `static char ibuffer[300]`), returning a fresh malloc'd
 * copy, or NULL if the string was empty -- matching source's own
 * "if (ibuffer[0]==0) { strptr[0]=NULL; return; }", which is why so
 * many of a real script's import slots are NULL rather than an empty
 * string (see M4's own exploratory notes: this build's compiler
 * allocates import slots against a fixed "all known native symbols"
 * index space, and a given script leaves most of them blank). */
static int freadstring(FILE *f, char **out)
{
    char ibuffer[300];
    int idx = 0;
    int c;

    for (;;) {
        c = fgetc(f);
        if (c == EOF) {
            return -1;
        }
        if (idx < (int)sizeof(ibuffer) - 1) {
            ibuffer[idx] = (char)c;
        }
        if (c == 0) {
            break;
        }
        idx++;
    }
    ibuffer[(idx < (int)sizeof(ibuffer) - 1) ? idx : (int)sizeof(ibuffer) - 1] = '\0';

    if (ibuffer[0] == '\0') {
        *out = NULL;
        return 0;
    }
    *out = (char *)malloc(strlen(ibuffer) + 1);
    if (*out == NULL) {
        return -1;
    }
    strcpy(*out, ibuffer);
    return 0;
}

struct ccScript *ags_cc_read_script(FILE *f, enum AgsScriptLoadError *out_error)
{
    struct ccScript *scri;
    char sig[4];
    int fileVer;
    unsigned int endsig;
    int i;

    *out_error = AGS_SCRIPT_LOAD_OK;

    scri = (struct ccScript *)calloc(1, sizeof(struct ccScript));
    if (!scri) {
        *out_error = AGS_SCRIPT_LOAD_READ_ERROR;
        return NULL;
    }

#define FAIL(code) do { *out_error = (code); free(scri); return NULL; } while (0)

    if (fread(sig, 1, 4, f) != 4) {
        FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
    }
    if (memcmp(sig, "SCOM", 4) != 0) {
        FAIL(AGS_SCRIPT_LOAD_BAD_SIGNATURE);
    }

    if (read_le32(f, &fileVer) != 0) {
        FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
    }
    {
        int globaldatasize, codesize, stringssize;
        if (read_le32(f, &globaldatasize) != 0) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
        scri->globaldatasize = globaldatasize;
        if (read_le32(f, &codesize) != 0) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
        scri->codesize = codesize;
        if (read_le32(f, &stringssize) != 0) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
        scri->stringssize = stringssize;
    }

    if (scri->globaldatasize > 0) {
        scri->globaldata = (char *)malloc((size_t)scri->globaldatasize);
        if (!scri->globaldata ||
            fread(scri->globaldata, 1, (size_t)scri->globaldatasize, f) != (size_t)scri->globaldatasize) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
    }

    if (scri->codesize > 0) {
        scri->code = (unsigned long *)malloc((size_t)scri->codesize * sizeof(long));
        if (!scri->code ||
            fread(scri->code, sizeof(long), (size_t)scri->codesize, f) != (size_t)scri->codesize) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
    }

    if (scri->stringssize > 0) {
        scri->strings = (char *)malloc((size_t)scri->stringssize);
        if (!scri->strings ||
            fread(scri->strings, 1, (size_t)scri->stringssize, f) != (size_t)scri->stringssize) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
    }

    {
        int numfixups;
        if (read_le32(f, &numfixups) != 0 || numfixups < 0) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
        scri->numfixups = numfixups;
    }
    if (scri->numfixups > 0) {
        scri->fixuptypes = (char *)malloc((size_t)scri->numfixups);
        scri->fixups = (long *)malloc((size_t)scri->numfixups * sizeof(long));
        if (!scri->fixuptypes || !scri->fixups ||
            fread(scri->fixuptypes, 1, (size_t)scri->numfixups, f) != (size_t)scri->numfixups ||
            fread(scri->fixups, sizeof(long), (size_t)scri->numfixups, f) != (size_t)scri->numfixups) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
    }

    {
        int numimports;
        if (read_le32(f, &numimports) != 0 || numimports < 0) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
        if (numimports > AGS_CC_MAX_IMPORTS) {
            FAIL(AGS_SCRIPT_LOAD_TOO_MANY);
        }
        scri->numimports = numimports;
    }
    for (i = 0; i < scri->numimports; i++) {
        char *imp;
        if (freadstring(f, &imp) != 0) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
        scri->imports[i] = imp;
    }

    {
        int numexports;
        if (read_le32(f, &numexports) != 0 || numexports < 0) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
        if (numexports > AGS_CC_MAX_EXPORTS) {
            FAIL(AGS_SCRIPT_LOAD_TOO_MANY);
        }
        scri->numexports = numexports;
    }
    for (i = 0; i < scri->numexports; i++) {
        char *exp;
        int addr;
        if (freadstring(f, &exp) != 0) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
        scri->exports[i] = exp;
        if (read_le32(f, &addr) != 0) {
            FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
        }
        scri->export_addr[i] = addr;
    }

    /* This build's own fileVer is always <83 (fread_script's own
     * matches.json entry), so 2011's numSections block never applies
     * here -- not read at all, matching the real disassembly. */

    if (read_le32(f, (int *)&endsig) != 0) {
        FAIL(AGS_SCRIPT_LOAD_READ_ERROR);
    }
    if (endsig != 0xBEEFCAFEu) {
        FAIL(AGS_SCRIPT_LOAD_BAD_ENDSIG);
    }

#undef FAIL

    scri->instances = 0;
    return scri;
}
