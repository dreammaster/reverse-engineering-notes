#ifndef YENDOR23_AILMENT_H
#define YENDOR23_AILMENT_H

#include <stdbool.h>
#include <stdint.h>

#include "party.h"
#include "random.h"
#include "savegame.h"

/*
 * TickPartyAilmentIconBar (yendor2.asm:17721, yendor3.asm:20010) -- the periodic sweep, called from
 * RunDungeonGameLoop and ApplyMapTriggerEffect, that stages status-ailment damage for the party's icon bar.
 * It does not apply anything itself: it fills the per-member icon slots (effect id + preset severity) and then
 * calls ApplyEffectAndDrawIconBar (the pipeline combat.c's combatApplyEffect implements). This module
 * decides, per pass, which members get which staged effect.
 *
 * Runs only while g_uiScratchFlags1 has 0x0800 or 0x1000 set. Per-member severities (the amount staged):
 *   disease pass, effect id 2   (a member that is not incapacitated): DISEASED 12 + POISONED 6 + SICK 3
 *   curse pass,   effect id 0xE (not incapacitated and MP > 0):      CURSED 16 + HEXED 8 + JINXED 4
 *   slow pass (Chapter 2 only), effect id 2, while the party is in a "travel ailment" place (word_36C79 bit 2,
 *     set by travelling to a destination with flag 0x4000): any member who is not dead is staged with a
 *     severity tiered from the Survival skill (+0x58): <= 55 -> 12, <= 75 -> 9, <= 80 -> 6, <= 100 -> 3,
 *     <= 150 -> none; above 150 nobody (the original just returns). Higher Survival, lower severity.
 * A member is staged only when the severity is non-zero. Every pass scans the four active party slots in
 * order and stops at the first empty slot.
 *
 * Timing. Chapter 2: the slow pass first (every call, while its counter word_36CBF is non-negative -- it is
 * reset to 0 on entering a travel-ailment place and back to 1 after each sweep, so the "slow" wraparound the
 * code suggests never happens), then
 * a counter word_36CBD is incremented and the disease and curse passes run when it reaches 40 (and it resets to
 * 1). Chapter 3: the counter must exceed 40 (so every 41st call), resetting to 1; it has no slow pass, and
 * instead a second counter (word 0xCF3F, counting while word_36C79 bit 1 is set -- a cold place) triggers the
 * cold pass every 41st call: a roll of RandomInRange(3) + 1 picks one member by loop position (the first
 * member is position 4 ... the fourth is 1); every member that is not incapacitated and does not wear
 * item 0x10B (DWARVEN FUR, in the slot at +0x154) is staged with effect 0x2E, except the picked one, who is
 * staged with effect 0x2F and a magnitude of their level + 4.
 */
typedef struct {
    uint16_t fast;  /* word_36CBD / 0xCF3B */
    uint16_t slow;  /* word_36CBF (Chapter 2) / 0xCF3F cold counter (Chapter 3) */
} AilmentClock;

typedef enum {
    AilmentPassSlow,
    AilmentPassDisease,
    AilmentPassCurse,
    AilmentPassCold
} AilmentPassKind;

typedef struct {
    unsigned slot;            /* 0-3, the position in SaveHeaderPartySlots */
    uint16_t partyId;
    uint16_t effectId;
    uint16_t severity;        /* the preset amount; 0 for the cold pass (its effect rolls its own) */
    uint16_t magnitude;       /* the cold pass's picked member: level + 4, else 0 */
} AilmentStage;

typedef struct {
    AilmentPassKind kind;
    unsigned count;
    AilmentStage stages[4];
} AilmentPass;

enum {
    AilmentUiGateMask = 0x1800,
    AilmentEffectSick = 2,
    AilmentEffectCurse = 0xE,
    AilmentEffectCold = 0x2E,
    AilmentEffectColdPicked = 0x2F,
    AilmentColdProtectionItem = 0x10B,
    AilmentColdProtectionSlot = 0x154
};

uint16_t ailmentDiseaseSeverity(const uint8_t *record);
uint16_t ailmentCurseSeverity(const uint8_t *record);
uint16_t ailmentSlowSeverity(const uint8_t *record);

/*
 * One call of the sweep. uiFlags1 is g_uiScratchFlags1, travelFlags word_36C79 (bit 2 = Chapter 2 slow place,
 * bit 1 = Chapter 3 cold place). Fills passes[] in execution order and returns how many ran (including passes
 * that staged nobody, which the original still runs).
 */
unsigned ailmentTickIconBar(AilmentClock *clock, SaveGame *save, GameKind game, uint16_t uiFlags1, uint16_t travelFlags,
                            RandomState *rng, AilmentPass passes[4]);

#endif
