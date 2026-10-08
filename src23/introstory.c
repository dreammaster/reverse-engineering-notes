#include "introstory.h"

#include <string.h>

#include "font.h"
#include "palette.h"

#define SCROLL(n) {IntroOpScroll, (n), 0, 0, 0}
#define FRAME {IntroOpDrawFrame, 0, 0, 0, 0}
#define WAITDRAW(n) {IntroOpWaitDraw, (n), 0, 0, 0}
#define WAIT(n) {IntroOpWait, (n), 0, 0, 0}
#define CELLSET(i, bits) {IntroOpCellSet, (i), (int16_t)(bits), 0, 0}
#define CELLSOFF {IntroOpCellClearAll, 0, 0, 0, 0}
#define CELLCLEAR(i, bits) {IntroOpCellClear, (i), (int16_t)(bits), 0, 0}
#define FADE(mode, rounds, count, first) {IntroOpFade, (mode), (rounds), (count), (first)}
#define FADEFRAMES(mode, rounds, count, first) {IntroOpFadeFrames, (mode), (rounds), (count), (first)}
#define UP(first) {IntroOpFade16Up, (first), 0, 0, 0}
#define DOWN(first) {IntroOpFade16Down, (first), 0, 0, 0}
#define CARD(i) {IntroOpCard, (i), 0, 0, 0}
#define PICTURE(cat, id, x, y) {IntroOpPicture, (cat), (id), (x), (y)}
#define CLEAR {IntroOpClear, 0, 0, 0, 0}
#define CLEARROWS(first, rows) {IntroOpClearRows, (first), (rows), 0, 0}
#define CLEARBOX(x, y, w, h) {IntroOpClearBox, (x), (y), (w), (h)}
#define CLEARBACKDROP(first, rows) {IntroOpClearBackdropRows, (first), (rows), 0, 0}
#define PALETTE(block) {IntroOpLoadPalette, (block), 0, 0, 0}
#define FADEOUT {IntroOpFadeOutAll, 0, 0, 0, 0}
#define WHITE {IntroOpWhite, 0, 0, 0, 0}
#define SOUND(id) {IntroOpSound, (id), 0, 0, 0}
#define CAPTURE {IntroOpCapture, 0, 0, 0, 0}
#define RESET {IntroOpScrollReset, 0, 0, 0, 0}
#define POLL {IntroOpPoll, 0, 0, 0, 0}
#define PREPARE {IntroOpPrepareFadeIn, 0, 0, 0, 0}
#define TITLEFADE {IntroOpTitleFadeIn, 0, 0, 0, 0}
#define SPARK(colour, frames, dim) {IntroOpSpark, (colour), (frames), (dim), 0}
#define DOWNPAIR {IntroOpFadeDownPair, 0, 0, 0, 0}
#define MUSIC(track) {IntroOpMusic, (track), 0, 0, 0}
#define PICOP(cat, id, x, y) {IntroOpPictureOpaque, (cat), (id), (x), (y)}
#define WIPE {IntroOpWipe, 0, 0, 0, 0}
#define ENDTEXT(i) {IntroOpEndingText, (i), 0, 0, 0}

/* Cell indices (intro2.c): 4 the guard, 5 the sorcerer, 6 the door. The numbers after the semicolons are the asm addresses of the original's calls. */
static const IntroOp kStory[] = {
    RESET,                                    /* :8802 word_2E402 = word_2E406 = 0 */
    SCROLL(31), UP(0x40), SCROLL(39), POLL,   /* :8806-8821 */
    FADE(1, 63, 0x30, 0x50), SCROLL(128), POLL,
    CELLSET(4, 0x4000), WAITDRAW(7),          /* :8845 the guard starts walking */
    FADE(0, 63, 6, 0x93),                     /* the card colours go dark */
    CARD(0), POLL,                            /* the first story card */
    FADEFRAMES(4, 63, 16, 0x93), POLL,
    WAITDRAW(45), POLL,
    CELLSET(4, 0x4020), WAITDRAW(15),
    FADEFRAMES(3, 63, 0xFF, 0),               /* :8909 everything fades out while the cells run */
    CELLSOFF, POLL,
    PALETTE(0), CLEAR,                        /* :8936 */
    CARD(1), UP(0x90), WAIT(25), DOWN(0x90), POLL,
    CLEAR, PICTURE(1, 0x27, 55, 47), UP(0xC0), POLL,
    CARD(2), UP(0x90), UP(0xB0), WAIT(30), POLL,
    SOUND(0x11), DOWN(0xB0), DOWN(0x90), POLL,
    CLEAR, PICTURE(1, 0x28, 55, 47), WHITE, SOUND(0x12), FADE(5, 63, 0xFF, 0), POLL,
    SOUND(0x13), UP(0x90), DOWN(0x90), POLL,
    CARD(3), UP(0x90), CLEARROWS(7, 10), WAIT(15), DOWN(0x90), POLL,
    CARD(4), UP(0x90), SOUND(0x18), POLL,
    WAIT(25), SOUND(0x19), DOWN(0x90),
    FADE(0, 63, 0x60, 0), FADE(3, 63, 0x40, 0x80), FADE(3, 63, 0x30, 0xD0), CLEARROWS(7, 21), POLL,
    CARD(5), UP(0x90), WAIT(20), DOWN(0x90), POLL,
    CLEARROWS(7, 14),                         /* :9177 the eight item pictures */
    PICTURE(7, 0xAF, 142, 1), PICTURE(7, 0xB0, 50, 23), PICTURE(7, 0xB1, 236, 23), PICTURE(7, 0xB2, 1, 82), PICTURE(7, 0xB3, 286, 82),
    PICTURE(7, 0xB4, 50, 143), PICTURE(7, 0xB5, 236, 143), PICTURE(7, 0xB6, 142, 162), POLL,
    FADE(4, 63, 0x20, 0xA0), UP(0x30), UP(0x10), UP(0x50), UP(0x40), UP(0x90), POLL,
    UP(0x20), UP(0x80), UP(0xD0), UP(0), DOWN(0xC0), DOWN(0x60), POLL,
    WHITE, SOUND(0x12), CLEAR, POLL,
    PICTURE(1, 0x27, 55, 47), CLEARBOX(211, 82, 58, 42), POLL,
    FADE(5, 63, 0xFF, 0), WAIT(10), POLL,
    FADE(0, 63, 0xC0, 0), FADE(3, 63, 0x30, 0xD0),
    CARD(6), UP(0x90), CAPTURE, POLL,
    CELLSET(5, 0xC000), FRAME,                /* :9365 the sorcerer */
    FADE(2, 63, 0xC0, 0), FADE(2, 63, 0x30, 0xD0), POLL,
    WAIT(45), POLL,
    SOUND(0x16), POLL,
    CLEARBACKDROP(7, 30),                     /* :9402 (only with sound effects off in the original) */
    WAITDRAW(3), SOUND(0x1A), WAITDRAW(2),
    FADE(0, 63, 0xC0, 0), FADE(3, 63, 0x30, 0xD0), POLL,
    CLEAR, WAIT(3), CELLCLEAR(5, 0xC000),
    FADEOUT, PALETTE(3), POLL,                /* :9440 */
    SOUND(0x1B), UP(0x90), CARD(7), POLL,
    WAIT(20), DOWN(0x90), CLEAR, POLL,
    SOUND(0x1B), CELLSET(6, 0xC000), FRAME, CAPTURE,
    FADE(1, 63, 256, 0), WAIT(1), SOUND(5), WAITDRAW(5), POLL,
    CARD(8), UP(0x90), WAIT(20)               /* :9527 */
};

