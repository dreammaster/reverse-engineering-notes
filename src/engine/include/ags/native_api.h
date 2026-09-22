/* ags/native_api.h -- M11 ("the long tail" / the remaining script-
 * API entries, see src/PLAN.md): wires the script interpreter's own
 * SCMD_CALLEXT opcode (ags/interp.c) to REAL engine functions instead
 * of unconditionally stubbing every native call.
 *
 * BACKGROUND: M4's own interp.c already logs, via ags_stub_hit, every
 * native function a compiled script calls -- that log is exactly
 * PLAN.md's own "data-driven priority list" for this milestone. But
 * until now, CALLEXT always returned a bare 0 regardless of which
 * function was named, even for native calls this project has ALREADY
 * built a real C implementation of elsewhere (ags_display_message,
 * ags_add_inventory, ags_play_sound, ...) -- meaning the game's own
 * compiled script could never actually DRIVE any of M9-M11's own
 * real engine logic, only this project's own test harnesses could.
 * This module closes that loop: ags_native_api_call is the single
 * dispatch point CALLEXT now goes through, mapping an import's name
 * to a real handler wherever one already exists, and falling back to
 * the same ags_stub_hit logging as before for anything that doesn't
 * (so the backlog stays visible and honest, not silently widened).
 *
 * CALLING CONVENTION (confirmed directly from ags_cc_run_code's own
 * SCMD_PUSHREAL/SCMD_CALLEXT handling, ags/interp.c): arguments are
 * raw `long`s in push order (args[0] is the FIRST pushed/leftmost
 * argument); an int/bool/enum argument's value IS the long value
 * directly; a string argument's value is a REAL, already-fixed-up
 * pointer into the instance's own globaldata/stack/strings memory
 * (FIXUP_STRING adds the real `cinst->strings` base address at
 * instance-creation time -- see ags/interp.c's own FIXUP_STRING case)
 * -- `(const char *)(size_t)(unsigned)args[i]` is directly
 * dereferenceable with no marshaling layer needed.
 *
 * SCOPE, this round: covers every import Rob Blanc 1's own real
 * compiled global script actually references (confirmed via
 * test_interpreter.c's own printed import list) that this project
 * already has a real, evidence-cited implementation for elsewhere.
 * ObjectOn/ObjectOff joined this table once ags/roomobj.h existed
 * (M11+'s own room-objects slice). Left unmapped (falls to the same
 * stub-logging path as before, cleanly documented rather than guessed
 * at): AnimateObject/MoveObject (real per-frame view-animation-
 * stepping/pathfinding-based movement, not built yet -- ags/roomobj.h
 * only covers RoomObject's own static, non-moving state so far),
 * RestoreGameDialog/SaveGameDialog (CSCI legacy dialog controls,
 * explicitly out of scope per src/PLAN.md's own "Explicitly deferred"
 * list), Debug (no real per-command behavior traced).
 */
#ifndef AGS_NATIVE_API_H
#define AGS_NATIVE_API_H

#include "ags/interaction.h"
#include "ags/gamesetup.h"
#include "ags/sprite_loader.h"
#include "ags/dialog_run.h"

/* Bundles every piece of engine state a mapped native call might
 * need to touch, deliberately no bigger than that (same convention
 * as ags/interaction.h's own AgsGameContext/ags/dialog_run.h's own
 * AgsDialogRunContext). Any field may be NULL/0 if the caller has
 * nothing real to supply for it -- a native call needing a field
 * that's NULL falls back to logging via ags_stub_hit rather than
 * dereferencing a null pointer. `game_paused`/`cur_cursor` are this
 * build's own confirmed STANDALONE globals (not GameState members --
 * see matches.json's own IsGamePaused/GetCursorMode/SetCursorMode
 * entries), owned by this context since nothing else in this engine
 * tracks them yet. */
struct AgsNativeApiContext {
    struct AgsGameContext *game_ctx;   /* rst/player/player_walk/quit_requested/play */
    struct GameSetupStructBase *game;
    struct AgsSpriteSet *sprites;
    struct AgsDialogRunContext *dialog_ctx; /* for dialog_request's own script-side hook, if this native call chain ever re-enters it */
    struct CharacterInfo *chars;       /* the FULL character array (game_ctx->player is only the current player) */
    int numcharacters;
    int screen_w, screen_h;
    int game_paused;
    int cur_cursor;
};

/* Sets the context every ags_native_api_call() dispatch below reads.
 * Call once before running the interpreter (ags_cc_call_instance) --
 * a single file-scope pointer, not threaded through ccInstance/
 * ags_cc_run_code's own parameter lists, so M4's existing interpreter
 * signatures don't need to change. Pass NULL to clear it (every
 * dispatch then safely falls back to stub-logging). */
void ags_native_api_set_context(struct AgsNativeApiContext *ctx);

/* The actual CALLEXT dispatch point (ags/interp.c's own SCMD_CALLEXT
 * case calls this directly in place of its own former bare stub-and-
 * return-0). `args`/`numargs` are exactly ags_cc_run_code's own local
 * `callstack`/`callstacksize` at the CALLEXT instruction -- see this
 * header's own file-level comment for the calling convention. Returns
 * the value to load into the AX register (this build's own
 * "safe default" of 0 for anything not mapped, matching the
 * interpreter's prior behavior exactly for the fallback case -- see
 * ags_stub_hit's own dedup-by-callsite behavior, called internally
 * here with "<script CALLEXT>"/0 as the site, same as before). */
long ags_native_api_call(const char *name, const long *args, int numargs);

#endif /* AGS_NATIVE_API_H */
