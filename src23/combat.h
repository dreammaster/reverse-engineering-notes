#ifndef YENDOR23_COMBAT_H
#define YENDOR23_COMBAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bcd4.h"
#include "effect.h"
#include "game.h"
#include "item.h"
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
 * (ProcessCombatRound) are both reimplemented, and so are the two
 * self-contained numeric primitives underneath attack resolution
 * (ResolveAttack, FailsSavingThrow). Composing those into a full
 * attack -- ResolveAttackerActionOutcome, ProcessMonsterAttackTurn,
 * and the player-input attack path inside HandleDungeonInput -- is
 * not: see combatFailsSavingThrow's own doc comment for why
 * ResolveAttackerActionOutcome specifically is deferred.
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

/*
 * ResolveAttack (yendor2.asm:38488, instruction-identical in Chapter
 * 3 -- checked directly): a single attacker-vs-defender damage roll.
 * Misses (returns 0) if power is 0, if accuracy < defense, or if
 * randomInRange(rng, 55) exceeds (accuracy - defense). Otherwise hits:
 * damage = (power * (accuracy - defense) + 50) / 100, clamped to a
 * minimum of 1. In ResolveAttackerActionOutcome's own call (the one
 * confirmed caller this project has traced), accuracy/power come from
 * the attacking monster's MonsterFieldAccuracy/MonsterFieldDamage;
 * defense comes from the defending party member's own
 * PartyStatEquipRating5 -- a genuinely satisfying find, since
 * party.h's own doc comment already noted EquipRating5 accumulates
 * every miscellaneous/armor-slot equipped item's bonus without having
 * a confirmed use for the aggregate; "the party's defense roll against
 * a monster's physical attack" is exactly that use.
 */
uint16_t combatResolveAttack(uint16_t defense, uint16_t accuracy, uint16_t power, RandomState *rng);

/*
 * FailsSavingThrow (yendor2.asm:41947, instruction-identical in
 * Chapter 3 -- checked directly): a generic saving-throw roll. Kept
 * here as a pure function of numbers, matching the original's own
 * genericity -- FailsSavingThrow itself never assumes a record type,
 * only its 5 call sites do (this project has only traced 2 of them,
 * both inside ResolveAttackerActionOutcome; the other 3 -- yendor2.asm
 * :14197/:44666/:48381 -- are elsewhere in the item-effect/status
 * system this project hasn't reached yet).
 *
 * chance = max(5, 5*(defenderStat - threshold) + bonus); rolls
 * randomInRange(rng, 100) against it. Returns true (the throw fails,
 * the effect applies) if the roll exceeds chance, false (resisted)
 * otherwise. In ResolveAttackerActionOutcome's own two call sites,
 * defenderStat is the target's PartyFieldLevel, threshold is the
 * attacking monster's MonsterFieldSaveDifficulty, and bonus is the
 * target's own PartyStatSurvival -- both plainly-named party.h fields,
 * once the defender side was confirmed to always be a party member
 * for this particular caller (ResolveAttackerActionOutcome is only
 * ever reached from ProcessMonsterAttackTurn, monster-attacks-party,
 * not the reverse).
 *
 * ResolveAttackerActionOutcome (below, as combatResolveAttackerAction)
 * composes combatResolveAttack/combatFailsSavingThrow to decide which
 * of 3 outcomes an attack has -- what it does NOT do (matching the
 * original exactly) is apply that outcome: it only decides and
 * describes it. The original stages its decision into a "pending
 * combat event" structure (word_32906, one of 4 g_partyEffectIconSlots
 * entries, 0xC50 + slotIndex*0x14 -- the same icon-bar slot mechanism
 * PrepareTrapEffectSlots/ApplyItemEffectIconSlot already reference)
 * for `ApplyEffectAndDrawIconBar` (yendor2.asm:13789) to read back and
 * apply; this reimplementation instead returns a plain
 * CombatAttackerAction value, leaving the caller to invoke
 * combatApplyEffect (for CombatAttackDamage/StatusEffect) or apply the
 * item replacement directly (for CombatAttackCorrosion -- see its own
 * doc comment for why that specific application isn't reimplemented
 * yet).
 */
bool combatFailsSavingThrow(int16_t defenderStat, int16_t threshold, int16_t bonus, RandomState *rng);

