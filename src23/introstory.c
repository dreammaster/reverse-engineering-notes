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
    if (!buildBackdrop(s, a) || a->worldSize < paletteBlockOffset(GameYendor2, 3) + PaletteBytes) {
        return false;
    }
    memset(&s->fader, 0, sizeof(s->fader));
    memcpy(s->fader.target, a->worldDat + paletteBlockOffset(GameYendor2, 3), PaletteBytes);
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

static void drawCard(IntroStory *s, const IntroAssets *a, const IntroHost *h, unsigned index) {
    const IntroCard *card = &introCards()[index];
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
        fontDrawString(GameYendor2, 0, s->screen, 320, card->x, y, text, 0x93, 0, FontTransparent);
        fontDrawString(GameYendor2, 0, s->screen, 320, card->x - 1, y - 1, text, 0x98, 0, FontTransparent);
    }
    present(s, h);
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
            if (a->worldSize >= paletteBlockOffset(GameYendor2, (unsigned)op->a) + PaletteBytes) {
                memcpy(s->fader.target, a->worldDat + paletteBlockOffset(GameYendor2, (unsigned)op->a), PaletteBytes);
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
        }
    }
    return true;
}

bool introStoryPlay(IntroStory *s, const IntroAssets *a, const IntroHost *h) {
    unsigned count;
    const IntroOp *ops = introStoryScript(&count);
    return runScript(s, a, h, ops, count);
}

bool introPlayAll(IntroStory *s, const IntroAssets *a, const IntroHost *h) {
    unsigned count;
    const IntroOp *ops = introOpeningScript(&count);
    return runScript(s, a, h, ops, count) && introStoryPlay(s, a, h);
}
