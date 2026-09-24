#ifndef YENDOR23_COMBAT_H
#define YENDOR23_COMBAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"
#include "monster.h"
#include "random.h"
#include "savegame.h"

/*
 * Turn-based combat: a genuinely separate subsystem from the
 * dungeon-exploration monster AI (monsterpool.h) -- surfaced while
 * resolving "do monsters ever reposition themselves" (they don't; see
 * dungeongrid.h/monsterpool.h), not otherwise touched by this project.
 * Combat works over its own small, fixed-size pool -- up to
 * CombatMonsterSlotCount live monster records (full copies, not
 * pointers into g_levelMonsters -- confirmed by DrawMonsterInfoPanels'
 * own read of these addresses as ordinary MonsterRecordSize records),
 * distinct from the 80-slot dungeon pool. Only the turn-order
 * construction (BuildCombatTurnOrder) is reimplemented so far --
 * actual turn advancement/attack resolution (ProcessCombatRound and
 * beyond) is a separate, much larger piece not started.
 */

enum {
    CombatMonsterSlotCount = 3,
    /*
     * BuildCombatTurnOrder (yendor2.asm:10952) zeroes a 14-entry buffer
     * but only ever populates/scans up to 4 party + 3 monster = 7
     * entries (confirmed by SelectActiveMonster's own cx=7 scan bound,
     * yendor2.asm:11346) -- the extra capacity is unused, not modeled.
     */
    CombatTurnOrderCapacity = SavePartyMemberSlots + CombatMonsterSlotCount
};

typedef struct {
    bool isMonster;
    /*
     * A party entry: 0-based index into SaveHeaderPartySlots (0-3).
     * A monster entry: 0-based index into the monsterSlots array passed
     * to combatBuildTurnOrder (0-2).
     */
    unsigned index;
    uint16_t speed; /* the sort key: PartyStatDexterity or MonsterFieldDexterity */
} CombatTurnOrderEntry;

/*
 * BuildCombatTurnOrder (yendor2.asm:10952, instruction-identical in
 * Chapter 3): builds a single turn order combining every occupied,
 * non-incapacitated (PartyStatusIncapacitated) party slot and every
 * occupied (MonsterFieldType != 0) monster slot, sorted by Dexterity
 * descending -- a stable insertion sort matching the original's own
 * swap-only-on-strictly-greater pass exactly (equal speeds keep their
 * original relative order).
 *
 * For every occupied monster slot, also assigns a fresh random living
 * party target: RandomInRange(3) retried until it lands on an
 * occupied, non-incapacitated party slot. (The original also
 * pre-tests each slot's own about-to-be-built turn-order flags before
 * doing this -- confirmed identical in both games -- but that test
 * reads never-yet-written, freshly-zeroed data within the same call,
 * so it can never actually skip the random assignment; not
 * reproduced, since reproducing a check that never fires would have
 * no observable effect.) Writes the chosen target as a 1-based
 * SaveHeaderPartySlots id into monsterTargets[i] for slot i (0 if no
 * living party member exists at all -- guarded here; the original
 * would loop forever, unreachable in practice since combat can't be
 * entered with a fully incapacitated party).
 *
 * Returns the number of entries written to out (<= CombatTurnOrderCapacity).
 */
unsigned combatBuildTurnOrder(SaveGame *save, const uint8_t *monsterSlots, RandomState *rng,
                               CombatTurnOrderEntry out[CombatTurnOrderCapacity],
                               uint16_t monsterTargets[CombatMonsterSlotCount]);

/*
 * SelectActiveMonster (yendor2.asm:11340, instruction-identical in
 * Chapter 3): the first turn-order entry that's a monster and not
 * flagged defeated. turnOrder/count is a combatBuildTurnOrder result;
 * defeated[i] should reflect monster slot i's own "already removed
 * this round" state (the original's turn-order entry flag 0x4000,
 * not otherwise modeled here since this project hasn't reimplemented
 * turn advancement yet). Returns true and sets *outMonsterSlot to the
 * monster slot index if found, else returns false.
 */
bool combatSelectActiveMonster(const CombatTurnOrderEntry *turnOrder, unsigned count,
                                const bool defeated[CombatMonsterSlotCount], unsigned *outMonsterSlot);

#endif
