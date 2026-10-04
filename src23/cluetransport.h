#ifndef YENDOR23_CLUETRANSPORT_H
#define YENDOR23_CLUETRANSPORT_H

#include <stdbool.h>
#include <stdint.h>

#include "bcd4.h"
#include "exedata.h"
#include "game.h"
#include "viewrender.h"

/*
 * The clue book's TRANSPORTATIONS page (ShowClueBookTransportDetail yendor2.asm:6430, DrawTransportDetailRow :6455; Chapter 3 the same). The
 * four mounts are 26-byte records in the executable's data segment (Chapter 2 DS:0x77C6, Chapter 3 0x7AF4): the name (12 characters + NUL)
 * at +0, the price (Bcd4) at +0x0E, the daily uses (u16) at +0x16 and the ability mask | time word at +0x18 (mount.h). The page shows three of
 * them -- PEGASUS, GIANT EAGLE and MAGIC DRAGON; the FLYING RUG record is skipped -- on the usual clue backdrop (screen cleared to 0,
 * category 0 picture 13 / Chapter 3 picture 6 at (1, 1)), the title TRANSPORTATIONS at (6, 4) in 0xD and the navigation bar. Each block, from
 * y = 26, 74 and 122:
 *   (91, y)        the name in 0xD
 *   (91, y + 12)   VALUE: in 0x0A, the price at x = 127 in 0x8A
 *   (97, y + 21)   USES: in 0x0A, the number at x = 127 in 0x59
 *   (97, y + 30)   TIME: in 0x0A, then at x = 127 either ANYTIME in 0xCA (time word bit 2 set) or "BETWEEN        AND" in 0xD with 7P.M. at x = 175
 *                  in 0xCA, and 7A.M. under it at y + 39. (The test is on the time word's day bit: the FLYING RUG, which only flies by day,
 *                  would read ANYTIME too -- it is not on the page -- while the MAGIC DRAGON, night only, reads "BETWEEN 7P.M. AND 7A.M.".)
 */
enum { ClueMountCount = 4 };

typedef struct {
    char name[16];
    Bcd4 price;
    unsigned uses, flags;
} ClueMount;

typedef struct {
    ClueMount mounts[ClueMountCount];
    char value[12], uses[12], time[12], between[24], sevenPm[12], sevenAm[12], anytime[12], title[24];
} ClueTransportData;

bool clueTransportLoad(ClueTransportData *data, const ExeData *exe, GameKind game);

void clueTransportPageDraw(const ViewRenderer *r, const ClueTransportData *data, uint16_t navFlags);

#endif
