/* ags/walk.h -- M8 ("The world starts moving", see src/PLAN.md): a
 * real, working port of the straight-line movement path through
 * walk_character/find_route/calculate_move_stage/do_movelist_move/
 * fix_player_sprite (Engine/acchars.cpp, Engine/routefnd.cpp,
 * Engine/AC.CPP -- all still present in this repo and read directly
 * for this milestone), narrowed to what this milestone's own test
 * needs: ONE character walking in a straight line toward a clicked
 * point.
 *
 * SCOPING DECISIONS (each a real, deliberate simplification, not a
 * faithfulness gap -- documented here since this milestone touches
 * more genuinely-simplified territory than any before it):
 *
 *   - find_route's full Dijkstra-based multi-waypoint obstacle-
 *     avoidance grid search (`__find_route`/`find_route_dijkstra`,
 *     Engine/routefnd.cpp) is NOT ported. Reading find_route's own
 *     real body (routefnd.cpp:766-899) shows the `ignore_walls=1`
 *     case this project's own MoveCharacterStraight already uses
 *     degenerates completely: `pathbackstage=0` skips the whole grid
 *     search, and the function falls straight through to building a
 *     plain TWO-waypoint MoveList (current position, target) via one
 *     calculate_move_stage call -- exactly what this module
 *     implements directly, with no approximation. Full obstacle-
 *     avoiding pathfinding (walking AROUND a wall rather than being
 *     stopped by it) is deferred; this module instead follows
 *     MoveCharacterStraight's own real behavior of stopping at the
 *     last visible point along the line (can_see_from/line_callback,
 *     ported below with zero simplification -- both are short,
 *     already-confirmed-verbatim matches).
 *   - fix_player_sprite's own gradual multi-frame turning animation
 *     (start_character_turning's TURNING_AROUND/TURNING_BACKWARDS
 *     state machine, woven into update_stuff's per-character loop,
 *     Engine/AC.CPP:6526-6559) is NOT ported, even though this game's
 *     own real data has OPT_ROTATECHARS enabled (confirmed via a
 *     direct read of game.options[18]==1) so the real engine WOULD
 *     exercise it. This module always takes the "OPT_ROTATECHARS
 *     disabled"/CHF_NOTURNING code path instead (`chin->loop=
 *     useloop;` immediately) -- a real, already-existing branch in
 *     the original, just taken unconditionally here. Purely cosmetic:
 *     the character still faces the correct new direction, just
 *     snaps to it instantly across a frame boundary rather than
 *     visibly rotating through intermediate loops first.
 *   - update_stuff's own per-character walking/animation section
 *     (AC.CPP:6520-6664) is written against 2011's much more elaborate
 *     CharacterExtras-based architecture (separate xwas/ywas/animwait
 *     fields, wantMoveNow/doNextCharMoveStep zoom-based sub-stepping,
 *     CHF_ANTIGLIDE/CHF_MOVENOTWALK flags) -- all already independently
 *     confirmed ABSENT from this build in earlier reversing sessions
 *     (struct-layout-drift.md: "CharacterExtras.xwas/.ywas/...
 *     confirmed absent"; "'animwait'/'walkwait' fold into
 *     CharacterInfo.wait@+0x1C"). This build's own real update loop is
 *     genuinely simpler than 2011's, not just renamed -- this module's
 *     ags_advance_walk_animation() implements that simpler shape
 *     directly (one shared `wait` counter, one do_movelist_move call
 *     per frame, no zoom sub-stepping) rather than porting 2011's own
 *     elaborate version and hoping the drift washes out.
 *   - No shared mls[]/CHMLSOFFS-indexed MoveList array -- this module
 *     gives each walking character its own single, directly-owned
 *     MoveList (struct AgsWalkState) rather than indexing into a
 *     global array, since M8 only ever needs one character walking
 *     at a time. calculate_move_stage's own move_speed_x/_y globals
 *     are similarly passed as plain parameters instead.
 */
#ifndef AGS_WALK_H
#define AGS_WALK_H

#include "ags/character.h"
#include "ags/view.h"
#include "ags/room.h"

struct AgsWalkState {
    struct MoveList mls;
    int active; /* 1 while a move is in progress */
};

/* MoveCharacterStraight + walk_character(ignwal=1) + find_route
 * (ignore_walls=1) + calculate_move_stage, fused into one call (see
 * the file-level comment above for the exact real behavior this
 * matches). Resolves line-of-sight via can_see_from against
 * `walkable_mask` (this build's own `wallscreen`, i.e. RoomStruct.
 * walls) -- if the target isn't visible, walks to the last visible
 * point along the line instead, exactly matching
 * MoveCharacterStraight's own real fallback. Sets chin->walking=1 and
 * calls ags_fix_player_sprite() once (matching the real engine's own
 * "only fix the sprite when a NEW stage begins" convention, and this
 * module's own single-stage-only model) if a move was started.
 * Returns 1 if a move was started, 0 if the character was already at
 * (or immediately blocked at) the target. */
int ags_walk_character_straight(struct CharacterInfo *chin, struct AgsWalkState *ws,
                                 block walkable_mask, const struct ViewStruct272 *views,
                                 int tox, int toy);

/* do_movelist_move (Engine/AC.CPP:17327), adapted to a single
 * directly-owned MoveList (see the file-level comment above) rather
 * than a shared indexed array -- otherwise a direct, unmodified port
 * of the real fixed-point stepping/target-snapping algorithm.
 * Updates *inout_x/*inout_y in place. Returns 0 if the move is still
 * in progress, or nonzero (matching source's own "need_to_fix_sprite"
 * return value) once this module's own single stage finishes. */
int ags_do_movelist_move(struct MoveList *cmls, int *inout_x, int *inout_y);

/* fix_player_sprite's own core direction-selection logic
 * (useDiagonal/hasUpDownLoops/the CHECK_DIAGONAL macro, Engine/
 * acchars.cpp:134-267) -- sets chin->loop directly, always taking the
 * no-gradual-turning path (see the file-level comment above). */
void ags_fix_player_sprite(const struct MoveList *cmls, struct CharacterInfo *chin,
                            const struct ViewStruct272 *views);

/* This module's own simplified update_stuff walking/animation tick
 * (see the file-level comment above) -- call once per frame for a
 * character with an active AgsWalkState. While chin->wait>0, just
 * decrements it. At 0: advances the move by one do_movelist_move
 * step; if that finishes the walk, stops and resets to the standing
 * frame; otherwise advances the animation frame within the current
 * loop (wrapping to frame 1, or 0 if the loop has under 2 frames,
 * matching source's own wraparound convention) and reloads chin->wait
 * from that frame's own speed + chin->animspeed. */
void ags_advance_walk_animation(struct CharacterInfo *chin, struct AgsWalkState *ws,
                                 const struct ViewStruct272 *views);

#endif /* AGS_WALK_H */