/* yendor2.asm:8626-8794. The tick flag tests before each cell frame are taken as always set. */
static const IntroOp kOpening[] = {
    PREPARE, MUSIC(0x12), TITLEFADE, POLL,       /* :8626 the title card and plaques fade in (colours 0-0x3F and 0x80-0xFF) */
    WAITDRAW(5), SPARK(0x1F, 32, 20), POLL,      /* :8674 the spark */
    WAITDRAW(10), FADEFRAMES(4, 63, 16, 0x40), POLL,
    WAITDRAW(10), FADEFRAMES(4, 63, 0x30, 0x50), POLL,
    WAITDRAW(20), DOWNPAIR
};

/*
 * yendor2.asm:2871-3063. The flourish of PlayCreditsFrameAnimation (:3120) is unrolled: category 6 pictures 0x22-0x2F at (124, 42), two ticks each. The
 * sound-driver waits (WaitForSoundDriverIdle before the voice cue and between sounds 0x47 and 0x48) are the host's; the clock set to 08:00 and the UI flag
 * writes of the original are game state, not part of the picture.
 */
#define FLOURISH(id) PICTURE(6, (id), 124, 42), WAIT(2)
static const IntroOp kEnding[] = {
    WAIT(10), MUSIC(0x14), PICOP(0, 7, 1, 1), WAIT(40),   /* :2871 the scroll backdrop */
    ENDTEXT(0), WAIT(100), WIPE,                          /* :2900 the parchment found in the chamber; the letter slides up */
    FADE(0, 20, 0x71, 0), ENDTEXT(1), WAIT(300),
    SOUND(0x0A), WHITE, PICOP(0, 9, 1, 1), WAIT(20),     /* the teleport: a flash, then the arrival at Yendor */
    FADE(5, 63, 0xFF, 0), WAIT(40), ENDTEXT(2), WAIT(350),
    FADEOUT, PICOP(0, 10, 1, 1), PICTURE(6, 0x21, 124, 42), /* the audience with Zamora */
    FADE(1, 63, 256, 0), WAIT(40), ENDTEXT(3), WAIT(350), ENDTEXT(4),
    FLOURISH(0x22), FLOURISH(0x23), FLOURISH(0x24), FLOURISH(0x25), FLOURISH(0x26), FLOURISH(0x27), FLOURISH(0x28),
    FLOURISH(0x29), FLOURISH(0x2A), FLOURISH(0x2B), FLOURISH(0x2C), FLOURISH(0x2D), FLOURISH(0x2E), FLOURISH(0x2F),
    SOUND(0x47), SOUND(0x48), WAIT(100),
    FADEOUT, CLEAR, PICOP(1, 0x64, 57, 49), WAIT(30),    /* :3010 a small emblem */
    FADE(1, 63, 256, 0), WAIT(50), FADEOUT,
    PALETTE(0), FADEOUT, PICOP(0, 0xB, 1, 1), WAIT(10),  /* :3050 the next chapter's title, in three fades */
    FADE(1, 63, 0x40, 0), WAIT(30), FADE(1, 63, 0x30, 0x40), WAIT(30), FADE(1, 63, 0x90, 0x6F), WAIT(100),
    FADEOUT
};

const IntroOp *introEndingScript(unsigned *count) {
    *count = (unsigned)(sizeof(kEnding) / sizeof(kEnding[0]));
    return kEnding;
}

/*
 * yendor3.asm:50964-51512, in the order RunCharacterCreation calls its six stages. The Escape polls (PollForEscapeKeyOnlyAlt) are POLL; the stage-level checks of
 * RunCharacterCreation are the same test. The pictures drawn "to the video buffer" are drawn to the screen, the ones drawn to the second page (53F6) are PICBACK.
 * The first three fades of the setup run over a black screen (a palette settle) and are kept as time.
 */
