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
#include "spellrecord.h"

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
 * **The actual item replacement -- confirmed and composable, see
 * combatApplyCorrosion below.** Earlier rounds left this open,
 * unsure whether combat's own corrosion staging feeds
 * HandleIconBarItemExpiry (defined for *item-expiry-on-use/wear*
 * semantics) with matching field semantics or just superficially
 * reuses the same byte offsets. Tracing ResolveAttackerActionOutcome's
 * corrosion branch and GetClassifiedItemStatField (yendor2.asm:19410,
 * instruction-identical in Chapter 3) directly resolved it: they
 * match exactly. GetClassifiedItemStatField(ax=equipped item id)
 * leaves ax unchanged and returns bx = itemCorrosionReplacement's own
 * result (0 on failure) -- the *exact* (equippedItemId,
 * corrosionReplacementId) pair partyHandleIconBarItemExpiry
 * (party.h) expects, and the icon slot's own +0x8/+0xA fields hold
 * the attacker's own selected trap-effect id/definition (from
 * combatSelectTrapEffectVariant's PrepareTrapEffectSlots call) --
 * whose modeFlags is exactly what ApplyEffectAndDrawIconBar's real
 * dispatch tests to route a slot to HandleIconBarItemExpiry in the
 * first place.
 *
 * Confirmed against real data: of both games' full monster rosters,
 * only one has a legitimate MonsterFlagSpecialMask flag combination
 * paired with an in-range special-attack effect id -- Chapter 3's
 * CROCODILE (catalog block 70), whose special-attack effect (id 22)
 * has modeFlags EffectModeItemReplace. Chapter 2 has no such monster
 * at all; its own sole flag-matching block (index 60) is an unnamed
 * placeholder with out-of-range effect ids, reached only by unused
 * type-id lookup slots -- not a real monster. So this whole mechanic
 * is vanishingly rare in practice (a single Chapter 3 creature), but
 * now fully composable and correct when it does trigger.
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
 * The equipment-corrosion write-back CombatAttackerAction's own doc
 * comment above describes: applies action's CombatAttackCorrosion
 * fields to defenderRecord via partyHandleIconBarItemExpiry (party.h),
 * using the attacking monster's own selected trap-effect definition
 * (selection, the same value combatSelectTrapEffectVariant already
 * returned for this attack -- its effectId's own modeFlags is what
 * the original's real dispatch keys off of) as the mode-bit source.
 * A no-op if action->outcome isn't CombatAttackCorrosion, or if
 * selection->effectId doesn't resolve to a valid definition (shouldn't
 * happen for an action combatResolveAttackerAction itself produced,
 * but guarded rather than assumed).
 */
void combatApplyCorrosion(uint8_t *defenderRecord, const ItemCatalog *catalog, GameKind game,
                           const CombatEffectSelection *selection, const CombatAttackerAction *action);

/*
 * ApplyTargetResistancesToAttack (yendor2.asm:52799, instruction-
 * identical in Chapter 3): filters a pending player-triggered
 * item/spell attack against a map monster (`g_levelMonsters`,
 * monsterpool.h) before it's committed -- one piece of a small,
 * genuinely separate attack-resolution family
 * (`ApplyAttackToTarget`/`ApplyDamageToMapMonster`, the "corridor/
 * ranged-attack path" `ApplyEncodedItemEffect`'s bits `0x4`/`0x2000`
 * both reach) this project surfaced while resolving the equipment-
 * corrosion write-back. Confirmed, cross-referencing every field
 * offset the whole family touches against monster.h's own already-
 * named fields, that the record these functions operate on is a full
 * `MonsterRecordSize` record -- resolving the original's own "di's
 * record type here is unknown" uncertainty (its own contemporaneous
 * comment).
 *
 * attackFlags is the caller's own `word_33304` (an untraced caller
 * context, same status as `ApplyEncodedItemEffect`'s other
 * caller-supplied globals -- not yet connected to a specific effect
 * definition or call site). Bits `0x400`-`0x8000` (monster.h's own
 * `MonsterImmunity` high bits) are candidate inflicted-status bits,
 * tested only when at least one of them is set: each survives into
 * the returned `statusFlags` only if the target lacks the matching
 * `MonsterFieldImmunities` bit. Bits `0x1`/`0x2`/`0x4`/`0x8`/`0x10`
 * (the same enum's low bits) are checked unconditionally instead: any
 * match against the target's own immunities zeroes `damage` entirely
 * and returns immediately -- a clean resist, not a partial filter.
 *
 * resistanceFlags is `word_33306` (bits `0x200`-`0x8000`, monster.h's
 * own `MonsterResistMagicMask`/`PhysicalMask`, tested in that exact
 * order): the first bit that's both requested and matches the
 * target's own `MonsterFieldResistances` halves `damage` and returns
 * immediately. Requesting bit `0x200` specifically also returns
 * immediately even on a *miss* (a genuine asymmetry -- it's the last
 * bit checked, so the original simply falls out of the chain rather
 * than continuing) -- reproduced exactly, not smoothed into matching
 * the other 6 bits' "keep checking" behavior.
 *
 * Only when resistanceFlags didn't request bit `0x200` at all does a
 * drain effect get a chance to run: if attackFlags has any of bits
 * `0x20`-`0x200` set, `drainAmount` is subtracted (floored at 0) from
 * one of the target's own combat-stat fields, selected by priority --
 * `0x200`->`MonsterFieldHealth`, `0x100`->`MonsterFieldAccuracy`,
 * `0x80`->`MonsterFieldDexterity`, `0x40`->`MonsterFieldAbsorption`,
 * `0x20`->`MonsterFieldDamage` -- a genuine "drain the monster's own
 * stat" effect category, distinct from (and reached via a completely
 * different code path than) ordinary HP damage; mutates targetRecord
 * directly rather than being reflected in the returned result.
 *
 * Composed with its own callers as combatResolveSpellAttack/
 * combatApplySpellAttack below, now that spellrecord.h has resolved
 * where attackFlags/resistanceFlags/drainAmount and the base magnitude
 * actually come from (the word_332D8-33306 cluster, file-formats.md).
 * `ApplyDamageToMapMonster` (adds death/reward handling on top, already
 * fully reimplemented elsewhere as `monsterGrantRewards`/
 * `monsterPoolRemove`) is still a separate, not-yet-composed step.
 */
