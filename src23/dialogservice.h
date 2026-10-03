#ifndef YENDOR23_DIALOGSERVICE_H
#define YENDOR23_DIALOGSERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bcd4.h"
#include "dialog.h"
#include "item.h"
#include "game.h"
#include "savegame.h"

/*
 * The topic handlers behind dialog.h's DialogTopicFlag bits, as
 * decide-and-apply functions on the data model (the UI parts -- the
 * "+10 STRENGTH" overlay, the icon-bar slots, the redraws -- are not
 * modeled). Both tomes below are one-time gifts: the NPC header names a
 * global flag (DialogNpcOneTimeFlag) that is set the first time and makes any
 * later use a no-op, whichever party member asks.
 */

typedef struct {
    bool applied;     /* false: the one-time flag was already set, nothing happened */
    unsigned members; /* party members changed */
} DialogTomeResult;

/*
 * UseAttributeBoostItem (yendor2.asm:20142, instruction-identical in
 * Chapter 3; topic flag DialogTopicAttributeBoost): adds DialogNpcParamB
 * (the amount) to the party-record word at byte offset DialogNpcParamA and to
 * its maximum 0x40 bytes further on, for every member in party order that
 * isn't Dead -- stopping dead at the first unoccupied slot, like the other
 * whole-party loops. A member whose current value isn't positive (an untrained
 * stat) is skipped. Each result is capped at 999, or 9999 when the offset is
 * 0x52/0x54 (current HP/MP), by ClampValueAtSlotToTypeCap -- applied to the
 * current value after its addition and to the maximum after its own, which is
 * also added unconditionally. Then the member's carry capacity and attribute
 * bonuses are refreshed.
 */
DialogTomeResult dialogApplyAttributeTome(const uint8_t *npc, SaveGame *save, uint8_t *globalFlags, size_t flagsSize);

/*
 * UseExperienceBoostItem (:20269, topic flag DialogTopicExperienceBoost): adds
 * the packed-BCD amount at DialogNpcParamA (4 bytes, spanning ParamA/ParamB) to
 * every non-Dead member's experience and runs the level-up check, again
 * stopping at the first unoccupied slot.
 */
DialogTomeResult dialogApplyExperienceTome(const uint8_t *npc, SaveGame *save, GameKind game, uint8_t *globalFlags,
                                           size_t flagsSize);

/*
 * The healer topics (UseHealingItem, yendor2.asm:21329, instruction-identical
 * in Chapter 3; topic flag DialogTopicPreview with a type word in
 * DialogTopicArg). The type word selects the service:
 */
typedef enum {
    DialogHealFull = 0x1000,   /* HP to maximum and every condition cleared, including death (priced as the sum of what's wrong) */
    DialogHealRevive = 0x2000, /* clear Dead and set HP to 2 */
    DialogHealCure = 0x4000,   /* clear every condition (the 0xFF80 status bits) */
    DialogHealHp = 0x8000      /* HP to maximum */
} DialogHealType;

/*
 * ClassifyPartyMemberCondition (:20420): decides which healer topics apply to
 * the chosen party member by rewriting the top bits of mask A (the low 10
 * bits are kept): 0x2000 if Dead, 0x4000 if any condition bit (0xFF80) is
 * set, 0x8000 if HP is below maximum, 0x1000 if both of the last two hold
 * (more than one "tier" to fix; a dead member counts only as 0x2000, though
 * being at 0 HP usually also sets 0x8000), 0x200 if nothing is wrong. These
 * are exactly the own bits of the healer's REVIVE / CURE / HEAL / HEAL ALL
 * topics, which is how the menu shows only what's needed.
 *
 * Quirk reproduced: the bits kept are the low TEN (mask 0x3FF), which includes
 * 0x200 itself, so a "nothing wrong" mark left by an earlier member survives
 * reclassifying a hurt one until a topic clears it.
 *
 * The type word (a topic's DialogTopicArg) convention seen in the healer data:
 * the greeting (flags DialogTopicPreview) carries the default type, each service
 * topic (HEAL, CURE, RESURRECT, RESTORATION) carries its DialogHealType, and
 * YES / NO carry 2 / 4 -- accepting the quote performs the service chosen last.
 */
void dialogClassifyCondition(DialogState *state, const uint8_t *partyRecord);

/*
 * The per-condition prices ComputeAfflictionHealingCost (:20759) sums over the
 * member's status bits: Sick 5, Poisoned 10, Diseased 20, Paralyzed 40,
 * Frozen 50, Stoned 60, Jinxed 20, Hexed 30, Cursed 40.
 */
