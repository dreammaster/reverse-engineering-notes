/* ags/script_loader.h -- M4 ("Run the interpreter on nothing", see
 * src/PLAN.md): a full (decoding, not skipping) port of fread_script
 * (Common/CSRUN.CPP:2029-2122, this build's own no-numSections
 * variant -- fileVer is always <83 here, see fread_script's own
 * matches.json entry). Unlike ags_skip_compiled_script (ags/loader.h,
 * M3), this allocates real ccScript storage and decodes the
 * bytecode/imports/exports for real, since M4 needs to actually run
 * it.
 */
#ifndef AGS_SCRIPT_LOADER_H
#define AGS_SCRIPT_LOADER_H

#include <stdio.h>

#include "ags/script.h"

enum AgsScriptLoadError {
    AGS_SCRIPT_LOAD_OK = 0,
    AGS_SCRIPT_LOAD_READ_ERROR = -1,
    AGS_SCRIPT_LOAD_BAD_SIGNATURE = -2, /* the leading 4 bytes weren't "SCOM" -- cc_error's own "file was not written by fwrite_script or seek position is incorrect" */
    AGS_SCRIPT_LOAD_BAD_ENDSIG = -3,    /* the trailing sentinel wasn't 0xBEEFCAFE -- cc_error's own "internal error rebuilding script" */
    AGS_SCRIPT_LOAD_TOO_MANY = -4       /* more imports/exports/fixups than this build's fixed 600-entry capacity (ags/script.h) */
};

/* Reads and fully decodes a compiled script from the CURRENT position
 * of `f`. Returns a malloc'd struct ccScript* on success (caller
 * owns it -- free with ags_cc_free_script()), NULL on failure with
 * *out_error set to why. */
struct ccScript *ags_cc_read_script(FILE *f, enum AgsScriptLoadError *out_error);

/* ccFreeScript(ccScript*) (Common/cscommon.cpp:116) -- a direct port,
 * added in M5 (src/PLAN.md) once room reloading needed a real
 * destructor for RoomStruct.compiled_script. Frees globaldata/code/
 * strings/fixuptypes/fixups (each individually null-checked) and
 * every non-NULL imports[]/every exports[] entry (source's own
 * exports loop has no null check either -- matched exactly), then
 * zeroes numimports/numexports, matching this build's own confirmed
 * simpler predecessor (no numSections loop, no free of the imports/
 * exports/export_addr ARRAYS themselves -- they're fixed embedded
 * arrays here, not 2011's separately malloc'd dynamic ones).
 * DELIBERATE DEVIATION: matches.json's own entry for the real
 * ccFreeScript confirms its body stops after zeroing numimports/
 * numexports -- it never frees the ccScript object itself, and
 * load_room's own disassembly (this build's one traced caller) never
 * does either, meaning the original engine leaks the (by-then-empty)
 * ccScript shell on every room reload. This port additionally frees
 * `scri` itself, since nothing in this codebase needs to keep a
 * freed script's empty shell around -- a real, intentional
 * improvement over the original's own behavior, not a faithfulness
 * gap. NULL-safe (a no-op on NULL). */
void ags_cc_free_script(struct ccScript *scri);

#endif /* AGS_SCRIPT_LOADER_H */
