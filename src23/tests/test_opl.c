/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_opl test_opl.c ../opl.c ../cmfplayer.c ../cmf.c ../audio.c -lm && ./test_opl
 *
 * The real-data check renders every music track of yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "cmfplayer.h"

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

static unsigned zeroCrossings(const int16_t *s, unsigned n) {
    unsigned c = 0;
    for (unsigned i = 1; i < n; i++) {
        c += (s[i - 1] < 0) != (s[i] < 0);
    }
    return c;
}

static int peak(const int16_t *s, unsigned n) {
    int p = 0;
    for (unsigned i = 0; i < n; i++) {
        int a = s[i] < 0 ? -s[i] : s[i];
        p = a > p ? a : p;
    }
    return p;
}

static void sineChannel(Opl *opl, unsigned fnum, unsigned block, bool keyOn) {
    oplWrite(opl, 0x20, 0x21);      /* modulator: sustained, multiple 1 */
    oplWrite(opl, 0x23, 0x21);      /* carrier */
    oplWrite(opl, 0x40, 0x3F);      /* modulator silent */
    oplWrite(opl, 0x43, 0x00);      /* carrier full level */
    oplWrite(opl, 0x60, 0xF0);
    oplWrite(opl, 0x63, 0xF0);      /* attack 15: instant */
    oplWrite(opl, 0x80, 0x00);
    oplWrite(opl, 0x83, 0x00);      /* sustain level 0 dB */
    oplWrite(opl, 0xC0, 0x01);      /* additive: the carrier's own sine */
    oplWrite(opl, 0xA0, (uint8_t)(fnum & 255));
    oplWrite(opl, 0xB0, (uint8_t)((keyOn ? 0x20 : 0) | (block << 2) | (fnum >> 8)));
}

static void testTone(void) {
    Opl opl;
    oplReset(&opl, OplChipRate);
    sineChannel(&opl, 580, 4, true); /* 580 * 49716 / 2^16 = 440 Hz */
    static int16_t s[OplChipRate];
    memset(s, 0, sizeof(s));
    oplGenerate(&opl, s, OplChipRate);
    unsigned crossings = zeroCrossings(s, OplChipRate);
    check("fnum 580 block 4 sounds at 440 Hz (880 zero crossings in a second)", crossings >= 870 && crossings <= 890);
    check("a full-level carrier is loud and unclipped", peak(s, OplChipRate) > 10000 && peak(s, OplChipRate) < 32767);

    oplReset(&opl, 44100);
    sineChannel(&opl, 580, 4, true);
    static int16_t t[44100];
    memset(t, 0, sizeof(t));
    oplGenerate(&opl, t, 44100);
    crossings = zeroCrossings(t, 44100);
    check("the pitch does not depend on the output sample rate", crossings >= 870 && crossings <= 890);

    oplReset(&opl, 44100);
    sineChannel(&opl, 580, 5, true);
    memset(t, 0, sizeof(t));
    oplGenerate(&opl, t, 44100);
    crossings = zeroCrossings(t, 44100);
    check("one block up is an octave up", crossings >= 1750 && crossings <= 1770);

    oplReset(&opl, 44100);
    oplWrite(&opl, 0x40, 0x3F);
    sineChannel(&opl, 580, 4, true);
    oplWrite(&opl, 0x83, 0xF8); /* release rate 8 */
    memset(t, 0, sizeof(t));
    oplGenerate(&opl, t, 4410);
    oplWrite(&opl, 0xB0, (uint8_t)((4 << 2) | 2));
    memset(t, 0, sizeof(t));
    oplGenerate(&opl, t, 44100);
    check("after key off the note dies away", peak(t, 400) > peak(t + 44100 - 400, 400) * 20 && peak(t + 44100 - 400, 400) < 200);
}

static void testSilence(void) {
    Opl opl;
    oplReset(&opl, 44100);
    static int16_t s[4410];
    memset(s, 0, sizeof(s));
    oplGenerate(&opl, s, 4410);
    check("a chip with no key on is silent", peak(s, 4410) == 0);
}

static void testPlayer(void) {
    uint8_t t[128];
    memset(t, 0, sizeof(t));
    memcpy(t, "CTMF", 4);
    t[4] = 1;
    t[5] = 1;
    t[6] = 40;
    t[8] = 56;
    t[10] = 48;
    t[12] = 96;
    t[36] = 1;
    t[38] = 120;
    static const uint8_t patch[16] = {0x21, 0x21, 0x3F, 0x00, 0xF0, 0xF0, 0x0A, 0x0A, 0, 0, 0x01, 0, 0, 0, 0, 0};
    memcpy(t + 40, patch, 16);
    /* note 69 on at once with full velocity, off after 96 ticks (one second at 120 bpm and 48 ticks per quarter), end */
    static const uint8_t music[] = {0x00, 0xC0, 0x00, 0x00, 0x90, 0x45, 0x7F, 0x60, 0x80, 0x45, 0x40, 0x00, 0xFF, 0x2F, 0x00};
    memcpy(t + 56, music, sizeof(music));
    CmfPlayer player;
    check("a synthetic track starts", cmfPlayerStart(&player, t, 56 + sizeof(music), 44100));
    static int16_t s[44100 * 4];
    unsigned got = cmfPlayerRender(&player, s, 44100 * 4);
    check("it ends after the note and its release tail (about three seconds)", got > 44100 * 2 && got < 44100 * 4);
    unsigned crossings = zeroCrossings(s + 2000, 40000);
    double hz = crossings / 2.0 / (40000.0 / 44100.0);
    check("MIDI note 69 plays at 440 Hz", hz > 430 && hz < 450);
    check("the note is silent well after its end", peak(s + 44100 * 2 + 20000, 2000) < 300);
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP %s (%s not found)\n", label, path);
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
        check(label, false);
        return;
    }
    unsigned tracks = audioMusicTrackCount(game), played = 0, silent = 0, clipped = 0;
    static CmfPlayer player;
    static int16_t pcm[44100 * 8];
    for (unsigned id = 1; id <= tracks; id++) {
        uint32_t offset, length;
        if (!audioMusicTrack(game, id, &offset, &length) || offset + length > size || !cmfPlayerStart(&player, world + offset, length, 44100)) {
            continue;
        }
        unsigned got = cmfPlayerRender(&player, pcm, 44100 * 8);
        int p = peak(pcm, got);
        played++;
        silent += p < 50;
        clipped += p >= 32767;
    }
    printf("     %s: %u tracks rendered, %u silent, %u clipped\n", label, played, silent, clipped);
    check(label, played == tracks && clipped == 0 && silent <= 2);
    free(world);
}

int main(void) {
    testTone();
    testSilence();
    testPlayer();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2: every music track renders (8 seconds each)");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3: every music track renders (8 seconds each)");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}
