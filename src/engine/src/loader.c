/* ags/loader.h's own implementation. See that header for the full
 * fidelity notes.
 */
#include "ags/loader.h"

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

int ags_load_game_file_header(FILE *f, struct AgsGameFileHeader *out)
{
    int verlen;
    int to_copy;

    memset(out, 0, sizeof(*out));

    if (fread(out->teststr, 1, 30, f) != 30) {
        return -1;
    }
    out->teststr[30] = '\0';

    if (read_le32(f, &out->marker) != 0) {
        return -1;
    }
    if (out->marker != AGS_GAME_FILE_MARKER) {
        return -2;
    }

    if (read_le32(f, &verlen) != 0 || verlen < 0) {
        return -1;
    }

    to_copy = verlen;
    if (to_copy > (int)sizeof(out->verstr) - 1) {
        to_copy = (int)sizeof(out->verstr) - 1;
    }
    if (to_copy > 0 && fread(out->verstr, 1, (size_t)to_copy, f) != (size_t)to_copy) {
        return -1;
    }
    out->verstr[to_copy] = '\0';
    if (verlen > to_copy) {
        /* Skip whatever we didn't copy into the (fixed-size) verstr
         * buffer, so the file position ends up exactly where the
         * real verlen says regardless of any truncation above. */
        fseek(f, verlen - to_copy, SEEK_CUR);
    }

    return 0;
}

int ags_load_gamesetup(FILE *f, struct GameSetupStructBase *out)
{
    if (fread(out, 1, sizeof(*out), f) != sizeof(*out)) {
        return -1;
    }
    return 0;
}
