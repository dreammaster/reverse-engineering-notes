#ifndef YENDOR23_PALETTEFADE_H
#define YENDOR23_PALETTEFADE_H

#include <stdbool.h>
#include <stdint.h>

#include "palette.h"

/*
 * StepPaletteFadeRange (yendor2.asm:38171; Chapter 3 the same), the general fader behind every fade of the opening story, the clue book and the screen
 * transitions: `rounds` (bx) rounds over `count` (cx) colours starting at colour `first` (dx), each round ending in SetPaletteRange (one DAC write of that
 * range, the caller's frame callback -- the original waits for the vertical retrace there when UI flag 0x800 of g_uiScratchFlags3 is set). The work
 * areas the original keeps in the data segment are the state: `buffer` = DS:0x412A (the "current" palette modes 0/2 fill from the DAC and 3/4 continue
 * from), `target` = DS:0x442A (the palette being faded to, loaded by LoadMasterPalette), `work` = DS:0x4D5C and `out` = DS:0x475A (modes 1 and 5).
 *
 *   mode 0  buffer = the DAC, then as mode 3
 *   mode 3  fade down: each round every component of the range above 0 loses 1; when a round changes nothing the call ends early (no write for that round)
 *   mode 1  fade in: work = target - 63 (a component below 0 shows black, kept in `out` as 0); each round a component not yet at its target gains 1 and,
 *           once it is >= 0, is copied to `out`; the range of `out` is written (so the brightest components appear first); `out` is zeroed afterwards
 *   mode 2  buffer = the DAC, then as mode 4
 *   mode 4  fade up: each round every component of the range below its target gains 1; the call ends early when nothing changed
 *   mode 6  RunPaletteFadeSequence (yendor2.asm:9738): mode 1's rounds without its initialisation and without clearing `out` -- the opening's title fade-in calls it
 *           63 times after preparing `work` once (paletteFadeInPrepare)
 *   mode 5  from white: each round every component of `work` over the range that differs from the target loses 1 (so after SetPaletteToWhite
 *           -- work all 63 -- it sinks onto the target); the range of `work` is written
 *
 * Returns the number of rounds that wrote the DAC.
 */
typedef struct {
    uint8_t dac[PaletteBytes];    /* what is on screen */
    uint8_t buffer[PaletteBytes]; /* DS:0x412A */
    uint8_t target[PaletteBytes]; /* DS:0x442A */
    uint8_t work[PaletteBytes];   /* DS:0x4D5C */
    uint8_t out[PaletteBytes];    /* DS:0x475A */
} PaletteFader;

/* Called after each SetPaletteRange with the DAC already updated. */
typedef void (*PaletteFrameFn)(void *ctx, const PaletteFader *fader, unsigned first, unsigned count);

unsigned paletteFadeRange(PaletteFader *fader, unsigned mode, unsigned rounds, unsigned count, unsigned first, PaletteFrameFn frame, void *ctx);

/* The preparation mode 1 does at its start (and PlayCharacterCreationIntroAnimation does by hand): work = target - 63 everywhere, `out` cleared. */
void paletteFadeInPrepare(PaletteFader *fader);

/* SetPaletteToWhite: work = all 63 and the whole DAC set to it. */
void paletteSetToWhite(PaletteFader *fader);

#endif