unsigned dialogAfflictionCost(const uint8_t *partyRecord);

/*
 * The cost of a healer service for a member (ShowHealingCostPrompt, :20687):
 * a base price by type -- 20 for HP, 100 for revive, the affliction sum for
 * cure, and for the full service whichever of the three apply per the
 * classification in state->availA -- times the NPC's DialogNpcPriceMultiplier
 * (truncated to 16 bits, as the original's `mul` is stored), summed once per
 * point of the member's level in packed BCD. The result is written to *cost.
 */
void dialogHealingCost(const uint8_t *npc, unsigned type, const DialogState *state, const uint8_t *partyRecord,
                       Bcd4 cost);

/*
 * Pays and performs the service (UseHealingItem's purchase branch, :21401):
 * false (nothing happens) if gold is less than cost; otherwise gold -= cost
 * and the effects apply in the original's order: Revive clears Dead and sets
 * HP 2; Cure keeps only the low 7 status bits; HP sets HP to maximum; Full
 * sets HP to maximum and keeps only the low 6 bits. Several type bits may be
 * set at once and all apply.
 */
bool dialogApplyHealing(unsigned type, const Bcd4 cost, Bcd4 gold, uint8_t *partyRecord, GameKind game);

/*
 * The trainer (UseTrainingItem, :21510; NPCs whose greeting carries type
 * 0x4000). dialogTrainingQuote is the TRAIN topic's quote (:21860): false if
 * the member's level + 1 would exceed the trainer's cap (DialogNpcParamB, e.g.
 * 10 for the Chapter 2 NPC 10 -- a trainer only takes you so far); otherwise
 * the price is 100 x DialogNpcPriceMultiplier x level, in BCD as for
 * healing. Accepting (TRAIN NOW) is party.h's partyApplyTraining with that
 * cost.
 */
bool dialogTrainingQuote(const uint8_t *npc, const uint8_t *partyRecord, Bcd4 cost);

/*
 * The challenge NPCs (UseItemType_800, :20988; greeting type 0x800 -- 20 in
 * Chapter 2, "THE CHALLENGE OF PROJECTILE ACCURACY / INTELLIGENCE / HEALTH
 * POINTS ..."). The header names a stat (DialogNpcParamA: an offset into the
 * party record's MAXIMUM-stat array, e.g. 0x88 = projectile accuracy rating,
 * 0x82 = intelligence, 0x92 = hit points), a threshold (DialogNpcParamB), an
 * entry fee (dialogNpcFee), a reward multiplier (DialogNpcPriceMultiplier) and
 * the per-character "already won" flag index (DialogNpcCharacterFlagIndex,
 * bank 0x10C).
 *
 * Accepting: not enough gold -> DialogChallengeNoGold, nothing happens.
 * Otherwise the fee is paid FIRST, whatever happens next; then if the stat
 * is at or above the threshold (signed compare, no dice -- the "challenge" is
 * a pure stat check) the member wins: the fee is paid back `multiplier` times
 * over (so the net prize is (multiplier - 1) x fee), `*reward` receives
 * fee x multiplier, and the member's flag is set so they can't win again.
 * Below the threshold the fee is simply lost and the flag stays clear (they may
 * try again).
 */
typedef enum {
    DialogChallengeNoGold,
    DialogChallengeLost,
    DialogChallengeWon
} DialogChallengeOutcome;

DialogChallengeOutcome dialogAttemptChallenge(const uint8_t *npc, uint8_t *partyRecord, Bcd4 gold, Bcd4 reward);

/*
 * CheckPartyMemberItemFlag(AndClearPanel) (:20392): marks the services of a
 * "once per character" NPC as available to the chosen member -- mask A
 * becomes (availA & 0x1FFF) | 0x1000, plus 0x8000 if the member's flag
 * (DialogNpcCharacterFlagIndex in bank 0x10C) is still clear. In the challenge
 * NPCs' data 0x8000 is the TRY CHALLENGE topic's own bit and 0x1000 FINISHED's,
 * so a member who already won sees only FINISHED.
 */
void dialogMarkServiceAvailability(DialogState *state, const uint8_t *npc, const uint8_t *partyRecord);

