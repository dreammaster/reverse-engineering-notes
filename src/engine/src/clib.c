/* ags/clib.h's own implementation. See that header for the full
 * scope/fidelity notes. Faithful port of Common/Clib32.cpp's
 * csetlib/read_new_format_clib/clibfindindex/clibfilesize/cliboffset/
 * clibopenfile/clibfopen, narrowed to this build's own confirmed
 * behavior (lib_version 6/10 only, no ci_fopen wrapper, the OLD
 * MultiFileLib layout directly -- see reversing/analysis/
 * matches.json's csetlib/read_new_format_clib/clibfindindex entries).
 */
#include "ags/clib.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Common/Clib32.cpp:76-77 */
static const char CLIB_END_SIG[13] = "CLIB\x01\x02\x03\x04SIGE";
static const char CLIB_PASSW_ENC_STRING[13] = "My\x01\xde\x04Jibzle";

/* The original's single-manifest global state (Clib32.cpp:44-78);
 * kept file-scope here the same way, matching the original design --
 * only one CLIB library can be open at a time. */
static struct MultiFileLib mflib;
static char lib_file_name[255] = " ";
static char base_path[255] = ".";
static char original_base_filename[255];
static long last_opened_size = -1;
static int g_lib_version = 0; /* not part of the original API; see ags/clib.h */

/* --- little-endian primitives -----------------------------------
 * The original uses getw()/fread() directly, relying on the host CRT
 * matching the format's own little-endian x86 layout. Implemented
 * explicitly here instead of via (deprecated, CRT-endianness-
 * dependent) getw() for portability/clarity -- functionally
 * identical on this project's own only-ever-x86 target. */
static int read_le32(FILE *f)
{
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) {
        return 0;
    }
    return (int)((unsigned int)b[0] | ((unsigned int)b[1] << 8) |
                 ((unsigned int)b[2] << 16) | ((unsigned int)b[3] << 24));
}

/* --- clib_decrypt_text (Clib32.cpp:91-106) ------------------------
 * Only exercised by read_new_format_clib's own libver>=11 branch,
 * which this build's csetlib (accepting only 6/10) never reaches --
 * ported anyway for fidelity, since it costs nothing and matches the
 * real function exactly. */
static void clib_decrypt_text(char *toenc)
{
    int adx = 0;
    for (;;) {
        toenc[0] = (char)(toenc[0] - CLIB_PASSW_ENC_STRING[adx]);
        if (toenc[0] == 0) {
            break;
        }
        adx++;
        toenc++;
        if (adx > 10) {
            adx = 0;
        }
    }
}

/* --- read_new_format_clib (Clib32.cpp:204-225) -------------------- */
static int read_new_format_clib(struct MultiFileLib *mfl, FILE *wout, int libver)
{
    mfl->num_data_files = read_le32(wout);
    if (mfl->num_data_files < 0 || mfl->num_data_files > AGS_CLIB_MAXMULTIFILES) {
        return -1;
    }
    if (fread(&mfl->data_filenames[0][0], 20, (size_t)mfl->num_data_files, wout) !=
        (size_t)mfl->num_data_files) {
        return -1;
    }

    mfl->num_files = read_le32(wout);
    if (mfl->num_files > AGS_CLIB_MAX_FILES) {
        return -1;
    }

    if (fread(&mfl->filenames[0][0], 25, (size_t)mfl->num_files, wout) != (size_t)mfl->num_files) {
        return -1;
    }
    if (fread(&mfl->offset[0], sizeof(long), (size_t)mfl->num_files, wout) != (size_t)mfl->num_files) {
        return -1;
    }
    if (fread(&mfl->length[0], sizeof(long), (size_t)mfl->num_files, wout) != (size_t)mfl->num_files) {
        return -1;
    }
    if (fread(&mfl->file_datafile[0], 1, (size_t)mfl->num_files, wout) != (size_t)mfl->num_files) {
        return -1;
    }

    if (libver >= 11) {
        int aa;
        for (aa = 0; aa < mfl->num_files; aa++) {
            clib_decrypt_text(mfl->filenames[aa]);
        }
    }
    return 0;
}

