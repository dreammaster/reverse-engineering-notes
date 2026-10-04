#ifndef YENDOR23_ROSTER_H
#define YENDOR23_ROSTER_H

#include <stdint.h>

#include "game.h"
#include "viewrender.h"

/*
 * The party roster screen: ShowWorldMap (yendor2.asm:50651; Chapter 3 sub_2B7AE) with DrawPartyRosterEntry (:50954), identical in
 * both games. The full-screen picture category 0 / 4 at (1, 1) holds nine slots in a 3 x 3 grid (the PartyRoster region table, four
 * entries per slot: face, name, class, check box). Every occupied slot (record level [+0x16] != 0) shows its face (category 7
 * picture [+0x12]), name and class name (font 0, colour 0xF, transparent) and a check box icon (category 9 picture 0x11, or 0x12
 * when the member is in the active party, record flag [+0x15C] & 0x800).
 */
enum { RosterSlots = 9 };

/* records[i] is party record i (NULL or level 0 = empty slot). */
void rosterDraw(const ViewRenderer *r, const uint8_t *const records[RosterSlots]);

#endif
