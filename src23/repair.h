#ifndef YENDOR23_REPAIR_H
#define YENDOR23_REPAIR_H

#include <stdbool.h>
#include <stdint.h>

#include "random.h"

/*
 * The skill-based repair attempt (RepairItemCommand, yendor2.asm:50996,
 * instruction-identical in Chapter 3; the "use a repair kit on a damaged item"
 * path HandleGameCommand reaches -- distinct from the NPC repair service in
 * dialogservice.h). The repairing party member (not incapacitated, chosen by
 * the player) rolls RandomInRange(100) -- 0..100 inclusive, see random.h -- against a
 * pair of thresholds from a table in the executable (DS:0x6B7E / 0x6EAC,
 * identical in both games; ida_scripts/dump_repair_table.py), embedded here:
 *
 *   row  = the damaged item's difficulty: its "+N" level for armour (target
 *          word 3), or target word 4 for a weapon -- 0..15;
 *   tier = from the repairer's PartyStatRepair: < 50, < 65, < 80, < 95, else
 *          0..4 (signed compares).
 *
 * Each (row, tier) has (low, high): a roll BELOW low is a critical failure (the
 * item is destroyed), a roll above high a soft failure (nothing happens), and
 * low..high inclusive a success (the item is repaired). Higher-level items are
 * much harder: row 4 tier 0 is (100, 0), i.e. an unskilled repairer destroys it
 * on any roll below 100; row 0 tier 4 is (0, 100), a guaranteed success.
 *
 * The repair kit's charge is spent on a critical failure and on a success, but
 * not on a soft failure (the original skips ConsumeItemChargeResource there).
 */
typedef enum {
    RepairSoftFail,
    RepairCriticalFail,
    RepairSuccess
} RepairOutcome;

enum { RepairRows = 16, RepairTiers = 5 };

unsigned repairSkillTier(uint16_t repairStat);

/* The (low, high) thresholds for a row/tier; false if the row is past the table. */
bool repairThresholds(unsigned row, unsigned tier, uint16_t *low, uint16_t *high);

/* Rolls once. A row past the table is a soft failure without rolling. */
RepairOutcome repairAttempt(unsigned row, uint16_t repairStat, RandomState *rng);

/* Whether the outcome spends the repair kit's charge. */
bool repairConsumesCharge(RepairOutcome outcome);

#endif
