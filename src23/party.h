#ifndef YENDOR23_PARTY_H
#define YENDOR23_PARTY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bcd4.h"
#include "effect.h"
#include "game.h"
#include "item.h"
#include "savegame.h"

/*
 * Party-member records: the 500-byte structures at g_partyRecords (nine per
 * save, stored in section 1 of CURGAME right after the game-state block; see
 * savegame.h). All accessors take a pointer to one record's first byte and
 * use little-endian u16 fields. Field meanings were checked against the
 * disassembly and against the four real Chapter 2 characters.
 *
 * Stats are stored in pairs: the value at offset X is the current value and
 * the copy at X + 0x40 is the maximum/natural value (e.g. HP 0x52/0x92).
 *
 * Chapter 3 uses the same offsets: its inventory/equipment lookup, class
 * tables and stat-name table index the record identically. It differs only
 * in a few names and one equipment-slot quirk (see partyStatName and
 * partyEquipmentSlot). Chapter 3 has no local save file to check data against.
 */

enum {
    PartyRecordSize = 500,
    PartyNameMaxLength = 13,
    PartyNameBufferSize = PartyNameMaxLength + 1,
    PartyProtectionCount = 9,
    PartyMaxStatOffset = 0x40 /* distance from a current stat to its maximum */
};

typedef enum {
    PartyFieldName = 0x00,        /* NUL-terminated, <= 13 chars */
    PartyFieldClass = 0x0E,       /* u16 class id, see partyClassName */
    PartyFieldGender = 0x10,      /* u16: 1 or 2 (the two real female characters are 2) */
    PartyFieldUnknown12 = 0x12,   /* u16, values 20-33 in real data; meaning not identified */
    PartyFieldPortrait = 0x14,    /* u16 portrait icon id */
    PartyFieldLevel = 0x16,       /* u16, capped at 90 by training items */
    PartyFieldExperience = 0x18,  /* Bcd4 */
    PartyFieldStatusFlags = 0x1C, /* u16, PartyStatus bits */
    /*
     * u16; set by partyCheckForLevelUp. Purely a UI hint (ShowLevelUpMessage's
     * second line, DrawPartyMemberStatusPanel's training-icon glyph) -- nothing
     * reads it back to raise PartyFieldLevel. The actual level increase is a
     * wholly separate mechanic: UseTrainingItem (yendor2.asm:21510, not
     * reimplemented -- large and UI-heavy) pays a gold cost, increments
     * PartyFieldLevel by exactly 1 (capped at 90), and grows several stats by
     * class-dependent percentages, all independent of this field's value.
     */
    PartyFieldPendingLevel = 0x1E,
    PartyFieldProtections = 0x20, /* 9 x u16 resistance values, order of PartyProtection */
    /*
     * A 5 x u16 gap between PartyFieldProtections and PartyFieldStats:
     * PartyStatEquipRating1-5's baseline, reset from these fields by
     * RecomputeEquipmentStatBonuses (yendor2.asm:19907, see
     * partyRecomputeEquipmentStatBonuses) before that function adds
     * equipped items' own bonuses on top. Note the swapped order --
     * EquipRating2's baseline is this gap's *third* slot, EquipRating3's
     * is its *second*, matching the original exactly. Nothing read so
     * far writes EquipRatingBase1/2/3 (they may simply always be 0,
     * or be set by an unread character-creation/leveling path); the
     * last two slots are separately traced and named below.
     */
    PartyFieldEquipRatingBase1 = 0x32, /* u16; EquipRating1's baseline */
    PartyFieldEquipRatingBase3 = 0x34, /* u16; EquipRating3's baseline (note: base *3*, not 2) */
    PartyFieldEquipRatingBase2 = 0x36, /* u16; EquipRating2's baseline (note: base *2*, not 3) */
    PartyFieldStrengthBonus = 0x38,    /* u16; 20% of (current Strength - 72), 0 if <= 72; EquipRating4's baseline */
    PartyFieldDexterityBonus = 0x3A,   /* u16; 20% of (current Dexterity - 72), 0 if <= 72; EquipRating5's baseline */
    PartyFieldStats = 0x3C,       /* 27 x u16 current values, indexed by PartyStat */
    /* Same 5-slot gap and same fields as above, mirrored for the max-value side. */
    PartyFieldEquipRatingBase1Max = 0x72,
    PartyFieldEquipRatingBase3Max = 0x74,
    PartyFieldEquipRatingBase2Max = 0x76,
    PartyFieldStrengthBonusMax = 0x78,  /* u16; 20% of (max Strength - 72), 0 if <= 72 */
    PartyFieldDexterityBonusMax = 0x7A, /* u16; 20% of (max Dexterity - 72), 0 if <= 72 */
    PartyFieldStatsMax = 0x7C,    /* 27 x u16 maximum values, same indexing */
    PartyFieldAbilities = 0xB4,   /* u16 bitmask of learned special abilities (0x8000..0x1000) */
    PartyFieldAbilityCharge = 0xB6, /* 4 x u16 charge counters, one per ability bit, high bit first */
    PartyFieldWearMain = 0xBE,    /* u16 wear counter for equipment slot 0xA (main weapon) */
    PartyFieldWearSecond = 0xC0,  /* u16 wear counter for equipment slot 0xC */
    PartyFieldWearThird = 0xC2,   /* u16 wear counter for equipment slot 0xD */
    PartyFieldFlagBankCA = 0xCA,  /* 16 words = 256 flags: known spells/abilities */
    PartyFieldFlagBank10C = 0x10C, /* 6 words = 96 flags: per-character one-time events */
    PartyFieldInventory = 0x118,  /* main InventoryGroup (34 bytes) */
    PartyFieldEquipment = 0x13A,  /* equipment slots, see partyEquipmentSlot */
    PartyFieldUiFlags = 0x15C,    /* u16, PartyUiFlags: which inventory group is displayed etc. */
    PartyFieldBagMarkers = 0x17C  /* 3 open-bag records of 0x26 bytes, see partyBagMarker */
} PartyField;