#define PICBACK(cat, id, x, y) {IntroOpPictureBack, (cat), (id), (x), (y)}
#define WIPER {IntroOpWipeRight, 0, 0, 0, 0}
#define WAITSND(n) {IntroOpWaitSound, (n), 0, 0, 0}
#define WORKT {IntroOpWorkFromTarget, 0, 0, 0, 0}
#define ZOOM(frames) {IntroOpZoom, (frames), 0, 0, 0}
#define PLAIN(i) {IntroOpPlainText, (i), 0, 0, 0}
#define STORY3(i) {IntroOpStory3, (i), 0, 0, 0}
#define FRAMES(first, count) {IntroOpFrames, (first), (count), 0, 0}
#define FLIP(first, count, passes, mode) {IntroOpFlip, (first), (count), (passes), (mode)}
#define SOUNDWAIT(id, ticks) SOUND(id), WAITSND(ticks), POLL
#define POLLN(times, ticks) {IntroOpPollWait, (times), (ticks), 0, 0}
#define FADELOOP(times, rounds, mode, colour) {IntroOpFadeLoop, (times), (rounds), (mode), (colour)}
static const IntroOp kOpening3[] = {
    WAIT(20), MUSIC(9), CLEAR, PALETTE(3),                              /* :51731 setup */
    FADE(0, 63, 256, 0), FADE(1, 63, 256, 0), FADE(0, 63, 256, 0),
    PICOP(0, 8, 1, 1), POLL, FADE(1, 63, 256, 0), POLL,                  /* :51690 the banner */
    POLLN(7, 5), PICBACK(0, 9, 1, 1), POLL, WIPER, POLL,                              /* :51110 the second picture wipes in */
    SOUNDWAIT(0x33, 8), SOUNDWAIT(0x34, 8), SOUNDWAIT(0x35, 8), WAIT(3),
    SOUNDWAIT(0x36, 10), SOUNDWAIT(0x37, 10), SOUNDWAIT(0x38, 10), SOUNDWAIT(0x39, 10), SOUNDWAIT(0x3A, 10),
    PICBACK(0, 10, 1, 1), POLL, WIPER, POLL,                             /* :51170 the third */
    SOUNDWAIT(0x3B, 9), SOUNDWAIT(0x3C, 9), SOUNDWAIT(0x3D, 9), SOUNDWAIT(0x3E, 9), SOUNDWAIT(0x3F, 9), SOUNDWAIT(0x40, 9), SOUNDWAIT(0x41, 9), SOUNDWAIT(0x42, 9),
    POLLN(5, 5), MUSIC(0x0A), WHITE, POLL, SOUND(0x53), POLL,                         /* :51210 the title flashes, fades in and zooms */
    PICOP(0, 11, 1, 1), FADE(5, 63, 0xFF, 0), POLL,
    POLLN(20, 5), WORKT, ZOOM(25), PLAIN(0),
    FADELOOP(31, 2, 6, 0x5F),                                           /* the copyright line fades in */
    POLLN(15, 5), FADE(0, 63, 256, 0),
    CLEAR, PALETTE(0), FADE(0, 63, 256, 0), PLAIN(1), FADE(1, 63, 256, 0), /* :51343 Zamora's request */
    POLLN(10, 5), FADE(0, 63, 256, 0), CLEAR, PREPARE, POLL,
    FLIP(0x28, 5, 7, 6), POLL,
    STORY3(0), FRAMES(0x2D, 5), SOUND(0x47), FRAMES(0x32, 23),
    STORY3(1), MUSIC(0x0B), FRAMES(0x49, 7),
    STORY3(2), WORKT, FLIP(0x49, 7, 5, 7)
};

const IntroOp *introOpening3Script(unsigned *count) {
    *count = (unsigned)(sizeof(kOpening3) / sizeof(kOpening3[0]));
    return kOpening3;
}

const IntroOp *introOpeningScript(unsigned *count) {
    *count = (unsigned)(sizeof(kOpening) / sizeof(kOpening[0]));
    return kOpening;
}

const IntroOp *introStoryScript(unsigned *count) {
    *count = (unsigned)(sizeof(kStory) / sizeof(kStory[0]));
    return kStory;
}

static void blitPicture(uint8_t *dest, const uint8_t *pixels, unsigned width, unsigned height, int x, int y, unsigned firstRow, unsigned rows) {
    for (unsigned r = 0; r < rows && firstRow + r < height; r++) {
        int dy = y + (int)r;
        if (dy < 0 || dy >= 200) {
            continue;
        }
        for (unsigned c = 0; c < width; c++) {
            int dx = x + (int)c;
            uint8_t pixel = pixels[(firstRow + r) * width + c];
            if (dx >= 0 && dx < 320 && pixel != 0xFF) {
                dest[dy * 320 + dx] = pixel;
            }
        }
    }
}

static void blitOpaque(uint8_t *dest, const uint8_t *pixels, unsigned width, unsigned height, int x, int y) {
    for (unsigned r = 0; r < height; r++) {
        int dy = y + (int)r;
        if (dy < 0 || dy >= 200) {
            continue;
        }
        for (unsigned c = 0; c < width; c++) {
            int dx = x + (int)c;
            if (dx >= 0 && dx < 320) {
                dest[dy * 320 + dx] = pixels[r * width + c];
            }
        }
    }
}

