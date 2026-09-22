/* ags/loader.h's own implementation. See that header for the full
 * fidelity notes.
 */
#include "ags/loader.h"
#include "ags/view.h"

#include <stdio.h>
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

int ags_load_views(FILE *f, const struct GameSetupStructBase *game, struct ViewStruct272 *out)
{
    if (game->numviews <= 0) {
        return 0;
    }
    if (fread(out, sizeof(struct ViewStruct272), (size_t)game->numviews, f) != (size_t)game->numviews) {
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

/* --- M11 (see ags/loader.h's own file-level comment for the section
 * this belongs to) -------------------------------------------------- */

/* Common/cscommon.cpp:194-196's own fgetstring(): byte-by-byte until a
 * NUL terminator, into a caller buffer -- unlike skip_cstr() above,
 * this one KEEPS the bytes. Source has no length cap at all (a real,
 * confirmed overflow risk in the original -- see matches.json's own
 * "fgetstring" entry); this port caps at bufsize-1 as a deliberate
 * safety improvement that doesn't change behavior for any in-range
 * message (every real string this game ships is well under the
 * 500-byte allocations both callers below use). */
static int ags_fgetstring(FILE *f, char *buf, size_t bufsize)
{
    size_t i = 0;
    int c;

    for (;;) {
        c = fgetc(f);
        if (c == EOF) {
            return -1;
        }
        if (i < bufsize - 1) {
            buf[i] = (char)c;
        }
        if (c == 0) {
            return 0;
        }
        i++;
    }
}

int ags_load_messages(FILE *f, struct GameSetupStructBase *game)
{
    int i;

    for (i = 0; i < 500; i++) {
        char *buf;
        if (game->messages[i] == NULL) {
            continue;
        }
        buf = (char *)malloc(500);
        if (!buf) {
            return -1;
        }
        game->messages[i] = buf;
        if (ags_fgetstring(f, buf, 500) != 0) {
            return -1;
        }
    }
    return 0;
}

static void set_default_glmsg(struct GameSetupStructBase *game, int msgnum, const char *val)
{
    /* set_default_glmsg's own confirmed lazy-init pattern: only fill
     * in a slot the game data didn't already override. msgnum indexes
     * from MSG_RESTORE=983 (Engine/acdialog.h), i.e. messages[500] is
     * addressed as messages[msgnum-983]. */
    int idx = msgnum - 983;
    char *buf;
    if (idx < 0 || idx >= 500 || game->messages[idx] != NULL) {
        return;
    }
    buf = (char *)malloc(strlen(val) + 5); /* source's own literal "+5", not "+1" */
    if (!buf) {
        return;
    }
    strcpy(buf, val);
    game->messages[idx] = buf;
}

void ags_set_builtin_glmsg_defaults(struct GameSetupStructBase *game)
{
    /* The exact 12 (msgnum, text) pairs read directly from
     * load_game_file's own disassembly, in its own call order. */
    set_default_glmsg(game, 984, "Restore");
    set_default_glmsg(game, 985, "Cancel");
    set_default_glmsg(game, 986, "Select a game to restore:");
    set_default_glmsg(game, 987, "Save");
    set_default_glmsg(game, 988, "Type a name to save as:");
    set_default_glmsg(game, 989, "Replace");
    set_default_glmsg(game, 990, "The save directory is full. You must remove some saved games before you can save any more.");
    set_default_glmsg(game, 991, "Replace:");
    set_default_glmsg(game, 992, "With:");
    set_default_glmsg(game, 993, "Quit");
    set_default_glmsg(game, 994, "Play");
    set_default_glmsg(game, 995, "Are you sure you want to quit?");
}

int ags_load_dialog_topics(FILE *f, int numdialog, struct DialogTopic **out)
{
    struct DialogTopic *dlg;
    int i;

    *out = NULL;
    if (numdialog <= 0) {
        return 0;
    }

    dlg = (struct DialogTopic *)calloc((size_t)numdialog, sizeof(struct DialogTopic));
    if (!dlg) {
        return -1;
    }
    if (fread(dlg, sizeof(struct DialogTopic), (size_t)numdialog, f) != (size_t)numdialog) {
        free(dlg);
        return -1;
    }

    for (i = 0; i < numdialog; i++) {
        int off;

        if (dlg[i].optionscripts != NULL) {
            /* on-disk presence flag from the bulk fread above, same
             * idiom as ccScript/dict/messages[]. */
            size_t size = (size_t)dlg[i].codesize + 0xA;
            unsigned char *code = (unsigned char *)malloc(size);
            if (!code) {
                free(dlg);
                return -1;
            }
            dlg[i].optionscripts = code;
            if (fread(code, (size_t)dlg[i].codesize, 1, f) != 1) {
                free(dlg);
                return -1;
            }
        }

        /* Real behavior, purpose not yet identified -- see
         * ags/loader.h's own comment on this step. Walked exactly as
         * the disassembly does regardless of the branch above. */
        if (read_le32(f, &off) != 0) {
            free(dlg);
            return -1;
        }
        if (fseek(f, off, SEEK_CUR) != 0) {
            free(dlg);
            return -1;
        }
    }

    *out = dlg;
    return 0;
}

int ags_load_dlgmessages(FILE *f, int numdlgmessage, char ***out)
{
    char **arr;
    int i;

    *out = NULL;
    if (numdlgmessage < 0 || numdlgmessage > 2000) {
        return -1; /* source's own "too many dialog lines" quit() */
    }
    if (numdlgmessage == 0) {
        return 0;
    }

    arr = (char **)calloc((size_t)numdlgmessage, sizeof(char *));
    if (!arr) {
        return -1;
    }
    for (i = 0; i < numdlgmessage; i++) {
        arr[i] = (char *)malloc(500);
        if (!arr[i]) {
            free(arr);
            return -1;
        }
        if (ags_fgetstring(f, arr[i], 500) != 0) {
            free(arr);
            return -1;
        }
    }

    *out = arr;
    return 0;
}
