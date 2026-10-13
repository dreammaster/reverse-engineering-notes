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
    IntroOpMusic,        /* music track a */
    IntroOpPictureOpaque, /* the same as IntroOpPicture but a plain copy (colour 0xFF is drawn too) */
    IntroOpWipe,         /* PlayCreditsWipeAnimation: picture 8 of category 0 slides up from the bottom edge over picture 7, 65 steps */
    IntroOpEndingText,   /* ending text panel a (intro2.h, introEndingTexts) */
    /* Chapter 3's opening (introOpening3Script) */
    IntroOpPictureBack,  /* category a, picture b at (c, d) into the back page, opaque */
    IntroOpWipeRight,    /* the back page onto the screen in 80 strips of 4 columns from the right edge, one retrace each (sub_2C376) */
    IntroOpWaitSound,    /* WaitForSoundDriverThenTicks: with sound effects on the host waits for the sound to end, off: a ticks */
    IntroOpWorkFromTarget, /* the fader's work area = the target palette, `out` cleared (the opening's zoom starts from the full palette) */
    IntroOpFadeStep,     /* mode 7: a rounds, b colours from c (FadePaletteStep) */
    IntroOpZoom,         /* a frames of the title zoom: picture 11 redrawn, picture 12 of category 0 scaled up from the screen centre, a fade step, an Escape poll each */
    IntroOpPlainText,    /* plain text line(s) a (intro2.h, introPlainTexts) */
    IntroOpStory3,       /* story slide a (introStory3Card); polls Escape inside */
    IntroOpFrames,       /* sub_2C11E: category 3 pictures a .. a + b - 1 at (70, 40), two ticks and an Escape poll each */
    IntroOpPollWait,     /* a times: an Escape poll, then b ticks */
    IntroOpFadeLoop,     /* a times: b rounds of fade mode c over the one colour d, then one tick */
    IntroOpFlip          /* category 3 pictures a .. a + b - 1 at (70, 40), c passes, each picture followed by two rounds of fade mode d (6 or 7) and two ticks; mode 7 polls per pass */
} IntroOpKind;

typedef struct {
    IntroOpKind kind;
    int16_t a, b, c, d;
} IntroOp;

/* The ops of the story (from the point where the title card fades in is `introOpeningScript`; this is the story proper) and their number. */
const IntroOp *introStoryScript(unsigned *count);

/* PlayCharacterCreationIntroAnimation (yendor2.asm:8626), the part before the story: the title card fades in, a spark crosses it, the flags and plaques light up. */
const IntroOp *introOpeningScript(unsigned *count);

/*
 * Chapter 2's ending (PlayStudioCreditsIntro, yendor2.asm:2871-3063, reached from `start` when the party has read the Quartz Chamber's parchment -- three global
 * flags 0x69-0x6B -- and clicked the toolbar icon): the parchment found in the chamber, the letter from the wizards' followers, the arrival at Yendor, the talk with
 * Zamora, and a closing picture of the next chapter's title. introEndingStart begins from the in-game master palette (WORLD.DAT block 0) fully shown.
 */
const IntroOp *introEndingScript(unsigned *count);

/*
 * Chapter 3's opening (RunCharacterCreation, yendor3.asm:50964-51512): the banner fades in; two pictures wipe in from the right with sound effects 0x33-0x3A and
 * 0x3B-0x42; the title flashes white, fades in and zooms up while the colours darken; Zamora's request (three story slides of category 3 pictures, with voices or text)
 * and a last fade. The conditional waits on the music state of the original (stop, wait 20 ticks) are taken as "music was playing".
 */
const IntroOp *introOpening3Script(unsigned *count);

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
    /* Chapter 3: block until the last sound event has finished (WaitForSoundDriverIdle); NULL = nothing to wait for */
    void (*waitSound)(void *ctx);
    /* Chapter 3: true while a sound event is still playing (word_2E494 != 0); NULL = never */
    bool (*soundBusy)(void *ctx);
} IntroHost;

typedef struct {
    uint8_t backdrop[320 * (196 + 198 + 4)];
    uint8_t screen[320 * 200];
    uint8_t back[320 * 200]; /* the second page (Chapter 3's PlayCharacterCreationOpeningSequence draws there and wipes it onto the screen) */
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
    GameKind game;           /* GameYendor2 (the default value) or GameYendor3: palette blocks and fonts */
} IntroAssets;

/* Builds the backdrop and the cells and starts from a fully visible block-3 palette; false when a picture or the palette is missing. */
bool introStoryStart(IntroStory *story, const IntroAssets *assets);

/* The same, but with the screen black as it is when the opening begins (the title then fades in with `introOpeningScript`). */
bool introOpeningStart(IntroStory *story, const IntroAssets *assets);

bool introEndingStart(IntroStory *story, const IntroAssets *assets);

/* The credits (the title menu's R and what follows Chapter 2's ending): the table of credit screens is read from the executable (Chapter 2 DS:0xC2FB, Chapter 3 DS:0xA919 with its own picture and layout). */
bool introCreditsStart(IntroStory *story, const IntroAssets *assets);
bool introCreditsPlay(IntroStory *story, const IntroAssets *assets, const IntroHost *host);

/* The boot title: Chapter 2's (PlayTitleScreenSequence) is the logo picture for 216 ticks over music track 3; Chapter 3's an animated logo (its VESA logo excepted). */
bool introSplashStart(IntroStory *story, const IntroAssets *assets);
bool introSplashPlay(IntroStory *story, const IntroAssets *assets, const IntroHost *host);

/* Chapter 3's opening: starts from the master palette (block 0) shown on a black screen. */
bool introOpening3Start(IntroStory *story, const IntroAssets *assets);
bool introOpening3Play(IntroStory *story, const IntroAssets *assets, const IntroHost *host);
bool introEndingPlay(IntroStory *story, const IntroAssets *assets, const IntroHost *host);

/* Runs the story script; returns false when the host's escape ended it early. */
bool introStoryPlay(IntroStory *story, const IntroAssets *assets, const IntroHost *host);

/* Runs the opening script, then (when it was not stopped) the story one. */
bool introPlayAll(IntroStory *story, const IntroAssets *assets, const IntroHost *host);

#endif
