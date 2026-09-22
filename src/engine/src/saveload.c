/* ags/saveload.h's own implementation. See that header for the
 * complete real file-format citation and scope decision.
 */
#include "ags/saveload.h"

#include <stdio.h>
#include <string.h>

static const char SAVE_SIGNATURE[] = "Adventure Game Studio saved game";
#define SAVE_VERSION 7 /* this build's own confirmed literal getw() check */

/* Common/cscommon.cpp:174-181's own fputstring(): a byte-by-byte
 * fputc loop over the string, then a final fputc(0) for the
 * terminator -- fgetstring's own write-side counterpart. */
static void ags_fputstring(FILE *f, const char *s)
{
    while (*s) {
        fputc((unsigned char)*s, f);
        s++;
    }
    fputc(0, f);
}

/* Common/cscommon.cpp:194-196's own fgetstring(): byte-by-byte until
 * a NUL terminator, into a caller buffer of `bufsize` bytes (this
 * build's own real signature has no explicit cap at all -- see
 * ags/loader.c's own ags_fgetstring for the same, already-established
 * deliberate safety bound this port applies uniformly). */
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

static void make_filename(char *out, int slotnum)
{
    sprintf(out, "agssave.%03d", slotnum);
}

enum AgsSaveLoadError ags_save_game_slot(int slotnum, const char *description,
                                          const struct GameState *play,
                                          const struct CharacterInfo *chars, int numcharacters,
                                          const struct DialogTopic *dialogs, int numdialog,
                                          int current_room)
{
    char filename[32];
    FILE *f;
    int i;

    make_filename(filename, slotnum);
    f = fopen(filename, "wb");
    if (!f) {
        return AGS_SAVE_OPEN_ERROR;
    }

    fwrite(SAVE_SIGNATURE, sizeof(SAVE_SIGNATURE), 1, f); /* includes the trailing NUL, matching source's own strlen+1-sized ElementSize */
    ags_fputstring(f, description ? description : "");
    {
        int version = SAVE_VERSION;
        fwrite(&version, sizeof(int), 1, f); /* putw(7,f) */
    }

    /* This module's own addition -- see ags/saveload.h's own
     * file-level comment for why (no other validation/room-reload
     * mechanism exists yet in this engine). Written and validated
     * BEFORE the variable-length chars[]/dialogs[] data below, so a
     * wrong caller-supplied count is caught cleanly instead of
     * silently misaligning every read that follows. */
    {
        int hdr[3];
        hdr[0] = numcharacters;
        hdr[1] = numdialog;
        hdr[2] = current_room;
        fwrite(hdr, sizeof(int), 3, f);
    }

    fwrite(play, sizeof(struct GameState), 1, f); /* source's own literal "ElementSize=0x964" */
    fwrite(chars, sizeof(struct CharacterInfo), (size_t)numcharacters, f);
    for (i = 0; i < numdialog; i++) {
        fwrite(dialogs[i].optionflags, sizeof(int), 15, f); /* source's own literal "ElementCount=0xF,ElementSize=4" */
    }

    fclose(f);
    return AGS_SAVE_OK;
}

/* Shared real header read (signature/description/version), matching
 * restore_game_data's own opening instructions exactly -- both real
 * callers below use it, mirroring the original's own "one fused
 * function, selected by whether a description buffer was passed"
 * design at the call-site level instead (kept as two separate C
 * functions here since neither needs the other's full body, and a
 * single function with a NULL-meaning-"do the extra part" parameter
 * reads less clearly in C than in the original's own idiom). */
static enum AgsSaveLoadError read_header(FILE *f, char *out_description)
{
    char sig[64];
    int version;

    if (fread(sig, 1, sizeof(SAVE_SIGNATURE), f) != sizeof(SAVE_SIGNATURE)) {
        fclose(f);
        return AGS_SAVE_NOT_A_SAVE_FILE;
    }
    sig[sizeof(SAVE_SIGNATURE) - 1] = '\0';
    if (strcmp(sig, SAVE_SIGNATURE) != 0) {
        fclose(f);
        return AGS_SAVE_NOT_A_SAVE_FILE;
    }

    if (ags_fgetstring(f, out_description, 200) != 0) {
        fclose(f);
        return AGS_SAVE_NOT_A_SAVE_FILE;
    }

    if (fread(&version, sizeof(int), 1, f) != 1 || version != SAVE_VERSION) {
        fclose(f);
        return AGS_SAVE_BAD_VERSION;
    }
    return AGS_SAVE_OK;
}

enum AgsSaveLoadError ags_get_save_slot_description(int slotnum, char *out_description)
{
    char filename[32];
    FILE *f;
    enum AgsSaveLoadError err;

    make_filename(filename, slotnum);
    f = fopen(filename, "rb");
    if (!f) {
        return AGS_SAVE_OPEN_ERROR;
    }

    err = read_header(f, out_description); /* closes f itself on any error */
    if (err != AGS_SAVE_OK) {
        return err;
    }
    fclose(f); /* GetSaveSlotDescription's own real early-exit -- no further reads */
    return AGS_SAVE_OK;
}

enum AgsSaveLoadError ags_restore_game_data(int slotnum, struct GameState *play,
                                             struct CharacterInfo *chars, int numcharacters,
                                             struct DialogTopic *dialogs, int numdialog,
                                             int *out_current_room)
{
    char filename[32];
    char description[200];
    FILE *f;
    enum AgsSaveLoadError err;
    int i;
    int hdr[3];

    make_filename(filename, slotnum);
    f = fopen(filename, "rb");
    if (!f) {
        return AGS_SAVE_OPEN_ERROR;
    }

    err = read_header(f, description); /* closes f itself on any error */
    if (err != AGS_SAVE_OK) {
        return err;
    }

    if (fread(hdr, sizeof(int), 3, f) != 3) {
        fclose(f);
        return AGS_SAVE_NOT_A_SAVE_FILE;
    }
    if (hdr[0] != numcharacters || hdr[1] != numdialog) {
        fclose(f);
        return AGS_SAVE_COUNT_MISMATCH; /* the real "!Restore_Game: Game has changed" case -- caught BEFORE misaligning any further read */
    }

    if (fread(play, sizeof(struct GameState), 1, f) != 1) {
        fclose(f);
        return AGS_SAVE_NOT_A_SAVE_FILE;
    }
    if (fread(chars, sizeof(struct CharacterInfo), (size_t)numcharacters, f) != (size_t)numcharacters) {
        fclose(f);
        return AGS_SAVE_NOT_A_SAVE_FILE;
    }
    for (i = 0; i < numdialog; i++) {
        if (fread(dialogs[i].optionflags, sizeof(int), 15, f) != 15) {
            fclose(f);
            return AGS_SAVE_NOT_A_SAVE_FILE;
        }
    }
    fclose(f);

    if (out_current_room) {
        *out_current_room = hdr[2];
    }
    return AGS_SAVE_OK;
}