/* Indices into the stat arrays at PartyFieldStats / PartyFieldStatsMax (the game's 27-entry name table). */
typedef enum {
    PartyStatStrength,
    PartyStatDexterity,
    PartyStatStamina,
    PartyStatIntelligence,
    PartyStatWisdom,
    PartyStatCharisma,
    /*
     * Five equipment-derived ratings (0x48-0x50); unnamed in the game's
     * table, but their derivation is fully traced -- see
     * partyRecomputeEquipmentStatBonuses. Roughly: 1/2 come from the main
     * weapon (code 0xA) plus your Projectile skill, 3/4 from the second
     * slot (code 0xC) plus a melee skill selected by its item type, and 5
     * accumulates every other equipped item's (codes 0xD-0x14) own bonus.
     */
    PartyStatEquipRating1,
    PartyStatEquipRating2,
    PartyStatEquipRating3,
    PartyStatEquipRating4,
    PartyStatEquipRating5,
    PartyStatHitPoints,
    PartyStatMagicPoints,
    PartyStatCarryCapacity, /* 10 x Strength (current/max), unnamed in the table -- see partyRefreshCarryCapacityAndAttributeBonuses */
    PartyStatSurvival,
    PartyStatProjectile,
    PartyStatSlashing,
    PartyStatBashing,
    PartyStatPolearm,
    PartyStatCasting,
    PartyStatMapping,
    PartyStatNavigation,
    PartyStatBartering,
    PartyStatRepair,
    PartyStatThievery,
    PartyStatLinguistics,
    PartyStatChemistry,
    PartyStatCount
} PartyStat;

typedef enum {
    PartyStatusSecondaryClassMask = 0x003F, /* one bit per secondary class, see partyClassSecondaryBit */
    PartyStatusDead = 0x0040,
    PartyStatusCursed = 0x0080,
    PartyStatusHexed = 0x0100,
    PartyStatusJinxed = 0x0200,
    PartyStatusStoned = 0x0400,
    PartyStatusFrozen = 0x0800,
    PartyStatusParalyzed = 0x1000,
    PartyStatusDiseased = 0x2000,
    PartyStatusPoisoned = 0x4000,
    PartyStatusSick = 0x8000,
    PartyStatusIncapacitated = 0x1C40 /* dead, stoned, frozen or paralyzed: can't act */
} PartyStatus;

/* Order of the nine values at PartyFieldProtections. */
typedef enum {
    PartyProtectionDisease,
    PartyProtectionPoison,
    PartyProtectionSickness,
    PartyProtectionStoning,
    PartyProtectionFrozen,
    PartyProtectionParalyze,
    PartyProtectionCursing,
    PartyProtectionHexing,
    PartyProtectionJinxing
} PartyProtection;

/* PartyFieldUiFlags bits set by GetInventorySlotPtr to record what the inventory screen is showing. */
typedef enum {
    PartyUiEquipmentShort = 0x0040, /* last slot looked up was a 2-byte equipment slot */
    PartyUiMainGroup = 0x0080,
    PartyUiBag3 = 0x0100,
    PartyUiBag2 = 0x0200,
    PartyUiBag1 = 0x0400
} PartyUiFlags;

uint16_t partyGetU16(const uint8_t *record, unsigned offset);
void partySetU16(uint8_t *record, unsigned offset, uint16_t value);

void partyGetName(const uint8_t *record, char out[PartyNameBufferSize]);

/* Current and maximum value of a stat; 0 for an out-of-range stat. */
uint16_t partyGetStat(const uint8_t *record, PartyStat stat);
uint16_t partyGetStatMax(const uint8_t *record, PartyStat stat);
void partySetStat(uint8_t *record, PartyStat stat, uint16_t value);
void partySetStatMax(uint8_t *record, PartyStat stat, uint16_t value);

/*
 * The game's name for a stat ("STRENGTH", ...), or NULL for the unnamed ones.
 * Chapter 3 calls hit points "HEALTH" and has no name for the chemistry
 * skill (its slot is still in the record).
 */
const char *partyStatName(PartyStat stat, GameKind game);

uint16_t partyGetProtection(const uint8_t *record, PartyProtection protection);

uint8_t *partyExperience(uint8_t *record); /* Bcd4 */

/*
 * DeductHPClamped (yendor2.asm:13965, instruction-identical in Chapter
 * 3): subtracts amount from PartyStatHitPoints' current value, clamped
 * at 0; reaching 0 also sets PartyStatusDead. The original also calls
 * ClearPartySlotReferenceOnDamage (clears a raw-pointer "who's
 * targeting whom" scratch table this project doesn't model -- targets
 * are tracked by SaveHeaderPartySlots id instead, see combat.h) and
 * UpdatePartyAverageStatTiers (recomputes 3 UI-only display-tier
 * globals) as side effects here; neither is reproduced.
 */
void partyDeductHp(uint8_t *record, uint16_t amount);

/* DeductMPClamped (yendor2.asm:13985, instruction-identical in Chapter 3): subtracts amount from
 * PartyStatMagicPoints' current value, clamped at 0. */
void partyDeductMp(uint8_t *record, uint16_t amount);

/*
 * CheckForLevelUp (yendor2.asm:20036, yendor3.asm:12025, instruction-
 * identical): walks the per-game 89-entry XP-threshold table
 * (partyXpThresholdTable) from the character's current PartyFieldLevel,
 * advancing while PartyFieldExperience is >= the next entry. If the
 * result exceeds the current level, stores it into PartyFieldPendingLevel
 * (left at 0 otherwise) -- purely a UI eligibility hint, per that field's
 * own doc comment; XP alone never raises PartyFieldLevel (see
 * PartyFieldPendingLevel for the actual, separate leveling mechanic).
 * PartyStatusIncapacitated characters are always left at
 * PartyFieldPendingLevel = 0, matching the original's own early-out.
 * Returns true if a level-up is now pending.
 */
bool partyCheckForLevelUp(uint8_t *record, GameKind game);

