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
 * STUBBED (AGS_STUB_VOID, see ags/stub.h): GUITextBox/GUIListBox/
 * GUISlider/GUIInv's own Draw() methods -- none of matches.json's own
 * entries for those four trace their draw algorithm statement-by-
 * statement yet (unlike Button/Label above), and this milestone's own
 * test game data doesn't exercise them, so a real port would be
 * guessing rather than citing evidence. Their CONTROLS still get
 * skipped cleanly (no crash, no visible garbage) -- just not drawn.
 * "Draw" isn't a native script call, but stubbing it through the same
 * logged convention keeps it visible in stub_hits.log's own backlog
 * rather than a silent gap, consistent with src/PLAN.md's own stub
 * convention.
 */
#ifndef AGS_GUI_RENDER_H
#define AGS_GUI_RENDER_H

#include "ags/gui_loader.h"
#include "ags/sprite_loader.h"

/* Draws every guis[] entry with .on!=0, in array order (this build's
 * own confirmed-absent z-order sorting -- see ags/gui.h's own
 * GUIMain.zorder comment -- means plain array order IS draw order),
 * onto `target` (typically Allegro's own `screen`). `sprites` supplies
 * button/bgpic sprites via ags_spriteset_load(); a sprite slot that
 * fails to load is silently skipped (matching this build's own
 * "spriteset[x]!=NULL" gate before drawing). Colors are used as raw
 * 8-bit palette indices directly (get_col8_lookup is the identity
 * function at color_depth==1 -- see ags/sprite_loader.h's own note
 * that this game is confirmed 8-bit throughout). */
void ags_gui_draw_all(const struct AgsGuiSet *set, struct AgsSpriteSet *sprites, BITMAP *target);

#endif /* AGS_GUI_RENDER_H */
