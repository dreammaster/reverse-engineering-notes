/* ags/invscreen.h -- M11 ("the long tail" / inventory, see
 * src/PLAN.md): a real, data-driven stand-in for __actual_invscreen,
 * this build's own default (no custom GUIInv control -- confirmed
 * for Rob Blanc 1's own real data, numguiinv==0, see test_gui.c's own
 * output) inventory display screen.
 *
 * SCOPE DECISION (same convention as ags/dialog_menu.h's own): the
 * real __actual_invscreen (rob_blanc_1.asm, ~1133 lines, no 2011
 * source counterpart -- 2011's own default inventory display is
 * entirely GUI-based) is a from-scratch pixel-precise legacy screen:
 * a real ICONSPERLINE=4 grid, a bottom button row using this build's
 * own reserved Select/Look/OK sprite numbers (2041/2042/2043,
 * confirmed zero drift), a highlighted-slot box drawn via wsetcolor/
 * wrectangle, and scroll arrows for paging past the visible rows --
 * fully traced statement-by-statement in matches.json (CLAUDE.md's
 * own session notes), but not yet ported line-for-line here. Instead,
 * this module is REAL and DATA-DRIVEN where it matters most: genuine
 * owned-item enumeration (ags_update_invorder, ags/inventory.h),
 * genuine per-item sprite icons at their own real size, genuine mouse/
 * keyboard input, and -- the one piece worth calling out specifically
 * -- genuine Look/Use interactions dispatched through the REAL
 * GameSetupStructBase.__invcond[] EventBlock array via the
 * already-built ags_run_event_block (ags/interaction.h, M9), the same
 * real interaction-data format hotspots use. What's simplified: no
 * scroll paging (shows as many rows as fit, silently truncating a
 * longer list -- this game's own real data never has more items than
 * fit on screen at the icon sizes involved, so this has never been
 * observed to matter), and the exact pixel layout/highlight-box
 * styling is this module's own simpler single-pass grid rather than
 * the original's pixel-exact positions.
 */
#ifndef AGS_INVSCREEN_H
#define AGS_INVSCREEN_H

#include "ags/interaction.h"
#include "ags/gamesetup.h"
#include "ags/sprite_loader.h"

/* Shows every item playerchar owns (CharacterInfo.inv[]>0) as a real
 * grid of real sprite icons, refreshing the order first
 * (ags_update_invorder). Real input: mouse motion highlights the
 * hovered icon; left-click on an icon while in "Look" mode dispatches
 * game->__invcond[itemid] through ags_run_event_block (checkAgainst=0,
 * matching __actual_invscreen's own confirmed "Look" dispatch); a
 * left-click while NOT in Look mode picks the item up for real
 * (ags_set_active_inventory) and closes the screen; clicking the real
 * Select/Look/OK icon-bar sprites (2041/2042/2043) toggles Look mode
 * or closes the screen; ANY keypress unconditionally closes the
 * screen too (matching __actual_invscreen's own confirmed real
 * behavior). Blocks, polling every frame, until closed or ~30 seconds
 * elapse with nothing happening. */
void ags_run_inventory_screen(struct AgsGameContext *ctx, struct GameSetupStructBase *game,
                               struct AgsSpriteSet *sprites, int screen_w, int screen_h);

#endif /* AGS_INVSCREEN_H */