/* --- base_path/basename splitting ---------------------------------
 * Source's own version advances `namm` one character at a time as
 * long as a '\\'/'/' still exists somewhere ahead of it, which
 * converges on `namm` pointing just past the LAST separator --
 * functionally a "find the basename" operation. Implemented directly
 * via strrchr here instead (same end result, clearer to read). */
static void split_base_path(const char *path, char *out_base_path, size_t base_path_size,
                             const char **out_basename)
{
    const char *last_bs = strrchr(path, '\\');
    const char *last_fs = strrchr(path, '/');
    const char *last_sep = last_bs;
    if (last_fs && (!last_sep || last_fs > last_sep)) {
        last_sep = last_fs;
    }

    if (!last_sep) {
        *out_basename = path;
        return; /* base_path stays at its caller-supplied default (".") */
    }

    {
        size_t dirlen = (size_t)(last_sep - path);
        if (dirlen >= base_path_size) {
            dirlen = base_path_size - 1;
        }
        memcpy(out_base_path, path, dirlen);
        out_base_path[dirlen] = '\0';
    }
    *out_basename = last_sep + 1;
}

/* --- ags_csetlib (Clib32.cpp:227-355) ------------------------------ */
int ags_csetlib(const char *namm)
{
    FILE *fff;
    char clbuff[20];
    long absoffs = 0;
    int lib_version;
    const char *basename;
    int aa;

    original_base_filename[0] = 0;

    if (namm == NULL) {
        lib_file_name[0] = ' ';
        lib_file_name[1] = 0;
        return 0;
    }
    strcpy(base_path, ".");

    fff = fopen(namm, "rb"); /* ci_fopen CONFIRMED ABSENT in this build -- plain fopen */
    if (fff == NULL) {
        return -1;
    }

    if (fread(clbuff, 5, 1, fff) != 1) {
        fclose(fff);
        return -2;
    }

    if (strncmp(clbuff, "CLIB", 4) != 0) {
        fseek(fff, -12, SEEK_END);
        if (fread(clbuff, 12, 1, fff) != 1 || memcmp(clbuff, CLIB_END_SIG, 12) != 0) {
            fclose(fff);
            return -2;
        }
        fseek(fff, -16, SEEK_END);
        absoffs = read_le32(fff);
        fseek(fff, absoffs + 5, SEEK_SET);
    }

    lib_version = fgetc(fff);
    if (lib_version != 6 && lib_version != 10) {
        /* This build's own confirmed check is exactly this two-value
         * test -- 2011's reference source additionally accepts
         * 11/15/20/21, none of which this build's csetlib does. */
        fclose(fff);
        return -3;
    }
    g_lib_version = lib_version;

    split_base_path(namm, base_path, sizeof(base_path), &basename);

    if (lib_version == 10) {
        struct MultiFileLib mflibOld;

        if (fgetc(fff) != 0) { /* chain byte: must be the first datafile in the chain */
            fclose(fff);
            return -4;
        }

        if (read_new_format_clib(&mflibOld, fff, lib_version) != 0) {
            fclose(fff);
            return -5;
        }
        fclose(fff);

        /* No old-to-new-format conversion step: this build's own live
         * `mflib` IS the old MultiFileLib struct directly (see the
         * header's own fidelity note), so mflibOld can just become
         * the live manifest outright. */
        mflib = mflibOld;

        strcpy(lib_file_name, basename);

        strcpy(original_base_filename, mflib.data_filenames[0]);
        {
            char *p = original_base_filename;
            for (; *p; p++) {
                *p = (char)tolower((unsigned char)*p);
            }
        }

        strncpy(mflib.data_filenames[0], basename, sizeof(mflib.data_filenames[0]) - 1);
        mflib.data_filenames[0][sizeof(mflib.data_filenames[0]) - 1] = '\0';

        for (aa = 0; aa < mflib.num_files; aa++) {
            if (mflib.file_datafile[aa] == 0) {
                mflib.offset[aa] += absoffs;
            }
        }
        return 0;
    }

    /* lib_version == 6 */
    {
        int passwmodifier = fgetc(fff);
        short tempshort;
        int cc;

        fgetc(fff); /* unused byte */
        mflib.num_data_files = 1;
        strncpy(mflib.data_filenames[0], basename, sizeof(mflib.data_filenames[0]) - 1);
        mflib.data_filenames[0][sizeof(mflib.data_filenames[0]) - 1] = '\0';

        if (fread(&tempshort, 2, 1, fff) != 1) {
            fclose(fff);
            return -2;
        }
        mflib.num_files = tempshort;
        if (mflib.num_files > AGS_CLIB_MAX_FILES) {
            fclose(fff);
            return -4;
        }

        fread(clbuff, 13, 1, fff); /* skip password dooberry */
        for (aa = 0; aa < mflib.num_files; aa++) {
            fread(&mflib.filenames[aa][0], 13, 1, fff);
            for (cc = 0; cc < (int)strlen(mflib.filenames[aa]); cc++) {
                mflib.filenames[aa][cc] = (char)(mflib.filenames[aa][cc] - passwmodifier);
            }
        }
        fread(&mflib.length[0], 4, (size_t)mflib.num_files, fff);
        fseek(fff, 2 * mflib.num_files, SEEK_CUR); /* skip flags & ratio */

        mflib.offset[0] = ftell(fff);
        strcpy(lib_file_name, basename);
        fclose(fff);

        for (aa = 1; aa < mflib.num_files; aa++) {
            mflib.offset[aa] = mflib.offset[aa - 1] + mflib.length[aa - 1];
            mflib.file_datafile[aa] = 0;
        }
        mflib.file_datafile[0] = 0;
        return 0;
    }
}

