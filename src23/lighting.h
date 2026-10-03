#ifndef YENDOR23_LIGHTING_H
#define YENDOR23_LIGHTING_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"

/*
 * ComputeAmbientLightingTable (yendor2.asm:41988, yendor3.asm:42334; the two are structurally identical): builds the
 * dungeon view's shading. Called whenever the view is redrawn and every few minutes by AdvanceDayNightPaletteFade.
 * Its output is a 7-entry gradient of shade deltas (zero or negative = darker; the globals word_328E6..328F2,
 * one per distance band, farthest last) which then fills a 63-entry table giving every cell of the 7-row x 9-column
 * viewport its delta (DrawPicture's per-pixel shift).
 *
 * The gradient, in order:
 *  1. a base gradient (the "ambient light"):
 *       Chapter 2: place flag word_36C79 bit 4 -> gradient A, else bit 2 -> gradient B, else bit 1 -> gradient C
 *       Chapter 3: travel flag word bit 0x8000 -> A, else 0x2000 -> C
 *       (A = -12..-6, B = -8..-2, C = -7..-1: the three fixed "indoor/underground" levels)
 *     otherwise the time of day: the 37-entry day table (start, end, 7 deltas), the first entry whose end minute is
 *     >= the clock (a clock past 1443 is reset to 0 and uses the first entry). Chapter 3 uses minute 720 (noon,
 *     brightest) instead of the clock while the travel flag 0x4000 is set. The table goes from -10..-4 at night to
 *     0 at day (Chapter 2 even +1 for 07:56-18:25; Chapter 3 stops at 0).
 *  2. light sources: a "light tier" k chosen by the first of
 *       k0 light flags 0x200 or 0x8     k1 0x400 or 0x10     k2 0x800 or 0x20
 *       k3 0x1000 or 0x40 or a lit wall tier of 3     k4 0x2000 or 0x80 or tier 2     k5 0x4000 or 0x100 or tier 1
 *     (the high bits are the carried candle/torch/lantern, the low ones spell timers; the tier is the wall torch
 *     in the 3x3 cells around the party, below) adds the table's value to each NEGATIVE delta, then caps the sum at
 *     0 (so light never brightens past the base). The adjustment table has 7 rows (one per delta) x 6 columns (k).
 *  3. wall torches: nine cells (the 3x3 around the party, row-major) carry an overlay type in a word; type 47 (0x2F)
 *     counts when the party faces north, 48 south, 49 east, 50 west. The first such cell's row gives the tier:
 *     index / 3 + 1 (1 = front row ... 3).
 */
enum { LightingGradientSize = 7, LightingViewportCells = 63, LightingNearCells = 9, LightingWallTorchBase = 47 };

typedef struct {
    uint16_t flagsA;  /* the place/travel flag word; see above */
    uint16_t flagsB;  /* the light-source flag word (Chapter 2: the same word as flagsA) */
    uint16_t clockMinutes;
    uint16_t facing;  /* SaveFacing */
} LightingInput;

/* The tier (0 = none, 1-3) of the wall torch among the nine near cells' overlay types. */
unsigned lightingWallTorchTier(const uint16_t nearOverlayTypes[LightingNearCells], uint16_t facing);

/*
 * The gradient. *clockReset is set when the clock ran past the table and the original zeroes its clock.
 */
void lightingComputeGradient(GameKind game, const LightingInput *in, unsigned wallTorchTier, int16_t out[LightingGradientSize],
                             bool *clockReset);

/* The 63-entry viewport table (row-major, 9 columns) from a gradient. */
void lightingViewportTable(const int16_t gradient[LightingGradientSize], int16_t out[LightingViewportCells]);

#endif
