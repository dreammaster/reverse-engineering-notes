#ifndef YENDOR23_RELICS_H
#define YENDOR23_RELICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bcd4.h"
#include "item.h"
#include "party.h"
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
    RelicCrystalBall = 0x253,
    RelicFlower = 0x254,
    RelicCocoon = 0x255,
    RelicFeather = 0x256,
    RelicOrange = 0x257,
    RelicEmptyBottle = 0x2C8,
    RelicPanFlute = 0x26D,
    RelicCacheAmount = 5000,
    RelicVisionX = 0x154,
    RelicVisionY = 0x63,
    RelicFluteTrack = 8,
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

/*
 * DispatchItemAbilityCommand (:49086) -- the top-level dispatcher for the quest/
 * relic item cluster (reached from HandleGameCommand for an item whose target
 * flag 0x4 is clear), as a classifier of the item id:
 *   0x253 CRYSTAL BALL        RelicActionVision: a peek at the fixed cell
 *                             (RelicVisionX, RelicVisionY) = (340, 99) facing
 *                             north; the party does not move (view-only)
 *   0x258                     RelicActionLocationPotion (relicUseLocationPotion)
 *   0x2C8 EMPTY POTION BOTTLE RelicActionAssemblePotion (relicAssemblePotion)
 *   0x242-0x245 the keys      RelicActionDiscoveryKey (UseAbilityOnTarget; the
 *                             0xDFBB discovery table travel.c already covers)
 *   0x26D ELFIN PAN FLUTE     RelicActionPlayFlute: stops the music and plays
 *                             track RelicFluteTrack
 *   0x246-0x249               RelicActionCharged (gated on relicReady; 0x249 also
 *                             needs combat) -- the four relics above
 * anything else does nothing.
 */
typedef enum {
    RelicActionNone,
    RelicActionVision,
    RelicActionLocationPotion,
    RelicActionAssemblePotion,
    RelicActionDiscoveryKey,
    RelicActionPlayFlute,
    RelicActionCharged
} RelicAction;

RelicAction relicClassify(unsigned itemId);

/*
 * CheckQuestItemsCompleted (:49380): the EMPTY POTION BOTTLE combines the four
 * ingredients FLOWER, COCOON, FEATHER and ORANGE (ids 0x254-0x257) into the
 * POTION OF APPRECIATION (0x258, which relicUseLocationPotion then uses at its
 * one cell). Each is looked up with itemRangeAvailable (the resource panel's
 * global slots first, then the party's packs); if any is missing nothing happens
 * ("you need ..." message, returns false). Otherwise all four ingredients and the
 * bottle are consumed (partyConsumeItemCharge / itemSlotConsumeGlobalCharge, in
 * the original's order: ORANGE, FEATHER, COCOON, FLOWER, bottle) and the caller
 * puts RelicPotionOfAppreciation on the cursor. Returns true when it brewed.
 */
enum { RelicPotionOfAppreciation = RelicLocationPotion };

bool relicAssemblePotion(uint8_t *globalSlots, SaveGame *save, const ItemCatalog *catalog);

#endif