/*
 * ApplyEffectCost's state-mutating half (yendor2.asm:14000,
 * instruction-identical in Chapter 3), the confirmed consumer of
 * ResolveAttackerActionOutcome's staged event once its magnitude and
 * inflicted-status fields are resolved (combat's own callers resolve
 * those two fields differently per branch -- a normal hit uses
 * combatResolveAttack's own damage as the magnitude and skips
 * resistance entirely; a status-effect hit uses effect.h's
 * effectResolveInflictedStatus after a combatFailsSavingThrow roll,
 * and a magnitude drawn directly from the attacking monster's own
 * record rather than rolled -- see file-formats.md's "Attack
 * resolution" section for exactly which fields).
 *
 * Dispatches spend (effect.h's effectSpend(def)) to
 * partyDeductHp/partyDeductMp for EffectSpendHp/Mp/HpAndMp, or to
 * bcd4SubClamped against save's SaveHeaderGold/OreCounter1/OreCounter2
 * for EffectSpendGold/Ore1/Ore2 -- materialAmount is only read in
 * those 3 cases (pass NULL/save NULL for an HP/MP/HpAndMp/None spend).
 * The one confirmed combat material-cost call site: a monster whose
 * special attack lands as branch 2's status-effect application (see
 * above) with an effect def selecting EffectSpendGold (effect id 15
 * in both games -- "takes gold, rolls no magnitude") steals
 * MonsterFieldGoldTheftAmount, the field that made this whole path
 * worth wiring up (confirmed against real WORLD.DAT: every monster
 * with a nonzero value there has exactly this effect as its special
 * attack, in both games -- see monster.h and file-formats.md). ORs
 * inflictedStatus into the defender's PartyFieldStatusFlags if
 * nonzero, regardless of spend type.
 *
 * Deliberately not reproduced (both UI/scratch-state side effects of
 * the original, not state a from-scratch port needs): clearing a
 * raw-pointer "who's targeting whom" scratch table
 * (ClearPartySlotReferenceOnDamage -- this project already tracks
 * combat targets by SaveHeaderPartySlots id, see
 * combatBuildTurnOrder's own design note), recomputing 3 UI-only
 * display-tier globals (UpdatePartyAverageStatTiers -- minimap fog
 * level, a weather overlay tier, and monster-detail reveal tier), and
 * the "resource depleted" overlay bcd4SubClamped's own return value
 * would trigger (the original's ShowResourceDepletedOverlay, pure UI).
 */
void combatApplyEffect(uint8_t *defenderRecord, SaveGame *save, EffectSpend spend, uint16_t amount,
                        const Bcd4 materialAmount, uint16_t inflictedStatus);

/*
 * SelectTrapEffectVariant (yendor2.asm:11373, instruction-identical in
 * Chapter 3): picks which of the attacking monster's two effects
 * applies this attack -- its ordinary one (MonsterFieldAttackEffect)
 * or, 25% of the time, its special one (MonsterFieldSpecialAttack) --
 * always the ordinary one if MonsterStateSpecialAttackDisabled is set
 * or there's no special attack at all (id 0). isSpecial mirrors the
 * original's g_uiScratchFlags4 bit 0x200, the same flag
 * combatResolveAttackerAction's own outer dispatch reads below.
 */
typedef struct {
    unsigned effectId;
    bool isSpecial;
} CombatEffectSelection;

CombatEffectSelection combatSelectTrapEffectVariant(const uint8_t *attackerRecord, RandomState *rng);

/*
 * ResolveAttackerActionOutcome (yendor2.asm:11173, yendor3.asm:2150):
 * decides one attacker-vs-defender action's outcome, given
 * combatSelectTrapEffectVariant's own isSpecial result. Doesn't apply
 * anything itself -- see this header's own design note above.
 *
 * isSpecial == false, or true but the attacker's MonsterFieldFlags has
 * none of MonsterFlagSpecialMask's bits set and its
 * MonsterFieldGoldTheftAmount is exactly 0 (a monster whose special
 * effect got selected but has no theft amount configured -- a
 * defensive fallback in the original, confirmed unreachable in every
 * real record found so far): CombatAttackDamage via
 * combatResolveAttack(defender's PartyStatEquipRating5, attacker's
 * MonsterFieldAccuracy/MonsterFieldDamage), or CombatAttackMiss if
 * that misses.
 *
 * isSpecial == true, attacker's MonsterFieldGoldTheftAmount != 0, and
 * none of MonsterFlagSpecialMask's bits are set:
 * CombatAttackStatusEffect if combatFailsSavingThrow(defender's
 * PartyFieldLevel, attacker's MonsterFieldSaveDifficulty, defender's
 * PartyStatSurvival) fails (goldAmount is a copy of the attacker's
 * MonsterFieldGoldTheftAmount for the caller to spend via
 * combatApplyEffect(EffectSpendGold, ...) once effectId's own
 * definition confirms that's its spend type -- see file-formats.md),
 * else CombatAttackMiss.
 *
 * isSpecial == true and MonsterFlagSpecialMask has a bit set:
 * CombatAttackCorrosion if a *half-bonus* combatFailsSavingThrow
 * (defender's PartyStatSurvival >> 1, matching the original's weaker
 * DC exactly) fails and the defender's equipped item at the selected
 * slot (MonsterFlagCorrodeWeaponSlot -> 0x13A, CorrodeSecondSlot ->
 * 0x142, neither -> 0x146, monster.h) both exists and classifies via
 * itemCorrosionReplacement (item.h); CombatAttackMiss if the save
 * succeeds, the slot is empty, or classification fails.
 *
 * **The actual item replacement isn't reimplemented**: the original
 * routes a corrosion outcome through the same icon-bar machinery as
 * every other effect, and its item-replacing consumer
 * (HandleIconBarItemExpiry) is defined for *item-expiry-on-use/wear*
 * semantics (TickEquippedItemDurability, item charges running out) --
 * this project hasn't confirmed that combat's own corrosion staging
 * feeds it with matching field semantics rather than superficially
 * reusing the same byte offsets. CombatAttackCorrosion gives the
 * caller equipSlotOffset/equippedItemId/corrosionReplacementId to
 * apply directly once that's confirmed, rather than guessing at the
 * write-back here.
 */
