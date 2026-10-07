/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_palettefade test_palettefade.c ../palettefade.c ../palette.c && ./test_palettefade
 */
#include <stdio.h>
#include <string.h>

#include "palettefade.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static unsigned g_frames;
static unsigned g_lastFirst, g_lastCount;

static void frame(void *ctx, const PaletteFader *fader, unsigned first, unsigned count) {
    (void)ctx;
    (void)fader;
    g_frames++;
    g_lastFirst = first;
    g_lastCount = count;
}

static void setUp(PaletteFader *f) {
    memset(f, 0, sizeof(*f));
    for (unsigned i = 0; i < PaletteBytes; i++) {
        f->target[i] = (uint8_t)(i % 64);
    }
}

int main(void) {
    static PaletteFader f, g;

    setUp(&f);
    memcpy(f.dac, f.target, PaletteBytes);
    g_frames = 0;
    unsigned written = paletteFadeRange(&f, 3, 63, 16, 0x40, frame, NULL);
    f.buffer[0] = 0; /* mode 3 continues from the buffer, which was never filled: nothing above 0 -> no write */
    check("mode 3 with an empty buffer ends at once", written == 0 && g_frames == 0);

    written = paletteFadeRange(&f, 0, 63, 16, 0x40, frame, NULL);
    bool rangeBlack = true, restKept = true;
    for (unsigned i = 0; i < PaletteBytes; i++) {
        bool inRange = i >= 0x40 * 3 && i < 0x50 * 3;
        rangeBlack = rangeBlack && (!inRange || f.dac[i] == 0);
        restKept = restKept && (inRange || f.dac[i] == f.target[i]);
    }
    check("mode 0 fades only the 16 colours to black (the largest component, 47, takes 47 rounds)", rangeBlack && restKept && written == 47 && g_frames == 47 && g_lastFirst == 0x40 && g_lastCount == 16);

    setUp(&f);
    g_frames = 0;
    written = paletteFadeRange(&f, 1, 63, 256, 0, frame, NULL);
    bool equal = memcmp(f.dac, f.target, PaletteBytes) == 0;
    check("mode 1 over all colours reaches the target after 63 rounds", equal && written == 63);
    uint8_t expected[PaletteBytes];
    setUp(&g);
    paletteFadeRange(&g, 1, 20, 256, 0, NULL, NULL);
    paletteFadeInFrame(g.target, 20, expected);
    check("... and round 20 matches paletteFadeInFrame (palette.h)", memcmp(g.dac, expected, PaletteBytes) == 0);
    check("mode 1 clears its output area afterwards", g.out[5] == 0 && g.out[700] == 0);

    setUp(&f);
    written = paletteFadeRange(&f, 4, 63, 256, 0, frame, NULL);
    check("mode 4 from black: up to the target, ending early (no change) before round 63 would be needed again", memcmp(f.dac, f.target, PaletteBytes) == 0 && written == 63);
    written = paletteFadeRange(&f, 4, 10, 256, 0, frame, NULL);
    check("a finished fade writes nothing more", written == 0);

    setUp(&f);
    paletteSetToWhite(&f);
    check("white: the work area and the DAC are all 63", f.work[0] == 63 && f.dac[767] == 63);
    written = paletteFadeRange(&f, 5, 63, 256, 0, frame, NULL);
    check("mode 5 sinks from white onto the target in at most 63 rounds", memcmp(f.dac, f.target, PaletteBytes) == 0 && written == 63);

    setUp(&f);
    memcpy(f.dac, f.target, PaletteBytes);
    written = paletteFadeRange(&f, 0, 5, 300, 250, frame, NULL);
    check("a range past the end is clipped to the 256 colours", g_lastFirst == 250 && g_lastCount == 6 && written == 5);

    printf("%s\n", g_failureCount ? "FAILED" : "ALL PASSED");
    return g_failureCount ? 1 : 0;
}
