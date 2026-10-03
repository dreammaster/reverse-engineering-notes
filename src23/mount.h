#ifndef YENDOR23_MOUNT_H
#define YENDOR23_MOUNT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "interact.h"
#include "savegame.h"

/*
 * The mount abilities in use: RevealMapRegion (yendor2.asm:44811; Chapter 3 identical apart from the travel
 * rule below). The four "special abilities" a party member can learn (PartyFieldAbilities bits 0x8000 PEGASUS,
 * 0x4000 GIANT EAGLE, 0x2000 FLYING RUG, 0x1000 MAGIC DRAGON; the table is described in dialogservice.h) are
 * fast-travel mounts: the ability command (action 2-5 = the member's 1st-4th learned mount, counting set bits
 * from the top) shows a box of the map around the party and lets the player click an explored cell to fly
 * there. Despite the old name this is not a weather or reveal effect.
 *
 * Use rules (mountCheck): the n-th learned mount's slot is its bit position; it needs charges left
 * (PartyFieldAbilityCharge[slot] < the mount's daily charges: 1, 2, 4, 4 -- ResetDailyAbilityCharges zeroes
 * them every new day) and the right time of day: between 07:00 and 19:00 inclusive (clock minutes 420-1140)
 * the mount's time word must have bit 2, otherwise bit 1 -- PEGASUS and GIANT EAGLE (3) fly at any time, the
 * FLYING RUG (2) only by day, the MAGIC DRAGON (1) only by night. A successful flight spends one charge.
 *
 * The box (mountRevealBox) grows with the party's average Navigation (the +0x66 tier of mapview.h):
 *   < 65: 11 x 7 cells   >= 65: 17 x 9   >= 80: 23 x 13   >= 95: 27 x 17
 * centred on the party (origin = party - (size - 1) / 2), drawn at pixel (76, 48), (52, 40), (28, 24), (12, 8).
 */
typedef struct {
    const char *name;
    uint16_t mask;
    uint16_t dailyCharges;
    uint16_t timeWord; /* bit 1: usable at night, bit 2: usable by day (the table stores mask | these) */
} MountAbility;

enum { MountCount = 4, MountDayStartMinutes = 0x1A4, MountDayEndMinutes = 0x474 };

const MountAbility *mountAbility(unsigned slot);

typedef enum {
    MountCheckOk,
    MountCheckNotAMount,  /* the command number is outside 2-5: nothing happens */
    MountCheckNotLearned, /* the party member has fewer learned mounts than that */
    MountCheckNoCharges,
    MountCheckWrongTime
} MountCheckResult;

/* ability = the command number (2-5). *slot receives the mount's table slot when learned. */
MountCheckResult mountCheck(const uint8_t *partyRecord, unsigned ability, uint16_t clockMinutes, unsigned *slot);

/* Spends one charge of the given slot after a successful flight. */
void mountSpendCharge(uint8_t *partyRecord, unsigned slot);

typedef struct {
    unsigned columns, rows;
    int pixelX, pixelY;
} MountBox;

MountBox mountRevealBox(uint16_t navigationAverage);

/* The box's top-left world cell for a party position. */
void mountBoxOrigin(const MountBox *box, int partyX, int partyY, int *originX, int *originY);

/* A click (pixels) to a world cell, or false when outside the box. Each cell is 8 x 8 pixels. */
bool mountCellFromClick(const MountBox *box, int originX, int originY, int clickX, int clickY, int *cellX, int *cellY);

/*
 * TryTravelToClickedMapCell's verdict, errorCode-style (:29270):
 *   1 outside the box (or, Chapter 3, a forbidden page)   2 unexplored   3 impassable floor/overlay type
 *   4 impassable wall type (ClassifyFloorType)   6 an interactive object (a trap, trigger...) -- the
 *   flight is refused with its own message     0 fine: the party moves there.
 * pageAttribute: the destination and origin pages' attribute byte from the per-page table (below). Chapter 3
 * refuses a flight to ANOTHER page when the origin page's attribute is 2 or the destination's is 2; Chapter 2
 * has no such check (it only hides those cells from the drawn box, see mountCellVisible).
 */
typedef enum {
    MountTravelOk = 0,
    MountTravelOutside = 1,
    MountTravelUnexplored = 2,
    MountTravelFloorBlocked = 3,
    MountTravelWallBlocked = 4,
    MountTravelInteractive = 6
} MountTravelResult;

MountTravelResult mountTravelVerdict(GameKind game, bool samePage, uint8_t originPageAttr, uint8_t destPageAttr, bool explored,
                                     bool floorTypeImpassable, bool wallTypeBlocked, InteractOutcome interact);

/*
 * Which explored cells of the box are drawn (RevealMapRegionRow): same page always; another page only when the
 * origin's attribute is 1 and the cell's is not 2.
 */
bool mountCellVisible(bool samePage, uint8_t originPageAttr, uint8_t destPageAttr);

/*
 * The per-page table (WORLD.DAT: Chapter 2 offset 0x70800, Chapter 3 0x83400; 6-byte records indexed by page
 * (y / 24) * 20 + x / 40, 120 / 140 pages; ida_scripts/dump_pagetable_offset.py): bytes 0-3 are a short
 * label (the clue book's location suffix), byte 4 an index into the clue book's name list, byte 5 the page
 * attribute 0, 1 or 2 used above (Chapter 2: 48 / 25 / 47 pages).
 */
enum { PageTableOffsetYendor2 = 0x70800, PageTableOffsetYendor3 = 0x83400, PageTableRecordSize = 6 };

uint8_t mountPageAttribute(GameKind game, const uint8_t *worldDat, size_t size, unsigned page);

#endif