static const unsigned kPictureSize[8][2] = {{318, 198}, {210, 105}, {140, 155}, {190, 110}, {224, 74}, {224, 62}, {56, 136}, {32, 32}};

static bool buildBackdrop(IntroStory *s, const IntroAssets *a) {
    memset(s->backdrop, 0, sizeof(s->backdrop));
    const uint8_t *top = a->picture(a->pictureCtx, 0, 5), *bottom = a->picture(a->pictureCtx, 0, 6);
    if (!top || !bottom) {
        return false;
    }
    for (unsigned r = 0; r < 198; r++) {
        memcpy(s->backdrop + r * 320 + 1, top + r * 318, 318);
    }
    for (unsigned r = 0; r < 198; r++) {
        memcpy(s->backdrop + (196 + r) * 320 + 1, bottom + r * 318, 318);
    }
    return true;
}

bool introOpeningStart(IntroStory *s, const IntroAssets *a) {
    if (!introStoryStart(s, a)) {
        return false;
    }
    memset(s->fader.dac, 0, PaletteBytes);
    memset(s->fader.buffer, 0, PaletteBytes);
    return true;
}

bool introStoryStart(IntroStory *s, const IntroAssets *a) {
    memset(s->screen, 0, sizeof(s->screen));
    if (!buildBackdrop(s, a) || a->worldSize < paletteBlockOffset(a->game, 3) + PaletteBytes) {
        return false;
    }
    memset(&s->fader, 0, sizeof(s->fader));
    memcpy(s->fader.target, a->worldDat + paletteBlockOffset(a->game, 3), PaletteBytes);
    memcpy(s->fader.dac, s->fader.target, PaletteBytes);
    introCellsInit(s->cells);
    s->scrollY = 0;
    return true;
}

static void present(const IntroStory *s, const IntroHost *h) {
    if (h->present) {
        h->present(h->ctx, s->screen, s->fader.dac);
    }
}

static void tick(const IntroHost *h) {
    if (h->tick) {
        h->tick(h->ctx);
    }
}

/* DrawCharacterCreationAnimationFrame: the backdrop from scrollY, the cells over it; one animation tick. */
static void drawFrame(IntroStory *s, const IntroAssets *a, const IntroHost *h) {
    memcpy(s->screen, s->backdrop + (size_t)s->scrollY * 320, sizeof(s->screen));
    IntroCellDraw draws[IntroCellCount];
    unsigned n = introCellsFrame(s->cells, 0, s->scrollY, true, draws);
    for (unsigned i = 0; i < n; i++) {
        const uint8_t *pixels = a->picture(a->pictureCtx, draws[i].category, draws[i].picture);
        if (pixels) {
            blitPicture(s->screen, pixels, kPictureSize[draws[i].category][0], kPictureSize[draws[i].category][1], draws[i].x, draws[i].y, draws[i].firstRow,
                        draws[i].rows);
        }
    }
    present(s, h);
    tick(h);
}

typedef struct {
    IntroStory *story;
    const IntroAssets *assets;
    const IntroHost *host;
    bool frames;
} FadeCtx;

static void onFadeRound(void *ctx, const PaletteFader *fader, unsigned first, unsigned count) {
    (void)fader;
    (void)first;
    (void)count;
    FadeCtx *c = ctx;
    if (c->frames) {
        drawFrame(c->story, c->assets, c->host);
    } else {
        present(c->story, c->host);
        tick(c->host);
    }
}

static void fade(IntroStory *s, const IntroAssets *a, const IntroHost *h, unsigned mode, unsigned rounds, unsigned count, unsigned first, bool frames) {
    FadeCtx ctx = {s, a, h, frames};
    if (frames) {
        for (unsigned r = 0; r < rounds; r++) { /* the original calls the fader with one round, then a frame */
            paletteFadeRange(&s->fader, mode, 1, count, first, onFadeRound, &ctx);
        }
    } else {
        paletteFadeRange(&s->fader, mode, rounds, count, first, onFadeRound, &ctx);
    }
}

static void drawText(IntroStory *s, const IntroAssets *a, const IntroHost *h, const IntroCard *card, uint8_t foreground, uint8_t background) {
    if (h->soundEffectsOn && card->voice != IntroCardNoVoice) {
        if (h->sound) {
            h->sound(h->ctx, card->voice);
        }
        return;
    }
    for (unsigned line = 0; line < card->lines; line++) {
        char text[IntroCardLineMax];
        if (!introCardLine(a->exe, card, line, text)) {
            break;
        }
        int y = card->y + (int)line * 6;
        fontDrawString(a->game, 0, s->screen, 320, card->x, y, text, background, 0, FontTransparent);
        fontDrawString(a->game, 0, s->screen, 320, card->x - 1, y - 1, text, foreground, 0, FontTransparent);
    }
    present(s, h);
}

static void drawCard(IntroStory *s, const IntroAssets *a, const IntroHost *h, unsigned index) {
    drawText(s, a, h, &introCards()[index], 0x98, 0x93);
}

/* PlayCreditsWipeAnimation (yendor2.asm:3084): 65 steps, each the backdrop again and then the top 1 + 3n rows of picture 8 (from column 1, row 1 of it, 318 pixels
 * per row -- the last one reads the next row's first pixel, as the original does) with its bottom edge on row 198. The original waits for three ticks only in the
 * first step (its tick counter is never cleared in the loop); two ticks per step here. */