/*
 * ApplyIconBarStatDelta (yendor2.asm:14068 vs. yendor3.asm:6623) --
 * ApplyEffectAndDrawIconBar's third dispatch variant (effect.h's
 * EffectModeStatCapped = 0x100 / EffectModeStatFloor = 0x80 mode
 * bits, passed here as modeFlags), for a caller this project hasn't
 * traced yet: unlike the "plain damage/status" dispatch's confirmed
 * callers, this one needs the icon slot's own +0x10/+0x12 fields
 * populated with raw *field offsets* into the recipient's own record
 * rather than resolved values, and no traced caller does that -- so
 * the specific stat this applies to in practice is still unconfirmed.
 *
 * EffectModeStatCapped set (checked first -- wins if both happen to
 * be set): adds delta to the value at currentFieldOffset, capped at
 * the value at maxFieldOffset -- genuinely data-driven, not hardcoded
 * to one stat. Pass maxFieldOffset == 0 for "uncapped" (**Chapter 3
 * fixes a real Chapter 2 bug here**: Chapter 2 has no zero guard and
 * would read the party record's own first 2 bytes -- PartyFieldName's
 * start -- as the cap whenever maxFieldOffset is 0; Chapter 3 adds
 * the check. Reimplemented once, matching Chapter 3's corrected
 * behavior for both games -- see engine-diffs.md). Else, if
 * EffectModeStatFloor is set: subtracts delta from the value at
 * currentFieldOffset instead, floored at 0 (maxFieldOffset unused).
 * Neither bit set: no stat change at all, but the tail below still
 * runs (matches the original's own unconditional fallthrough).
 *
 * Both paths finish by ANDing statusFlagsClearMask into
 * PartyFieldStatusFlags (pass 0xFFFF for a no-op -- the original's
 * own caller-context suppression, word_33306 bits 0x40/0x80, an
 * untraced source), then partyRefreshCarryCapacityAndAttributeBonuses
 * + partyCheckForLevelUp (UpdatePartyAverageStatTiers, pure UI, is
 * not). **A second Chapter 2 bug, also fixed in Chapter 3**: the
 * original RefreshCarryCapacityAndAttributeBonuses/CheckForLevelUp
 * read g_currentPartyRecord internally rather than taking a record
 * parameter, and Chapter 2's own ApplyIconBarStatDelta never updates
 * that global before calling them -- for a whole-party effect they'd
 * silently operate on whatever record was left over from an earlier,
 * unrelated call. Chapter 3 saves/sets/restores g_currentPartyRecord
 * around the same 3 calls so each recipient gets its own correct
 * refresh. This reimplementation always takes record as an explicit
 * parameter, matching Chapter 3's corrected behavior for both games.
 */
void partyApplyIconBarStatDelta(uint8_t *record, GameKind game, uint16_t modeFlags, uint16_t delta,
                                 unsigned currentFieldOffset, unsigned maxFieldOffset, uint16_t statusFlagsClearMask);

/*
 * ApplyMultiStatEffectForItem / RemoveMultiStatEffect (yendor2.asm:18864/
 * :19135, instruction-identical in Chapter 3): the add/remove halves of
 * applying an item's own multi-stat bonus table -- item.h's
 * itemEffectEntry, already fully decoded (up to 4 (field, amount)
 * pairs, each field confirmed to be a *raw byte offset* directly into
 * the party record, not an index into any name table -- the earlier
 * "type id" framing in this project's own docs undersold how literal
 * it is). effect is an itemEffectEntry result; NULL is a no-op
 * (matching the original's own "no effect table for this item" gate).
 *
 * Two real asymmetries reproduced exactly, not smoothed over, both
 * turning on whether a pair's field is below or at/above 0x32 (50) --
 * matching PartyFieldProtections' own 9-field range (0x20-0x30) below
 * that split and PartyFieldStats/StatsMax's ranges (0x3C+/0x7C+) at or
 * above it:
 *   - field >= 0x32 additionally skips the pair entirely when the
 *     target field currently reads 0 -- the same "an untrained stat
 *     isn't grown from zero" convention already used by
 *     partyApplyTraining's own trainingAddStatMaxCapped. field < 0x32
 *     (protections) always applies.
 *   - On removal, field >= 0x32 is floor-clamped at 0; field < 0x32 is
 *     NOT -- subtracting past 0 wraps the u16 field, an original quirk
 *     this reimplementation reproduces via ordinary unsigned
 *     wraparound rather than "fixing" it with an unwritten floor.
 * On addition (both ranges), the result is capped at 999.
 */
void partyApplyMultiStatEffect(uint8_t *record, const uint8_t *effect);
void partyRemoveMultiStatEffect(uint8_t *record, const uint8_t *effect);

/*
 * HandleIconBarItemExpiry (yendor2.asm:13896 vs. yendor3.asm:6429,
 * instruction-identical) -- ApplyEffectAndDrawIconBar's last dispatch
 * variant (effect.h's EffectModeItemDestroy = 0x200 / EffectModeItemReplace
 * = 0x400 mode bits, passed as modeFlags): an equipped item's timed
 * effect expiring, either transforming it into a different item or
 * destroying it outright. This is the piece that closes out
 * `ApplyEffectAndDrawIconBar`'s full 3-way dispatch (the other two,
 * plain damage/status and stat-delta, were already done).
 *
 * Always removes equippedItemId's own stat bonuses first (via
 * partyRemoveMultiStatEffect + itemEffectEntry/itemCatalogRecord).
 * modeFlags & EffectModeItemDestroy (0x200) selects what happens at
 * record + slotOffset next:
 *   - set (destroy): clears the slot's id field only (record +
 *     slotOffset -- the slot's own "extra" field at +2 is
 *     deliberately left untouched, matching the original exactly),
 *     and subtracts equippedItemId's own ItemFieldWeight from
 *     PartyFieldInventory's running total (+0x118 -- still not fully
 *     confirmed what unit that field tracks, see this header's own
 *     PartyFieldInventory note; manipulated the same way
 *     PlaceItemInSlot/PickUpItemFromSlot already do elsewhere).
 *   - clear (replace): writes replacementItemId as the slot's id and
 *     equippedItemId as its own extra field (itemSlotSet's own
 *     argument order -- the *expiring* item's id ends up parked in
 *     "extra," an original quirk reproduced as found, not
 *     reinterpreted), then applies replacementItemId's own stat
 *     bonuses via partyApplyMultiStatEffect.
 * Finishes with partyRefreshCarryCapacityAndAttributeBonuses
 * (UpdatePartyAverageStatTiers, pure UI, is not modeled).
 * g_currentPartyRecord isn't modeled either -- like
 * partyApplyIconBarStatDelta, this reimplementation always takes
 * record as an explicit parameter instead.
 */
void partyHandleIconBarItemExpiry(uint8_t *record, const ItemCatalog *catalog, uint16_t modeFlags,
                                   uint16_t equippedItemId, uint16_t replacementItemId, unsigned slotOffset);

