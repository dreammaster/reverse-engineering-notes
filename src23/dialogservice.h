#ifndef YENDOR23_DIALOGSERVICE_H
#define YENDOR23_DIALOGSERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bcd4.h"
#include "dialog.h"
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

#endif
