#ifndef YENDOR23_CONSUMABLE_H
#define YENDOR23_CONSUMABLE_H

#include <stdbool.h>
#include <stdint.h>

#include "bcd4.h"
#include "game.h"

/*
 * Using a restorative item or the alchemist's ore transmutation on a party
 * member. In the original these are the branches of `CastSpell`
 * (yendor2.asm:48623; Chapter 3 has the same restorative branches under
 * different item ids and drops the transmutation -- a misleading name:
 * `HandleGameCommand` sends these item ids here, and they are the potions and
 * the like, not spells). Each branch picks a recipient,
 * checks that the item would actually do something, and on success spends the
 * item's charge (ConsumeItemChargeResource); when it wouldn't do anything the
 * original just flashes a warning and asks for another recipient, spending
 * nothing -- the `false` returns here.
 *
 * The six effects, all on the chosen recipient:
 *   HpQuarter   HP + max/4, capped at max        (needs HP < max)
 *   HpHalf      HP + max/2, capped at max        (needs HP < max)
 *   HpFull      HP to max                        (needs HP < max)
 *   MpHalf      MP + max/2, capped at max        (needs a nonzero max MP, MP < max)
 *   MpFull      MP to max                        (same)
 *   Cure        clear Diseased/Poisoned/Sick     (needs one of those: status 0xE000)
 * All comparisons are signed 16-bit, as in the original; the divisions truncate.
 *
 * The item ids (g_currentActionId) differ per game:
 *   Chapter 2: 0x12 HpQuarter, 0x13 HpHalf, 0x14 HpFull, 0x1D MpHalf, 0x17 MpFull,
 *              0x18 Cure (and 0x1C is the ore transmutation below)
 *   Chapter 3: 0x34 HpQuarter, 0x35 HpHalf, 0x36 HpFull, 0x37 MpHalf, 0x38 MpFull,
 *              0x39 Cure (no transmutation: the ore economy is gone)
 */
typedef enum {
    RestorativeNone,
    RestorativeHpQuarter,
    RestorativeHpHalf,
    RestorativeHpFull,
    RestorativeMpHalf,
    RestorativeMpFull,
    RestorativeCure
} RestorativeKind;

RestorativeKind partyRestorativeForItem(GameKind game, unsigned itemId);

/* False (nothing changes, nothing is spent) when the item would do nothing for this recipient. */
bool partyUseRestorative(RestorativeKind kind, uint8_t *partyRecord);

/*
 * The alchemist's transmutation (item id 0x1C): MAGIC ORE <-> NUORE.
 *
 * The user picks the direction (the confirm prompt's two answers). The
 * alchemist must have PartyStatChemistry >= 65 (0x41) -- otherwise "YOUR SKILL
 * IS NOT HIGH ENOUGH" -- and the source counter must hold at least 10 units
 * ("YOU MUST HAVE AT LEAST 10 UNITS"). Then up to 100 units of the source
 * are consumed (all of it, if it has fewer) and the destination gains
 * floor(consumed / divisor), where the divisor falls with skill:
 *      Chemistry >= 110: 2   >= 95: 4   >= 80: 5   otherwise: 10
 * (the original computes (consumed / d) x d / d, which is the same floor).
 * The old note that this converts "10 units" was wrong: it is the whole
 * stack, up to 100.
 */
typedef enum {
    AlchemySkillTooLow,
    AlchemyTooFewUnits,
    AlchemyDone
} AlchemyResult;

unsigned alchemyYieldDivisor(uint16_t chemistry);

/* source/destination are the two ore counters in the chosen direction. *consumed and *yield report what moved. */
AlchemyResult partyTransmuteOre(const uint8_t *partyRecord, Bcd4 source, Bcd4 destination, unsigned *consumed,
                                unsigned *yield);

/*
 * The percentage restoratives (RestCharacter, yendor2.asm:48544, yendor3.asm:49236): the item's target entry carries a percentage (word 2,
 * [+4]); `restoresMp` is the entry's word 1 bit 0x8000. Chapter 2 uses item ids 0x36-0x46 (health and magic variants), Chapter 3 only 0x1F-0x20
 * and always restores magic -- its health branch is gone.
 *
 * Health: HP += (max HP x percent + 50) / 100, capped at max, no other condition. Magic: the class id ([+0x0E], reduced by 10 twice while
 * above 9) must be at least 4, otherwise nothing is restored and the user is made Sick (PartyStatusSick); a caster gains
 * (max MP x percent + 50) / 100, capped at max. In both cases the item's charge is spent. The division is the original's 32-bit
 * DX:AX by 100 of the unreduced product with the +50 added to the low word only (a carry out of it is lost), reproduced here.
 */
typedef enum { PercentRestoreApplied, PercentRestoreMadeSick } PercentRestoreResult;

bool partyIsPercentRestorativeItem(GameKind game, unsigned itemId);
PercentRestoreResult partyUsePercentRestorative(GameKind game, uint8_t *partyRecord, bool restoresMp, unsigned percent);

#endif
