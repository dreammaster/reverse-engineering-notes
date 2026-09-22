/* ags/gui_loader.h -- M11 ("the long tail" / GUI rendering, see
 * src/PLAN.md): read_gui/GUIMain::rebuild_array, ported directly from
 * rob_blanc_1.asm (read_gui: proc near at 0x407CB4-ish, GUIMain::
 * rebuild_array at 0x4073FE-ish -- both already "confirmed match" in
 * matches.json, but this header/its .c file are the first place their
 * EXACT byte format gets pushed into real, runnable C rather than just
 * prose evidence).
 *
 * Prerequisite call order, on the same FILE* used by ags/loader.h's
 * own M2/M3/M11 steps, immediately after ags_load_dlgmessages
 * succeeds: game->numgui is still whatever garbage the on-disk
 * GameSetupStructBase blob happened to carry at that field until
 * ags_load_guis() below overwrites it for real (read_gui's own first
 * job, matching source's "arg_8->numgui=..." write).
 *
 * FIXED POOLS, not 2011's DynamicArray<T>: this build's own
 * GUIMain::rebuild_array (read directly from the disassembly) resolves
 * each GUIMain.objrefptr[] entry -- packed as (type<<16)|index, the
 * same convention EventBlockCmd/AnimationStruct already established
 * elsewhere in this project -- into a pointer INTO one of six shared,
 * fixed-size global pools (guibuts[81]/guilabels[81]/guiinv[81]/
 * guislider[81]/guitext[81]/guilist[81], all six sizes already
 * confirmed in ags/gui.h's own struct headers), never a per-GUI
 * allocation. struct AgsGuiSet below reproduces that same shape as one
 * self-contained value instead of seven separate process-wide globals,
 * since this project's own test-harness convention (AgsSpriteSet,
 * AgsWalkState, AgsGameContext) already prefers instance state over
 * bare globals wherever the original used them only for lack of a
 * "session" concept.
 */
#ifndef AGS_GUI_LOADER_H
#define AGS_GUI_LOADER_H

#include <stdio.h>

#include "ags/gamesetup.h"
#include "ags/gui.h"

/* read_gui's own confirmed capacity checks (both real quit()-worthy
 * error conditions in the original, mapped to error codes here
 * instead of aborting the whole process -- same convention as
 * ags/room_loader.h's AgsRoomLoadError). */
#define AGS_GUI_MAX_GUIS 20      /* "read_gui: too many GUIs" -- numgui<=0x14 */
#define AGS_GUI_MAX_CONTROLS 81  /* "...too many controls..." -- each count<0x51 */
#define AGS_GUI_MAX_LISTBOX_ITEMS 100 /* GUIListBox.items[100]'s own confirmed capacity, ags/gui.h */

struct AgsGuiSet {
    struct GUIMain guis[AGS_GUI_MAX_GUIS];
    int numgui;
    struct GUIButton guibuts[AGS_GUI_MAX_CONTROLS];
    int numguibuts;
    struct GUILabel guilabels[AGS_GUI_MAX_CONTROLS];
    int numguilabels;
    struct GUIInv guiinv[AGS_GUI_MAX_CONTROLS];
    int numguiinv;
    struct GUISlider guislider[AGS_GUI_MAX_CONTROLS];
    int numguislider;
    struct GUITextBox guitext[AGS_GUI_MAX_CONTROLS];
    int numguitext;
    struct GUIListBox guilist[AGS_GUI_MAX_CONTROLS];
    int numguilist;
};

enum AgsGuiLoadError {
    AGS_GUI_LOAD_OK = 0,
    AGS_GUI_LOAD_READ_ERROR,
    AGS_GUI_LOAD_BAD_VERSION,     /* "read_gui: unknwon version" -- gver in (0x66,anything unhandled] */
    AGS_GUI_LOAD_TOO_MANY_GUIS,   /* numgui > AGS_GUI_MAX_GUIS */
    AGS_GUI_LOAD_TOO_MANY_CONTROLS /* any one control-type count >= AGS_GUI_MAX_CONTROLS */
};

/* Resolves every guis[ff].objrefptr[] entry into a real objs[]
 * pointer into one of AgsGuiSet's six fixed pools, matching GUIMain::
 * rebuild_array's own 6-way (type<<16)|index dispatch exactly
 * (type 1=button/2=label/3=inv/4=slider/5=textbox/6=listbox -- an
 * "unknown control type" is the original's own quit(), which this
 * port turns into a silently-skipped slot instead of aborting).
 * Called once per GUI by ags_load_guis(); exposed separately since a
 * caller reconstructing/mutating objrefptr[] by hand (not done by
 * anything in this milestone) would need to call it again. */
void ags_gui_rebuild_array(struct AgsGuiSet *set, struct GUIMain *gm);

/* Ports read_gui(FILE*,GUIMain*,GameSetupStructBase*,GUIMain**) --
 * except the destination is `set` (see this header's own file-level
 * comment) rather than raw global pool addresses. `f` must be
 * positioned exactly where ags/loader.h's own M11 steps
 * (ags_load_characters -> ags_load_messages -> ags_load_dialog_topics
 * -> ags_load_dlgmessages) leave it. On success, `game->numgui` is
 * overwritten with the real on-disk value (matching source's own
 * "arg_8->numgui=..." write) and every populated GUIMain's objs[]
 * pointers are already resolved (rebuild_array called for each, same
 * as the original's own per-GUI loop). */
enum AgsGuiLoadError ags_load_guis(FILE *f, struct GameSetupStructBase *game, struct AgsGuiSet *set);

#endif /* AGS_GUI_LOADER_H */