/*
 * Riddle and password topics (UseRiddleAnswerItem, :18436; a topic with flag
 * DialogTopicBuy whose name does not start with "BUY "). The topic's
 * DialogTopicArg is the riddle id (1-based; Chapter 2 has 6, Chapter 3 11) and
 * the expected answers live in the executable, not WORLD.DAT -- a table of
 * near pointers (DS:0xBF48 / 0xA8C6), extracted with ida_scripts/
 * dump_riddle_answers.py and embedded here:
 *   Chapter 2: PENTAGON, LINGUISTIC, WHITE POTION, THAINE, SHIRLEY, GAIN
 *   Chapter 3: PEACEFUL, 120, ARCHIBALD, OVIAS, WIN, 30, 500, 3925, 46080,
 *              400000, 70
 * The check is an exact, case-sensitive, whole-string compare against what the
 * player typed into the 34-character field. A correct answer is the "handler
 * succeeded" signal that makes a DialogTopicConditional topic's result flags
 * apply (dialogVisitTopic); a wrong one just asks again.
 */
const char *dialogRiddleAnswer(GameKind game, unsigned riddleId);
bool dialogCheckRiddleAnswer(GameKind game, unsigned riddleId, const char *typed);

/*
 * "BUY NUORE" / "BUY MAGIC ORE" (PromptBuyOreQuantity, :19565; topic flag
 * DialogTopicBuy, argument 3 for NUORE and 2 for MAGIC ORE, "ORE COSTS 10 GOLD
 * PER UNIT"). The player types a quantity; it must not exceed what the gold
 * affords (gold with its last digit dropped -- gold / 10 truncated). On success
 * the ore counter gains quantity and gold loses quantity x 10. Returns false,
 * changing nothing, when it is too much. Any argument other than 2 buys NUORE.
 * (The original also flashes its "resource depleted" overlay the first time a
 * counter gains ore or the gold hits exactly zero; not modeled.)
 */
bool dialogBuyOre(unsigned kind, Bcd4 gold, Bcd4 magicOre, Bcd4 nuore, const Bcd4 quantity);

/*
 * The item services -- ENHANCE, REPAIR and SELL topics. The party carries the
 * item on the cursor; the NPC header supplies the terms.
 *
 * Selling (TrySellItemForGold, :15997): the NPC buys an item when the SELL
 * topic's argument (DialogTopicArg: a class mask such as 0x8000 = gear, 0x4000
 * = potions...) shares a bit with the item's ItemFieldClass; the price
 * paid is shop.h's shopSellPrice.
 *
 * Enhancing (TryEnhanceItemForGold, :15729; IsItemEligibleForEnhance :19713):
 * an item qualifies if, for armour (item flags 0x800 or 0x200) whose target
 * entry has slot-flag bit 0x100, target word 3 -- or, for a weapon (0x4000 or
 * 0x8000) whose target has bit 0x800, target word 4 -- lies between
 * DialogNpcParamA and DialogNpcParamB inclusive (signed). The upgrade is the
 * NEXT catalog id (id + 1: the "+1" version sits right after the base item);
 * it costs that item's base value x DialogNpcPriceMultiplier percent (BCD).
 *
 * Repairing (TryRepairItemForGold, :15876; IsItemEligibleForRepair :19793):
 * a weapon (0x4000/0x8000) whose target has slot-flag bit 0x100, or an item with
 * flag 0x800 whose target has bit 0x40, can be repaired; the damaged item
 * carries its original id in its "extra" word, and repair costs THAT item's base
 * value x the multiplier percent and restores it.
 *
 * Real data bears the enhance rule out: Chapter 2's "+5 up to +7" smith (NPC
 * 57, ParamA 5, ParamB 6, 80%) takes exactly the +5 and +6 armour -- target
 * word 3 (item.h calls it ItemTargetBreakChanceA) is the item's "+N" level --
 * and the first smith (NPC 9, 0..2) the plain through +2 ones. A quirk not
 * reproduced: IsItemEligibleForRepair's second test reuses a register that
 * still points at the target entry when the first test fell through, reading
 * 12 bytes past it; the intended meaning is implemented.
 */
bool dialogSellAccepts(uint16_t sellTopicArg, const uint8_t *itemRecord);
bool dialogEnhanceEligible(const uint8_t *npc, const ItemCatalog *catalog, const uint8_t *itemRecord);
bool dialogEnhanceCost(const uint8_t *npc, const ItemCatalog *catalog, unsigned itemId, Bcd4 cost);
bool dialogRepairEligible(const ItemCatalog *catalog, const uint8_t *itemRecord);
bool dialogRepairCost(const uint8_t *npc, const ItemCatalog *catalog, unsigned originalItemId, Bcd4 cost);

#endif
