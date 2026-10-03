#ifndef YENDOR23_RELICS_H
#define YENDOR23_RELICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bcd4.h"
#include "savegame.h"

/*
 * Chapter 2's quest relics (DispatchItemAbilityCommand, yendor2.asm:49086;
 * HandleGameCommand's fall-through for item ids 0x242-0x2C8 -- the "themed
 * cluster" earlier notes called UseAbilityOnTarget / the item-icon
 * dispatcher). Chapter 3 has no equivalent (its ore economy and relic quest
 * are replaced by the artifact system).
 *
 * The four charge-gated relics (item ids 0x246-0x249) all require global flag
 * 0xB1 to be set -- the original's "recharge" flag -- and otherwise only
 * show "PATIENCE IS A VIRTUE.". Each use spends the item's charge
 * (ConsumeItemChargeResource, not modeled here):
 *   0x246  +5,000 NUORE          0x247  +5,000 MAGIC ORE
 *   0x248  mass heal and overheal (relicMassHealAndOverheal)
 *   0x249  instant kill of the engaged monster, in combat only (relicInstantKill)
 * Outside combat 0x249 also shows the patience message and does nothing.
 */
enum {
    RelicRechargeFlag = 0xB1,
    RelicNuoreCache = 0x246,
    RelicMagicOreCache = 0x247,
    RelicMassHeal = 0x248,
    RelicInstantKill = 0x249,
    RelicLocationPotion = 0x258,
    RelicCacheAmount = 5000,
    LocationPotionX = 0x68,
    LocationPotionY = 0x6E,
    LocationPotionFlag = 0x48
};

/* Whether the recharge flag allows a relic to be used. */
bool relicReady(const uint8_t *globalFlags, size_t flagsSize);

/* The ore cache relics: adds RelicCacheAmount to the counter. */
void relicAddCache(Bcd4 counter);

/*
 * PartyMassHealAndOverheal (:49306): for every member in party order (stopping
 * dead at the first unoccupied slot), clears every status bit above the low six
 * -- including Dead -- and sets current HP and MP to TWICE their maxima
 * (16-bit doubling, so a maximum above 32767 wraps), then refreshes carry
 * capacity/attribute bonuses. Returns how many members were affected.
 */
unsigned relicMassHealAndOverheal(SaveGame *save);

/* InstantKillActiveMonster (:49363): zeroes the monster's health (MonsterFieldHealth). */
void relicInstantKill(uint8_t *monsterRecord);

/*
 * UseLocationBoundPotion (:49201, item 0x258): works only standing on the map
 * cell (0x68, 0x6E) = (104, 110); there it sets global flag 0x48 and spends the
 * potion, elsewhere it says "YOU CAN NOT USE THAT HERE!". Returns whether it
 * worked and applies the flag.
 */
bool relicUseLocationPotion(int worldX, int worldY, uint8_t *globalFlags, size_t flagsSize);

#endif