typedef enum {
    CombatAttackMiss,
    CombatAttackDamage,
    CombatAttackStatusEffect,
    CombatAttackCorrosion
} CombatAttackOutcome;

typedef struct {
    CombatAttackOutcome outcome;
    uint16_t damage;                 /* CombatAttackDamage */
    Bcd4 goldAmount;                  /* CombatAttackStatusEffect */
    unsigned equipSlotOffset;        /* CombatAttackCorrosion: 0x13A/0x142/0x146 */
    uint16_t equippedItemId;         /* CombatAttackCorrosion */
    uint16_t corrosionReplacementId; /* CombatAttackCorrosion */
} CombatAttackerAction;

CombatAttackerAction combatResolveAttackerAction(const uint8_t *attackerRecord, const uint8_t *defenderRecord,
                                                  const ItemCatalog *catalog, bool isSpecial, RandomState *rng);

/*
 * ApplySavingThrowEffect (yendor2.asm:44646, instruction-identical in
 * Chapter 3): the search/lockpicking trap composition party.h's
 * partyDecodeSavingThrowEffect leaves for "whoever composes this
 * next". actingRecord is the character attempting the lock/search
 * (g_currentPartyRecord in the original -- the one whose own skill can
 * avoid triggering the trap at all).
 *
 * Two independent combatFailsSavingThrow rolls against the *same*
 * decoded threshold, never re-derived per recipient:
 *   1. A trigger roll: defenderStat = actingRecord's own
 *      PartyFieldLevel, bonus = their own PartyStatThievery. Resisting
 *      this one avoids the trap outright -- returns
 *      CombatSavingThrowTrapNone (packedValue == 0 short-circuits to
 *      the same outcome, matching partyDecodeSavingThrowEffect's own
 *      false return, without spending a roll).
 *   2. If the trigger roll fails (the trap goes off), the effect
 *      applies once per recipient via effect.h's own per-recipient
 *      pipeline (effectRollMagnitude when effectRollsMagnitude(def),
 *      then -- only when the effect actually inflicts something and
 *      gates on a save (RollEffectResistance's own double early-out,
 *      yendor2.asm:14127) -- a second, independent
 *      combatFailsSavingThrow roll: same threshold, bonus =
 *      effectResistanceBonus(def, recipient) this time, not
 *      PartyStatThievery -- then combatApplyEffect).
 *      effectId < 50 (partyDecodeSavingThrowEffect's wholeParty ==
 *      false): actingRecord alone. >= 50: every occupied,
 *      non-incapacitated SaveHeaderPartySlots member.
 *
 * **Two quirks reproduced exactly, not "fixed"**: (1) the original
 * never populates a gold/ore effect's material amount for this call
 * path at all (RollEffectMagnitude's own bits-0-2 early-out leaves it
 * at the icon slot's cleared 0) -- passing a zeroed Bcd4 here matches
 * that rather than inventing a nonzero source. (2) the whole-party
 * scan stops dead at the first *unoccupied* slot instead of skipping
 * past it (ApplySavingThrowEffect's own di-indexed loop tests
 * `[di] == 0` with a jump straight past the remaining iterations, not
 * a per-slot skip) -- real party layouts are always front-packed in
 * practice, so this is presumed unobserved, but it's what the
 * disassembly does.
 *
 * effectGetDef failing (an out-of-range effectId) isn't a case the
 * original guards against -- its own PrepareTrapEffectSlots indexes
 * g_trapEffectDefs unconditionally. Treated here as
 * CombatSavingThrowTrapNone rather than reading past the table, since
 * real CURGAME/lock data is expected to always encode a valid id.
 */
typedef enum {
    CombatSavingThrowTrapNone,   /* packedValue == 0, the trigger roll was avoided, or an invalid effect id */
    CombatSavingThrowTrapSingle, /* applied to actingRecord alone */
    CombatSavingThrowTrapParty   /* applied to some subset of the party */
} CombatSavingThrowTrapOutcome;

CombatSavingThrowTrapOutcome combatApplySavingThrowTrap(uint16_t packedValue, uint8_t *actingRecord,
                                                          SaveGame *save, GameKind game, RandomState *rng);

#endif
