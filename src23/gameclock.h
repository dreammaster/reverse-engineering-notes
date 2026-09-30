#ifndef YENDOR23_GAMECLOCK_H
#define YENDOR23_GAMECLOCK_H

#include <stdbool.h>
#include <stdint.h>

/*
 * The in-game clock/calendar, and the "R rest" command's own decision
 * logic (`RestPartyAndAdvanceClock`, yendor2.asm:25686, yendor3.asm:24173).
 * A 30-day-month, 12-month-year calendar; `minutes` is "minutes since
 * midnight" (1-1440, matching `AdvanceGameClock`'s own per-minute ISR
 * tick, not otherwise reimplemented here since it's driven by real-time
 * hardware timing this project doesn't simulate).
 */
typedef struct {
    uint16_t minutes;
    uint16_t day;
    uint16_t month;
    uint16_t year;
} GameClock;

/*
 * Advances the clock by deltaMinutes and rolls the calendar exactly
 * like `RestPartyAndAdvanceClock`'s own inlined day-rollover math
 * (instruction-identical in both games, including a real, shared
 * original bug: the rollover check fires at `minutes >= 1439`, one
 * short of the actual day length (1440), so subtracting 1440
 * underflows to 65535 whenever the pre-subtraction value is exactly
 * 1439 -- reachable with entirely ordinary inputs (e.g. resting from
 * minute 1379 for exactly one hour). Confirmed identical in both
 * games (0x59F/0x5A0), not a Chapter 2 bug Chapter 3 fixes -- reproduced
 * faithfully, not corrected). Also reproduces a second bug, likewise
 * shared: when a month rollover also triggers a year rollover, `month`
 * is left at 13 instead of being reset to 1 -- unlike `AdvanceGameClock`'s
 * own per-minute tick, which does reset it. Both are real, if rare,
 * player-visible clock/calendar corruption in the original, not this
 * reimplementation's own invention.
 *
 * Returns true if a day rolled over (the caller should then reset
 * every party member's daily ability charges -- see
 * partyResetDailyAbilityCharges, party.h -- matching
 * `ResetDailyAbilityCharges`'s own call site here).
 */
bool gameClockAdvance(GameClock *clock, uint16_t deltaMinutes);

/*
 * IsRestingAllowedHere (yendor2.asm:26030, yendor3.asm:24610). Chapter
 * 2's own copy additionally carries a whole dead branch testing a
 * "forbidden map id" global (`word_34748`) that has no confirmed write
 * site anywhere in its disassembly (so it always reads 0 with real
 * data) -- Chapter 3 doesn't even have this branch at all, confirming
 * it really is unused rather than merely unconfirmed. Not modeled
 * here since neither game's real behavior depends on it.
 * isInTriggerList is IsPositionInTriggerList's own result -- a
 * genuinely separate, general-purpose map-trigger table (also feeding
 * ApplyMapTriggerEffect, not just resting) that hasn't been extracted
 * or reimplemented yet; supply it as an already-computed input.
 * noRestFlag is the global "no resting allowed" flag (word_36C79 bit
 * 0x2, Chapter 2; the Chapter 3 equivalent word wasn't traced this
 * round) -- also supply already-decoded.
 */
bool gameClockRestAllowed(bool noRestFlag, bool isInTriggerList);

#endif
