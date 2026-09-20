/* ags/clib.h -- M1 ("Read the manifest", see src/PLAN.md): the CLIB
 * asset-library reader. Direct port of Common/Clib32.cpp's csetlib/
 * read_new_format_clib/clibfindindex/clibopenfile/clibfopen, adapted
 * to this build's own CONFIRMED simpler reality rather than the full
 * 2011 reference implementation:
 *
 *   - Only lib_version 6 and 10 are supported (this build's own
 *     csetlib: "if(lib_version!=6 && lib_version!=10) return -3;" --
 *     2011 additionally accepts 11/15/20/21, none of which this build
 *     understands, so read_new_new_format_clib/
 *     read_new_new_enc_format_clib aren't ported here at all).
 *   - The live in-memory manifest is the OLD `MultiFileLib` struct
 *     directly (Clib32.cpp:53-62) -- CONFIRMED via clibfindindex's
 *     own 25-byte filenames[] stride and clibopenfile's own 20-byte
 *     data_filenames[] stride -- not 2011's `MultiFileLibNew`
 *     (100-byte/50-byte strides). There is therefore no "convert old
 *     format to new format" step (Clib32.cpp:291-306) in this port at
 *     all -- this build has no newer format to convert TO.
 *   - `ci_fopen()` (2011's case-insensitive-path-resolution wrapper)
 *     is CONFIRMED ABSENT from this build's own csetlib/clibopenfile/
 *     clibfopen -- every one of them calls the plain CRT fopen()
 *     directly, matched here the same way.
 *
 * See reversing/analysis/matches.json (csetlib, read_new_format_clib,
 * clibfindindex) and reversing/scripts/parse_clib_manifest.py (the
 * already-verified Python prototype this C port's own test checks
 * against byte-for-byte).
 *
 * Not a Step-0-style serialized struct: MultiFileLib is pure runtime
 * state, never itself written to/read from a file as a raw blob (only
 * its INDIVIDUAL FIELDS are read field-by-field from the real CLIB
 * format, which this module already parses exactly) -- so, unlike
 * ags/gamesetup.h etc., there's no byte-packing/offsetof requirement
 * here, ordinary C struct layout is fine.
 */
#ifndef AGS_CLIB_H
#define AGS_CLIB_H

#include <stdio.h>

/* This build's own CONFIRMED capacity: read_new_format_clib's
 * "if(mfl->num_files>MAX_FILES) return -1" bounds check compiles to a
 * literal 0x7D0(2000) with zero drift (2011 later raised this to
 * 10000). MAXMULTIFILES (the data_filenames[] capacity) is NOT
 * independently confirmed against this build's own disassembly the
 * same way -- borrowed from 2011's declared value (25) as the best
 * available number; a real chained-datafile game could in principle
 * need more, but this project has never found evidence either way. */
#define AGS_CLIB_MAX_FILES 2000
#define AGS_CLIB_MAXMULTIFILES 25

struct MultiFileLib {
    char data_filenames[AGS_CLIB_MAXMULTIFILES][20];
    int num_data_files;
    char filenames[AGS_CLIB_MAX_FILES][25];
    long offset[AGS_CLIB_MAX_FILES];
    long length[AGS_CLIB_MAX_FILES];
    char file_datafile[AGS_CLIB_MAX_FILES];
    int num_files;
};

/* ags_csetlib -- port of csetlib(char*,char*) (Clib32.cpp:227-355).
 * The original's second parameter (passw) is never actually read by
 * this build's own confirmed simpler csetlib body, so it's dropped
 * here rather than carried as dead weight (every real call site in
 * 2011 itself only ever passes "" anyway, via the setlib() macro).
 *
 * Returns 0 on success, matching this build's own negative error
 * codes on failure: -1 open failed, -2 no CLIB signature found
 * (neither at file-start nor via the appended-to-EXE trailer), -3
 * unsupported lib_version, -4 too many files (or not first datafile
 * in a chain), -5 read_new_format_clib itself failed.
 *
 * Only one manifest can be open at a time (matching the original's
 * own single-global-`mflib` design) -- calling this again replaces
 * whatever was previously open. */
int ags_csetlib(const char *namm);

/* clibGetNumFiles/clibGetFileName */
int ags_clib_get_num_files(void);
const char *ags_clib_get_file_name(int index);

/* ags_clib_get_lib_version -- NOT part of the original API (the real
 * engine discards lib_version once the manifest is parsed, it's
 * never needed again). Exposed anyway since it costs nothing and is
 * useful for verification/logging. Returns 0 if nothing is loaded. */
int ags_clib_get_lib_version(void);

/* clibfindindex/clibfilesize/cliboffset */
int ags_clib_find_index(const char *filename);
long ags_clib_file_size(const char *filename);
long ags_clib_offset(const char *filename);

/* clibgetoriginalfilename -- only meaningful after a lib_version>=10
 * manifest is loaded (data_filenames[0] gets overwritten with the
 * caller's own path; this preserves what it originally said, matching
 * source's own backup-before-overwrite behavior). */
const char *ags_clib_get_original_filename(void);

/* ags_clib_open_file -- port of clibopenfile (Clib32.cpp:424-439):
 * opens filename's own underlying datafile (following
 * data_filenames[file_datafile[idx]], relative to the manifest's own
 * base directory) and seeks to its offset. Returns NULL if filename
 * isn't in the manifest, or if the underlying datafile can't be
 * opened -- unlike the original, this does NOT fall back to a plain
 * fopen(filename) on a lookup miss (that fallback belongs to
 * clibfopen, not clibopenfile, matching Clib32.cpp's own separation
 * of concerns; clibopenfile's own trailing "return ci_fopen(filly,
 * readmode);" on a total lookup miss is folded into ags_clib_fopen
 * below instead, for a cleaner match to what each function's NAME
 * promises). */
FILE *ags_clib_open_file(const char *filename, const char *mode);

/* ags_clib_fopen -- port of clibfopen (Clib32.cpp:445-480), the
 * function AGS/game code actually calls to open a named asset. Only
 * PR_DATAFIRST priority is implemented (this build's own confirmed
 * default; no PR_FILEFIRST call site found anywhere in the
 * disassembly). last_opened_size (source's own global) is returned
 * via ags_clib_last_opened_size() instead of a global the caller has
 * to know to read afterward. */
FILE *ags_clib_fopen(const char *filename, const char *mode);
long ags_clib_last_opened_size(void);

#endif /* AGS_CLIB_H */