/*
 * TickEquippedItemDurability (yendor2.asm:19242 vs. yendor3.asm:11226,
 * instruction-identical): the equipped-item wear/breakage tracker for
 * one equipment slot (slotOffset: 0x13A/0x142/0x146). Reuses
 * item.h's already-decoded classification/break-target fields
 * (itemClassifyServiceTier, itemTargetEntry/itemTargetWord with
 * ItemTargetBreakChanceA/B and ItemTargetBreakItemA/B) rather than
 * re-deriving them.
 *
 * An empty slot, or an item itemClassifyServiceTier doesn't recognize
 * (not wearable/weapon-like), is left untouched:
 * PartyItemDurabilityUnchanged, no counter incremented. Otherwise the
 * slot's own wear counter (PartyFieldWearMain/Second/Third, at
 * 0xBE/0xC0/0xC2) increments by 1; while it's still at or below the
 * slot's own threshold (0x78/0x50/0x14 -- deliberately different per
 * slot, not reproduced as one shared constant), nothing else happens
 * this call. Once the counter exceeds the threshold, rolls
 * randomInRange(rng, 1000) against the item's own break chance
 * (ItemTargetBreakChanceA for a category-A/C item -- ItemFlagEquipCode0A
 * or 0C set -- ItemTargetBreakChanceB otherwise, the same category
 * split itemCorrosionReplacement already uses). A roll within chance
 * "breaks" the item: the slot is replaced via
 * partyHandleIconBarItemExpiry (matching effect id 0's own definition
 * -- always EffectModeItemReplace, fetched via effectGetDef rather
 * than hardcoding the bit, matching combatApplyCorrosion's own
 * convention) with ItemTargetBreakItemA/B as the replacement, and the
 * *replacement* item's own equip-category flags (not necessarily the
 * same category the broken item had) select which of the 3 wear
 * counters gets reset to 0 -- confirmed by tracing what the original's
 * own `g_currentItemRecord` actually is at that point (a fixed alias
 * for LoadItemCatalogRecord's 0xB50 scratch buffer, last overwritten
 * by partyHandleIconBarItemExpiry's own internal load of the
 * replacement item -- not a separately-tracked pointer, and not
 * modeled as one here since this reimplementation's item lookups
 * don't share that scratch-buffer aliasing). Returns
 * PartyItemDurabilityBroke. A roll outside chance (the item survives)
 * returns PartyItemDurabilityUnchanged, same as the not-yet-due case
 * -- the original's own errorCode conflates both under "1" too.
 *
 * Deliberately simplified from the original by one step: the original
 * re-runs ClassifyItemServiceTier a second time right before the roll
 * (its own EMS-scratch-buffer item record could have been clobbered
 * by intervening calls since the first classification); this
 * reimplementation's item.h lookups aren't scratch-buffer-based, so
 * there's nothing to go stale and the second call would be a pure
 * no-op repeat -- omitted rather than translated literally.
 */
typedef enum {
    PartyItemDurabilityUnchanged,
    PartyItemDurabilityBroke
} PartyItemDurabilityOutcome;

PartyItemDurabilityOutcome partyTickEquippedItemDurability(uint8_t *record, const ItemCatalog *catalog,
                                                             GameKind game, unsigned slotOffset, RandomState *rng);

enum { PartyXpThresholdCount = 89 };

/*
 * 89 x Bcd4 entries, index 0 = the XP needed to advance from level 1 to
 * level 2, ..., index 88 = level 89 to level 90 (matching
 * PartyFieldLevel's own "capped at 90 by training items" ceiling -- the
 * XP curve alone can't reach past it). Entries beyond a game's last real
 * jump (39 in both games) repeat an effectively-unreachable sentinel --
 * 90,000,000 in Chapter 2, 99,999,999 in Chapter 3, a genuine content
 * difference, not just a coincidence of a shared table -- confirmed via
 * yendor2/yendor3's own dump_xp_threshold_table.py IDA scripts, real
 * addresses not guessed.
 */
const uint8_t (*partyXpThresholdTable(GameKind game))[4];

typedef struct {
    uint16_t tier1At; /* PartyFieldClass += 10 (promotes tier 0 -> 1) once PartyFieldLevel reaches this */
    uint16_t tier2At; /* PartyFieldClass += 10 again (tier 1 -> 2) at this level */
} PartyClassPromotionThresholds;

/*
 * Ch2: {10, 30} (`_val25`/`_val26`, yendor2.asm:3489-3490).
 * **Ch3: {0, 0}** -- the equivalent globals (`word_331F8`/`word_331FA`,
 * yendor3.asm) are read but never written anywhere in the disassembly,
 * always 0, so `partyApplyTraining`'s comparison against them can never
 * match a real level (which is always >= 1) -- promotion is effectively
 * disabled in Chapter 3. The third always-zero-global quirk found in
 * this project (see lockcatalog.h's "LoadCurgameRecord" note and
 * interact.h's `curgameIdOffset`), now in a third, unrelated subsystem
 * -- three independent instances make a systemic Chapter 3 change more
 * plausible than three separate coincidences, but that's not confirmed.
 */
const PartyClassPromotionThresholds *partyClassPromotionThresholds(GameKind game);

typedef enum {
    PartyTrainOutcomeInsufficientGold, /* cost > the save's gold; nothing changed */
    PartyTrainOutcomeApplied
} PartyTrainOutcome;

/*
 * UseTrainingItem's bit-0x2 branch (yendor2.asm:21510 on, structurally
 * identical in Chapter 3 apart from the promotion-threshold quirk
 * above) -- the game's actual leveling mechanic; see
 * PartyFieldPendingLevel's own doc comment for why XP alone never
 * raises PartyFieldLevel. `cost` is the training item's own price --
 * caller-supplied, since this project hasn't built the upstream
 * item-use pipeline that would resolve it (UseItem/SelectItemUseRecord,
 * out of scope here) -- the same kind of "not-yet-resolved input taken
 * as a parameter" already used for interact.h's curgame flags.
 *
 * On success: spends `cost` from the save's gold (SaveHeaderGold),
 * PartyFieldLevel += 1 (capped at 90), max HP grows by 30% of max
 * Stamina with current HP set to the new max (a full heal), max MP
 * grows by a class-base-dependent formula (0 for FIGHTER/MERCHANT/
 * ROGUE and their promoted tiers -- see the .c file for the other 6
 * bases' exact weightings, read directly from the branch structure,
 * not guessed), every one of the 6 core attributes and 13 skills grows
 * by a flat +2 (max only), applies partyApplyAbilityUnlocks using the
 * *pre*-promotion class id (matching the original's own order -- the
 * ability walk runs before the promotion check below), and
 * PartyFieldClass gets +10 at each promotion threshold. Every max-stat
 * growth silently no-ops for a stat that's currently 0
 * (AddToStatCapped's own "untrained slot" skip, reproduced exactly).
 * Finally calls partySyncStagedStats then
 * partyRefreshCarryCapacityAndAttributeBonuses, in that order, exactly
 * matching the original's own tail call sequence -- so besides HP/MP,
 * every attribute and skill's *current* value also gets pulled up to
 * its (possibly just-grown) max, and carry capacity/the two excess-stat
 * bonus pairs get recomputed from the final Strength/Dexterity values.
 *
 * The original's tail call chain also reaches
 * RecomputeEquipmentStatBonuses (via RefreshCarryCapacityAndAttributeBonuses's
 * own tail call -- training re-evaluates equipped items' bonuses too,
 * not just base stats). This reimplementation splits that step out as
 * a separate function, partyRecomputeEquipmentStatBonuses (below),
 * that partyApplyTraining does NOT call automatically, since it needs
 * an ItemCatalog this function doesn't take -- a caller wanting full
 * fidelity should call it too after partyApplyTraining.
 */