typedef struct {
    uint16_t damage;
    uint16_t statusFlags;
} CombatTargetAttackResult;

CombatTargetAttackResult combatApplyTargetResistances(uint8_t *targetRecord, uint16_t damage, uint16_t attackFlags,
                                                        uint16_t resistanceFlags, uint16_t drainAmount);

/*
 * ApplyAttackToTarget/TryResolveAttackAgainstTarget (yendor2.asm:53164/
 * 52756, instruction-identical in Chapter 3): the composed attack a
 * player-triggered spell/item effect makes against a map monster
 * (`g_levelMonsters`), now that spellrecord.h has resolved the
 * "word_332D8-33306 cluster" these two functions read as a genuine
 * 80-byte spell/ability catalog record rather than untraced caller
 * state.
 *
 * alreadyResolved is the caller's own `g_uiScratchFlags4` bit `0x80` --
 * still an untraced caller-context input (which of
 * `ApplyEncodedItemEffect`'s call sites sets it isn't pinned down), but
 * its own effect here is fully clear: true skips `combatResolveAttack`'s
 * roll entirely and uses `SpellFieldAttackMagnitude` directly as the
 * damage.
 *
 * Both paths share the same gate first: if the spell record's
 * `SpellResistTypeRestricted` bit is set, the target's own
 * `MonsterFieldUnknown4E` must equal `SpellFieldTargetTypeId` or the
 * whole attack is a no-op (`hasEffect = false`, no roll attempted at
 * all -- this is `TryResolveAttackAgainstTarget`'s own gate, reproduced
 * once here since both callers apply the identical condition to the
 * identical fields). Otherwise: alreadyResolved short-circuits straight
 * to `SpellFieldAttackMagnitude`; the normal path rolls
 * `combatResolveAttack(target's MonsterFieldAbsorption, caster's
 * PartyStatCasting, SpellFieldAttackMagnitude, rng)` and bails on a
 * roll of 0 (resistances are never even consulted -- a clean miss, not
 * a 0-damage hit) -- `alreadyResolved` skips this zero-check too, so a
 * direct hit with magnitude 0 still reaches resistances/drain.
 *
 * Either way, the resulting damage is filtered through
 * `combatApplyTargetResistances` using the record's own
 * `SpellFieldAttackFlags`/`SpellFieldResistFlags`/`SpellFieldDrainAmount`.
 * `hasEffect` is false (no commit at all) if nothing survives that --
 * no damage and no status.
 */
typedef struct {
    uint16_t damage;
    uint16_t statusFlags;
    bool hasEffect;
} CombatSpellAttackResult;

CombatSpellAttackResult combatResolveSpellAttack(const uint8_t *targetRecord, const uint8_t *casterRecord,
                                                   const uint8_t *spellRecord, bool alreadyResolved, RandomState *rng);

