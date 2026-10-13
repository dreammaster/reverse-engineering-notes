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
    unsigned presents, ticks, sounds, marks, escapeAfter, polls, music;
    uint8_t lastDac[768], afterTitle[768];
    bool haveAfterTitle;
    unsigned cardsSeen[9];
} Counts;

static void onPresent(void *ctx, const uint8_t *screen, const uint8_t *dac) {
    (void)screen;
    memcpy(((Counts *)ctx)->lastDac, dac, 768);
    ((Counts *)ctx)->presents++;
}

static void onMusic(void *ctx, unsigned track) {
    Counts *c = ctx;
    c->music = track;
}

static void onTick(void *ctx) {
    ((Counts *)ctx)->ticks++;
}

static void onSound(void *ctx, unsigned id) {
    (void)id;
    ((Counts *)ctx)->sounds++;
}

static void onMark(void *ctx, unsigned index, const IntroOp *op) {
    Counts *c = ctx;
    c->marks++;
    if (index == 3 && !c->haveAfterTitle && op->kind == IntroOpPoll) {
        memcpy(c->afterTitle, c->lastDac, 768); /* the poll right after the title fade-in */
        c->haveAfterTitle = true;
    }
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
    IntroAssets assets = {world, size, &exe, picture, NULL, GameYendor2};
    static IntroStory story;
    check("the story starts", introStoryStart(&story, &assets));
    Counts counts;
    memset(&counts, 0, sizeof(counts));
    IntroHost host = {&counts, onPresent, onSound, onMusic, onTick, onEscape, onMark, false, NULL, NULL};
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

    /* the opening: from black, the title fades in, music track 0x12 starts, the cells animate, and it runs into the story */
    memset(&counts, 0, sizeof(counts));
    check("the opening starts from a black screen", introOpeningStart(&story, &assets) && story.fader.dac[100] == 0 && story.fader.dac[500] == 0);
    host.soundEffectsOn = false;
    const IntroOp *opening;
    unsigned openingCount;
    opening = introOpeningScript(&openingCount);
    check("the opening script has a few dozen ops", opening && openingCount > 10 && openingCount < 30);
    finished = introPlayAll(&story, &assets, &host);
    check("the whole thing runs to the end", finished && counts.music == 0x12);
    check("the opening adds about 63 + 52 + 63 + 63 + 63 + frames to the tick count", counts.ticks > 3443 + 300 && counts.ticks < 3443 + 600);
    bool titleVisible = counts.haveAfterTitle;
    for (unsigned i = 0; titleVisible && i < 0x40 * 3; i++) {
        titleVisible = counts.afterTitle[i] == story.fader.target[i] || story.fader.target[i] == 0;
    }
    bool darkMiddle = counts.haveAfterTitle && counts.afterTitle[0x50 * 3 + 20] == 0;
    check("after the title fade-in colours 0-0x3F show the target palette while 0x50-0x7F are still dark", titleVisible && darkMiddle);
    /* the ending of Chapter 2 */
    for (unsigned t = 0; t < IntroEndingTextCount; t++) {
        const IntroCard *card = &introEndingTexts()[t].card;
        size_t at = 0x21660 + card->textOffset;
        for (unsigned line = 0; line < card->lines; line++) {
            memcpy(image + at, "X", 2);
            at += 2;
        }
    }
    unsigned endingCount;
    const IntroOp *ending = introEndingScript(&endingCount);
    unsigned texts[IntroEndingTextCount] = {0}, wipes = 0;
    for (unsigned i = 0; i < endingCount; i++) {
        if (ending[i].kind == IntroOpEndingText) {
            texts[ending[i].a]++;
        }
        wipes += ending[i].kind == IntroOpWipe;
    }
    bool eachText = true;
    for (unsigned t = 0; t < IntroEndingTextCount; t++) {
        eachText = eachText && texts[t] == 1;
    }
    check("the ending script shows each of its five text panels once and wipes once, and fades out at the end",
          eachText && wipes == 1 && ending[endingCount - 1].kind == IntroOpFadeOutAll && endingCount > 60 && endingCount < 90);
    ExeData exe2;
    exeDataOpen(&exe2, GameYendor2, image, imageSize);
    IntroAssets endAssets = {world, size, &exe2, picture, NULL, GameYendor2};
    check("the ending starts from the master palette, fully shown", introEndingStart(&story, &endAssets) && memcmp(story.fader.dac, story.fader.target, 768) == 0);
    memset(&counts, 0, sizeof(counts));
    host.soundEffectsOn = false;
    finished = introEndingPlay(&story, &endAssets, &host);
    check("it runs to the end", finished && counts.marks == endingCount);
    check("it takes about 2400 animation ticks (waits 1630, the wipe 130, the fades 650) and ends on a black screen", counts.ticks > 2300 && counts.ticks < 2500 && counts.lastDac[300] == 0 && counts.lastDac[30] == 0);
    check("its three sound events (the flash and the sequence 0x47, 0x48) play with sound effects off", counts.sounds == 3);
    memset(&counts, 0, sizeof(counts));
    host.soundEffectsOn = true;
    introEndingStart(&story, &endAssets);
    introEndingPlay(&story, &endAssets, &host);
    check("with sound effects on the last panel plays its voice instead of drawing", counts.sounds == 4);
    /* the credits: a table of two screens at DS:0xC2FB */
    static const unsigned table[] = {2, 5, 0xC400, 1, 3, 0xC420, 0xFFFF, 0, 0};
    for (unsigned i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        image[0x21660 + 0xC2FB + i * 2] = (uint8_t)(table[i] & 0xFF);
        image[0x21660 + 0xC2FB + i * 2 + 1] = (uint8_t)(table[i] >> 8);
    }
    image[0x21660 + 0xC400] = 'A';
    image[0x21660 + 0xC402] = 'B';
    memcpy(image + 0x21660 + 0xC420, "C", 2);
    ExeData exe3;
    exeDataOpen(&exe3, GameYendor2, image, imageSize);
    IntroAssets creditAssets = {world, size, &exe3, picture, NULL, GameYendor2};
    memset(&counts, 0, sizeof(counts));
    host.soundEffectsOn = false;
    host.escape = onEscape;
    check("the credits start from the master palette", introCreditsStart(&story, &creditAssets));
    finished = introCreditsPlay(&story, &creditAssets, &host);
    check("they run through both screens (music 1, about 480 ticks: the fades, 126 rounds of the pair and the 8 ticks of waiting) and end on black", finished && counts.music == 1 && counts.ticks > 400 && counts.ticks < 600 && counts.lastDac[30] == 0);
    memset(&counts, 0, sizeof(counts));
    counts.escapeAfter = 1;
    introCreditsStart(&story, &creditAssets);
    finished = introCreditsPlay(&story, &creditAssets, &host);
    check("Escape ends them early", !finished);
    memset(&counts, 0, sizeof(counts));
    host.escape = onEscape;
    check("the boot title starts", introSplashStart(&story, &creditAssets));
    finished = introSplashPlay(&story, &creditAssets, &host);
    check("it plays music 3, waits 216 ticks between two 63-round fades and ends on black", finished && counts.music == 3 && counts.ticks > 216 + 100 && counts.ticks < 216 + 300 && counts.lastDac[30] == 0);
    memset(&counts, 0, sizeof(counts));
    counts.escapeAfter = 1;
    introSplashStart(&story, &creditAssets);
    finished = introSplashPlay(&story, &creditAssets, &host);
    check("Escape skips the boot title", !finished);
    free(world);
    free(image);
}