PartyTrainOutcome partyApplyTraining(uint8_t *record, GameKind game, SaveGame *save, const Bcd4 cost);

/*
 * RefreshCarryCapacityAndAttributeBonuses (yendor2.asm:18962,
 * instruction-identical in Chapter 3): recomputes PartyStatCarryCapacity
 * (current and max) as 10x the matching Strength value, and the four
 * "excess over 72" bonus fields above (PartyFieldStrengthBonus/
 * DexterityBonus and their Max counterparts) as 20% of however far the
 * matching Strength/Dexterity value is past 72 (0 if not past it).
 * Called after any stat change (training, items, equipment) throughout
 * the original -- UseTrainingItem is one caller among several.
 *
 * Only reimplements this function's own body. Its own tail call,
 * RecomputeEquipmentStatBonuses (yendor2.asm:19907), is a separate
 * function here -- partyRecomputeEquipmentStatBonuses, below -- since
 * it needs an ItemCatalog this function doesn't take.
 */
void partyRefreshCarryCapacityAndAttributeBonuses(uint8_t *record);

/*
 * RecomputeEquipmentStatBonuses (yendor2.asm:19907, instruction-identical
 * in Chapter 3): resets PartyStatEquipRating1-5 (current/max) from their
 * baseline fields (PartyFieldEquipRatingBase1/2/3, PartyFieldStrengthBonus,
 * PartyFieldDexterityBonus, and their Max counterparts -- note
 * EquipRating2/3 use the *swapped* base fields, matching the original),
 * then adds each currently-equipped item's own bonus on top:
 *   - Main weapon (equipment code 0xA): EquipRating1 += the character's
 *     own Projectile skill (current/max -- yes, a skill value, not an
 *     item property); EquipRating2 += the weapon's own target-entry
 *     bonus (ItemTargetAbsorption -- for a weapon this is a damage/
 *     accuracy figure, not literally "absorption").
 *   - Second slot (code 0xC, likely off-hand/shield): EquipRating3 +=
 *     a melee skill selected by the item's own ItemTargetSlotFlags bit
 *     (0x4000 -> Slashing, 0x2000 -> Bashing, 0x1000 -> Polearm, none
 *     of the three -> no skill bonus at all); EquipRating4 += the
 *     item's own target-entry bonus. If ItemTargetSlotFlags bit 0x1 is
 *     also set, PartyFieldUiFlags bit 0x20 is set (else cleared) --
 *     meaning not confirmed, a field already used for other UI
 *     purposes per its own doc comment.
 *   - Every other equipped item (codes 0xD-0xF, then 0x10-0x14):
 *     EquipRating5 += each one's own target-entry bonus, accumulated
 *     across all of them.
 * Each item lookup uses itemCatalogRecord/itemTargetEntry; an empty
 * slot (item id 0) or an item with no target entry contributes nothing.
 * Called from several places in the original, not just training --
 * matching that, this project's partyApplyTraining does NOT call it
 * automatically (see that function's own doc comment).
 */
void partyRecomputeEquipmentStatBonuses(uint8_t *record, const ItemCatalog *catalog, GameKind game);

enum {
    PartyAbilityUnlockRowCount = 6,     /* one per class base 4-9 (MONK..MARKSMAN); see partyAbilityUnlocksAtLevel */
    PartyAbilityUnlockColumnCount = 20, /* one per even level, 2..40 */
    PartyAbilityUnlockSlotCount = 2     /* up to 2 ability ids unlocked per level */
};

typedef uint16_t PartyAbilityUnlockRow[PartyAbilityUnlockColumnCount * PartyAbilityUnlockSlotCount];

/*
 * UseTrainingItem's ability/spell-unlock table walk (yendor2.asm:21764
 * on, DS:0xD22B; yendor3.asm, DS:0xB8B5; same shape, different ids --
 * a real per-game content difference like the rest of this project's
 * per-game id spaces). Only ever consulted by the original on an even
 * PartyFieldLevel, hence 20 columns (levels 2, 4, ..., 40) rather than
 * PartyXpThresholdCount's 89 -- the real per-level ability curve simply
 * stops mattering past level 40, same story as the XP curve stopping
 * at level 39.
 *
 * **The table is exactly 6 rows, confirmed architecturally, not just
 * by row-count counting**: `TABLE_OFFSET + 6*0x50` lands exactly on
 * `TravelToDestination`'s own destination-table base address in BOTH
 * games (`0xD40B` in Chapter 2, `0xBA95` in Chapter 3) -- reading a 7th
 * row would silently read that unrelated table's own data instead.
 * The original's own row-index arithmetic can produce exactly that:
 * class base 1-3 (FIGHTER/MERCHANT/ROGUE, at *any* tier -- not just
 * unpromoted) reduces to a negative or out-of-range row, an apparent
 * genuine original-engine bug (or at minimum, an intentional "physical
 * classes have nothing to learn here" case implemented via an
 * unguarded out-of-bounds read rather than an explicit check) --
 * confirmed reachable in normal play, since a level-2+ FIGHTER/
 * MERCHANT/ROGUE at any tier is an entirely ordinary character state.
 * Not reproduced: partyAbilityUnlocksAtLevel returns 0 ability ids for
 * these bases instead of reading adjacent unrelated data.
 */
const PartyAbilityUnlockRow *partyAbilityUnlockTable(GameKind game);

/*
 * The 0-2 ability-flag-bank ids (see PartyFieldFlagBankCA, flagBankSet)
 * a character unlocks at `level`, given their (pre-promotion) class id.
 * Returns 0 ids for: an odd level, a level outside [2,40], or a class
 * base outside 4-9 (see partyAbilityUnlockTable's comment). Stops at
 * the table's own first zero entry per column exactly like the
 * original -- a nonzero id after a zero one (never observed in either
 * game's real table, but not assumed impossible) would not be read.
 */
unsigned partyAbilityUnlocksAtLevel(unsigned classId, unsigned level, GameKind game, uint16_t out[PartyAbilityUnlockSlotCount]);

