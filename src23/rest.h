#ifndef YENDOR23_REST_H
#define YENDOR23_REST_H

#include <stdbool.h>
#include <stdint.h>

#include "gameclock.h"
#include "item.h"
#include "savegame.h"

/*
 * The R command, RestPartyAndAdvanceClock (yendor2.asm:25686, Chapter 3 :24173), composed from the pieces that already exist (gameclock.h, party.h):
 *
 *  1. Not allowed while an item is held on the cursor (the command returns at once), nor where IsRestingAllowedHere says no (gameclock.h: the "no rest"
 *     flag or the special-cell table); that case only flashes the message and plays sound 3.
 *  2. Up to eight one-hour slices. Each slice first runs ProcessLevelMonsters (the caller's `monstersTurn`); if that starts a combat (UI flag 0x1000) the rest
 *     stops there with no time added for that slice. Otherwise the clock gains 60 minutes. The message shows the number of the slice reached (`hour`): 8 after a
 *     full rest, the interrupted slice's number (1 = the first) otherwise.
 *  3. The calendar rolls once afterwards on the total (gameClockAdvance, with its two shared original bugs); a day change resets every character's daily
 *     ability charges (partyResetDailyAbilityCharges -- the caller does that on `dayRolled`).
 *  4. Only when not interrupted: each active member (not incapacitated) eats one camping food item if any is available (partyDeriveRestRegenPercent), the
 *     regeneration percentage is (100 / active) * eaten, and every member takes partyApplyRestEffects with it -- Sick / Jinxed cured, Diseased and Cursed drained,
 *     otherwise HP and MP healed by that percentage of their maximum. The message then shows how many members ate.
 */
typedef struct {
    bool refused;        /* held item or no resting here: nothing happened (`noRest`: the message and sound 3) */
    bool noRest;
    bool interrupted;    /* a monster engaged the party; no healing */
    unsigned hour;       /* the slice number shown in the message */
    unsigned minutes;    /* clock minutes added */
    bool dayRolled;
    unsigned fed;        /* members who ate (not interrupted) */
    uint16_t regenPercent;
    unsigned died;       /* members the Diseased drain killed */
} RestOutcome;

/* Called once per slice; true when a monster has engaged the party (combat starts). */
typedef bool (*RestMonstersTurn)(void *ctx);

RestOutcome restParty(SaveGame *save, GameClock *clock, const ItemCatalog *catalog, uint8_t *globalSlots, bool heldItem, bool noRestFlag, bool inTriggerList,
                      RestMonstersTurn monstersTurn, void *ctx);

#endif
