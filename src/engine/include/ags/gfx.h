/* ags/gfx.h -- M6 ("See the room", see src/PLAN.md): the thinnest
 * possible wiring between a decoded RoomStruct (M5) and Allegro's own
 * real display pipeline (already linked since M0, unused until now).
 * No engine logic lives here yet -- just enough to prove the pipeline
 * works end to end: open a real graphics mode, apply the room's own
 * palette, blit its decompressed background onto the visible screen.
 * Later milestones (M7+) will grow this into real sprite/character
 * drawing; for now it's intentionally minimal.
 */
#ifndef AGS_GFX_H
#define AGS_GFX_H

#include "ags/room.h"

/* Wraps set_color_depth() + Allegro's own set_gfx_mode(
 * GFX_AUTODETECT_WINDOWED, width, height, 0, 0) -- a real, visible
 * window on the desktop (no scaling/letterboxing logic yet; that's
 * later milestone work once resolution-doubling matters). Must be
 * called after allegro_init(). Returns 0 on success, or Allegro's own
 * nonzero set_gfx_mode() failure code (see allegro_error for why). */
int ags_gfx_init_windowed(int width, int height, int color_depth);

/* Applies rst->pal (already in Allegro's own {r,g,b,filler}-per-entry
 * PALETTE layout -- RoomStruct.pal is byte-identical, see ags/room.h)
 * via set_palette(), then blits rst->ebscene[0] (the room's
 * decompressed background, M5) onto the visible `screen` bitmap at
 * (0,0). Must be called after ags_gfx_init_windowed(). */
void ags_gfx_show_background(const struct RoomStruct *rst);

#endif /* AGS_GFX_H */
