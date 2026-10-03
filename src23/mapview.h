#ifndef YENDOR23_MAPVIEW_H
#define YENDOR23_MAPVIEW_H

#include <stdbool.h>
#include <stdint.h>

#include "party.h"
#include "savegame.h"

/*
 * The map-view tiers: UpdatePartyAverageStatTiers (yendor2.asm:19029,
 * yendor3.asm:11009, instruction-identical) and the commands they gate,
 * ShowLocalAreaMap (the "map" key, action 0x1E, yendor2.asm:31676) and
 * ToggleMapViewMode (action 0x1F, :31887). This settles the three field
 * identities earlier notes left open: party record +0x64 is the MAPPING skill,
 * +0x66 NAVIGATION and +0x58 SURVIVAL (stat indices 20, 21 and 14 of the array
 * at PartyFieldStats = 0x3C; see party.h).
 *
 * The three averages are taken over the party members in slot order, stopping
 * dead at the first empty slot and skipping any member who is Dead, Stoned,
 * Frozen or Paralyzed (PartyStatusIncapacitated); each sum is 16-bit and
 * divided by the number counted (0 if nobody counts). The Mapping average
 * also sets tier bits in the minimap word word_36C7F ("map flags" here):
 *   >= 45  0x400      >= 50  0x8000 (large-minimap capability)
 *   >= 60  0x200 (the local area map works)
 *   >= 70  0x800 (the local map prints the party's coordinates)
 *   >= 80  0x100 (the full-screen overview works)
 * The tiers are cumulative. Navigation drives RevealMapRegion's area size and
 * Survival the monster info panel's detail tiers (the panel shows more at
 * 0x37, 0x4B and 0x50).
 *
 * The same word holds the minimap's display mode (set by the Tab key handler in
 * `start`): 0x1000 hidden, 0x2000 small, 0x4000 large. Recomputing the tiers
 * first clears everything but those three mode bits (and the low byte), then
 * sets the tier bits; finally a large minimap whose 0x8000 capability was lost
 * gains the hidden bit (0x1000) and asks for a status-panel redraw -- the 0x4000
 * bit itself is not cleared (only 0x8000 is), reproduced; the Tab handler
 * clears the mode bits before setting one, so the combination is transient.
 */
typedef struct {
    uint16_t mapping;
    uint16_t navigation;
    uint16_t survival;
    unsigned counted;
} PartyStatAverages;

enum {
    MapFlagFullOverview = 0x0100,
    MapFlagLocalMap = 0x0200,
    MapFlagExtended = 0x0400,
    MapFlagCoordinates = 0x0800,
    MapFlagHidden = 0x1000,
    MapFlagSmall = 0x2000,
    MapFlagLarge = 0x4000,
    MapFlagLargeCapable = 0x8000
};

enum { MapTierExtended = 45, MapTierLargeCapable = 50, MapTierLocalMap = 60, MapTierCoordinates = 70, MapTierFullOverview = 80 };

PartyStatAverages partyAverageStatTiers(SaveGame *save);

/*
 * The new map-flags word for these averages. *redrawStatusPanel is set when a
 * large minimap was dropped (g_uiScratchFlags1 bit 0x400).
 */
uint16_t mapviewApplyTiers(uint16_t oldFlags, uint16_t mappingAverage, bool *redrawStatusPanel);

/*
 * Whether the local area map opens. g_uiScratchFlags1 bit 0x1 (the map editor's
 * call) always allows it; Chapter 3 also lets bit 0x8000 through. Otherwise
 * MapFlagLocalMap is required, else "YOUR SKILL IS NOT HIGH ENOUGH!". The
 * full-screen overview needs MapFlagFullOverview with no bypass.
 */
bool mapviewLocalMapAllowed(GameKind game, uint16_t mapFlags, uint16_t uiScratchFlags1);
bool mapviewOverviewAllowed(uint16_t mapFlags);

/*
 * The local map shows one 40 x 24 page of the world. pageIndex is what the clue
 * book's location suffix is built from ((y / 24) * 20 + x / 40); originX/Y are
 * the page's top-left world cell; markerPixelX/Y where the party's arrow is
 * drawn ((x % 40) * 8, (y % 24 + 1) * 8, the top row being the title line).
 * Facing picks the arrow picture: north 0, south 2, east 1, west 3 (the
 * original tests north, south, east in that order and defaults to west).
 */
typedef struct {
    unsigned pageIndex;
    int originX, originY;
    int markerPixelX, markerPixelY;
    unsigned arrowPicture;
} MapviewPage;

enum { MapviewPageColumns = 40, MapviewPageRows = 24, MapviewPagesAcross = 20 };

MapviewPage mapviewPage(int worldX, int worldY, uint16_t facing);

/*
 * DrawPlayerPositionMarker (:32051): the full-screen overview only covers world
 * x 160..639, y 48..239; inside it the marker is at
 * ((x - 160) / 40 * 24 + 28, (y - 48) / 24 * 20 + 23). Returns false outside.
 */
bool mapviewOverviewMarker(int worldX, int worldY, int *pixelX, int *pixelY);

#endif
