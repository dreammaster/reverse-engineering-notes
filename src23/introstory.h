#ifndef YENDOR23_INTROSTORY_H
#define YENDOR23_INTROSTORY_H

#include <stdbool.h>
#include <stdint.h>

#include "exedata.h"
#include "intro2.h"
#include "palettefade.h"

/*
 * Chapter 2's story cinematic as a script and a player (RunCharacterCreationSelectionStep, yendor2.asm:8800-9543; the part after the title card fades in,
 * see intro2.h and file-formats.md "The opening story"). The script is the original's calls in order, one op each; the player executes it against a small host
 * interface, so an engine only supplies the pixels-to-screen, sound and timing. Escape polls of the original (`PollForEscapeKeyOnlyAlt` after every stage)
 * are one `host->escape` check before every op that follows one in the original; a true result ends the story.
 *
 * Screens: a tall `backdrop` (pictures 5 and 6 of category 0, intro2.h) of which 200 rows from `scrollY` are copied to the `screen` by IntroOpDrawFrame, with the
 * animated cells drawn over; every other drawing op (cards, pictures, clears) writes to the `screen` directly, so what it drew disappears at the next frame copy,
 * exactly as in the original. The palette is the PaletteFader of palettefade.h: `target` the palette being faded to (WORLD.DAT block 3 or 0), `dac` what shows.
 */
typedef enum {
    IntroOpScroll,       /* a rows: scroll the view down one row and draw a frame, a times */
    IntroOpDrawFrame,
    IntroOpWaitDraw,     /* a ticks, a cell frame per tick */
    IntroOpWait,         /* a ticks, no drawing */
    IntroOpCellSet,      /* cell a: flags |= b */
    IntroOpCellClearAll, /* every cell: flags &= 0x3FFF */
    IntroOpCellClear,    /* cell a: flags &= ~b */
    IntroOpFade,         /* mode a, b rounds, c colours from d */
    IntroOpFadeFrames,   /* the same, one cell frame per round */
    IntroOpFade16Up,     /* 63 rounds over 16 colours from a (RunPaletteRange16FadeUp) */
    IntroOpFade16Down,
    IntroOpCard,         /* story card a (intro2.h) */
    IntroOpPicture,      /* category a, picture b at (c, d), colour 0xFF transparent, into the screen */
    IntroOpClear,        /* the screen to 0 */
    IntroOpClearRows,    /* rows a .. a + b - 1 of the screen to 0 */
    IntroOpClearBox,     /* the box at x a, y b, width c, height d of the screen to 0 */
    IntroOpClearBackdropRows, /* the same on the backdrop's first 200 rows */
    IntroOpLoadPalette,  /* the target palette = WORLD.DAT palette block a */
    IntroOpFadeOutAll,   /* 63 rounds of mode 0 over all 256 colours */
    IntroOpWhite,        /* the DAC and the fader's work area to white */
    IntroOpSound,        /* sound event a */
    IntroOpCapture,      /* the screen becomes the backdrop's first 200 rows and the scroll goes back to 0 */
    IntroOpScrollReset,
    IntroOpPoll,         /* the original's Escape poll: ends the story when the host says Escape */
    IntroOpPrepareFadeIn, /* the fader's work area = target - 63 (PlayCharacterCreationIntroAnimation's first loop) */
    IntroOpTitleFadeIn,  /* 63 rounds of RunPaletteFadeSequence over colours 0-0x3F and 0x80-0xFF, a cell frame each */
    IntroOpSpark,        /* a spark of colour a runs diagonally for b frames from (268, 8), 2 pixels down-left per frame; c frames more dimming 1 per frame */
    IntroOpFadeDownPair, /* 63 rounds of: 16 colours at 0x40 down, 0x30 colours at 0x50 down, a cell frame */
    IntroOpMusic         /* music track a */
} IntroOpKind;

typedef struct {
    IntroOpKind kind;
    int16_t a, b, c, d;
} IntroOp;

/* The ops of the story (from the point where the title card fades in is `introOpeningScript`; this is the story proper) and their number. */
const IntroOp *introStoryScript(unsigned *count);

/* PlayCharacterCreationIntroAnimation (yendor2.asm:8626), the part before the story: the title card fades in, a spark crosses it, the flags and plaques light up. */
const IntroOp *introOpeningScript(unsigned *count);

typedef struct {
    void *ctx;
    /* the picture or the palette changed (a frame, a fade round, a wait tick): show `screen` through `dac` */
    void (*present)(void *ctx, const uint8_t *screen, const uint8_t *dac);
    void (*sound)(void *ctx, unsigned id);
    void (*music)(void *ctx, unsigned track);
    /* one animation tick has passed (the host may sleep) */
    void (*tick)(void *ctx);
    /* true when the player pressed Escape */
    bool (*escape)(void *ctx);
    /* called before op `index` runs, for tools that want stills */
    void (*mark)(void *ctx, unsigned index, const IntroOp *op);
    bool soundEffectsOn; /* with sound effects on a card plays its voice and draws no text */
} IntroHost;

typedef struct {
    uint8_t backdrop[320 * (196 + 198 + 4)];
    uint8_t screen[320 * 200];
    PaletteFader fader;
    IntroCell cells[IntroCellCount];
    int scrollY;
} IntroStory;

typedef struct {
    const uint8_t *worldDat; /* palette blocks */
    size_t worldSize;
    const ExeData *exe;      /* the card text */
    /* picture pixels (category, id) as in viewrender.h */
    const uint8_t *(*picture)(void *ctx, unsigned category, unsigned id);
    void *pictureCtx;
} IntroAssets;

/* Builds the backdrop and the cells and starts from a fully visible block-3 palette; false when a picture or the palette is missing. */
bool introStoryStart(IntroStory *story, const IntroAssets *assets);

/* The same, but with the screen black as it is when the opening begins (the title then fades in with `introOpeningScript`). */
bool introOpeningStart(IntroStory *story, const IntroAssets *assets);

/* Runs the story script; returns false when the host's escape ended it early. */
bool introStoryPlay(IntroStory *story, const IntroAssets *assets, const IntroHost *host);

/* Runs the opening script, then (when it was not stopped) the story one. */
bool introPlayAll(IntroStory *story, const IntroAssets *assets, const IntroHost *host);

#endif
