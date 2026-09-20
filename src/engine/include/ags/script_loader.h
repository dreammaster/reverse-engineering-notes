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
 * owns it -- there's no ags_cc_free_script() yet; M4 is a first real
 * test of the interpreter, not production memory management yet),
 * NULL on failure with *out_error set to why. */
struct ccScript *ags_cc_read_script(FILE *f, enum AgsScriptLoadError *out_error);

#endif /* AGS_SCRIPT_LOADER_H */