int ags_clib_get_num_files(void)
{
    if (lib_file_name[0] == ' ') {
        return 0;
    }
    return mflib.num_files;
}

const char *ags_clib_get_file_name(int index)
{
    if (lib_file_name[0] == ' ') {
        return NULL;
    }
    if (index < 0 || index >= mflib.num_files) {
        return NULL;
    }
    return &mflib.filenames[index][0];
}

int ags_clib_find_index(const char *filename)
{
    int bb;
    if (lib_file_name[0] == ' ') {
        return -1;
    }
    for (bb = 0; bb < mflib.num_files; bb++) {
        if (_stricmp(mflib.filenames[bb], filename) == 0) {
            return bb;
        }
    }
    return -1;
}

long ags_clib_file_size(const char *filename)
{
    int idxx = ags_clib_find_index(filename);
    if (idxx >= 0) {
        return mflib.length[idxx];
    }
    return -1;
}

long ags_clib_offset(const char *filename)
{
    int idxx = ags_clib_find_index(filename);
    if (idxx >= 0) {
        return mflib.offset[idxx];
    }
    return -1;
}

const char *ags_clib_get_original_filename(void)
{
    return original_base_filename;
}

FILE *ags_clib_open_file(const char *filename, const char *mode)
{
    int bb;
    for (bb = 0; bb < mflib.num_files; bb++) {
        if (_stricmp(mflib.filenames[bb], filename) == 0) {
            char actfilename[250];
            FILE *tfil;
            snprintf(actfilename, sizeof(actfilename), "%s\\%s", base_path,
                      mflib.data_filenames[(unsigned char)mflib.file_datafile[bb]]);
            tfil = fopen(actfilename, mode); /* ci_fopen CONFIRMED ABSENT */
            if (tfil == NULL) {
                return NULL;
            }
            fseek(tfil, mflib.offset[bb], SEEK_SET);
            return tfil;
        }
    }
    return NULL; /* NOTE: the fallback-to-plain-fopen belongs to ags_clib_fopen, see ags/clib.h */
}

FILE *ags_clib_fopen(const char *filename, const char *mode)
{
    FILE *tfil;

    last_opened_size = -1;

    /* PR_DATAFIRST (this build's own confirmed default priority) */
    if (ags_clib_offset(filename) < 1 || mode[0] != 'r') {
        tfil = fopen(filename, mode);
    } else {
        tfil = ags_clib_open_file(filename, mode);
        if (tfil != NULL) {
            last_opened_size = ags_clib_file_size(filename);
        }
    }

    if (last_opened_size < 0 && tfil != NULL) {
        long cur = ftell(tfil);
        fseek(tfil, 0, SEEK_END);
        last_opened_size = ftell(tfil);
        fseek(tfil, cur, SEEK_SET);
    }

    return tfil;
}

long ags_clib_last_opened_size(void)
{
    return last_opened_size;
}

int ags_clib_get_lib_version(void)
{
    return g_lib_version;
}
