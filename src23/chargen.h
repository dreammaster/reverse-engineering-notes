#ifndef YENDOR23_CHARGEN_H
#define YENDOR23_CHARGEN_H

#include <stdbool.h>
#include <stdint.h>

#include "item.h"
#include "party.h"
#include "random.h"

/*
 * Character creation's stat generation: RollCharacterAttributes (yendor2.asm:37365,
 * yendor3.asm:37121 -- the two are instruction-identical) and
 * ComputeDerivedCharacterStats (yendor2.asm:35535, yendor3.asm:35331 -- same
 * structure, different class tables). Both write the current value and its copy
 * 0x40 higher (the stat maximum).
 *
 * Attributes. Six rolls of RandomInRange(15) + 45 (0..15 inclusive, so 45-60), in
 * THIS order: Strength (0x3C), Dexterity (0x3E), Intelligence (0x42), Wisdom
 * (0x44), Charisma (0x46), Stamina (0x40) -- not the stat-table order. Then
 *   - Carry capacity (0x56) = 10 x Strength (16-bit);
 *   - Hit points (0x52) = Stamina x 25 % (rounded, see below);
 *   - Magic points (0x54) and the CASTING skill (0x62) from the class
 *     (record +0xE; a signed compare, classes below 4 get 0 / 0):
 *       class  4 (MONK)       base = Wisdom,           casting = base + 10
 *       class  5 (ALCHEMIST)  base = 75 % Wis + 25 % Int, casting = base + 5
 *       class  6 (PALADIN)    base = 50 % Wis,         casting = 2 x base, minus 1 when Wisdom is odd
 *       class  8 (DRUID)      base = 75 % Int + 25 % Wis, casting = base + 5
 *       class  9 (MARKSMAN)   base = 50 % Int,         casting = 2 x base, minus 1 when Wisdom is odd
 *       class  7 and 10+      base = Intelligence,     casting = base + 10
 *     MP = base / 4. (Classes 1-3 -- FIGHTER, MERCHANT, ROGUE -- have no magic.)
 *     (The PALADIN/MARKSMAN "+ base - odd" is the original's quirk: the bonus
 *     register is loaded with the base and decremented by an odd Wisdom.)
 *   - EquipRatingBase1/3 (0x32, 0x34 and their maxima) are zeroed: the "nothing
 *     writes them" fields of party.h are character creation's own reset.
 * The percentage scaling everywhere is ScaleByPercentRounded: (v * pct + 50) / 100
 * with the multiply and the +50 done in 16 bits (they wrap above 655 / pct).
 *
 * Derived skills (12 of them, 0x58-0x70 except 0x62): each is a blend of
 * attributes -- a sum of scaled terms, or (percent 0) a raw copy of one attribute
 * -- plus a per-class bonus, or for some classes a flat value (0 or 40) that
 * replaces the blend. The tables are generated from the disassembly and checked
 * against an emulation of it. Class 0 and every class above 9 take the "default"
 * column. Chapter 3 re-tunes most class bonuses and flat values and does not
 * compute Chemistry (0x70) at all (the record keeps whatever it held).
 */
typedef struct {
    uint16_t recordOffset;
    uint8_t termCount;
    struct {
        PartyStat attribute;
        uint8_t percent; /* 0 = the attribute itself, unscaled */
    } terms[3];
    int8_t flat[10];  /* by class; -1 = no flat value, use the blend */
    int8_t bonus[10]; /* by class */
} DerivedStatRule;

enum { CharGenRollBound = 15, CharGenRollBase = 45 };

/* The six rolls in roll order (each 0..CharGenRollBound), applied to the record (its class must be set). */
void partyApplyRolledAttributes(uint8_t *record, const uint8_t rolls[6]);

/* RollCharacterAttributes: rolls the six values from rng, then partyApplyRolledAttributes. */
void partyRollAttributes(uint8_t *record, RandomState *rng);

/* ComputeDerivedCharacterStats for the record's class. */
void partyComputeDerivedStats(uint8_t *record, GameKind game);

/*
 * ApplySecondaryClassTierFlags (yendor2.asm:37509, yendor3.asm:37265), run when a new character's summary is
 * shown: for a character with any magic (maximum MP above 0) whose class is 4-9, the starting ability flags
 * (PartyFieldFlagBankCA) of that class are set -- up to two, from a table that is identical in both games
 * (ida_scripts/dump_class_start_flags.py):
 *   MONK 1,3   ALCHEMIST 1,2   PALADIN 1   MAGE 2,3   DRUID 1,2   MARKSMAN 2
 * (a 0 entry ends the pair). Chapter 2 also records the secondary-class bit (partyClassSecondaryBit) in the
 * status flags; Chapter 3 does not. Returns how many flags were set.
 */
unsigned partyApplyStartingAbilities(uint8_t *record, GameKind game);

/* The class base's (4-9) two starting ability ids (0 = none) -- the table the clue book's spell page checks for its "CREATION" rows. */
void partyStartingAbilityIds(unsigned classBase, uint16_t out[2]);

/*
 * The class-selection step of character creation (the screen the roster calls ShowCharacterSkills,
 * yendor2.asm:35989; its keys F M R O A P D K and the rest pick class base 1-9, also reachable from the summary
 * screen's class key). Entering the screen clears the record's secondary-class status bits (the low six of
 * +0x1C) and the 16-word ability flag bank at +0xCA; choosing a class sets PartyFieldClass = classBase and
 * PartyFieldLevel = 1, and, when the choice is made during creation (not just browsing), rolls the
 * attributes, derives the skills and recomputes the equipment ratings. The summary screen's 'R' reroll is the
 * same roll + derive + recompute. gender, portrait and name come from the other steps (UI).
 */
void partyBeginClassSelection(uint8_t *record);
void partyChooseClass(uint8_t *record, unsigned classBase, GameKind game, const ItemCatalog *catalog, RandomState *rng);
void partyRerollAttributes(uint8_t *record, GameKind game, const ItemCatalog *catalog, RandomState *rng);

/* The rule table (for tests/tools); *count receives its length. */
const DerivedStatRule *partyDerivedStatRules(GameKind game, unsigned *count);

#endif
