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
#include "ags/view.h"
#include "ags/dialog.h"

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

/* Step 4, decoding variant (M7, "Meet the room's people", see
 * src/PLAN.md): the same game->numviews-sized flat block
 * ags_skip_views jumps over, but read for real into caller-allocated
 * `out` (at least game->numviews slots) -- one bulk fread per view,
 * matching ViewStruct272's own already byte-verified packed layout
 * (Step 0) exactly, since the on-disk format IS that struct's own
 * raw bytes (already proven by ags_skip_views's own successful
 * byte-count skip in M3/M4/M5/M6's test runs). Returns 0 on success,
 * -1 on a short read. */
int ags_load_views(FILE *f, const struct GameSetupStructBase *game, struct ViewStruct272 *out);

/* Step 5: a second getw()-prefixed forward skip, this time SCALED by
 * 0x204 bytes per unit -- role not identified either. */
int ags_skip_unidentified_block2(FILE *f);

/* Step 6: reads game->numcharacters entries into `out` (caller-
 * allocated, at least that many struct CharacterInfo slots) via one
 * bulk read, matching the original's own "fread(chars,
 * sizeof(CharacterInfo)*numcharacters,1,iii)"-shaped call. Returns 0
 * on success, -1 on a short read. */
int ags_load_characters(FILE *f, const struct GameSetupStructBase *game, struct CharacterInfo *out);

/* --- M11 ("the long tail" / GUI rendering, see src/PLAN.md) --------
 * load_game_file's own sequence continues past CharacterInfo[] with
 * three more variable-length sections before it finally reaches
 * read_gui (see ags/gui_loader.h) -- all read here directly from
 * rob_blanc_1.asm (load_game_file, the region right after the
 * already-verified character-array fread), since matches.json's own
 * prose only covered load_game_file up through CharacterInfo[]
 * ("extend past step 8 ... if a future round needs to validate").
 * Call these three in order immediately after ags_load_characters:
 *   7. ags_load_messages
 *   8. ags_load_dialog_topics
 *   9. ags_load_dlgmessages
 * then game->numgui is finally at its real on-disk value and
 * ags_load_guis (ags/gui_loader.h) can run.
 */

/* Step 7: GameSetupStructBase.messages[500] (already bulk-read as
 * part of ags_load_gamesetup -- each non-NULL slot is this build's
 * own on-disk "populate this slot" presence flag, the same idiom as
 * ccScript/dict) each get a fresh malloc(500)+fgetstring(f) in file
 * order. Matches load_game_file's own "for(i=0;i<500;i++)
 * if(messages[i]!=0){messages[i]=malloc(500); fgetstring(messages[i],
 * f);}" loop exactly (disassembly read directly, no 2011 source
 * counterpart named for this specific loop). Immediately followed
 * (real behavior, not modeled here since it touches no file bytes) by
 * 12 unconditional built-in-message defaults (MSG_RESTORE=984 through
 * MSG_QUITDIALOG=995, matching set_default_glmsg's own already-
 * confirmed 12 call sites) -- call ags_set_builtin_glmsg_defaults()
 * separately if those defaults matter to a caller; skipped by default
 * here since Rob Blanc 1's own real data always overrides all 12 (see
 * struct-layout-drift.md). Returns 0 on success, -1 on a short read
 * or allocation failure. Caller owns freeing any newly-malloc'd
 * messages[i] afterward. */
int ags_load_messages(FILE *f, struct GameSetupStructBase *game);

/* Sets the 12 built-in default global messages (MSG_RESTORE=984
 * through MSG_QUITDIALOG=995) on any still-NULL game->messages[]
 * slot, matching set_default_glmsg's own lazy-init malloc(strlen+5)
 * pattern. Optional -- see ags_load_messages's own comment. */
void ags_set_builtin_glmsg_defaults(struct GameSetupStructBase *game);

/* Step 8: game->numdialog DialogTopic entries (ags/dialog.h), read as
 * one bulk fread(dialog,0x484,numdialog,f) matching load_game_file's
 * own call exactly, followed per-topic by a conditional codesize-sized
 * fread into a freshly malloc'd optionscripts buffer (gated on the
 * bulk-read's own optionscripts field being non-NULL, the on-disk
 * presence-flag idiom again) AND, unconditionally per topic regardless
 * of that gate, one more getw()-prefixed forward fseek(SEEK_CUR) whose
 * role isn't identified (this build's own load_game_file walks past it
 * either way -- same "unidentified skip, walked but not decoded"
 * status as ags_skip_unidentified_block/2 above). `*out` receives a
 * freshly malloc'd array of `numdialog` entries (caller must free it,
 * and each non-NULL .optionscripts, when done). Returns 0 on success,
 * -1 on a short read or allocation failure. */
int ags_load_dialog_topics(FILE *f, int numdialog, struct DialogTopic **out);

/* Step 9: game->numdlgmessage (a separate GameSetupStructBase field
 * from numdialog -- legacy free-text dialog lines, distinct from
 * DialogTopic's own inline optionnames[]) consecutive malloc(500)+
 * fgetstring(f) strings, gated by a literal "numdlgmessage > 2000 =>
 * quit(\"too many dialog lines\")" bounds check matching
 * load_game_file's own disassembly exactly. `*out` receives a freshly
 * malloc'd `char*[numdlgmessage]` array (caller must free each string
 * and the array itself). Returns 0 on success, -1 on a short read,
 * allocation failure, or numdlgmessage>2000. */
int ags_load_dlgmessages(FILE *f, int numdlgmessage, char ***out);

#endif /* AGS_LOADER_H */
