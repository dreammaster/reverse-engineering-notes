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
#include "ags/character.h"

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

/* --- M3 ("Meet the cast", see src/PLAN.md) -------------------------
 * Continues load_game_file's own exact byte-consumption sequence past
 * GameSetupStructBase to reach the real CharacterInfo array. Ported
 * from reversing/scripts/dump_characters_from_data.py's own already-
 * verified walk (calibrated the same way -- rb.exe specifically), in
 * turn built from fread_script's own matches.json entry and
 * Common/CSRUN.CPP:2029's real source (still present in this repo).
 * Call these five functions in order, on the SAME FILE* used for
 * ags_load_gamesetup() above, immediately after it succeeds:
 *   1. ags_skip_words_dictionary
 *   2. ags_skip_unidentified_block
 *   3. ags_skip_compiled_script
 *   4. ags_skip_views
 *   5. ags_skip_unidentified_block2
 *   6. ags_load_characters
 * Steps 1/2/4/5 are honest structural skips (matching this
 * milestone's own "skip the parts you don't need yet, but for
 * structural/byte-skipping reasons, not fake logic" principle) --
 * decoding WordsDictionary's/ViewStruct272's own contents is later
 * milestone work; we already have real struct headers for both
 * (ags/dialog.h, ags/view.h), just not a reason to populate them yet.
 * Step 3 additionally verifies the two self-checks the real format
 * provides (the "SCOM" signature, the 0xBEEFCAFE ENDFILESIG) even
 * though it doesn't decode the script's own bytecode either (that's
 * M4) -- a misaligned read fails loudly here rather than silently
 * corrupting the character data read afterward.
 */

/* Step 1: if game->dict is non-NULL (this build's own on-disk
 * presence-flag idiom -- see ags/gamesetup.h), skips past
 * read_dictionary's own on-disk format (num_words, then per word: a
 * length-prefixed word text + a 2-byte wordnum -- Common/
 * CSRUN.CPP:1552-1560/1541-1550, `read_string_decrypt`'s own
 * length-prefix without the decryption, which doesn't change the
 * byte count so doesn't matter for a skip). Returns 0 on success. */
int ags_skip_words_dictionary(FILE *f, const struct GameSetupStructBase *game);

/* Step 2: one getw()-prefixed forward skip (relative to the current
 * position) whose role isn't yet identified -- this build's own
 * load_game_file walks past it unconditionally either way. */
int ags_skip_unidentified_block(FILE *f);

struct AgsScriptBlockInfo {
    int fileVer;
    int globaldatasize;
    int codesize;
    int stringssize;
    int numfixups;
    int numimports;
    int numexports;
};

/* Step 3: skips past the compiled global script's own bytes
 * (fread_script's exact format, Common/CSRUN.CPP:2029-2122, this
 * build's own no-numSections variant -- fileVer is always <83 here,
 * see fread_script's own matches.json entry) without decoding the
 * bytecode itself (that's M4's job). Returns 0 on success, -1 on a
 * short/failed read, -2 if the leading 4-byte signature isn't "SCOM",
 * -3 if the trailing ENDFILESIG doesn't read as 0xBEEFCAFE -- either
 * mismatch means something upstream is misaligned and nothing read
 * afterward (including step 4/5/6) can be trusted. `*out_info` is
 * filled in even on success only (left zeroed on failure). */
int ags_skip_compiled_script(FILE *f, struct AgsScriptBlockInfo *out_info);

/* Step 4: game->numviews * sizeof(struct ViewStruct272) bytes, read
 * as one flat block per fread_script's own caller in load_game_file
 * -- skipped for now (decoding ViewStruct272's own contents is later
 * milestone work, though ags/view.h's struct is already verified). */
int ags_skip_views(FILE *f, const struct GameSetupStructBase *game);

/* Step 5: a second getw()-prefixed forward skip, this time SCALED by
 * 0x204 bytes per unit -- role not identified either. */
int ags_skip_unidentified_block2(FILE *f);

/* Step 6: reads game->numcharacters entries into `out` (caller-
 * allocated, at least that many struct CharacterInfo slots) via one
 * bulk read, matching the original's own "fread(chars,
 * sizeof(CharacterInfo)*numcharacters,1,iii)"-shaped call. Returns 0
 * on success, -1 on a short read. */
int ags_load_characters(FILE *f, const struct GameSetupStructBase *game, struct CharacterInfo *out);

#endif /* AGS_LOADER_H */
