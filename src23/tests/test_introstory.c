/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_introstory test_introstory.c ../introstory.c ../intro2.c ../palettefade.c ../palette.c ../exedata.c ../font.c && ./test_introstory
 *
 * The run also uses yendor2/game/WORLD.DAT's palette block when present (skipped otherwise; YENDOR2_GAME_DIR overrides); the pictures are synthetic.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "introstory.h"

static int g_failureCount = 0;
static int g_skipCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static const uint8_t *picture(void *ctx, unsigned category, unsigned id) {
    (void)ctx;
    static uint8_t pixels[318 * 198];
    (void)id;
    (void)category;
    memset(pixels, 0x40, sizeof(pixels));
    return pixels;
}

typedef struct {
    unsigned presents, ticks, sounds, marks, escapeAfter, polls;
    unsigned cardsSeen[9];
} Counts;

static void onPresent(void *ctx, const uint8_t *screen, const uint8_t *dac) {
    (void)screen;
    (void)dac;
    ((Counts *)ctx)->presents++;
}

static void onTick(void *ctx) {
    ((Counts *)ctx)->ticks++;
}

static void onSound(void *ctx, unsigned id) {
    (void)id;
    ((Counts *)ctx)->sounds++;
}

static void onMark(void *ctx, unsigned index, const IntroOp *op) {
    (void)index;
    Counts *c = ctx;
    c->marks++;
    if (op->kind == IntroOpCard && op->a >= 0 && op->a < 9) {
        c->cardsSeen[op->a]++;
    }
}

static bool onEscape(void *ctx) {
    Counts *c = ctx;
    c->polls++;
    return c->escapeAfter && c->polls >= c->escapeAfter;
}

static void testScript(void) {
    unsigned count;
    const IntroOp *ops = introStoryScript(&count);
    check("the script has about a hundred and fifty ops", count > 120 && count < 200);
    unsigned cards[9] = {0}, polls = 0, badCards = 0;
    for (unsigned i = 0; i < count; i++) {
        if (ops[i].kind == IntroOpCard) {
            if (ops[i].a < 0 || ops[i].a >= 9) {
                badCards++;
            } else {
                cards[ops[i].a]++;
            }
        }
        polls += ops[i].kind == IntroOpPoll;
    }
    bool each = true;
    for (unsigned c = 0; c < 9; c++) {
        each = each && cards[c] == 1;
    }
    check("every one of the nine story cards is shown exactly once, in range", each && badCards == 0);
    check("the original polls Escape about thirty-five times", polls >= 30 && polls <= 40);
    check("it ends by fading the last card up and waiting (no poll after it)", ops[count - 1].kind == IntroOpWait && ops[count - 3].kind == IntroOpCard);
}

static void testPlay(void) {
    const char *dir = getenv("YENDOR2_GAME_DIR");
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : "../../yendor2/game");
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP the player (%s not found)\n", path);
        g_skipCount++;
        return;
    }
    fseek(f, 0, SEEK_END);
    size_t size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *world = malloc(size);
    bool read = world && fread(world, 1, size, f) == size;
    fclose(f);
    if (!read) {
        check("WORLD.DAT reads", false);
        return;
    }
    /* a synthetic executable image: the check label and the nine cards' first line (a letter each) wherever the cards point */
    size_t imageSize = 0x21660 + 0x10000;
    uint8_t *image = calloc(imageSize, 1);
    memcpy(image + 0x21660 + 0x79E5, "QUIT \"CREATE\"", 14);
    const IntroCard *cards = introCards();
    for (unsigned c = 0; c < IntroCardCount; c++) {
        size_t at = 0x21660 + cards[c].textOffset;
        for (unsigned line = 0; line < cards[c].lines; line++) {
            memcpy(image + at, "X", 2);
            at += 2;
        }
    }
    ExeData exe;
    check("the synthetic executable opens", exeDataOpen(&exe, GameYendor2, image, imageSize));
    IntroAssets assets = {world, size, &exe, picture, NULL};
    static IntroStory story;
    check("the story starts", introStoryStart(&story, &assets));
    Counts counts;
    memset(&counts, 0, sizeof(counts));
    IntroHost host = {&counts, onPresent, onSound, onTick, onEscape, onMark, false};
    bool finished = introStoryPlay(&story, &assets, &host);
    unsigned count;
    introStoryScript(&count);
    check("it runs to the end without Escape", finished && counts.marks == count);
    check("it takes about 3400 animation ticks (a minute and a half at the usual rate)", counts.ticks > 3000 && counts.ticks < 3900);
    check("with sound effects off no voice plays but the script's own sound events do", counts.sounds >= 8 && counts.sounds <= 14);
    bool allCards = true;
    for (unsigned c = 0; c < 9; c++) {
        allCards = allCards && counts.cardsSeen[c] == 1;
    }
    check("every card was reached", allCards);

    memset(&counts, 0, sizeof(counts));
    host.soundEffectsOn = true;
    introStoryStart(&story, &assets);
    introStoryPlay(&story, &assets, &host);
    unsigned withVoices = counts.sounds;
    check("with sound effects on the six voiced cards play their voices instead of text", withVoices == 11 + 6);

    memset(&counts, 0, sizeof(counts));
    counts.escapeAfter = 3;
    introStoryStart(&story, &assets);
    finished = introStoryPlay(&story, &assets, &host);
    check("Escape at the third poll ends the story there", !finished && counts.polls == 3 && counts.marks < count / 2);
    free(world);
    free(image);
}

int main(void) {
    testScript();
    testPlay();
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}