/*
 * The commit half of the pair above (`ApplyAttackToTarget`'s own tail,
 * past its call to `ApplyTargetResistancesToAttack`). A no-op if
 * `!result.hasEffect`. Otherwise, in the original's exact order:
 *
 *  1. If the spell record's `SpellResistHalfTargetDamage` bit is set,
 *     damage is unconditionally replaced by half the TARGET's own
 *     `MonsterFieldDamage` (even if `result.damage` was 0) -- a
 *     genuinely separate "reflect part of its own attack back" damage
 *     source, not a modifier on the rolled/preset value. Exactly one
 *     real record in each game sets this bit.
 *  2. `MonsterFieldState` unconditionally gains `MonsterStateAware`
 *     and `MonsterStateHitFlashPending` (the latter a one-shot render
 *     cue, see monster.h), then `MonsterFieldHealth` is reduced by the
 *     (possibly just-replaced) damage.
 *  3. If any status survived: those bits are OR'd into
 *     `MonsterFieldState` itself (this is how a landed Cursing/Hexing
 *     status also happens to set `MonsterStateSpecialAttackDisabled`/
 *     `MonsterStateBusy` -- same bit positions, not a separate
 *     mechanism -- see monster.h), and `SpellFieldTickAmount`/
 *     `TickCountdown` are copied into the target's own
 *     `MonsterFieldTickAmount`/`TickCountdown` unconditionally.
 *     Additionally, if the record's `SpellFlagsAPersistAffliction` bit
 *     is set, the same surviving status bits are *also* OR'd into
 *     `MonsterFieldImmunities` to mark the target as currently
 *     afflicted (monster.h's own note on that field's dual role) --
 *     without this bit, the tick-timer fields still get armed, but the
 *     target isn't marked as already-afflicted for a future
 *     reapplication to skip.
 *  4. If the spell record's `SpellResistClearAware` bit is set,
 *     `MonsterStateAware` is cleared again -- reproduced for fidelity,
 *     though no real record in either game ever sets this bit (see
 *     spellrecord.h).
 */
void combatApplySpellAttack(uint8_t *targetRecord, const uint8_t *spellRecord, CombatSpellAttackResult result);

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

/*
 * ApplyEncodedItemEffect's single-target and whole-party status-effect
 * branches (yendor2.asm:51106 bits 0x8000/0x4000 of word_33302,
 * yendor3.asm:51993 same bits) -- the two branches roadmap.md already
 * flagged as reusing this project's icon-bar effect machinery, out of
 * that function's ~19-branch bitmask dispatch. Unlike
 * combatApplySavingThrowTrap, there's no roll here at all: the caller
 * supplies an already-resolved inflictedStatus/magnitude pair
 * (word_332DC/word_332DE in the original) -- confirmed by reading
 * RollEffectResistance/RollEffectMagnitude's own "already resolved"/
 * "already set" early-outs directly against this exact call path: this
 * is the concrete case those two functions' own doc comments were
 * describing generically, without a concrete example. Presumably
 * RunAlchemyScreen's own ingredient-mixing preview computes those two
 * words; that computation isn't traced.
 *
 * A gate confirmed via ResetOrCopyTargetPositionFields
 * (yendor2.asm:53238, instruction-identical in Chapter 3): if a global
 * flag is set AND the *acting* character (g_currentPartyRecord, not
 * the recipient) is Cursed, both values are zeroed instead of applied
 * -- narrative unconfirmed (a cursed character's alchemy/container use
 * fizzling?), but the mechanism is exact. combatResolveEncodedItemEffectValue
 * is this gate, standalone so a caller computes it once and reuses it
 * for every recipient -- the original recomputes it per icon slot, but
 * it only ever depends on the acting record, so the result can't
 * change within one call.
 *
 * combatApplyEncodedItemEffectSingle: despite searching the 4 existing
 * icon-bar slots for a reusable one, the record actually affected is
 * unconditionally the acting character -- the slot search only picks
 * a UI icon-slot index, never the target. Not modeled here (no
 * icon-bar UI to reuse a slot index for).
 *
 * combatApplyEncodedItemEffectParty: loops SaveHeaderPartySlots and
 * reproduces the same "stops dead at the first unoccupied slot" quirk
 * already found in combatApplySavingThrowTrap's own whole-party
 * branch -- but, confirmed by reading both games directly, does NOT
 * skip incapacitated members the way that other mechanism does.
 * **A real Chapter 2 vs. Chapter 3 difference found here**: Chapter 3
 * adds a per-recipient skip for a Cursed party member (left untouched,
 * not even icon-slot-populated) that Chapter 2 lacks entirely --
 * gated on `game` here. This is independent of the acting-character
 * curse gate above: a whole-party application can be entirely zeroed
 * (acting character cursed) while also skipping specific cursed
 * recipients (Chapter 3 only) -- two unrelated checks that happen to
 * both key off PartyStatusCursed.
 *
 * Both functions are the confirmed, reusable core; the surrounding
 * dispatch decision (which of word_33302's ~19 bits fires, and
 * whether word_33300's own 0x800/0x1000 bits -- read from an untraced
 * caller context -- select this path at all, versus skipping the
 * icon-bar entirely) is not reimplemented. See roadmap.md candidate 8
 * for the rest of ApplyEncodedItemEffect.
 */
