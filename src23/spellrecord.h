#ifndef YENDOR23_SPELLRECORD_H
#define YENDOR23_SPELLRECORD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * The spell/ability catalog LoadClueBookSpellEntry (yendor2.asm:23355,
 * instruction-identical in Chapter 3) loads by 1-based index: 80-byte
 * records, immediately following the monster catalog's type-lookup table in
 * WORLD.DAT (monster.h's MonsterCatalogLayout -- block5+block6 end exactly
 * where this catalog starts in both games, confirmed by address arithmetic).
 *
 * This is the table behind the long-open "word_332D8-33306 encoded effect
 * descriptor cluster" mystery (file-formats.md): those globals are simply
 * named offsets into whichever record was most recently loaded by index,
 * addressed via real-mode segment arithmetic (es:0x5A5A, es = seg129) that
 * nothing in the disassembly visually connects to the record's own absolute
 * addresses. The record count for each game is a confirmed, named engine
 * constant (word_3330C in Chapter 2's InitGlobals, its Chapter 3 equivalent
 * at ds:0x5DF8) -- the same value party.h's partyKnownAbilityIdMax already
 * used without the connection to this table being made.
 *
 * Field offsets and the handful of bit meanings below were confirmed two
 * ways: tracing every reader (ShowClueBookSpellDetail's clue-book UI,
 * ApplyEncodedItemEffect's icon-bar branches, and the ApplyAttackToTarget/
 * TryResolveAttackAgainstTarget/ApplyTargetResistancesToAttack
 * attack-resolution family combat.h already covers) back to a record-
 * relative offset, and cross-checking against real byte statistics from
 * both games' actual WORLD.DAT files -- every bit this header names is set
 * by at least one real record in both games except SpellResistClearAware
 * (0x20 of SpellFieldResistFlags), which no real record in either game ever
 * sets; the original still reads it, so combat.h reproduces it anyway for
 * fidelity rather than dropping it as unreachable.
 */

enum {
    SpellRecordSize = 80,
    SpellRecordCountYendor2 = 125, /* word_3330C, InitGlobals */
    SpellRecordCountYendor3 = 107, /* ds:0x5DF8, InitGlobals */
    SpellRecordCountMax = 125,
    SpellNameFieldSize = 22 /* space/NUL-terminated, not guaranteed NUL-terminated on disk */
};

typedef struct {
    uint32_t recordsOffset; /* WORLD.DAT offset */
    uint16_t recordCount;
} SpellCatalogLayout;

const SpellCatalogLayout *spellCatalogLayout(GameKind game);

typedef struct {
    GameKind game;
    uint16_t recordCount;
    uint8_t records[SpellRecordCountMax * SpellRecordSize];
} SpellCatalog;

bool spellCatalogParse(SpellCatalog *catalog, GameKind game, const uint8_t *region, size_t size);
bool spellCatalogParseWorldDat(SpellCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size);

/* 1-based id, matching the original's own indexing; NULL if out of range. */
const uint8_t *spellRecord(const SpellCatalog *catalog, unsigned id);

uint16_t spellGetU16(const uint8_t *record, unsigned offset);

/* Trimmed name (space/NUL run at the end stripped); out must hold SpellNameFieldSize + 1 bytes. */
void spellGetName(const uint8_t *record, char *out);

