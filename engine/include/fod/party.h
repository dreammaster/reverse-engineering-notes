#pragma once
// Party inventory and item handling (KEH itemActionMenu, ingestItem, giveItemToMember, chooseAmmoToLoad, tryUnjamWeapon, ...).
#include "fod/vm.h"

namespace fod {

// Item side effect on a member (status bit or mutagen exposure) -- KEH applyItemEffect.
void applyItemEffect(World &w, Member &m, const ItemDef &item, bool apply);
// Chance roll that schedules a mutation onset (member +53h..56h) -- KEH exposeToMutagens.
void exposeToMutagens(World &w, Member &m, int percent);
// Moves item `itemSlot` from party member `from` to `to` -- KEH giveItemToMember. `report` prints the "too much to carry" message;
// with `dryRun` nothing is moved (used to validate a queued combat trade).
bool giveItemToMember(World &w, Host &h, int from, int to, int itemSlot, bool report, bool dryRun = false);
// Pick ammunition for the weapon in `itemSlot`: slot index, -1 cancelled, -2 none available -- KEH chooseAmmoToLoad.
int chooseAmmoToLoad(World &w, Host &h, int member, int itemSlot);
// Skill roll to clear a jam on the weapon in the member's cmdArg slot -- KEH tryUnjamWeapon.
void tryUnjamWeapon(World &w, Host &h, int member);
// "Who will ingest this?" -- KEH ingestItem.
void ingestItem(World &w, Host &h, int member, int ingestType, int itemSlot);
// The per-item menu of the character screen (and of the combat item command). Returns true when the caller should go on
// to pick a map target (field use) or, in combat, when a command was queued -- KEH itemActionMenu.
bool itemActionMenu(World &w, Host &h, int member, int itemSlot, bool inCombat);
// Doctor skill on another member (field or combat). Returns true if a treatment was attempted -- KEH doctorTreat.
bool doctorTreat(World &w, Host &h, int doctor, bool combat, bool report);

} // namespace fod
