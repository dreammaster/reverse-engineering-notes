#ifndef YENDOR23_PALETTE_H
#define YENDOR23_PALETTE_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"

/*
 * The palette effects (all values are 6-bit VGA DAC components, 3 bytes per colour, 256 colours = 768 bytes per palette).
 * WORLD.DAT holds consecutive 768-byte palettes starting at the master palette (pictures.h, offset 0x8270A / 0x95BDA): block 0 is the
 * game palette (LoadMasterPalette; ShowIntroPicture loads block 3 for the chapter title card); loadWorldDat5 (yendor2.asm:3708) also reads block 2, a sunrise/sunset ramp (black, blue,
 * orange, pale blue ...), used by two effects:
 *
 *  - the dawn/dusk fade (AdvanceDayNightPaletteFade, yendor2.asm:28158): AdvanceGameClock starts it at 06:00 and 18:00; every call
 *    (a timer tick countdown of 91 apart) writes a sliding 32-colour window of block 2 into DAC entries 0xE0-0xFF (the sky colours)
 *    and re-computes the ambient lighting. 113 windows: at 06:00 the window starts at colour 0 and moves up one colour per step, at
 *    18:00 it starts at colour 111 and moves down (its last window starts one colour before the table, reading whatever precedes it
 *    in memory -- zeros here).
 *  - the colour cycle (AnimatePaletteCycle, :28024, every 5 timer ticks): block 2 colours 144-159 are written to DAC entries
 *    0xD0-0xDF in four phases; phase 0 as stored, phases 1-3 rotate each group of four colours left by 1, 2 and 3.
 * Both games use the same algorithms (only the block offsets differ).
 */
enum { PaletteBytes = 768, DayNightWindowBytes = 96, DayNightSteps = 113, CycleBytes = 48, CycleFirstColour = 144, CycleDacStart = 0xD0, DayNightDacStart = 0xE0 };

/* WORLD.DAT offset of palette block `block` (0 = the master palette). */
uint32_t paletteBlockOffset(GameKind game, unsigned block);

typedef struct {
    int offset; /* bytes into block 2 */
    int delta;  /* +3 (dawn) or -3 (dusk) */
    int stepsLeft;
} DayNightFade;

/* Starts the fade for the clock minute that triggered it (1080 = 18:00 dusk; anything else, 06:00, dawn). */
void dayNightFadeBegin(DayNightFade *fade, unsigned clockMinutes);

/* Produces the next window (32 colours, 96 bytes); returns true while more steps follow. */
bool dayNightFadeStep(DayNightFade *fade, const uint8_t block2[PaletteBytes], uint8_t out[DayNightWindowBytes]);

/* The 16 colours (48 bytes) for DAC entries 0xD0-0xDF in cycle phase 0-3. */
void paletteCycleFrame(const uint8_t block2[PaletteBytes], unsigned phase, uint8_t out[CycleBytes]);

/*
 * TriggerFullPaletteFadeOut / TriggerFullPaletteFadeIn (yendor2.asm:38133 / :38152) are StepPaletteFadeRange (:38171) with a round
 * count of 63 over all 256 colours; one round writes the DAC once (SetPaletteRange):
 *   fade out (mode 0): from the DAC's current palette, every nonzero component loses 1 per round (so black after 63 rounds; it stops
 *     early once nothing changes);
 *   fade in (mode 1): round r (1-63) shows each component as clamp(target - 63 + r, 0, target) of the master palette (so the brightest
 *     components appear first), reaching the master palette after round 63;
 *   modes 2/4 move the current palette up toward the master palette by 1 per round (skipping components already at or above it).
 */
enum { FadeRounds = 63 };

/* One fade-out round on a 768-byte palette; false when every component is already 0 (nothing changed). */
bool paletteFadeOutRound(uint8_t palette[PaletteBytes]);

/* The palette shown after fade-in round `round` (1-63). */
void paletteFadeInFrame(const uint8_t master[PaletteBytes], unsigned round, uint8_t out[PaletteBytes]);

/* One round of StepPaletteFadeRange modes 2/4: every component below the master's gains 1; false when none changed. */
bool paletteFadeUpRound(uint8_t palette[PaletteBytes], const uint8_t master[PaletteBytes]);

#endif