static void wipe(IntroStory *s, const IntroAssets *a, const IntroHost *h) {
    const uint8_t *back = a->picture(a->pictureCtx, 0, 7), *scroll = a->picture(a->pictureCtx, 0, 8);
    if (!back || !scroll) {
        return;
    }
    for (int step = 0; step < 65; step++) {
        blitOpaque(s->screen, back, 318, 198, 1, 1);
        int top = 198 - 3 * step, rows = 1 + 3 * step;
        for (int r = 0; r < rows; r++) {
            int dy = top + r;
            if (dy < 0 || dy >= 200) {
                continue;
            }
            for (int c = 0; c < 318; c++) {
                uint8_t pixel = scroll[(1 + r) * 318 + 1 + c];
                if (pixel != 0xFF) {
                    s->screen[dy * 320 + 1 + c] = pixel;
                }
            }
        }
        for (int t = 0; t < 2; t++) {
            present(s, h);
            tick(h);
        }
    }
}

static bool escaped(const IntroHost *h) {
    return h->escape && h->escape(h->ctx);
}

static void waitTicks(IntroStory *s, const IntroHost *h, int ticks) {
    for (int n = 0; n < ticks; n++) {
        present(s, h);
        tick(h);
    }
}

/* sub_2C376: 80 strips of four columns, from column 316 leftwards, 200 rows each, one vertical retrace per strip. */
static void wipeRight(IntroStory *s, const IntroHost *h) {
    for (int strip = 0; strip < 80; strip++) {
        int column = 316 - 4 * strip;
        for (int row = 0; row < 200; row++) {
            memcpy(s->screen + row * 320 + column, s->back + row * 320 + column, 4);
        }
        present(s, h);
        tick(h);
    }
}

/* DrawViewportSprite's layer 0x14 (yendor3.asm:47648): the 318 x 198 picture stretched to w x h at (x, y), source steps of floor(size * 1000 / w) thousandths,
 * colour 0xFF transparent. */
static void zoomSprite(uint8_t *screen, const uint8_t *picture, int x, int y, int w, int h) {
    unsigned ratioX = 318000u / (unsigned)w, ratioY = 198000u / (unsigned)h;
    uint32_t accY = 0;
    for (int row = 0; row < h; row++) {
        unsigned srcY = accY / 1000;
        uint32_t accX = 0;
        for (int col = 0; col < w; col++) {
            unsigned srcX = accX / 1000;
            uint8_t pixel = picture[srcY * 318 + srcX];
            int dx = x + col, dy = y + row;
            if (pixel != 0xFF && dx >= 0 && dx < 320 && dy >= 0 && dy < 200) {
                screen[dy * 320 + dx] = pixel;
            }
            accX += ratioX;
        }
        accY += ratioY;
    }
}

/* The loop of PlayCharacterCreationIntroAnimation (:51264): the title picture redrawn, the second picture growing from the centre, the colours dimming. */
static bool zoom(IntroStory *s, const IntroAssets *a, const IntroHost *h, int frames) {
    int x = 157, y = 98, w = 6, height = 4;
    const uint8_t *title = a->picture(a->pictureCtx, 0, 11), *art = a->picture(a->pictureCtx, 0, 12);
    for (int frame = 0; frame < frames && title && art; frame++) {
        blitOpaque(s->screen, title, 318, 198, 1, 1);
        zoomSprite(s->screen, art, x, y, w, height);
        FadeCtx ctx = {s, a, h, false};
        paletteFadeRange(&s->fader, 7, 2, 0xE0, 0, onFadeRound, &ctx);
        x -= 6;
        y -= 4;
        w += 12;
        height += 8;
        if (escaped(h)) {
            return false;
        }
    }
    return true;
}

static void drawStory3Picture(IntroStory *s, const IntroAssets *a, const IntroHost *h, unsigned id) {
    const uint8_t *pixels = a->picture(a->pictureCtx, 3, id);
    if (pixels) {
        blitOpaque(s->screen, pixels, kPictureSize[3][0], kPictureSize[3][1], 70, 40);
    }
    present(s, h);
}

/* sub_2C13C (:51545): a story slide. With sound effects off the text is written (no shadow) and its pictures loop until the slide's timer has run down by two per
 * frame; with them on the slide's sound events play one after the other, the pictures looping while each is still playing. False when Escape ended it. */
static bool story3(IntroStory *s, const IntroAssets *a, const IntroHost *h, unsigned index) {
    IntroStory3Card c;
    if (!introStory3Card(a->exe, index, &c)) {
        return true;
    }
    if (!h->soundEffectsOn) {
        memset(s->screen, 0, sizeof(s->screen));
        for (unsigned line = 0; line < c.lines; line++) {
            char text[IntroCardLineMax];
            IntroCard card = {c.textOffset, c.lines, 10, 160, IntroCardNoVoice};
            if (!introCardLine(a->exe, &card, line, text)) {
                break;
            }
            fontDrawString(a->game, 0, s->screen, 320, 10, 160 + (int)line * 6, text, 0x9B, 0, FontTransparent);
        }
        int timer = (int)c.timer;
        for (;;) {
            for (unsigned frame = 0; frame < c.frames; frame++) {
                drawStory3Picture(s, a, h, c.firstPicture + frame);
                waitTicks(s, h, 2);
                if (escaped(h)) {
                    return false;
                }
                timer -= 2;
                if (timer <= 0) {
                    return true;
                }
            }
        }
    }
    for (unsigned n = 0; n < c.voiceRepeat; n++) {
        if (h->sound) {
            h->sound(h->ctx, c.voice + n);
        }
        do {
            for (unsigned frame = 0; frame < c.frames; frame++) {
                drawStory3Picture(s, a, h, c.firstPicture + frame);
                waitTicks(s, h, 2);
            }
        } while (h->soundBusy && h->soundBusy(h->ctx));
        if (escaped(h)) {
            return false;
        }
    }
    return true;
}

