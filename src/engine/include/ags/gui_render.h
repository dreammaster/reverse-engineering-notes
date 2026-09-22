/* ags/gui_render.h -- M11 ("the long tail" / GUI rendering, see
 * src/PLAN.md): GUIMain::draw_at + a scoped subset of the six
 * GUIObject-derived Draw() methods.
 *
 * REAL, cited implementation: GUIMain::draw_at itself (matches.json's
 * own entry, a complete decisive match -- wtexttransparent(0) header,
 * the wid<1||hit<1 early-out, a sub-bitmap sized to wid/hit, the
 * fgcol==0&&bgcol!=0 => fgcol=16 default, bgcol-based clear_to_color,
 * a two-pass wrectangle outline when fgcol!=bgcol, and a bgpic-based
 * draw_sprite_compensate call) plus GUIButton::Draw's own pic-based
 * branch (usepic, matching source's isover/ispushed-driven picture
 * selection exactly) and GUILabel::Draw's text branch (wtextcolor +
 * text -- a SIMPLIFIED word-wrap using this project's own established
 * wrap_text() convention from ags/interaction.h's DisplayMessage,
 * rather than porting GUILabel::Draw's own fused byte-by-byte
 * line-break scan statement-for-statement; both produce the same
 * visible result, real word-wrapped text, just via a different loop).
 *
 * GUIInv::Draw (M11's own inventory slice) is now REAL too -- see
 * gui_render.c's own file-level comment on draw_inventory for the
 * complete citation (the grid-layout algorithm routes entirely
 * through GameState's own inv_numinline/inv_numdisp/inv_top/
 * inv_numorder/play_invorder[]/inv_item_wid/inv_item_hit fields, not
 * any per-control field). Use ags_gui_draw_all_with_inventory (below)
 * to get it -- plain ags_gui_draw_all still stubs GOBJ_INVENTORY
 * (passes no GameState*), for callers with no inventory state to
 * thread through.
 *
 * STUBBED (AGS_STUB_VOID, see ags/stub.h): GUITextBox/GUIListBox/
 * GUISlider's own Draw() methods -- none of matches.json's own
 * entries for those three trace their draw algorithm statement-by-
 * statement yet, and this milestone's own test game data doesn't
 * exercise them, so a real port would be guessing rather than citing
 * evidence. Their CONTROLS still get skipped cleanly (no crash, no
 * visible garbage) -- just not drawn. "Draw" isn't a native script
 * call, but stubbing it through the same logged convention keeps it
 * visible in stub_hits.log's own backlog rather than a silent gap,
 * consistent with src/PLAN.md's own stub convention.
 */
#ifndef AGS_GUI_RENDER_H
#define AGS_GUI_RENDER_H

#include "ags/gui_loader.h"
#include "ags/sprite_loader.h"
#include "ags/gamestate.h"
#include "ags/character.h"
#include "ags/gamesetup.h"

/* Draws every guis[] entry with .on!=0, in array order (this build's
 * own confirmed-absent z-order sorting -- see ags/gui.h's own
 * GUIMain.zorder comment -- means plain array order IS draw order),
 * onto `target` (typically Allegro's own `screen`). `sprites` supplies
 * button/bgpic sprites via ags_spriteset_load(); a sprite slot that
 * fails to load is silently skipped (matching this build's own
 * "spriteset[x]!=NULL" gate before drawing). Colors are used as raw
 * 8-bit palette indices directly (get_col8_lookup is the identity
 * function at color_depth==1 -- see ags/sprite_loader.h's own note
 * that this game is confirmed 8-bit throughout). Any GOBJ_INVENTORY
 * control is stubbed (see ags_gui_draw_all_with_inventory instead). */
void ags_gui_draw_all(const struct AgsGuiSet *set, struct AgsSpriteSet *sprites, BITMAP *target);

/* Same as ags_gui_draw_all, but also renders any GOBJ_INVENTORY
 * control for real (draw_inventory, see this header's own file-level
 * comment) using `play`'s own real inventory-layout/ordering state,
 * `playerchar`'s own owned-item counts (CharacterInfo.inv[]), and
 * `invinfo`'s own per-item display picture
 * (GameSetupStructBase.invinfo[].pic, `numinvitems` entries). */
void ags_gui_draw_all_with_inventory(const struct AgsGuiSet *set, struct AgsSpriteSet *sprites,
                                      BITMAP *target, struct GameState *play,
                                      struct CharacterInfo *playerchar, int numinvitems,
                                      const struct InventoryItemInfo *invinfo);

#endif /* AGS_GUI_RENDER_H */
