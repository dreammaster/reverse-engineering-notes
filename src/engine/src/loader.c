/* ags/loader.h's own implementation. See that header for the full
 * fidelity notes.
 */
#include "ags/loader.h"
#include "ags/view.h"

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

/* --- M3 ("Meet the cast") ------------------------------------------ */

/* Common/CSRUN.CPP:2005-2020's own freadstring(): byte-by-byte until
 * a NUL terminator (no length prefix) -- read and discard, since
 * we're skipping past import/export NAMES here, not decoding the
 * script's own symbol table. */
static int skip_cstr(FILE *f)
{
    int c;
    do {
        c = fgetc(f);
        if (c == EOF) {
            return -1;
        }
    } while (c != 0);
    return 0;
}

int ags_skip_words_dictionary(FILE *f, const struct GameSetupStructBase *game)
{
    int num_words;
    int i;

    if (game->dict == NULL) {
        return 0;
    }

    if (read_le32(f, &num_words) != 0 || num_words < 0) {
        return -1;
    }

    for (i = 0; i < num_words; i++) {
        int wordlen;
        if (read_le32(f, &wordlen) != 0 || wordlen < 0) {
            return -1;
        }
        if (wordlen > 0 && fseek(f, wordlen, SEEK_CUR) != 0) {
            return -1;
        }
        if (fseek(f, 2, SEEK_CUR) != 0) { /* wordnum */
            return -1;
        }
    }
    return 0;
}

int ags_skip_unidentified_block(FILE *f)
{
    int skip;
    if (read_le32(f, &skip) != 0 || skip < 0) {
        return -1;
    }
    if (skip > 0 && fseek(f, skip, SEEK_CUR) != 0) {
        return -1;
    }
    return 0;
}

int ags_skip_compiled_script(FILE *f, struct AgsScriptBlockInfo *out_info)
{
    char sig[4];
    int endsig;
    int i;

    memset(out_info, 0, sizeof(*out_info));

    if (fread(sig, 1, 4, f) != 4) {
        return -1;
    }
    if (memcmp(sig, "SCOM", 4) != 0) {
        return -2;
    }

    if (read_le32(f, &out_info->fileVer) != 0) {
        return -1;
    }
    if (read_le32(f, &out_info->globaldatasize) != 0) {
        return -1;
    }
    if (read_le32(f, &out_info->codesize) != 0) {
        return -1;
    }
    if (read_le32(f, &out_info->stringssize) != 0) {
        return -1;
    }

    if (out_info->globaldatasize > 0 && fseek(f, out_info->globaldatasize, SEEK_CUR) != 0) {
        return -1;
    }
    if (out_info->codesize > 0 && fseek(f, (long)out_info->codesize * 4, SEEK_CUR) != 0) {
        return -1;
    }
    if (out_info->stringssize > 0 && fseek(f, out_info->stringssize, SEEK_CUR) != 0) {
        return -1;
    }

    if (read_le32(f, &out_info->numfixups) != 0) {
        return -1;
    }
    if (out_info->numfixups > 0) {
        if (fseek(f, out_info->numfixups, SEEK_CUR) != 0) { /* fixuptypes */
            return -1;
        }
        if (fseek(f, (long)out_info->numfixups * 4, SEEK_CUR) != 0) { /* fixups */
            return -1;
        }
    }

    if (read_le32(f, &out_info->numimports) != 0 || out_info->numimports < 0) {
        return -1;
    }
    for (i = 0; i < out_info->numimports; i++) {
        if (skip_cstr(f) != 0) {
            return -1;
        }
    }

    if (read_le32(f, &out_info->numexports) != 0 || out_info->numexports < 0) {
        return -1;
    }
    for (i = 0; i < out_info->numexports; i++) {
        if (skip_cstr(f) != 0) {
            return -1;
        }
        if (fseek(f, 4, SEEK_CUR) != 0) { /* export_addr */
            return -1;
        }
    }

    /* This build's own fileVer is always <83 (see fread_script's own
     * matches.json entry), so the numSections block 2011 adds here
     * never applies -- not read at all, matching the real disassembly. */

    if (read_le32(f, &endsig) != 0) {
        return -1;
    }
    if ((unsigned int)endsig != 0xBEEFCAFEu) {
        return -3;
    }

    return 0;
}

int ags_skip_views(FILE *f, const struct GameSetupStructBase *game)
{
    long bytes = (long)game->numviews * (long)sizeof(struct ViewStruct272);
    if (bytes > 0 && fseek(f, bytes, SEEK_CUR) != 0) {
        return -1;
    }
    return 0;
}

int ags_skip_unidentified_block2(FILE *f)
{
    int units;
    if (read_le32(f, &units) != 0 || units < 0) {
        return -1;
    }
    if (units > 0 && fseek(f, (long)units * 0x204, SEEK_CUR) != 0) {
        return -1;
    }
    return 0;
}

int ags_load_characters(FILE *f, const struct GameSetupStructBase *game, struct CharacterInfo *out)
{
    size_t n = (size_t)game->numcharacters * sizeof(struct CharacterInfo);
    if (game->numcharacters <= 0) {
        return 0;
    }
    if (fread(out, 1, n, f) != n) {
        return -1;
    }
    return 0;
}
