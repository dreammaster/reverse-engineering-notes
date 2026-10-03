#ifndef YENDOR23_THROWN_H
#define YENDOR23_THROWN_H

#include <stdbool.h>
#include <stdint.h>

#include "random.h"
#include "savegame.h"

/*
 * Thrown potions and flasks, and the ranged-weapon shot: ResolveAbilityEffect (yendor2.asm:24427,
 * yendor3.asm:22868, same logic, other item ids), ResolveAttackOrAbilityAction (:24310) and
 * ApplyResolvedDamageWithResistance (:24223). HandleRangedOrCombatAction (:23769) is the animation shell
 * around them (the projectile flying down the corridor row by row, message boxes, the weapon-select icons).
 * The "abilities" its g_currentActionId names are consumable items used on a monster:
 *
 *                 Chapter 2   Chapter 3   effect
 *   GOLD POTION      0x19        0x3D      60 damage
 *   SILVER POTION    0x1A        0x3E      35 damage and the monster is poisoned (status 0x8000, timer 6 x 5) unless immune to poison
 *   BLUE POTION      0x1B        0x3F      holy water: 50 (Chapter 3: 85) damage, only to an UNDEAD monster (kind 13,
 *                                          MonsterFieldUnknown4E) that does not resist 0x200; otherwise nothing
 *   FLAMING OIL FLASK 0x240      0x3C      40 damage along the corridor (area); nothing in formal combat
 * (the original also has branches for two more ids, 25 damage + status 0x400 and 15 + 0x4000, whose id globals are
 * 0xFFFF in both games: dead code, not reproduced.)
 *
 * Every use first clears the monster's timer words (+0x1C, +0x1E) and rolls RandomInRange(100): above 85 the
 * attack fizzles (no damage -- a 14 % failure in 0..100 inclusive). A damaging result always carries type flag
 * 0x8000; when a status was staged, a second roll above 70 discards the status (but the timers stay set -- an
 * original quirk) .
 */
enum {
    ThrownFailAbove = 85,
    ThrownStatusKeepAtMost = 70,
    ThrownTypePhysical = 0x8000,
    ThrownTypeWeaponMagic = 0x1000,
    ThrownTypeWeaponSecondary = 0x0800,
    ThrownKindUndead = 13,
    ThrownResistHolyBlock = 0x0200
};

typedef enum {
    ThrownNone,
    ThrownGoldPotion,
    ThrownSilverPotion,
    ThrownBluePotion,
    ThrownFlamingOil
} ThrownKind;

ThrownKind thrownKindForItem(GameKind game, unsigned itemId);

typedef struct {
    uint16_t damage;
    uint16_t typeFlags;
    uint16_t statusFlags;
    bool corridor; /* the flask: ResolveAttackOrAbilityAction applies it to three triples of viewport rows */
} ThrownEffect;

/* monsterRecord is modified (timers, see above). inFormalCombat = g_uiScratchFlags4 bit 0x1000. */
ThrownEffect thrownResolveAbilityEffect(GameKind game, unsigned itemId, uint8_t *monsterRecord, bool inFormalCombat, RandomState *rng);

/*
 * ApplyResolvedDamageWithResistance: nothing happens at 0 damage. Otherwise the staged status bits not covered by
 * the monster's immunities (+0x96) are ORed into its state (+0xC); the damage is halved once for every bit
 * set in (resistances (+0x98) AND the attack's type flags) -- 16 bit tests, so at most a /65536 -- and subtracted from
 * health (+0x10, clamped at 0); the monster's state gets 0x3 (hit markers) and the attack counts as having hit.
 * Returns true when it hit.
 */
bool thrownApplyResolvedDamage(uint8_t *monsterRecord, const ThrownEffect *effect);

/*
 * The ranged-weapon branch (g_uiScratchFlags3 bit 0x100): type flags from the weapon's target entry -- 0x1000 if
 * its word 1 has bit 0x400, 0x800 if word 4 is non-zero, always 0x8000. The hit roll is
 * combatResolveAttack(monster Absorption, shooter EquipRating1, shooter EquipRating2).
 */
uint16_t thrownWeaponTypeFlags(uint16_t entryWord1, uint16_t entryWord4);

/*
 * The depth-row triples a flask burns: for the projectile's current viewport depth row 0x24, 0x28, 0x2B or
 * anything else (0x2E) the three starting rows below; each start covers three consecutive rows
 * (ApplyDamageAlongCorridorLine), so the flask hits nine rows around where it landed.
 */
void thrownCorridorRowStarts(unsigned depthRow, unsigned starts[3]);

#endif
