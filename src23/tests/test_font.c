/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_font test_font.c ../font.c && ./test_font
 */
#include <stdio.h>
#include <string.h>

#include "font.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void row(const uint8_t *surface, size_t stride, int x, int y, char out[7]) {
    for (int i = 0; i < 6; i++) {
        out[i] = surface[y * stride + x + i] == 1 ? '#' : '.';
    }
    out[6] = 0;
}

int main(void) {
    const uint8_t *a = fontGlyph(GameYendor2, 0, 'A');
    check("'A' of the normal font is the 5x5 capital", a && a[0] == 0x20 && a[1] == 0x50 && a[2] == 0x88 && a[3] == 0xF8 && a[4] == 0x88 && a[5] == 0);
    check("' ' is blank", memcmp(fontGlyph(GameYendor2, 0, ' '), "\0\0\0\0\0\0", 6) == 0);
    check("characters outside 0x20-0x7F and fonts past 3 have no glyph", fontGlyph(GameYendor2, 0, 0x1F) == NULL && fontGlyph(GameYendor2, 0, 0x80) == NULL &&
                                                                              fontGlyph(GameYendor2, 4, 'A') == NULL);
    check("the other three fonts differ from the first", memcmp(fontGlyph(GameYendor2, 1, 'A'), a, 6) != 0 && memcmp(fontGlyph(GameYendor2, 2, 'A'), a, 6) != 0 &&
                                                              memcmp(fontGlyph(GameYendor2, 3, 'A'), a, 6) != 0);
    check("only ':' and ';' differ between the games", memcmp(fontGlyph(GameYendor2, 0, 'A'), fontGlyph(GameYendor3, 0, 'A'), 6) == 0 &&
                                                            memcmp(fontGlyph(GameYendor2, 0, ':'), fontGlyph(GameYendor3, 0, ':'), 6) != 0 &&
                                                            memcmp(fontGlyph(GameYendor2, 3, ';'), fontGlyph(GameYendor3, 3, ';'), 6) != 0);

    uint8_t surface[20 * 12];
    memset(surface, 9, sizeof(surface));
    int next = fontDrawChar(GameYendor2, 0, surface, 20, 2, 3, 'A', 1, 0, FontOpaque);
    char text[7];
    row(surface, 20, 2, 3, text);
    check("drawing advances 6 pixels", next == 8);
    check("...row 0 of 'A': a single pixel, third from the left", strcmp(text, "..#...") == 0 && surface[3 * 20 + 2] == 0);
    row(surface, 20, 2, 6, text);
    check("...row 3 is the crossbar", strcmp(text, "#####.") == 0);
    check("...opaque mode painted the background colour inside the cell", surface[3 * 20 + 2 + 5] == 0 && surface[(3 + 5) * 20 + 2] == 0);
    check("...and left the pixels outside alone", surface[2 * 20 + 2] == 9 && surface[3 * 20 + 8] == 9 && surface[9 * 20 + 2] == 9);

    memset(surface, 9, sizeof(surface));
    fontDrawChar(GameYendor2, 0, surface, 20, 2, 3, 'A', 1, 0, FontTransparent);
    check("transparent mode keeps what is behind unset pixels", surface[3 * 20 + 2] == 9 && surface[3 * 20 + 4] == 1);

    memset(surface, 9, sizeof(surface));
    next = fontDrawString(GameYendor2, 0, surface, 20, 0, 0, "AB", 1, 0, FontOpaque);
    check("a string advances 6 per character", next == 12 && surface[0 * 20 + 2] == 1 && surface[0 * 20 + 6 + 1] == 1);

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}
