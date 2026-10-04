/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_voc test_voc.c ../voc.c ../audio.c && ./test_voc
 *
 * Real-data checks parse every effect of WORLD.DAT in yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR /
 * YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "voc.h"

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

static void testSynthetic(void) {
    uint8_t v[64];
    memset(v, 0, sizeof(v));
    memcpy(v, "Creative Voice File\x1A", 20);
    v[20] = 26;
    v[22] = 0x0A;
    v[23] = 0x01;
    v[24] = 0x29;
    v[25] = 0x11;
    v[26] = 1; /* type 1 */
    v[27] = 6; /* length 6 = divisor + codec + 4 samples */
    v[30] = 131;
    v[31] = 0;
    v[32] = 0x80;
    v[33] = 0x90;
    v[34] = 0x70;
    v[35] = 0x80;
    v[36] = 0; /* terminator */
    VocSound s;
    check("a synthetic voice file opens", vocOpen(&s, v, 37) && s.length == 4 && s.samples == v + 32 && s.sampleRate == 8000);
    v[31] = 4;
    check("another codec is refused", !vocOpen(&s, v, 37));
    check("a bad signature is refused", !vocOpen(&s, (const uint8_t *)"Creative Voice FilX", 19));
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
    bool ok = true, rates = true;
    for (unsigned id = 1; id <= audioEffectCount(game); id++) {
        uint32_t offset, length;
        audioEffect(game, id, &offset, &length);
        uint8_t *block = malloc(length);
        VocSound sound;
        bool read = block && fseek(f, (long)offset, SEEK_SET) == 0 && fread(block, 1, length, f) == length;
        if (!read || !vocOpen(&sound, block, length)) {
            ok = false;
        } else {
            rates = rates && sound.sampleRate >= 3000 && sound.sampleRate <= 24000 && sound.length > 0;
        }
        free(block);
    }
    fclose(f);
    check(label, ok);
    check("...with sensible sample rates", rates);
}

int main(void) {
    testSynthetic();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2: every effect is a single 8-bit voice block");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3: every effect is a single 8-bit voice block");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}
