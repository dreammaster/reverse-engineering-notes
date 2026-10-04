/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_textpanel test_textpanel.c ../textpanel.c ../font.c ../bcd4.c ../viewrender.c ../random.c ../pictures.c ../worldmap.c && ./test_textpanel
 */
#include <stdio.h>
#include <string.h>

#include "textpanel.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static bool anyColour(const uint8_t *screen, int x0, int y0, int x1, int y1, uint8_t colour) {
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            if (screen[y * 320 + x] == colour) {
                return true;
            }
        }
    }
    return false;
}

int main(void) {
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, NULL, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    textPanelClear(&r, false);
    check("outside combat the header band and the message area are colour 4", screen[87 * 320 + 241] == 4 && screen[92 * 320 + 312] == 4 && screen[93 * 320 + 241] == 0xEE &&
                                                                                 screen[96 * 320 + 240] == 4 && screen[155 * 320 + 311] == 4 && screen[156 * 320 + 240] == 0xEE);
    memset(screen, 0xEE, sizeof(screen));
    textPanelClear(&r, true);
    check("in combat one block (240, 86) 73 x 109", screen[86 * 320 + 240] == 4 && screen[194 * 320 + 312] == 4 && screen[195 * 320 + 240] == 0xEE && screen[85 * 320 + 240] == 0xEE &&
                                                        screen[100 * 320 + 313] == 0xEE);
    memset(screen, 0xEE, sizeof(screen));
    const char *lines[2] = {"LOCKED", "NO KEY"};
    textPanelMessage(&r, lines, 2, 0x8A);
    check("message lines: 6 pixels apart from (240, 96), transparent", anyColour(screen, 240, 96, 280, 102, 0x8A) && anyColour(screen, 240, 102, 280, 108, 0x8A) &&
                                                                           !anyColour(screen, 240, 108, 280, 114, 0x8A));
    memset(screen, 0xEE, sizeof(screen));
    Bcd4 gold = {0x00, 0x01, 0x23, 0x45};
    textPanelGold(&r, gold);
    check("the gold HUD: '$' at (241, 87) and the number from x = 247, on colour 4", anyColour(screen, 241, 87, 247, 93, 0x8A) && anyColour(screen, 247, 87, 290, 93, 0x8A) && screen[87 * 320 + 300] == 4);
    textPanelHeader(&r, "YENDOR");
    check("the header text is opaque over colour 4", screen[87 * 320 + 241] == 4 || screen[87 * 320 + 241] == 0x8A);

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}
