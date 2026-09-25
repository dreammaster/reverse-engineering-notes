#ifndef YENDOR23_EFFECT_H
#define YENDOR23_EFFECT_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "random.h"

/*
 * Trap and status effect definitions: the 12-byte records at g_trapEffectDefs
 * (yendor2.asm:84959, indexed by effect id and resolved by
 * PrepareTrapEffectSlots, 13722). An effect is what a trap, a monster's
 * attack, an ailment tick, a healing item or an expiring item applies to a
 * party member: it costs HP, MP, gold or ore, and may inflict status
 * conditions after a saving throw. The table is static data inside the
 * executable, so it is embedded here (extracted with
 * ida_scripts/dump_trap_effects.py). Chapter 2 defines 45 effects, Chapter 3
 * 49; ids past the table aren't valid.
 *
 * Definitions were decoded from ApplyEffectAndDrawIconBar, ApplyEffectCost,
 * RollEffectMagnitude, RollEffectResistance and ApplyIconBarStatDelta, and
 * cross-checked against the monster catalogs: every monster's primary attack
 * effect (+0x6C) costs HP and its special effect (+0x6E) inflicts a status.
 */

enum {
    EffectDefSize = 12,
    EffectCountYendor2 = 45,
    EffectCountYendor3 = 49,
    EffectCountMax = EffectCountYendor3
};

typedef struct {
    uint16_t sound;        /* +0 sound event id, 0 = silent */
    uint16_t icon;         /* +2 picture id in category 0x70 (0x80 for the item-expiry effects) */
    uint16_t magnitudeMin; /* +4 */
    uint16_t magnitudeMax; /* +6 */
    uint16_t costFlags;    /* +8, EffectCost bits plus the inflicted-status bits */
    uint16_t modeFlags;    /* +0xA, EffectMode bits */
} EffectDef;

/*
 * costFlags low bits: what the effect spends. The first match in the order
 * HP, MP, HP+MP, then gold, ore 1, ore 2 wins (ApplyEffectCost).
 */
typedef enum {
    EffectCostGold = 0x0001,     /* the party gold counter */
    EffectCostOre2 = 0x0002,     /* the second ore counter */
    EffectCostOre1 = 0x0004,     /* the first ore counter */
    EffectCostMp = 0x0008,
    EffectCostHp = 0x0010,
    EffectCostHpAndMp = 0x0020,
    /*
     * The high bits (0xFF80) are the status conditions inflicted when the
     * saving throw fails: exactly the party status bits Cursed (0x80)
     * through Sick (0x8000), see party.h.
     */
    EffectInflictMask = 0xFF80
} EffectCost;

typedef enum {
    EffectModeStatFloor = 0x0080,     /* subtract from a stat, floored at 0 (ApplyIconBarStatDelta) */
    EffectModeStatCapped = 0x0100,    /* add to a stat, capped at its maximum */
    EffectModeItemDestroy = 0x0200,   /* an equipped item expires and is destroyed */
    EffectModeItemReplace = 0x0400,   /* an equipped item expires and is replaced */
    EffectModeRollResistance = 0x1000, /* the party member's protections get a saving throw */
    EffectModeMagnitudeScaled = 0x2000, /* magnitudeMin * level, no random roll */
    EffectModeMagnitudeFixed = 0x4000   /* magnitudeMin as is */
} EffectMode;

typedef enum {
    EffectSpendNone,
    EffectSpendHp,
    EffectSpendMp,
    EffectSpendHpAndMp,
    EffectSpendGold,
    EffectSpendOre1,
    EffectSpendOre2
} EffectSpend;

/* How many effects the game defines. */
unsigned effectCount(GameKind game);

/* Copies effect id's definition; false if the id is past the table. */
bool effectGetDef(GameKind game, unsigned id, EffectDef *out);

/* Reads a definition from 12 raw little-endian bytes. */
void effectParseDef(const uint8_t raw[EffectDefSize], EffectDef *out);

/* What the effect spends, applying ApplyEffectCost's precedence. */
EffectSpend effectSpend(const EffectDef *def);

/* Status conditions (party.h PartyStatus bits) inflicted if the saving throw fails. */
uint16_t effectInflictedStatus(const EffectDef *def);

/*
 * Whether the game rolls a magnitude for the effect: not for the gold and ore
 * effects, whose amounts are supplied by the caller (RollEffectMagnitude).
 */
bool effectRollsMagnitude(const EffectDef *def);

/* The argument the game passes to RandomInRange for the random part (magnitudeMax - magnitudeMin). */
uint16_t effectRandomBound(const EffectDef *def);

/*
 * The magnitude for a party member of the given level. randomValue is the
 * result of RandomInRange(effectRandomBound(def)); it is ignored for fixed
 * and level-scaled effects. As in the original the product is truncated to
 * 16 bits.
 */
uint16_t effectMagnitude(const EffectDef *def, uint16_t level, uint16_t randomValue);

/*
 * The protection total added to the saving throw for a party record: the sum
 * of the protection values matching each inflicted condition (RollEffectResistance).
 * Zero if the effect doesn't roll a resistance.
 */
uint16_t effectResistanceBonus(const EffectDef *def, const uint8_t *partyRecord);

/*
 * RollEffectResistance's status-inflicted decision (yendor2.asm:14127,
 * instruction-identical in Chapter 3), given the saving throw's own
 * outcome rather than rolling it here -- keeps this function free of
 * an RNG dependency; a caller that needs the roll itself uses
 * combat.h's combatFailsSavingThrow with effectResistanceBonus(def,
 * defenderRecord) as its bonus parameter. Returns 0 if the effect
 * inflicts nothing at all (effectInflictedStatus(def) == 0);
 * effectInflictedStatus(def) outright if the effect doesn't roll a
 * resistance at all (EffectModeRollResistance unset -- the original
 * applies the status unconditionally, with no saving throw in this
 * case); otherwise effectInflictedStatus(def) if savingThrowFailed is
 * true, 0 (resisted) if false.
 */
uint16_t effectResolveInflictedStatus(const EffectDef *def, bool savingThrowFailed);

/*
 * RollEffectMagnitude's own roll-and-compute (yendor2.asm:14214,
 * instruction-identical in Chapter 3): rolls randomInRange(rng,
 * effectRandomBound(def)) only when the effect is neither fixed nor
 * level-scaled (matching the original's exact conditional RNG call --
 * a Fixed or Scaled effect never consumes a random draw), then
 * returns effectMagnitude(def, level, thatRoll). Effects that don't
 * roll a magnitude at all (effectRollsMagnitude(def) == false -- the
 * gold/ore-cost effects) aren't meaningfully covered by this function;
 * no combat call site this project has traced needs it (a monster's
 * ordinary attack effect is always HP-cost), so where a gold/ore
 * effect's spent amount actually comes from is still unconfirmed.
 */
uint16_t effectRollMagnitude(const EffectDef *def, uint16_t level, RandomState *rng);

#endif
