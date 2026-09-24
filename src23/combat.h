#ifndef YENDOR23_COMBAT_H
#define YENDOR23_COMBAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"
#include "monster.h"
#include "monsterpool.h"
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
 * distinct from the 80-slot dungeon pool. Turn-order construction
 * (BuildCombatTurnOrder) and per-round death/advancement handling
 * (ProcessCombatRound) are both reimplemented; attack resolution
 * itself (ResolveAttack/ResolveAttackerActionOutcome/
 * ProcessMonsterAttackTurn and the player-input attack path in
 * HandleDungeonInput) is a separate, much larger piece not started.
 *
 * **Design note on the original's "active combat monster" global**
 * (`g_activeCombatMonster`/`word_32A1E`): the original caches this as
 * mutable global state and re-establishes it (via SelectActiveMonster)
 * from two different places whenever it's unset -- once at the end of
 * BuildCombatTurnOrder, once at the top of ProcessCombatRound's
 * turn-advance branch. This reimplementation treats it as a pure,
 * on-demand query instead (combatSelectActiveMonster, taking the
 * current turnOrder/defeated state and returning the answer fresh):
 * recomputing it is cheap and always gives the same result the
 * original's caching was trying to preserve, so there's no cached
 * state to thread through combatBuildTurnOrder/combatProcessRound's
 * own signatures. Call combatSelectActiveMonster whenever the current
 * active monster is actually needed (e.g. for UI highlighting or
 * attack targeting).
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

/*
 * ProcessCombatRound (yendor2.asm:11094, instruction-identical in
 * Chapter 3): called once per game-loop tick, right after whichever
 * combatant's turn was current (turnOrder[*turnCursor]) has acted.
 * First, scans every occupied monster slot: any with MonsterFieldHealth
 * <= 0 is flagged defeated[slot] = true, has its rewards granted into
 * staging (see monsterpool.h's monsterGrantRewards) and its record
 * zeroed. If that leaves no monster slot both occupied and alive,
 * returns CombatRoundNoMonstersLeft (errorCode 0 in the original) --
 * the caller should end combat (show loot/XP, matching
 * ShowLootAndAwardExperience). Otherwise advances *turnCursor to the
 * next turnOrder entry that isn't a defeated monster, matching the
 * original's forward-only scan from word_32BF4 (no wraparound --
 * reaching the end of turnOrder without finding one returns
 * CombatRoundNewRound, errorCode 1, meaning the caller should rebuild
 * the turn order via combatBuildTurnOrder for a fresh round; finding
 * one returns CombatRoundContinue, errorCode 2, *turnCursor now the
 * next entry to act).
 *
 * **CompactMonsterSlots, deliberately not reproduced**: the original
 * also physically shifts live g_monsterSlots records to keep them
 * front-loaded after a death, then rewrites any g_combatTurnOrder
 * entry that still points at a moved record's old address -- a
 * technical workaround for its raw-pointer turn-order entries. This
 * reimplementation's CombatTurnOrderEntry references monsters by
 * stable slot index rather than by pointer (see the type above), so
 * there's no address to go stale and nothing to fix up; a defeated
 * slot is simply left zeroed at its own index rather than compacted
 * forward. No observable difference in behavior, only in bookkeeping.
 *
 * *turnCursor must start at the index whose turn combatBuildTurnOrder
 * (or the previous round) already caused to be acted on -- i.e. 0
 * right after combatBuildTurnOrder, matching the original's
 * word_32BF4 pointing at turnOrder[0] itself, not "before" it.
 */
typedef enum {
    CombatRoundNoMonstersLeft = 0,
    CombatRoundNewRound = 1,
    CombatRoundContinue = 2
} CombatRoundOutcome;

CombatRoundOutcome combatProcessRound(uint8_t *monsterSlots, CombatTurnOrderEntry *turnOrder, unsigned turnOrderCount,
                                       bool defeated[CombatMonsterSlotCount], unsigned *turnCursor,
                                       MonsterRewardStaging *staging, uint8_t *globalFlags, size_t globalFlagsSize);

#endif
