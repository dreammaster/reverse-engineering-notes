/* ags/inventory.h -- M11 ("the long tail" / inventory, see
 * src/PLAN.md): the real, shared inventory-bookkeeping core
 * (add_inventory/LoseInventory/update_invorder/SetActiveInventory),
 * all read directly from their own matches.json entries (Engine/
 * AC.CPP, this build's own genuine single-character predecessors of
 * 2011's later per-character-generalized versions -- see each
 * function's own comment below for the exact citation).
 *
 * `play_invorder[]`/`inv_numorder` are treated as ONE synchronized
 * pair throughout (matches.json's own repeated finding, across both
 * add_inventory and LoseInventory's independent entries): every
 * function here that changes ownership keeps that pair consistent,
 * the same real invariant the original engine maintains.
 */
#ifndef AGS_INVENTORY_H
#define AGS_INVENTORY_H

#include "ags/character.h"
#include "ags/gamestate.h"

/* add_inventory(int inum) (Engine/AC.CPP:16029-16036) -- validates
 * inum in [0,100) (source's own "!AddInventory: invalid invnetory
 * number" quit(), a genuine period-typo preserved verbatim in the
 * real error string -- mapped to a silent no-op here, matching this
 * project's own "map quit() to a safe no-op" convention), increments
 * playerchar->inv[inum], and appends inum to play_invorder[]/bumps
 * inv_numorder if not already present. Source's own trailing
 * guis_need_update=1 + run_on_event(GE_ADD_INV=7,inum) hook call is
 * NOT ported -- no GUI-invalidation-flag or event-hook subsystem is
 * threaded through this call. */
void ags_add_inventory(struct CharacterInfo *playerchar, struct GameState *play, int inum);

/* LoseInventory(int inum) -- validates inum in [0,100), decrements
 * playerchar->inv[inum], clears playerchar->activeinv (and would
 * reset the mouse cursor mode if it was currently MODE_USE -- NOT
 * ported, no live cursor-mode state is threaded through this call)
 * if the now-empty item was the active one, and -- if the count
 * reached zero -- removes inum from play_invorder[]/decrements
 * inv_numorder, shifting every later entry down by one. Source's own
 * trailing guis_need_update=1 + run_on_event(GE_LOSE_INV=8,inum) hook
 * call is NOT ported, same reason as ags_add_inventory. */
void ags_lose_inventory(struct CharacterInfo *playerchar, struct GameState *play, int inum);

/* update_invorder() (Engine/AC.CPP, this build's own single-character
 * predecessor of 2011's per-character version -- matches.json's own
 * entry: "play_inv_numorder=0; for(ff=0;ff<game_numinvitems;ff++) {
 * if(playerchar->inv[ff]>0) { play_invorder[play_inv_numorder]=ff;
 * play_inv_numorder++; } }"). Rebuilds play_invorder[]/inv_numorder
 * from scratch by scanning playerchar->inv[0..numinvitems) for owned
 * items, in item-number order (NOT a MRU/recency order -- this
 * build's own real behavior, matching source exactly). Used to
 * refresh the ordering after any change that doesn't go through
 * ags_add_inventory/ags_lose_inventory's own incremental maintenance
 * (e.g. loading a save, or GUIInv::Draw's own lazy refresh when
 * inv_numorder<0 -- see ags/gui_render.h's own real port of that). */
void ags_update_invorder(struct CharacterInfo *playerchar, struct GameState *play, int numinvitems);

/* SetActiveInventory(int iit) (Engine/AC.CPP) -- iit==-1 deselects
 * (playerchar->activeinv=-1); otherwise validates iit in [1,100) AND
 * that playerchar->inv[iit]>=1 (the player must actually own at least
 * one -- source's own "!SetActiveInventory: player doesn't have this
 * item" quit(), mapped to a no-op returning AGS_MODE_WALK here rather
 * than aborting) before setting playerchar->activeinv=iit. Source's
 * own item-cursor-graphic application (sub_40CF16, "update_inv_
 * cursor") and its SetCursorMode(MODE_USE)/reset-to-MODE_WALK calls
 * are NOT performed here -- no live cursor-mode/cursor-graphic state
 * is threaded through this call. Returns the cursor mode the REAL
 * function would have switched to, as one of ags/interaction.h's own
 * AgsCursorMode values (returned as a plain int here so this header
 * doesn't need to depend on ags/interaction.h just for the enum type)
 * -- AGS_MODE_USE(4) on a successful select, AGS_MODE_WALK(0) on a
 * deselect or a failed/invalid selection -- for a caller that DOES
 * track cursor mode to apply for real. */
int ags_set_active_inventory(struct CharacterInfo *playerchar, int iit);

#endif /* AGS_INVENTORY_H */