typedef struct {
    uint16_t inflictedStatus;
    uint16_t magnitude;
} CombatEncodedItemEffectValue;

CombatEncodedItemEffectValue combatResolveEncodedItemEffectValue(bool curseGateActive, const uint8_t *actingRecord,
                                                                   uint16_t inflictedStatus, uint16_t magnitude);

void combatApplyEncodedItemEffectSingle(uint8_t *actingRecord, SaveGame *save, unsigned effectId, GameKind game,
                                          CombatEncodedItemEffectValue value);

void combatApplyEncodedItemEffectParty(SaveGame *save, unsigned effectId, GameKind game,
                                        CombatEncodedItemEffectValue value);

/*
 * The "wall/door trap" side of the side-trap/ambush pipeline
 * (`file-formats.md`'s "side trap"/ambush section; `roadmap.md`
 * candidate 6). Resolved 2026-09-30: there is no separate creation
 * mechanism for a wall/door trap pool entry -- an exhaustive
 * whole-binary search (both games) for any write to `MonsterFieldWound`
 * (`monster.h`, `+0xE`) bit `0x1000` (`MonsterWoundAmbushPending`)
 * found exactly one site in each game, and it's `monsterApproachParty`'s
 * own ambush-arming write (`monsterpool.c`), already reimplemented. A
 * "wall/door trap" pool entry is simply an ordinary monster record in
 * the ambush-pending state, presented differently depending on which
 * direction the party is currently facing relative to it.
 *
 * `RollTrapAvoidanceMagnitude` (yendor2.asm:32706) is the roll:
 * `survivalStat` is the acting party member's own `PartyStatEquipRating5`
 * (`party.h`, confirmed by offset: `PartyFieldStats + 10*2` == `0x50`,
 * the same physical-defense stat `combatResolveAttack` already uses),
 * `threshold`/`magnitudeCap` are a trap pool entry's own
 * `MonsterFieldRangedAccuracy`/`MonsterFieldRangedDamage` fields
 * (`monster.h`, `+0x64`/`+0x66`) -- confirming "trap" pool entries
 * really do carry a full monster catalog block, reusing its ordinary
 * ranged-attack fields as trap avoidance threshold/magnitude data
 * rather than needing a separate trap-specific record shape.
 * Higher `survivalStat` means both less likely to trigger and a
 * smaller magnitude when it does. Called unconditionally (rolls -- and
 * consumes RNG -- every movement step for every armed trap slot,
 * regardless of the party's current facing; fidelity matters here for
 * RNG-draw-count parity with the original). **A real Chapter 2 vs.
 * Chapter 3 difference, easy to miss**: Chapter 2 rolls
 * `RandomInRange(100)`; Chapter 3 rolls `RandomInRange(55)` instead --
 * the final `magnitudeCap * margin / 100` formula is unchanged in both
 * games, only the roll's own upper bound shrinks, so Chapter 3 traps
 * trigger noticeably more often for the same margin (any margin >= 55
 * always triggers in Chapter 3, vs. needing a margin of 100 to always
 * trigger in Chapter 2). Confirmed by reading both games' disassembly
 * side by side, not assumed from the otherwise instruction-identical
 * surrounding code.
 *
 * The facing gate (`TriggerSideTrapForRandomPartyMember`,
 * yendor2.asm:32889, instruction-identical in Chapter 3) is a separate,
 * unconditional-after-the-roll check: exactly one of the 4
 * `MonsterWoundPartyMustFace*` bits is tested, selected by the party's
 * *current* `SaveFacing` (not the monster's position -- that's a
 * different check `monsterApproachParty` already does when arming the
 * trap). Only when it matches does the original go on to stage
 * icon-bar/sound presentation; that presentation step (a 4-slot
 * scratch table, `PresentTriggeredSideTrapEffects`'s own drawing/sound
 * sequencing) is UI-heavy and not reimplemented here.
 */
uint16_t combatRollTrapAvoidanceMagnitude(GameKind game, uint16_t survivalStat, uint16_t threshold,
                                            uint16_t magnitudeCap, RandomState *rng);

typedef struct {
    uint16_t magnitude; /* combatRollTrapAvoidanceMagnitude's result -- 0 means avoided */
    bool facingReady;   /* true if the party's current facing matches the trap's own armed direction */
} CombatSideTrapOutcome;

CombatSideTrapOutcome combatResolveSideTrap(GameKind game, uint16_t partyFacing, uint16_t monsterWoundFlags,
                                              uint16_t survivalStat, uint16_t threshold, uint16_t magnitudeCap,
                                              RandomState *rng);

#endif
