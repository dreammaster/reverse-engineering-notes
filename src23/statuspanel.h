#ifndef YENDOR23_STATUSPANEL_H
#define YENDOR23_STATUSPANEL_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "viewrender.h"

/*
 * One party member's panel on the main screen: DrawPartyMemberStatusPanel (yendor2.asm:32307, yendor3.asm:31580), with
 * DrawAfflictionIconRow and DrawStatBar. Four panels sit along the bottom, laid out by the `PartyPanels` region table
 * (uiregions.h: 8 entries per panel -- 0 the 32 x 32 face, 1-3 the left column of affliction icons, 4 the training mark, 5 the
 * protection icon, 6 the abilities icon, 7 the bar area at x = left edge, three 5-row bars 5 pixels apart).
 *
 * Drawn from a party record (party.h):
 *   - the face: category 7 picture [+0x12], opaque; a dead/stoned/frozen-style overlay (category 7, id 0xE0 in Chapter 2,
 *     1 in Chapter 3, transparent) when status [+0x1C] has any of 0x1C40 or [+0x15E] has 0x8000
 *   - bars, 38 pixels wide (statusPanelBarWidth): HP [+0x52] of [+0x92] (shown as 0 when dead), colour 0x59; MP [+0x54] of [+0x94],
 *     0xCA; carried load [+0x118] of capacity [+0x56], 0x86; background colour 6; a value above its maximum is clamped and
 *     drawn 2 colours brighter
 *   - the abilities icon (category 9): picture 0x14 if [+0xB4] (learned abilities) is nonzero else 0x15 (Chapter 3: 0xF / 0x10)
 *   - the training mark: a 'T' (font 0, colour 0xF, transparent, 2 pixels in) when alive and [+0x1E] (a pending level) is
 *     nonzero, else the blank icon 0x15 (Chapter 3: 0x10)
 *   - three affliction icons (category 9; blank = 4): [+0x1C] 0x2000 / 0x4000 / 0x8000 -> 7 / 6 / 5; 0x400 / 0x800 / 0x1000 -> 10 /
 *     9 / 8; 0x80 / 0x100 / 0x200 -> 13 / 12 / 11; and the protection icon 0xE if the nine protections [+0x20..+0x30] are not all 0
 *     (else the blank 0x15 / Chapter 3 0x10)
 *   - "DEAD" (colour 0xF, transparent) over the bars of a dead member (status 0x40)
 * An empty slot draws nothing (the frame stays).
 */
enum { StatusPanelCount = 4, StatusPanelBarWidth = 38, StatusPanelBarHeight = 5 };

/* DrawStatBar's filled width for `current` of `maximum` (0 when current <= 0). */
unsigned statusPanelBarWidth(int current, unsigned maximum);

void statusPanelDraw(const ViewRenderer *r, unsigned panel, const uint8_t *partyRecord);

#endif
