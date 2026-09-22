/* ags/dialog_menu.h -- M11 ("the long tail" / options-menu UI, see
 * src/PLAN.md): do_conversation's own "show the enabled options, let
 * the player pick one" step.
 *
 * SCOPE DECISION (read before extending this file): do_conversation
 * itself (rob_blanc_1.asm, proc bounds 0x41D8C0-0x41EBE1, ~870 lines)
 * is, statement-by-statement, on the same scale as this project's own
 * biggest struct-cracking marathons (GameSetupStructBase, GameState).
 * Its real layout algorithm branches in two entirely different
 * directions depending on GameSetupStructBase.options[OPT_DIALOGIFACE]
 * (confirmed ==4 for Rob Blanc 1's own real data via a throwaway
 * diagnostic against ac2game.dta, i.e. THIS game uses the custom-GUI
 * branch, guis[4]): a custom-GUI-hosted list (this game's own real
 * path) or, for the OTHER branch this game never exercises, a
 * from-scratch draw_text_window()-based popup with its own two-pass
 * (measure, then draw) word-wrap layout via a private 12-parameter
 * helper (sub_41D7F7, called twice) that also draws a per-option
 * dialog_bullet SPRITE beside each line. Two facts make chasing full
 * pixel fidelity a poor use of further reversing effort right now:
 * (1) GameSetupStructBase.dialog_bullet==0 for this game's own real
 * data (also confirmed via the same diagnostic) -- there is no bullet
 * sprite to draw either way; (2) this engine has no .wfn/TrueType
 * font system ported yet (every milestone through M11 still draws
 * with Allegro's own built-in font), so a byte-exact line-height/
 * margin computation built on wgettextheight's own real font metrics
 * would not even look pixel-identical once drawn here regardless.
 * Given that, this module is a REAL, DATA-DRIVEN, interactive menu --
 * genuine option text from DialogTopic.optionnames[], genuine
 * optionflags[]&1 filtering, genuine mouse/keyboard input -- built on
 * a simpler single-pass layout instead of do_conversation's own
 * exact two-branch/two-pass pixel math. Matches this project's own
 * established convention for this kind of tradeoff (e.g. ags/
 * gui_render.h's GUILabel::Draw word-wrap: "same visible result,
 * different loop", or ags/interaction.h's DisplaySpeech-via-
 * DisplayMessage stand-in) rather than either faking it or spending
 * many more hours transcribing a function whose remaining fidelity
 * gap is now purely cosmetic for this specific game's own real data.
 */
#ifndef AGS_DIALOG_MENU_H
#define AGS_DIALOG_MENU_H

#include "ags/dialog.h"
#include "ags/gui.h"

/* Renders dtpp's own currently-enabled options (optionflags[i]&1) as
 * a real, clickable/key-navigable list, one line per option, using
 * the real option text (DialogTopic.optionnames[], untranslated --
 * see ags/dialog_run.h's own note on GetTranslation's identity
 * fallback with no .tra file loaded) with a small drawn bullet marker
 * standing in for the real (here, always-absent) dialog_bullet
 * sprite. Positioned inside `dialogface_gui`'s own real x/y/wid/hit
 * (its bgcol is filled first, matching this game's own real
 * OPT_DIALOGIFACE branch) if non-NULL, else a plain bordered box at
 * the bottom of a `screen_w`x`screen_h` screen (the DEFAULT-
 * textwindow branch's own stand-in, for a game that doesn't set
 * OPT_DIALOGIFACE -- Rob Blanc 1 itself always takes the first path).
 *
 * Real input handling: mouse motion highlights the hovered line,
 * left-click on an enabled line chooses it; Up/Down move the
 * keyboard-highlighted line among enabled options, Enter/Space
 * chooses it, Escape cancels. Blocks, polling every frame, until the
 * player chooses (returns that option's index into optionnames[]/
 * entrypoints[]/optionflags[], always one with optionflags[]&1 set),
 * cancels or ~30 seconds elapse with nothing chosen (returns -1 --
 * matching this project's own "time-bounded so a scripted/automated
 * caller can't hang forever" convention, e.g. ags_display_text_box). */
int ags_show_dialog_options(const struct DialogTopic *dtpp, const struct GUIMain *dialogface_gui,
                             int screen_w, int screen_h);

#endif /* AGS_DIALOG_MENU_H */