/* Composes partyAbilityUnlocksAtLevel with flagBankSet on PartyFieldFlagBankCA. Returns the same count. */
unsigned partyApplyAbilityUnlocks(uint8_t *record, unsigned classId, unsigned level, GameKind game);

/*
 * BuildAlchemySpellList's own known-ability scan (yendor2.asm:25154,
 * yendor3.asm:23659, instruction-identical): the highest ability-flag
 * index (`PartyFieldFlagBankCA`) the alchemy/spell-casting screen ever
 * tests -- an `InitGlobals` constant, 125 in Chapter 2
 * (`word_3330C`), 107 in Chapter 3 (confirmed via direct comparison of
 * both `InitGlobals` copies).
 */
unsigned partyKnownAbilityIdMax(GameKind game);

/*
 * The same scan's filtering step: tests flagBankTest(PartyFieldFlagBankCA,
 * ...) for every index 1..partyKnownAbilityIdMax(game) and returns each
 * set one, in ascending order -- the same ids partyApplyAbilityUnlocks
 * sets via partyAbilityUnlockTable. Writes up to outCapacity ids into
 * out; the return value is the true total found, which may exceed
 * outCapacity (matching how a caller would detect a too-small buffer).
 * Not reproduced: `CheckSpellCastability`'s own per-entry "can this be
 * cast right now" gate (a separate, not-yet-traced UI-affordability
 * check) and `BuildAlchemySpellList`'s pagination/current-selection
 * bookkeeping that follows this filtering step -- both belong to the
 * eventual UI layer, not this data-model query.
 */
unsigned partyKnownAbilityIds(const uint8_t *record, GameKind game, unsigned *out, unsigned outCapacity);

/*
 * SyncPartyRecordStagedStats (yendor2.asm:22633; called from UseTrainingItem
 * and UseItemType_400, a different, not-yet-decoded item handler): sets
 * every stat's current value to its max, for every PartyStat except
 * PartyStatHitPoints/PartyStatMagicPoints (both handled by their own
 * explicit full-heal-to-new-max step elsewhere -- partyApplyTraining's own
 * HP/MP growth already does this). In particular this is what makes
 * training also refill an already-partly-depleted attribute or skill
 * (survival, thievery, ...) up to its current max, not just raise the max
 * itself -- a real effect of training beyond stat growth, easy to miss
 * from the growth formulas alone.
 */
void partySyncStagedStats(uint8_t *record);

/*
 * Class ids are tier * 10 + base: base 1-9 (FIGHTER..MARKSMAN), tier 0-2.
 * Valid ids are 1-9, 11-19 and 21-29; the game's own table lookup gives
 * garbage for 0, 10 and 20 (GetClassNameString, yendor2.asm:16564).
 */
bool partyClassIsValid(unsigned classId);
unsigned partyClassBase(unsigned classId);
unsigned partyClassTier(unsigned classId);
const char *partyClassName(unsigned classId); /* NULL if invalid */

/* Status bit recording that a character reached secondary class 4-9 (tier 0 only); 0 otherwise. */
uint16_t partyClassSecondaryBit(unsigned classId);

/*
 * Flag banks: index 1..(words*16), 1-based and most-significant-bit first
 * (GetGlobalFlagBitAndWord, yendor2.asm:42355). Index 0 and out-of-range
 * indices test false and are ignored on set/clear.
 */
bool flagBankTest(const uint8_t *bank, unsigned words, unsigned index);
void flagBankSet(uint8_t *bank, unsigned words, unsigned index);
void flagBankClear(uint8_t *bank, unsigned words, unsigned index);

bool partyTestAbilityFlag(const uint8_t *record, unsigned index);
void partySetAbilityFlag(uint8_t *record, unsigned index);
bool partyTestEventFlag(const uint8_t *record, unsigned index);
void partySetEventFlag(uint8_t *record, unsigned index);

/*
 * An inventory group is 34 bytes: a u16 total carried weight followed by 8
 * item slots of 4 bytes (u16 item id, u16 extra word; id 0 = empty). The
 * same layout is used by a character's main inventory, an open bag, and
 * every 34-byte item-instance record in the save file (section 3).
 */
enum {
    InventoryGroupSize = 34,
    InventorySlotCount = 8,
    ItemSlotSize = 4
};

typedef enum {
    PartyGroupMain,
    PartyGroupBag1,
    PartyGroupBag2,
    PartyGroupBag3,
    PartyGroupCount
} PartyGroup;

uint8_t *partyInventoryGroup(uint8_t *record, PartyGroup group);

/*
 * The group the inventory screen acts on: the first open bag in priority
 * order (bag 1, 2, 3), otherwise the main inventory (GetInventorySlotPtr).
 */
uint8_t *partyActiveInventoryGroup(uint8_t *record);

uint16_t inventoryGroupWeight(const uint8_t *group);
void inventoryGroupSetWeight(uint8_t *group, uint16_t weight);

/* Slot 1-8, or NULL. */
uint8_t *inventoryGroupSlot(uint8_t *group, unsigned slot);

uint16_t itemSlotId(const uint8_t *slot);
uint16_t itemSlotExtra(const uint8_t *slot);
void itemSlotSet(uint8_t *slot, uint16_t id, uint16_t extra);

/*
 * Equipment slots use the game's command codes 0xA-0x14. 0xA-0xF are 4-byte
 * slots (id + extra) at 0x13A, 0x13E, ... 0x14E; 0x10-0x14 are 2-byte,
 * id-only slots at 0x152, 0x154, ... 0x15A. NULL for other codes.
 *
 * Chapter 3's lookup has no case for code 0x12, so it returns the same
 * slot as 0x14 (0x15A) and slot 0x156 can't be reached through it.
 */
enum {
    PartyEquipmentFirst = 0x0A,
    PartyEquipmentFirstShort = 0x10,
    PartyEquipmentLast = 0x14
};
uint8_t *partyEquipmentSlot(uint8_t *record, unsigned code, GameKind game);

/*
 * An open bag: [+0] container item id (0 = closed), [+2] the record number
 * of that container's contents in the save's item-instance table, [+4] the
 * 34-byte contents (an InventoryGroup) held while the bag is open. Bag 0-2.
 */
enum { PartyBagMarkerSize = 0x26 };
uint8_t *partyBagMarker(uint8_t *record, unsigned bag);

