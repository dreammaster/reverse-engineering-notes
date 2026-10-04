/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_intro2 test_intro2.c ../intro2.c ../exedata.c && ./test_intro2
 *
 * The card text check reads yendor2/game/SW.EXE (skipped if absent; YENDOR2_GAME_DIR overrides the directory).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intro2.h"

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

static void testFrame(void) {
    IntroCell cells[IntroCellCount];
    IntroCellDraw draws[IntroCellCount];
    introCellsInit(cells);
    unsigned n = introCellsFrame(cells, 0, 0, false, draws);
    check("at the top only the two faces are on screen", n == 2 && draws[0].picture == 0x1A && draws[1].picture == 0x1C && draws[0].x == 0x48 && draws[0].y == 0x5E);
    check("without a tick nothing steps", cells[0].picture == 0x1A && cells[1].picture == 0x1C);
    introCellsFrame(cells, 0, 0, true, draws);
    check("a tick steps the animating cells up", cells[0].picture == 0x1B && cells[1].picture == 0x1D);
    for (int i = 0; i < 4; i++) {
        introCellsFrame(cells, 0, 0, true, draws);
    }
    check("the first face reached its last picture and the second has wrapped", cells[0].picture == 0x1F && cells[1].picture == 0x1B);
    introCellsFrame(cells, 0, 0, true, draws);
    check("a looping cell wraps to its first picture past the last", cells[0].picture == 0x1A && cells[1].picture == 0x1C);
    check("cells below the screen are not drawn", introCellsFrame(cells, 0, 0, false, draws) == 2);

    introCellsInit(cells);
    unsigned count = introCellsFrame(cells, 0, 198, false, draws);
    check("scrolled to the end the two banners and the door are in view", count == 3 && draws[0].category == 6 && draws[2].category == 2);
    bool banner = false;
    for (unsigned i = 0; i < count; i++) {
        if (draws[i].category == 6 && draws[i].picture == 0x19) {
            banner = draws[i].y == 0xED - 198 && draws[i].rows == 0x88;
        }
    }
    check("a banner is drawn whole at its scrolled position", banner);

    introCellsInit(cells);
    count = introCellsFrame(cells, 0, 100, false, draws);
    check("a face cut off at the top starts part way down the picture", count >= 1 && draws[0].y == 0 && draws[0].firstRow == 6 && draws[0].rows == 0x2F);
    introCellsInit(cells);
    count = introCellsFrame(cells, 0, 40, false, draws);
    check("a cell cut off at the bottom draws the rows that fit", count == 4 && draws[2].y == 197 && draws[2].rows == 3 && draws[0].rows == 0x2F);
    introCellsInit(cells);
    introCellsFrame(cells, 0, 142, false, draws);
    check("a face scrolled out above the screen is switched off", !(cells[0].flags & IntroCellOn) && !(cells[1].flags & IntroCellOn) && (cells[2].flags & IntroCellOn));
}

static void testStepModes(void) {
    IntroCell cells[IntroCellCount];
    IntroCellDraw draws[IntroCellCount];
    memset(cells, 0, sizeof(cells));
    /* cell 0: counts down and stops at the first picture; cell 1: counts up once and stops clearing 0x4000/0x40/0x20; cell 2: loops */
    cells[0] = (IntroCell){IntroCellOn | IntroCellAnimating, 5, 7, 2, 0, 8, 0, 8, 6};
    cells[1] = (IntroCell){IntroCellOn | IntroCellAnimating | IntroCellUp | IntroCellOnce, 5, 7, 2, 0, 8, 20, 8, 7};
    cells[2] = (IntroCell){IntroCellOn | IntroCellAnimating | IntroCellLoop, 5, 7, 2, 0, 8, 40, 8, 7};
    introCellsFrame(cells, 0, 0, true, draws);
    check("down: 6 to 5", cells[0].picture == 5);
    check("once at its last picture: stops, flags cleared", cells[1].picture == 7 && cells[1].flags == IntroCellOn);
    check("loop at its last picture: back to the first", cells[2].picture == 5);
    introCellsFrame(cells, 0, 0, true, draws);
    check("down at the first picture: stays and keeps animating", cells[0].picture == 5 && (cells[0].flags & IntroCellAnimating));
    check("a looping cell counts up again after wrapping", (cells[2].flags & IntroCellLoop) && cells[2].picture == 6);
}

static void testCards(const char *path) {
    const IntroCard *cards = introCards();
    check("nine cards", IntroCardCount == 9 && cards[8].voice == 0x1C && cards[0].voice == IntroCardNoVoice);
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP the card text (%s not found)\n", path);
        g_skipCount++;
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)size);
    bool read = data && fread(data, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    ExeData exe;
    check("SW.EXE opens", read && exeDataOpen(&exe, GameYendor2, data, (size_t)size));
    if (!read) {
        return;
    }
    unsigned total = 0;
    bool allRead = true;
    for (unsigned c = 0; c < IntroCardCount; c++) {
        for (unsigned line = 0; line < cards[c].lines; line++) {
            char text[IntroCardLineMax];
            allRead = allRead && introCardLine(&exe, &cards[c], line, text) && strlen(text) > 0;
            total++;
        }
        char text[IntroCardLineMax];
        allRead = allRead && !introCardLine(&exe, &cards[c], cards[c].lines, text);
    }
    check("every card line reads as non-empty text", allRead && total == 23);
    char first[IntroCardLineMax], last[IntroCardLineMax];
    check("the first card's last line and the last card's second line read",
          introCardLine(&exe, &cards[0], 4, first) && introCardLine(&exe, &cards[8], 1, last) && strlen(first) > 10 && strlen(last) > 10);
    free(data);
}

int main(void) {
    testFrame();
    testStepModes();
    const char *dir = getenv("YENDOR2_GAME_DIR");
    char path[512];
    snprintf(path, sizeof(path), "%s/SW.EXE", dir ? dir : "../../yendor2/game");
    testCards(path);
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}