static bool g_busy;
static unsigned g_busyChecks;

static bool onBusy(void *ctx) {
    (void)ctx;
    g_busyChecks++;
    return g_busy && g_busyChecks % 3 != 0; /* a sound that lasts for two extra passes of the pictures */
}

/* Chapter 3's opening against its own WORLD.DAT (palette blocks) and a synthetic executable image (the slide table and the texts). */
static void testOpening3(void) {
    const char *dir = getenv("YENDOR3_GAME_DIR");
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : "../../yendor3/game");
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP Chapter 3's opening (%s not found)\n", path);
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
        check("Chapter 3's WORLD.DAT reads", false);
        return;
    }
    size_t imageSize = 0x21DB0 + 0x10000;
    uint8_t *image = calloc(imageSize, 1);
    memcpy(image + 0x21DB0 + 0x7D17, "QUIT \"CREATE\"", 14);
    static const unsigned records[3][7] = {{0x43, 0x28, 0x935C, 4, 5, 2, 0x78}, {0x48, 0x49, 0x93C2, 1, 7, 3, 0xB4}, {0x49, 0x49, 0x9440, 3, 7, 5, 0x118}};
    for (unsigned r = 0; r < 3; r++) {
        for (unsigned w = 0; w < 7; w++) {
            image[0x21DB0 + 0x597C + r * 14 + w * 2] = (uint8_t)(records[r][w] & 0xFF);
            image[0x21DB0 + 0x597C + r * 14 + w * 2 + 1] = (uint8_t)(records[r][w] >> 8);
        }
        for (unsigned line = 0; line < records[r][5]; line++) {
            memcpy(image + 0x21DB0 + records[r][2] + line * 2, "X", 2);
        }
    }
    memcpy(image + 0x21DB0 + 0x9626, "X", 2);
    memcpy(image + 0x21DB0 + 0x9325, "X\0X", 4);
    ExeData exe;
    check("the synthetic Chapter 3 executable opens", exeDataOpen(&exe, GameYendor3, image, imageSize));
    IntroStory3Card card;
    check("the slide table is read from the executable", introStory3Card(&exe, 2, &card) && card.voice == 0x49 && card.voiceRepeat == 3 && card.lines == 5 && card.timer == 0x118);
    IntroAssets assets = {world, size, &exe, picture, NULL, GameYendor3};
    static IntroStory story;
    unsigned count;
    const IntroOp *ops = introOpening3Script(&count);
    unsigned slides = 0, zooms = 0, wipes = 0;
    for (unsigned i = 0; i < count; i++) {
        slides += ops[i].kind == IntroOpStory3;
        zooms += ops[i].kind == IntroOpZoom;
        wipes += ops[i].kind == IntroOpWipeRight;
    }
    check("the opening script has its three slides, one zoom and two wipes", slides == 3 && zooms == 1 && wipes == 2 && count > 80 && count < 120);
    check("it starts on a black screen with the master palette shown", introOpening3Start(&story, &assets) && story.screen[100] == 0);
    Counts counts;
    memset(&counts, 0, sizeof(counts));
    IntroHost host = {&counts, onPresent, onSound, onMusic, onTick, onEscape, onMark, false, NULL, onBusy};
    bool finished = introOpening3Play(&story, &assets, &host);
    check("it runs to the end", finished && counts.marks == count);
    check("it ends on black (colours 0-254 are dark)", ({ bool dark = true; for (unsigned i = 0; i < 255 * 3; i++) { dark = dark && counts.lastDac[i] == 0; } dark; }));
    check("it takes about 2200 animation ticks", counts.ticks > 2000 && counts.ticks < 2400);
    check("its 18 sound events play with sound effects off", counts.sounds == 18 && counts.music == 0x0B);
    memset(&counts, 0, sizeof(counts));
    host.soundEffectsOn = true;
    g_busy = true;
    introOpening3Start(&story, &assets);
    finished = introOpening3Play(&story, &assets, &host);
    check("with sound effects on the slides add their eight voices", finished && counts.sounds == 18 + 8);
    memset(&counts, 0, sizeof(counts));
    counts.escapeAfter = 5;
    host.soundEffectsOn = false;
    introOpening3Start(&story, &assets);
    finished = introOpening3Play(&story, &assets, &host);
    check("Escape ends it early", !finished && counts.polls == 5);
    /* Chapter 3's credits: a table of two screens at DS:0xA919 */
    static const unsigned table[] = {2, 5, 0xA950, 1, 3, 0xA970, 0xFFFF, 0, 0};
    for (unsigned i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        image[0x21DB0 + 0xA919 + i * 2] = (uint8_t)(table[i] & 0xFF);
        image[0x21DB0 + 0xA919 + i * 2 + 1] = (uint8_t)(table[i] >> 8);
    }
    image[0x21DB0 + 0xA950] = 'A';
    image[0x21DB0 + 0xA952] = 'B';
    memcpy(image + 0x21DB0 + 0xA970, "C", 2);
    memset(&counts, 0, sizeof(counts));
    check("Chapter 3's credits start", introCreditsStart(&story, &assets));
    finished = introCreditsPlay(&story, &assets, &host);
    check("they run through both screens and end on black", finished && counts.ticks > 300 && counts.ticks < 450 && counts.lastDac[30] == 0);
    memset(&counts, 0, sizeof(counts));
    counts.escapeAfter = 1;
    introCreditsStart(&story, &assets);
    finished = introCreditsPlay(&story, &assets, &host);
    check("Escape ends them early", !finished);
    free(world);
    free(image);
}

int main(void) {
    testScript();
    testPlay();
    testOpening3();
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}
