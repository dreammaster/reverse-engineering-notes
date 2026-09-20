/* ags/loader.h -- M2 ("Read the header", see src/PLAN.md): parses
 * ac2game.dta's own small file-level header and reads the raw
 * GameSetupStructBase blob that follows it.
 *
 * Ported from load_game_file's own opening sequence (asm_name
 * load_ac2game_dta in reversing/analysis/matches.json -- the legacy
 * label kept there so apply_matches.py can still resolve the live
 * IDB entry; its real 2011 identity is `load_game_file`,
 * Engine/AC.CPP:11587 onward): "clibfopen("game28.dta","rb");
 * ...teststr[30] fread...filever check...fread(&game,
 * sizeof(GameSetupStructBase),1,iii)". The exact header shape (a
 * plaintext 30-byte signature, a 4-byte marker checked against the
 * literal 12, a 4-byte-length-prefixed "engine needs" version string)
 * was reverse-engineered against REAL game data by
 * reversing/scripts/dump_gamesetup_from_data.py rather than purely
 * from the disassembly -- this header shape is confirmed specifically
 * for rb.exe (the Windows release, this project's actual disassembly
 * target); the DOS release's ac2game.dat is loaded by a DIFFERENT
 * engine binary (ac.exe) with an unconfirmed, possibly different
 * header shape.
 *
 * Typically used together with ags/clib.h: locate and open
 * "ac2game.dta" via ags_clib_fopen(), then call
 * ags_load_game_file_header() followed immediately by
 * ags_load_gamesetup() on that same FILE*.
 */
#ifndef AGS_LOADER_H
#define AGS_LOADER_H

#include <stdio.h>

#include "ags/gamesetup.h"

/* This build's own load_game_file checks the header's marker field
 * against exactly this literal (per dump_gamesetup_from_data.py's own
 * docstring, calibrated against real rb.exe data). */
#define AGS_GAME_FILE_MARKER 12

struct AgsGameFileHeader {
    char teststr[31]; /* null-terminated copy of the 30-byte plaintext signature, e.g. "Adventure Creator Game File v2" */
    int marker;
    char verstr[64]; /* null-terminated copy of the length-prefixed "engine needs" version string, e.g. "2.3" (truncated if longer than 63 bytes -- no real version string is anywhere near that long) */
};

/* Reads ac2game.dta's own small file-level header from the CURRENT
 * position of `f`. Returns 0 on success, -1 on a short/failed read,
 * -2 if the marker field doesn't match AGS_GAME_FILE_MARKER (most
 * likely meaning a differently-shaped header than this function's
 * own rb.exe-calibrated assumptions -- see the file-level comment
 * above). On a -2, teststr/verstr are NOT filled in (the function
 * returns as soon as the mismatch is detected, matching the real
 * engine's own filever check happening before those fields would
 * otherwise be read here) and `f`'s position is left right after the
 * marker field. */
int ags_load_game_file_header(FILE *f, struct AgsGameFileHeader *out);

/* Reads exactly sizeof(struct GameSetupStructBase) raw bytes from the
 * CURRENT position of `f` (which must be positioned immediately after
 * ags_load_game_file_header's own read) into `out`. Returns 0 on
 * success, -1 on a short read. This is the first real exercise of
 * Step 0's own struct headers against actual game data -- if
 * GameSetupStructBase's packed layout is wrong in any way, every
 * field read back here will be garbage. */
int ags_load_gamesetup(FILE *f, struct GameSetupStructBase *out);

#endif /* AGS_LOADER_H */