typedef enum {
    SpellFieldName = 0x00, /* SpellNameFieldSize bytes */

    /*
     * word_332D0: drawn as a labeled number in ShowClueBookSpellDetail, but
     * only for a class already confirmed eligible-and-leveled, gated on a
     * SpellFieldClassEligibility bit -- not identified beyond that; climbs
     * slowly and roughly monotonically with record id in both games (1, 1,
     * 1, 2, 2, 3, 4, 4, 4, 4, 4, 5, ... for Chapter 2's first 12 records),
     * suggesting a tier/grouping id rather than a per-class value, but not
     * confirmed.
     */
    SpellFieldGroup = 0x16,

    SpellFieldMpCost = 0x18,   /* word_332D2; compared against the caster's own PartyFieldMp */
    SpellFieldNuoreCost = 0x1A, /* word_332D4; checked via IsBCDCounterAtLeast against SaveHeaderOreCounter2 */
    SpellFieldOreCost = 0x1C,  /* word_332D6; checked the same way against SaveHeaderOreCounter1 */

    /*
     * word_332D8: 0 in the large majority of real records (119/125 Chapter
     * 2, 99/107 Chapter 3); where nonzero (9 or 13 in every real record
     * found), matched directly against monster.h's MonsterFieldUnknown4E by
     * combatApplySpellAttack, but only when SpellResistTypeRestricted
     * (SpellFieldResistFlags bit 0x100) is set -- an attack that doesn't
     * match the target's own value is a complete no-op, no roll attempted
     * at all. Also drives two of ShowClueBookSpellDetail's own "WHEN:" text
     * choices (compared against literal 9 and 13).
     */
    SpellFieldTargetTypeId = 0x1E,

    /*
     * word_332DC/word_332DE: an already-resolved inflicted-status/magnitude
     * pair, used verbatim (never rolled) by ApplyEncodedItemEffect's
     * single-target and whole-party icon-bar branches -- see combat.h's
     * combatResolveEncodedItemEffectValue.
     */
    SpellFieldInflictedStatus = 0x22,
    SpellFieldInflictedMagnitude = 0x24,

    /*
     * word_332E0: the stat-drain amount ApplyTargetResistancesToAttack
     * subtracts (floored at 0) from one of the target's own combat-stat
     * fields when SpellFieldAttackFlags requests a drain category -- see
     * combat.h's combatApplyTargetResistances.
     */
    SpellFieldDrainAmount = 0x26,

    /*
     * word_332E2: ResetOrCopyTargetPositionFields's own bit-0x40 gate
     * (yendor2.asm:53238) -- whether a cursed acting character zeroes a
     * target's position fields instead of copying word_332DC/word_332DE
     * (note: a DIFFERENT pair of globals at the same numeric distance as
     * SpellFieldInflictedStatus/Magnitude's offsets but a separate call
     * path -- not cross-checked against those two fields' own values).
     */
    SpellFieldPositionResetFlags = 0x28,

    /*
     * 0x2A/0x2C (word_332E4/word_332E6): read as effect ids (fed to
     * PrepareTrapEffectSlots) by ApplyEncodedItemEffect's loc_2CF51
     * diversion (file-formats.md's own section on it) -- a real,
     * confirmed consumer, but that branch's overall shape isn't
     * understood well enough yet to give these two offsets their own
     * named SpellField constants without guessing at their exact role.
     */

    /*
     * word_332E8: ResolveAttack's "power" input (combat.h's
     * combatResolveAttack) for a normal roll, and the direct-hit fallback
     * damage (ApplyAttackToTarget's "already resolved" path, bypassing the
     * roll entirely) when SpellFieldResistFlags bit 0x100 either isn't set
     * or matches the target's SpellFieldTargetTypeId gate.
     */
    SpellFieldAttackMagnitude = 0x2E,

    /* 0x30-0x33: referenced by nothing traced so far. */

    /*
     * word_332EE: written verbatim into MonsterFieldTickAmount (monster.h)
     * by combatApplySpellAttack whenever ANY inflicted status survives
     * resistances, regardless of SpellFlagsAPersistAffliction -- that bit
     * only gates the separate MonsterFieldImmunities "mark afflicted" OR,
     * not this write. TickMonsterTimer's own gate bits (MonsterFieldState
     * mask 0xFC10) aren't set by this same write, though -- what arms the
     * actual countdown mechanism this value feeds is still a separate,
     * unfound write (monster.h's own long-open note on monsterTickTimer).
     */
    SpellFieldTickAmount = 0x34,

    /* 0x36-0x3F: referenced by nothing traced so far (10 bytes -- plausibly
     * a description-block id for ShowClueBookSpellDetail's own "EFFECT:"/
     * "WHEN:" text via LookupSpellDescriptionBlockOffset, not confirmed). */

    /*
     * word_332FA: a byte offset into `g_currentPartyRecord` where
     * ApplyEncodedItemEffect's `SpellFlagsBLocationBookmark` branch
     * reads/writes a small position bookmark (combat.h's
     * `combatSaveLocationBookmark`/`combatRestoreLocationBookmark`).
     * Exactly one real record in each game has this branch's own bit set
     * -- "MARK OR RETURN" (id 29 both games) -- and its own value here,
     * `0xF0`, lands in the one real gap in `party.h`'s own field map
     * (`PartyFieldFlagBankCA`'s own 32 bytes end at `0xEA`,
     * `PartyFieldFlagBank10C` starts at `0x10C`) -- confirming this
     * isn't collision-prone reserved padding being reused, not a guess.
     */
    SpellFieldBookmarkOffset = 0x40,

    SpellFieldTickCountdown = 0x42, /* word_332FC; -> MonsterFieldTickCountdown, same gate as SpellFieldTickAmount above */

    /*
     * word_332FE: a 6-bit class-eligibility mask (ShowClueBookSpellDetail's
     * per-class eligibility/level row, tested highest-bit-first against a
     * 6-iteration class loop -- which of the 6 class bases each bit maps to
     * isn't individually confirmed here, only that it's a 6-slot mask).
     */
    SpellFieldClassEligibility = 0x44,

    SpellFieldFlagsA = 0x46,     /* word_33300; see SpellFlagsA */
    SpellFieldFlagsB = 0x48,     /* word_33302; see SpellFlagsB -- ApplyEncodedItemEffect's own dispatch key */
    SpellFieldAttackFlags = 0x4A, /* word_33304; combat.h's combatApplyTargetResistances attackFlags parameter */
    SpellFieldResistFlags = 0x4C  /* word_33306; combatApplyTargetResistances resistanceFlags parameter, plus 3 more bits combatApplySpellAttack itself reads -- see SpellResistFlag */
} SpellField;