/*
 * ApplySavingThrowEffect's packed value (yendor2.asm:44646,
 * instruction-identical in Chapter 3), decoded -- this closes out a
 * long-open question: it's the exact 4-byte record LoadCurgameRecord
 * (lockcatalog.h) reads via EMS paging, confirmed live against
 * yendor2.idb (2026-09-26) to be nothing more exotic than
 * `g_lockStatusFlags` (the record's first word) plus this packed
 * second word -- the *same two globals* `LoadLockState` populates for
 * an ordinary lock. That's why `interact.h`'s `curgameFlags` and this
 * packed value slot cleanly into the existing lock-status machinery:
 * a CURGAME "trigger" record and a lock record are the same physical
 * shape, just read through two different loaders into the same
 * scratch globals.
 *
 * `UseAbilityCommand` (`yendor2.asm:12821`) and `HandleSearchCommand`
 * both feed this packed value straight to `ApplySavingThrowEffect`
 * after loading either kind of record: it's a search/lockpicking-
 * triggered magical trap. `packedValue / 100` is a saving-throw DC
 * (the same DC used twice -- see below); `packedValue % 100` is an
 * `effect.h` effect id, `< 50` targeting the triggering character
 * alone or `>= 50` (subtract 50 for the real id) targeting every
 * occupied, non-incapacitated party member instead. `packedValue == 0`
 * means no trap at all.
 *
 * The original's own two-tier roll structure -- composed in full as
 * `combatApplySavingThrowTrap` (`combat.h`, kept there rather than
 * here since this header can't include `combat.h`): one *trigger*
 * roll first, using the character attempting the lock/search
 * (`combatFailsSavingThrow`, `defenderStat` = their own
 * `PartyFieldLevel`, `threshold` = this decode's own `threshold`,
 * `bonus` = their own `PartyStatThievery` -- their lockpicking/search
 * skill helping them avoid triggering it at all); a failed trigger
 * roll (the common case name is misleading again -- *failing* means
 * the trap *does* go off) then runs effect.h's own
 * magnitude/resistance/cost pipeline
 * (`effectRollMagnitude`/`effectResolveInflictedStatus` via another,
 * independent `combatFailsSavingThrow` call per recipient using the
 * *same* `threshold`/`effectResistanceBonus`/`combatApplyEffect`) once
 * per recipient -- one call for the single-target case, once per
 * eligible party member for the whole-party case, matching
 * `ApplyEffectAndDrawIconBar`'s own per-slot processing exactly.
 */
typedef struct {
    unsigned threshold;
    unsigned effectId;
    bool wholeParty;
} PartySavingThrowEffect;

/* False (out left untouched) if packedValue == 0 -- no trap configured at all. */
bool partyDecodeSavingThrowEffect(uint16_t packedValue, PartySavingThrowEffect *out);

/*
 * ResetDailyAbilityCharges' own per-record step (yendor2.asm:45069,
 * yendor3.asm:45484, instruction-identical): zeroes all 4
 * PartyFieldAbilityCharge entries. Special abilities recharge once per
 * in-game day -- see gameclock.h's gameClockAdvance, whose return
 * value tells the caller when to call this for every party member.
 */
void partyResetDailyAbilityCharges(uint8_t *record);

/*
 * ApplyRestEffectsToCharacter (yendor2.asm:25926, yendor3.asm:24437,
 * instruction-identical). The "R rest" command's own per-character
 * tick: skips incapacitated characters entirely. Otherwise, if any of
 * Sick/Poisoned/Diseased/Hexed/Jinxed/Cursed is set, this is a
 * degrade tick instead of a regen one: Sick and Jinxed are silently
 * cured (cleared, no cost); Diseased drains 36 HP via partyDeductHp
 * (which itself sets PartyStatusDead at 0, matching the original's
 * own inline clamp+flag logic exactly) -- if that kills the character,
 * the Cursed check below is skipped entirely, matching the original's
 * own early return; Cursed (if the character is still alive) drains
 * 48 MP via partyDeductMp. Poisoned/Hexed alone (with none of the
 * other 4 bits also set) fall into this branch but have no explicit
 * case of their own here -- they simply block normal regen for the
 * tick, with no separate degrade of their own (their own periodic
 * damage, if any, comes from elsewhere -- the ailment-tick system).
 *
 * With none of those 6 bits set: normal regen, `round(max * regenPercent
 * / 100)` added to current HP (always) and MP (only if the character
 * has a nonzero max MP at all), each clamped at its own max.
 *
 * `regenPercent` is the original's own `word_328C2`
 * (`RestPartyAndAdvanceClock`, `yendor2.asm:25855`-`25870`), now fully
 * composed as `partyDeriveRestRegenPercent` (below): `activeCount` =
 * non-incapacitated `SaveHeaderPartySlots` members; then, up to
 * `activeCount` times, `itemRangeAvailable(0x36, 0x40, ...)` (a
 * confirmed item range -- real `WORLD.DAT` data in both games: MEAT,
 * BREAD, FOOD, CHEESE, ALE, plain FOOD -- literal camping provisions,
 * not a guess) and `partyConsumeItemCharge` on a hit, breaking early
 * the first time nothing more is found;
 * `regenPercent = (100 / activeCount) * consumedCount`. Still missing
 * from that composition: `IsItemRangeAvailable`'s own container
 * recursion and `ConsumeItemChargeResource`'s 3 special modes (see
 * `partyDeriveRestRegenPercent`'s own doc comment). `partyApplyRestEffects`
 * itself still takes `regenPercent` as an explicit parameter rather
 * than calling that composition internally, keeping this function pure
 * and independently testable.
 */
typedef struct {
    bool wasSkipped; /* incapacitated -- no change made at all */
    bool died;       /* the Diseased HP drain reached 0 and set PartyStatusDead this call */
} PartyRestOutcome;

PartyRestOutcome partyApplyRestEffects(uint8_t *record, uint16_t regenPercent);

/*
 * FindItemInInventoryRange (yendor2.asm:22854, instruction-identical in
 * Chapter 3): searches one party member's 8 main inventory slots
 * (PartyFieldInventory + 2 + slot*ItemSlotSize, the same layout
 * inventoryGroupSlot already uses) for an item id within
 * [lowId, highId] inclusive. Returns the id found (0 if none) and, via
 * outSlotOffset, the matching slot's own record-relative byte offset.
 *
 * NOT reimplemented: recursing into a container-type item's own
 * contents when a slot's own id doesn't match directly (ItemFlag bit
 * 0x2000) -- the original reads a CURGAME-backed "ground item
 * container" record for this (FindItemInsideContainer,
 * yendor2.asm:22911), a genuinely separate, not-yet-built subsystem
 * (see file-formats.md's "world ailments" section for where this
 * surfaced). Also omitted: the original's own extra check of equipment
 * slot 0xB (record +0x13E) -- that slot is *only* ever consulted as a
 * possible container, never range-matched directly, so without
 * container support it can never contribute a match and is safely left
 * out rather than partially modeled.
 */
