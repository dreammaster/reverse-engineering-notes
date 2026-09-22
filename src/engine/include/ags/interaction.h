/* ags/interaction.h -- M9 ("Say something", see src/PLAN.md): a real
 * port of get_hotspot_at/RunHotspotInteraction/run_event_block
 * (Engine/AC.CPP -- get_hotspot_at/RunHotspotInteraction still
 * present in this repo; run_event_block does NOT exist there at all,
 * see below), plus a minimal real DisplayMessage/Display text-box
 * renderer.
 *
 * run_event_block's own full 15-value respond[] dispatch table has
 * NO 2011 source counterpart to read at all -- the whole EventBlock/
 * "interaction response type" subsystem it belongs to was replaced
 * by the NewInteraction system before the 2011 reference build, and
 * 2011 keeps only a dead, commented-out 4-argument prototype
 * (Engine/AC.CPP:756) as a trace of it ever existing. Every case
 * below is reconstructed PURELY from this project's own exhaustive
 * disassembly reads (reversing/analysis/matches.json's own
 * `run_event_block` entry, and the earlier CLAUDE.md session notes it
 * summarizes) -- there is no source file to cite a line number in.
 * The complete, disassembly-confirmed table (all 15 values,
 * confirmed exhaustive via the dispatch's own `default: quit(...)`
 * catch-all):
 *   0  = NewRoom(respondval)            [+ edge-position encoding
 *                                          when fired from misccond
 *                                          for a room-edge condition
 *                                          -- not reproduced here,
 *                                          M9 only exercises this
 *                                          from a per-hotspot click]
 *   1  = no-op ("None")
 *   2  = StopMoving(player)
 *   3  = run_on_event(GE_MAN_DIES, data)
 *   4  = Run Animation (the GameAnimation/AnimationStruct resource
 *        table, respond==4's own separate "Animations" subsystem)
 *   5  = DisplayMessage(respondval) [+ optional character-attributed
 *        speech via the `extra`/`xx` mechanism when called with a
 *        non-negative `extra` argument -- not reproduced here, no
 *        speech/portrait rendering exists yet]
 *   6  = ObjectOff(respondval)
 *   7  = ObjectOff(respondval) THEN add_inventory(data)  (compound)
 *   8  = add_inventory(data)
 *   9  = Run Script (make_ts_func_name + run_another/
 *        run_text_script_2iparam-equivalent dispatch)
 *   10 = run_graph_script(respondval)   (the separate GRAPHSCRIPT
 *                                          subsystem, M5 deliberately
 *                                          doesn't extract its data)
 *   11 = PlaySound(respondval)
 *   12 = PlayFlic(data, respondval)
 *   13 = ObjectOn(respondval)
 *   14 = RunDialog(respondval)
 *
 * SCOPE, this milestone: only 0 (NewRoom), 1 (no-op), 2 (StopMoving),
 * and 5 (DisplayMessage, MINUS the character-attribution extension)
 * get REAL implementations -- exactly the ones both reachable with
 * this engine's own current infrastructure (no RoomObject/inventory/
 * dialog/sound/named-script-function-call machinery exists yet, and
 * M5 deliberately never extracts the GRAPHSCRIPT payload) and
 * actually exercised by real room data this milestone's own test
 * uses (room6.crm's own real hotspot interactions are respond==5 and
 * respond==10). Every other case calls AGS_STUB_VOID() instead of
 * silently doing nothing -- per PLAN.md's own stub convention, a
 * real, logged "implement this next" marker, not a fake no-op.
 *
 * checkAgainst matching: this module treats each EventBlock command's
 * `list[i]` as a plain equality test against `checkAgainst` --
 * matches.json's own note that list[i] is "compared against the
 * checkAgainst/matchType parameters" together doesn't fully spell out
 * matchType's own separate contribution (plausibly a MODE_USE-
 * specific inventory-item-id refinement, per 2011's own dead
 * `run_interaction_event(...,int extra,bool checkAll)` signature this
 * function's prototype survives alongside), and no real room data
 * this project has decoded so far exercises MODE_USE-style hotspot
 * interactions to observe it directly -- left as a documented
 * simplification rather than guessed at.
 */
#ifndef AGS_INTERACTION_H
#define AGS_INTERACTION_H

#include "ags/room.h"
#include "ags/character.h"
#include "ags/walk.h"

/* This build's own confirmed mood->passon values (Common/acruntim.h,
 * matching RunHotspotInteraction's own dispatch table exactly). */
enum AgsCursorMode {
    AGS_MODE_WALK = 0,
    AGS_MODE_LOOK = 1,
    AGS_MODE_HAND = 2,
    AGS_MODE_TALK = 3,
    AGS_MODE_USE = 4,
    AGS_MODE_PICKUP = 5,
    AGS_MODE_CUSTOM1 = 8,
    AGS_MODE_CUSTOM2 = 9
};

/* Bundles the pieces of engine state run_event_block's own real
 * cases need to touch -- deliberately small, grown only as far as
 * this milestone's own real cases require. */
struct AgsGameContext {
    struct RoomStruct *rst;
    struct CharacterInfo *player;
    struct AgsWalkState *player_walk;
    int quit_requested; /* set by a NewRoom targeting a room this engine can't load, so the caller can stop cleanly instead of pressing on with a stale room */
};

/* get_hotspot_at (Engine/AC.CPP:8939) -- a real port, minus 2011's
 * own RoomStatus.hotspot_enabled[] gate: this engine has no per-
 * save-slot RoomStatus yet (that's later milestone work), so every
 * hotspot the mask names is treated as enabled. Returns the hotspot
 * index at (x,y) (0 = "no hotspot"/background). */
int ags_get_hotspot_at(const struct RoomStruct *rst, int x, int y);

/* run_event_block, see the file-level comment above for the complete
 * 15-value table and this milestone's own real-vs-stubbed split. */
void ags_run_event_block(struct AgsGameContext *ctx, const struct EventBlock *block, int checkAgainst);

/* RunHotspotInteraction (Engine/AC.CPP:16606) -- this build's own
 * real mood->passon mapping (see enum AgsCursorMode above), then two
 * ags_run_event_block passes into rst->hscond[hotspot]: the specific
 * mood, then checkAgainst=5 ("any click on this hotspot"), matching
 * both of RunHotspotInteraction's own real call sites exactly. */
void ags_run_hotspot_interaction(struct AgsGameContext *ctx, int hotspot, int mood);

/* DisplayMessage's own minimal real rendering: draws room->message[
 * msgnum] in a simple bordered text box over whatever is already on
 * `screen` (caller draws the background/characters first), using
 * Allegro's built-in font (this engine hasn't ported AGS's own .wfn
 * font loading yet -- later milestone work) with basic word-wrap.
 * Blocks, polling keyboard/mouse each iteration and redrawing
 * nothing else, until a key/click arrives or ~2 seconds elapse --
 * real modal-dialog behavior, time-bounded so a scripted/automated
 * caller can't hang forever waiting for input nobody will provide. */
void ags_display_message(const struct RoomStruct *rst, int msgnum);

#endif /* AGS_INTERACTION_H */