/*
 * SpellFieldFlagsA (word_33300) bits actually tested anywhere traced.
 * Real-data counts, Chapter 2 / Chapter 3 out of 125 / 107 records:
 * 0x200 (3/2), 0x400 (42/41), 0x800 (4/4), 0x1000 (19/19), 0x4000 (13/13),
 * 0x8000 (16/16).
 */
typedef enum {
    /*
     * Gates whether a surviving inflicted status also gets OR'd into the
     * target's own MonsterFieldImmunities (marking it "currently afflicted",
     * monster.h's own note on that field) by combatApplySpellAttack --
     * MonsterFieldTickAmount/TickCountdown get armed either way whenever any
     * status survives, this bit only controls the affliction-tracking mark.
     */
    SpellFlagsAPersistAffliction = 0x0200,
    /*
     * Bit 0x400 selects one of ShowClueBookSpellDetail's "EFFECT:" message
     * variants (yendor2.asm:6380).
     */
    SpellFlagsAEffectTextVariant = 0x0400,
    /*
     * Read directly inside ApplyEncodedItemEffect's own single-target
     * (word_33302 bit 0x8000, yendor2.asm:51241) and whole-party (bit
     * 0x4000, yendor2.asm:51292) branches -- confirmed by direct read this
     * round, not inferred: in the single-target branch, if NEITHER this bit
     * nor SpellFlagsAIconBarPresetAmount is set, the branch does nothing at
     * all (no icon-bar call, no effect) -- combatApplyEncodedItemEffectSingle
     * should only be called by a future composing dispatcher when
     * `(flagsA & (SpellFlagsAPositionReset|SpellFlagsAIconBarPresetAmount))
     * != 0`. In the whole-party branch this same bit instead only gates
     * whether `ResetOrCopyTargetPositionFields` runs for each individual
     * recipient (yendor2.asm:51292-51298) -- the icon-bar call itself
     * (combatApplyEncodedItemEffectParty's own role) always happens once
     * after the loop regardless, matching the existing, already-correct
     * implementation; the per-recipient gate controls only whether that
     * recipient's `+0xE`/`+0x10` (inflicted-status/magnitude) icon-slot
     * fields get overwritten from SpellFieldInflictedStatus/Magnitude or
     * left stale -- a "genuine loose end" this reimplementation doesn't
     * model (no persistent icon-slot memory exists to leave stale).
     */
    SpellFlagsAPositionReset = 0x0800,
    /*
     * When set (word_33302 bit 0x1000, yendor2.asm:51224/51284), the icon
     * slot's own `+0x12` word is pre-set directly from
     * SpellFieldDrainAmount instead of running SpellFlagsAPositionReset's
     * usual logic -- in the single-target branch this ALSO still falls
     * through into running SpellFlagsAPositionReset's own copy afterward
     * (yendor2.asm:51252's unconditional jmp lands past that bit's own
     * check), so `+0xE`/`+0x10` get the usual preset pair while `+0x12`
     * additionally gets this field -- together, for a gold/ore-cost
     * effect, SpellFieldInflictedMagnitude/DrainAmount would form the same
     * two-word packed-BCD amount `ApplyEffectCost`'s material-spend branch
     * reads elsewhere (file-formats.md's "staged combat event" section).
     * Not reimplemented: constructing that Bcd4 here would need
     * word_332DA's own source traced first (the effect id
     * `PrepareTrapEffectSlots` resolves at the top of both branches, still
     * an untraced caller-context global, distinct from anything in this
     * record).
     */
    SpellFlagsAIconBarPresetAmount = 0x1000,
    /* ApplyEncodedItemEffect's own whole-party/single-target branches -- combat.h's combatApplyEncodedItemEffectParty/Single. */
    SpellFlagsAWholeParty = 0x4000,
    SpellFlagsASingleTarget = 0x8000
} SpellFlagsA;