/* Category 3 pictures `first` .. `first + count - 1`, `passes` times,, each with two fade rounds (mode 6 on the way in, 7 on the way out) and two ticks. */
static bool flip(IntroStory *s, const IntroAssets *a, const IntroHost *h, unsigned first, unsigned count, unsigned passes, unsigned mode) {
    FadeCtx ctx = {s, a, h, false};
    for (unsigned pass = 0; pass < passes; pass++) {
        for (unsigned i = 0; i < count; i++) {
            drawStory3Picture(s, a, h, first + i);
            paletteFadeRange(&s->fader, mode, 2, 0xFF, 0, onFadeRound, &ctx);
            waitTicks(s, h, 2);
        }
        if (mode == 7 && escaped(h)) {
            return false;
        }
    }
    return true;
}

static bool runScript(IntroStory *s, const IntroAssets *a, const IntroHost *h, const IntroOp *ops, unsigned count) {
    for (unsigned i = 0; i < count; i++) {
        const IntroOp *op = &ops[i];
        if (h->mark) {
            h->mark(h->ctx, i, op);
        }
        switch (op->kind) {
        case IntroOpPoll:
            if (h->escape && h->escape(h->ctx)) {
                return false;
            }
            break;
        case IntroOpScroll:
            for (int n = 0; n < op->a; n++) {
                s->scrollY++;
                drawFrame(s, a, h);
            }
            break;
        case IntroOpDrawFrame:
            drawFrame(s, a, h);
            break;
        case IntroOpWaitDraw:
            for (int n = 0; n < op->a; n++) {
                drawFrame(s, a, h);
            }
            break;
        case IntroOpWait:
            for (int n = 0; n < op->a; n++) {
                present(s, h);
                tick(h);
            }
            break;
        case IntroOpCellSet:
            s->cells[op->a].flags |= (uint16_t)op->b;
            break;
        case IntroOpCellClearAll:
            for (unsigned c = 0; c < IntroCellCount; c++) {
                s->cells[c].flags &= 0x3FFF;
            }
            break;
        case IntroOpCellClear:
            s->cells[op->a].flags &= (uint16_t)~op->b;
            break;
        case IntroOpFade:
            fade(s, a, h, (unsigned)op->a, (unsigned)op->b, (unsigned)op->c, (unsigned)op->d, false);
            break;
        case IntroOpFadeFrames:
            fade(s, a, h, (unsigned)op->a, (unsigned)op->b, (unsigned)op->c, (unsigned)op->d, true);
            break;
        case IntroOpFade16Up:
            fade(s, a, h, 4, 63, 16, (unsigned)op->a, false);
            break;
        case IntroOpFade16Down:
            fade(s, a, h, 3, 63, 16, (unsigned)op->a, false);
            break;
        case IntroOpCard:
            drawCard(s, a, h, (unsigned)op->a);
            break;
        case IntroOpPicture: {
            const uint8_t *pixels = a->picture(a->pictureCtx, (unsigned)op->a, (unsigned)op->b);
            if (pixels) {
                blitPicture(s->screen, pixels, kPictureSize[op->a][0], kPictureSize[op->a][1], op->c, op->d, 0, kPictureSize[op->a][1]);
            }
            present(s, h);
            break;
        }
        case IntroOpClear:
            memset(s->screen, 0, sizeof(s->screen));
            present(s, h);
            break;
        case IntroOpClearRows:
            memset(s->screen + (size_t)op->a * 320, 0, (size_t)op->b * 320);
            present(s, h);
            break;
        case IntroOpClearBox:
            for (int r = 0; r < op->d; r++) {
                memset(s->screen + (size_t)(op->b + r) * 320 + op->a, 0, (size_t)op->c);
            }
            present(s, h);
            break;
        case IntroOpClearBackdropRows:
            memset(s->backdrop + (size_t)op->a * 320, 0, (size_t)op->b * 320);
            break;
        case IntroOpLoadPalette:
            if (a->worldSize >= paletteBlockOffset(a->game, (unsigned)op->a) + PaletteBytes) {
                memcpy(s->fader.target, a->worldDat + paletteBlockOffset(a->game, (unsigned)op->a), PaletteBytes);
            }
            break;
        case IntroOpFadeOutAll:
            fade(s, a, h, 0, 63, 256, 0, false);
            break;
        case IntroOpWhite:
            paletteSetToWhite(&s->fader);
            present(s, h);
            break;
        case IntroOpSound:
            if (h->sound) {
                h->sound(h->ctx, (unsigned)op->a);
            }
            break;
        case IntroOpCapture:
            memcpy(s->backdrop, s->screen, sizeof(s->screen));
            s->scrollY = 0;
            break;
        case IntroOpScrollReset:
            s->scrollY = 0;
            break;
        case IntroOpPrepareFadeIn:
            paletteFadeInPrepare(&s->fader);
            break;
        case IntroOpTitleFadeIn: {
            FadeCtx ctx = {s, a, h, true};
            for (unsigned r = 0; r < 63; r++) {
                paletteFadeRange(&s->fader, 6, 1, 0x40, 0, NULL, NULL);
                paletteFadeRange(&s->fader, 6, 1, 0x80, 0x80, NULL, NULL);
                onFadeRound(&ctx, &s->fader, 0, 0);
            }
            memset(s->fader.out, 0, PaletteBytes);
            break;
        }
        case IntroOpSpark: {
            int x = 268, y = 8;
            uint8_t colour = (uint8_t)op->a;
            for (int n = 0; n < op->b + op->c; n++) {
                drawFrame(s, a, h);
                if (n > 0) {
                    x -= 2;
                    y += 2;
                }
                if (n >= op->b) {
                    colour--;
                }
                if (x >= 0 && x < 320 && y >= 0 && y < 200) {
                    s->screen[y * 320 + x] = colour;
                }
                present(s, h);
            }
            break;
        }
        case IntroOpFadeDownPair: {
            FadeCtx ctx = {s, a, h, true};
            for (unsigned r = 0; r < 63; r++) {
                paletteFadeRange(&s->fader, 3, 1, 16, 0x40, NULL, NULL);
                paletteFadeRange(&s->fader, 3, 1, 0x30, 0x50, NULL, NULL);
                onFadeRound(&ctx, &s->fader, 0, 0);
            }
            break;
        }
        case IntroOpMusic:
            if (h->music) {
                h->music(h->ctx, (unsigned)op->a);
            }
            break;
        case IntroOpPictureOpaque: {
            const uint8_t *pixels = a->picture(a->pictureCtx, (unsigned)op->a, (unsigned)op->b);
            if (pixels) {
                blitOpaque(s->screen, pixels, kPictureSize[op->a][0], kPictureSize[op->a][1], op->c, op->d);
            }
            present(s, h);
            break;
        }
        case IntroOpWipe:
            wipe(s, a, h);
            break;
        case IntroOpPictureBack: {
            const uint8_t *pixels = a->picture(a->pictureCtx, (unsigned)op->a, (unsigned)op->b);
            if (pixels) {
                blitOpaque(s->back, pixels, kPictureSize[op->a][0], kPictureSize[op->a][1], op->c, op->d);
            }
            break;
        }
        case IntroOpWipeRight:
            wipeRight(s, h);
            break;
        case IntroOpWaitSound:
            if (h->soundEffectsOn) {
                if (h->waitSound) {
                    h->waitSound(h->ctx);
                }
            } else {
                waitTicks(s, h, op->a);
            }
            break;
        case IntroOpWorkFromTarget:
            memcpy(s->fader.work, s->fader.target, PaletteBytes);
            memset(s->fader.out, 0, PaletteBytes);
            break;
        case IntroOpFadeStep: {
            FadeCtx ctx = {s, a, h, false};
            paletteFadeRange(&s->fader, 7, (unsigned)op->a, (unsigned)op->b, (unsigned)op->c, onFadeRound, &ctx);
            break;
        }
        case IntroOpZoom:
            if (!zoom(s, a, h, op->a)) {
                return false;
            }
            break;
        case IntroOpPlainText: {
            const IntroPlainText *plain = &introPlainTexts()[op->a];
            for (unsigned line = 0; line < plain->card.lines; line++) {
                char text[IntroCardLineMax];
                if (!introCardLine(a->exe, &plain->card, line, text)) {
                    break;
                }
                fontDrawString(a->game, 0, s->screen, 320, plain->card.x, plain->card.y + (int)line * 6, text, plain->colour, 0, FontTransparent);
            }
            present(s, h);
            break;
        }
        case IntroOpStory3:
            if (!story3(s, a, h, (unsigned)op->a)) {
                return false;
            }
            break;
        case IntroOpFrames:
            for (int n = 0; n < op->b; n++) {
                drawStory3Picture(s, a, h, (unsigned)(op->a + n));
                waitTicks(s, h, 2);
                if (escaped(h)) {
                    return false;
                }
            }
            break;
        case IntroOpPollWait:
            for (int n = 0; n < op->a; n++) {
                if (escaped(h)) {
                    return false;
                }
                waitTicks(s, h, op->b);
            }
            break;
        case IntroOpFadeLoop: {
            FadeCtx ctx = {s, a, h, false};
            for (int n = 0; n < op->a; n++) {
                paletteFadeRange(&s->fader, (unsigned)op->c, (unsigned)op->b, 1, (unsigned)op->d, onFadeRound, &ctx);
                waitTicks(s, h, 1);
            }
            break;
        }
        case IntroOpFlip:
            if (!flip(s, a, h, (unsigned)op->a, (unsigned)op->b, (unsigned)op->c, (unsigned)op->d)) {
                return false;
            }
            break;
        case IntroOpEndingText:
            drawText(s, a, h, &introEndingTexts()[op->a].card, introEndingTexts()[op->a].foreground, introEndingTexts()[op->a].background);
            break;
        }
    }
    return true;
}

