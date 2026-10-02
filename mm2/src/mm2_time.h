/* Game calendar and resting (resident advance_time 150CE, age_party_one_day 15092; ovl/2MISC party_do_rest 1CD8A). */
#ifndef MM2_TIME_H
#define MM2_TIME_H

#include "mm2_combat.h"
#include "mm2_inn.h"

/* Time as stored in the saved state. */
int mm2_day_of_year(const Mm2Roster *r);           /* 1..180 in the current era */
int mm2_year(const Mm2Roster *r);
int mm2_era(const Mm2Roster *r);
int mm2_day_fraction(const Mm2Roster *r);

/* Adds `units` to the day fraction (256 per day): a new day increments the day of the current era, ages the party, resets
 * the monthly flags on days 60/120/180 and rolls the year after day 180.  darkCell: map flag 20h of the party's cell (a
 * single unit of time then burns one unit of Light). */
void mm2_advance_time(Mm2Roster *r, int units, int darkCell);

/* Rest (R): clears timed effects, heals/restores each living member, advances 85 time units; returns 1 if the party was
 * thrown into era 9 (1 in 6 when not already there). */
int mm2_party_rest(Mm2Roster *r, const Mm2Rng *rng);

#endif