/*
 * SpellFieldFlagsB (word_33302): ApplyEncodedItemEffect's own ~19-branch
 * dispatch key (roadmap.md candidate 8), now confirmed to be this record's
 * own field rather than per-call caller state. Only the bits this project
 * has traced a concrete consumer for are named; the rest of the ~19 are
 * still open (candidate 8's own remaining scope).
 */
typedef enum {
    SpellFlagsBWholeParty = 0x4000,  /* alias of SpellFlagsAWholeParty's role, same bit position as word_33300's -- different word, same name pattern kept distinct on purpose */
    SpellFlagsBSingleTarget = 0x8000,
    /*
     * Routes into the ApplyAttackToTarget family (this header's own
     * SpellField attack fields) against g_activeCombatMonster, a single
     * already-engaged combat slot -- rather than the icon-bar
     * status-effect path. Confirmed against real data: every Chapter
     * 2/3 record with this bit set is a plain damage/status attack
     * spell (MAGIC ATTACK, COLD SLASH, FEET OF LEAD, INSECT REPELLENT,
     * ELECTRIC BURST, ...). Not the same mechanism as
     * ApplyEncodedItemEffect's own word_33302 bit 0x200 -- a different
     * dispatch bit entirely, reaching the straight-line map-monster
     * attack (combat.h's combatApplyDamageToMapMonster) instead of this
     * combat-slot one.
     */
    SpellFlagsBAttackPath = 0x2000,
    /*
     * Routes to ApplyEncodedItemEffect's own "MARK OR RETURN" branch
     * (yendor2.asm:51685, combat.h's combatSaveLocationBookmark/
     * RestoreLocationBookmark) -- exactly one real record in each game
     * sets this bit, named literally "MARK OR RETURN" in both games'
     * real `WORLD.DAT` data.
     */
    SpellFlagsBLocationBookmark = 0x0020
} SpellFlagsB;

/*
 * SpellFieldResistFlags (word_33306) bits beyond the
 * combatApplyTargetResistances resistanceFlags role (MonsterResistMagicMask/
 * PhysicalMask, 0x200-0x8000) that combatApplySpellAttack itself reads
 * directly.
 */
typedef enum {
    /*
     * combatApplySpellAttack: "already resolved" direct-hit path uses half
     * of the TARGET's own MonsterFieldDamage as the fallback damage instead
     * of word_332E8, when set. Exactly 1 real record in each game.
     */
    SpellResistHalfTargetDamage = 0x0010,
    /*
     * Clears the target's own MonsterStateAware bit after the attack
     * resolves (ApplyAttackToTarget's tail) -- reproduced for fidelity, but
     * genuinely unreachable: no real record in either game sets this bit.
     */
    SpellResistClearAware = 0x0020,
    /* Gates SpellFieldTargetTypeId's match requirement (see that field's own doc comment). */
    SpellResistTypeRestricted = 0x0100
} SpellResistFlag;

#endif