bool introStoryPlay(IntroStory *s, const IntroAssets *a, const IntroHost *h) {
    unsigned count;
    const IntroOp *ops = introStoryScript(&count);
    return runScript(s, a, h, ops, count);
}

/*
 * ShowIntroPicture (yendor2.asm:2545), the title menu's R "Credits" (and what `start` shows after the ending): music track 1; picture 5 of category 0 (the stone backdrop) at (1, 1);
 * palette block 3 faded in over 63 rounds, 16 colours at 0x40 and 0x30 at 0x50 together; then the credit screens of the table at DS:0xC2FB (six bytes each: the number of lines, the
 * ticks to stay, the near pointer to the NUL-separated lines; 0xFFFF ends it): the colours 0xE0-0xEF fade down, the lower region (rows 84-193) is cleared, the lines are written at
 * (40, 87) in 0xE0 and again at (39, 86) in 0xEB (a shadow), the colours fade up, and the screen stays for its ticks (Escape ends it all at any point). Afterwards block 0 is
 * the palette again and the screen fades out. Chapter 2's; Chapter 3's routine (yendor3.asm:9324) draws picture 11 at (240, 0) and the same table in colour 0xF9 and is not scripted.
 */
static bool credits(IntroStory *s, const IntroAssets *a, const IntroHost *h) {
    if (h->music) {
        h->music(h->ctx, 1);
    }
    const uint8_t *backdrop = a->picture(a->pictureCtx, 0, 5);
    if (backdrop) {
        blitOpaque(s->screen, backdrop, 318, 198, 1, 1);
    }
    present(s, h);
    if (a->worldSize >= paletteBlockOffset(a->game, 3) + PaletteBytes) {
        memcpy(s->fader.target, a->worldDat + paletteBlockOffset(a->game, 3), PaletteBytes);
    }
    fade(s, a, h, 0, 63, 256, 0, false);
    paletteFadeInPrepare(&s->fader);
    bool stopped = escaped(h);
    if (!stopped) {
        FadeCtx ctx = {s, a, h, false};
        for (unsigned round = 0; round < 63; round++) {
            paletteFadeRange(&s->fader, 6, 1, 16, 0x40, onFadeRound, &ctx);
            paletteFadeRange(&s->fader, 6, 1, 0x30, 0x50, onFadeRound, &ctx);
        }
        memset(s->fader.out, 0, PaletteBytes);
        stopped = escaped(h);
    }
    for (unsigned at = 0xC2FB; !stopped && exeDataU16(a->exe, at) != 0xFFFF; at += 6) {
        IntroCard card = {exeDataU16(a->exe, at + 4), exeDataU16(a->exe, at), 40, 87, IntroCardNoVoice};
        unsigned ticks = exeDataU16(a->exe, at + 2);
        fade(s, a, h, 3, 63, 16, 0xE0, false);
        memset(s->screen + 84 * 320, 0, (size_t)110 * 320);
        for (unsigned line = 0; line < card.lines; line++) {
            char text[IntroCardLineMax];
            if (!introCardLine(a->exe, &card, line, text)) {
                break;
            }
            fontDrawString(a->game, 0, s->screen, 320, 40, 87 + (int)line * 6, text, 0xE0, 0, FontTransparent);
            fontDrawString(a->game, 0, s->screen, 320, 39, 86 + (int)line * 6, text, 0xEB, 0, FontTransparent);
        }
        present(s, h);
        if (escaped(h)) {
            break;
        }
        fade(s, a, h, 4, 63, 16, 0xE0, false);
        for (unsigned t = 0; t < ticks; t++) {
            waitTicks(s, h, 1);
            if (escaped(h)) {
                stopped = true;
                break;
            }
        }
    }
    fade(s, a, h, 3, 63, 16, 0xE0, false);
    fade(s, a, h, 0, 63, 256, 0, false);
    if (a->worldSize >= paletteBlockOffset(a->game, 0) + PaletteBytes) {
        memcpy(s->fader.target, a->worldDat + paletteBlockOffset(a->game, 0), PaletteBytes);
    }
    fade(s, a, h, 0, 63, 256, 0, false);
    return !stopped;
}