uint16_t partyFindItemInRange(const uint8_t *record, uint16_t lowId, uint16_t highId, unsigned *outSlotOffset);

/*
 * IsItemRangeAvailable (yendor2.asm:22764, instruction-identical in
 * Chapter 3; "CORRECTED from 'CheckTransportAvailability' -- too
 * specific a guess" per an earlier session's own comment) -- a generic
 * "does the party have an item in this id range" check, used both for
 * a boat/horse-style transport gate and, via `CheckQuestItemsCompleted`
 * (not reimplemented), a quest-item-completion check.
 *
 * A no-op (`.found == false`) if lowId is 0 or lowId > highId, matching
 * the original's own two early-exit guards. Otherwise: checks
 * globalSlots (the original's own fixed 6-entry table at `DS:0x9519`,
 * the "resource panel" -- see file-formats.md's "world ailments"
 * section) for a direct id match first; if none, scans each occupied
 * `SaveHeaderPartySlots` member's own inventory via
 * partyFindItemInRange, stopping dead at the first *unoccupied* slot
 * instead of skipping past it -- the same quirk already found in
 * `combatApplySavingThrowTrap`/`combatApplyEncodedItemEffectParty`'s
 * own whole-party loops.
 *
 * NOT reimplemented: container recursion (see partyFindItemInRange's
 * own doc comment) and the original's own `SyncAllContainers` call
 * (flushes any open container UI state back to its CURGAME record --
 * moot without container support).
 */
typedef struct {
    bool found;
    uint16_t itemId;
    bool inGlobalTable;      /* true: globalSlots; false: a party member's own inventory */
    unsigned slotOffset;     /* inGlobalTable: byte offset into globalSlots (0, 4, ..., 20); else: byte offset into the owning party record */
    uint16_t partyRecordId;  /* only meaningful when !inGlobalTable */
} ItemRangeAvailability;

/* globalSlots: 24 bytes, 6 x 4-byte item slots (itemSlotId/itemSlotExtra shape), matching DS:0x9519. */
ItemRangeAvailability itemRangeAvailable(const uint8_t *globalSlots, SaveGame *save, uint16_t lowId, uint16_t highId);

/*
 * ConsumeItemChargeResource (yendor2.asm:41546, instruction-identical in
 * Chapter 3) -- a shared, ~21-call-site "spend one use of an item-based
 * resource" engine with 4 consumption modes selected by a caller-context
 * flag (`g_uiScratchFlags3` bits `0x8000`/`0x4000`/`0x2000`). Only the
 * *default* mode (none of those 3 bits set) is reimplemented here --
 * confirmed to be what every caller except `RepairItemCommand`
 * (`yendor2.asm:51046`) gets: that's the only site in either game's
 * disassembly that ever sets any of the 3 bits, and it always clears
 * them again immediately after its own single `ConsumeItemChargeResource`
 * call -- no other caller (including `RestPartyAndAdvanceClock`,
 * which drives this function's own first real use, the "R rest"
 * regen-rate food consumption) ever touches them.
 *
 * Also only the party-inventory case is covered (matching
 * `itemRangeAvailable`'s own `!inGlobalTable` output) -- `slot` must be
 * a 4-byte item slot inside `partyRecord`'s own main inventory group.
 * The 6-entry global-table case (`itemRangeAvailable`'s
 * `.inGlobalTable == true`) is not reimplemented, nor is container
 * recursion or the 3 special modes' own equipment-slot write-back
 * (`partyHandleIconBarItemExpiry` already covers conceptually similar
 * ground for a different caller).
 *
 * In this mode: if the item's own target-entry flags
 * (`itemTargetWord(entry, 1)` bit `0x1` -- "has multiple uses," not
 * otherwise named; real food items in both games all have it clear)
 * are set, decrements the slot's own `itemSlotExtra`; if that stays
 * above 0, nothing else happens -- the item remains, with one fewer
 * use. Otherwise (the bit is clear, or the decrement reached 0): the
 * whole slot is cleared (`itemSlotSet(slot, 0, 0)`) and the item's own
 * `ItemFieldWeight` is subtracted from the *main* inventory group's own
 * weight total -- unconditionally the main group, matching the
 * original's own hardcoded offset (`[partyRecord+0x118]`) regardless of
 * which group `slot` actually belongs to; not clamped at 0, matching
 * the original's own plain `sub` (this project's standing practice of
 * reproducing confirmed original behavior rather than silently
 * correcting it).
 */
void partyConsumeItemCharge(uint8_t *partyRecord, const ItemCatalog *catalog, uint8_t *slot);

/*
 * RestPartyAndAdvanceClock's own regen-rate derivation
 * (yendor2.asm:25832-25870, instruction-identical in Chapter 3), now
 * fully composed: counts active (non-incapacitated) `SaveHeaderPartySlots`
 * members -- stopping dead at the first *unoccupied* slot, the same
 * whole-party quirk `itemRangeAvailable`'s own fallback scan has --
 * then attempts up to that many camping-supply consumptions (item id
 * range `0x36`-`0x40`, confirmed real food items -- MEAT, BREAD, FOOD,
 * CHEESE, ALE -- in both games' real `WORLD.DAT` data), stopping at the
 * first miss. `regenPercent = (100 / activeCount) * consumedCount` --
 * the *original's own* truncation order, not `(100 * consumedCount) /
 * activeCount`: with 3 active members and all 3 fed, that's `(100/3)*3
 * = 33*3 = 99`, not 100, reproduced exactly rather than "fixed."
 *
 * A match in the 6-entry global table (`itemRangeAvailable`'s own
 * `.inGlobalTable == true`) is treated the same as "nothing found" --
 * `partyConsumeItemCharge` only covers the party-inventory case (see
 * its own doc comment), so this stops the loop early rather than
 * silently miscounting. A conservative under-approximation in the rare
 * case food sits in the resource panel instead of a party member's own
 * inventory, not a wrong answer.
 *
 * Returns 0 if there are no active members at all -- the original's
 * own `div` by that count would fault; not reproduced as a crash since
 * this shouldn't be reachable with real save data.
 */
uint16_t partyDeriveRestRegenPercent(const uint8_t *globalSlots, SaveGame *save, const ItemCatalog *catalog);

#endif
