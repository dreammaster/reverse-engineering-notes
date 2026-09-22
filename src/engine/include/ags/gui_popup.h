/* ags/gui_popup.h -- M11 ("the long tail" / GUI rendering, see
 * src/PLAN.md): check_controls's own POPUP_MOUSEY auto-show/hide
 * logic, found while investigating a visual artifact in M11's own
 * earlier GUI-rendering screenshot (a GUI sitting on top of the
 * status bar/verb icons that shouldn't have been visible at all).
 *
 * REAL EVIDENCE (matches.json's own check_controls/
 * remove_popup_interface entries, rob_blanc_1.asm read directly):
 * check_controls (Engine/AC.CPP:5640) does, once per frame, for each
 * GUI: "cmp [guis+ee*184h+40h],1; jz <continue>" -- only GUIs with
 * popup==POPUP_MOUSEY(1, Common/acroom.h:277) participate at all;
 * "cmp mouseY,[guis+ee*184h+44h]; jge <not-yet>" -- shows it
 * (guis[ee].on=1) only while mouseY<popupyp, an exact, literal
 * match. remove_popup_interface(int ifacenum) (AC.CPP:5339) is the
 * matching hide half: "if(ifacepopped!=ifacenum) return;
 * ifacepopped=-1; UnPauseGame(); guis[ifacenum].on=0; if(mousey<=
 * guis[ifacenum].popupyp) <reposition the mouse cursor>" -- called
 * twice from check_controls (both call sites not individually traced
 * this round).
 *
 * Checked against Rob Blanc 1's own real data (a throwaway
 * diagnostic, per CLAUDE.md's own check_options.c precedent): guis
 * [2]/[3]/[4] are ALL popup==POPUP_MOUSEY(1) -- only guis[1] (the
 * status line) is POPUP_NONE(0), i.e. genuinely always-visible.
 * gui[2]'s popupyp=12 (near the very top of the screen -- the
 * classic "move the mouse up to reveal the verb bar" pattern);
 * gui[3]/gui[4] both have popupyp=0, meaning mouseY<0 can never hold
 * -- these two are NEVER shown by this mechanism at all in normal
 * play (gui[4] is separately confirmed as the dialog-options GUI,
 * shown by do_conversation's own code instead -- a different
 * mechanism entirely, not popup-related; gui[3]'s own real trigger,
 * if any exists beyond this dead mouseY<0 condition, is unidentified).
 * This means the on-disk `on=1` these GUIs carry (read directly by
 * ags_load_guis, ags/gui_loader.h) is NOT their true default runtime
 * visibility -- it's this milestone's own earlier gap, not a real
 * finding about the game.
 *
 * SCOPE: this module reproduces the observable show/hide BEHAVIOR
 * (at most one POPUP_MOUSEY GUI visible at a time, whichever one's
 * mouseY<popupyp holds, checked in array order) via a simpler,
 * idempotent recompute-from-scratch call rather than porting check_
 * controls'/remove_popup_interface's own incremental single-`
 * ifacepopped`-global state machine statement-by-statement. NOT
 * reproduced: UnPauseGame()'s own game_paused decrement (no live
 * pause-nesting-counter is threaded through this call), and remove_
 * popup_interface's own mouse-cursor-repositioning side effect.
 * POPUP_NONE(0)/POPUP_SCRIPT(2) GUIs' own `.on` flag is left
 * completely untouched -- this module only concerns the ONE mechanism
 * it's named for.
 */
#ifndef AGS_GUI_POPUP_H
#define AGS_GUI_POPUP_H

#include "ags/gui_loader.h"

/* Tracks which GUI (if any) is currently shown via this mechanism --
 * this build's own confirmed single `ifacepopped` global (-1 = none). */
struct AgsGuiPopupState {
    int ifacepopped;
};

/* Recomputes every POPUP_MOUSEY(1) GUI's own `on` flag from scratch
 * against the current mouse Y position: exactly one (the first, in
 * array order, matching this project's own established "array order
 * IS z-order" finding) with mouseY<popupyp is turned on; every other
 * POPUP_MOUSEY GUI is turned off. Call once per frame, before
 * rendering, with the real current mouse Y. Idempotent -- safe to
 * call every frame regardless of what a GUI's own on-disk `on` value
 * was (correctly overrides ags_load_guis's own raw read for every
 * POPUP_MOUSEY GUI, which is not this GUI's true runtime state -- see
 * this header's own file-level comment). */
void ags_gui_update_popups(struct AgsGuiSet *set, struct AgsGuiPopupState *state, int mouse_y);

#endif /* AGS_GUI_POPUP_H */