bool introCreditsStart(IntroStory *s, const IntroAssets *a) {
    if (a->game != GameYendor2 || a->worldSize < paletteBlockOffset(a->game, 0) + PaletteBytes) {
        return false;
    }
    memset(s->screen, 0, sizeof(s->screen));
    memset(&s->fader, 0, sizeof(s->fader));
    memcpy(s->fader.target, a->worldDat + paletteBlockOffset(a->game, 0), PaletteBytes);
    memcpy(s->fader.dac, s->fader.target, PaletteBytes);
    memcpy(s->fader.buffer, s->fader.target, PaletteBytes);
    s->scrollY = 0;
    return true;
}

bool introCreditsPlay(IntroStory *s, const IntroAssets *a, const IntroHost *h) {
    return credits(s, a, h);
}

bool introEndingStart(IntroStory *s, const IntroAssets *a) {
    if (a->worldSize < paletteBlockOffset(a->game, 0) + PaletteBytes) {
        return false;
    }
    memset(s->screen, 0, sizeof(s->screen));
    memset(&s->fader, 0, sizeof(s->fader));
    memcpy(s->fader.target, a->worldDat + paletteBlockOffset(a->game, 0), PaletteBytes);
    memcpy(s->fader.dac, s->fader.target, PaletteBytes);
    memcpy(s->fader.buffer, s->fader.target, PaletteBytes);
    s->scrollY = 0;
    return true;
}

bool introEndingPlay(IntroStory *s, const IntroAssets *a, const IntroHost *h) {
    unsigned count;
    const IntroOp *ops = introEndingScript(&count);
    return runScript(s, a, h, ops, count);
}

bool introOpening3Start(IntroStory *s, const IntroAssets *a) {
    if (a->worldSize < paletteBlockOffset(a->game, 0) + PaletteBytes) {
        return false;
    }
    memset(s->screen, 0, sizeof(s->screen));
    memset(s->back, 0, sizeof(s->back));
    memset(&s->fader, 0, sizeof(s->fader));
    memcpy(s->fader.target, a->worldDat + paletteBlockOffset(a->game, 0), PaletteBytes);
    memcpy(s->fader.dac, s->fader.target, PaletteBytes);
    memcpy(s->fader.buffer, s->fader.target, PaletteBytes);
    s->scrollY = 0;
    return true;
}

bool introOpening3Play(IntroStory *s, const IntroAssets *a, const IntroHost *h) {
    unsigned count;
    const IntroOp *ops = introOpening3Script(&count);
    return runScript(s, a, h, ops, count);
}

bool introPlayAll(IntroStory *s, const IntroAssets *a, const IntroHost *h) {
    unsigned count;
    const IntroOp *ops = introOpeningScript(&count);
    return runScript(s, a, h, ops, count) && introStoryPlay(s, a, h);
}
