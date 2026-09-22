/* ags/gfx.h -- M6 ("See the room", see src/PLAN.md): the thinnest
 * possible wiring between a decoded RoomStruct (M5) and Allegro's own
 * real display pipeline (already linked since M0, unused until now).
 * No engine logic lives here yet -- just enough to prove the pipeline
 * works end to end: open a real graphics mode, apply the room's own
 * palette, blit its decompressed background onto the visible screen.
 * Later milestones (M7+) will grow this into real sprite/character
 * drawing; for now it's intentionally minimal.
 *
 * CORRECTION (found after M9, while investigating why every sprite in
 * every screenshot through M6-M9 rendered as a solid black
 * silhouette): this build is NOT free to just apply RoomStruct.pal
 * wholesale. AGS's own real palette model (Engine/AC.CPP:4114-4124,
 * still present in this repo) splits the 256 palette slots by
 * GameSetupStructBase.paluses[i]: PAL_BACKGROUND(2) slots really do
 * come from the CURRENT room's own pal[i], but PAL_GAMEWIDE(0)/
 * PAL_LOCKED(1) slots stay pinned to the game's own persistent
 * defpal[i] regardless of which room is showing -- specifically so
 * that character/sprite colors (which are drawn using palette indices
 * in the locked range) look the same in every room. A real check of
 * Rob Blanc 1's own data confirms this isn't academic: sprite #2000
 * (ROB's own view/loop/frame pose, M7) uses palette indices 0,1,3,6,
 * 11,15,20,25,29,32,36,40 -- EVERY one of them GAMEWIDE or LOCKED in
 * game.paluses[], with real, varied defpal[] colors (skin tones,
 * clothing, highlights) -- but room6.crm's own pal[] at those exact
 * same indices is uniformly (0,0,0), since a room file was never
 * meant to supply real values there at all. Blindly applying rst->pal
 * (this header's own original design) renders every sprite pitch
 * black. ags_gfx_show_background() now takes the game's own paluses[]
 * /defpal[] and does the real per-slot merge before calling
 * set_palette(). */
#ifndef AGS_GFX_H
#define AGS_GFX_H

#include "ags/room.h"
#include "ags/gamesetup.h"

/* Wraps set_color_depth() + Allegro's own set_gfx_mode(
 * GFX_AUTODETECT_WINDOWED, width, height, 0, 0) -- a real, visible
 * window on the desktop (no scaling/letterboxing logic yet; that's
 * later milestone work once resolution-doubling matters). Must be
 * called after allegro_init(). Returns 0 on success, or Allegro's own
 * nonzero set_gfx_mode() failure code (see allegro_error for why). */
int ags_gfx_init_windowed(int width, int height, int color_depth);

/* Builds the real merged 256-entry palette described in the file-
 * level comment above: PAL_BACKGROUND slots from rst->pal, everything
 * else (PAL_GAMEWIDE/PAL_LOCKED) from game->defpal. Exposed on its
 * own (not just folded into ags_gfx_show_background) so a caller that
 * needs to pass a palette to something else -- e.g. save_bitmap(),
 * for a screenshot that shows real sprite colors instead of the raw
 * room-only palette -- can get the SAME merged palette without
 * re-deriving it. */
void ags_gfx_build_merged_palette(const struct RoomStruct *rst, const struct GameSetupStructBase *game,
                                   RGB out[256]);

/* Applies ags_gfx_build_merged_palette()'s own result via
 * set_palette(), then blits rst->ebscene[0] (the room's decompressed
 * background, M5) onto the visible `screen` bitmap at (0,0). Must be
 * called after ags_gfx_init_windowed(). */
void ags_gfx_show_background(const struct RoomStruct *rst, const struct GameSetupStructBase *game);

#endif /* AGS_GFX_H */
